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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <wordexp.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <libxfce4util/i18n.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include "menu.h"
#include "common.h"
#include "default.xpm"

extern GModule *xfmime_icon_cm;

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

enum {
	TITLE = 0,
	LUNCHER,
	SYSTEM,
	EXTERNAL,
	SEPARATOR,
	MENU,
	BUILTIN
};

struct menu_entry {
	short type;

	struct menu_entry *next;
	struct menu_entry *parent;
	struct menu_entry *child;

	char *name;
	char *command;
	char *icon;

	gboolean term;

	int count;
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

static char *fstab_path = "/etc/fstab";
static char *mtab_path = "/etc/mtab";

static void menu_class_init (MenuClass * klass);
static void menu_init (Menu * menu);

static void show_menu                           (GtkWidget * self, gpointer data);
static void private_cb_eventbox_style_set       (GtkWidget * widget,
						 GtkStyle * old_style);
static void run_menu_app                        (GtkWidget * self, gpointer data);
static void repack_user_buttons                 (Menu *menu, gboolean destroy);
static void menu_destroy                        (GtkObject *object);

static GtkHBoxClass *parent_class = NULL;

static guint fs_browser_signals[LAST_SIGNAL] = { 0 };

/******************************************************************************************/

static void scrolled_box_class_init (ScrolledBoxClass* klass);
static void scrolled_box_init (ScrolledBox* menu);
static gboolean scroll_box (GtkWidget *self, GdkEventScroll *event, gpointer data);
static void size_allocate (GtkWidget *widget, GtkAllocation *allocation, gpointer data);

GType scrolled_box_get_type ()
{
	static GType type = 0;

	type = g_type_from_name ("ScrolledBox");

	if (!type) {
		static const GTypeInfo info = {
			sizeof (ScrolledBoxClass),
			NULL,
			NULL,
			(GClassInitFunc) scrolled_box_class_init,
			NULL,
			NULL,
			sizeof (ScrolledBox),
			0,
			(GInstanceInitFunc) scrolled_box_init,
		};
		type = g_type_register_static (GTK_TYPE_LAYOUT, "ScrolledBox", &info,
					       0);
	}
	return type;
}

static void scrolled_box_class_init (ScrolledBoxClass* klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
}

static void scrolled_box_init (ScrolledBox* sb)
{
	sb->x = 0;
	sb->y = 0;
}

GtkWidget *scrolled_box_new (GtkWidget *child)
{
	ScrolledBox *sb;

	sb = SCROLLED_BOX (g_object_new (scrolled_box_get_type (), NULL));

	sb->child = child;
	gtk_layout_put (GTK_LAYOUT (sb), child, 0, 0);

	g_signal_connect (G_OBJECT (sb), "scroll-event",
			  G_CALLBACK (scroll_box), NULL);
	g_signal_connect (G_OBJECT (sb), "size-allocate",
			  G_CALLBACK (size_allocate), NULL);

	return GTK_WIDGET (sb);
}

static void size_allocate (GtkWidget *self, GtkAllocation *allocation, gpointer data)
{
	GtkAllocation callocation = SCROLLED_BOX (self)->child->allocation;

	callocation.width = allocation->width;
	gtk_widget_size_allocate (SCROLLED_BOX (self)->child, &callocation);
}

static gboolean scroll_box (GtkWidget *self, GdkEventScroll *event, gpointer data)
{
	switch (event->direction) {
	case GDK_SCROLL_UP:
		SCROLLED_BOX (self)->y = MIN (SCROLLED_BOX (self)->y + 20, 0);
		break;
	case GDK_SCROLL_DOWN:
		SCROLLED_BOX (self)->y = MAX
			(SCROLLED_BOX (self)->y - 20,
			 MIN (0, self->allocation.height
			      - SCROLLED_BOX (self)->child->allocation.height));
		break;
	}

	gtk_layout_move (GTK_LAYOUT (self), SCROLLED_BOX (self)->child,
			 SCROLLED_BOX (self)->x, SCROLLED_BOX (self)->y);
}

static void scrolled_box_reset (ScrolledBox *sb)
{
	sb->y = 0;

	gtk_layout_move (GTK_LAYOUT (sb), sb->child, sb->x, sb->y);
}

/******************************************************************************************/

struct menu_entry *append_menu_entry (struct menu_entry *menu, struct menu_entry *entry)
{
	struct menu_entry *tmp = menu;

	if (!menu) {
		return entry;
	}

	for (; tmp->next; tmp = tmp->next);

	tmp->next = entry;

	return menu;
}

struct menu_entry *init_menu_entry (struct menu_entry *parent)
{
	struct menu_entry *entry;

	entry = (struct menu_entry *) malloc (sizeof (struct menu_entry));

	entry->parent = parent;
	entry->next = NULL;
	entry->child = NULL;
	entry->name = NULL;
	entry->command = NULL;
	entry->icon = NULL;
	entry->count = 0;

	return entry;
}

void free_menu_entry (struct menu_entry *entry)
{
	if (entry->next) {
		free_menu_entry (entry->next);
	}
	if (entry->child) {
		free_menu_entry (entry->child);
	}
	if (entry->name) {
		free (entry->name);
	}
	if (entry->command) {
		free (entry->command);
	}
	if (entry->icon) {
		free (entry->icon);
	}
	free (entry);
}

struct menu_entry *get_level (xmlNodePtr node, struct menu_entry *parent)
{
	struct menu_entry *level = NULL, *tmp = NULL;

	for (; node; node = node->next) {
		char *visible = NULL;

		visible = xmlGetProp (node, "visible");

		if (visible && strcmp (visible, "false") == 0) {
			continue;
		}
		if (visible) {
			xmlFree (visible);
		}

		if (xmlStrEqual (node->name, "menu")) {
			tmp = init_menu_entry (parent);
			level = append_menu_entry (level, tmp);

			tmp->type = MENU;
			tmp->name = xmlGetProp (node, "name");
			tmp->icon = xmlGetProp (node, "icon");
			tmp->child = get_level (node->children, tmp);
		}
		if (xmlStrEqual (node->name, "app")) {
			char *term = NULL;

			tmp = init_menu_entry (parent);
			level = append_menu_entry (level, tmp);

			tmp->type = LUNCHER;
			tmp->name = xmlGetProp (node, "name");
			tmp->icon = xmlGetProp (node, "icon");
			tmp->command = xmlGetProp (node, "cmd");
			term = xmlGetProp (node, "term");
			if (term && strcmp (term, "yes") == 0) {
				tmp->term = TRUE;
			} else {
				tmp->term = FALSE;
			}
			if (term) {
				xmlFree (term);
			}
			term = xmlGetProp (node, "count");
			if (term) {
				tmp->count = atoi (term);
				xmlFree (term);
			}
		}
		if (xmlStrEqual (node->name, "separator")) {
			tmp = init_menu_entry (parent);
			level = append_menu_entry (level, tmp);

			tmp->type = SEPARATOR;
		}
		if (xmlStrEqual (node->name, "title")) {
			tmp = init_menu_entry (parent);
			level = append_menu_entry (level, tmp);

			tmp->type = TITLE;
			tmp->name = xmlGetProp (node, "name");
			tmp->icon = xmlGetProp (node, "icon");
		}
/* 		if (xmlStrEqual (node->name, "builtin")) { */
/* 			tmp = init_menu_entry (parent); */
/* 			level = append_menu_entry (level, tmp); */

/* 			tmp->type = BUILTIN; */
/* 		} */
/* 		if (xmlStrEqual (node->name, "include")) { */
/* 			tmp = init_menu_entry (parent); */
/* 			level = append_menu_entry (level, tmp); */
/* 		} */
	}

	return level;
}

struct menu_entry *parse_menu (char *read_file)
{
	xmlDocPtr doc = NULL;
	xmlNodePtr node = NULL;
	struct menu_entry *menu;

	if (!g_file_test (read_file, G_FILE_TEST_EXISTS)) {
		return NULL;
	}

	doc = xmlParseFile (read_file);
	node = xmlDocGetRootElement (doc);

	menu = get_level (node->children, NULL);

	xmlFreeDoc (doc);

	return menu;
}

gboolean parse_menu_2 (char *read_file, struct menu_entry **menu, int count)
{
	xmlDocPtr doc = NULL;
	xmlNodePtr node = NULL;
	int i = 0;

	if (!g_file_test (read_file, G_FILE_TEST_EXISTS)) {
		return FALSE;
	}

	doc = xmlParseFile (read_file);
	node = xmlDocGetRootElement (doc);

	for (node = node->children; node && i < count; node = node->next) {
		if (xmlStrEqual (node->name, "menu")) {
			menu[i] = get_level (node->children, NULL);
			i++;
		}
	}

	xmlFreeDoc (doc);

	return TRUE;
}

xmlNodePtr get_menu_level_xml (struct menu_entry *menu, xmlNodePtr parent)
{
	xmlNodePtr node = NULL;
	char count[6];

	for (; menu; menu = menu->next) {
		switch (menu->type) {
		case MENU:
			node = xmlNewChild(parent, NULL, BAD_CAST "menu", NULL);
			xmlSetProp(node, (const xmlChar *) "name", menu->name);
			if (menu->icon) {
				xmlSetProp(node, (const xmlChar *) "icon", menu->icon);
			}
			if (menu->child) {
				get_menu_level_xml (menu->child, node);
			}
			break;
		case LUNCHER:
			node = xmlNewChild(parent, NULL, BAD_CAST "app", NULL);
			xmlSetProp(node, (const xmlChar *) "name", menu->name);
			xmlSetProp(node, (const xmlChar *) "cmd", menu->command);
			if (menu->icon) {
				xmlSetProp(node, (const xmlChar *) "icon", menu->icon);
			}
			if (menu->term) {
				xmlSetProp(node, (const xmlChar *) "term", "yes");
			}
			if (menu->count) {
				sprintf (count, "%d", menu->count);
				xmlSetProp(node, (const xmlChar *) "count", count);
			}
			break;
		case SEPARATOR:
			node = xmlNewChild(parent, NULL, BAD_CAST "separator", NULL);
			break;
		case TITLE:
			node = xmlNewChild(parent, NULL, BAD_CAST "title", NULL);
			xmlSetProp(node, (const xmlChar *) "name", menu->name);
			if (menu->icon) {
				xmlSetProp(node, (const xmlChar *) "icon", menu->icon);
			}
			break;
		}
	}

	return node;
}

void write_menu (struct menu_entry *menu, char *save_file)
{
	FILE *file;
	xmlDocPtr doc = NULL;
	xmlNodePtr node = NULL, root_node = NULL;

	doc = xmlNewDoc(BAD_CAST "1.0");
	root_node = xmlNewNode(NULL, BAD_CAST "xfdesktop-menu");

	xmlDocSetRootElement(doc, root_node);

	get_menu_level_xml (menu, root_node);

	xmlSaveFormatFile (save_file, doc, 1);
}

struct menu_entry *insert_into_menu (struct menu_entry *menu, struct menu_entry *entry)
{
	struct menu_entry *node, *prev_node = NULL, *tmp = NULL;

	if (menu == NULL) {
		tmp = init_menu_entry (NULL);
		tmp->command = g_strdup (entry->command);
		tmp->icon = g_strdup (entry->icon);
		tmp->name = g_strdup (entry->name);
		tmp->term = entry->term;
		tmp->type = entry->type;

		tmp->count = 1;
		return tmp;
	}

	if (strcmp (menu->command, entry->command) == 0) {
		menu->count++;
		return menu;
	}
	for (node = menu;
	     node->next && strcmp (node->next->command, entry->command);
	     node = node->next) {
		prev_node = node;
	}
	if (node->next == NULL) {
		node->next = init_menu_entry (menu->parent);
		node->next->command = g_strdup (entry->command);
		node->next->icon = g_strdup (entry->icon);
		node->next->name = g_strdup (entry->name);
		node->next->term = entry->term;
		node->next->type = entry->type;
		node->next->count = 1;

		return menu;
	} else {
		tmp = node->next;
		tmp->count++;
		if (node->count < tmp->count) {
			if (prev_node) {
				prev_node->next = tmp;
				node->next = tmp->next;
				tmp->next = node;
			} else {
				node->next = tmp->next;
				tmp->next = node;
				menu = tmp;
			}
			if (tmp->parent && tmp->parent->child == node) {
				tmp->parent->child = tmp;
			}
		}
	}

	return menu;
}

/******************************************************************************************/

enum {
	BM_COMPLETED,
	BM_LAST_SIGNAL
};

static guint bm_signals[BM_LAST_SIGNAL] = { 0 };

static void box_menu_class_init (BoxMenuClass* klass);
static void box_menu_init (BoxMenu* menu);

void box_menu_level (BoxMenu *bm, struct menu_entry *entry);

GType box_menu_get_type ()
{
	static GType type = 0;

	type = g_type_from_name ("BoxMenu");

	if (!type) {
		static const GTypeInfo info = {
			sizeof (BoxMenuClass),
			NULL,
			NULL,
			(GClassInitFunc) box_menu_class_init,
			NULL,
			NULL,
			sizeof (BoxMenu),
			0,
			(GInstanceInitFunc) box_menu_init,
		};
		type = g_type_register_static (GTK_TYPE_VBOX, "BoxMenu", &info,
					       0);
	}
	return type;
}

static void box_menu_class_init (BoxMenuClass* klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GObjectClass *object_class;

	object_class = (GObjectClass *) klass;

	bm_signals[BM_COMPLETED] =
		g_signal_new ("completed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (BoxMenuClass,
					       completed), NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

static void box_menu_init (BoxMenu* bm)
{
	bm->header_button = NULL;

	bm->limit = 0;

	bm->pop_apps = NULL;

	bm->menu_box = gtk_vbox_new (FALSE, 0);
	bm->scrolled_box = scrolled_box_new (bm->menu_box);

	gtk_box_pack_end (GTK_BOX (bm), bm->scrolled_box, TRUE, TRUE, 0);
}

GtkWidget *box_menu_new (struct menu_entry **menu, short type)
{
	BoxMenu *bm;

	bm = BOX_MENU (g_object_new (box_menu_get_type (), NULL));

	bm->menu = menu;
	bm->type = type;

	return GTK_WIDGET (bm);
}

void box_menu_activate_button (GtkWidget *self, gpointer data)
{
	struct menu_entry *menu_entry = (struct menu_entry *) data;
	BoxMenu *bm;

	switch (menu_entry->type) {
	case MENU:
		bm = g_object_get_data (G_OBJECT (self), "box_menu");
		switch (bm->type) {
		case DROP_DOWN:
			
			break;
		case IN_PLACE:
			box_menu_level (bm, menu_entry->child);
			break;
		}
		break;
	case LUNCHER:
		bm = g_object_get_data (G_OBJECT (self), "box_menu");
		g_signal_emit_by_name (G_OBJECT (bm), "completed");
		if (bm->pop_apps) {
			*(bm->pop_apps) = insert_into_menu (*(bm->pop_apps), menu_entry);
		}
		xfce_exec (menu_entry->command, menu_entry->term, FALSE, NULL);
		break;
	}
}

void box_menu_up_level (GtkWidget *self, gpointer data)
{
	struct menu_entry *menu_entry = (struct menu_entry *) data;
	BoxMenu *bm;

	bm = g_object_get_data (G_OBJECT (self), "box_menu");

	box_menu_level (bm, menu_entry->parent ? menu_entry->parent->parent : NULL);
}

void box_menu_level (BoxMenu *bm, struct menu_entry *entry)
{
	struct menu_entry *menu_entry;
	GList *buttons, *iter;
	GtkWidget *button;
	int i = 1;

	if (bm->header_button) {
		gtk_container_remove (GTK_CONTAINER (bm), bm->header_button);
		bm->header_button = NULL;
	}

	if (entry && entry->parent) {
		gchar *title;

		title = entry->parent->name;

		bm->header_button = menu_start_create_button
			("gtk-go-up", title,
			 G_CALLBACK (box_menu_up_level), entry);
		g_object_set_data (G_OBJECT (bm->header_button),
				   "box_menu", (gpointer) bm);
		gtk_box_pack_end (GTK_BOX (bm), bm->header_button, FALSE, TRUE, 0);
	}

	buttons = gtk_container_get_children (GTK_CONTAINER (bm->menu_box));
	for (iter = buttons; iter; iter = iter->next) {
		gtk_container_remove (GTK_CONTAINER (bm->menu_box), GTK_WIDGET (iter->data));
	}

	scrolled_box_reset (SCROLLED_BOX (bm->scrolled_box));

	for (menu_entry = entry ? entry : *bm->menu; menu_entry; menu_entry = menu_entry->next) {
		if (bm->limit && i > bm->limit) {
			break;
		}
		i++;

		switch (menu_entry->type) {
		case LUNCHER:
			button = menu_start_create_button_name
				(menu_entry->icon, menu_entry->name,
				 G_CALLBACK (box_menu_activate_button), menu_entry,
				 FALSE);
			g_object_set_data (G_OBJECT (button), "box_menu",
					   (gpointer) bm);
			gtk_box_pack_start (GTK_BOX (bm->menu_box), button, FALSE, TRUE, 0);
			break;
		case MENU:
			button = menu_start_create_button_name
				(menu_entry->icon, menu_entry->name,
				 G_CALLBACK (box_menu_activate_button), menu_entry,
				 TRUE);
			g_object_set_data (G_OBJECT (button), "box_menu",
					   (gpointer) bm);
			gtk_box_pack_start (GTK_BOX (bm->menu_box), button, FALSE, TRUE, 0);
			break;
		case SEPARATOR:
			button = gtk_hseparator_new ();
			gtk_box_pack_start (GTK_BOX (bm->menu_box), button, FALSE, TRUE, 0);
			break;
		case TITLE:
			button = menu_start_create_button_name
				(menu_entry->icon, menu_entry->name, NULL, NULL, FALSE);
			gtk_widget_set_sensitive (button, FALSE);
			gtk_box_pack_start (GTK_BOX (bm->menu_box), button, FALSE, TRUE, 0);
			break;
		}
	}
	gtk_widget_show_all (GTK_WIDGET (bm));
}

void box_menu_root (BoxMenu *bm)
{
	box_menu_level (bm, NULL);
}

void box_menu_set_menu (BoxMenu *bm, struct menu_entry **menu)
{
	bm->menu = menu;
}

void box_menu_set_most_often_menu (BoxMenu *bm, struct menu_entry **menu)
{
	bm->pop_apps = menu;
}

void box_menu_set_type (BoxMenu *bm, short type)
{
	bm->type = type;
}

void box_menu_set_limit (BoxMenu *bm, int limit)
{
	if (limit < 0) {
		bm->limit = 0;
	}
	bm->limit = limit;
}

/******************************************************************************************/

GtkWidget *get_menu_image (char *icon)
{
	GdkPixbuf *pixbuf = NULL;
	GdkPixbuf *normal_pixbuf = NULL;
	GdkPixbuf *def = NULL;
	GtkWidget *image;

	if (icon) {
		pixbuf = xfce_icon_theme_load
			(xfce_icon_theme_get_for_screen (NULL),
			 icon, 16);

		if (pixbuf) {
			normal_pixbuf = gdk_pixbuf_scale_simple
				(pixbuf, 16, 16, GDK_INTERP_HYPER);
		}

		if (normal_pixbuf) {
			image = gtk_image_new_from_pixbuf (normal_pixbuf);
		} else {
			pixbuf = gdk_pixbuf_new_from_xpm_data (default_xpm);
			image = gtk_image_new_from_pixbuf (pixbuf);
		}
	} else {
		pixbuf = gdk_pixbuf_new_from_xpm_data (default_xpm);
		image = gtk_image_new_from_pixbuf (pixbuf);
	}

	return image;
}

void run_menu_app_cb (GtkWidget *self, gpointer data)
{
	struct menu_entry *menu_entry = (struct menu_entry *) data;
	Menu *menu;

	menu = g_object_get_data (G_OBJECT (self), "menu");

	g_signal_emit_by_name (G_OBJECT (menu), "completed");
	menu->pop_apps = insert_into_menu (menu->pop_apps, menu_entry);
	xfce_exec (menu_entry->command, menu_entry->term, FALSE, NULL);
}

GtkWidget *build_gtk_menu (struct menu_entry *menu, Menu *start)
{
	GtkWidget *menu_widget;
	GtkWidget *submenu_widget;
	GtkWidget *menu_item;

	menu_widget = gtk_menu_new ();

	for (; menu; menu = menu->next) {
		GdkPixbuf *pixbuf = NULL;
		GdkPixbuf *normal_pixbuf = NULL;
		GdkPixbuf *def = NULL;
		GtkWidget *image;

		switch (menu->type) {
		case LUNCHER:
			menu_item = gtk_image_menu_item_new_with_label (menu->name);
			g_object_set_data (G_OBJECT (menu_item), "name-data",
					   menu->name);
			g_object_set_data (G_OBJECT (menu_item), "icon-data",
					   menu->icon);
			g_object_set_data (G_OBJECT (menu_item), "menu", start);

			image = get_menu_image (menu->icon);

			gtk_image_menu_item_set_image
				(GTK_IMAGE_MENU_ITEM (menu_item), image);

			g_object_set_data (G_OBJECT (menu_item), "app",
					   menu->command);
			g_signal_connect_after (G_OBJECT (menu_item),
						"activate",
						G_CALLBACK (run_menu_app_cb),
						menu);

			gtk_widget_show (menu_item);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu_widget),
					       menu_item);
			break;
		case MENU:
			submenu_widget = build_gtk_menu (menu->child, start);
			menu_item = gtk_image_menu_item_new_with_label (menu->name);

			image = get_menu_image (menu->icon);

			gtk_image_menu_item_set_image
				(GTK_IMAGE_MENU_ITEM (menu_item), image);
			gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item),
						   submenu_widget);
			gtk_widget_show (menu_item);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu_widget),
					       menu_item);
			break;
		case SEPARATOR:
			menu_item = gtk_separator_menu_item_new ();
			gtk_widget_show (menu_item);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu_widget),
					       menu_item);
			break;
		case TITLE:
			menu_item = gtk_image_menu_item_new_with_label (menu->name);

			image = get_menu_image (menu->icon);

			gtk_image_menu_item_set_image
				(GTK_IMAGE_MENU_ITEM (menu_item), image);
			gtk_widget_set_sensitive (menu_item, FALSE);
			gtk_widget_show (menu_item);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu_widget),
					       menu_item);
			break;
		}
	}

	gtk_widget_show (menu_widget);

	return menu_widget;
}

static void
run_app (char *app)
{
	xfce_exec(app, FALSE, FALSE, NULL);
}

static void
button_press_g (GtkWidget * self, gpointer data)
{
	Menu *menu = (Menu *) data;

	g_signal_emit_by_name (menu, "completed");
}

static void private_cb_eventbox_style (GtkWidget * widget, GtkStyle * old_style)
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

GtkWidget *create_menu_header_with_label (GtkWidget *label)
{
	GtkStyle *style, *labelstyle;
	GtkWidget *box;

	box = gtk_event_box_new ();

	style = gtk_widget_get_style (box);
	labelstyle = gtk_widget_get_style (label);

	gtk_widget_modify_bg (GTK_WIDGET (box), GTK_STATE_NORMAL,
			      &style->bg[GTK_STATE_SELECTED]);
	gtk_widget_modify_fg (GTK_WIDGET (label), GTK_STATE_NORMAL,
			      &style->fg[GTK_STATE_SELECTED]);

	g_signal_connect_after (G_OBJECT (label), "style_set",
				G_CALLBACK (private_cb_label_style_set),
				NULL);
	g_signal_connect_after (G_OBJECT (box), "style_set",
				G_CALLBACK (private_cb_eventbox_style),
				NULL);

	gtk_container_add (GTK_CONTAINER (box), label);

	return box;
}

GtkWidget *create_menu_header (gchar *title)
{
	GtkWidget *label;

	label = gtk_label_new (title);

	return create_menu_header_with_label (label);
}

static GtkWidget *
menu_start_create_button_image (GtkWidget *image, gchar * text,
				GCallback callback, gpointer data,
				gboolean bold)
{
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *button_hbox;

	button = gtk_button_new ();
	label = gtk_label_new (NULL);
	if (bold) {
		char *markup;

		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		markup = g_strjoin ("", "<b>", text, "</b>", NULL);
		gtk_label_set_markup (GTK_LABEL (label), markup);
		g_free (markup);
	} else {
		gtk_label_set_text (GTK_LABEL (label), text);
	}
	gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	button_hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (button_hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (button_hbox), label, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (button), button_hbox);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);

	g_object_set_data (G_OBJECT (button), "label", label);

	if (callback)
		g_signal_connect (G_OBJECT (button), "clicked",
				  G_CALLBACK (callback), data);

	return button;
}

GtkWidget *menu_start_create_button_name (char *icon, gchar * text,
					  GCallback callback, gpointer data,
					  gboolean bold)
{
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *button_hbox;
	GtkWidget *image;
	GdkPixbuf *pixbuf = NULL;
	GtkStyle *style;

	if (icon && g_file_test(icon, G_FILE_TEST_EXISTS)) {
		pixbuf = gdk_pixbuf_new_from_file_at_size
			(icon, 16, 16, NULL);
	}

	if (!pixbuf && icon && icon[0] != '/') {
		pixbuf = xfce_icon_theme_load (xfce_icon_theme_get_for_screen (NULL), icon, 16);
	}
	if (!pixbuf) {
		pixbuf = gdk_pixbuf_new_from_xpm_data (default_xpm);
	}
	image = gtk_image_new_from_pixbuf (pixbuf);

	return menu_start_create_button_image (image, text, callback, data, bold);
}

GtkWidget *menu_start_create_button (gchar * stock_id, gchar * text,
				     GCallback callback, gpointer data)
{
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *button_hbox;
	GtkWidget *image;

	image = gtk_image_new_from_stock (stock_id,
					  GTK_ICON_SIZE_LARGE_TOOLBAR);

	return menu_start_create_button_image (image, text, callback, data, FALSE);
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

	return menu_start_create_button_image (image, text, callback, data, FALSE);
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

	g_object_set_data (G_OBJECT (button), "label", label);
	g_object_set_data (G_OBJECT (button), "box", button_hbox);
	g_object_set_data (G_OBJECT (button), "arrow", arrow);

	return button;
}

void arrow_button_show_arrow (GtkWidget *button, gboolean show)
{
	GtkWidget *arrow;
	GtkWidget *box;

	arrow = g_object_get_data (G_OBJECT (button), "arrow");
	box = g_object_get_data (G_OBJECT (button), "box");

	if ((show == TRUE && arrow->parent == box)
	    || (show == FALSE && arrow->parent != box)) {
		return;
	}
	if (show == TRUE) {
		gtk_box_pack_start (GTK_BOX (box), arrow, FALSE, FALSE, 4);
	} else {
		g_object_ref (G_OBJECT (arrow));
		gtk_container_remove (GTK_CONTAINER (box), arrow);
	}
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
	int i;

	menu->time = 0;
	menu->menu = NULL;

	menu->atime = 0;
	for (i = 0; i < COLUMNS_COUNT; i++) {
		menu->user_actions[i] = NULL;
	}

	for (i = 0; i < COLUMNS_COUNT; i++) {
		menu->user_apps[i] = NULL;
	}

	for (i = 0; i < COLUMNS_COUNT; i++) {
		menu->menu_boxes[i] = NULL;
	}

	menu->menu_style = TRADITIONAL;
	menu->menu_shown = FALSE;
	menu->first_show_menu = TRUE;

	menu->menu_data = NULL;

	menu->columns = 2;

	menu->r_apps_count = 10;

	gtk_box_set_homogeneous (GTK_BOX (menu), TRUE);
	gtk_box_set_spacing (GTK_BOX (menu), 1);
}

void menu_completed (GtkWidget *self, gpointer data)
{
	Menu *menu = (Menu *) data;

	g_signal_emit_by_name (G_OBJECT (menu), "completed");
}

GtkWidget *menu_new ()
{
	Menu *menu;
	GtkWidget *col_boxes[1];
	GtkWidget *button;
	GList *ra;
	GtkTooltips *tooltips;
	GtkStyle *style;
	GtkWidget *separator;
	GtkWidget *header;
	gchar *read_file;
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

		save_file = ms_get_save_file ("recentapps.xml");
		if (!g_file_test (save_file, G_FILE_TEST_EXISTS)) {
			FILE *file;

			file = fopen (save_file, "w");

			fprintf (file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
			fprintf (file, "<xfdesktop-menu>\n\n");
			fprintf (file, "</xfdesktop-menu>\n");

			fclose (file);
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
			fprintf (file, "<menu name=\"1 Column\">\n");
			fprintf (file, "</menu>\n");
			fprintf (file, "<menu name=\"2 Column\">\n");
			fprintf (file, "</menu>\n");
			fprintf (file, "<menu name=\"3 Column\">\n");
			fprintf (file, "</menu>\n");
			fprintf (file, "<menu name=\"4 Column\">\n");
			fprintf (file, "</menu>\n");
			fprintf (file, "<menu name=\"5 Column\">\n");
			fprintf (file, "</menu>\n");
			fprintf (file, "</xfdesktop-menu>\n");

			fclose (file);
		}
		g_free (save_file);
	}

	read_file = ms_get_read_file ("recentapps.xml");
	menu->pop_apps = parse_menu (read_file);

/* 	if (menu->menu_shown && menu->menu_style == MODERN) { */
		read_file = ms_get_read_file ("menu.xml");
		menu->menu_data = parse_menu (read_file);
		g_free (read_file);
/* 	} */

	/*
	 * left pane 
	 */

	menu->col_boxes[0] = gtk_vbox_new (FALSE, 0);

	menu->rec_app_box = gtk_vbox_new (FALSE, 0);

	if (menu->menu_style == MODERN && menu->menu_shown) {
		menu->app_menu_header = gtk_label_new (_("Applications menu"));
	} else {
		menu->app_menu_header = gtk_label_new (_("Most often used apps"));
	}
	menu->app_header = create_menu_header_with_label (menu->app_menu_header);
	gtk_box_pack_start (GTK_BOX (menu->col_boxes[0]), menu->app_header, FALSE, TRUE, 0);

	menu->rec_app_scroll = scrolled_box_new (menu->rec_app_box);

	if (menu->menu_shown && menu->menu_style == MODERN) {
		menu->box_menu = box_menu_new (&menu->menu_data, IN_PLACE);
	} else {
		menu->box_menu = box_menu_new (&menu->pop_apps, IN_PLACE);
	}
        box_menu_set_most_often_menu (BOX_MENU (menu->box_menu), &menu->pop_apps);
	g_signal_connect (G_OBJECT (menu->box_menu), "completed",
			  G_CALLBACK (menu_completed), menu);
	box_menu_root (BOX_MENU (menu->box_menu));
	gtk_box_pack_start (GTK_BOX (menu->col_boxes[0]), menu->box_menu, TRUE, TRUE, 0);

	separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (menu->col_boxes[0]), separator, FALSE, FALSE, 1);

	if (menu->menu_style == TRADITIONAL || menu->menu_shown == FALSE) {
		menu->app_button = create_arrow_button ("gtk-index", _("Applications"));
	} else {
		menu->app_button = create_arrow_button ("gtk-index", _("Most often used apps"));
	}
	if (menu->menu_style == TRADITIONAL) {
		arrow_button_show_arrow (menu->app_button, TRUE);
	} else {
		arrow_button_show_arrow (menu->app_button, FALSE);
	}
	g_signal_connect (G_OBJECT (menu->app_button), "clicked",
			  G_CALLBACK (show_menu), menu);
	gtk_box_pack_start (GTK_BOX (menu->col_boxes[0]), menu->app_button, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (menu), menu->col_boxes[0], TRUE, TRUE, 0);

	/*
	 * right pane 
	 */

	menu->col_boxes[1] = gtk_vbox_new (FALSE, 0);

	header = create_menu_header (_("User shortcuts"));
	gtk_box_pack_start (GTK_BOX (menu->col_boxes[1]),
			    header, FALSE, FALSE, 0);

	menu->menu_boxes[1] = box_menu_new (&menu->user_apps[1], IN_PLACE);
	g_signal_connect (G_OBJECT (menu->menu_boxes[1]), "completed",
			  G_CALLBACK (menu_completed), menu);
	gtk_box_pack_start (GTK_BOX (menu->col_boxes[1]), menu->menu_boxes[1],
			    TRUE, TRUE, 0);

	header = create_menu_header (_("Extensions"));
	gtk_box_pack_start (GTK_BOX (menu->col_boxes[1]), header, FALSE, TRUE, 0);

	menu->recentfilesbutton = menu_start_create_button
		("gtk-index", _("Recent Files"), NULL, NULL);
	gtk_box_pack_start (GTK_BOX (menu->col_boxes[1]), menu->recentfilesbutton, FALSE, FALSE, 0);

	menu->fsbrowserbutton = menu_start_create_button
		("gtk-open", _("Browse Files"), NULL, NULL);
	gtk_box_pack_start (GTK_BOX (menu->col_boxes[1]),
			    menu->fsbrowserbutton, FALSE, FALSE, 0);

	menu->fstabbutton = menu_start_create_button
		("gtk-harddisk", _("Mount"), NULL, NULL);
	gtk_box_pack_start (GTK_BOX (menu->col_boxes[1]),
			    menu->fstabbutton, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (menu), menu->col_boxes[1], TRUE, TRUE, 0);

	for (i = 2; i < COLUMNS_COUNT; i++) {
		menu->col_boxes[i] = gtk_vbox_new (FALSE, 0);

		header = create_menu_header (_("User shortcuts"));
		gtk_box_pack_start (GTK_BOX (menu->col_boxes[i]), header,
				    FALSE, TRUE, 0);

		menu->menu_boxes[i] = box_menu_new (&menu->user_apps[i], IN_PLACE);
		g_signal_connect (G_OBJECT (menu->menu_boxes[i]), "completed",
				  G_CALLBACK (menu_completed), menu);
		gtk_box_pack_start (GTK_BOX (menu->col_boxes[i]), menu->menu_boxes[i],
				    TRUE, TRUE, 0);

		gtk_box_pack_start (GTK_BOX (menu), menu->col_boxes[i],
				    TRUE, TRUE, 0);
	}

	return GTK_WIDGET (menu);
}

static void menu_destroy (GtkObject *object)
{
	Menu *menu;
	int i;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_MENU (object));

	menu = MENU (object);

	if (menu->menu) {
		gtk_widget_destroy (menu->menu);
		menu->menu = NULL;
	}

	if (menu->menu_data) {
		free_menu_entry (menu->menu_data);
		menu->menu_data = NULL;
	}

	for (i = 1; i < COLUMNS_COUNT; i++) {
		if (menu->user_apps[i]) {
			free_menu_entry (menu->user_apps[i]);
			menu->user_apps[i] = NULL;
		}
	}

	if (menu->pop_apps) {
		char *save_file;

		save_file = ms_get_save_file ("recentapps.xml");
		write_menu (menu->pop_apps, save_file);
		g_free (save_file);
		free_menu_entry (menu->pop_apps);
		menu->pop_apps = NULL;
	}

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

void set_menu_style (Menu *menu, MenuStyle style)
{
	if (menu->menu_style == style) {
		return;
	}

	menu->menu_style = style;
	switch (style) {
	case TRADITIONAL:
		arrow_button_show_arrow (menu->app_button, TRUE);
		box_menu_set_menu (BOX_MENU (menu->box_menu), &menu->pop_apps);
		break;
	case MODERN:
		arrow_button_show_arrow (menu->app_button, FALSE);
		box_menu_set_menu (BOX_MENU (menu->box_menu), &menu->pop_apps);
		break;
	}
}

void show_menu_widget (GtkWidget *widget)
{
	Menu *menu = (Menu *) widget;
	struct stat fi;
	char *read_file;
	int i;
	GtkWidget *label;

	read_file = ms_get_read_file ("userapps.xml");
	stat (read_file, &fi);

	if (fi.st_mtime > menu->atime) {
		menu->atime = fi.st_mtime;

		for (i = 1; i < COLUMNS_COUNT; i++) {
			if (menu->user_apps[i]) {
				free_menu_entry (menu->user_apps[i]);
				menu->user_apps[i] = NULL;
			}
		}
		parse_menu_2 (read_file, menu->user_apps + 1, COLUMNS_COUNT - 1);
	}
	g_free (read_file);

	read_file = ms_get_read_file ("menu.xml");
	stat (read_file, &fi);
	g_free (read_file);

	if (fi.st_mtime > menu->time) {
		menu->time = fi.st_mtime;
		if (menu->menu_data) {
			free_menu_entry (menu->menu_data);
		}
		read_file = ms_get_read_file ("menu.xml");
		menu->menu_data = parse_menu (read_file);
		g_free (read_file);
	}

	label = g_object_get_data (G_OBJECT (menu->app_button), "label");

	if (menu->menu_style == MODERN && menu->first_show_menu) {
		box_menu_set_menu (BOX_MENU (menu->box_menu), &menu->menu_data);
		box_menu_set_limit (BOX_MENU (menu->box_menu), 0);
		menu->menu_shown = TRUE;
		gtk_label_set_text (GTK_LABEL (menu->app_menu_header),
				    _("Applications menu"));
		gtk_label_set_text (GTK_LABEL (label),
				    _("Most often used apps"));
	} else {
		box_menu_set_menu (BOX_MENU (menu->box_menu), &menu->pop_apps);
		box_menu_set_limit (BOX_MENU (menu->box_menu), menu->r_apps_count);
		menu->menu_shown = FALSE;
		gtk_label_set_text (GTK_LABEL (menu->app_menu_header),
				    _("Most often used apps"));
		gtk_label_set_text (GTK_LABEL (label),
				    _("Applications"));
	}
	box_menu_root (BOX_MENU (menu->box_menu));

	for (i = 0; i < menu->columns; i++) {
		gtk_widget_show_all (menu->col_boxes[i]);
		if (menu->menu_boxes[i]) {
			box_menu_root (BOX_MENU (menu->menu_boxes[i]));
		}
	}
	for (; i < COLUMNS_COUNT; i ++) {
		gtk_widget_hide (menu->col_boxes[i]);
	}

	gtk_widget_show (widget);
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
	GtkWidget *label;

	read_file = ms_get_read_file ("menu.xml");
	stat (read_file, &fi);
	g_free (read_file);

	label = g_object_get_data (G_OBJECT (self), "label");

	switch (menu->menu_style) {
	case TRADITIONAL:
		if (fi.st_mtime > menu->time) {
			menu->time = fi.st_mtime;
			if (menu->menu) {
				gtk_widget_destroy (menu->menu);
				menu->menu = NULL;
				free_menu_entry (menu->menu_data);
			}
			read_file = ms_get_read_file ("menu.xml");
			menu->menu_data = parse_menu (read_file);
			g_free (read_file);
		}

		if (!menu->menu) {
			menu->menu = build_gtk_menu (menu->menu_data, menu);

			g_signal_connect
				(G_OBJECT (menu->menu), "deactivate",
				 G_CALLBACK (menu_deactivated), menu);
		}

		time = gtk_get_current_event_time ();
		gtk_menu_popup (GTK_MENU (menu->menu), NULL, NULL,
				(GtkMenuPositionFunc) position_menu,
				self, 0, time);
		break;
	case MODERN:
		if (menu->menu_shown) {
			menu->menu_shown = FALSE;
			gtk_label_set_text (GTK_LABEL (menu->app_menu_header),
					    _("Most often used apps"));
			gtk_label_set_text (GTK_LABEL (label),
					    _("Applications"));
			box_menu_set_menu (BOX_MENU (menu->box_menu), &menu->pop_apps);
			box_menu_set_limit (BOX_MENU (menu->box_menu), menu->r_apps_count);
			box_menu_root (BOX_MENU (menu->box_menu));
		} else {
			if (fi.st_mtime > menu->time) {
				menu->time = fi.st_mtime;
				if (menu->menu_data) {
					free_menu_entry (menu->menu_data);
				}
				read_file = ms_get_read_file ("menu.xml");
				menu->menu_data = parse_menu (read_file);
				g_free (read_file);
			}
			menu->menu_shown = TRUE;
			gtk_label_set_text (GTK_LABEL (menu->app_menu_header),
					    _("Applications menu"));
			gtk_label_set_text (GTK_LABEL (label),
					    _("Most often used apps"));
			box_menu_set_menu (BOX_MENU (menu->box_menu), &menu->menu_data);
			box_menu_set_limit (BOX_MENU (menu->box_menu), 0);
			box_menu_root (BOX_MENU (menu->box_menu));
		}
		break;
	}
}
