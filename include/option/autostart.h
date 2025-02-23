// #ifndef AUTOSTART_H
// #define AUTOSTART_H

// #include <gtk/gtk.h>

// extern GtkWidget* AutostartPage;

// void change_panel_to_autostart(gpointer);
// static void autostart_to_stack(GtkStack*);

// #endif

// autostart.h
#ifndef AUTOSTART_H
#define AUTOSTART_H

#include <gtk/gtk.h>
#include <adwaita.h>

void change_panel_to_autostart(gpointer user_data);
void autostart_to_stack(GtkStack *stack);

#endif // AUTOSTART_H
