#ifndef KEYBOARD_SHORTCUTS_H
#define KEYBOARD_SHORTCUTS_H

#include <gtk/gtk.h>

extern GtkWidget* Keyboard_ShortcutsPage;

void change_panel_to_keyboard_shortcuts(gpointer);
static void keyboard_shortcuts_to_stack(GtkStack*);

#endif
