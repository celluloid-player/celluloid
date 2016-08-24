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

#ifndef GEOMETRY_H
#define GEOMETRY_H

#define GMPV_GEOMETRY_INIT {0, G_VALUE_INIT, G_VALUE_INIT, 0, 0}

typedef enum _GmpvGeometryFlag GmpvGeometryFlag;
typedef struct _GmpvGeometry GmpvGeometry;

enum _GmpvGeometryFlag
{
	GMPV_GEOMETRY_IGNORE_POS	= 1 << 0,
	GMPV_GEOMETRY_IGNORE_DIM	= 1 << 1,
	GMPV_GEOMETRY_FLIP_X		= 1 << 2,
	GMPV_GEOMETRY_FLIP_Y		= 1 << 3
};

struct _GmpvGeometry
{
	gint flags;
	GValue x;
	GValue y;
	gint64 width;
	gint64 height;
};

#endif
