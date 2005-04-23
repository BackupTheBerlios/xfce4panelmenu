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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <gtk/gtk.h>

#include <libxfce4util/i18n.h>
#include <libxfcegui4/xfce-exec.h>
#include <libxfcegui4/libxfcegui4.h>

#include "fstab.h"
#include "common.h"

enum {
	F_COMPLETED,
	F_GETGRAB,
	F_LAST_SIGNAL
};

enum {
	PATH = 0,
	DEV,
	MOUNTED,
	ICON,
	FSTYPE,
	OPTIONS,
	MARKUP,
	MARKUP2,
	STATE,
	FS,
	COLUMNS
};

static void          fs_tab_class_init  (FsTabWidgetClass *klass);
static void          fs_tab_init        (FsTabWidget *ft);
static GtkTreeModel *create_model       (FsTabWidget *ft);
static void          update_model       (FsTabWidget *ft);
static void          mount_button_click (GtkWidget *self, gpointer data);
static void          reload_fstab       (GtkWidget *self, gpointer data);
static void          row_activated      (GtkIconView *self, GtkTreePath *path,
					 gpointer data);

static guint fs_tab_signals[F_LAST_SIGNAL] = { 0 };

extern GModule *xfmime_icon_cm;

GType fs_tab_widget_get_type ()
{
	static GType type = 0;

	type = g_type_from_name ("FsTabWidget");

	if (!type) {
		static const GTypeInfo info = {
			sizeof (FsTabWidgetClass),
			NULL,
			NULL,
			(GClassInitFunc) fs_tab_class_init,
			NULL,
			NULL,
			sizeof (FsTabWidget),
			0,
			(GInstanceInitFunc) fs_tab_init
		};
		type = g_type_register_static (GTK_TYPE_VBOX, "FsTabWidget", &info, 0);
	}
	return type;
}

static void fs_tab_class_init (FsTabWidgetClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *) klass;

	fs_tab_signals[F_COMPLETED] =
			g_signal_new ("completed",
				      G_OBJECT_CLASS_TYPE (object_class),
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET (FsTabWidgetClass,
						       completed), NULL, NULL,
				      g_cclosure_marshal_VOID__VOID,
				      G_TYPE_NONE, 0);

	fs_tab_signals[F_GETGRAB] =
			g_signal_new ("getgrab",
				      G_OBJECT_CLASS_TYPE (object_class),
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET (FsTabWidgetClass,
						       getgrab), NULL, NULL,
				      g_cclosure_marshal_VOID__VOID,
				      G_TYPE_NONE, 0);
}

static void fs_tab_init (FsTabWidget *ft)
{
	GtkWidget *hbox;
	GtkWidget *scroll;
	GtkWidget *button;
	GtkTreeModel *model;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	gtk_box_set_homogeneous (GTK_BOX (ft), FALSE);

	model = create_model (ft);

	ft->view = gtk_icon_view_new_with_model (GTK_TREE_MODEL (model));
	g_signal_connect (G_OBJECT (ft->view), "item-activated",
			  G_CALLBACK (row_activated), ft);

	gtk_icon_view_set_markup_column (GTK_ICON_VIEW (ft->view), MARKUP);
	gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (ft->view), ICON);

	gtk_icon_view_set_orientation (GTK_ICON_VIEW (ft->view), GTK_ORIENTATION_HORIZONTAL);
	gtk_icon_view_set_item_width (GTK_ICON_VIEW (ft->view), 250);

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
					     GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (scroll), ft->view);

	gtk_box_pack_start (GTK_BOX (ft), scroll, TRUE, TRUE, 0);

	hbox = gtk_hbox_new (TRUE, 0);

	button = menu_start_create_button ("gtk-harddisk", _("Mount/Unmount"), NULL, NULL);
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (mount_button_click), ft);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

	button = menu_start_create_button ("gtk-refresh", _("Reload fstab"), NULL, NULL);
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (reload_fstab), ft);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

	ft->closebutton = menu_start_create_button ("gtk-close", _("Close"), NULL, NULL);
	gtk_box_pack_start (GTK_BOX (hbox), ft->closebutton, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (ft), hbox, FALSE, FALSE, 0);
}

GtkWidget *fs_tab_widget_new ()
{
	GtkWidget *ft;

	ft = GTK_WIDGET (g_object_new (fs_tab_widget_get_type (), NULL));

	return ft;
}

static GtkTreeModel *create_model (FsTabWidget *ft)
{
	GtkListStore *list;
	GtkTreeIter iter;
	FILE *file;
	char fsopt[1024];
	char name[FILENAME_MAX];
	char dev[FILENAME_MAX];
	char fs[32];
	char opt[128];
	char line[FILENAME_MAX];

	list = gtk_list_store_new (COLUMNS,
				   G_TYPE_STRING,
				   G_TYPE_STRING,
				   G_TYPE_BOOLEAN,
				   GDK_TYPE_PIXBUF,
				   G_TYPE_STRING,
				   G_TYPE_STRING,
				   G_TYPE_STRING,
				   G_TYPE_STRING,
				   G_TYPE_STRING,
				   G_TYPE_STRING);

	file = fopen ("/etc/fstab", "r");

	while (fgets (line, 4096, file)) {
		if (line[0] && line[0] != '#') {
			char *stock_id;
			int response;
			char *markup;
			GdkPixbuf *pixbuf;

			response = sscanf (line, "%s%s%s%s", dev, name, fs, opt);

			if (response != 4) {
				continue;
			}

			if ((strcmp (fs, "swap") == 0)
			    || (strcmp (fs, "proc") == 0)
			    || (strcmp (fs, "devpts") == 0)) {
				continue;
			}

			gtk_list_store_append (list, &iter);

			if (strcmp (fs, "iso9660") == 0) {
				stock_id = xfce_icon_theme_lookup
					(xfce_icon_theme_get_for_screen (NULL),
					 "gnome-dev-cdrom", 48);
				//stock_id = "stock_cdrom.png";
			} else if (strcmp (fs, "cd9660") == 0){
				stock_id = xfce_icon_theme_lookup
					(xfce_icon_theme_get_for_screen (NULL),
					 "gnome-dev-cdrom", 48);
				//stock_id = "stock_cdrom.png";
			} else if (strcmp (fs, "swap") == 0){
				stock_id = xfce_icon_theme_lookup
					(xfce_icon_theme_get_for_screen (NULL),
					 "gnome-dev-memory", 48);
				//stock_id = "stock_convert.png";
			} else if (strcmp (fs, "proc") == 0){
				stock_id = xfce_icon_theme_lookup
					(xfce_icon_theme_get_for_screen (NULL),
					 "gnome-dev-memory", 48);
				//stock_id = "stock_index.png";
			} else if (strcmp (fs, "nfs") == 0){
				stock_id = xfce_icon_theme_lookup
					(xfce_icon_theme_get_for_screen (NULL),
					 "gnome-dev-harddisk", 48);
				//stock_id = "stock_network.png";
			} else {
				stock_id = xfce_icon_theme_lookup
					(xfce_icon_theme_get_for_screen (NULL),
					 "gnome-dev-harddisk", 48);
				//stock_id = "stock_harddisk.png";
			}

			//pixbuf = MIME_ICON_create_pixbuf (stock_id);
			pixbuf = gdk_pixbuf_new_from_file (stock_id, NULL);
			if (!pixbuf) {
				pixbuf = gdk_pixbuf_new_from_file_at_size
					(ICONDIR "/xfce4_xicon2.png", 47, 48, NULL);
			}
			gtk_list_store_set (list, &iter,
					    PATH, name,
					    DEV, dev,
					    FSTYPE, fs,
					    OPTIONS, opt,
 					    MARKUP, " ",
					    ICON, pixbuf,
					    -1);
			g_free (stock_id);
		}
	}

	fclose (file);

	return GTK_TREE_MODEL (list);
}

static void update_model (FsTabWidget *ft)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	FILE *file;
	char line[4096];
	char dev[4096];
	char buffer[1024];
	GList *mounted = NULL, *tmp = NULL;
	char *mdev;

	model = gtk_icon_view_get_model (GTK_ICON_VIEW (ft->view));

	if (gtk_tree_model_get_iter_first (model, &iter)) {
		char *path = NULL;

		file = fopen ("/etc/mtab", "r");
		if (!file) {
			char *command;

			path = ms_get_save_file ("mtab");
			command = g_strjoin (" ", "mount -p >", path, NULL);
			system (command); /* FIXME!!! */
			//xfce_exec (command, FALSE, FALSE, NULL);
			file = fopen (path, "r");
			g_free (command);
		}

		while (fgets (line, 4096, file)) {
			if (line[0] != '#') {
				sscanf (line, "%s", dev);
				mounted = g_list_append (mounted, strdup (dev));
			}
		}

		fclose (file);

		if (path) {
			unlink (path);
			g_free (path);
		}

		do {
			char *state;
			char *path, *fs, *opt;
			char *markup;

			gtk_tree_model_get (model, &iter,
					    DEV, &mdev,
					    PATH, &path,
					    FSTYPE, &fs,
					    OPTIONS, &opt,
					    -1);
			for (tmp = mounted; tmp; tmp = tmp->next) {
				if (strcmp (mdev, (char *) tmp->data) == 0) {
					state = g_strdup (_("mounted"));
					gtk_list_store_set
						(GTK_LIST_STORE (model), &iter,
						 MOUNTED, TRUE,
						 -1);
					break;
				} else {
					int count = 0;

					count = readlink (mdev, buffer, 1024);
					if (count > 0) {
						buffer[count] = '\0';
						if (strcmp ((char *) tmp->data, buffer) == 0) {
							state = g_strdup (_("mounted"));
							gtk_list_store_set
								(GTK_LIST_STORE (model), &iter,
								 MOUNTED, TRUE,
								 -1);
							break;
						}
					}
				}
			}
			if (!tmp) {
				state = g_strdup (_("not mounted"));
				gtk_list_store_set (GTK_LIST_STORE (model), &iter,
						    MOUNTED, FALSE,
						    -1);
			}
			markup = g_strjoin
				("",
				 "<i><tt>", _("dev  :"), " </tt></i><b>", mdev, "</b>\n",
				 "<i><tt>", _("dir  :"), " </tt></i><b>", path, "</b>\n",
				 "<i><tt>", _("fs   :"), " </tt></i>", fs, "\n",
				 "<i><tt>", _("opt  :"), " </tt></i>", opt, "\n",
				 "<i><tt>", _("state:"), " </tt></i><i>", state, "</i>\n",
				 NULL);
			g_free (state);

			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    MARKUP, markup,
					    -1);
		} while (gtk_tree_model_iter_next (model, &iter));

		for (tmp = mounted; tmp; tmp = tmp->next) {
			free (tmp->data);
		}
		g_list_free (mounted);
	}
}

static void reload_fstab (GtkWidget *self, gpointer data)
{
	FsTabWidget *ft = (FsTabWidget *) data;
	GtkTreeModel *model;

	model = gtk_icon_view_get_model (GTK_ICON_VIEW (ft->view));
	gtk_list_store_clear (GTK_LIST_STORE (model));

	gtk_icon_view_set_model (GTK_ICON_VIEW (ft->view), create_model (ft));
	update_model (ft);
}

void fs_tab_widget_update (FsTabWidget *ft)
{
	update_model (ft);
}

static void mount_button_click (GtkWidget *self, gpointer data)
{
      	FsTabWidget *ft = (FsTabWidget *) data;
	GtkTreeModel *model;
	GList *selected, *tmp;
	GtkTreeIter iter;

	model = gtk_icon_view_get_model (GTK_ICON_VIEW (ft->view));
	selected = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (ft->view));

	for (tmp = selected; tmp; tmp = tmp->next) {
		if (gtk_tree_model_get_iter (model, &iter, tmp->data)) {
			char *dev;
			gboolean mounted;
			int fds[2];
			pid_t pid;

			g_signal_emit_by_name (ft, "completed");

			gtk_tree_model_get (model, &iter,
					    DEV, &dev,
					    MOUNTED, &mounted,
					    -1);
			pipe (fds);
			pid = fork ();
			if (pid) {/* parent */
				char message[1024];
				GtkWidget *dialog;
				int len = 0;
				int r = 0;

				close (fds[1]);
				while ((r = read (fds[0], message + len, 1024 - len))) {
					len = r + len;
				}
				message[len] = '\0';

				if (len > 0) {
					dialog = gtk_message_dialog_new
						(NULL, GTK_DIALOG_MODAL,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 message);
					gtk_dialog_run (GTK_DIALOG (dialog));
					gtk_widget_destroy (dialog);
				}
			} else {
				close (fds[0]);
				dup2 (fds[1], STDERR_FILENO);
				if (mounted) {
					execlp ("umount", "umount", dev, NULL);
				} else {
					execlp ("mount", "mount", dev, NULL);
				}
				exit (1);
			}	
		}
	}

	g_list_foreach (selected, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (selected);
}

static void row_activated (GtkIconView *self, GtkTreePath *path,
			   gpointer data)
{
	FsTabWidget *ft = (FsTabWidget *) data;
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = gtk_icon_view_get_model (GTK_ICON_VIEW (ft->view));

	g_signal_emit_by_name (ft, "completed");

	if (gtk_tree_model_get_iter (model, &iter, path)) {
		char *dev;
		gboolean mounted;
		int fds[2];
		pid_t pid;

		gtk_tree_model_get (model, &iter,
				    DEV, &dev,
				    MOUNTED, &mounted,
				    -1);

		g_signal_emit_by_name (ft, "completed");

		pipe (fds);
		pid = fork ();
		if (pid) {/* parent */
			char message[1024];
			GtkWidget *dialog;
			int len = 0;
			int r = 0;

			close (fds[1]);
			while ((r = read (fds[0], message + len, 1024 - len))) {
				len = r + len;
			}
			message[len] = '\0';

			if (len > 0) {
				dialog = gtk_message_dialog_new
					(NULL, GTK_DIALOG_MODAL,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 message);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
			}
			waitpid (pid, NULL, 0);
		} else {
			close (fds[0]);
			dup2 (fds[1], STDERR_FILENO);
			if (mounted) {
				execlp ("umount", "umount", dev, NULL);
			} else {
				execlp ("mount", "mount", dev, NULL);
			}
			exit (1);
		}
	}
}
