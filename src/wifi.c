#include "option/wifi.h"
#include <gtk/gtk.h>
#include <adwaita.h>
#include <glib.h>
#include <stdio.h>
#include <gtk/gtkwidget.h>
#include <gio/gio.h>

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
static GtkWidget* create_network_row(const char *ssid, int signal_strength, gboolean is_secured);
static const char* get_signal_icon_name(int signal_strength);
static void on_wifi_switch_active(GObject *wifi_switch, GParamSpec *pspec, gpointer user_data);
static gboolean get_wifi_status(void);
static void set_wifi_switch_state(GtkWidget *wifi_switch, gboolean is_active);
static void toggle_wifi(gboolean enable);
static void wifi_to_stack(GtkStack *stack);
static gboolean on_refresh_timeout(gpointer user_data);
static void refresh_wifi_networks(GtkListBox *list_box);

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
//         refresh_timeout_id = g_timeout_add_seconds(30, on_refresh_timeout, init_data->wifi_list);
//     }

//     // Start initial WiFi network scan
//     refresh_wifi_networks(init_data->wifi_list);
    
//     g_free(init_data);
// }
static void complete_initialization(InitData *init_data) {
    // Only set up periodic updates
    if (refresh_timeout_id == 0) {
        refresh_timeout_id = g_timeout_add_seconds(30, on_refresh_timeout, init_data->wifi_list);
    }
    
    g_object_unref(init_data->builder);
    g_free(init_data);
}


// Async WiFi scan operation
static void wifi_scan_thread(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable) { 
    GError *error = NULL;
    gchar *output = NULL;

    if (!g_spawn_command_line_sync("nmcli device wifi rescan", NULL, NULL, NULL, &error)) {
        g_task_return_error(task, error);
        return;
    }

    if (!g_spawn_command_line_sync("nmcli -t -f SSID,SIGNAL,SECURITY device wifi list", &output, NULL, NULL, &error)) {
        g_task_return_error(task, error);
        return;
    }

    g_task_return_pointer(task, output, g_free);
}

static void wifi_scan_complete(GObject *source_object, GAsyncResult *result, gpointer user_data) {
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
static void wifi_toggle_thread(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable) {
    gboolean enable = GPOINTER_TO_INT(task_data);
    GError *error = NULL;
    gchar *command = g_strdup_printf("nmcli radio wifi %s", enable ? "on" : "off");

    if (!g_spawn_command_line_sync(command, NULL, NULL, NULL, &error)) {
        g_free(command);
        g_task_return_error(task, error);
        return;
    }

    g_free(command);
    g_task_return_boolean(task, TRUE);
}

static void wifi_toggle_complete(GObject *source_object, GAsyncResult *result, gpointer user_data) {
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

static const char* get_signal_icon_name(int signal_strength) {
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

static GtkWidget* create_network_row(const char *ssid, int signal_strength, gboolean is_secured) {
    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), ssid);
    adw_action_row_set_subtitle(row, is_secured ? "Secured with WPA" : "Open Network");

    GtkImage *signal_icon = GTK_IMAGE(gtk_image_new_from_icon_name(
        get_signal_icon_name(signal_strength)));
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

    GTask *task = g_task_new(NULL, scan_data->cancellable,
                            wifi_scan_complete, scan_data);
    g_task_run_in_thread(task, wifi_scan_thread);
    g_object_unref(task);
}

static gboolean on_refresh_timeout(gpointer user_data) {
    GtkListBox *list_box = GTK_LIST_BOX(user_data);
    refresh_wifi_networks(list_box);
    return G_SOURCE_CONTINUE;
}

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

    if (!g_spawn_command_line_sync("nmcli radio wifi", &output, NULL, NULL, &error)) {
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
    gchar *command = g_strdup_printf("nmcli radio wifi %s", enable ? "on" : "off");
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
static void wifi_to_stack(GtkStack *stack) {
    // Check if page already exists
    if (WifiPage != NULL) {
        gtk_stack_set_visible_child_name(stack, "wifi_page");
        return;
    }

    GtkBuilder *wifi_builder = gtk_builder_new_from_file("ui/wifi.ui");
    if (wifi_builder == NULL) {
        g_printerr("Failed to load wifi.ui\n");
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

    init_data->wifi_switch = GTK_WIDGET(gtk_builder_get_object(wifi_builder, "wifi_switch"));
    if (init_data->wifi_switch != NULL) {
        g_signal_connect(init_data->wifi_switch, "notify::active", G_CALLBACK(on_wifi_switch_active), NULL);
        // Set initial state immediately
        set_wifi_switch_state(init_data->wifi_switch, get_wifi_status());
    }

    init_data->wifi_list = GTK_LIST_BOX(gtk_builder_get_object(wifi_builder, "wifi_networks_list"));
    
    // Start immediate network scan
    if (init_data->wifi_list != NULL) {
        refresh_wifi_networks(init_data->wifi_list);
    }

    // Start async initialization chain for additional setup
    GTask *wifi_task = g_task_new(NULL, NULL, wifi_status_complete, init_data);
    g_task_run_in_thread(wifi_task, wifi_status_thread);  // Fixed: changed 'task' to 'wifi_task'
    g_object_unref(wifi_task);
}
void cleanup_wifi(void) {
    if (refresh_timeout_id > 0) {
        g_source_remove(refresh_timeout_id);
        refresh_timeout_id = 0;
    }
}