#ifndef USER_PERMISSIONS_H
#define USER_PERMISSIONS_H

#include <gtk/gtk.h>

extern GtkWidget* User_PermissionsPage;

void change_panel_to_user_permissions(gpointer);
static void user_permissions_to_stack(GtkStack*);

#endif
