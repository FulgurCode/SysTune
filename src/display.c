#include "option/display.h"
#include "command/command.h"
#include <adwaita.h>
#include <gtk/gtk.h>

GtkWidget *DisplayPage;

typedef struct {
  char resolution[32]; // Stores resolution as a string (e.g., "1920x1080")
  float refresh_rate;  // Stores refresh rate (e.g., 59.98)
} DisplayMode;

DisplayMode *DisplayList;

int count;

void change_panel_to_display(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  display_to_stack(stack);
  gtk_stack_set_visible_child_name(stack, "display_page");
}
// Function to parse the xrandr output and update the global DisplayList
void get_available_resalutions(GtkStringList *sink_list) {
  char *result = execute_command("xrandr");
  if (result == NULL) {
    g_print("Failed to execute command.\n");
    return;
  }

  // Parse the result line by line
  char *line = strtok(result, "\n");
  while (line != NULL) {
    // Check if the line contains a resolution (e.g., "1920x1080 60.00")
    if (strstr(line, "x") != NULL && strstr(line, ".") != NULL) {
      char resolution[32];
      float refresh_rate;

      // Parse the resolution and refresh rate
      if (sscanf(line, " %31s %f", resolution, &refresh_rate) == 2) {
        // Resize the global DisplayList to accommodate the new mode
        DisplayList = realloc(DisplayList, (count + 1) * sizeof(DisplayMode));
        if (DisplayList == NULL) {
          perror("realloc");
          free(result);
          return;
        }

        // Store the resolution and refresh rate in the global DisplayList
        strncpy(DisplayList[count].resolution, resolution,
                sizeof(DisplayList[count].resolution) - 1);
        DisplayList[count]
            .resolution[sizeof(DisplayList[count].resolution) - 1] =
            '\0'; // Ensure null-termination
        DisplayList[count].refresh_rate = refresh_rate;

        // Append the resolution to the GTK string list
        gtk_string_list_append(sink_list, DisplayList[count].resolution);

        count++;
      }
    }
    line = strtok(NULL, "\n");
  }

  // Debug: Print the first resolution
  if (count > 0) {
    g_print("res-%s\n", DisplayList[0].resolution);
  }

  free(result);
}

// Callback function for when the slider value changes
static void on_res_change(AdwComboRow *combo, gpointer user_data) {
  GtkStringList *sink_list = GTK_STRING_LIST(adw_combo_row_get_model(combo));
  guint selected_index = adw_combo_row_get_selected(combo);

  if (selected_index == GTK_INVALID_LIST_POSITION) {
    g_print("No device selected.\n");
    return;
  }

  DisplayMode res = DisplayList[selected_index];
  g_print("%s", res.resolution);
  g_print("%f", res.refresh_rate);

  // Construct the xrandr command
  char command[512];
  snprintf(command, sizeof(command), "xrandr -s %s", res.resolution);

  // Execute the command
  execute_command(command);
}

static void on_slider_value_changed(GtkRange *range, gpointer user_data) {
  gdouble value = gtk_range_get_value(range);

  gchar *command = g_strdup_printf("brightnessctl set %.0f%%\n", value);
  g_print("Executing command: %s\n", command);

  GError *error = NULL;
  gint exit_status;
  g_spawn_command_line_sync(command, NULL, NULL, &exit_status, &error);
  g_free(command);

  if (error) {
    g_printerr("Failed to toggle Brigthness: %s\n", error->message);
    g_error_free(error);
  }
}

static void on_file_dialog_response(GtkDialog *dialog, gint response_id,
                                    GtkImage *preview_image) {
  if (response_id == GTK_RESPONSE_ACCEPT) {
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    GFile *file = gtk_file_chooser_get_file(chooser);
    char *file_path = g_file_get_path(file);

    // Set the selected image file to the GtkImage widget
    gtk_image_set_from_file(preview_image, file_path);

    char *display_server = execute_command("echo $XDG_SESSION_TYPE");
    char command[512];
    g_strchomp(display_server);

    if (g_strcmp0(display_server, "wayland") == 0) {
      snprintf(command, sizeof(command), "swww img %s", file_path);
    } else {
      g_print("%s", display_server);
      snprintf(command, sizeof(command), "feh --bg-scale %s", file_path);
    }

    char *result = execute_command(command);

    g_free(file_path);
    g_object_unref(file);
  }

  // Destroy the dialog
  gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_choose_background(GtkButton *button, GtkImage *preview_image) {
  GtkWidget *dialog;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;

  // Create a new file chooser dialog
  dialog = gtk_file_chooser_dialog_new(
      "Open File", GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(button))), action,
      "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);

  // Connect a callback to handle the response asynchronously
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_present(GTK_WINDOW(dialog));

  g_signal_connect(dialog, "response", G_CALLBACK(on_file_dialog_response),
                   preview_image);
}

static void display_to_stack(GtkStack *stack) {
  if (DisplayPage) {
    return;
  }

  GtkBuilder *display_builder = gtk_builder_new_from_file("ui/display.ui");
  if (display_builder == NULL) {
    g_printerr("Failed to load display.ui\n");
    return;
  }

  DisplayPage =
      GTK_WIDGET(gtk_builder_get_object(display_builder, "display_page"));
  if (DisplayPage == NULL) {
    g_printerr("Failed to get display_page from display.ui\n");
    g_object_unref(display_builder);
    return;
  }

  GtkAdjustment *adjustment = GTK_ADJUSTMENT(
      gtk_builder_get_object(display_builder, "adjustment_slider"));
  if (!adjustment) {
    g_printerr("Failed to get GtkAdjustment from display.ui\n");
    g_object_unref(display_builder);
    return;
  }

  GtkWidget *slider =
      GTK_WIDGET(gtk_builder_get_object(display_builder, "slider"));
  if (!slider) {
    g_printerr("Failed to get slider from display.ui\n");
    g_object_unref(display_builder);
    return;
  }

  gtk_range_set_adjustment(GTK_RANGE(slider), adjustment);
  g_signal_connect(slider, "value-changed", G_CALLBACK(on_slider_value_changed),
                   NULL);

  GtkStringList *sink_list = GTK_STRING_LIST(
      gtk_builder_get_object(display_builder, "display_res_sink_list"));

  get_available_resalutions(sink_list);

  AdwComboRow *combo =
      ADW_COMBO_ROW(gtk_builder_get_object(display_builder, "display_res"));

  // Connect the selection change event
  g_signal_connect(combo, "notify::selected", G_CALLBACK(on_res_change), NULL);

  GtkWidget *choose_bg_button =
      GTK_WIDGET(gtk_builder_get_object(display_builder, "choose_bg_button"));
  GtkWidget *preview_image =
      GTK_WIDGET(gtk_builder_get_object(display_builder, "preview_image"));

  // Connect the "clicked" signal of the button to the callback function
  g_signal_connect(choose_bg_button, "clicked",
                   G_CALLBACK(on_choose_background), preview_image);

  gtk_stack_add_named(stack, DisplayPage, "display_page");
  g_object_unref(display_builder);
}
