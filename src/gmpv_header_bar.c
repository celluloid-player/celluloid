/*
 * Copyright (c) 2016-2017 gnome-mpv
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

#include "gmpv_header_bar.h"
#include "gmpv_menu.h"

struct _GmpvHeaderBar
{
	GtkHeaderBar parent_instance;
	GtkWidget *open_btn;
	GtkWidget *fullscreen_btn;
	GtkWidget *menu_btn;
};

struct _GmpvHeaderBarClass
{
	GtkHeaderBarClass parent_class;
};

G_DEFINE_TYPE(GmpvHeaderBar, gmpv_header_bar, GTK_TYPE_HEADER_BAR)

static void gmpv_header_bar_class_init(GmpvHeaderBarClass *klass)
{
}

static void gmpv_header_bar_init(GmpvHeaderBar *hdr)
{
	GtkHeaderBar *ghdr;
	GMenu *open_btn_menu;
	GMenu *menu_btn_menu;
	GtkWidget *open_icon;
	GtkWidget *fullscreen_icon;
	GtkWidget *menu_icon;

	ghdr = GTK_HEADER_BAR(hdr);
	open_btn_menu = g_menu_new();
	menu_btn_menu = g_menu_new();

	open_icon =		gtk_image_new_from_icon_name
				("list-add-symbolic", GTK_ICON_SIZE_MENU);
	fullscreen_icon =	gtk_image_new_from_icon_name
				("view-fullscreen-symbolic", GTK_ICON_SIZE_MENU);
	menu_icon =		gtk_image_new_from_icon_name
				("open-menu-symbolic", GTK_ICON_SIZE_MENU);

	hdr->open_btn = gtk_menu_button_new();
	hdr->fullscreen_btn = gtk_button_new();
	hdr->menu_btn = gtk_menu_button_new();

	gmpv_menu_build_open_btn(open_btn_menu);
	gmpv_menu_build_menu_btn(menu_btn_menu, NULL);

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
	gtk_header_bar_set_show_close_button(ghdr, TRUE);
}

GtkWidget *gmpv_header_bar_new()
{
	return GTK_WIDGET(g_object_new(gmpv_header_bar_get_type(), NULL));
}

gboolean gmpv_header_bar_get_open_button_popup_visible(GmpvHeaderBar *hdr)
{
	GtkMenuButton *btn = GTK_MENU_BUTTON(hdr->open_btn);
	GtkWidget *popover = GTK_WIDGET(gtk_menu_button_get_popover(btn));

	return gtk_widget_is_visible(popover);
}

gboolean gmpv_header_bar_get_menu_button_popup_visible(GmpvHeaderBar *hdr)
{
	GtkMenuButton *btn = GTK_MENU_BUTTON(hdr->menu_btn);
	GtkWidget *popover = GTK_WIDGET(gtk_menu_button_get_popover(btn));

	return gtk_widget_is_visible(popover);
}

void gmpv_header_bar_set_fullscreen_state(	GmpvHeaderBar *hdr,
						gboolean fullscreen )
{
	GtkWidget *image = gtk_button_get_image(GTK_BUTTON(hdr->fullscreen_btn));

	gtk_image_set_from_icon_name(	GTK_IMAGE(image),
					fullscreen?
					"view-restore-symbolic":
					"view-fullscreen-symbolic",
					GTK_ICON_SIZE_MENU );

	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hdr), !fullscreen);
}

void gmpv_header_bar_update_track_list(	GmpvHeaderBar *hdr,
					const GPtrArray *track_list )
{
	GMenu *menu = g_menu_new();

	gmpv_menu_build_menu_btn(menu, track_list);
	gtk_menu_button_set_menu_model
		(GTK_MENU_BUTTON(hdr->menu_btn), G_MENU_MODEL(menu));
}
