#include "option/display.h"
#include <gtk/gtk.h>

GtkWidget *DisplayPage;

void change_panel_to_display(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  display_to_stack(stack);
  gtk_stack_set_visible_child_name(stack, "display_page");
}

static void on_slider_value_changed(GtkRange *range, gpointer user_data) {
  gdouble value = gtk_range_get_value(range);

  gchar *command = g_strdup_printf("brightnessctl set %.0f%%\n", value);
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

  GtkAdjustment *adjustment = GTK_ADJUSTMENT(gtk_builder_get_object(display_builder, "adjustment_slider"));
  if (!adjustment) {
    g_printerr("Failed to get GtkAdjustment from display.ui\n");
    g_object_unref(display_builder);
    return;
  }

  GtkWidget *slider = GTK_WIDGET(gtk_builder_get_object(display_builder, "slider"));
  if (!slider) {
    g_printerr("Failed to get slider from display.ui\n");
    g_object_unref(display_builder);
    return;
  }

  gtk_range_set_adjustment(GTK_RANGE(slider), adjustment);
  g_signal_connect(slider, "value-changed", G_CALLBACK(on_slider_value_changed), NULL);


  gtk_stack_add_named(stack, DisplayPage, "display_page");
  g_object_unref(display_builder);
}
