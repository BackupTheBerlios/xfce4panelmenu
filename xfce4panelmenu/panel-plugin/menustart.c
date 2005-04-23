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

#include <stdlib.h>

#include <libxfce4util/i18n.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include <panel/main.h>

#include "menustart.h"
#include "common.h"
#include "menu.h"
#include "fstab.h"
#include "fsbrowser.h"

static void     menu_start_class_init (MenuStartClass * klass);
static void     menu_start_init       (MenuStart * ms);
static void     menu_start_paint      (GtkWidget * widget, GdkEventExpose * event);
static gboolean menu_start_expose     (GtkWidget * widget, GdkEventExpose * event);
static gboolean button_press          (GtkWidget * widget, GdkEventButton * event);

static void menu_start_destroy (GtkObject *object);

static void     button_style_cb               (GtkWidget *widget, GtkStyle *old_style);

static GtkWindowClass *parent_class = NULL;

GtkWidget *menu_start_get_menu_widget (MenuStart * ms)
{
	return ms->menu;
}

GtkWidget *menu_start_get_browser_widget (MenuStart * ms)
{
	return ms->fsbrowser;
}

static void private_cb_eventbox_style_set (GtkWidget * widget, GtkStyle * old_style)
{
	static gboolean recursive = 0;
	GtkStyle *style;

	if (recursive > 0)
		return;

	++recursive;
	style = gtk_widget_get_style (widget);
	gtk_widget_modify_bg (widget, GTK_STATE_NORMAL,
			      &style->bg[GTK_STATE_SELECTED]);
	--recursive;
}

static void private_cb_label_style_set (GtkWidget * widget, GtkStyle * old_style)
{
	static gboolean recursive = 0;
	GtkStyle *style;

	if (recursive > 0)
		return;

	++recursive;
	style = gtk_widget_get_style (widget);
	gtk_widget_modify_fg (widget, GTK_STATE_NORMAL,
			      &style->fg[GTK_STATE_SELECTED]);
	--recursive;
}

static void button_style_cb (GtkWidget * widget, GtkStyle * old_style)
{
	static gboolean recursive = 0;
	GtkStyle *style;

	if (recursive > 0)
		return;

	++recursive;
	style = gtk_widget_get_style (widget);
	gtk_widget_modify_bg (widget, GTK_STATE_NORMAL,
			      &style->bg[GTK_STATE_SELECTED]);
	gtk_widget_modify_bg (widget, GTK_STATE_PRELIGHT,
			      &style->bg[GTK_STATE_SELECTED]);
	--recursive;
}

static GtkWidget *create_ms_header (MenuStart *ms)
{
	GtkWidget *box;
	GtkStyle *style, *labelstyle;
	GtkWidget *hbox;
	gchar *markup;

	ms->title = gtk_label_new (NULL);
	gtk_label_set_use_markup (GTK_LABEL (ms->title), TRUE);
	markup = g_strjoin ("", "<big><b>", _("Menu"), "</b></big>", NULL);
	gtk_label_set_markup (GTK_LABEL (ms->title), markup);
	g_free (markup);
	ms->menu_image = gtk_image_new_from_stock
		("gtk-index", GTK_ICON_SIZE_LARGE_TOOLBAR);

	box = gtk_event_box_new ();

	style = gtk_widget_get_style (box);
	labelstyle = gtk_widget_get_style (ms->title);

	gtk_widget_modify_bg (GTK_WIDGET (box), GTK_STATE_NORMAL,
			      &style->bg[GTK_STATE_SELECTED]);
	gtk_widget_modify_fg (GTK_WIDGET (ms->title), GTK_STATE_NORMAL,
			      &style->fg[GTK_STATE_SELECTED]);

	hbox = gtk_hbox_new (FALSE, 10);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);

	gtk_box_pack_start (GTK_BOX (hbox), ms->menu_image, FALSE, FALSE, 1);
	gtk_box_pack_start (GTK_BOX (hbox), ms->title, FALSE, FALSE, 1);

	gtk_container_add (GTK_CONTAINER (box), hbox);

	g_signal_connect_after (G_OBJECT (ms->title), "style_set",
				G_CALLBACK (private_cb_label_style_set),
				NULL);
	g_signal_connect_after (G_OBJECT (box), "style_set",
				G_CALLBACK (private_cb_eventbox_style_set),
				NULL);

	gtk_widget_show_all (box);

	return box;
}

static void update_ms_header (MenuStart *menu, gchar *title, gchar *stock_id)
{
	gchar *markup;

	markup = g_strjoin (NULL, "<big><b>", title, "</b></big>", NULL);

	gtk_label_set_markup (GTK_LABEL (menu->title), markup);
	gtk_image_set_from_stock (GTK_IMAGE (menu->menu_image), stock_id,
				  GTK_ICON_SIZE_LARGE_TOOLBAR);

	g_free (markup);
}

static void show_fstab_widget (GtkWidget *self, gpointer data)
{
	MenuStart *menu = (MenuStart *) data;

	fs_tab_widget_update (FS_TAB_WIDGET (menu->fstab));

	update_ms_header (menu, _("Mounter"), "gtk-harddisk");

	gtk_widget_hide (menu->menu);

	if (!menu->show_header) {
		gtk_widget_show_all (menu->small_header);
	}

	gtk_widget_show_all (menu->fstab);
}

static void hide_fstab_widget (GtkWidget *self, gpointer data)
{
	MenuStart *menu = (MenuStart *) data;

	update_ms_header (menu, _("Menu"), "gtk-index");

	gtk_widget_hide (menu->fstab);

	if (!menu->show_header) {
		gtk_widget_hide (menu->small_header);
	}

	gtk_widget_show (menu->menu);
	show_menu_widget (menu->menu);
}


static void show_fsbrowser_widget (GtkWidget *self, gpointer data)
{
	MenuStart *menu = (MenuStart *) data;

	update_ms_header (menu, _("File Browser"), "gtk-open");

	gtk_widget_hide (menu->menu);

	if (!menu->show_header) {
		gtk_widget_show_all (menu->small_header);
	}

	fs_browser_show (FS_BROWSER (menu->fsbrowser), FILE_BROWSER);
}

static void hide_fsbrowser_widget (GtkWidget *self, gpointer data)
{
	MenuStart *menu = (MenuStart *) data;

	update_ms_header (menu, _("Menu"), "gtk-index");

	gtk_widget_hide (menu->fsbrowser);

	if (!menu->show_header) {
		gtk_widget_hide (menu->small_header);
	}

	gtk_widget_show (menu->menu);
	show_menu_widget (menu->menu);
}

static void show_fsbrowser_widget_with_rf (GtkWidget *self, gpointer data)
{
	MenuStart *menu = (MenuStart *) data;

	update_ms_header (menu, _("File Browser"), "gtk-open");

	gtk_widget_hide (menu->menu);

	if (!menu->show_header) {
		gtk_widget_show_all (menu->small_header);
	}

	fs_browser_show (FS_BROWSER (menu->fsbrowser), RECENTLY_USED);
}

GType menu_start_get_type ()
{
	static GType type = 0;

	type = g_type_from_name ("MenuStart");

	if (!type) {
		static const GTypeInfo info = {
			sizeof (MenuStartClass),
			NULL,
			NULL,
			(GClassInitFunc) menu_start_class_init,
			NULL,
			NULL,
			sizeof (MenuStart),
			0,
			(GInstanceInitFunc) menu_start_init,
		};
		type = g_type_register_static
			(GTK_TYPE_WINDOW, "MenuStart", &info, 0);
	}
	return type;
}

static void menu_start_class_init (MenuStartClass *klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) klass;
	parent_class = g_type_class_peek_parent (klass);
}

static void hide_cb (GtkWidget * self, gpointer data)
{
	menu_start_hide (MENU_START (data));
}

static void get_grab_cb (GtkWidget * self, gpointer data)
{
	MenuStart *menu = (MenuStart *) data;
	GdkWindow *window = g_object_get_data
			(G_OBJECT (menu),
			 "gtk-menu-transfer-window");
	popup_grab_on_window (window, GDK_CURRENT_TIME);
}

static void lock_screen_cb (GtkWidget *self, gpointer data)
{
	MenuStart *ms = (MenuStart *) data;

	menu_start_hide (ms);
	xfce_exec (ms->lock_app, FALSE, FALSE, NULL);
}

static void switch_user_cb (GtkWidget *self, gpointer data)
{
	MenuStart *ms = (MenuStart *) data;

	menu_start_hide (ms);
	xfce_exec (ms->switch_app, FALSE, FALSE, NULL);
}

static void logout_cb (GtkWidget *self, gpointer data)
{
	MenuStart *ms = (MenuStart *) data;

	menu_start_hide (ms);

	quit (FALSE);
}

static GtkWidget *create_foot_button (gchar *stock_id, gchar *text,
				      GCallback cb, gpointer data)
{
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *image;
	GtkStyle *style;
	GtkWidget *button_hbox;

	button = gtk_button_new ();
	image = gtk_image_new_from_stock
		(stock_id, GTK_ICON_SIZE_LARGE_TOOLBAR);
	style = gtk_widget_get_style (button);
	g_signal_connect (G_OBJECT (button), "clicked",
			  cb, data);
	gtk_widget_modify_bg
		(button, GTK_STATE_PRELIGHT, &style->bg[GTK_STATE_SELECTED]);
	gtk_widget_modify_bg
		(button, GTK_STATE_ACTIVE, &style->bg[GTK_STATE_SELECTED]);
	g_signal_connect_after (G_OBJECT (button), "style_set",
				G_CALLBACK (button_style_cb),
				NULL);

	label = gtk_label_new (text);
	g_signal_connect_after (G_OBJECT (label), "style_set",
				G_CALLBACK (private_cb_label_style_set),
				NULL);
	button_hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (button_hbox), image, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (button_hbox), label, FALSE, FALSE, 2);
	gtk_container_add (GTK_CONTAINER (button), button_hbox);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);

	return button;
}

static void menu_start_init (MenuStart *ms)
{
	GdkPixbuf *logo;
	gchar *text;
	GtkWidget *button;
	GtkWidget *image;
	GtkWidget *label;
	GtkStyle *style;
	GtkWidget *button_hbox;
	char *logo_path;

	ms->menu_is_shown = TRUE;

	ms->width = 400;
	ms->height = 480;

	ms->show_header = TRUE;
	ms->show_footer = TRUE;

	gtk_window_set_resizable (GTK_WINDOW (ms), FALSE);

	ms->frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (ms->frame), GTK_SHADOW_OUT);

	ms->vbox = gtk_vbox_new (FALSE, 1);

	gtk_container_set_border_width (GTK_CONTAINER (ms->vbox), 0);

	ms->menu_ebox = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (ms->menu_ebox), ms->vbox);

	gtk_container_add (GTK_CONTAINER (ms->frame), ms->menu_ebox);
	gtk_container_add (GTK_CONTAINER (ms), ms->frame);

	logo_path = g_strdup (ICONDIR "/xfce4_xicon.png");

	logo = gdk_pixbuf_new_from_file (logo_path, NULL);
	g_free (logo_path);

	ms->small_header = create_menu_header (_("Xfce4 Panel Menu"));
	gtk_box_pack_start (GTK_BOX (ms->vbox), ms->small_header, FALSE, TRUE, 0);
	gtk_widget_hide (ms->small_header);

	text = g_strdup_printf (_("Xfce4 Panel Menu"));
	ms->header = create_ms_header (ms);
	gtk_box_pack_start (GTK_BOX (ms->vbox), ms->header, FALSE, FALSE, 0);
	g_object_unref (logo);
	g_free (text);

	ms->fsbrowser = fs_browser_new ();
	g_signal_connect (G_OBJECT (FS_BROWSER (ms->fsbrowser)->closebutton),
			  "clicked", G_CALLBACK (hide_fsbrowser_widget), ms);
	g_signal_connect (G_OBJECT (ms->fsbrowser),
			  "completed", G_CALLBACK (hide_cb), ms);
	gtk_box_pack_start (GTK_BOX (ms->vbox), ms->fsbrowser, TRUE, TRUE, 0);

	ms->fstab = fs_tab_widget_new ();
	g_signal_connect (G_OBJECT (FS_TAB_WIDGET (ms->fstab)->closebutton),
			  "clicked", G_CALLBACK (hide_fstab_widget), ms);
	g_signal_connect (G_OBJECT (ms->fstab),
			  "completed", G_CALLBACK (hide_cb), ms);
	gtk_box_pack_start (GTK_BOX (ms->vbox), ms->fstab, TRUE, TRUE, 0);

	ms->menu = menu_new ();
	g_signal_connect (G_OBJECT (MENU (ms->menu)->recentfilesbutton),
			  "clicked", G_CALLBACK (show_fsbrowser_widget_with_rf), ms);
	g_signal_connect (G_OBJECT (MENU (ms->menu)->fstabbutton),
			  "clicked", G_CALLBACK (show_fstab_widget), ms);
	g_signal_connect (G_OBJECT (MENU (ms->menu)->fsbrowserbutton),
			  "clicked", G_CALLBACK (show_fsbrowser_widget), ms);
	g_signal_connect (G_OBJECT (ms->menu),
			  "completed", G_CALLBACK (hide_cb), ms);
	g_signal_connect (G_OBJECT (ms->menu),
			  "getgrab", G_CALLBACK (get_grab_cb), ms);

	gtk_box_pack_start (GTK_BOX (ms->vbox), ms->menu, TRUE, TRUE, 0);

	/* foot */

	ms->foot = gtk_hbox_new (FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (ms->foot), 3);
	ms->footbox = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (ms->footbox), ms->foot);
	style = gtk_widget_get_style (ms->footbox);
	gtk_widget_modify_bg (ms->footbox, GTK_STATE_NORMAL,
			      &style->bg[GTK_STATE_SELECTED]);
	g_signal_connect_after (G_OBJECT (ms->footbox), "style_set",
				G_CALLBACK (private_cb_eventbox_style_set),
				NULL);

	button = create_foot_button ("gtk-quit", _("Exit"),
				     G_CALLBACK (logout_cb), ms);
	gtk_box_pack_end (GTK_BOX (ms->foot), button, FALSE, FALSE, 1);

	button = create_foot_button ("gtk-dialog-authentication", _("Lock Screen"),
				     G_CALLBACK (lock_screen_cb), ms);
	gtk_box_pack_end (GTK_BOX (ms->foot), button, FALSE, FALSE, 1);

	button = create_foot_button ("gtk-refresh", _("Switch User"),
				     G_CALLBACK (switch_user_cb), ms);
	gtk_box_pack_end (GTK_BOX (ms->foot), button, FALSE, FALSE, 1);

	gtk_box_pack_start (GTK_BOX (ms->vbox), ms->footbox, FALSE, TRUE, 0);

	gtk_widget_show (ms->header);
	gtk_widget_show_all (ms->footbox);

	ms->small_footer = create_menu_header (_("Xfce4 Panel Menu"));
	gtk_box_pack_start (GTK_BOX (ms->vbox), ms->small_footer, FALSE, TRUE, 0);
	gtk_widget_hide (ms->small_footer);

	show_menu_widget (ms->menu);

	g_signal_connect (G_OBJECT (ms), "button-press-event",
			  G_CALLBACK (button_press), NULL);

	gtk_widget_show (GTK_WIDGET (ms->vbox));
	gtk_widget_show (GTK_WIDGET (ms->frame));

	ms->lock_app = g_strdup ("xscreensaver-command --lock");
	ms->switch_app = g_strdup ("gdmflexiserver");
}

GtkWidget *menu_start_new ()
{
	return GTK_WIDGET
		(g_object_new (menu_start_get_type (),
			       "type", GTK_WINDOW_POPUP,
			       NULL));
}

/* from GtkMenu sources */
gboolean popup_grab_on_window (GdkWindow * window, guint32 activate_time)
{
	if ((gdk_pointer_grab (window, TRUE,
			       GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
			       | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK
			       | GDK_POINTER_MOTION_MASK, NULL, NULL,
			       activate_time) == 0)) {
		if (gdk_keyboard_grab (window, TRUE, activate_time) == 0)
			return TRUE;
		else {
			gdk_display_pointer_ungrab (gdk_drawable_get_display
						    (window), activate_time);
			return FALSE;
		}
	}

	return FALSE;
}

/* from GtkMenu sources */
static GdkWindow *menu_grab_transfer_window_get (MenuStart *menu)
{
	GdkWindow *window = g_object_get_data
		(G_OBJECT (menu), "gtk-menu-transfer-window");

	if (!window) {
		GdkWindowAttr attributes;
		gint attributes_mask;

		attributes.x = -100;
		attributes.y = -100;
		attributes.width = 10;
		attributes.height = 10;
		attributes.window_type = GDK_WINDOW_TEMP;
		attributes.wclass = GDK_INPUT_ONLY;
		attributes.override_redirect = TRUE;
		attributes.event_mask = 0;

		attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_NOREDIR;

		window = gdk_window_new (gtk_widget_get_root_window
					 (GTK_WIDGET (menu)), &attributes,
					 attributes_mask);
		gdk_window_set_user_data (window, menu);

		gdk_window_show (window);

		g_object_set_data
			(G_OBJECT (menu), "gtk-menu-transfer-window", window);
	}

	return window;
}

/* from GtkMenu sources */
static void menu_grab_transfer_window_destroy (MenuStart * menu)
{
	GdkWindow *window = g_object_get_data
		(G_OBJECT (menu), "gtk-menu-transfer-window");

	if (window) {
		gdk_window_set_user_data (window, NULL);
		gdk_window_destroy (window);
		g_object_set_data (G_OBJECT (menu),
				   "gtk-menu-transfer-window", NULL);
	}
}

void menu_start_show (MenuStart * ms, int xpos, int ypos, MenuStartPosition pos)
{
	GdkWindow *transfer_window;

	transfer_window = menu_grab_transfer_window_get (ms);
	if (!popup_grab_on_window (transfer_window, GDK_CURRENT_TIME))
		return;

	gtk_widget_set_size_request (GTK_WIDGET (ms), ms->width, ms->height);

	if (ms->show_header) {
		gtk_widget_show (ms->header);
	} else {
		gtk_widget_hide (ms->header);
	}
	gtk_widget_hide (ms->small_header);

	if (ms->show_footer) {
		gtk_widget_show_all (ms->footbox);
		gtk_widget_hide (ms->small_footer);
	} else {
		gtk_widget_hide (ms->footbox);
		gtk_widget_show_all (ms->small_footer);
	}

	gtk_widget_hide (ms->fsbrowser);
	gtk_widget_hide (ms->fstab);

	gtk_widget_show (ms->menu_ebox);
	update_ms_header (ms, _("Menu"), "gtk-index");
	show_menu_widget (ms->menu);

	if (pos == MENU_START_BOTTOM)
		gtk_window_move (GTK_WINDOW (ms), xpos, ypos);
	else {
		gtk_window_move (GTK_WINDOW (ms), xpos, ypos);
	}

	gtk_widget_show (GTK_WIDGET (ms));

	gtk_grab_add (GTK_WIDGET (ms));
}

void menu_start_hide (MenuStart * ms)
{
	gtk_grab_remove (GTK_WIDGET (ms));
	gtk_widget_hide (GTK_WIDGET (ms));
	menu_grab_transfer_window_destroy (ms);
}

static gboolean button_press (GtkWidget *widget, GdkEventButton *event)
{
	if (gtk_get_event_widget ((GdkEvent *) event) == widget) {
		menu_start_hide (MENU_START (widget));
	}

	return FALSE;
}
