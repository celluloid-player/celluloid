/*
 * Copyright (c) 2016-2021 gnome-mpv
 *
 * This file is part of Celluloid.
 *
 * Celluloid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Celluloid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Celluloid.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "celluloid-header-bar.h"
#include "celluloid-menu.h"
#include "celluloid-def.h"

enum
{
	PROP_0,
	PROP_TITLE,
	PROP_OPEN_BUTTON_ACTIVE,
	PROP_MENU_BUTTON_ACTIVE,
	N_PROPERTIES
};

struct _CelluloidHeaderBar
{
	GtkBox parent_instance;
	GtkWidget *header_bar;
	GtkWidget *open_btn;
	GtkWidget *fullscreen_btn;
	GtkWidget *menu_btn;

	gchar *title;
	gboolean open_popover_visible;
	gboolean menu_popover_visible;
};

struct _CelluloidHeaderBarClass
{
	GtkBoxClass parent_class;
};

G_DEFINE_TYPE(CelluloidHeaderBar, celluloid_header_bar, GTK_TYPE_BOX)

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidHeaderBar *self = CELLULOID_HEADER_BAR(object);

	switch(property_id)
	{
		case PROP_TITLE:
		g_free(self->title);
		self->title = g_value_dup_string(value);
		break;

		case PROP_OPEN_BUTTON_ACTIVE:
		self->open_popover_visible = g_value_get_boolean(value);
		break;

		case PROP_MENU_BUTTON_ACTIVE:
		self->menu_popover_visible = g_value_get_boolean(value);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec )
{
	CelluloidHeaderBar *self = CELLULOID_HEADER_BAR(object);

	switch(property_id)
	{
		case PROP_TITLE:
		g_value_set_string(value, self->title);
		break;

		case PROP_OPEN_BUTTON_ACTIVE:
		g_value_set_boolean(value, self->open_popover_visible);
		break;

		case PROP_MENU_BUTTON_ACTIVE:
		g_value_set_boolean(value, self->menu_popover_visible);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}
static void
celluloid_header_bar_class_init(CelluloidHeaderBarClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	object_class->set_property = set_property;
	object_class->get_property = get_property;

	pspec = g_param_spec_string
		(	"title",
			"Title",
			"The title to display in the header bar",
			NULL,
			G_PARAM_READWRITE );
	g_object_class_install_property
		(object_class, PROP_TITLE, pspec);

	pspec = g_param_spec_boolean
		(	"open-button-active",
			"Open button active",
			"Whether or not the open button is active",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property
		(object_class, PROP_OPEN_BUTTON_ACTIVE, pspec);

	pspec = g_param_spec_boolean
		(	"menu-button-active",
			"Menu button active",
			"Whether or not the menu button is active",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property
		(object_class, PROP_MENU_BUTTON_ACTIVE, pspec);
}

static void
celluloid_header_bar_init(CelluloidHeaderBar *hdr)
{
	GtkHeaderBar *ghdr;
	GSettings *settings;
	gboolean csd;
	GMenu *open_btn_menu;
	GMenu *menu_btn_menu;
	GtkWidget *open_icon;
	GtkWidget *fullscreen_icon;
	GtkWidget *menu_icon;

	settings = g_settings_new(CONFIG_ROOT);
	csd = g_settings_get_boolean(settings, "csd-enable");
	open_btn_menu = g_menu_new();
	menu_btn_menu = g_menu_new();

	open_icon =		gtk_image_new_from_icon_name
				("list-add-symbolic", GTK_ICON_SIZE_MENU);
	fullscreen_icon =	gtk_image_new_from_icon_name
				("view-fullscreen-symbolic", GTK_ICON_SIZE_MENU);
	menu_icon =		gtk_image_new_from_icon_name
				("open-menu-symbolic", GTK_ICON_SIZE_MENU);

	hdr->header_bar = gtk_header_bar_new();
	hdr->open_btn = gtk_menu_button_new();
	hdr->fullscreen_btn = gtk_button_new();
	hdr->menu_btn = gtk_menu_button_new();
	hdr->open_popover_visible = FALSE;
	hdr->menu_popover_visible = FALSE;

	ghdr = GTK_HEADER_BAR(hdr->header_bar);

	celluloid_menu_build_open_btn(open_btn_menu, NULL);
	celluloid_menu_build_menu_btn(menu_btn_menu, NULL);

	g_object_set(open_icon, "use-fallback", TRUE, NULL);
	g_object_set(fullscreen_icon, "use-fallback", TRUE, NULL);
	g_object_set(menu_icon, "use-fallback", TRUE, NULL);

	gtk_button_set_image(GTK_BUTTON(hdr->open_btn), open_icon);
	gtk_button_set_image(GTK_BUTTON(hdr->fullscreen_btn), fullscreen_icon);
	gtk_button_set_image(GTK_BUTTON(hdr->menu_btn), menu_icon);

	gtk_menu_button_set_menu_model
		(	GTK_MENU_BUTTON(hdr->open_btn),
			G_MENU_MODEL(open_btn_menu) );
	gtk_menu_button_set_menu_model
		(	GTK_MENU_BUTTON(hdr->menu_btn),
			G_MENU_MODEL(menu_btn_menu) );

	gtk_widget_set_tooltip_text
		(hdr->fullscreen_btn, _("Toggle Fullscreen"));
	gtk_actionable_set_action_name
		(GTK_ACTIONABLE(hdr->fullscreen_btn), "win.toggle-fullscreen");

	gtk_widget_set_can_focus(hdr->open_btn, FALSE);
	gtk_widget_set_can_focus(hdr->fullscreen_btn, FALSE);
	gtk_widget_set_can_focus(hdr->menu_btn, FALSE);

	gtk_header_bar_pack_start(ghdr, hdr->open_btn);
	gtk_header_bar_pack_end(ghdr, hdr->menu_btn);
	gtk_header_bar_pack_end(ghdr, hdr->fullscreen_btn);

	gtk_box_pack_start(GTK_BOX(hdr), hdr->header_bar, TRUE, TRUE, 0);

	gtk_widget_set_no_show_all(hdr->fullscreen_btn, TRUE);
	gtk_header_bar_set_show_close_button(ghdr, TRUE);

	g_object_bind_property(	hdr, "title",
				hdr->header_bar, "title",
				G_BINDING_DEFAULT );
	g_object_bind_property(	hdr->open_btn, "active",
				hdr, "open-button-active",
				G_BINDING_DEFAULT );
	g_object_bind_property(	hdr->menu_btn, "active",
				hdr, "menu-button-active",
				G_BINDING_DEFAULT );

	gtk_widget_set_visible(hdr->fullscreen_btn, csd);

	g_object_unref(settings);
}

GtkWidget *
celluloid_header_bar_new()
{
	return GTK_WIDGET(g_object_new(celluloid_header_bar_get_type(), NULL));
}

gboolean
celluloid_header_bar_get_open_button_popup_visible(CelluloidHeaderBar *hdr)
{
	GtkMenuButton *btn = GTK_MENU_BUTTON(hdr->open_btn);
	GtkWidget *popover = GTK_WIDGET(gtk_menu_button_get_popover(btn));

	return gtk_widget_is_visible(popover);
}

void
celluloid_header_bar_set_open_button_popup_visible(	CelluloidHeaderBar *hdr,
							gboolean visible )
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hdr->open_btn), visible);
}

gboolean
celluloid_header_bar_get_menu_button_popup_visible(CelluloidHeaderBar *hdr)
{
	GtkMenuButton *btn = GTK_MENU_BUTTON(hdr->menu_btn);
	GtkWidget *popover = GTK_WIDGET(gtk_menu_button_get_popover(btn));

	return gtk_widget_is_visible(popover);
}

void
celluloid_header_bar_set_menu_button_popup_visible(	CelluloidHeaderBar *hdr,
							gboolean visible )
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hdr->menu_btn), visible);
}

void
celluloid_header_bar_set_fullscreen_state(	CelluloidHeaderBar *hdr,
						gboolean fullscreen )
{
	GtkWidget *image = gtk_button_get_image(GTK_BUTTON(hdr->fullscreen_btn));

	gtk_image_set_from_icon_name(	GTK_IMAGE(image),
					fullscreen?
					"view-restore-symbolic":
					"view-fullscreen-symbolic",
					GTK_ICON_SIZE_MENU );

	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hdr->header_bar), !fullscreen);
}

void
celluloid_header_bar_update_track_list(	CelluloidHeaderBar *hdr,
					const GPtrArray *track_list )
{
	GMenu *menu = g_menu_new();

	celluloid_menu_build_menu_btn(menu, track_list);
	gtk_menu_button_set_menu_model
		(GTK_MENU_BUTTON(hdr->menu_btn), G_MENU_MODEL(menu));
}

void
celluloid_header_bar_update_disc_list(	CelluloidHeaderBar *hdr,
					const GPtrArray *disc_list )
{
	GMenu *menu = g_menu_new();

	celluloid_menu_build_open_btn(menu, disc_list);

	gtk_menu_button_set_menu_model
		(GTK_MENU_BUTTON(hdr->open_btn), G_MENU_MODEL(menu));
}
