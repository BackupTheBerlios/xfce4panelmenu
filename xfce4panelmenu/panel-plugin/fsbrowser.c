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

#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <wordexp.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <gtk/gtk.h>

#include <gmodule.h>

#include <xfce4-modules/mime.h>
#include <xfce4-modules/mime_icons.h>

#include "fsbrowser.h"
#include "common.h"

enum {
	COMPLETED,
	LAST_SIGNAL
};

#define ADD_ENDING_SLASH \
		if (s_path[s_path_len - 1] != '/') { \
			s_path[s_path_len] = '/';        \
			s_path[s_path_len + 1] = '\0';   \
			s_path_len++;                    \
		}

static char s_path[FILENAME_MAX];
static int s_path_len;

static void fs_browser_class_init (FsBrowserClass *klass);
static void fs_browser_init (FsBrowser *fb);
static void fs_browser_destroy (GtkObject *object);
static int fs_browser_read_dir (FsBrowser *browser);

static GtkVBoxClass *parent_class = NULL;
static guint fs_browser_signals[LAST_SIGNAL] = { 0 };

static GModule *xfmime_cm = NULL;
xfmime_functions *xfmime_fun = NULL;

static GModule *xfmime_icon_cm = NULL;
xfmime_icon_functions *xfmime_icon_fun = NULL;
gchar *icon_theme_name=NULL;

enum {
	NAME_COLUMN = 0,
	DESC_COLUMN,
	ICON_COLUMN,
	MIME_COLUMN,
	TYPE_COLUMN
};

xfmime_functions *load_mime_module ()
{
	gchar  *module;
	xfmime_functions *(*module_init)(void);
    
	if (xfmime_fun) return xfmime_fun;
    
	module = g_module_build_path(MODULEDIR, "xfce4_mime");
    
	xfmime_cm = g_module_open (module, 0);
	if (!xfmime_cm){
		g_error("%s\n",g_module_error ());
		exit(1);
	}
    
	if (!g_module_symbol (xfmime_cm, "module_init",(gpointer *) &(module_init)) ) {
		g_error("g_module_symbol(module_init) != FALSE\n");
		exit(1);
	}
    
	xfmime_fun = (*module_init)();
    
#if 0         
	/* these functions are imported from the module */
	if (!g_module_symbol (xfmime_cm, "mime_typeinfo",
			      (gpointer *) &(xfmime_fun->mime_typeinfo)) ||
	    !g_module_symbol (xfmime_cm, "mime_key_type",
			      (gpointer *) &(xfmime_fun->mime_key_type)) ||
	    !g_module_symbol (xfmime_cm, "mime_get_type",
			      (gpointer *) &(xfmime_fun->mime_get_type)) ||
	    !g_module_symbol (xfmime_cm, "mime_command",
			      (gpointer *) &(xfmime_fun->mime_command)) ||
	    !g_module_symbol (xfmime_cm, "mime_add",
			      (gpointer *) &(xfmime_fun->mime_add)) ||
	    !g_module_symbol (xfmime_cm, "is_valid_command",
			      (gpointer *) &(xfmime_fun->is_valid_command)) ||
	    !g_module_symbol (xfmime_cm, "mime_key_type",
			      (gpointer *) &(xfmime_fun->mime_key_type)) ||
	    !g_module_symbol (xfmime_cm, "mime_apps",
			      (gpointer *) &(xfmime_fun->mime_apps)) ||
	    !g_module_symbol (xfmime_cm, "mk_command_line",
			      (gpointer *) &(xfmime_fun->mk_command_line)) ||
	    ) {
		g_error("g_module_symbol() != FALSE\n");
		exit(1);
	}
#endif
    
	g_message ("module %s successfully loaded", module);
	g_free(module);
	return xfmime_fun;
}

xfmime_icon_functions *load_mime_icon_module ()
{
	gchar  *module;
	xfmime_icon_functions *(*module_init)(void);
    
	if (xfmime_icon_fun) return xfmime_icon_fun;
    
	module = g_module_build_path(MODULEDIR, "xfce4_mime_icons");
    
	xfmime_icon_cm = g_module_open (module, 0);
	if (!xfmime_icon_cm){
		g_error("%s\n",g_module_error ());
		exit(1);
	}
    
	if (!g_module_symbol (xfmime_icon_cm, "module_init",(gpointer *) &(module_init)) ) {
		g_error("g_module_symbol(module_init) != FALSE\n");
		exit(1);
	}
    
	xfmime_icon_fun = (*module_init)();
    
#if 0         
	/* these functions are imported from the module */
	if (!g_module_symbol (xfmime_icon_cm, "mime_icon_get_iconset",
			      (gpointer *) &(xfmime_icon_fun->mime_icon_get_iconset)) ||    
	    !g_module_symbol (xfmime_icon_cm, "mime_icon_add_iconset",
			      (gpointer *) &(xfmime_icon_fun->mime_icon_add_iconset)) ||    
	    !g_module_symbol (xfmime_icon_cm, "mime_icon_load_theme",
			      (gpointer *) &(xfmime_icon_fun->mime_icon_load_theme)) ||    
	    !g_module_symbol (xfmime_icon_cm, "mime_icon_find_pixmap_file",
			      (gpointer *) &(xfmime_icon_fun->mime_icon_find_pixmap_file)) ||    
	    !g_module_symbol (xfmime_icon_cm, "mime_icon_create_pixmap",
			      (gpointer *) &(xfmime_icon_fun->mime_icon_create_pixmap)) ||    
	    !g_module_symbol (xfmime_icon_cm, "mime_icon_create_pixbuf",
			      (gpointer *) &(xfmime_icon_fun->mime_icon_create_pixbuf))    
	    ) {
		g_error("g_module_symbol() != FALSE\n");
		exit(1);
	}
#endif
    
	/*g_message ("module %s successfully loaded", library);	    */
	g_free(module);
	return xfmime_icon_fun;
}

static GList *free_recent_files_end (GList *list)
{
	GList *tmp;

	for (tmp = list; tmp; tmp = tmp->next) {
		g_free (tmp->data);
	}

	g_list_free (list);

	return NULL;
}

static GList *add_recent_file (GList *list, const gchar *path)
{
	GList *tmp;
	int flag = 1;

	while (flag) {
		for (tmp = list; tmp; tmp = tmp->next) {
			if (strcmp (path, (char *) tmp->data) == 0) {
				g_free (tmp->data);
				list = g_list_delete_link (list, tmp);
				flag = 0;
				break;
			}
		}
		if (!flag)
			flag = 1;
		else
			flag = 0;
	}

	return g_list_prepend (list, g_strdup (path));
}

static void show_recent_files (FsBrowser *browser)
{
	GtkListStore *list;
	GtkTreeIter iter;
	GList *tmp = NULL;
	int i = 0;

	browser->active = FALSE;

	gtk_widget_set_sensitive (browser->entry, FALSE);

	list = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (browser->view)));

	gtk_list_store_clear (list);

	for (tmp = browser->recent_files; tmp && (i < 50); tmp = tmp->next) {
		const gchar *str;
		gchar *desc;
		gchar *path;

		path = (gchar *) tmp->data;

		str = MIME_get_type (path, TRUE);
		if (str) {
			desc = g_strjoin ("", path,
					  "\n\t<i>",
					  str, "</i>", NULL);
		} else {
			desc = path;
		}

		gtk_list_store_append (list, &iter);
		gtk_list_store_set (list, &iter,
				    NAME_COLUMN, (gchar *) tmp->data,
				    DESC_COLUMN, desc,
				    -1);
		i++;				    
	}

	if ((i == 50) && (tmp)) {
		tmp->prev->next = NULL;
		free_recent_files_end (tmp);
	}
}

static void hide_recent_files (FsBrowser *browser)
{
	browser->active = TRUE;

	gtk_widget_set_sensitive (browser->entry, TRUE);

	fs_browser_read_dir (browser);
}

static void rfiles_toggled (GtkToggleButton *self, gpointer data)
{
	FsBrowser *browser = (FsBrowser *)  data;

	if (browser->active) {
		show_recent_files (browser);
	} else {
		hide_recent_files (browser);
	}
}

static GList *read_recent_files (void)
{
	GList *files = NULL;
	xmlDocPtr doc = NULL;
	xmlNodePtr node = NULL;
	char *read_file;

	read_file = ms_get_read_file ("recentfiles.xml");

	if (!g_file_test (read_file, G_FILE_TEST_EXISTS)) {
		return NULL;
	}

	doc = xmlParseFile (read_file);
	node = xmlDocGetRootElement (doc);

	for (node = node->children; node; node = node->next) {
		if (xmlStrEqual (node->name, "file")) {
			files = g_list_append (files, xmlGetProp (node, "path"));
		}
	}

	return files;
}

static void write_recent_files (GList *rfiles)
{
	char *save_file;
	xmlDocPtr doc = NULL;
	xmlNodePtr node = NULL, root_node = NULL;

	save_file = ms_get_save_file ("recentfiles.xml");

	doc = xmlNewDoc(BAD_CAST "1.0");
	root_node = xmlNewNode(NULL, BAD_CAST "recentfiles");
	xmlDocSetRootElement(doc, root_node);

	for (; rfiles; rfiles = rfiles->next) {
		node = xmlNewChild(root_node, NULL, BAD_CAST "file", NULL);
		xmlSetProp(node, (const xmlChar *) "path", (char *) rfiles->data);
	}

	xmlSaveFormatFile (save_file, doc, 1);
}

static GtkWidget *create_apps_list (const gchar **command_fmt)
{
	GtkWidget *view;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkListStore *list;
	GtkTreeIter iter;
	int i;

	if (!command_fmt) {
		return NULL;
	}

	list = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);

	for (i = 0; command_fmt[i]; i++) {
		gtk_list_store_append (list, &iter);
		gtk_list_store_set (list, &iter,
				    0, command_fmt[i],
				    1, i,
				    -1);
	}

	view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list));

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes
		(column, renderer, "text", 0, NULL);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

	return view;
}


static int get_dot_file (const struct dirent *entry)
{
	if (entry->d_name[0] != '.') {
		return 1;
	} else if ((entry->d_name[1] == '\0')
		   || ((entry->d_name[1] == '.')
		       && (entry->d_name[2] == '\0'))) {
		return 0;
	}
	return 1;
}

static int not_get_dot_file (const struct dirent *entry)
{
	if (entry->d_name[0] == '.') {
		return 0;
	}
	return 1;
}

static int sort_dir (const void *a, const void *b)
{
	struct dirent **A = (struct dirent **) a;
	struct dirent **B = (struct dirent **) b;
	struct stat file_info_A, file_info_B;

	strcat (s_path, (*A)->d_name);
	stat (s_path, &file_info_A);
	s_path[s_path_len] = '\0';

	strcat (s_path, (*B)->d_name);
	stat (s_path, &file_info_B);
	s_path[s_path_len] = '\0';

	if ((S_ISDIR (file_info_A.st_mode))
	    && !(S_ISDIR (file_info_B.st_mode))) {
		return -1;
	}
	if ((S_ISDIR (file_info_B.st_mode))
	    && !(S_ISDIR (file_info_A.st_mode))) {
		return 1;
	}

	return strcmp ((*A)->d_name, (*B)->d_name);
}

static int fs_browser_read_dir (FsBrowser *browser)
{
	struct dirent **entry;
	struct stat file_info;
	int len, i;
	GtkListStore *list;
	GtkTreeIter iter;
	gchar *desc;

	list = GTK_LIST_STORE (gtk_tree_view_get_model
			       (GTK_TREE_VIEW (browser->view)));
	gtk_list_store_clear (list);

	if (browser->dot_files)
		len = scandir (s_path, &entry, get_dot_file, sort_dir);
	else
		len = scandir (s_path, &entry, not_get_dot_file, sort_dir);

	for (i = 0; i < len; i++) {
		gboolean is_dir;
		const gchar *str;

		strcat (s_path, entry[i]->d_name);
		stat (s_path, &file_info);
		s_path[s_path_len] = '\0';
		if (S_ISDIR (file_info.st_mode)) {
			desc = entry[i]->d_name;
			is_dir = TRUE;
		} else {
			is_dir = FALSE;

			str = MIME_get_type (entry[i]->d_name, TRUE);
			if (str) {
				desc = g_strjoin ("", entry[i]->d_name,
						  "\n\t<i>",
						  str, "</i>", NULL);
			} else {
				desc = entry[i]->d_name;
			}
		}

		gtk_list_store_append (list, &iter);
		gtk_list_store_set (list, &iter,
				    NAME_COLUMN, entry[i]->d_name,
				    DESC_COLUMN, desc,
				    TYPE_COLUMN, is_dir,
				    -1);
	}

	gtk_entry_set_text (GTK_ENTRY (browser->entry), s_path);

	return 0;
}

static void go_entry_dir (GtkEntry *entry, gpointer user_data)
{
	FsBrowser *browser = (FsBrowser *) user_data;
	G_CONST_RETURN gchar *text = gtk_entry_get_text (entry);

	if (access (text, R_OK | X_OK) == 0) {
		struct stat fi;

		stat (text, &fi);
		if (S_ISDIR (fi.st_mode)) {
			strcpy (s_path, text);
			s_path_len = strlen (s_path);
			ADD_ENDING_SLASH;
			fs_browser_read_dir (browser);
		}
	} else {
		gtk_entry_set_text (entry, s_path);
	}
}

static GtkWidget *create_button (gchar *stock_id, gchar *text,
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

static GtkWidget *create_toggle_button (gchar *stock_id, gchar *text,
				 GCallback callback, gpointer data)
{
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *button_hbox;
	GtkWidget *image;

	button = gtk_toggle_button_new ();
	image = gtk_image_new_from_stock (stock_id,
					  GTK_ICON_SIZE_LARGE_TOOLBAR);
	label = gtk_label_new (text);
	button_hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (button_hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (button_hbox), label, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (button), button_hbox);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	if (callback)
		g_signal_connect (G_OBJECT (button), "toggled",
				  G_CALLBACK (callback), data);

	return button;
}

static void switch_dot_files (GtkWidget *self, gpointer data)
{
	FsBrowser *browser = (FsBrowser *) data;

	if (!browser->active)
		return;

	if (browser->dot_files)
		browser->dot_files = FALSE;
	else
		browser->dot_files = TRUE;

	fs_browser_read_dir (browser);
}

static void go_up_dir (GtkWidget * self, gpointer data)
{
	FsBrowser *browser = (FsBrowser *) data;
	int i;

	if (!browser->active)
		return;

	for (i = s_path_len - 2; (i > 0) && (s_path[i] != '/'); i--);
	i = MAX (i, 0);
	s_path[++i] = '\0';
	s_path_len = strlen (s_path);
	fs_browser_read_dir (browser);
}

static void go_home_dir (GtkWidget * self, gpointer data)
{
	FsBrowser *browser = (FsBrowser *) data;
	char *path;

	if (!browser->active)
		return;

	path = getenv ("HOME");

	if (path) {
		strcpy (s_path, path);
		s_path_len = strlen (s_path);
		ADD_ENDING_SLASH;
		fs_browser_read_dir (browser);
		free (path);
	}
}

static void go_root_dir (GtkWidget * self, gpointer data)
{
	FsBrowser *browser = (FsBrowser *) data;

	if (!browser->active)
		return;

	strcpy (s_path, "/");
	s_path_len = strlen (s_path);
	fs_browser_read_dir (browser);
}

struct mime_dialog
{
	gchar *file;
	GtkTreeView *view;
	GtkEntry *entry;
};

static void add_mime (GtkWidget *self, gpointer data)
{
	struct mime_dialog *md = (struct mime_dialog *) data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *command;

	command = g_strdup (gtk_entry_get_text (md->entry));
	if (strlen (command)) {
		model = gtk_tree_view_get_model (md->view);
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    0, command,
				    1, -1,
				    -1);

		MIME_add (md->file, command);
	}
	g_free (command);
}

static void go_selected_dir (GtkTreeView *self, GtkTreePath *treepath,
			     GtkTreeViewColumn *column, gpointer data)
{
	FsBrowser *browser = (FsBrowser *) data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *name;
	const gchar **commands = NULL;
	struct stat info;

	model = gtk_tree_view_get_model (self);
	gtk_tree_model_get_iter (model, &iter, treepath);
	gtk_tree_model_get (model, &iter, NAME_COLUMN, &name, -1);
	if (browser->active) {
		strcat (s_path, name);
		stat (s_path, &info);
		if (S_ISDIR (info.st_mode)) {
			strcat (s_path, "/");
			s_path_len = strlen (s_path);
			fs_browser_read_dir (browser);

			return;
		} else {
			s_path[s_path_len] = '\0';
		}
		if (!S_ISREG (info.st_mode))
			return;
	}

	commands = MIME_apps (name);

	if (commands) {
		gchar *file = NULL, *message;
		const gchar *command = NULL;
		GtkWidget *view;
		GtkWidget *dialog;
		GtkWidget *label;
		GtkWidget *entry;
		GtkWidget *button;
		GtkWidget *hbox;
		GtkWidget *scroll;
		GtkTreeIter inneriter;
		GtkTreeSelection *selection;
		GtkTreeModel *innermodel;
		struct mime_dialog md;
		int response, id;

		g_signal_emit_by_name (browser, "completed");

		view = create_apps_list (commands);

		if (browser->active)
			file = g_strjoin ("", s_path, name, NULL);
		else
			file = g_strdup (name);

		dialog = gtk_dialog_new_with_buttons
			("Choose Application", NULL, GTK_DIALOG_MODAL,
			 GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			 GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
			 NULL);

		scroll = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy
			(GTK_SCROLLED_WINDOW (scroll),
			 GTK_POLICY_AUTOMATIC,
			 GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type
			(GTK_SCROLLED_WINDOW (scroll),
			 GTK_SHADOW_ETCHED_IN);
		gtk_container_add (GTK_CONTAINER (scroll), view);

		message = g_strjoin ("", "Open file \"", name, "\" with:", NULL);
		label = gtk_label_new (message);
		g_free (message);
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
				    label, FALSE, FALSE, 2);

		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
				    scroll, TRUE, TRUE, 0);

		entry = gtk_entry_new ();

		md.file = file;
		md.view = (GtkTreeView *) view;
		md.entry = (GtkEntry *) entry;

		button = gtk_button_new_from_stock ("gtk-add");
		g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (add_mime), &md);
		hbox = gtk_hbox_new (FALSE, 3);
		gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
				    hbox, FALSE, FALSE, 3);

		gtk_widget_set_size_request (dialog, 300, 300);

		gtk_widget_show_all (dialog);
		response = gtk_dialog_run (GTK_DIALOG (dialog));

		switch (response) {
		case GTK_RESPONSE_ACCEPT:
			innermodel = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
			if (gtk_tree_selection_get_selected
			    (selection, &innermodel, &inneriter)) {
				gtk_tree_model_get (innermodel, &inneriter, 1, &id, -1);
				if (id == -1) {
					gchar *tmpc;

					gtk_tree_model_get (innermodel, &inneriter,
							    0, &tmpc,
							    -1);
					command = MIME_mk_command_line
						(tmpc, file, FALSE, FALSE);
					g_free (tmpc);
				} else {
					command = MIME_mk_command_line
						(commands[id], file, FALSE, FALSE);
				}

				browser->recent_files = add_recent_file
					(browser->recent_files, file);

				if (!browser->active) {
					show_recent_files (browser);
				}

				xfce_exec (command, FALSE, FALSE, NULL);
			}
			break;

		case GTK_RESPONSE_REJECT:

			break;
		}

		gtk_widget_destroy (dialog);
	} else {
		GtkWidget *dialog;
		GtkWidget *label;
		GtkWidget *entry;
		int response;
		gchar *file = NULL, *message;
		const gchar *command = NULL;

		g_signal_emit_by_name (browser, "completed");

		if (browser->active)
			file = g_strjoin ("", s_path, name, NULL);
		else
			file = g_strdup (name);

		dialog = gtk_dialog_new_with_buttons
			("Choose Application", NULL, GTK_DIALOG_MODAL,
			 GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			 GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
			 NULL);

		message = g_strjoin ("", "Open file \"", name, "\" with:", NULL);
		label = gtk_label_new (message);
		g_free (message);
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
				    label, FALSE, FALSE, 2);

		entry = gtk_entry_new ();

		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
				    entry, FALSE, FALSE, 3);

		gtk_widget_show_all (dialog);
		response = gtk_dialog_run (GTK_DIALOG (dialog));

		switch (response) {
		case GTK_RESPONSE_ACCEPT:
			command = MIME_mk_command_line
				(gtk_entry_get_text (GTK_ENTRY (entry)),
				 file, FALSE, FALSE);

			browser->recent_files = add_recent_file
				(browser->recent_files, file);

			if (!browser->active) {
				show_recent_files (browser);
			}

			xfce_exec (command, FALSE, FALSE, NULL);
			break;
		case GTK_RESPONSE_REJECT:

			break;
		}

		gtk_widget_destroy (dialog);
	}
}

GType fs_browser_get_type ()
{
	static GType type;

	type = g_type_from_name ("FsBrowser");

	if (!type) {
		static const GTypeInfo info = {
			sizeof (FsBrowserClass),
			NULL,
			NULL,
			(GClassInitFunc) fs_browser_class_init,
			NULL,
			NULL,
			sizeof (FsBrowser),
			0,
			(GInstanceInitFunc) fs_browser_init,
		};
		type = g_type_register_static (GTK_TYPE_VBOX, "FsBrowser",
					       &info, 0);
	}
	return type;
}

static void fs_browser_class_init (FsBrowserClass * klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) klass;
	parent_class = g_type_class_peek_parent (klass);

	object_class->destroy = fs_browser_destroy;

	fs_browser_signals[COMPLETED] =
			g_signal_new ("completed",
				      G_OBJECT_CLASS_TYPE (object_class),
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET (FsBrowserClass,
						       completed), NULL, NULL,
				      g_cclosure_marshal_VOID__VOID,
				      G_TYPE_NONE, 0);
}

static void
fs_browser_init (FsBrowser * fb)
{
	GtkWidget *button;
	GtkWidget *hbox;
	GtkWidget *scroll;
	GtkListStore *list;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

/* 	if (dir_icon) { */
/* 		fb->dir_pixbuf = gdk_pixbuf_new_from_file_at_size (dir_icon, */
/* 								   24, 24, */
/* 								   NULL); */
/* 	} else { */
/* 		fb->dir_pixbuf = NULL; */
/* 	} */

	fb->active = TRUE;

	gtk_box_set_homogeneous (GTK_BOX (fb), FALSE);

	fb->entry = gtk_entry_new ();
	gtk_entry_set_editable (GTK_ENTRY (fb->entry), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (fb->entry), FALSE);
	gtk_box_pack_start (GTK_BOX (fb), fb->entry, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (TRUE, 0);

	button = create_button ("gtk-home", "Home", NULL, NULL);
	g_signal_connect_after (G_OBJECT (button), "clicked",
				G_CALLBACK (go_home_dir), fb);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

	button = create_button ("gtk-harddisk", "Root", NULL, NULL);
	g_signal_connect_after (G_OBJECT (button), "clicked",
				G_CALLBACK (go_root_dir), fb);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (fb), hbox, FALSE, FALSE, 0);

	fb->togglerecent = create_toggle_button ("gtk-index", "Recent files",
						 G_CALLBACK (rfiles_toggled), fb);
	gtk_box_pack_start (GTK_BOX (hbox), fb->togglerecent, TRUE, TRUE, 0);

	list = gtk_list_store_new (5,
				   G_TYPE_STRING,
				   G_TYPE_STRING,
				   GDK_TYPE_PIXBUF, G_TYPE_POINTER,
				   G_TYPE_BOOLEAN);
	fb->view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (fb->view), FALSE);

	g_signal_connect (G_OBJECT (fb->view), "row-activated",
			  G_CALLBACK (go_selected_dir), fb);

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", ICON_COLUMN, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	g_object_set (G_OBJECT (renderer), "weight", 500, NULL);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "markup", DESC_COLUMN,
					     "weight-set", TYPE_COLUMN, NULL);
	gtk_tree_view_column_set_sizing (column,
					 GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	gtk_tree_view_append_column (GTK_TREE_VIEW (fb->view), column);

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
					     GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (scroll), fb->view);

	gtk_box_pack_start (GTK_BOX (fb), scroll, TRUE, TRUE, 0);

	hbox = gtk_hbox_new (TRUE, 0);

	button = create_button ("gtk-convert", "show/hide .files", NULL, fb);
	g_signal_connect_after (G_OBJECT (button), "clicked",
				G_CALLBACK (switch_dot_files), fb);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

	button = create_button ("gtk-go-up", "Up directory", NULL, fb);
	g_signal_connect_after (G_OBJECT (button), "clicked",
				G_CALLBACK (go_up_dir), fb);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

	fb->closebutton = create_button ("gtk-close", "Close", NULL, fb);
	gtk_box_pack_start (GTK_BOX (hbox), fb->closebutton, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (fb), hbox, FALSE, FALSE, 0);
}

GtkWidget *fs_browser_new ()
{
	GtkWidget *browser;
	char *path;
	int len;

#if GTK_CHECK_VERSION (2,4,0)
	g_object_get (gtk_settings_get_default(), "gtk-icon-theme-name", &icon_theme_name, NULL);
#else /* gtk < 2.4 */
	{
		GtkSettings *gsettings = gtk_settings_get_default ();
		g_object_get (G_OBJECT (gsettings), "gtk-icon-theme-name", &icon_theme_name, NULL);
	}
#endif

	MIME_ICON_load_theme ();

	browser = GTK_WIDGET (g_object_new (fs_browser_get_type (), NULL));
	path = (gchar *) getenv ("HOME");
	strcpy (FS_BROWSER (browser)->path, path);
	len = strlen (path);
	if (FS_BROWSER (browser)->path[len - 1] != '/') {
		FS_BROWSER (browser)->path[len] = '/';
		FS_BROWSER (browser)->path[len + 1] = '\0';
	}
	free (path);
	strcpy (s_path, FS_BROWSER (browser)->path);
	s_path_len = strlen (s_path);
	FS_BROWSER (browser)->dot_files = FALSE;
	fs_browser_read_dir (FS_BROWSER (browser));

	FS_BROWSER (browser)->recent_files = read_recent_files ();

	return browser;
}

static void fs_browser_destroy (GtkObject *object)
{
	FsBrowser *browser;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_FS_BROWSER (object));

	browser = FS_BROWSER (object);

	write_recent_files (browser->recent_files);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}