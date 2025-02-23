#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <gtk/gtk.h>

extern GtkWidget* BluetoothPage;

void change_panel_to_bluetooth(gpointer);
static void bluetooth_to_stack(GtkStack*);

#endif
