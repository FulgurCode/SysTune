#include "option/user_permissions.h"
#include <gtk/gtk.h>

GtkWidget *User_PermissionsPage;

void change_panel_to_user_permissions(gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  user_permissions_to_stack(stack);
  gtk_stack_set_visible_child_name(stack, "user_permissions_page");
}

static void user_permissions_to_stack(GtkStack *stack) {
  if (User_PermissionsPage) {
    return;
  }

  GtkBuilder *user_permissions_builder = gtk_builder_new_from_file("ui/user_permissions.ui");
  if (user_permissions_builder == NULL) {
    g_printerr("Failed to load user_permissions.ui\n");
    return;
  }

  User_PermissionsPage = GTK_WIDGET(gtk_builder_get_object(user_permissions_builder, "user_permissions_page"));
  if (User_PermissionsPage == NULL) {
    g_printerr("Failed to get user_permissions_page from user_permissions.ui\n");
    g_object_unref(user_permissions_builder);
    return;
  }

  gtk_stack_add_named(stack, User_PermissionsPage, "user_permissions_page");
  g_object_unref(user_permissions_builder);
}
