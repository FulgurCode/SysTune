#include "option/audio.h"
#include "command/command.h"
#include <adwaita.h>
#include <gtk/gtk.h>
#include <stdio.h>

#define MAX_SINKS 16
#define MAX_DESC_LENGTH 256

GtkWidget *AudioPage;

typedef struct {
  uint32_t index;
  char description[256];
} SinkInfo;

SinkInfo *sinks;    // Array to store sink information
int sink_count = 0; // Counter for the number of sinks

void change_panel_to_audio(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  audio_to_stack(stack);
  gtk_stack_set_visible_child_name(stack, "audio_page");
}

void get_audio_sources(GtkStringList *sink_list) {
  char *output =
      execute_command("pactl list sinks | grep -E \"Sink #|Description:\" | "
                      "awk '/Sink #/{idx=$2} /Description:/{print idx, $0}'");

  sinks = malloc(10 * sizeof(SinkInfo));

  char *line = strtok(output, "\n"); // Split output into lines
  while (line) {
    SinkInfo s;
    sscanf(line, "#%d Description: %[^\n]", &s.index, &s.description);
    gtk_string_list_append(sink_list, s.description);
    line = strtok(NULL, "\n"); // Split output into lines
    sinks[sink_count] = s;
    sink_count++;
    g_print("%s", line);
  }
}

// Callback function when output device selection changes
void on_output_device_changed(AdwComboRow *combo, gpointer user_data) {
  GtkStringList *sink_list = GTK_STRING_LIST(adw_combo_row_get_model(combo));
  guint selected_index = adw_combo_row_get_selected(combo);

  if (selected_index == GTK_INVALID_LIST_POSITION) {
    g_print("No device selected.\n");
    return;
  }

  int index = sinks[selected_index].index;

  char command[512];
  sprintf(command, "pactl set-default-sink %d", index);
  execute_command(command);
}

static void audio_to_stack(GtkStack *stack) {
  if (AudioPage) {
    return;
  }

  GtkBuilder *audio_builder = gtk_builder_new_from_file("ui/audio.ui");
  if (audio_builder == NULL) {
    g_printerr("Failed to load audio.ui\n");
    return;
  }

  AudioPage = GTK_WIDGET(gtk_builder_get_object(audio_builder, "audio_page"));
  if (AudioPage == NULL) {
    g_printerr("Failed to get audio_page from audio.ui\n");
    g_object_unref(audio_builder);
    return;
  }

  GtkStringList *sink_list =
      GTK_STRING_LIST(gtk_builder_get_object(audio_builder, "audio_sink_list"));

  get_audio_sources(sink_list);

  AdwComboRow *combo =
      ADW_COMBO_ROW(gtk_builder_get_object(audio_builder, "output_device"));

  // Connect the selection change event
  g_signal_connect(combo, "notify::selected",
                   G_CALLBACK(on_output_device_changed), NULL);

  gtk_stack_add_named(stack, AudioPage, "audio_page");
  g_object_unref(audio_builder);
}
