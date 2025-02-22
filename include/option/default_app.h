#ifndef DEFAULT_APPS_H
#define DEFAULT_APPS_H

#include <gtk/gtk.h>

extern GtkWidget* Default_AppsPage;

void change_panel_to_default_apps(gpointer);
static void default_apps_to_stack(GtkStack*);

#endif
