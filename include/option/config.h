#ifndef CONFIG_H
#define CONFIG_H

#include <gtk/gtk.h>

extern GtkWidget* ConfigPage;

void change_panel_to_config(gpointer);
static void config_to_stack(GtkStack*);

#endif
