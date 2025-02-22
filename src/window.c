#include "window/window.h"

static void print_hello(GtkWidget *widget, gpointer data) {
    g_print("Hello World\n");
}

static void quit_cb(GtkWindow *window) {
    gtk_window_close(window);
}

GtkWidget *create_main_window(GtkApplication *app) {
    /* Load UI from file */
    GtkBuilder *builder = gtk_builder_new();
    if (!gtk_builder_add_from_file(builder, "ui/main.ui", NULL)) {
        g_printerr("Failed to load UI file\n");
        return NULL;
    }

    /* Get main window */
    GObject *window = gtk_builder_get_object(builder, "window");
    if (!window) {
        g_printerr("Failed to find 'window' in UI file\n");
        return NULL;
    }
    gtk_window_set_application(GTK_WINDOW(window), app);

    /* /\* Connect buttons *\/ */
    /* GObject *button = gtk_builder_get_object(builder, "button1"); */
    /* if (button) g_signal_connect(button, "clicked", G_CALLBACK(print_hello), NULL); */

    /* button = gtk_builder_get_object(builder, "button2"); */
    /* if (button) g_signal_connect(button, "clicked", G_CALLBACK(print_hello), NULL); */

    /* button = gtk_builder_get_object(builder, "quit"); */
    /* if (button) g_signal_connect_swapped(button, "clicked", G_CALLBACK(quit_cb), window); */

    g_object_unref(builder);
    return GTK_WIDGET(window);
}
