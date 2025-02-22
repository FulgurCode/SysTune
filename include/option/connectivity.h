#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#include <gtk/gtk.h>

extern GtkWidget* ConnectivityPage;

void change_panel_to_connectivity(gpointer);
static void connectivity_to_stack(GtkStack*);

#endif
