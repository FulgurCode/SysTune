#include "option/connectivity.h"
#include <gtk/gtk.h>
#include <adwaita.h>
#include <glib.h>
#include <stdio.h>
#include <gtk/gtkwidget.h>

GtkWidget *ConnectivityPage;

// Forward declarations for all functions
static void update_wifi_networks_list(GtkListBox *list_box);
static void clear_list_box(GtkListBox *list_box);
static GtkWidget* create_network_row(const char *ssid, int signal_strength, gboolean is_secured);
static const char* get_signal_icon_name(int signal_strength);
static void on_wifi_switch_active(GObject *wifi_switch, GParamSpec *pspec, gpointer user_data);
static void on_bluetooth_switch_active(GObject *bluetooth_switch, GParamSpec *pspec, gpointer user_data);
static gboolean get_wifi_status(void);
static void set_wifi_switch_state(GtkWidget *wifi_switch, gboolean is_active);
static void toggle_wifi(gboolean enable);
static gboolean get_bluetooth_status(void);
static void set_bluetooth_switch_state(GtkWidget *bluetooth_switch, gboolean is_active);
static void toggle_bluetooth(gboolean enable);
static void connectivity_to_stack(GtkStack *stack);
static gboolean on_refresh_timeout(gpointer user_data);

// Global variables
static guint refresh_timeout_id = 0;

// Get signal strength icon name based on percentage
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

// Create a network row widget
static GtkWidget* create_network_row(const char *ssid, int signal_strength, gboolean is_secured) {
    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), ssid);
    
    // Set subtitle based on security
    adw_action_row_set_subtitle(row, is_secured ? "Secured with WPA" : "Open Network");

    // Create and add signal strength icon
    GtkImage *signal_icon = GTK_IMAGE(gtk_image_new_from_icon_name(
        get_signal_icon_name(signal_strength)));
    gtk_widget_set_valign(GTK_WIDGET(signal_icon), GTK_ALIGN_CENTER);
    adw_action_row_add_suffix(row, GTK_WIDGET(signal_icon));

    return GTK_WIDGET(row);
}

// Clear all items from the list box
static void clear_list_box(GtkListBox *list_box) {
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(GTK_WIDGET(list_box))) != NULL) {
        gtk_list_box_remove(list_box, child);
    }
}

// Update the WiFi networks list using nmcli
static void update_wifi_networks_list(GtkListBox *list_box) {
    gchar *command = "nmcli -t -f SSID,SIGNAL,SECURITY device wifi list";
    gchar *output = NULL;
    GError *error = NULL;
    gint exit_status;

    // Trigger a new scan first
    g_spawn_command_line_sync("nmcli device wifi rescan", NULL, NULL, NULL, NULL);
    // Wait a bit for scan to complete
    g_usleep(1000000); // 1 second delay

    g_spawn_command_line_sync(command, &output, NULL, &exit_status, &error);

    if (error) {
        g_printerr("Failed to get WiFi networks: %s\n", error->message);
        g_error_free(error);
        return;
    }

    if (!output) {
        return;
    }

    // Clear existing list
    clear_list_box(list_box);

    // Parse the output and create rows
    gchar **lines = g_strsplit(output, "\n", -1);
    for (int i = 0; lines[i] != NULL && *lines[i] != '\0'; i++) {
        gchar **fields = g_strsplit(lines[i], ":", -1);
        if (g_strv_length(fields) >= 3) {
            const char *ssid = fields[0];
            int signal_strength = atoi(fields[1]);
            gboolean is_secured = (fields[2] != NULL && *fields[2] != '\0');

            if (strlen(ssid) > 0) {
                GtkWidget *row = create_network_row(ssid, signal_strength, is_secured);
                gtk_list_box_append(list_box, row);
            }
        }
        g_strfreev(fields);
    }

    g_strfreev(lines);
    g_free(output);
}

// Timeout callback for periodic updates
static gboolean on_refresh_timeout(gpointer user_data) {
    GtkListBox *list_box = GTK_LIST_BOX(user_data);
    update_wifi_networks_list(list_box);
    return G_SOURCE_CONTINUE;
}

void change_panel_to_connectivity(gpointer user_data) {
    GtkStack *stack = GTK_STACK(user_data);
    connectivity_to_stack(stack);
    gtk_stack_set_visible_child_name(stack, "connectivity_page");
}

static void on_wifi_switch_active(GObject *wifi_switch, GParamSpec *pspec, gpointer user_data) {
    gboolean is_active;
    g_object_get(wifi_switch, "active", &is_active, NULL);
    toggle_wifi(is_active);
}

static void on_bluetooth_switch_active(GObject *bluetooth_switch, GParamSpec *pspec, gpointer user_data) {
    gboolean is_active;
    g_object_get(bluetooth_switch, "active", &is_active, NULL);
    toggle_bluetooth(is_active);
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

static gboolean get_bluetooth_status(void) {
    gchar *command = "nmcli radio bluetooth";
    gchar *output = NULL;
    GError *error = NULL;
    gint exit_status;
    g_spawn_command_line_sync(command, &output, NULL, &exit_status, &error);
    if (error) {
        g_printerr("Failed to get Bluetooth status: %s\n", error->message);
        g_error_free(error);
        return FALSE;
    }

    gboolean bluetooth_enabled = (g_strstr_len(output, -1, "enabled") != NULL);
    g_free(output);
    return bluetooth_enabled;
}

static void set_bluetooth_switch_state(GtkWidget *bluetooth_switch, gboolean is_active) {
    g_object_set(bluetooth_switch, "active", is_active, NULL);
}

static void toggle_bluetooth(gboolean enable) {
    gchar *command = g_strdup_printf("%s", enable ? "rfkill unblock bluetooth" : "rfkill block bluetooth");
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

static void connectivity_to_stack(GtkStack *stack) {
    if (ConnectivityPage) {
        return;
    }

    GtkBuilder *connectivity_builder = gtk_builder_new_from_file("ui/connectivity.ui");
    if (connectivity_builder == NULL) {
        g_printerr("Failed to load connectivity.ui\n");
        return;
    }

    ConnectivityPage = GTK_WIDGET(gtk_builder_get_object(connectivity_builder, "connectivity_page"));
    if (ConnectivityPage == NULL) {
        g_printerr("Failed to get connectivity_page from connectivity.ui\n");
        g_object_unref(connectivity_builder);
        return;
    }

    // Set up WiFi switch
    GtkWidget *wifi_switch = GTK_WIDGET(gtk_builder_get_object(connectivity_builder, "wifi_switch"));
    if (wifi_switch != NULL) {
        gboolean wifi_enabled = get_wifi_status();
        set_wifi_switch_state(wifi_switch, wifi_enabled);
        g_signal_connect(wifi_switch, "notify::active", G_CALLBACK(on_wifi_switch_active), NULL);
    } else {
        g_printerr("Wi-Fi switch not found in the UI file.\n");
    }

    // Set up Bluetooth switch
    GtkWidget *bluetooth_switch = GTK_WIDGET(gtk_builder_get_object(connectivity_builder, "bluetooth_switch"));
    if (bluetooth_switch != NULL) {
        gboolean bluetooth_enabled = get_bluetooth_status();
        g_print("Bluetooth status: %s\n", bluetooth_enabled ? "enabled" : "disabled");
        set_bluetooth_switch_state(bluetooth_switch, bluetooth_enabled);
        g_signal_connect(bluetooth_switch, "notify::active", G_CALLBACK(on_bluetooth_switch_active), NULL);
    } else {
        g_printerr("Bluetooth switch not found in the UI file.\n");
    }

    // Set up WiFi networks list
    GtkListBox *wifi_networks_list = GTK_LIST_BOX(gtk_builder_get_object(connectivity_builder, "wifi_networks_list"));
    if (wifi_networks_list != NULL) {
        // Initial update
        update_wifi_networks_list(wifi_networks_list);
        
        // Set up periodic updates (every 30 seconds)
        refresh_timeout_id = g_timeout_add_seconds(30, on_refresh_timeout, wifi_networks_list);
    }

    gtk_stack_add_named(stack, ConnectivityPage, "connectivity_page");
    g_object_unref(connectivity_builder);
}

void cleanup_connectivity(void) {
    if (refresh_timeout_id > 0) {
        g_source_remove(refresh_timeout_id);
        refresh_timeout_id = 0;
    }
}