#ifndef WINDOW_H
#define WINDOW_H

#include <gtk/gtk.h>

GtkWidget *create_main_window(GtkApplication *app);

static void print_hello(GtkWidget* , gpointer );

static void quit_cb(GtkWindow*);

#endif // WINDOW_H
