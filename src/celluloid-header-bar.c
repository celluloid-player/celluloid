/*
 * Copyright (c) 2016-2022, 2024 gnome-mpv
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
	PROP_FULLSCREENED,
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

	gboolean fullscreened;
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
		GParamSpec *pspec );

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec );

static void
create_popup(GtkMenuButton *menu_button, gpointer data);

static void
set_fullscreen_state(CelluloidHeaderBar *hdr, gboolean fullscreen);

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidHeaderBar *self = CELLULOID_HEADER_BAR(object);

	switch(property_id)
	{
		case PROP_FULLSCREENED:
		self->fullscreened = g_value_get_boolean(value);
		set_fullscreen_state(self, self->fullscreened);
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
		case PROP_FULLSCREENED:
		g_value_set_boolean(value, self->fullscreened);
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
create_popup(GtkMenuButton *menu_button, gpointer data)
{
	// Bind the 'visible' property then unset the popup func. We can't do
	// this in the init function because the popover will only be created
	// when the button is activated for the first time.

	GtkPopover *menu_popover = gtk_menu_button_get_popover(menu_button);

	g_object_bind_property
		(	menu_popover, "visible",
			data, "menu-button-active",
			G_BINDING_DEFAULT );

	gtk_menu_button_set_create_popup_func(menu_button, NULL, NULL, NULL);
}

static void
set_fullscreen_state(CelluloidHeaderBar *hdr, gboolean fullscreen)
{
	GSettings *settings =
		g_settings_new(CONFIG_ROOT);
	const gchar *icon_name =
		fullscreen ?
		"view-restore-symbolic" :
		"view-fullscreen-symbolic";
	const gboolean show_title_buttons =
		!fullscreen ||
		g_settings_get_boolean(settings, "always-show-title-buttons");

	gtk_button_set_icon_name
		(GTK_BUTTON(hdr->fullscreen_btn), icon_name);
	gtk_header_bar_set_show_title_buttons
		(GTK_HEADER_BAR(hdr->header_bar), show_title_buttons);

	g_object_unref(settings);
}

static void
celluloid_header_bar_class_init(CelluloidHeaderBarClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	object_class->set_property = set_property;
	object_class->get_property = get_property;

	pspec = g_param_spec_boolean
		(	"fullscreened",
			"Fullscreened",
			"Whether the header bar is in fullscreen configuration",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property
		(object_class, PROP_FULLSCREENED, pspec);

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

	settings = g_settings_new(CONFIG_ROOT);
	csd = g_settings_get_boolean(settings, "csd-enable");
	open_btn_menu = g_menu_new();
	menu_btn_menu = g_menu_new();

	hdr->header_bar = gtk_header_bar_new();
	hdr->open_btn = gtk_menu_button_new();
	hdr->fullscreen_btn =
		gtk_button_new_from_icon_name("view-fullscreen-symbolic");
	hdr->menu_btn = gtk_menu_button_new();
	hdr->fullscreened = FALSE;
	hdr->open_popover_visible = FALSE;
	hdr->menu_popover_visible = FALSE;

	ghdr = GTK_HEADER_BAR(hdr->header_bar);

	celluloid_menu_build_open_btn(open_btn_menu, NULL);
	celluloid_menu_build_menu_btn(menu_btn_menu);

	gtk_menu_button_set_icon_name
		(GTK_MENU_BUTTON(hdr->open_btn), "list-add-symbolic");
	gtk_menu_button_set_icon_name
		(GTK_MENU_BUTTON(hdr->menu_btn), "open-menu-symbolic");

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

	gtk_widget_set_hexpand(GTK_WIDGET(ghdr), TRUE);

	gtk_header_bar_pack_start(ghdr, hdr->open_btn);
	gtk_header_bar_pack_end(ghdr, hdr->menu_btn);
	gtk_header_bar_pack_end(ghdr, hdr->fullscreen_btn);

	gtk_box_prepend(GTK_BOX(hdr), hdr->header_bar);
	gtk_header_bar_set_show_title_buttons(ghdr, TRUE);
	gtk_widget_set_visible(hdr->fullscreen_btn, csd);

	gtk_menu_button_set_create_popup_func
		(GTK_MENU_BUTTON(hdr->open_btn), create_popup, hdr, NULL);
	gtk_menu_button_set_create_popup_func
		(GTK_MENU_BUTTON(hdr->menu_btn), create_popup, hdr, NULL);

	gtk_menu_button_set_primary(GTK_MENU_BUTTON(hdr->menu_btn), TRUE);

	gchar css_data[] = ".floating-header {background: rgba(0,0,0,0.7); border-radius: 12px; box-shadow: none;}";
	GtkCssProvider *css = gtk_css_provider_new();
	gtk_css_provider_load_from_data(css, css_data, -1);

	gtk_style_context_add_provider_for_display
		(	gtk_widget_get_display(GTK_WIDGET(hdr)),
			GTK_STYLE_PROVIDER(css),
			GTK_STYLE_PROVIDER_PRIORITY_USER );

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
	if(visible)
	{
		gtk_menu_button_popup(GTK_MENU_BUTTON(hdr->menu_btn));
	}
	else
	{
		gtk_menu_button_popdown(GTK_MENU_BUTTON(hdr->menu_btn));
	}
}

void
celluloid_header_bar_set_floating(CelluloidHeaderBar *hdr, gboolean floating)
{
	if(floating)
	{
		gtk_widget_add_css_class
			(GTK_WIDGET(hdr->header_bar), "osd");
		gtk_widget_add_css_class
			(GTK_WIDGET(hdr->header_bar), "floating-header");

		gtk_widget_set_margin_start(GTK_WIDGET(hdr), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(hdr), 12);
		gtk_widget_set_margin_top(GTK_WIDGET(hdr), 12);
		gtk_widget_set_margin_bottom(GTK_WIDGET(hdr), 12);
	}
	else
	{
		gtk_widget_remove_css_class
			(GTK_WIDGET(hdr->header_bar), "osd");
		gtk_widget_remove_css_class
			(GTK_WIDGET(hdr->header_bar), "floating-header");

		gtk_widget_set_margin_start(GTK_WIDGET(hdr), 0);
		gtk_widget_set_margin_end(GTK_WIDGET(hdr), 0);
		gtk_widget_set_margin_top(GTK_WIDGET(hdr), 0);
		gtk_widget_set_margin_bottom(GTK_WIDGET(hdr), 0);
	}
}

void
celluloid_header_bar_update_track_list(	CelluloidHeaderBar *hdr,
					const GPtrArray *track_list )
{
	GMenu *menu = g_menu_new();

	celluloid_menu_build_menu_btn(menu);

	gtk_menu_button_set_menu_model
		(GTK_MENU_BUTTON(hdr->menu_btn), G_MENU_MODEL(menu));
	gtk_menu_button_set_create_popup_func
		(GTK_MENU_BUTTON(hdr->menu_btn), create_popup, hdr, NULL);
}

void
celluloid_header_bar_update_disc_list(	CelluloidHeaderBar *hdr,
					const GPtrArray *disc_list )
{
	GMenu *menu = g_menu_new();

	celluloid_menu_build_open_btn(menu, disc_list);

	gtk_menu_button_set_menu_model
		(GTK_MENU_BUTTON(hdr->open_btn), G_MENU_MODEL(menu));
	gtk_menu_button_set_create_popup_func
		(GTK_MENU_BUTTON(hdr->open_btn), create_popup, hdr, NULL);
}
