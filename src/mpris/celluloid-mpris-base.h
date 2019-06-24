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

#ifndef MPRIS_BASE_H
#define MPRIS_BASE_H

#include "celluloid-mpris.h"
#include "celluloid-mpris-module.h"
#include "celluloid-controller.h"

G_BEGIN_DECLS

#define CELLULOID_TYPE_MPRIS_BASE (celluloid_mpris_base_get_type())
G_DECLARE_FINAL_TYPE(CelluloidMprisBase, celluloid_mpris_base, CELLULOID, MPRIS_BASE, CelluloidMprisModule)

CelluloidMprisModule *
celluloid_mpris_base_new(	CelluloidController *controller,
				GDBusConnection *conn );

G_END_DECLS

#endif
