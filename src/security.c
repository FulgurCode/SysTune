#include <adwaita.h>
#include <gtk/gtk.h>
#include "option/security.h"
#include "command/command.h"

GtkWidget *SecurityPage;

void change_panel_to_security(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  security_to_stack(stack);
  gtk_stack_set_visible_child_name(stack, "security_page");
}

void enable_firewall() { execute_command("pkexec ufw enable"); }

void disable_firewall() { execute_command("pkexec ufw disable"); }

static void on_firewall_switch_activated(GObject *object, GParamSpec *pspec,
                                         gpointer user_data) {
  AdwSwitchRow *switch_row = ADW_SWITCH_ROW(object);

  if (adw_switch_row_get_active(switch_row)) {
    // Enable the firewall
    enable_firewall();
  } else {
    // Disable the firewall
    disable_firewall();
  }
}

static void security_to_stack(GtkStack *stack) {
  if (SecurityPage) {
    return;
  }

  GtkBuilder *security_builder =
      gtk_builder_new_from_file("ui/security_settings.ui");
  if (security_builder == NULL) {
    g_printerr("Failed to load security.ui\n");
    return;
  }

  SecurityPage = GTK_WIDGET(
      gtk_builder_get_object(security_builder, "security_settings_page"));
  if (SecurityPage == NULL) {
    g_printerr("Failed to get security_page from security.ui\n");
    g_object_unref(security_builder);
    return;
  }

  // Get the AdwSwitchRow from the UI file
  AdwSwitchRow *firewall_switch = ADW_SWITCH_ROW(
      gtk_builder_get_object(security_builder, "firewall_switch"));

  // Connect the switch's "notify::active" signal to the callback
  g_signal_connect(firewall_switch, "notify::active",
                   G_CALLBACK(on_firewall_switch_activated), NULL);

  gtk_stack_add_named(stack, SecurityPage, "security_page");
  g_object_unref(security_builder);
}
