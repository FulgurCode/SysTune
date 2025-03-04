#include "option/wifi.h"
#include <adwaita.h>
#include <gio/gio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gtk/gtkwidget.h>
#include <stdio.h>

GtkWidget *WifiPage;

typedef struct {
  GtkListBox *list_box;
  GCancellable *cancellable;
} ScanData;

typedef struct {
  GtkStack *stack;
  GtkBuilder *builder;
  GtkWidget *wifi_switch;
  GtkListBox *wifi_list;
} InitData;

// Forward declarations for all functions
static void update_wifi_networks_list(GtkListBox *list_box);
static void clear_list_box(GtkListBox *list_box);
static GtkWidget *create_network_row(const char *ssid, int signal_strength,
                                     gboolean is_secured);
static const char *get_signal_icon_name(int signal_strength);
static void on_wifi_switch_active(GObject *wifi_switch, GParamSpec *pspec,
                                  gpointer user_data);
static gboolean get_wifi_status(void);
static void set_wifi_switch_state(GtkWidget *wifi_switch, gboolean is_active);
static void toggle_wifi(gboolean enable);
static void wifi_to_stack(GtkStack *stack);
static gboolean on_refresh_timeout(gpointer user_data);
static void refresh_wifi_networks(GtkListBox *list_box);
static void on_row_clicked(GtkGestureClick *gesture, gint n_press, gdouble x,
                           gdouble y, gpointer user_data);
static void connect_to_network(const char *ssid, gboolean is_secured);
static void show_password_dialog(const char *ssid);
static void connect_with_password(const char *ssid, const char *password);
static void on_password_dialog_response(GtkDialog *dialog, gint response_id,
                                        gpointer user_data);
static void update_current_network(GtkBuilder *builder);

// Global variables
static guint refresh_timeout_id = 0;

// Function to complete initialization after all async operations
// static void complete_initialization(InitData *init_data) {
//     if (!gtk_widget_get_parent(WifiPage)) {
//         gtk_stack_add_named(init_data->stack, WifiPage, "wifi_page");
//     }
//     gtk_widget_set_visible(WifiPage, TRUE);
//     g_object_unref(init_data->builder);

//     // Set up periodic updates
//     if (refresh_timeout_id == 0) {
//         refresh_timeout_id = g_timeout_add_seconds(30, on_refresh_timeout,
//         init_data->wifi_list);
//     }

//     // Start initial WiFi network scan
//     refresh_wifi_networks(init_data->wifi_list);

//     g_free(init_data);
// }
static void complete_initialization(InitData *init_data) {
  // Only set up periodic updates
  if (refresh_timeout_id == 0) {
    refresh_timeout_id =
        g_timeout_add_seconds(30, on_refresh_timeout, init_data->wifi_list);
  }

  g_object_unref(init_data->builder);
  g_free(init_data);
}

// Async WiFi scan operation
static void wifi_scan_thread(GTask *task, gpointer source_object,
                             gpointer task_data, GCancellable *cancellable) {
  GError *error = NULL;
  gchar *output = NULL;

  if (!g_spawn_command_line_sync("nmcli device wifi rescan", NULL, NULL, NULL,
                                 &error)) {
    g_task_return_error(task, error);
    return;
  }

  if (!g_spawn_command_line_sync(
          "nmcli -t -f SSID,SIGNAL,SECURITY device wifi list", &output, NULL,
          NULL, &error)) {
    g_task_return_error(task, error);
    return;
  }

  g_task_return_pointer(task, output, g_free);
}

static void wifi_scan_complete(GObject *source_object, GAsyncResult *result,
                               gpointer user_data) {
  GError *error = NULL;
  gchar *output;
  ScanData *scan_data = user_data;

  output = g_task_propagate_pointer(G_TASK(result), &error);
  if (error) {
    g_warning("WiFi scan failed: %s", error->message);
    g_error_free(error);
    goto cleanup;
  }

  clear_list_box(scan_data->list_box);

  gchar **lines = g_strsplit(output, "\n", -1);
  for (int i = 0; lines[i] != NULL && *lines[i] != '\0'; i++) {
    gchar **fields = g_strsplit(lines[i], ":", -1);
    if (g_strv_length(fields) >= 3) {
      const char *ssid = fields[0];
      int signal_strength = atoi(fields[1]);
      gboolean is_secured = (fields[2] != NULL && *fields[2] != '\0');

      if (strlen(ssid) > 0) {
        GtkWidget *row = create_network_row(ssid, signal_strength, is_secured);
        gtk_list_box_append(scan_data->list_box, row);
      }
    }
    g_strfreev(fields);
  }
  g_strfreev(lines);
  g_free(output);

cleanup:
  g_free(scan_data);
}

// Async WiFi toggle operation
static void wifi_toggle_thread(GTask *task, gpointer source_object,
                               gpointer task_data, GCancellable *cancellable) {
  gboolean enable = GPOINTER_TO_INT(task_data);
  GError *error = NULL;
  gchar *command =
      g_strdup_printf("nmcli radio wifi %s", enable ? "on" : "off");

  if (!g_spawn_command_line_sync(command, NULL, NULL, NULL, &error)) {
    g_free(command);
    g_task_return_error(task, error);
    return;
  }

  g_free(command);
  g_task_return_boolean(task, TRUE);
}

static void wifi_toggle_complete(GObject *source_object, GAsyncResult *result,
                                 gpointer user_data) {
  GError *error = NULL;
  GtkWidget *wifi_switch = user_data;

  if (!g_task_propagate_boolean(G_TASK(result), &error)) {
    g_warning("Failed to toggle WiFi: %s", error->message);
    g_error_free(error);

    gboolean current_state;
    g_object_get(wifi_switch, "active", &current_state, NULL);
    g_object_set(wifi_switch, "active", !current_state, NULL);
  }
}

static const char *get_signal_icon_name(int signal_strength) {
  if (signal_strength > 80) {
    return "network-wireless-signal-excellent-symbolic";
  } else if (signal_strength > 55) {
    return "network-wireless-signal-good-symbolic";
  } else if (signal_strength > 30) {
    return "network-wireless-signal-ok-symbolic";
  } else {
    return "network-wireless-signal-weak-symbolic";
  }
}
// Modify create_network_row to add click gesture
static GtkWidget *create_network_row(const char *ssid, int signal_strength,
                                     gboolean is_secured) {
  AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), ssid);
  adw_action_row_set_subtitle(row,
                              is_secured ? "Secured with WPA" : "Open Network");

  gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);

  // Add click gesture to the row
  GtkGesture *click = gtk_gesture_click_new();
  g_signal_connect(click, "pressed", G_CALLBACK(on_row_clicked), row);
  gtk_widget_add_controller(GTK_WIDGET(row), GTK_EVENT_CONTROLLER(click));

  // Print debug info
  g_print("DEBUG: Created row for network: %s\n", ssid);

  GtkImage *signal_icon = GTK_IMAGE(
      gtk_image_new_from_icon_name(get_signal_icon_name(signal_strength)));
  gtk_widget_set_valign(GTK_WIDGET(signal_icon), GTK_ALIGN_CENTER);
  adw_action_row_add_suffix(row, GTK_WIDGET(signal_icon));

  return GTK_WIDGET(row);
}
static void clear_list_box(GtkListBox *list_box) {
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(GTK_WIDGET(list_box))) != NULL) {
    gtk_list_box_remove(list_box, child);
  }
}

static void update_wifi_networks_list(GtkListBox *list_box) {
  refresh_wifi_networks(list_box);
}

static void refresh_wifi_networks(GtkListBox *list_box) {
  ScanData *scan_data = g_new0(ScanData, 1);
  scan_data->list_box = list_box;
  scan_data->cancellable = g_cancellable_new();

  GTask *task =
      g_task_new(NULL, scan_data->cancellable, wifi_scan_complete, scan_data);
  g_task_run_in_thread(task, wifi_scan_thread);
  g_object_unref(task);
}

static gboolean on_refresh_timeout(gpointer user_data) {
  GtkListBox *list_box = GTK_LIST_BOX(user_data);
  refresh_wifi_networks(list_box);
  return G_SOURCE_CONTINUE;
}
// static gboolean on_refresh_timeout(gpointer user_data) {
//     InitData *init_data = (InitData *)user_data;

//     // Refresh both list and current status
//     refresh_wifi_networks(init_data->wifi_list);
//     update_current_network(init_data->builder);

//     return G_SOURCE_CONTINUE;
// }

void change_panel_to_wifi(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  if (WifiPage != NULL) {
    gtk_stack_set_visible_child_name(stack, "wifi_page");
  } else {
    wifi_to_stack(stack);
  }
}
static void on_wifi_switch_active(GObject *wifi_switch, GParamSpec *pspec,
                                  gpointer user_data) {
  gboolean is_active;
  g_object_get(wifi_switch, "active", &is_active, NULL);

  GTask *task = g_task_new(NULL, NULL, wifi_toggle_complete, wifi_switch);
  g_task_set_task_data(task, GINT_TO_POINTER(is_active), NULL);
  g_task_run_in_thread(task, wifi_toggle_thread);
  g_object_unref(task);
}

static void wifi_status_thread(GTask *task, gpointer source_object,
                               gpointer task_data, GCancellable *cancellable) {
  GError *error = NULL;
  gchar *output = NULL;

  if (!g_spawn_command_line_sync("nmcli radio wifi", &output, NULL, NULL,
                                 &error)) {
    g_task_return_error(task, error);
    return;
  }

  gboolean wifi_enabled = (g_strstr_len(output, -1, "enabled") != NULL);
  g_free(output);
  g_task_return_boolean(task, wifi_enabled);
}

static void wifi_status_complete(GObject *source_object, GAsyncResult *result,
                                 gpointer user_data) {
  InitData *init_data = user_data;
  GError *error = NULL;
  gboolean wifi_enabled;

  wifi_enabled = g_task_propagate_boolean(G_TASK(result), &error);
  if (!error) {
    set_wifi_switch_state(init_data->wifi_switch, wifi_enabled);
  }

  complete_initialization(init_data);
}

static gboolean get_wifi_status(void) {
  gchar *command = "nmcli radio wifi";
  gchar *output = NULL;
  GError *error = NULL;
  gint exit_status;

  g_spawn_command_line_sync(command, &output, NULL, &exit_status, &error);

  if (error) {
    g_printerr("Failed to get Wi-Fi status: %s\n", error->message);
    g_error_free(error);
    return FALSE;
  }

  gboolean wifi_enabled = (g_strstr_len(output, -1, "enabled") != NULL);
  g_free(output);
  return wifi_enabled;
}

static void set_wifi_switch_state(GtkWidget *wifi_switch, gboolean is_active) {
  g_object_set(wifi_switch, "active", is_active, NULL);
}

static void toggle_wifi(gboolean enable) {
  gchar *command =
      g_strdup_printf("nmcli radio wifi %s", enable ? "on" : "off");
  g_print("Executing command: %s\n", command);

  GError *error = NULL;
  gint exit_status;
  g_spawn_command_line_sync(command, NULL, NULL, &exit_status, &error);
  g_free(command);

  if (error) {
    g_printerr("Failed to toggle Wi-Fi: %s\n", error->message);
    g_error_free(error);
  }
}

// Modify the row click handler to initiate connection
static void on_row_clicked(GtkGestureClick *gesture, gint n_press, gdouble x,
                           gdouble y, gpointer user_data) {
  AdwActionRow *row = ADW_ACTION_ROW(user_data);
  if (row) {
    const char *ssid = adw_preferences_row_get_title(ADW_PREFERENCES_ROW(row));
    const char *subtitle = adw_action_row_get_subtitle(row);
    gboolean is_secured = g_str_has_prefix(subtitle, "Secured");

    g_print("Attempting to connect to: %s\n", ssid);
    connect_to_network(ssid, is_secured);
  }
}

// Function to handle network connection
static void connect_to_network(const char *ssid, gboolean is_secured) {
  if (is_secured) {
    show_password_dialog(ssid);
  } else {
    // Connect directly to open network
    connect_with_password(ssid, NULL);
  }
}

// Function to show password dialog for secured networks
static void show_password_dialog(const char *ssid) {
  GtkWidget *dialog;
  GtkWidget *content_area;
  GtkWidget *password_entry;
  GtkWidget *box;
  GtkWidget *label;

  // Create dialog
  dialog = gtk_dialog_new_with_buttons(
      "Connect to Network", NULL, GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR,
      "Cancel", GTK_RESPONSE_CANCEL, "Connect", GTK_RESPONSE_OK, NULL);

  content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  // Create a box for the content
  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 20);
  gtk_widget_set_margin_end(box, 20);
  gtk_widget_set_margin_top(box, 20);
  gtk_widget_set_margin_bottom(box, 20);

  // Add network name label
  label = gtk_label_new(NULL);
  char *markup = g_markup_printf_escaped("Enter password for <b>%s</b>", ssid);
  gtk_label_set_markup(GTK_LABEL(label), markup);
  g_free(markup);
  gtk_box_append(GTK_BOX(box), label);

  // Add password entry
  password_entry = gtk_password_entry_new();
  gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(password_entry),
                                        TRUE);
  gtk_widget_set_hexpand(password_entry, TRUE);
  gtk_box_append(GTK_BOX(box), password_entry);

  // Add box to dialog
  gtk_box_append(GTK_BOX(content_area), box);

  // Connect response signal
  g_signal_connect(dialog, "response", G_CALLBACK(on_password_dialog_response),
                   (gpointer)g_strdup(ssid));

  // Show dialog
  gtk_widget_show(dialog);
}

// Handle dialog response
static void on_password_dialog_response(GtkDialog *dialog, gint response_id,
                                        gpointer user_data) {
  char *ssid = (char *)user_data;

  if (response_id == GTK_RESPONSE_OK) {
    GtkWidget *content_area = gtk_dialog_get_content_area(dialog);
    GtkWidget *box = gtk_widget_get_first_child(content_area);
    GtkWidget *password_entry = gtk_widget_get_last_child(box);

    const char *password = gtk_editable_get_text(GTK_EDITABLE(password_entry));
    connect_with_password(ssid, password);
  }

  g_free(ssid);
  gtk_window_destroy(GTK_WINDOW(dialog));
}

// Function to actually connect to the network
static void connect_with_password(const char *ssid, const char *password) {
  GError *error = NULL;
  char *command;

  if (password) {
    command = g_strdup_printf("nmcli dev wifi connect '%s' password '%s' ",
                              ssid, password);
  } else {
    command = g_strdup_printf("nmcli device wifi connect '%s'", ssid);
  }

  g_print("Executing: %s\n", command);

  gchar *output = NULL;
  if (!g_spawn_command_line_sync(command, &output, NULL, NULL, &error)) {
    g_print("Failed to connect: %s\n", error->message);
    g_error_free(error);
  } else {
    g_print("Connection output: %s\n", output);
  }

  // if (!error) {
  //     g_print("Connected successfully!\n");
  //     // Update network list and current connection
  //     refresh_wifi_networks(init_data->wifi_list);
  //     update_current_network(init_data->builder);
  // }

  g_free(command);
  g_free(output);
}

static void update_current_network(GtkBuilder *builder) {
  GError *error = NULL;
  gchar *output = NULL;
  GtkWidget *current_row =
      GTK_WIDGET(gtk_builder_get_object(builder, "current_network_row"));
  GtkWidget *current_icon =
      GTK_WIDGET(gtk_builder_get_object(builder, "current_network_icon"));

  if (!g_spawn_command_line_sync("nmcli -t -f active,ssid dev wifi", &output,
                                 NULL, NULL, &error)) {
    g_warning("Failed to get current network: %s", error->message);
    g_error_free(error);
    return;
  }

  gchar **lines = g_strsplit(output, "\n", -1);
  gboolean connected = FALSE;

  for (int i = 0; lines[i] != NULL; i++) {
    gchar **fields = g_strsplit(lines[i], ":", 2);
    if (g_strv_length(fields) >= 2 && g_strcmp0(fields[0], "yes") == 0) {
      adw_preferences_row_set_title(ADW_PREFERENCES_ROW(current_row),
                                    fields[1]);
      adw_action_row_set_subtitle(ADW_ACTION_ROW(current_row), "Connected");
      gtk_image_set_from_icon_name(
          GTK_IMAGE(current_icon),
          "network-wireless-signal-excellent-symbolic");
      connected = TRUE;
      break;
    }
    g_strfreev(fields);
  }

  if (!connected) {
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(current_row),
                                  "Not Connected");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(current_row),
                                "No active network connection");
    gtk_image_set_from_icon_name(GTK_IMAGE(current_icon),
                                 "network-wireless-offline-symbolic");
  }

  g_strfreev(lines);
  g_free(output);
}

static void wifi_to_stack(GtkStack *stack) {
  // Check if page already exists
  if (WifiPage != NULL) {
    gtk_stack_set_visible_child_name(stack, "wifi_page");
    return;
  }

  const char *ui_paths[] = {"ui/wifi.ui",
                            "/usr/local/share/systune/ui/wifi.ui"};

  GtkBuilder *wifi_builder = NULL;
  size_t num_paths = sizeof(ui_paths) / sizeof(ui_paths[0]);

  for (size_t i = 0; i < num_paths; i++) {
    if (g_file_test(ui_paths[i], G_FILE_TEST_EXISTS)) {
      wifi_builder = gtk_builder_new_from_file(ui_paths[i]);
      if (wifi_builder != NULL) {
        g_print("Loaded UI file from: %s\n", ui_paths[i]);
        break;
      }
    }
  }

  if (wifi_builder == NULL) {
    g_warning("UI file 'wifi.ui' not found in any of the expected locations");
    return;
  }

  WifiPage = GTK_WIDGET(gtk_builder_get_object(wifi_builder, "wifi_page"));
  if (WifiPage == NULL) {
    g_printerr("Failed to get wifi_page from wifi.ui\n");
    g_object_unref(wifi_builder);
    return;
  }

  // Add page to stack immediately
  gtk_stack_add_named(stack, WifiPage, "wifi_page");
  gtk_widget_set_visible(WifiPage, TRUE);
  gtk_stack_set_visible_child_name(stack, "wifi_page");

  InitData *init_data = g_new0(InitData, 1);
  init_data->stack = stack;
  init_data->builder = wifi_builder;

  init_data->wifi_switch =
      GTK_WIDGET(gtk_builder_get_object(wifi_builder, "wifi_switch"));
  if (init_data->wifi_switch != NULL) {
    g_signal_connect(init_data->wifi_switch, "notify::active",
                     G_CALLBACK(on_wifi_switch_active), NULL);
    // Set initial state immediately
    set_wifi_switch_state(init_data->wifi_switch, get_wifi_status());
  }

  init_data->wifi_list =
      GTK_LIST_BOX(gtk_builder_get_object(wifi_builder, "wifi_networks_list"));
  if (init_data->wifi_list != NULL) {
    g_print("DEBUG: Found wifi_list\n");

    // Add a direct click handler to the list box as well
    GtkGesture *list_click = gtk_gesture_click_new();
    g_signal_connect(list_click, "pressed", G_CALLBACK(on_row_clicked), NULL);
    gtk_widget_add_controller(GTK_WIDGET(init_data->wifi_list),
                              GTK_EVENT_CONTROLLER(list_click));

    refresh_wifi_networks(init_data->wifi_list);
  }

  // Start immediate network scan
  if (init_data->wifi_list != NULL) {
    refresh_wifi_networks(init_data->wifi_list);
    update_current_network(init_data->builder);
  }

  // Start async initialization chain for additional setup
  GTask *wifi_task = g_task_new(NULL, NULL, wifi_status_complete, init_data);
  g_task_run_in_thread(
      wifi_task, wifi_status_thread); // Fixed: changed 'task' to 'wifi_task'
  g_object_unref(wifi_task);
}
void cleanup_wifi(void) {
  if (refresh_timeout_id > 0) {
    g_source_remove(refresh_timeout_id);
    refresh_timeout_id = 0;
  }
}
