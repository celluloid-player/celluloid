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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <gtk/gtk.h>

#include "celluloid-main-window.h"
#include "celluloid-main-window.h"
#include "celluloid-mpv.h"
#include "celluloid-mpv.h"

G_BEGIN_DECLS

#define CELLULOID_TYPE_APPLICATION (celluloid_application_get_type())

G_DECLARE_FINAL_TYPE(CelluloidApplication, celluloid_application, CELLULOID, APPLICATION, GtkApplication)

CelluloidApplication *
celluloid_application_new(gchar *id, GApplicationFlags flags);

void
celluloid_application_quit(CelluloidApplication *app);

const gchar *
celluloid_application_get_mpv_options(CelluloidApplication *app);

G_END_DECLS

#endif
