/*
 * Copyright (c) 2016 gnome-mpv
 *
 * This file is part of GNOME MPV.
 *
 * GNOME MPV is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GNOME MPV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNOME MPV.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PLUGINS_MANAGER_ITEM_H
#define PLUGINS_MANAGER_ITEM_H

#include <gtk/gtk.h>

#define PLUGINS_MANAGER_ITEM_TYPE (plugins_manager_item_get_type ())

#define	PLUGINS_MANAGER_ITEM(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), PLUGINS_MANAGER_ITEM_TYPE, PluginsManagerItem))

#define	PLUGINS_MANAGER_ITEM_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), PLUGINS_MANAGER_ITEM_TYPE, PluginsManagerItemClass))

#define	IS_PLUGINS_MANAGER_ITEM(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), PLUGINS_MANAGER_ITEM_TYPE))

#define	IS_PLUGINS_MANAGER_ITEM_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), PLUGINS_MANAGER_ITEM_TYPE))

typedef struct _PluginsManagerItem PluginsManagerItem;
typedef struct _PluginsManagerItemClass PluginsManagerItemClass;

GType plugins_manager_item_get_type(void);
GtkWidget *plugins_manager_item_new(	GtkWindow *parent,
					const gchar *title,
					const gchar *path );

#endif
