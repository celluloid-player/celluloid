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

#define GMPV_TYPE_PLUGINS_MANAGER (gmpv_plugins_manager_get_type ())

G_DECLARE_FINAL_TYPE(GmpvPluginsManager, gmpv_plugins_manager, GMPV, PLUGINS_MANAGER, GtkGrid)

GtkWidget *gmpv_plugins_manager_new(GtkWindow *parent);
void gmpv_plugins_manager_set_directory(	GmpvPluginsManager *pmgr,
						const gchar *path );

G_END_DECLS

#endif
