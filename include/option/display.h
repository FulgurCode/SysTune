#ifndef DISPLAY_H
#define DISPLAY_H

#include <gtk/gtk.h>

extern GtkWidget* DisplayPage;

void change_panel_to_display(gpointer);
static void display_to_stack(GtkStack*);

#endif
