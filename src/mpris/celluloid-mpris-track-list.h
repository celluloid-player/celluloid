/*
 * Copyright (c) 2017-2019 gnome-mpv
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


#ifndef MPRIS_TRACK_LIST_H
#define MPRIS_TRACK_LIST_H

#include "celluloid-mpris-module.h"
#include "celluloid-controller.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define CELLULOID_TYPE_MPRIS_TRACK_LIST (celluloid_mpris_track_list_get_type())
G_DECLARE_FINAL_TYPE(CelluloidMprisTrackList, celluloid_mpris_track_list, CELLULOID, MPRIS_TRACK_LIST, CelluloidMprisModule)

CelluloidMprisModule *
celluloid_mpris_track_list_new(	CelluloidController *controller,
				GDBusConnection *conn );

G_END_DECLS

#endif
