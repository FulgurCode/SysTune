#include "option/display.h"
#include <gtk/gtk.h>

GtkWidget *DisplayPage;

void change_panel_to_display(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  display_to_stack(stack);
  gtk_stack_set_visible_child_name(stack, "display_page");
}

static void display_to_stack(GtkStack *stack) {
  if (DisplayPage) {
    return;
  }

  GtkBuilder *display_builder = gtk_builder_new_from_file("ui/display.ui");
  if (display_builder == NULL) {
    g_printerr("Failed to load display.ui\n");
    return;
  }

  DisplayPage = GTK_WIDGET(gtk_builder_get_object(display_builder, "display_page"));
  if (DisplayPage == NULL) {
    g_printerr("Failed to get display_page from display.ui\n");
    g_object_unref(display_builder);
    return;
  }

  gtk_stack_add_named(stack, DisplayPage, "display_page");
  g_object_unref(display_builder);
}
