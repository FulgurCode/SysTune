#include "window/window.h"

static void print_hello(GtkWidget *widget, gpointer data) {
  g_print("Hello World\n");
}

static void quit_cb(GtkWindow *window) { gtk_window_close(window); }

static void on_setting_selected(GtkListBox *listbox, GtkListBoxRow *row,
                                gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  if (row == NULL)
    return; // Ensure the row is valid

  // Get the child widget of the row (e.g., the GtkLabel)
  GtkWidget *child = gtk_list_box_row_get_child(row);
  if (child == NULL)
    return; // Ensure the child exists

  // Get the name of the child widget
  const char *name = gtk_widget_get_name(child);
  g_print("\n%s", name);
  if (g_strcmp0(name, "audio_controls") == 0) {
    // Check if audio_page is already loaded
    GtkWidget *audio_page = gtk_stack_get_child_by_name(stack, "audio_page");
    if (!audio_page) {
      // Load audio.ui and add to the stack
      GtkBuilder *audio_builder = gtk_builder_new_from_file("ui/audio.ui");
      if (audio_builder == NULL) {
        g_printerr("Failed to load audio.ui\n");
        return;
      }

      audio_page =
          GTK_WIDGET(gtk_builder_get_object(audio_builder, "audio_page"));
      if (audio_page == NULL) {
        g_printerr("Failed to get audio_page from audio.ui\n");
        g_object_unref(audio_builder);
        return;
      }

      gtk_stack_add_named(stack, audio_page, "audio_page");
      g_object_unref(audio_builder);
    }
    // Switch to audio_page
    gtk_stack_set_visible_child_name(stack, "audio_page");
  } else if (g_strcmp0(name, "display_settings") == 0) {
    g_print("displayyy");
    GtkWidget *display_page =
        gtk_stack_get_child_by_name(stack, "display_page");
    if (!display_page) {
      // Load display.ui and add to the stack
      GtkBuilder *display_builder = gtk_builder_new_from_file("ui/display.ui");
      if (display_builder == NULL) {
        g_printerr("Failed to load display.ui\n");
        return;
      }

      display_page =
          GTK_WIDGET(gtk_builder_get_object(display_builder, "display_page"));
      if (display_page == NULL) {
        g_printerr("Failed to get display_page from display.ui\n");
        g_object_unref(display_builder);
        return;
      }

      gtk_stack_add_named(stack, display_page, "display_page");
      g_object_unref(display_builder);

    }
    // Switch to display_page
    gtk_stack_set_visible_child_name(stack, "display_page");
  } else if (g_strcmp0(name, "connectivity_options") == 0) {
    GtkWidget *connectivity_page = gtk_stack_get_child_by_name(stack, "connectivity_page");
    if (!connectivity_page) {
    GtkBuilder *connectivity_builder = gtk_builder_new_from_file("ui/connectivity.ui");
    if (connectivity_builder == NULL) {
        g_printerr("Failed to load connectivity.ui\n");
        return;
    }
    connectivity_page = GTK_WIDGET(gtk_builder_get_object(connectivity_builder, "connectivity_page"));
    if (connectivity_page == NULL) {
        g_printerr("Failed to get connectivity_page from connectivity.ui\n");
        g_object_unref(connectivity_builder);
        return;
    }
    gtk_stack_add_named(stack, connectivity_page, "connectivity_page");
    g_object_unref(connectivity_builder);
    }
    gtk_stack_set_visible_child_name(stack, "connectivity_page");
  }
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
  GtkListBox *left_panel =
      GTK_LIST_BOX(gtk_builder_get_object(builder, "left_panel"));
  GtkStack *right_panel =
      GTK_STACK(gtk_builder_get_object(builder, "right_panel"));

  g_signal_connect(left_panel, "row-activated", G_CALLBACK(on_setting_selected),
                   right_panel);

  if (!window) {
    g_printerr("Failed to find 'window' in UI file\n");
    return NULL;
  }
  gtk_window_set_application(GTK_WINDOW(window), app);

  g_object_unref(builder);
  return GTK_WIDGET(window);
}
