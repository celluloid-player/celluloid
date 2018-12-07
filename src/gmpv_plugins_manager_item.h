/*
 * Copyright (c) 2016 gnome-mpv
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

#ifndef PLUGINS_MANAGER_ITEM_H
#define PLUGINS_MANAGER_ITEM_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GMPV_TYPE_PLUGINS_MANAGER_ITEM (gmpv_plugins_manager_item_get_type ())

G_DECLARE_FINAL_TYPE(GmpvPluginsManagerItem, gmpv_plugins_manager_item, GMPV, PLUGINS_MANAGER_ITEM, GtkListBoxRow)

GtkWidget *gmpv_plugins_manager_item_new(	GtkWindow *parent,
						const gchar *title,
						const gchar *path );

G_END_DECLS

#endif
