#include "option/config.h"
#include <gtk/gtk.h>

GtkWidget *ConfigPage;

void change_panel_to_config(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  config_to_stack(stack);
  gtk_stack_set_visible_child_name(stack, "config_page");
}

static void config_to_stack(GtkStack *stack) {
  if (ConfigPage) {
    return;
  }

  GtkBuilder *config_builder = gtk_builder_new_from_file("ui/config_files.ui");
  if (config_builder == NULL) {
    g_printerr("Failed to load config.ui\n");
    return;
  }

  ConfigPage = GTK_WIDGET(gtk_builder_get_object(config_builder, "config_files_page"));
  if (ConfigPage == NULL) {
    g_printerr("Failed to get config_page from config.ui\n");
    g_object_unref(config_builder);
    return;
  }

  gtk_stack_add_named(stack, ConfigPage, "config_page");
  g_object_unref(config_builder);
}
