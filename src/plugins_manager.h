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

#ifndef PLUGINS_MANAGER_H
#define PLUGINS_MANAGER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PLUGINS_MANAGER_TYPE (plugins_manager_get_type ())

#define	PLUGINS_MANAGER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), PLUGINS_MANAGER_TYPE, PluginsManager))

#define	PLUGINS_MANAGER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), PLUGINS_MANAGER_TYPE, PluginsManagerClass))

#define	IS_PLUGINS_MANAGER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), PLUGINS_MANAGER_TYPE))

#define	IS_PLUGINS_MANAGER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), PLUGINS_MANAGER_TYPE))

typedef struct _PluginsManager PluginsManager;
typedef struct _PluginsManagerClass PluginsManagerClass;

GType plugins_manager_get_type(void);
GtkWidget *plugins_manager_new(GtkWindow *parent);
void plugins_manager_set_directory(PluginsManager *pmgr, const gchar *path);

G_END_DECLS

#endif
