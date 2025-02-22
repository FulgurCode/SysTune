#ifndef SECURITY_H
#define SECURITY_H

#include <gtk/gtk.h>

extern GtkWidget* SecurityPage;

void change_panel_to_security(gpointer);
static void security_to_stack(GtkStack*);

#endif
