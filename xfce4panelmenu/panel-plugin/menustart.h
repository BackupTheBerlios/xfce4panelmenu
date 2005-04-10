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

#ifndef _MENUSTART_H
#define _MENUSTART_H

#include <gtk/gtk.h>

#define MENU_START_TYPE            (menu_start_get_type ())
#define MENU_START(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                    MENU_START_TYPE, MenuStart))
#define MENU_START_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),\
                                    MENU_START_TYPE, MenuStartClass))
#define IS_MENU_START(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                    MENU_START_TYPE))
#define IS_MENU_START_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                                    MENU_START_TYPE))

typedef struct MenuStart {
	GtkWindow window;

	GtkWidget *frame;

	GtkWidget *vbox;

	GtkWidget *header;
	GtkWidget *menu;
	GtkWidget *fstab;
	GtkWidget *fsbrowser;

	gboolean menu_is_shown;

	GtkWidget *title;
	GtkWidget *menu_image;

	GtkWidget *footbox;
	GtkWidget *foot;
	GtkWidget *button_logout;
	GtkWidget *button_lock;

	int width;
	int height;

	char *lock_app;
	char *switch_app;

} MenuStart;

typedef struct MenuStartClass {
	GtkWindowClass parent_class;
} MenuStartClass;

typedef enum {
	MENU_START_TOP,
	MENU_START_BOTTOM
} MenuStartPosition;

GType menu_start_get_type ();
GtkWidget *menu_start_new ();
void menu_start_show (MenuStart * ms, int xpos, int ypos,
		      MenuStartPosition pos);
void menu_start_hide (MenuStart * ms);

GtkWidget *menu_start_get_menu_widget (MenuStart * ms);
GtkWidget *menu_start_get_browser_widget (MenuStart * ms);

gboolean popup_grab_on_window (GdkWindow * window, guint32 activate_time);

#endif /* _MENUSTART_H */
