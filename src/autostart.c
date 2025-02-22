#include "option/autostart.h"
#include <gtk/gtk.h>

GtkWidget *AutostartPage;

void change_panel_to_autostart(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  autostart_to_stack(stack);
  gtk_stack_set_visible_child_name(stack, "autostart_page");
}

static void autostart_to_stack(GtkStack *stack) {
  if (AutostartPage) {
    return;
  }

  GtkBuilder *autostart_builder = gtk_builder_new_from_file("ui/autostart_apps.ui");
  if (autostart_builder == NULL) {
    g_printerr("Failed to load autostart.ui\n");
    return;
  }

  AutostartPage = GTK_WIDGET(gtk_builder_get_object(autostart_builder, "autostart_apps_page"));
  if (AutostartPage == NULL) {
    g_printerr("Failed to get autostart_page from autostart.ui\n");
    g_object_unref(autostart_builder);
    return;
  }

  gtk_stack_add_named(stack, AutostartPage, "autostart_page");
  g_object_unref(autostart_builder);
}
