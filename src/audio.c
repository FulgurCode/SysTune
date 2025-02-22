#include "option/audio.h"
#include <gtk/gtk.h>

GtkWidget *AudioPage;

void change_panel_to_audio(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  audio_to_stack(stack);
  gtk_stack_set_visible_child_name(stack, "audio_page");
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

  gtk_stack_add_named(stack, AudioPage, "audio_page");
  g_object_unref(audio_builder);
}
