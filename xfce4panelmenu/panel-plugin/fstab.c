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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <gtk/gtk.h>

#include <libxfcegui4/xfce-exec.h>

#include "fstab.h"

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

static void fs_tab_class_init (FsTabWidgetClass *klass);
static void fs_tab_init (FsTabWidget *ft);
static GtkWidget *create_button (gchar * stock_id, gchar * text,
				 GCallback callback, gpointer data);
static GtkTreeModel *create_model (FsTabWidget *ft);
static void update_model (FsTabWidget *ft);
static void mount_button_click (GtkWidget *self, gpointer data);
static void row_activated (GtkTreeView *self, GtkTreePath *path,
			   GtkTreeViewColumn *column, gpointer data);

static guint fs_tab_signals[F_LAST_SIGNAL] = { 0 };

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

	ft->view = gtk_tree_view_new_with_model (model);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (ft->view), FALSE);

	renderer = gtk_cell_renderer_pixbuf_new ();
	g_object_set (G_OBJECT (renderer), "stock-size", 24, NULL);
	column = gtk_tree_view_column_new_with_attributes
		(" ", renderer, "stock-id", ICON, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (ft->view), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes
		(" ", renderer, "text", DEV, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (ft->view), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes
		(" ", renderer, "text", PATH, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (ft->view), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes
		(" ", renderer, "text", FSTYPE, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (ft->view), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes
		(" ", renderer, "text", OPTIONS, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (ft->view), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes
		(" ", renderer, "markup", STATE, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (ft->view), column);

	g_signal_connect (G_OBJECT (ft->view), "row-activated",
			  G_CALLBACK (row_activated), ft);


	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
					     GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (scroll), ft->view);

	gtk_box_pack_start (GTK_BOX (ft), scroll, TRUE, TRUE, 0);

	hbox = gtk_hbox_new (TRUE, 0);

	button = create_button ("gtk-harddisk", "Mount/Unmount", NULL, NULL);
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (mount_button_click), ft);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

	ft->closebutton = create_button ("gtk-close", "Close", NULL, NULL);
	gtk_box_pack_start (GTK_BOX (hbox), ft->closebutton, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (ft), hbox, FALSE, FALSE, 0);
}

GtkWidget *fs_tab_widget_new ()
{
	GtkWidget *ft;

	ft = GTK_WIDGET (g_object_new (fs_tab_widget_get_type (), NULL));

	return ft;
}

static GtkWidget *create_button (gchar * stock_id, gchar * text,
				 GCallback callback, gpointer data)
{
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *button_hbox;
	GtkWidget *image;

	button = gtk_button_new ();
	image = gtk_image_new_from_stock (stock_id,
					  GTK_ICON_SIZE_LARGE_TOOLBAR);
	label = gtk_label_new (text);
	button_hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (button_hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (button_hbox), label, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (button), button_hbox);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	if (callback)
		g_signal_connect (G_OBJECT (button), "clicked",
				  G_CALLBACK (callback), data);

	return button;
}

static GtkTreeModel *create_model (FsTabWidget *ft)
{
	GtkListStore *list;
	GtkTreeIter iter;
	FILE *file;
	char line[4096];
	char fsopt[1024];
	char name[FILENAME_MAX];
	char dev[FILENAME_MAX];
	char fs[32];
	char opt[128];

	list = gtk_list_store_new (COLUMNS,
				   G_TYPE_STRING,
				   G_TYPE_STRING,
				   G_TYPE_BOOLEAN,
				   G_TYPE_STRING,
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

			response = sscanf (line, "%s%s%s%s", dev, name, fs, opt);

			if (response != 4) {
				continue;
			}

			gtk_list_store_append (list, &iter);

			sprintf (line, "%s\t \n%s", dev, name);

			sprintf (fsopt, "%s\n<i>%s</i>", fs, opt);

			if (strcmp (fs, "iso9660") == 0) {
				stock_id = "gtk-cdrom";
			} else if (strcmp (fs, "cd9660") == 0){
				stock_id = "gtk-cdrom";
			} else if (strcmp (fs, "swap") == 0){
				stock_id = "gtk-convert";
			} else if (strcmp (fs, "proc") == 0){
				stock_id = "gtk-index";
			} else if (strcmp (fs, "nfs") == 0){
				stock_id = "gtk-network";
			} else {
				stock_id = "gtk-harddisk";
			}

			sprintf (line, "\t%s", name);
			gtk_list_store_set (list, &iter, PATH, line, -1);

			sprintf (line, "%s", dev);
			gtk_list_store_set (list, &iter, DEV, line, -1);

			sprintf (line, "\t%s", fs);
			gtk_list_store_set (list, &iter, FSTYPE, line, -1);

			sprintf (line, "\t%s", opt);
			gtk_list_store_set (list, &iter, OPTIONS, line, -1);

			gtk_list_store_set (list, &iter,
					    STATE, "\t<i>not mounted</i>",
					    ICON, stock_id,
					    -1);
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
	GList *mounted = NULL, *tmp = NULL;
	char *mdev;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ft->view));

	if (gtk_tree_model_get_iter_first (model, &iter)) {

		file = fopen ("/etc/mtab", "r");
		if (!file) {
			return;
		}

		while (fgets (line, 4096, file)) {
			if (line[0] != '#') {
				sscanf (line, "%s", dev);
				mounted = g_list_append (mounted, strdup (dev));
			}
		}

		fclose (file);

		do {
			gtk_tree_model_get (model, &iter, DEV, &mdev, -1);
			for (tmp = mounted; tmp; tmp = tmp->next) {
				if (strcmp (mdev, (char *) tmp->data) == 0) {
					gtk_list_store_set
						(GTK_LIST_STORE (model), &iter,
						 STATE, "\t<i>mounted</i>",
						 MOUNTED, TRUE,
						 -1);
					break;
				}
			}
			if (!tmp) {
				gtk_list_store_set (GTK_LIST_STORE (model), &iter,
						    STATE, "\t<i>not mounted</i>",
						    MOUNTED, FALSE,
						    -1);
			}
		} while (gtk_tree_model_iter_next (model, &iter));

		for (tmp = mounted; tmp; tmp = tmp->next) {
			free (tmp->data);
		}
		g_list_free (mounted);
	}
}

void fs_tab_widget_update (FsTabWidget *ft)
{
	update_model (ft);
}

static void mount_button_click (GtkWidget *self, gpointer data)
{
	FsTabWidget *ft = (FsTabWidget *) data;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ft->view));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ft->view));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
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

	g_signal_emit_by_name (ft, "completed");
}

static void row_activated (GtkTreeView *self, GtkTreePath *path,
			   GtkTreeViewColumn *column, gpointer data)
{
	FsTabWidget *ft = (FsTabWidget *) data;
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ft->view));

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

	g_signal_emit_by_name (ft, "completed");
}
