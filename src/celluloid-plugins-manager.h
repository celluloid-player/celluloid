/*
 * Copyright (c) 2016-2019 gnome-mpv
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

#ifndef PLUGINS_MANAGER_H
#define PLUGINS_MANAGER_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <adwaita.h>

G_BEGIN_DECLS

#define CELLULOID_TYPE_PLUGINS_MANAGER (celluloid_plugins_manager_get_type ())

G_DECLARE_FINAL_TYPE(CelluloidPluginsManager, celluloid_plugins_manager, CELLULOID, PLUGINS_MANAGER, GtkGrid)

AdwPreferencesPage *
celluloid_plugins_manager_new(GtkWindow *parent);

void
celluloid_plugins_manager_set_directory(	CelluloidPluginsManager *pmgr,
						const gchar *path );

G_END_DECLS

#endif
