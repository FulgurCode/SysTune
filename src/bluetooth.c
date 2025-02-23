#include "option/bluetooth.h"
#include <gtk/gtk.h>
#include <adwaita.h>
#include <glib.h>
#include <stdio.h>
#include <gtk/gtkwidget.h>
#include <gio/gio.h>

GtkWidget *BluetoothPage;

typedef struct {
    GtkStack *stack;
    GtkBuilder *builder;
    GtkWidget *bluetooth_switch;
} InitData;

// Forward declarations
static void clear_list_box(GtkListBox *list_box);
static GtkWidget* create_network_row(const char *ssid, int signal_strength, gboolean is_secured);
static const char* get_signal_icon_name(int signal_strength);
static void on_bluetooth_switch_active(GObject *bluetooth_switch, GParamSpec *pspec, gpointer user_data);
static gboolean get_bluetooth_status(void);
static void set_bluetooth_switch_state(GtkWidget *bluetooth_switch, gboolean is_active);
static void toggle_bluetooth(gboolean enable);
static void bluetooth_to_stack(GtkStack *stack);
static gboolean on_refresh_timeout(gpointer user_data);

// Global variables
static guint refresh_timeout_id = 0;

// Function to complete initialization after all async operations
static void complete_initialization(InitData *init_data) {
    // Add the page to the stack if it's not already there
    if (gtk_widget_get_parent(BluetoothPage) == NULL) {
        gtk_stack_add_named(init_data->stack, BluetoothPage, "bluetooth_page");
    }
    
    // Immediately show the bluetooth page
    gtk_stack_set_visible_child_name(init_data->stack, "bluetooth_page");
    
    g_object_unref(init_data->builder);
    g_free(init_data);
}


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
    adw_action_row_set_subtitle(row, is_secured ? "Secured with WPA" : "Open Network");

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

// Modified timeout callback for Bluetooth
static gboolean on_refresh_timeout(gpointer user_data) {
    // Implement Bluetooth device refresh logic here
    return G_SOURCE_CONTINUE;
}

void change_panel_to_bluetooth(gpointer user_data) {
    GtkStack *stack = GTK_STACK(user_data);
    
    // If the page already exists, just show it
    if (BluetoothPage && gtk_widget_get_parent(BluetoothPage) != NULL) {
        gtk_stack_set_visible_child_name(stack, "bluetooth_page");
        return;
    }
    
    // Otherwise, create and show it
    bluetooth_to_stack(stack);
}


static void on_bluetooth_switch_active(GObject *bluetooth_switch, GParamSpec *pspec, gpointer user_data) {
    gboolean is_active;
    g_object_get(bluetooth_switch, "active", &is_active, NULL);
    toggle_bluetooth(is_active);
}

// Async Bluetooth status check
static void bluetooth_status_thread(GTask *task, gpointer source_object,
    gpointer task_data, GCancellable *cancellable) {
    GError *error = NULL;
    gchar *output = NULL;

    if (!g_spawn_command_line_sync("bluetoothctl show", &output, NULL, NULL, &error)) {
        g_task_return_error(task, error);
        return;
    }

    gboolean bt_enabled = (g_strstr_len(output, -1, "Powered: yes") != NULL);
    g_free(output);
    g_task_return_boolean(task, bt_enabled);
}

static void bluetooth_status_complete(GObject *source_object, GAsyncResult *result,
    gpointer user_data) {
    InitData *init_data = user_data;
    GError *error = NULL;
    gboolean bt_enabled;

    bt_enabled = g_task_propagate_boolean(G_TASK(result), &error);
    if (!error) {
        set_bluetooth_switch_state(init_data->bluetooth_switch, bt_enabled);
    }

    // Complete initialization
    complete_initialization(init_data);
}

static gboolean get_bluetooth_status(void) {
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

    gboolean bluetooth_enabled = (g_strstr_len(output, -1, "Powered: yes") != NULL);
    g_free(output);
    return bluetooth_enabled;
}

static void set_bluetooth_switch_state(GtkWidget *bluetooth_switch, gboolean is_active) {
    g_object_set(bluetooth_switch, "active", is_active, NULL);
}

static void toggle_bluetooth(gboolean enable) {
    gchar *command = g_strdup_printf("bluetoothctl power %s", enable ? "on" : "off");
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

static void bluetooth_to_stack(GtkStack *stack) {
    // If the page already exists, just show it
    if (BluetoothPage) {
        gtk_stack_set_visible_child_name(stack, "bluetooth_page");
        return;
    }

    GtkBuilder *bluetooth_builder = gtk_builder_new_from_file("ui/bluetooth.ui");
    if (bluetooth_builder == NULL) {
        g_printerr("Failed to load bluetooth.ui\n");
        return;
    }

    BluetoothPage = GTK_WIDGET(gtk_builder_get_object(bluetooth_builder, "bluetooth_page"));
    if (BluetoothPage == NULL) {
        g_printerr("Failed to get bluetooth_page from bluetooth.ui\n");
        g_object_unref(bluetooth_builder);
        return;
    }

    InitData *init_data = g_new0(InitData, 1);
    init_data->stack = stack;
    init_data->builder = bluetooth_builder;

    // Set up Bluetooth switch
    init_data->bluetooth_switch = GTK_WIDGET(gtk_builder_get_object(bluetooth_builder, "bluetooth_switch"));
    if (init_data->bluetooth_switch != NULL) {
        g_signal_connect(init_data->bluetooth_switch, "notify::active", G_CALLBACK(on_bluetooth_switch_active), NULL);
        
        // Initialize switch state immediately
        gboolean bt_status = get_bluetooth_status();
        set_bluetooth_switch_state(init_data->bluetooth_switch, bt_status);
    }

    // Add and show the page immediately
    gtk_stack_add_named(stack, BluetoothPage, "bluetooth_page");
    gtk_stack_set_visible_child_name(stack, "bluetooth_page");

    // Start async Bluetooth status check for updating the UI
    GTask *bt_task = g_task_new(NULL, NULL, bluetooth_status_complete, init_data);
    g_task_run_in_thread(bt_task, bluetooth_status_thread);
    g_object_unref(bt_task);
}

void cleanup_bluetooth(void) {
    if (refresh_timeout_id > 0) {
        g_source_remove(refresh_timeout_id);
        refresh_timeout_id = 0;
    }
}