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

#ifndef _FSBROWSER_H
#define _FSBROWSER_H

#include <stdio.h>
#include <regex.h>

#define FS_BROWSER_TYPE (fs_browser_get_type ())
#define FS_BROWSER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                        FS_BROWSER_TYPE, FsBrowser))
#define FS_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
                        FS_BROWSER_TYPE, FsBrowserClass))
#define IS_FS_BROWSER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                        FS_BROWSER_TYPE))
#define IS_FS_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                        FS_BROWSER_TYPE))


typedef struct _FsBrowserClass {
	GtkVBoxClass parent_class;

	void (*completed) (GtkWidget * self, gpointer data);
} FsBrowserClass;

typedef struct _FsBrowser {
	GtkVBox vbox;

	GtkWidget *entry;
	GtkWidget *view;

	gchar *mime_command;
	gboolean mime_check;

	gboolean active;
	GtkWidget *togglerecent;

	GtkWidget *closebutton;

	gchar path[FILENAME_MAX];
	gchar *dir_icon;
	GdkPixbuf *dir_pixbuf;
	gboolean dot_files;

	GList *recent_files;
} FsBrowser;

GType fs_browser_get_type ();
GtkWidget *fs_browser_new ();
GtkWidget *fs_browser_get_recent_files_menu (FsBrowser *browser);
int fs_browser_read_dir (FsBrowser *browser);

#endif /* _FSBROWSER_H */
