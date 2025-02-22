#include "option/keyboard_shortcuts.h"
#include <gtk/gtk.h>

GtkWidget *Keyboard_ShortcutsPage;

void change_panel_to_keyboard_shortcuts(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  keyboard_shortcuts_to_stack(stack);
  gtk_stack_set_visible_child_name(stack, "keyboard_shortcuts_page");
}

static void keyboard_shortcuts_to_stack(GtkStack *stack) {
  if (Keyboard_ShortcutsPage) {
    return;
  }

  GtkBuilder *keyboard_shortcuts_builder = gtk_builder_new_from_file("ui/keyboard_shortcuts.ui");
  if (keyboard_shortcuts_builder == NULL) {
    g_printerr("Failed to load keyboard_shortcuts.ui\n");
    return;
  }

  Keyboard_ShortcutsPage = GTK_WIDGET(gtk_builder_get_object(keyboard_shortcuts_builder, "keyboard_shortcuts_page"));
  if (Keyboard_ShortcutsPage == NULL) {
    g_printerr("Failed to get keyboard_shortcuts_page from keyboard_shortcuts.ui\n");
    g_object_unref(keyboard_shortcuts_builder);
    return;
  }

  gtk_stack_add_named(stack, Keyboard_ShortcutsPage, "keyboard_shortcuts_page");
  g_object_unref(keyboard_shortcuts_builder);
}
