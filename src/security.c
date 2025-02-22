#include "option/security.h"
#include <gtk/gtk.h>

GtkWidget *SecurityPage;

void change_panel_to_security(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  security_to_stack(stack);
  gtk_stack_set_visible_child_name(stack, "security_page");
}

static void security_to_stack(GtkStack *stack) {
  if (SecurityPage) {
    return;
  }

  GtkBuilder *security_builder = gtk_builder_new_from_file("ui/security_settings.ui");
  if (security_builder == NULL) {
    g_printerr("Failed to load security.ui\n");
    return;
  }

  SecurityPage = GTK_WIDGET(gtk_builder_get_object(security_builder, "security_settings_page"));
  if (SecurityPage == NULL) {
    g_printerr("Failed to get security_page from security.ui\n");
    g_object_unref(security_builder);
    return;
  }

  gtk_stack_add_named(stack, SecurityPage, "security_page");
  g_object_unref(security_builder);
}
