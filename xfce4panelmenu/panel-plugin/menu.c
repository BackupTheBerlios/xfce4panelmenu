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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <wordexp.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <libxfce4util/i18n.h>
#include <libxfcegui4/libxfcegui4.h>

#include "menu.h"
#include "common.h"

enum {
	COMPLETED,
	GETGRAB,
	LAST_SIGNAL
};

struct rec_app {
	char *name;
	char *app;
	char *icon;
	int count;
	GtkWidget *button;
};

#define REC_APP_INIT(X) \
	(X) = (struct rec_app *) malloc (sizeof (struct rec_app)); \
	(X)->name = NULL;				 \
	(X)->app = NULL;				 \
	(X)->icon = NULL;				 \
	(X)->count = 1; \
        (X)->button = NULL;

#define USER_ACTION_INIT(X) \
	(X) = (struct user_action *) malloc (sizeof (struct user_action)); \
	(X)->icon_path = NULL;				 \
	(X)->icon = NULL;				 \
	(X)->label = NULL;				 \
	(X)->app = NULL;

static char *def_path = "/home/PlumTG/Programming/C/widget/default.png";
static char *fstab_path = "/etc/fstab";
static char *mtab_path = "/etc/mtab";

static GList *s_rec_apps;

static void show_menu                           (GtkWidget * self, gpointer data);
static void private_cb_eventbox_style_set       (GtkWidget * widget,
						 GtkStyle * old_style);
static GtkWidget *menu_start_create_button_name (char *icon, gchar * text,
						 GCallback callback, gpointer data);
static void run_menu_app                        (GtkWidget * self, gpointer data);
static void repack_rec_apps_buttons             (Menu *menu);
static void menu_destroy                        (GtkObject *object);

static GtkHBoxClass *parent_class = NULL;

static guint fs_browser_signals[LAST_SIGNAL] = { 0 };


int rec_apps_cmp (gconstpointer a, gconstpointer b)
{
	struct rec_app *A = (struct rec_app *) a;
	struct rec_app *B = (struct rec_app *) b;

	return B->count - A->count;
}

static GList *get_user_actions (Menu *menu)
{
	GList *actions = NULL;
	xmlDocPtr doc = NULL;
	xmlNodePtr node = NULL;
	char *read_file;
	GtkTooltips *tooltips;

	read_file = ms_get_read_file ("userapps.xml");
	doc = xmlParseFile (read_file);
	node = xmlDocGetRootElement (doc);

	for (node = node->children; node; node = node->next) {
		if (xmlStrEqual (node->name, "app")) {
			struct user_action *action;

			USER_ACTION_INIT (action);

			action->icon_path = xmlGetProp (node, "icon");
			action->app = xmlGetProp (node, "cmd");
			action->label = xmlGetProp (node, "name");

			actions = g_list_append (actions, action);
		}
	}

	xmlFreeDoc (doc);

	return actions;
}

static void free_user_actions (GList *actions)
{
	GList *tmp;

	for (tmp = actions; tmp; tmp = tmp->next) {
		struct user_action *action = (struct user_action *) tmp->data;

		if (action->icon_path)
			free (action->icon_path);
		if (action->icon)
			free (action->icon);
		if (action->label)
			free (action->label);
		if (action->app)
			free (action->app);

		free (action);
	}
}

GList *get_rec_apps_list (Menu *menu)
{
	GList *apps = NULL;
	xmlDocPtr doc = NULL;
	xmlNodePtr node = NULL;
	char *read_file;
	GtkTooltips *tooltips;

	read_file = ms_get_read_file ("recentapps.xml");
	doc = xmlParseFile (read_file);
	node = xmlDocGetRootElement (doc);

	tooltips = gtk_tooltips_new ();
	gtk_tooltips_set_delay (tooltips, 2000);

	for (node = node->children; node; node = node->next) {
		if (xmlStrEqual (node->name, "app")) {
			struct rec_app *app;
			char *prop;

			REC_APP_INIT (app);

			app->name = xmlGetProp (node, "name");
			app->app = xmlGetProp (node, "cmd");
			app->icon = xmlGetProp (node, "icon");
			if (app->icon && (strlen (app->icon) == 0)) {
				free (app->icon);
				app->icon = NULL;
			}
			prop = xmlGetProp (node, "count");
			app->count = atoi (prop);

			app->button = menu_start_create_button_name 
				(app->icon, app->name, G_CALLBACK (run_menu_app), menu);
			g_object_set_data (G_OBJECT (app->button), "name-data", app->name);
			g_object_set_data (G_OBJECT (app->button), "app", app->app);
			g_object_set_data (G_OBJECT (app->button), "icon-data", app->icon);

			gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), app->button,
					      app->name, NULL);
			free (prop);
			apps = g_list_append (apps, app);
		}
	}

	xmlFreeDoc (doc);

	if (apps)
		apps = g_list_sort (apps, rec_apps_cmp);

	return apps;
}

void
write_rec_apps_list (GList * apps)
{
	FILE *file;
	char *save_file;
	xmlDocPtr doc = NULL;
	xmlNodePtr node = NULL, root_node = NULL;
	char count[6];

	save_file = ms_get_save_file ("recentapps.xml");

	doc = xmlNewDoc(BAD_CAST "1.0");
	root_node = xmlNewNode(NULL, BAD_CAST "apps");
	xmlDocSetRootElement(doc, root_node);

	for (; apps; apps = apps->next) {
		struct rec_app *app = (struct rec_app *) apps->data;

		node = xmlNewChild(root_node, NULL, BAD_CAST "app", NULL);

		xmlSetProp(node, (const xmlChar *) "name", app->name);
		xmlSetProp(node, (const xmlChar *) "cmd", app->app);
		if (app->icon)
			xmlSetProp(node, (const xmlChar *) "icon", app->icon);

		sprintf (count, "%d", app->count);
		xmlSetProp(node, (const xmlChar *) "count", count);
	}

	xmlSaveFormatFile (save_file, doc, 1);
}

GList *
update_rec_app_list (GList * apps, GObject * obj, Menu *menu)
{
	GList *tmp = apps;
	struct rec_app *app;
	char *data;
	char *cmd = g_object_get_data (obj, "app");
	GtkTooltips *tooltips;

	for (; tmp; tmp = tmp->next) {
		if (strcmp (((struct rec_app *) tmp->data)->app, cmd) == 0) {
			((struct rec_app *) tmp->data)->count++;
			apps = g_list_sort (apps, rec_apps_cmp);
			repack_rec_apps_buttons (menu);
			return apps;
		}
	}

	REC_APP_INIT (app);
	if ((data = g_object_get_data (obj, "name-data")))
		app->name = strdup (data);
	if ((data = g_object_get_data (obj, "app")))
		app->app = strdup (data);
	if ((data = g_object_get_data (obj, "icon-data")))
		app->icon = strdup (data);

	tooltips = gtk_tooltips_new ();
	gtk_tooltips_set_delay (tooltips, 2000);

	app->button = menu_start_create_button_name (app->icon, app->name,
						     G_CALLBACK
						     (run_menu_app), menu);
	g_object_set_data (G_OBJECT (app->button), "name-data", app->name);
	g_object_set_data (G_OBJECT (app->button), "app", app->app);
	g_object_set_data (G_OBJECT (app->button), "icon-data", app->icon);
	
	gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), app->button,
			      app->name, NULL);

	apps = g_list_append (apps, app);
	apps = g_list_sort (apps, rec_apps_cmp);

	repack_rec_apps_buttons (menu);

	return apps;
}

void menu_repack_recent_apps (Menu *menu)
{
	repack_rec_apps_buttons (menu);
}

static void
run_app (char *app)
{
	pid_t pid;

	pid = fork ();
	if (pid) {

	} else {
		wordexp_t words;
		char **command;

		wordexp (app, &words, 0);
		command = (char **) malloc ((words.we_wordc + 1) *
					    sizeof (char *));
		memcpy (command, words.we_wordv,
			sizeof (char *) * words.we_wordc);
		command[words.we_wordc] = NULL;

		execvp (command[0], command);
		exit (1);
	}
}

static void
run_menu_app (GtkWidget * self, gpointer data)
{
	Menu *menu = (Menu *) data;
	char *app;

	app = g_object_get_data (G_OBJECT (self), "app");
	s_rec_apps = update_rec_app_list (s_rec_apps, G_OBJECT (self), menu);
	write_rec_apps_list (s_rec_apps);

	g_signal_emit_by_name (menu, "completed");

	run_app (app);
}

static void set_button_press (GtkWidget *self, gpointer data)
{
	Menu *menu = (Menu *) data;

	g_signal_emit_by_name (menu, "completed");

	run_app (menu->set_app);
}

static void
menu_run_app (GtkWidget * self, gpointer data)
{
	Menu *menu = (Menu *) data;

	g_signal_emit_by_name (menu, "completed");

	run_app (menu->run_app);
}

static void
menu_term_app (GtkWidget * self, gpointer data)
{
	Menu *menu = (Menu *) data;

	g_signal_emit_by_name (menu, "completed");

	run_app (menu->term_app);
}

static GtkWidget *
get_menu (xmlNodePtr node, Menu * start)
{
	GtkWidget *menu, *menuitem = NULL, *submenu = NULL;
	char *icon;
	GdkPixbuf *def;
	GtkWidget *image;
	char *prop;

	def = gdk_pixbuf_new_from_file (def_path, NULL);

	menu = gtk_menu_new ();

	for (node = node->children; node; node = node->next) {
		char *visible = xmlGetProp (node, "visible");

		if (visible && strcmp (visible, "false") == 0) {
			free (visible);
			continue;
		}
		if (xmlStrEqual (node->name, "menu")) {
			icon = xmlGetProp (node, "icon");
			submenu = get_menu (node, start);
			prop = xmlGetProp (node, "name");
			menuitem = gtk_image_menu_item_new_with_label (prop);
			free (prop);
			if (icon) {
				GdkPixbuf *pixbuf =
						gdk_pixbuf_new_from_file_at_size
						(icon, 16,
						 16, NULL);

				image = gtk_image_new_from_pixbuf (pixbuf);
			} else
				image = gtk_image_new_from_pixbuf (def);
			gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM
						       (menuitem), image);
			gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem),
						   submenu);
			gtk_widget_show (menuitem);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu),
					       menuitem);
		}
		if (xmlStrEqual (node->name, "app")
		    || xmlStrEqual (node->name, "builtin")) {

			prop = xmlGetProp (node, "name");
			menuitem = gtk_image_menu_item_new_with_label (prop);
			g_object_set_data (G_OBJECT (menuitem), "name-data",
					   prop);

			icon = xmlGetProp (node, "icon");
			g_object_set_data (G_OBJECT (menuitem), "icon-data",
					   icon);

			if (icon) {
				GdkPixbuf *pixbuf =
						gdk_pixbuf_new_from_file_at_size
						(icon, 16,
						 16, NULL);

				image = gtk_image_new_from_pixbuf (pixbuf);
			} else
				image = gtk_image_new_from_pixbuf (def);
			gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM
						       (menuitem), image);

			g_object_set_data (G_OBJECT (menuitem), "app",
					   xmlGetProp (node, "cmd"));
			g_signal_connect_after (G_OBJECT (menuitem),
						"activate",
						G_CALLBACK (run_menu_app),
						start);

			gtk_widget_show (menuitem);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu),
					       menuitem);
		}
		if (xmlStrEqual (node->name, "separator")) {
			menuitem = gtk_separator_menu_item_new ();
			gtk_widget_show (menuitem);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu),
					       menuitem);
		}
		if (xmlStrEqual (node->name, "title")) {
			prop = xmlGetProp (node, "name");
			menuitem = gtk_image_menu_item_new_with_label (prop);
			free (prop);
			gtk_widget_set_sensitive (menuitem, FALSE);
			gtk_widget_show (menuitem);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu),
					       menuitem);
		}
	}

	gtk_widget_show (menu);
	return menu;
}

static GtkWidget *
create_app_menu (Menu * menu)
{
	GtkWidget *menu_start;
	xmlDocPtr doc = NULL;
	xmlNodePtr node = NULL;
	char *read_file;

	read_file = ms_get_read_file ("menu.xml");
	doc = xmlParseFile (read_file);
	node = xmlDocGetRootElement (doc);

	menu_start = get_menu (node, menu);

	xmlFreeDoc (doc);

	return menu_start;
}

static void menu_class_init (MenuClass * klass);
static void menu_init (Menu * menu);

static void
button_press_g (GtkWidget * self, gpointer data)
{
	Menu *menu = (Menu *) data;

	g_signal_emit_by_name (menu, "completed");
}

static GtkWidget *
menu_start_create_button_image (GtkWidget *image, gchar * text,
				GCallback callback, gpointer data)
{
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *button_hbox;

	button = gtk_button_new ();
	label = gtk_label_new (text);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	button_hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (button_hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (button_hbox), label, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (button), button_hbox);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);

	g_object_set_data (G_OBJECT (button), "label", label);

	if (callback)
		g_signal_connect (G_OBJECT (button), "clicked",
				  G_CALLBACK (callback), data);

	return button;
}

static GtkWidget *
menu_start_create_button_name (char *icon, gchar * text,
			       GCallback callback, gpointer data)
{
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *button_hbox;
	GtkWidget *image;
	GdkPixbuf *pixbuf = NULL;
	GtkStyle *style;

	if (icon && g_file_test(icon, G_FILE_TEST_EXISTS)) {
		pixbuf = gdk_pixbuf_new_from_file_at_size
			(icon, 20, 20, NULL);
	}

	if (!pixbuf) {
		pixbuf = gdk_pixbuf_new_from_file_at_size
			(ICONDIR "/xfce4_xicon2.png", 20, 20, NULL);
	}
	image = gtk_image_new_from_pixbuf (pixbuf);

	return menu_start_create_button_image (image, text, callback, data);
}

static GtkWidget *
menu_start_create_button (gchar * stock_id, gchar * text,
			  GCallback callback, gpointer data)
{
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *button_hbox;
	GtkWidget *image;

	image = gtk_image_new_from_stock (stock_id,
					  GTK_ICON_SIZE_LARGE_TOOLBAR);

	return menu_start_create_button_image (image, text, callback, data);
}

static GtkWidget *
menu_start_create_button_pixbuf (GdkPixbuf * pixbuf,
				 gchar * text,
				 GCallback callback, gpointer data)
{
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *button_hbox;
	GtkWidget *image;

	image = gtk_image_new_from_pixbuf (pixbuf);

	return menu_start_create_button_image (image, text, callback, data);
}

GType
menu_get_type ()
{
	static GType type = 0;

	type = g_type_from_name ("Menu");

	if (!type) {
		static const GTypeInfo info = {
			sizeof (MenuClass),
			NULL,
			NULL,
			(GClassInitFunc) menu_class_init,
			NULL,
			NULL,
			sizeof (Menu),
			0,
			(GInstanceInitFunc) menu_init,
		};
		type = g_type_register_static (GTK_TYPE_HBOX, "Menu", &info,
					       0);
	}
	return type;
}

static void
menu_class_init (MenuClass * klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) klass;
	parent_class = g_type_class_peek_parent (klass);

	object_class->destroy = menu_destroy;

	fs_browser_signals[COMPLETED] =
			g_signal_new ("completed",
				      G_OBJECT_CLASS_TYPE (object_class),
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET (MenuClass,
						       completed), NULL, NULL,
				      g_cclosure_marshal_VOID__VOID,
				      G_TYPE_NONE, 0);

	fs_browser_signals[GETGRAB] =
			g_signal_new ("getgrab",
				      G_OBJECT_CLASS_TYPE (object_class),
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET (MenuClass,
						       getgrab), NULL, NULL,
				      g_cclosure_marshal_VOID__VOID,
				      G_TYPE_NONE, 0);
}

GtkWidget *
create_arrow_button (char *stock_id, char *text)
{
	GtkWidget *button;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *arrow;
	GtkWidget *button_hbox;

	button = gtk_button_new ();
	image = gtk_image_new_from_stock (stock_id,
					  GTK_ICON_SIZE_LARGE_TOOLBAR);
	label = gtk_label_new (text);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
	button_hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (button_hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (button_hbox), label, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (button_hbox), arrow, FALSE, FALSE, 4);
	gtk_container_add (GTK_CONTAINER (button), button_hbox);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);

	return button;
}

static void
private_cb_eventbox_style_set (GtkWidget * widget, GtkStyle * old_style)
{
	static gboolean recursive = 0;
	GtkStyle *style;

	if (recursive > 0)
		return;

	++recursive;
	style = gtk_widget_get_style (widget);
	gtk_widget_modify_bg (widget, GTK_STATE_NORMAL,
			      &style->bg[GTK_STATE_PRELIGHT]);
	gtk_widget_modify_bg (widget, GTK_STATE_PRELIGHT,
			      &style->bg[GTK_STATE_PRELIGHT]);
	--recursive;
}

static void
menu_init (Menu * menu)
{
	menu->time = 0;
	menu->menu = NULL;

	menu->atime = 0;
	menu->user_actions = NULL;

	menu->r_apps_count = 10;
	menu->set_app = g_strdup ("xfce-setting-show");
	menu->run_app = g_strdup ("xfrun4");
	menu->term_app = g_strdup ("xfterm4");

	gtk_box_set_homogeneous (GTK_BOX (menu), TRUE);
	gtk_box_set_spacing (GTK_BOX (menu), 0);
}

static void run_user_action (GtkWidget *self, gpointer data)
{
	Menu *menu = (Menu *) data;
	struct user_action *action = g_object_get_data (G_OBJECT (self), "user_action");

	if (!action) {
		return;
	}
	if (action->app) {
		run_app (action->app);
	}

	g_signal_emit_by_name (menu, "completed");
}

static void repack_user_buttons (Menu *menu)
{
	GList *old_list;
	GList *tmp;

	old_list = gtk_container_get_children (GTK_CONTAINER (menu->rbox));
	for (tmp = old_list; tmp; tmp = tmp->next) {
		if (g_object_get_data (tmp->data, "user_action")) {
			gtk_container_remove (GTK_CONTAINER (menu->rbox),
					      GTK_WIDGET (tmp->data));
		}
	}

	for (tmp = menu->user_actions; tmp; tmp = tmp->next) {
		GtkWidget *button;
		struct user_action *action = (struct user_action *) tmp->data;

		button = menu_start_create_button_name
			(action->icon_path, action->label,
			 G_CALLBACK (run_user_action), menu);
		g_object_set_data (G_OBJECT (button), "user_action", action);
		gtk_box_pack_start (GTK_BOX (menu->rbox), button, FALSE, FALSE, 0);
	}
}

static void
repack_rec_apps_buttons (Menu *menu)
{
	GList *old_list, *tmp;
	int i = 0;

	old_list = gtk_container_get_children (GTK_CONTAINER (menu->lbox));
	for (tmp = old_list; tmp; tmp = tmp->next) {
		if (g_object_get_data (tmp->data, "app")) {
			g_object_ref (tmp->data);
			gtk_container_remove (GTK_CONTAINER (menu->lbox),
					      GTK_WIDGET (tmp->data));
		}
	}
	for (tmp = s_rec_apps; tmp && (i < menu->r_apps_count); tmp = tmp->next) {
		struct rec_app *app = (struct rec_app *) tmp->data;

		gtk_box_pack_start (GTK_BOX (menu->lbox), app->button, FALSE, FALSE, 0);

		i++;
	}
}

static void free_recent_apps (GList *apps)
{
	GList *tmp = apps;

	for (; tmp; tmp = tmp->next) {
		struct rec_app *app = (struct rec_app *) tmp->data;

		if (app->name) {
			free (app->name);
		}

		if (app->app) {
			free (app->app);
		}

		if (app->name) {
			free (app->icon);
		}

		free (app);
	}

	g_list_free (apps);
}

GtkWidget *menu_new ()
{
	Menu *menu;
	GtkWidget *rbox;
	GtkWidget *button;
	GList *ra;
	GtkTooltips *tooltips;
	GtkStyle *style;
	GtkWidget *rebox;
	int i = 0;

	menu = MENU (g_object_new (menu_get_type (), NULL));

	if (1) {
		char *save_file;

		save_file = ms_get_save_file (NULL);
		if (!g_file_test (save_file, G_FILE_TEST_EXISTS)) {
			long mode = S_IRUSR | S_IWUSR | S_IXUSR |
				S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
			xfce_mkdirhier (save_file, mode, NULL);
		}
		g_free (save_file);
	}

	if (1) {
		char *save_file;

		save_file = ms_get_save_file ("recentapps.xml");
		if (!g_file_test (save_file, G_FILE_TEST_EXISTS)) {
			FILE *file;

			file = fopen (save_file, "w");

			fprintf (file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
			fprintf (file, "<apps>\n\n");
			fprintf (file, "</apps>\n");

			fclose (file);
		}
		g_free (save_file);
	}

	if (1) {
		char *save_file;

		save_file = ms_get_save_file ("menu.xml");
		if (!g_file_test (save_file, G_FILE_TEST_EXISTS)) {
			FILE *file;
			GtkWidget *dialog;

			file = fopen (save_file, "w");

			fprintf (file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
			fprintf (file, "<xfdesktop-menu>\n\n");
			fprintf (file, "<title name=\"Applications Menu\" />");
			fprintf (file, "</xfdesktop-menu>\n");

			fclose (file);

			dialog = gtk_message_dialog_new
				(NULL, GTK_DIALOG_MODAL,
				 GTK_MESSAGE_INFO,
				 GTK_BUTTONS_CLOSE,
				 "Empty menu file '%s' created\n"
				 "You can replace it with Xfce4 desktop menu",
				 save_file);
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
		}
		g_free (save_file);
	}

	if (1) {
		char *save_file;

		save_file = ms_get_save_file ("userapps.xml");
		if (!g_file_test (save_file, G_FILE_TEST_EXISTS)) {
			FILE *file;

			file = fopen (save_file, "w");

			fprintf (file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
			fprintf (file, "<xfdesktop-menu>\n\n");
			fprintf (file, "<app name=\"File find utility\" cmd=\"xfglob4\" />");
			fprintf (file, "</xfdesktop-menu>\n");

			fclose (file);
		}
		g_free (save_file);
	}

	s_rec_apps = get_rec_apps_list (menu);

	/*
	 * left pane 
	 */

	menu->lbox = gtk_vbox_new (FALSE, 0);

	repack_rec_apps_buttons (menu);

	button = create_arrow_button ("gtk-index", "Applications");
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (show_menu), menu);
	gtk_box_pack_end (GTK_BOX (menu->lbox), button, FALSE, FALSE, 0);

	menu->lebox = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (menu->lebox), menu->lbox);

	gtk_box_pack_start (GTK_BOX (menu), menu->lebox, TRUE, TRUE, 0);

	/*
	 * right pane 
	 */

	menu->rbox = gtk_vbox_new (FALSE, 0);

	rebox = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (rebox), menu->rbox);

	menu->fstabbutton = menu_start_create_button
		("gtk-harddisk", "Mount...", NULL, NULL);
	gtk_box_pack_end (GTK_BOX (menu->rbox),
			  menu->fstabbutton, FALSE, FALSE, 0);

	menu->fsbrowserbutton = menu_start_create_button
		("gtk-open", "Browse Files", NULL, NULL);
	gtk_box_pack_end (GTK_BOX (menu->rbox),
			  menu->fsbrowserbutton, FALSE, FALSE, 0);

	button = menu_start_create_button ("gtk-preferences", "Settings...",
					   NULL, NULL);
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (set_button_press), menu);
	gtk_box_pack_end (GTK_BOX (menu->rbox), button, FALSE, FALSE, 0);

	button = menu_start_create_button
		("gtk-network", "Terminal",
		 G_CALLBACK (menu_term_app),
		 menu);
	gtk_box_pack_end (GTK_BOX (menu->rbox), button, FALSE, FALSE, 0);

	button = menu_start_create_button
		("gtk-execute", "Run ...",
		 G_CALLBACK (menu_run_app),
		 menu);
	gtk_box_pack_end (GTK_BOX (menu->rbox), button, FALSE, FALSE, 0);

	button = gtk_hseparator_new ();
	gtk_box_pack_end (GTK_BOX (menu->rbox), button, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (menu), rebox, TRUE, TRUE, 0);

	return GTK_WIDGET (menu);
}

static void menu_destroy (GtkObject *object)
{
	Menu *menu;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_MENU (object));

	menu = MENU (object);

	if (menu->set_app) {
		g_free (menu->set_app);
		menu->set_app = NULL;
	}

	if (menu->term_app) {
		g_free (menu->term_app);
		menu->term_app = NULL;
	}

	if (menu->run_app) {
		g_free (menu->run_app);
		menu->run_app = NULL;
	}

	if (menu->menu) {
		gtk_widget_destroy (menu->menu);
		menu->menu = NULL;
	}

	free_recent_apps (s_rec_apps);
	s_rec_apps = NULL;

	free_user_actions (menu->user_actions);
	menu->user_actions = NULL;

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
position_menu (GtkMenu * menu, gint * x, gint * y,
	       gboolean push_in, gpointer data)
{
	GtkWidget *button = (GtkWidget *) data;
	GtkWidget *parent;
	int pos_x = 0, pos_y = 0, tmp = 0;
	GtkRequisition req;
	GtkAllocation alloc;
	int sh;
	GdkScreen *screen;

	screen = gdk_screen_get_default ();
	sh = gdk_screen_get_height (screen);

	gtk_widget_size_request (GTK_WIDGET (menu), &req);

	alloc = button->allocation;

	*x = button->allocation.x + button->allocation.width - 4;
	parent = button;
	while (parent) {
		if (!GTK_WIDGET_NO_WINDOW (parent)) {
			gdk_window_get_position (parent->window, &pos_x,
						 &tmp);
			*x += pos_x;
			pos_y += tmp;
		}
		parent = parent->parent;
	}
	if ((pos_y + button->allocation.y + button->allocation.height / 2) >
	    sh / 2) {
		*y = MAX (0,
			  pos_y + button->allocation.y +
			  button->allocation.height - req.height);
	} else {
		*y = MAX (0, pos_y + button->allocation.y);
	}
}

void show_menu_widget (GtkWidget *widget)
{
	Menu *menu = (Menu *) widget;
	struct stat fi;
	char *read_file;

	read_file = ms_get_read_file ("userapps.xml");
	stat (read_file, &fi);
	g_free (read_file);

	if (fi.st_mtime > menu->atime) {
		menu->atime = fi.st_mtime;

		free_user_actions (menu->user_actions);

		menu->user_actions = get_user_actions (menu);
		repack_user_buttons (menu);
	}

	gtk_widget_show_all (widget);
}

static void menu_deactivated (GtkWidget * self, gpointer data)
{
	Menu *menu = (Menu *) data;
	g_signal_emit_by_name (menu, "getgrab");
}

static void show_menu (GtkWidget * self, gpointer data)
{
	Menu *menu = (Menu *) data;
	guint32 time;
	struct stat fi;
	char *read_file;

	read_file = ms_get_read_file ("menu.xml");
	stat (read_file, &fi);
	g_free (read_file);

	if (fi.st_mtime > menu->time) {
		menu->time = fi.st_mtime;
		if (menu->menu) {
			gtk_widget_destroy (menu->menu);
		}
		menu->menu = create_app_menu (menu);
		g_signal_connect
			(G_OBJECT (menu->menu), "deactivate",
			 G_CALLBACK (menu_deactivated), menu);
	}

	time = gtk_get_current_event_time ();
	gtk_menu_popup (GTK_MENU (menu->menu), NULL, NULL,
			(GtkMenuPositionFunc) position_menu,
			self, 0, time);
}
