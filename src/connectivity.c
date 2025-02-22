#include "option/connectivity.h"
#include <gtk/gtk.h>

GtkWidget *ConnectivityPage;

void change_panel_to_connectivity(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  connectivity_to_stack(stack);
  gtk_stack_set_visible_child_name(stack, "connectivity_page");
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

  gtk_stack_add_named(stack, ConnectivityPage, "connectivity_page");
  g_object_unref(connectivity_builder);
}
