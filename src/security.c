#include "option/security.h"
#include "command/command.h"
#include <adwaita.h>
#include <gtk/gtk.h>

GtkWidget *SecurityPage;

void change_panel_to_security(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  security_to_stack(stack);
  gtk_stack_set_visible_child_name(stack, "security_page");
}

void enable_firewall() { execute_command("pkexec ufw enable"); }

void disable_firewall() { execute_command("pkexec ufw disable"); }

static void on_ssh_switch_activated(GObject *object, GParamSpec *pspec,
                                    gpointer user_data) {
  GtkSwitch *ssh_switch = GTK_SWITCH(object);
  if (gtk_switch_get_active(ssh_switch)) {
    execute_command("pkexec ufw allow ssh");
  } else {
    execute_command("pkexec ufw deny ssh");
  }
}

static void on_smtp_switch_activated(GObject *object, GParamSpec *pspec,
                                     gpointer user_data) {
  GtkSwitch *smtp_switch = GTK_SWITCH(object);
  if (gtk_switch_get_active(smtp_switch)) {
    execute_command("pkexec ufw allow smtp");
  } else {
    execute_command("pkexec ufw deny smtp");
  }
}

static void on_vnc_switch_activated(GObject *object, GParamSpec *pspec,
                                    gpointer user_data) {
  GtkSwitch *vnc_switch = GTK_SWITCH(object);
  if (gtk_switch_get_active(vnc_switch)) {
    execute_command("pkexec ufw allow vnc");
  } else {
    execute_command("pkexec ufw deny vnc");
  }
}

static void on_firewall_switch_activated(GObject *object, GParamSpec *pspec,
                                         gpointer user_data) {
  AdwSwitchRow *switch_row = ADW_SWITCH_ROW(object);
  if (adw_switch_row_get_active(switch_row)) {
    enable_firewall();
  } else {
    disable_firewall();
  }
}

static void on_port_switch_activated(GObject *object, GParamSpec *pspec,
                                     gpointer user_data) {
  GtkWidget *port_entry = GTK_WIDGET(user_data);
  const gchar *port_number = gtk_editable_get_text(GTK_EDITABLE(port_entry));
  GtkSwitch *port_switch = GTK_SWITCH(object);

  if (port_number && *port_number) {
    gchar *command;
    if (gtk_switch_get_active(port_switch)) {
      command = g_strdup_printf("pkexec ufw allow %s", port_number);
    } else {
      command = g_strdup_printf("pkexec ufw deny %s", port_number);
    }
    execute_command(command);
    g_free(command);
  }
}

static void security_to_stack(GtkStack *stack) {
  if (SecurityPage) {
    return;
  }

  const char *ui_paths[] = {"ui/security_settings.ui",
                            "/usr/share/systune/ui/security_settings.ui"};

  GtkBuilder *security_builder = NULL;
  size_t num_paths = sizeof(ui_paths) / sizeof(ui_paths[0]);

  for (size_t i = 0; i < num_paths; i++) {
    if (g_file_test(ui_paths[i], G_FILE_TEST_EXISTS)) {
      security_builder = gtk_builder_new_from_file(ui_paths[i]);
      if (security_builder != NULL) {
        g_print("Loaded UI file from: %s\n", ui_paths[i]);
        break;
      }
    }
  }

  if (security_builder == NULL) {
    g_warning("UI file 'security_settings.ui' not found in any of the expected "
              "locations");
    return;
  }

  SecurityPage = GTK_WIDGET(
      gtk_builder_get_object(security_builder, "security_settings_page"));
  if (SecurityPage == NULL) {
    g_printerr("Failed to get security_page from security.ui\n");
    g_object_unref(security_builder);
    return;
  }

  AdwSwitchRow *firewall_switch = ADW_SWITCH_ROW(
      gtk_builder_get_object(security_builder, "firewall_switch"));
  g_signal_connect(firewall_switch, "notify::active",
                   G_CALLBACK(on_firewall_switch_activated), NULL);

  GtkSwitch *ssh_switch =
      GTK_SWITCH(gtk_builder_get_object(security_builder, "ssh_switch"));
  g_signal_connect(ssh_switch, "notify::active",
                   G_CALLBACK(on_ssh_switch_activated), NULL);

  GtkSwitch *vnc_switch =
      GTK_SWITCH(gtk_builder_get_object(security_builder, "vnc_switch"));
  g_signal_connect(vnc_switch, "notify::active",
                   G_CALLBACK(on_vnc_switch_activated), NULL);

  GtkSwitch *smtp_switch =
      GTK_SWITCH(gtk_builder_get_object(security_builder, "smtp_switch"));
  g_signal_connect(smtp_switch, "notify::active",
                   G_CALLBACK(on_smtp_switch_activated), NULL);

  GtkEntry *port_entry =
      GTK_ENTRY(gtk_builder_get_object(security_builder, "port_entry"));
  GtkSwitch *port_switch =
      GTK_SWITCH(gtk_builder_get_object(security_builder, "port_switch"));
  g_signal_connect(port_switch, "notify::active",
                   G_CALLBACK(on_port_switch_activated), port_entry);

  gtk_stack_add_named(stack, SecurityPage, "security_page");
  g_object_unref(security_builder);
}
