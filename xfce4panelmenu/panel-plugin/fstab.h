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

#ifndef FS_TAB_WIDGET_H
#define FS_TAB_WIDGET_H

#include <gtk/gtk.h>

#define FS_TAB_WIDGET_TYPE            (fs_tab_widget_get_type ())
#define FS_TAB_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                            FS_TAB_WIDGET_TYPE, FsTabWidget))
#define FS_TAB_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),\
                            FS_TAB_WIDGET_TYPE, FsTabWidgetClass))
#define IS_FS_TAB_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                            FS_TAB_WIDGET_TYPE))
#define IS_FS_TAB_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                            FS_TAB_WIDGET_TYPE))

typedef struct _FsTabWidgetClass
{
	GtkVBoxClass parent_class;

	void (*completed) (GtkWidget *self, gpointer data);
	void (*getgrab) (GtkWidget *self, gpointer data);
} FsTabWidgetClass;

typedef struct _FsTabWidget
{
	GtkVBox vbox;

	GtkWidget *view;

	GtkWidget *closebutton;
} FsTabWidget;

GType fs_tab_widget_get_type ();
GtkWidget *fs_tab_widget_new ();
void fs_tab_widget_update (FsTabWidget *ft);

#endif /* FS_TAB_WIDGET_H */
