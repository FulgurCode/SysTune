#ifndef AUDIO_H
#define AUDIO_H

#include <gtk/gtk.h>

extern GtkWidget* AudioPage;

void change_panel_to_audio(gpointer);
static void audio_to_stack(GtkStack*);

#endif
