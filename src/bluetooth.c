#include "option/bluetooth.h"
#include "command/command.h"
#include <adwaita.h>
#include <gio/gio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gtk/gtkwidget.h>
#include <stdio.h>
#include <string.h>

// Structures
typedef struct {
  char address[18];
  char name[128];
  gboolean is_connected;
} BluetoothDevice;

typedef struct {
  GtkStack *stack;
  GtkBuilder *builder;
  GtkWidget *bluetooth_switch;
} InitData;

// Global variables
GtkWidget *BluetoothPage = NULL;
guint refresh_timeout_id = 0;

// Forward declarations
void clear_list_box(GtkListBox *list_box);
const char *get_signal_icon_name(int signal_strength);
void on_bluetooth_switch_active(GObject *bluetooth_switch, GParamSpec *pspec,
                                gpointer user_data);
gboolean get_bluetooth_status(void);
void set_bluetooth_switch_state(GtkWidget *bluetooth_switch,
                                gboolean is_active);
void toggle_bluetooth(gboolean enable);
void bluetooth_to_stack(GtkStack *stack);
gboolean on_refresh_timeout(gpointer user_data);
void on_device_row_activated(GtkListBox *box, GtkListBoxRow *row,
                             gpointer user_data);
gboolean connect_bluetooth_device(const char *address);
gboolean disconnect_bluetooth_device(const char *address);
gboolean is_device_connected(const char *address);
void scan_bluetooth_devices(GtkListBox *devices_list);
void initialize_bluetooth_page(GtkBuilder *builder);
void complete_initialization(InitData *init_data);
void bluetooth_status_thread(GTask *task, gpointer source_object,
                             gpointer task_data, GCancellable *cancellable);
void bluetooth_status_complete(GObject *source_object, GAsyncResult *result,
                               gpointer user_data);

// Implementation of core functions
void clear_list_box(GtkListBox *list_box) {
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(GTK_WIDGET(list_box))) != NULL) {
    gtk_list_box_remove(list_box, child);
  }
}

const char *get_signal_icon_name(int signal_strength) {
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

GtkWidget *create_device_row(const char *name, const char *status,
                             const char *icon_name) {
  AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), name);
  adw_action_row_set_subtitle(row, status);

  // Add a settings button
  GtkButton *settings_button =
      GTK_BUTTON(gtk_button_new_from_icon_name("emblem-system-symbolic"));
  gtk_widget_set_valign(GTK_WIDGET(settings_button), GTK_ALIGN_CENTER);
  gtk_widget_set_tooltip_text(GTK_WIDGET(settings_button), "Device settings");
  adw_action_row_add_suffix(row, GTK_WIDGET(settings_button));

  // Store the device name in the row widget
  g_object_set_data(G_OBJECT(row), "device-name", (gpointer)name);

  return GTK_WIDGET(row);
}

void on_device_row_activated(GtkListBox *box, GtkListBoxRow *row,
                             gpointer user_data) {
  g_print("clicked");
  BluetoothDevice *device = g_object_get_data(G_OBJECT(row), "device-data");
  GtkSpinner *spinner =
      GTK_SPINNER(g_object_get_data(G_OBJECT(row), "spinner"));
  if (!device)
    return;

  // Show spinner during operation
  gtk_widget_set_visible(GTK_WIDGET(spinner), TRUE);
  gtk_spinner_start(spinner);

  // Disable row during operation
  gtk_widget_set_sensitive(GTK_WIDGET(row), FALSE);

  if (device->is_connected) {
    g_print("connected");
    if (disconnect_bluetooth_device(device->address)) {
      device->is_connected = FALSE;
      adw_action_row_set_subtitle(ADW_ACTION_ROW(row), "Available");
    }
  } else {
    g_print("not connected");
    if (connect_bluetooth_device(device->address)) {
      device->is_connected = TRUE;
      adw_action_row_set_subtitle(ADW_ACTION_ROW(row), "Connected");
    }
  }

  // Hide spinner and re-enable row
  gtk_spinner_stop(spinner);
  gtk_widget_set_visible(GTK_WIDGET(spinner), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(row), TRUE);
}

gboolean connect_bluetooth_device(const char *address) {
  GError *error = NULL;
  gchar *command = g_strdup_printf("bluetoothctl connect %s", address);
  gchar *output = NULL;
  gint exit_status;

  g_spawn_command_line_sync(command, &output, NULL, &exit_status, &error);
  g_free(command);

  if (error) {
    g_printerr("Failed to connect device: %s\n", error->message);
    g_error_free(error);
    g_free(output);
    return FALSE;
  }

  gboolean success =
      (g_strstr_len(output, -1, "Connection successful") != NULL);
  g_free(output);
  return success;
}

gboolean disconnect_bluetooth_device(const char *address) {
  GError *error = NULL;
  gchar *command = g_strdup_printf("bluetoothctl disconnect %s", address);
  gchar *output = NULL;
  gint exit_status;

  g_spawn_command_line_sync(command, &output, NULL, &exit_status, &error);
  g_free(command);

  if (error) {
    g_printerr("Failed to disconnect device: %s\n", error->message);
    g_error_free(error);
    g_free(output);
    return FALSE;
  }

  gboolean success =
      (g_strstr_len(output, -1, "Successful disconnected") != NULL);
  g_free(output);
  return success;
}

gboolean is_device_connected(const char *address) {
  GError *error = NULL;
  gchar *command = g_strdup_printf("bluetoothctl info %s", address);
  gchar *output = NULL;
  gint exit_status;

  g_spawn_command_line_sync(command, &output, NULL, &exit_status, &error);
  g_free(command);

  if (error) {
    g_printerr("Failed to get device info: %s\n", error->message);
    g_error_free(error);
    g_free(output);
    return FALSE;
  }

  gboolean connected = (g_strstr_len(output, -1, "Connected: yes") != NULL);
  g_free(output);
  return connected;
}

void scan_bluetooth_devices(GtkListBox *devices_list) {
  clear_list_box(devices_list);

  // Start scanning
  system("bluetoothctl scan on &");

  // Get list of devices
  char *output = execute_command("bluetoothctl devices");
  if (!output)
    return;

  // Connect row activation handler
  g_signal_connect(devices_list, "pressed",
                   G_CALLBACK(on_device_row_activated), NULL);

  // Parse and add devices
  char *line = strtok(output, "\n");
  while (line) {
    char address[18], name[128];
    if (sscanf(line, "Device %17s %[^\n]", address, name) == 2) {
      gboolean is_connected = is_device_connected(address);
      GtkWidget *row = create_device_row(name, address, "*");
      g_signal_connect(row, "pressed", G_CALLBACK(on_device_row_activated), NULL);
      gtk_list_box_append(devices_list, row);
    }
    line = strtok(NULL, "\n");
  }

  free(output);
}

void on_bluetooth_switch_active(GObject *bluetooth_switch, GParamSpec *pspec,
                                gpointer user_data) {
  gboolean is_active;
  g_object_get(bluetooth_switch, "active", &is_active, NULL);
  toggle_bluetooth(is_active);
}

void on_discoverable_switch_active(GObject *discoverable_switch,
                                   GParamSpec *pspec, gpointer user_data) {
  gboolean is_active;
  g_object_get(discoverable_switch, "active", &is_active, NULL);

  GError *error = NULL;
  gchar *command =
      g_strdup_printf("bluetoothctl discoverable %s", is_active ? "on" : "off");
  g_spawn_command_line_sync(command, NULL, NULL, NULL, &error);
  g_free(command);

  if (error) {
    g_printerr("Failed to set discoverable: %s\n", error->message);
    g_error_free(error);
  }
}

gboolean get_bluetooth_status(void) {
  gchar *command = "bluetoothctl show | grep Powered";
  gchar *output = NULL;
  GError *error = NULL;
  gint exit_status;

  g_spawn_command_line_sync(command, &output, NULL, &exit_status, &error);
  if (error) {
    g_printerr("Failed to get Bluetooth status: %s\n", error->message);
    g_error_free(error);
    return FALSE;
  }

  gboolean bluetooth_enabled =
      (g_strstr_len(output, -1, "Powered: yes") != NULL);
  g_free(output);
  return bluetooth_enabled;
}

void set_bluetooth_switch_state(GtkWidget *bluetooth_switch,
                                gboolean is_active) {
  g_object_set(bluetooth_switch, "active", is_active, NULL);
}

void toggle_bluetooth(gboolean enable) {
  gchar *command =
      g_strdup_printf("bluetoothctl power %s", enable ? "on" : "off");
  g_print("Executing command: %s\n", command);

  GError *error = NULL;
  gint exit_status;
  g_spawn_command_line_sync(command, NULL, NULL, &exit_status, &error);
  g_free(command);

  if (error) {
    g_printerr("Failed to toggle Bluetooth: %s\n", error->message);
    g_error_free(error);
  }
}

gboolean on_refresh_timeout(gpointer user_data) {
  GtkListBox *devices_list = GTK_LIST_BOX(user_data);
  if (devices_list) {
    scan_bluetooth_devices(devices_list);
  }
  return G_SOURCE_CONTINUE;
}

void complete_initialization(InitData *init_data) {
  if (gtk_widget_get_parent(BluetoothPage) == NULL) {
    gtk_stack_add_named(init_data->stack, BluetoothPage, "bluetooth_page");
  }
  gtk_stack_set_visible_child_name(init_data->stack, "bluetooth_page");

  g_object_unref(init_data->builder);
  g_free(init_data);
}

void bluetooth_status_thread(GTask *task, gpointer source_object,
                             gpointer task_data, GCancellable *cancellable) {
  GError *error = NULL;
  gchar *output = NULL;

  if (!g_spawn_command_line_sync("bluetoothctl show", &output, NULL, NULL,
                                 &error)) {
    g_task_return_error(task, error);
    return;
  }

  gboolean bt_enabled = (g_strstr_len(output, -1, "Powered: yes") != NULL);
  g_free(output);
  g_task_return_boolean(task, bt_enabled);
}

void bluetooth_status_complete(GObject *source_object, GAsyncResult *result,
                               gpointer user_data) {
  InitData *init_data = user_data;
  GError *error = NULL;
  gboolean bt_enabled;

  bt_enabled = g_task_propagate_boolean(G_TASK(result), &error);
  if (!error) {
    set_bluetooth_switch_state(init_data->bluetooth_switch, bt_enabled);
  }

  complete_initialization(init_data);
}

void initialize_bluetooth_page(GtkBuilder *builder) {
  GtkWidget *bluetooth_switch =
      GTK_WIDGET(gtk_builder_get_object(builder, "bluetooth_switch"));
  GtkWidget *discoverable_switch =
      GTK_WIDGET(gtk_builder_get_object(builder, "discoverable_switch"));
  GtkListBox *devices_list =
      GTK_LIST_BOX(gtk_builder_get_object(builder, "bluetooth_devices_list"));

  if (bluetooth_switch && discoverable_switch && devices_list) {
    scan_bluetooth_devices(devices_list);

    g_signal_connect(bluetooth_switch, "notify::active",
                     G_CALLBACK(on_bluetooth_switch_active), devices_list);
    g_signal_connect(discoverable_switch, "notify::active",
                     G_CALLBACK(on_discoverable_switch_active), NULL);

    // Start periodic refresh
    refresh_timeout_id =
        g_timeout_add_seconds(10, on_refresh_timeout, devices_list);
  }
}

// Public functions
void change_panel_to_bluetooth(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);

  if (BluetoothPage && gtk_widget_get_parent(BluetoothPage) != NULL) {
    gtk_stack_set_visible_child_name(stack, "bluetooth_page");
    return;
  }

  bluetooth_to_stack(stack);
}

void bluetooth_to_stack(GtkStack *stack) {
  // If page exists, just show it
  if (BluetoothPage) {
    gtk_stack_set_visible_child_name(stack, "bluetooth_page");
    return;
  }

  GtkBuilder *bluetooth_builder = gtk_builder_new_from_file("ui/bluetooth.ui");
  if (bluetooth_builder == NULL) {
    g_printerr("Failed to load bluetooth.ui\n");
    return;
  }

  BluetoothPage =
      GTK_WIDGET(gtk_builder_get_object(bluetooth_builder, "bluetooth_page"));
  if (BluetoothPage == NULL) {
    g_printerr("Failed to get bluetooth_page from bluetooth.ui\n");
    g_object_unref(bluetooth_builder);
    return;
  }

  InitData *init_data = g_new0(InitData, 1);
  init_data->stack = stack;
  init_data->builder = bluetooth_builder;

  // Set up Bluetooth switch
  init_data->bluetooth_switch =
      GTK_WIDGET(gtk_builder_get_object(bluetooth_builder, "bluetooth_switch"));
  if (init_data->bluetooth_switch != NULL) {
    g_signal_connect(init_data->bluetooth_switch, "notify::active",
                     G_CALLBACK(on_bluetooth_switch_active), NULL);

    // Initialize switch state
    gboolean bt_status = get_bluetooth_status();
    set_bluetooth_switch_state(init_data->bluetooth_switch, bt_status);
  }

  // Initialize Bluetooth page
  initialize_bluetooth_page(bluetooth_builder);

  // Add and show page
  gtk_stack_add_named(stack, BluetoothPage, "bluetooth_page");
  gtk_stack_set_visible_child_name(stack, "bluetooth_page");

  // Start async Bluetooth status check
  GTask *bt_task = g_task_new(NULL, NULL, bluetooth_status_complete, init_data);
  g_task_run_in_thread(bt_task, bluetooth_status_thread);
  g_object_unref(bt_task);
}

void refresh_bluetooth_devices(GtkBuilder *builder) {
  if (BluetoothPage && builder) {
    // Get the devices_list from the builder
    GtkListBox *devices_list =
        GTK_LIST_BOX(gtk_builder_get_object(builder, "bluetooth_devices_list"));
    if (devices_list) {
      // Clear the existing list and scan for new devices
      clear_list_box(devices_list);
      scan_bluetooth_devices(devices_list);
    }
  }
}

// Cleanup function
void cleanup_bluetooth(void) {
  if (refresh_timeout_id > 0) {
    g_source_remove(refresh_timeout_id);
    refresh_timeout_id = 0;
  }

  // Stop scanning
  system("bluetoothctl scan off");

  // Cleanup any remaining device data
  if (BluetoothPage) {
    // Get the GtkBuilder instance associated with the BluetoothPage
    GtkBuilder *builder = gtk_builder_new_from_file("bluetooth.ui");
    if (!builder) {
      g_printerr("Failed to load UI file.\n");
      return;
    }

    // Get the devices_list from the builder
    GtkListBox *devices_list =
        GTK_LIST_BOX(gtk_builder_get_object(builder, "bluetooth_devices_list"));
    if (devices_list) {
      clear_list_box(devices_list);
    }

    // Clean up the builder
    g_object_unref(builder);
  }
}

// Function to get device name from address
char *get_device_name(const char *address) {
  GError *error = NULL;
  gchar *command = g_strdup_printf("bluetoothctl info %s", address);
  gchar *output = NULL;
  char *name = NULL;

  g_spawn_command_line_sync(command, &output, NULL, NULL, &error);
  g_free(command);

  if (error) {
    g_printerr("Failed to get device info: %s\n", error->message);
    g_error_free(error);
    g_free(output);
    return NULL;
  }

  // Parse name from output
  char *name_line = g_strstr_len(output, -1, "Name: ");
  if (name_line) {
    name_line += 6; // Skip "Name: "
    char *end = strchr(name_line, '\n');
    if (end) {
      size_t len = end - name_line;
      name = g_malloc(len + 1);
      strncpy(name, name_line, len);
      name[len] = '\0';
    }
  }

  g_free(output);
  return name;
}

// Function to pair with a device
gboolean pair_device(const char *address) {
  GError *error = NULL;
  gchar *command = g_strdup_printf("bluetoothctl pair %s", address);
  gchar *output = NULL;
  gint exit_status;

  g_spawn_command_line_sync(command, &output, NULL, &exit_status, &error);
  g_free(command);

  if (error) {
    g_printerr("Failed to pair device: %s\n", error->message);
    g_error_free(error);
    g_free(output);
    return FALSE;
  }

  gboolean success = (g_strstr_len(output, -1, "Pairing successful") != NULL);
  g_free(output);
  return success;
}

// Function to remove a paired device
gboolean remove_device(const char *address) {
  GError *error = NULL;
  gchar *command = g_strdup_printf("bluetoothctl remove %s", address);
  gchar *output = NULL;
  gint exit_status;

  g_spawn_command_line_sync(command, &output, NULL, &exit_status, &error);
  g_free(command);

  if (error) {
    g_printerr("Failed to remove device: %s\n", error->message);
    g_error_free(error);
    g_free(output);
    return FALSE;
  }

  gboolean success =
      (g_strstr_len(output, -1, "Device has been removed") != NULL);
  g_free(output);
  return success;
}
