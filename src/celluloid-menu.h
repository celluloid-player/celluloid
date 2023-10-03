/*
 * Copyright (c) 2015-2019 gnome-mpv
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

#ifndef MENU_H
#define MENU_H

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define CELLULOID_MENU_ITEM(title, action) {title, action, NULL}
#define CELLULOID_MENU_SUBMENU(title, submenu) {title, NULL, submenu}
#define CELLULOID_MENU_SEPARATOR {NULL, "", NULL}
#define CELLULOID_MENU_END {NULL, NULL, NULL}

struct CelluloidMenuEntry
{
	gchar *title;
	gchar *action;
	GMenu *submenu;
};

typedef struct CelluloidMenuEntry CelluloidMenuEntry;

void
celluloid_menu_build_full(	GMenu *celluloid_menu,
				const GPtrArray *track_list,
				const GPtrArray *disc_list );

void
celluloid_menu_build_menu_btn(GMenu *celluloid_menu);

void
celluloid_menu_build_track_menu_btn(GMenu *celluloid_menu, const GPtrArray *track_list);

void
celluloid_menu_build_open_btn(GMenu *celluloid_menu, const GPtrArray *disc_list);

void
celluloid_menu_build_app_menu(GMenu *celluloid_menu);

void
celluloid_menu_build_menu(	GMenu *menu,
				const CelluloidMenuEntry *entries,
				gboolean flat );

G_END_DECLS

#endif
