/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _MENU_H
#define _MENU_H

#include <gtk/gtk.h>

#define MENU_TYPE            (menu_get_type ())
#define MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                            MENU_TYPE, Menu))
#define MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),\
                            MENU_TYPE, MenuClass))
#define IS_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                            MENU_TYPE))
#define IS_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                            MENU_TYPE))

#define COLUMNS_COUNT 10

struct MenuStart;

struct user_action
{
	gchar *icon_path;
	GdkPixbuf *icon;
	gchar *label;
	gchar *app;
};

typedef struct Menu {
	GtkHBox box;

	GList *user_actions;
	unsigned int atime;

	int columns;
	GtkWidget *column_boxes[COLUMNS_COUNT];
	GtkWidget *column_eboxes[COLUMNS_COUNT];

	int user_apps_count;

	int r_apps_count;
	gchar *set_app;
	gchar *term_app;
	gchar *run_app;

	GtkWidget *menu;
	unsigned int time;

	gboolean grab;

	GtkWidget *rbox;
	GtkWidget *rebox;

	GtkWidget *lbox;
	GtkWidget *lebox;

	GtkWidget *recentfilesbutton;
	GtkWidget *fsbrowserbutton;
	GtkWidget *fstabbutton;
} Menu;

typedef struct MenuClass {
	GtkHBoxClass parent_class;

	void (*completed) (GtkWidget *self, gpointer data);
	void (*getgrab) (GtkWidget *self, gpointer data);
} MenuClass;

GType menu_get_type ();
GtkWidget *menu_new ();
void menu_repack_recent_apps (Menu *menu);
void menu_repack_user_apps (Menu *menu);
void show_menu_widget (GtkWidget *widget);

#endif /* _MENU_H */
