#include "option/autostart.h"
#include <adwaita.h>
#include <errno.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AUTOSTART_SCRIPT "autostart.sh"
#define HYPRLAND_CONF "hyprland.conf"
#define AUTOSTART_MARKER "# Autostart script managed by autostart manager"
#define AUTOSTART_EXEC "exec = ~/.config/hypr/autostart.sh"

static GtkWidget *AutostartPage = NULL;
static GtkListBox *AutostartList = NULL;

typedef struct {
  char *name;
  char *exec;
  char *description;
  gboolean enabled;
} AutostartApp;

static char *get_config_dir(void) {
  const char *home_dir = g_get_home_dir();
  return g_build_filename(home_dir, ".config", "hypr", NULL);
}

static char *get_autostart_script_path(void) {
  char *config_dir = get_config_dir();
  char *script_path = g_build_filename(config_dir, AUTOSTART_SCRIPT, NULL);
  g_free(config_dir);
  return script_path;
}

static char *get_hyprland_conf_path(void) {
  char *config_dir = get_config_dir();
  char *conf_path = g_build_filename(config_dir, HYPRLAND_CONF, NULL);
  g_free(config_dir);
  return conf_path;
}

static void ensure_autostart_script(void) {
  char *script_path = get_autostart_script_path();

  // Create config directory if it doesn't exist
  char *config_dir = get_config_dir();
  g_mkdir_with_parents(config_dir, 0755);
  g_free(config_dir);

  // Create script if it doesn't exist
  if (!g_file_test(script_path, G_FILE_TEST_EXISTS)) {
    FILE *f = fopen(script_path, "w");
    if (f) {
      fprintf(f, "#!/bin/bash\n\n# Autostart entries will be added here\n");
      fclose(f);
      chmod(script_path, 0755); // Make executable
    }
  }
  g_free(script_path);
}

static void ensure_hyprland_conf_entry(void) {
  char *conf_path = get_hyprland_conf_path();
  gboolean entry_exists = FALSE;

  if (g_file_test(conf_path, G_FILE_TEST_EXISTS)) {
    gchar *contents = NULL;
    if (g_file_get_contents(conf_path, &contents, NULL, NULL)) {
      if (strstr(contents, AUTOSTART_MARKER) != NULL) {
        entry_exists = TRUE;
      }
      g_free(contents);
    }
  }

  if (!entry_exists) {
    FILE *f = fopen(conf_path, G_FILE_TEST_EXISTS ? conf_path : "w+");
    if (f) {
      fprintf(f, "\n%s\n%s\n", AUTOSTART_MARKER, AUTOSTART_EXEC);
      fclose(f);
    }
  }
  g_free(conf_path);
}

static void add_to_autostart_script(const char *command) {
  char *script_path = get_autostart_script_path();
  FILE *f = fopen(script_path, "a");
  if (f) {
    fprintf(f, "%s &\n", command);
    fclose(f);
  }
  g_free(script_path);
}

static void remove_from_autostart_script(const char *command) {
  char *script_path = get_autostart_script_path();
  gchar *contents = NULL;

  if (g_file_get_contents(script_path, &contents, NULL, NULL)) {
    GString *new_contents = g_string_new("");
    gchar **lines = g_strsplit(contents, "\n", -1);

    for (int i = 0; lines[i] != NULL; i++) {
      if (strstr(lines[i], command) == NULL) {
        g_string_append(new_contents, lines[i]);
        g_string_append(new_contents, "\n");
      }
    }

    g_file_set_contents(script_path, new_contents->str, -1, NULL);
    g_string_free(new_contents, TRUE);
    g_strfreev(lines);
    g_free(contents);
  }
  g_free(script_path);
}

static void on_app_switch_toggled(GtkSwitch *widget, gboolean state,
                                  AutostartApp *app) {
  if (state) {
    add_to_autostart_script(app->exec);
  } else {
    remove_from_autostart_script(app->exec);
  }
}

static void add_app_to_list(AutostartApp *app) {
  GtkWidget *row = GTK_WIDGET(adw_action_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), app->name);
  if (app->description)
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), app->description);

  GtkWidget *toggle = gtk_switch_new();
  gtk_switch_set_active(GTK_SWITCH(toggle), app->enabled);
  g_signal_connect(toggle, "state-set", G_CALLBACK(on_app_switch_toggled), app);
  adw_action_row_add_suffix(ADW_ACTION_ROW(row), toggle);
  gtk_list_box_append(AutostartList, row);
}

static void on_app_chosen(GtkNativeDialog *dialog, gint response,
                          gpointer user_data) {
  if (response == GTK_RESPONSE_ACCEPT) {
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    GFile *file = gtk_file_chooser_get_file(chooser);
    char *path = g_file_get_path(file);
    char *name = g_path_get_basename(path);

    AutostartApp *app = g_new0(AutostartApp, 1);
    app->name = name;
    app->exec = path;
    app->enabled = TRUE;

    add_to_autostart_script(app->exec);
    add_app_to_list(app);

    g_object_unref(file);
  }
  g_object_unref(dialog);
}

static void on_browse_clicked(GtkButton *button, gpointer user_data) {
  GtkFileChooserNative *dialog = gtk_file_chooser_native_new(
      "Choose Application", GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(button))),
      GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");

  GtkFileFilter *filter = gtk_file_filter_new();
  gtk_file_filter_add_mime_type(filter, "application/x-executable");
  gtk_file_filter_set_name(filter, "Executable files");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

  filter = gtk_file_filter_new();
  gtk_file_filter_add_pattern(filter, "*");
  gtk_file_filter_set_name(filter, "All files");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

  g_signal_connect(dialog, "response", G_CALLBACK(on_app_chosen), NULL);
  gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
}

typedef struct {
  GtkWidget *dialog;
  GtkWidget *name_entry;
  GtkWidget *command_entry;
  GtkWidget *desc_entry;
} CustomDialogData;

static void on_custom_add_clicked(GtkButton *button, gpointer user_data) {
  CustomDialogData *data = user_data;
  const char *name_text = gtk_editable_get_text(GTK_EDITABLE(data->name_entry));
  const char *command_text =
      gtk_editable_get_text(GTK_EDITABLE(data->command_entry));
  const char *desc_text = gtk_editable_get_text(GTK_EDITABLE(data->desc_entry));

  if (command_text && *command_text != '\0') {
    AutostartApp *app = g_new0(AutostartApp, 1);
    app->name = g_strdup(name_text && *name_text != '\0' ? name_text
                                                         : "Custom Command");
    app->exec = g_strdup(command_text);
    app->description = g_strdup(desc_text);
    app->enabled = TRUE;

    add_to_autostart_script(app->exec);
    add_app_to_list(app);
  }

  gtk_window_destroy(GTK_WINDOW(data->dialog));
  g_free(data);
}

static void on_custom_command_clicked(GtkButton *button, gpointer user_data) {
  GtkWidget *dialog = GTK_WIDGET(adw_dialog_new());
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(box, 24);
  gtk_widget_set_margin_end(box, 24);
  gtk_widget_set_margin_top(box, 24);
  gtk_widget_set_margin_bottom(box, 24);

  GtkWidget *name_entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(name_entry), "Application Name");

  GtkWidget *command_entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(command_entry), "Command");

  GtkWidget *desc_entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(desc_entry),
                                 "Description (optional)");

  GtkWidget *add_button = gtk_button_new_with_label("Add");
  gtk_widget_add_css_class(add_button, "suggested-action");

  gtk_box_append(GTK_BOX(box), name_entry);
  gtk_box_append(GTK_BOX(box), command_entry);
  gtk_box_append(GTK_BOX(box), desc_entry);
  gtk_box_append(GTK_BOX(box), add_button);

  adw_dialog_set_child(ADW_DIALOG(dialog), box);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(button));

  CustomDialogData *data = g_new0(CustomDialogData, 1);
  data->dialog = dialog;
  data->name_entry = name_entry;
  data->command_entry = command_entry;
  data->desc_entry = desc_entry;

  g_signal_connect(add_button, "clicked", G_CALLBACK(on_custom_add_clicked),
                   data);
}

static void load_autostart_entries(void) {
  char *script_path = get_autostart_script_path();

  if (g_file_test(script_path, G_FILE_TEST_EXISTS)) {
    gchar *contents = NULL;
    if (g_file_get_contents(script_path, &contents, NULL, NULL)) {
      gchar **lines = g_strsplit(contents, "\n", -1);

      for (int i = 0; lines[i] != NULL; i++) {
        if (strlen(lines[i]) > 0 && lines[i][0] != '#') {
          // Remove trailing '&' if present
          char *cmd = g_strdup(lines[i]);
          size_t len = strlen(cmd);
          if (len > 0 && cmd[len - 1] == '&') {
            cmd[len - 1] = '\0';
            g_strstrip(cmd);
          }

          AutostartApp *app = g_new0(AutostartApp, 1);
          app->exec = g_strdup(cmd);
          char **argv = g_strsplit(cmd, " ", 2);
          app->name = g_path_get_basename(argv[0]);
          app->enabled = TRUE;
          add_app_to_list(app);

          g_strfreev(argv);
          g_free(cmd);
        }
      }

      g_strfreev(lines);
      g_free(contents);
    }
  }
  g_free(script_path);
}

void autostart_to_stack(GtkStack *stack) {
  if (AutostartPage)
    return;

  // Ensure our autostart infrastructure exists
  ensure_autostart_script();
  ensure_hyprland_conf_entry();

  const char *ui_paths[] = {"ui/autostart_apps.ui",
                            "/usr/local/share/systune/ui/autostart_apps.ui"};

  GtkBuilder *builder = NULL;
  size_t num_paths = sizeof(ui_paths) / sizeof(ui_paths[0]);

  for (size_t i = 0; i < num_paths; i++) {
    if (g_file_test(ui_paths[i], G_FILE_TEST_EXISTS)) {
      builder = gtk_builder_new_from_file(ui_paths[i]);
      if (builder != NULL) {
        g_print("Loaded UI file from: %s\n", ui_paths[i]);
        break;
      }
    }
  }

  if (builder == NULL) {
    g_warning(
        "UI file 'autostart_apps.ui' not found in any of the expected locations");
    return;
  }

  AutostartPage =
      GTK_WIDGET(gtk_builder_get_object(builder, "autostart_apps_page"));
  if (!AutostartPage) {
    g_printerr("Failed to get autostart_apps_page from UI file\n");
    g_object_unref(builder);
    return;
  }

  AutostartList =
      GTK_LIST_BOX(gtk_builder_get_object(builder, "autostart_list"));
  GtkWidget *browse_button =
      GTK_WIDGET(gtk_builder_get_object(builder, "browse_button"));
  GtkWidget *custom_button =
      GTK_WIDGET(gtk_builder_get_object(builder, "custom_command_button"));

  g_signal_connect(browse_button, "clicked", G_CALLBACK(on_browse_clicked),
                   NULL);
  g_signal_connect(custom_button, "clicked",
                   G_CALLBACK(on_custom_command_clicked), NULL);

  load_autostart_entries();

  gtk_stack_add_named(stack, AutostartPage, "autostart_page");
  g_object_unref(builder);
}

void change_panel_to_autostart(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  autostart_to_stack(stack);
  gtk_stack_set_visible_child_name(stack, "autostart_page");
}
