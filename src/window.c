#include "window/window.h"
#include "option/audio.h"
#include "option/display.h"
#include "option/connectivity.h"
#include "option/wifi.h"
#include "option/bluetooth.h"
#include "option/autostart.h"
#include "option/security.h"
#include "option/default_app.h"
#include "option/user_permissions.h"
#include "option/keyboard_shortcuts.h"
#include "option/config.h"

static void quit_cb(GtkWindow *window) { gtk_window_close(window); }

static void on_setting_selected(GtkListBox *listbox, GtkListBoxRow *row,
                                gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  if (row == NULL)
    return; // Ensure the row is valid

  // Get the child widget of the row (e.g., the GtkLabel)
  GtkWidget *child = gtk_list_box_row_get_child(row);
  if (child == NULL)
    return; // Ensure the child exists

  // Get the name of the child widget
  const char *name = gtk_widget_get_name(child);

  if (g_strcmp0(name, "audio_controls") == 0) {
    change_panel_to_audio(user_data);
  } else if (g_strcmp0(name, "display_settings") == 0) {
    change_panel_to_display(user_data);
  } else if (g_strcmp0(name, "connectivity_options") == 0) {
    change_panel_to_connectivity(user_data);
  }else if (g_strcmp0(name, "wifi_options") == 0) {
    change_panel_to_wifi(user_data);
  }else if (g_strcmp0(name, "bluetooth_options") == 0) {
    change_panel_to_bluetooth(user_data);
  }else if (g_strcmp0(name, "autostart_apps") == 0) {
    change_panel_to_autostart(user_data);
  }else if (g_strcmp0(name, "security_settings") == 0) {
    change_panel_to_security(user_data);
  }else if (g_strcmp0(name, "default_apps") == 0) {
    change_panel_to_default_apps(user_data);
  }else if (g_strcmp0(name, "user_permissions") == 0) {
    change_panel_to_user_permissions(user_data);
  }else if (g_strcmp0(name, "keyboard_shortcuts") == 0) {
    change_panel_to_keyboard_shortcuts(user_data);
  }else if (g_strcmp0(name, "config_files") == 0) {
    change_panel_to_config(user_data);
  }
}

GtkWidget *create_main_window(GtkApplication *app) {
  /* Load UI from file */
  GtkBuilder *builder = gtk_builder_new();
  if (!gtk_builder_add_from_file(builder, "ui/main.ui", NULL) && !gtk_builder_add_from_file(builder, "/usr/local/share/systune/ui/main.ui", NULL)) {
    g_printerr("Failed to load UI file\n");
    return NULL;
  }

  /* Get main window */
  GObject *window = gtk_builder_get_object(builder, "window");
  GtkListBox *left_panel =
      GTK_LIST_BOX(gtk_builder_get_object(builder, "left_panel"));
  GtkStack *right_panel =
      GTK_STACK(gtk_builder_get_object(builder, "right_panel"));

  g_signal_connect(left_panel, "row-activated", G_CALLBACK(on_setting_selected),
                   right_panel);

  if (!window) {
    g_printerr("Failed to find 'window' in UI file\n");
    return NULL;
  }

  gtk_window_set_application(GTK_WINDOW(window), app);
  change_panel_to_display(right_panel);

  g_object_unref(builder);
  return GTK_WIDGET(window);
}
