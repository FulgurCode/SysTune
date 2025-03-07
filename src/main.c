#include "window/window.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <sys/stat.h>

#ifndef G_APPLICATION_DEFAULT_FLAGS
#define G_APPLICATION_DEFAULT_FLAGS 0
#endif

static void load_css(void) {
  GtkCssProvider *provider = gtk_css_provider_new();
  GdkDisplay *display = gdk_display_get_default();

  const gchar *css_file_path = "styles/main.css";
  struct stat buffer;

  // Check if the first path exists, otherwise use the fallback
  if (stat(css_file_path, &buffer) != 0) {
    css_file_path = "/usr/share/systune/styles/main.css";
  }

  gtk_css_provider_load_from_path(provider, css_file_path);

  gtk_style_context_add_provider_for_display(
      display, GTK_STYLE_PROVIDER(provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  g_object_unref(provider);
}

static void activate(GtkApplication *app, gpointer user_data) {
  load_css();

  GtkWidget *window = create_main_window(app);
  gtk_widget_set_visible(window, TRUE);
}

int main(int argc, char *argv[]) {
  GtkApplication *app =
      gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);

  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}
