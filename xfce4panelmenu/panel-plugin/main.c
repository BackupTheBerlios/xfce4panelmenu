#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <gtk/gtk.h>

#include <libxfce4util/i18n.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include <panel/plugins.h>
#include <panel/xfce.h>

#include <locale.h>

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

	gchar *icon;
	GtkWidget *image;
	GtkWidget *box;

	GtkWidget *icon_button;

	GtkWidget *width_spin;
	GtkWidget *height_spin;

	GtkWidget *columns_spin;

	GtkWidget *mime_check;

	GtkWidget *show_header;
	GtkWidget *show_footer;

	GtkWidget *mime_builtin;
	GtkWidget *mime_outside;
	GtkWidget *mime_entry;

	GtkWidget *user_count;
	GtkWidget *recent_count;
	GtkWidget *lock_entry;
	GtkWidget *switch_entry;

	GtkWidget *menu_trad;
	GtkWidget *menu_mod;

	GtkWidget *menu_first;
	GtkWidget *menu_second;
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

	menustart->icon = NULL;

	menustart->button = gtk_button_new ();
	gtk_button_set_relief(GTK_BUTTON (menustart->button), GTK_RELIEF_NONE);
	read_file = g_strdup (ICONDIR "/xfce4_xicon.png");
	pixbuf = gdk_pixbuf_new_from_file_at_size (read_file, 16, 16, NULL);
	g_free (read_file);
 	menustart->image = gtk_image_new_from_pixbuf (pixbuf);
	menustart->box = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (menustart->box), menustart->image, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (menustart->button), menustart->box);
	g_signal_connect (G_OBJECT (menustart->button), "clicked",
			  G_CALLBACK (button_clicked), menustart->menustart);

	gtk_container_add (GTK_CONTAINER (control->base), menustart->button);
	gtk_widget_set_size_request (control->base, -1, -1);

	control->data = (gpointer) menustart;
	control->with_popup = FALSE;

	gtk_widget_show_all (menustart->button);

	return TRUE;
}

void set_menustart_icon (struct menu_start *menustart, const gchar *path)
{
	GdkPixbuf *pixbuf;

	if (menustart->icon) {
		g_free (menustart->icon);
		menustart->icon = NULL;
	}
	gtk_container_remove (GTK_CONTAINER (menustart->box), menustart->image);

	if (g_file_test (path, G_FILE_TEST_EXISTS)) {
		pixbuf = gdk_pixbuf_new_from_file_at_size (path, 16, 16, NULL);
		if (pixbuf) {
			menustart->icon = g_strdup (path);
			menustart->image = gtk_image_new_from_pixbuf (pixbuf);
			gtk_box_pack_start (GTK_BOX (menustart->box), menustart->image, FALSE, FALSE, 0);
			gtk_widget_show_all (menustart->box);

			return;
		}
	}

	pixbuf = gdk_pixbuf_new_from_file_at_size (ICONDIR "/xfce4_xicon.png", 16, 16, NULL);
	menustart->image = gtk_image_new_from_pixbuf (pixbuf);
	gtk_box_pack_start (GTK_BOX (menustart->box), menustart->image, FALSE, FALSE, 0);
	gtk_widget_show_all (menustart->box);
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

GtkWidget *init_browser_page (Control *ctrl)
{
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *label;
	FsBrowser *browser;
	struct menu_start *ms = (struct menu_start *) ctrl->data;

	browser = FS_BROWSER (menu_start_get_browser_widget
			      (MENU_START (ms->menustart)));

	vbox = gtk_vbox_new (FALSE, 1);

	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	table = gtk_table_new (6, 3, TRUE);

	ms->mime_check = gtk_check_button_new_with_label
		(_("Get MIME information when reading directory"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (ms->mime_check), browser->mime_check);
	gtk_table_attach (GTK_TABLE (table), ms->mime_check, 1, 3, 0, 1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	ms->mime_builtin = gtk_radio_button_new_with_label
		(NULL, _("Use Xfce4 MIME-type module when opening files"));
	if (!browser->mime_command) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ms->mime_builtin), TRUE);
	}
	gtk_table_attach (GTK_TABLE (table), ms->mime_builtin, 0, 3, 1, 2,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	ms->mime_outside = gtk_radio_button_new_with_label_from_widget
		(GTK_RADIO_BUTTON (ms->mime_builtin), _("or use external program (e.g. rox):"));
	gtk_table_attach (GTK_TABLE (table), ms->mime_outside, 0, 2, 2, 3,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	ms->mime_entry = gtk_entry_new ();
	if (browser->mime_command) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ms->mime_outside), TRUE);
		gtk_entry_set_text (GTK_ENTRY (ms->mime_entry), browser->mime_command);
	}
	gtk_table_attach (GTK_TABLE (table), ms->mime_entry, 2, 3, 2, 3,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 3);

	return vbox;
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

	vbox = gtk_vbox_new (FALSE, 5);

	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

	table = gtk_table_new (4, 3, TRUE);

	label = gtk_label_new (_("Menu Icon"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	ms->icon_button = gtk_file_chooser_button_new (_("Select file"), GTK_FILE_CHOOSER_ACTION_OPEN);
	if (ms->icon) {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (ms->icon_button), ms->icon);
	}
	gtk_table_attach (GTK_TABLE (table), ms->icon_button, 1, 3, 0, 1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	label = gtk_label_new (_("Switch User Command"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);
	ms->switch_entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (ms->switch_entry),
			    MENU_START (ms->menustart)->switch_app);
	gtk_table_attach (GTK_TABLE (table), ms->switch_entry, 1, 3, 1, 2,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	label = gtk_label_new (_("Lock Screen Command"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);
	ms->lock_entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (ms->lock_entry),
			    MENU_START (ms->menustart)->lock_app);
	gtk_table_attach (GTK_TABLE (table), ms->lock_entry, 1, 3, 2, 3,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	ms->show_header = gtk_check_button_new_with_label (_("Show Header"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ms->show_header),
				      MENU_START (((struct menu_start *) ctrl->data)->menustart)->show_header);
	gtk_table_attach (GTK_TABLE (table), ms->show_header, 0, 1, 3, 4,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	ms->show_footer = gtk_check_button_new_with_label (_("Show Footer"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ms->show_footer),
				      MENU_START (((struct menu_start *) ctrl->data)->menustart)->show_footer);
	gtk_table_attach (GTK_TABLE (table), ms->show_footer, 1, 2, 3, 4,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 3);

	table = gtk_table_new (1, 3, TRUE);

	button = gtk_button_new_with_label (_("Edit Applications Menu"));
	g_signal_connect_after (G_OBJECT (button),
				"clicked",
				G_CALLBACK (edit_apps_menu),
				NULL);
	gtk_table_attach (GTK_TABLE (table), button, 1, 2, 0, 1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	button = gtk_button_new_with_label (_("Edit User Apps"));
	g_signal_connect_after (G_OBJECT (button),
				"clicked",
				G_CALLBACK (edit_user_apps_menu),
				NULL);
	gtk_table_attach (GTK_TABLE (table), button, 2, 3, 0, 1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	label = gtk_label_new (_("Size (width x height)"));
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

	label = gtk_label_new (_("Columns"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);
	ms->columns_spin = gtk_spin_button_new_with_range (2, COLUMNS_COUNT, 1);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (ms->columns_spin), 1, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (ms->columns_spin),
				   MENU (menu)->columns);
	gtk_table_attach (GTK_TABLE (table), ms->columns_spin, 1, 2, 2, 3,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 3);

	return vbox;
}

void menu_style_toggled (GtkWidget *self, gpointer data)
{
	struct menu_start *ms = (struct menu_start *) data;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ms->menu_trad))) {
		gtk_widget_set_sensitive (ms->menu_first, FALSE);
		gtk_widget_set_sensitive (ms->menu_second, FALSE);
	} else {
		gtk_widget_set_sensitive (ms->menu_first, TRUE);
		gtk_widget_set_sensitive (ms->menu_second, TRUE);
	}
}

GtkWidget *init_menu_page (Control *ctrl)
{
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *label;
	Menu *menu;
	struct menu_start *ms = (struct menu_start *) ctrl->data;

	menu = MENU (menu_start_get_menu_widget
		     (MENU_START (((struct menu_start *) ctrl->data)->menustart)));

	vbox = gtk_vbox_new (FALSE, 1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	table = gtk_table_new (4, 3, TRUE);

	label = gtk_label_new (_("Items count in recent apps menu"));
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

	label = gtk_label_new (_("For applications menu use:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 3, 1, 2,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	ms->menu_trad = gtk_radio_button_new_with_label
		(NULL, _("eXPerince style"));
	if (menu->menu_style == TRADITIONAL) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ms->menu_trad), TRUE);
	}
	gtk_table_attach (GTK_TABLE (table), ms->menu_trad, 1, 3, 1, 2,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	ms->menu_mod = gtk_radio_button_new_with_label_from_widget
		(GTK_RADIO_BUTTON (ms->menu_trad), _("or new style"));
	if (menu->menu_style == MODERN) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ms->menu_mod), TRUE);
	}
	gtk_table_attach (GTK_TABLE (table), ms->menu_mod, 1, 3, 2, 3,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	label = gtk_label_new (_("First show:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 3, 3, 4,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	ms->menu_first = gtk_radio_button_new_with_label
		(NULL, _("Applications menu"));
	if (menu->first_show_menu) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ms->menu_first), TRUE);
	}
	gtk_table_attach (GTK_TABLE (table), ms->menu_first, 1, 3, 3, 4,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	ms->menu_second = gtk_radio_button_new_with_label_from_widget
		(GTK_RADIO_BUTTON (ms->menu_first), _("Most often used apps menu"));
	if (!menu->first_show_menu) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ms->menu_second), TRUE);
	}
	gtk_table_attach (GTK_TABLE (table), ms->menu_second, 1, 3, 4, 5,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);

	g_signal_connect (G_OBJECT (ms->menu_trad), "toggled", G_CALLBACK (menu_style_toggled), ms);
	menu_style_toggled (ms->menu_trad, ms);

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

	value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (ms->width_spin));
	MENU_START (ms->menustart)->width = value;

	value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (ms->height_spin));
	MENU_START (ms->menustart)->height = value;

	gtk_widget_set_size_request (ms->menustart,
				     MENU_START (ms->menustart)->width, MENU_START (ms->menustart)->height);

	check = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ms->mime_check));
	browser->mime_check = check;

	check = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ms->show_header));
	MENU_START (ms->menustart)->show_header = check;

	check = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ms->show_footer));
	MENU_START (ms->menustart)->show_footer = check;

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

	app = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (ms->icon_button));
	set_menustart_icon (ms, app);
	g_free (app);

	app = g_strdup (gtk_entry_get_text (GTK_ENTRY (ms->lock_entry)));
	if (MENU_START (ms->menustart)->lock_app)
		free (MENU_START (ms->menustart)->lock_app);
	MENU_START (ms->menustart)->lock_app = app;

	app = g_strdup (gtk_entry_get_text (GTK_ENTRY (ms->switch_entry)));
	if (MENU_START (ms->menustart)->switch_app)
		free (MENU_START (ms->menustart)->switch_app);
	MENU_START (ms->menustart)->switch_app = app;

	check = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ms->menu_trad));
	if (check) {
		set_menu_style (menu, TRADITIONAL);
	} else {
		set_menu_style (menu, MODERN);
	}

	check = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ms->menu_first));
	if (check) {
		menu->first_show_menu = TRUE;
	} else {
		menu->first_show_menu = FALSE;
	}
}

static void
menustart_create_options (Control *ctrl, GtkContainer *con, GtkWidget *done)
{
	GtkWidget *vbox;
	GtkWidget *notebook;
	GtkWidget *label;

	notebook = gtk_notebook_new ();

	label = gtk_label_new (_("General"));
	vbox = init_general_page (ctrl);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);

	label = gtk_label_new (_("Applications Menu"));
	vbox = init_menu_page (ctrl);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);

	label = gtk_label_new (_("Files Browser"));
	vbox = init_browser_page (ctrl);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);

	gtk_widget_show_all (notebook);

	gtk_container_add (con, notebook);

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
		}
	}

	value = xmlGetProp(node, (const xmlChar *) "menu_icon");
	if (value) {
		set_menustart_icon (ms, value);
		g_free (value);
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

	value = xmlGetProp(node, (const xmlChar *) "show_header");
	if (value) {
		int v = atoi (value);
		MENU_START (ms->menustart)->show_header = v;
	} else {
		MENU_START (ms->menustart)->show_header = TRUE;
	}

	value = xmlGetProp(node, (const xmlChar *) "show_footer");
	if (value) {
		int v = atoi (value);
		MENU_START (ms->menustart)->show_footer = v;
	} else {
		MENU_START (ms->menustart)->show_footer = TRUE;
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

	value = xmlGetProp(node, (const xmlChar *) "menu_style");
	if (value && strcmp (value, "modern") == 0) {
		set_menu_style (menu, MODERN);
	} else {
		set_menu_style (menu, TRADITIONAL);
	}
	if (value) {
		xmlFree (value);
	}

	value = xmlGetProp(node, (const xmlChar *) "menu_first");
	if (value && strcmp (value, "true") == 0) {
		menu->first_show_menu = TRUE;
	} else {
		menu->first_show_menu = FALSE;
	}
	if (value) {
		xmlFree (value);
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

	sprintf (count, "%d", MENU_START (ms->menustart)->show_header);
	xmlSetProp(node, (const xmlChar *) "show_header", count);

	sprintf (count, "%d", MENU_START (ms->menustart)->show_footer);
	xmlSetProp(node, (const xmlChar *) "show_footer", count);

	sprintf (count, "%d", MENU_START (ms->menustart)->width);
	xmlSetProp(node, (const xmlChar *) "width", count);

	sprintf (count, "%d", MENU_START (ms->menustart)->height);
	xmlSetProp(node, (const xmlChar *) "height", count);

	sprintf (count, "%d", menu->columns);
	xmlSetProp(node, (const xmlChar *) "columns", count);

	if (ms->icon) {
		xmlSetProp (node, "menu_icon", ms->icon);
	}

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

	if (menu->menu_style == MODERN) {
		xmlSetProp(node, (const xmlChar *) "menu_style", "modern");
	} else {
		xmlSetProp(node, (const xmlChar *) "menu_style", "traditional");
	}

	if (menu->first_show_menu) {
		xmlSetProp(node, (const xmlChar *) "menu_first", "true");
	} else {
		xmlSetProp(node, (const xmlChar *) "menu_first", "false");
	}
}

G_MODULE_EXPORT void xfce_control_class_init (ControlClass *cc)
{
	xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

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
