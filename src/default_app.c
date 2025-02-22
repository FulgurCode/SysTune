#include "option/default_app.h"
#include <gtk/gtk.h>

GtkWidget *Default_AppsPage;

void change_panel_to_default_apps(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  default_apps_to_stack(stack);
  gtk_stack_set_visible_child_name(stack, "default_apps_page");
}

static void default_apps_to_stack(GtkStack *stack) {
  if (Default_AppsPage) {
    return;
  }

  GtkBuilder *default_apps_builder = gtk_builder_new_from_file("ui/default_apps.ui");
  if (default_apps_builder == NULL) {
    g_printerr("Failed to load default_apps.ui\n");
    return;
  }

  Default_AppsPage = GTK_WIDGET(gtk_builder_get_object(default_apps_builder, "default_apps_page"));
  if (Default_AppsPage == NULL) {
    g_printerr("Failed to get default_apps_page from default_apps.ui\n");
    g_object_unref(default_apps_builder);
    return;
  }

  gtk_stack_add_named(stack, Default_AppsPage, "default_apps_page");
  g_object_unref(default_apps_builder);
}
