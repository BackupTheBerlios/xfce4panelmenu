#include <stdio.h>

#include <gtk/gtk.h>

#include <libxfce4util/i18n.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include <panel/plugins.h>
#include <panel/xfce.h>

#include "menustart.h"
#include "menu.h"
#include "fsbrowser.h"

/* from xfce sources */
char *ms_get_save_file (const char *name)
{
	int scr;
	char *path, *file = NULL;

/* 	scr = DefaultScreen (gdk_display); */

/* 	if (scr == 0) { */
	if (1) {
		path = g_build_filename ("xfce4", "menustart", name, NULL);
	} else {
		char *realname;

		realname = g_strdup_printf ("%s.%u", name, scr);

		path = g_build_filename ("xfce4", "panel", realname, NULL);

		g_free (realname);
	}

	file = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, path, TRUE);
	g_free (path);

	return file;
}

/* from xfce sources */
static char *get_localized_rcfile (const char *path)
{
	char *fmt, *result;
	char buffer[PATH_MAX];

	fmt = g_strconcat (path, ".%l", NULL);

	result = xfce_get_path_localized (buffer, PATH_MAX, fmt, NULL,
					  G_FILE_TEST_EXISTS);

	g_free (fmt);

	if (result) {
		DBG ("file: %s", buffer);
		return g_strdup (buffer);
	}

	if (g_file_test (path, G_FILE_TEST_EXISTS)) {
		DBG ("file: %s", path);
		return g_strdup (path);
	}

	return NULL;
}

/* from xfce sources */
char *
ms_get_read_file (const char *name)
{
	char **paths, **p, *file = NULL;

/* 	if (G_UNLIKELY (disable_user_config)) { */
	if (0) {
/* 		p = paths = g_new (char *, 2); */

/* 		paths[0] = g_build_filename (SYSCONFDIR, "xdg", "xfce4", */
/* 		                           "panel", name, NULL); */
/* 		paths[1] = NULL; */
	} else {
		int n;
		char **d;

		d = xfce_resource_dirs (XFCE_RESOURCE_CONFIG);

		for (n = 0; d[n] != NULL; ++n)
			/**/;

		p = paths = g_new0 (char *, n + 1);

		paths[0] = ms_get_save_file (name);

		for (n = 1; d[n] != NULL; ++n)
			paths[n] = g_build_filename (d[n], "xfce4",
						     "menustart", name, NULL);

		g_strfreev (d);

		/* first entry is not localized ($XDG_CONFIG_HOME)
		 * unless disable_user_config == TRUE */
		if (g_file_test (*p, G_FILE_TEST_EXISTS))
			file = g_strdup (*p);

		p++;
	}

	for (; file == NULL && *p != NULL; ++p)
		file = get_localized_rcfile (*p);

	g_strfreev (paths);

	return file;
}

struct menu_start
{
	GtkWidget *button;
	GtkWidget *menustart;

	GtkWidget *width_spin;
	GtkWidget *height_spin;

	GtkWidget *columns_spin;

	GtkWidget *mime_check;

	GtkWidget *mime_builtin;
	GtkWidget *mime_outside;
	GtkWidget *mime_entry;

	GtkWidget *user_count;
	GtkWidget *recent_count;
	GtkWidget *set_entry;
	GtkWidget *lock_entry;
	GtkWidget *switch_entry;
	GtkWidget *term_entry;
	GtkWidget *run_entry;
};

void button_clicked (GtkWidget *self, gpointer data)
{
	GtkWidget *parent;
	GtkAllocation alloc;
	int x, y, pos_x, pos_y;
	GtkRequisition req;
	GdkWindow *p;
	int side;
	int pos = MENU_START_BOTTOM;
	GdkScreen *screen;
	int sh, sw;

	screen = gdk_screen_get_default ();
	sh = gdk_screen_get_height (screen);
	sw = gdk_screen_get_width (screen);

	MenuStart *ms = (MenuStart *) data;
	gtk_widget_size_request (GTK_WIDGET (ms), &req);

	p = gtk_widget_get_parent_window (self);
	gdk_window_get_root_origin (p, &pos_x, &pos_y);

	alloc = self->allocation;
	x = pos_x + self->parent->allocation.x;
	y = pos_y + self->parent->allocation.y;

	side = panel_get_side ();
	switch (side)
		{
		case LEFT:
			x += self->allocation.width;
			y = MIN (sh - req.height, y);
			break;
		case RIGHT:
			x -= GTK_WIDGET (ms)->allocation.width;
			y = MIN (sh - req.height, y);
			break;
		case TOP:
			x = MIN (sw - req.width, x);
			y += self->allocation.height;
			break;
		default:
			x = MIN (sw - req.width, x);
			y -= req.height;
		}

	menu_start_show (ms, x, y, pos);
}

gboolean create_menustart_control (Control *control)
{
	GtkWidget *label;
	GtkWidget *button_hbox;
	GtkWidget *image;
	GdkPixbuf *pixbuf;
	char *read_file;
	struct menu_start *menustart;

	menustart = (struct menu_start *) malloc (sizeof (struct menu_start));

	menustart->menustart = menu_start_new ();
	gtk_widget_set_size_request (menustart->menustart, 400, 480);

	menustart->button = gtk_button_new ();
	gtk_button_set_relief(GTK_BUTTON (menustart->button), GTK_RELIEF_NONE);
	read_file = g_strdup (ICONDIR "/xfce4_xicon.png");
	pixbuf = gdk_pixbuf_new_from_file_at_size (read_file, 24, 24, NULL);
	g_free (read_file);
 	image = gtk_image_new_from_pixbuf (pixbuf);
	button_hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (button_hbox), image, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (menustart->button), button_hbox);
	g_signal_connect (G_OBJECT (menustart->button), "clicked",
			  G_CALLBACK (button_clicked), menustart->menustart);

	gtk_container_add (GTK_CONTAINER (control->base), menustart->button);
	gtk_widget_set_size_request (control->base, -1, -1);

	control->data = (gpointer) menustart;
	control->with_popup = FALSE;

	gtk_widget_show_all (menustart->button);

	return TRUE;
}

static void menustart_free (Control * control)
{
	struct menu_start *ms = (struct menu_start *) control->data;
	MenuStart *menustart = MENU_START (ms->menustart);

	if (menustart->lock_app) {
		g_free (menustart->lock_app);
	}

	if (menustart->switch_app) {
		g_free (menustart->switch_app);
	}

 	gtk_widget_destroy (ms->menustart);

	g_free (ms);
}

static void menustart_attach_callback (Control * control, const char *signal,
				       GCallback callback, gpointer data)
{
	g_signal_connect
		(G_OBJECT (( (struct menu_start*) control->data)->button),
		 signal, callback, data);
}

static void menustart_set_size (Control *control, int size)
{

}

static void menustart_set_orientation (Control *control, int orientation)
{

}

enum {
	ICON_PIXBUF_COL = 0,
	DESC_COL,
	FMT_COL,
	APP_COL,
	LAST_VIEW_COL,
	MIME_COL,
	LAST_COL
};

static void edit_apps_menu (GtkWidget *self, gpointer data)
{
	gchar *command;
	gchar *read_file;

	read_file = ms_get_read_file ("menu.xml");
	command = g_strjoin (" ", "xfce4-menueditor", read_file, NULL);
	g_free (read_file);

	xfce_exec (command, FALSE, FALSE, NULL);
	g_free (command);
}

static void edit_user_apps_menu (GtkWidget *self, gpointer data)
{
	gchar *command;
	gchar *read_file;

	read_file = ms_get_read_file ("userapps.xml");
	command = g_strjoin (" ", "xfce4-menueditor", read_file, NULL);
	g_free (read_file);

	xfce_exec (command, FALSE, FALSE, NULL);
	g_free (command);
}

GtkWidget *init_general_page (Control *ctrl)
{
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *spin;
	GtkWidget *menu;
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *combo;
	GtkWidget *entry;
	GtkWidget *separator;
	FsBrowser *browser;
	int i;
	struct menu_start *ms = (struct menu_start *) ctrl->data;

	menu = menu_start_get_menu_widget
		(MENU_START (((struct menu_start *) ctrl->data)->menustart));
	browser = FS_BROWSER (menu_start_get_browser_widget
			      (MENU_START (ms->menustart)));

	vbox = gtk_vbox_new (FALSE, 1);

	table = gtk_table_new (6, 3, TRUE);


	label = gtk_label_new ("Items count in recent apps menu");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);
	ms->recent_count = gtk_spin_button_new (NULL, 1, 0);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (ms->recent_count), 1, 128);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (ms->recent_count), 1, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (ms->recent_count),
				   MENU (menu)->r_apps_count);
	gtk_table_attach (GTK_TABLE (table), ms->recent_count, 2, 3, 0, 1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);


	label = gtk_label_new ("Switch User Command");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);
	ms->switch_entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (ms->switch_entry),
			    MENU_START (ms->menustart)->switch_app);
	gtk_table_attach (GTK_TABLE (table), ms->switch_entry, 1, 3, 1, 2,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	label = gtk_label_new ("Lock Screen Command");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);
	ms->lock_entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (ms->lock_entry),
			    MENU_START (ms->menustart)->lock_app);
	gtk_table_attach (GTK_TABLE (table), ms->lock_entry, 1, 3, 2, 3,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	label = gtk_label_new ("Setting Command");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);
	ms->set_entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (ms->set_entry),
			    MENU (menu)->set_app);
	gtk_table_attach (GTK_TABLE (table), ms->set_entry, 1, 3, 3, 4,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	label = gtk_label_new ("Run Command");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);
	ms->run_entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (ms->run_entry),
			    MENU (menu)->run_app);
	gtk_table_attach (GTK_TABLE (table), ms->run_entry, 1, 3, 4, 5,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	label = gtk_label_new ("Term Command");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);
	ms->term_entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (ms->term_entry),
			    MENU (menu)->term_app);
	gtk_table_attach (GTK_TABLE (table), ms->term_entry, 1, 3, 5, 6,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 3);

	table = gtk_table_new (1, 3, TRUE);

	button = gtk_button_new_with_label ("Edit Applications Menu");
	g_signal_connect_after (G_OBJECT (button),
				"clicked",
				G_CALLBACK (edit_apps_menu),
				NULL);
	gtk_table_attach (GTK_TABLE (table), button, 1, 2, 0, 1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	button = gtk_button_new_with_label ("Edit User Apps");
	g_signal_connect_after (G_OBJECT (button),
				"clicked",
				G_CALLBACK (edit_user_apps_menu),
				NULL);
	gtk_table_attach (GTK_TABLE (table), button, 2, 3, 0, 1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	label = gtk_label_new ("Size (width x height)");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);
	ms->width_spin = gtk_spin_button_new (NULL, 1, 0);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (ms->width_spin), 1, 1024);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (ms->width_spin), 1, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (ms->width_spin),
				   MENU_START (ms->menustart)->width);
	gtk_table_attach (GTK_TABLE (table), ms->width_spin, 1, 2, 1, 2,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	ms->height_spin = gtk_spin_button_new (NULL, 1, 0);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (ms->height_spin), 1, 1024);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (ms->height_spin), 1, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (ms->height_spin),
				   MENU_START (ms->menustart)->height);
	gtk_table_attach (GTK_TABLE (table), ms->height_spin, 2, 3, 1, 2,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	label = gtk_label_new ("Columns");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);
	ms->columns_spin = gtk_spin_button_new_with_range (2, 12, 1);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (ms->columns_spin), 1, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (ms->columns_spin),
				   MENU (menu)->columns);
	gtk_table_attach (GTK_TABLE (table), ms->columns_spin, 1, 2, 2, 3,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	label = gtk_label_new ("Set user apps count in 2nd column");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 2, 3, 4,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);
	ms->user_count = gtk_spin_button_new_with_range (1, 128, 1);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (ms->user_count), 1, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (ms->user_count),
				   MENU (menu)->user_apps_count);
	gtk_table_attach (GTK_TABLE (table), ms->user_count, 2, 3, 3, 4,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	ms->mime_check = gtk_check_button_new_with_label
		("Get MIME information when reading directory");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (ms->mime_check), browser->mime_check);
	gtk_table_attach (GTK_TABLE (table), ms->mime_check, 1, 3, 4, 5,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	ms->mime_builtin = gtk_radio_button_new (NULL);
	if (!browser->mime_command) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ms->mime_builtin), TRUE);
	}
	gtk_table_attach (GTK_TABLE (table), ms->mime_builtin, 0, 1, 5, 6,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	label = gtk_label_new ("Use Xfce4 MIME-type module");
	gtk_table_attach (GTK_TABLE (table), label, 1, 3, 5, 6,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	ms->mime_outside = gtk_radio_button_new_from_widget
		(GTK_RADIO_BUTTON (ms->mime_builtin));
	gtk_table_attach (GTK_TABLE (table), ms->mime_outside, 0, 1, 6, 7,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	ms->mime_entry = gtk_entry_new ();
	if (browser->mime_command) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ms->mime_outside), TRUE);
		gtk_entry_set_text (GTK_ENTRY (ms->mime_entry), browser->mime_command);
	}
	gtk_table_attach (GTK_TABLE (table), ms->mime_entry, 1, 3, 6, 7,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 3);

	return vbox;
}

static void
apply_options (gpointer data)
{
	gdouble value;
	struct menu_start *ms = (struct menu_start *) data;
	gchar *app;
	Menu *menu;
	FsBrowser *browser;
	gboolean check;

	menu = MENU (menu_start_get_menu_widget (MENU_START (ms->menustart)));
	browser = FS_BROWSER (menu_start_get_browser_widget
			      (MENU_START (ms->menustart)));

	value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (ms->recent_count));
	menu->r_apps_count = (int) value;

	value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (ms->user_count));
	menu->user_apps_count = (int) value;

	menu_repack_user_apps (menu);
	menu_repack_recent_apps (menu);

	value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (ms->width_spin));
	MENU_START (ms->menustart)->width = value;

	value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (ms->height_spin));
	MENU_START (ms->menustart)->height = value;

	check = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ms->mime_check));
	browser->mime_check = check;

	value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (ms->columns_spin));
	menu->columns = value;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ms->mime_builtin))) {
		if (browser->mime_command) {
			g_free (browser->mime_command);
			browser->mime_command = NULL;
		}
	} else {
		app = g_strdup (gtk_entry_get_text (GTK_ENTRY (ms->mime_entry)));
		g_free (browser->mime_command);
		browser->mime_command = app;
	}

	app = g_strdup (gtk_entry_get_text (GTK_ENTRY (ms->lock_entry)));
	if (MENU_START (ms->menustart)->lock_app)
		free (MENU_START (ms->menustart)->lock_app);
	MENU_START (ms->menustart)->lock_app = app;

	app = g_strdup (gtk_entry_get_text (GTK_ENTRY (ms->switch_entry)));
	if (MENU_START (ms->menustart)->switch_app)
		free (MENU_START (ms->menustart)->switch_app);
	MENU_START (ms->menustart)->switch_app = app;

	app = g_strdup (gtk_entry_get_text (GTK_ENTRY (ms->set_entry)));
	if (menu->set_app)
		free (menu->set_app);
	menu->set_app = app;

	app = g_strdup (gtk_entry_get_text (GTK_ENTRY (ms->run_entry)));
	if (menu->run_app)
		free (menu->run_app);
	menu->run_app = app;

	app = g_strdup (gtk_entry_get_text (GTK_ENTRY (ms->term_entry)));
	if (menu->term_app)
		free (menu->term_app);
	menu->term_app = app;
}

static void
menustart_create_options (Control *ctrl, GtkContainer *con, GtkWidget *done)
{
	GtkWidget *vbox;

	vbox = init_general_page (ctrl);

	gtk_widget_show_all (vbox);

	gtk_container_add (con, vbox);

	g_signal_connect_swapped
		(done, "clicked",
		 G_CALLBACK (apply_options), ctrl->data);
}

static void
read_conf (Control *control, xmlNodePtr node)
{
	struct menu_start *ms = control->data;
	xmlChar *value;
	Menu *menu;
	FsBrowser *browser;

	menu = MENU (menu_start_get_menu_widget (MENU_START (ms->menustart)));
	browser = FS_BROWSER (menu_start_get_browser_widget
			      (MENU_START (ms->menustart)));

	value = xmlGetProp(node, (const xmlChar *) "mime_app");
	if (value && (strlen (value) > 0)) {
		browser->mime_command = value;
	} else {
		browser->mime_command = NULL;
	}

	value = xmlGetProp(node, (const xmlChar *) "mime_check");
	if (value) {
		int check = atoi (value);
		browser->mime_check = check;
	}

	value = xmlGetProp(node, (const xmlChar *) "recent_app_count");
	if (value) {
		int count = atoi (value);
		if (menu->r_apps_count != count) {
			menu->r_apps_count = count;
			menu_repack_recent_apps (menu);
		}
	}

	value = xmlGetProp(node, (const xmlChar *) "user_app_count");
	if (value) {
		int count = atoi (value);
		if (menu->user_apps_count != count) {
			menu->user_apps_count = count;
			menu_repack_user_apps (menu);
		}
	}

	value = xmlGetProp(node, (const xmlChar *) "lock_app");
	if (value) {
		if (MENU_START (ms->menustart)->lock_app)
			free (MENU_START (ms->menustart)->lock_app);
		MENU_START (ms->menustart)->lock_app = value;
	}

	value = xmlGetProp(node, (const xmlChar *) "switch_app");
	if (value) {
		if (MENU_START (ms->menustart)->switch_app)
			free (MENU_START (ms->menustart)->switch_app);
		MENU_START (ms->menustart)->switch_app = value;
	}

	value = xmlGetProp(node, (const xmlChar *) "set_app");
	if (value) {
		if (menu->set_app)
			free (menu->set_app);
		menu->set_app = value;
	}

	value = xmlGetProp(node, (const xmlChar *) "run_app");
	if (value) {
		if (menu->run_app)
			free (menu->run_app);
		menu->run_app = value;
	}

	value = xmlGetProp(node, (const xmlChar *) "term_app");
	if (value) {
		if (menu->term_app)
			free (menu->term_app);
		menu->term_app = value;
	}

	value = xmlGetProp(node, (const xmlChar *) "width");
	if (value) {
		MENU_START (ms->menustart)->width = atoi (value);
	}

	value = xmlGetProp(node, (const xmlChar *) "height");
	if (value) {
		MENU_START (ms->menustart)->height = atoi (value);
	}

	value = xmlGetProp(node, (const xmlChar *) "columns");
	if (value) {
		menu->columns = atoi (value);
	}
}

static void
write_conf (Control *control, xmlNodePtr node)
{
	struct menu_start *ms = control->data;
	Menu *menu;
	FsBrowser *browser;
	char count[4];

	menu = MENU (menu_start_get_menu_widget (MENU_START (ms->menustart)));
	browser = FS_BROWSER (menu_start_get_browser_widget
			      (MENU_START (ms->menustart)));

	if (browser->mime_command) {
		xmlSetProp(node, (const xmlChar *) "mime_app", browser->mime_command);
	} else {
		xmlSetProp(node, (const xmlChar *) "mime_app", "");
	}

	sprintf (count, "%d", browser->mime_check);
	xmlSetProp(node, (const xmlChar *) "mime_check", count);

	sprintf (count, "%d", menu->r_apps_count);
	xmlSetProp(node, (const xmlChar *) "recent_app_count", count);

	sprintf (count, "%d", menu->user_apps_count);
	xmlSetProp(node, (const xmlChar *) "user_app_count", count);

	sprintf (count, "%d", MENU_START (ms->menustart)->width);
	xmlSetProp(node, (const xmlChar *) "width", count);

	sprintf (count, "%d", MENU_START (ms->menustart)->height);
	xmlSetProp(node, (const xmlChar *) "height", count);

	sprintf (count, "%d", menu->columns);
	xmlSetProp(node, (const xmlChar *) "columns", count);

	xmlSetProp(node, (const xmlChar *) "set_app", menu->set_app);
	xmlSetProp(node, (const xmlChar *) "run_app", menu->run_app);
	xmlSetProp(node, (const xmlChar *) "term_app", menu->term_app);

	xmlSetProp(node,
		   (const xmlChar *) "lock_app",
		   MENU_START (ms->menustart)->lock_app
		   ? MENU_START (ms->menustart)->lock_app
		   : "xf4lock");
	xmlSetProp(node,
		   (const xmlChar *) "switch_app",
		   MENU_START (ms->menustart)->switch_app
		   ? MENU_START (ms->menustart)->switch_app
		   : "gdmflexiserver");
}

G_MODULE_EXPORT void xfce_control_class_init (ControlClass *cc)
{
	xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

	cc->name = "menustart";
	cc->caption = _("Xfce4 Panel Menu");

	cc->read_config = read_conf;
	cc->write_config = write_conf;

	cc->create_control = (CreateControlFunc) create_menustart_control;

	cc->create_options = menustart_create_options;

	cc->free = menustart_free;
	cc->attach_callback = menustart_attach_callback;

	cc->set_size = menustart_set_size;
	cc->set_orientation = menustart_set_orientation;

	control_class_set_unique (cc, TRUE);
}

XFCE_PLUGIN_CHECK_INIT
