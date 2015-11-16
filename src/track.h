/*
 * Copyright (c) 2015 gnome-mpv
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

#ifndef TRACK_H
#define TRACK_H

#include <glib.h>

typedef enum TrackType TrackType;
typedef struct Track Track;

enum TrackType
{
	TRACK_TYPE_INVALID,
	TRACK_TYPE_AUDIO,
	TRACK_TYPE_VIDEO,
	TRACK_TYPE_SUBTITLE
};

struct Track
{
	TrackType type;
	gint64 id;
	gchar *title;
	gchar *lang;
};

Track *track_new(void);
void track_free(Track *entry);

#endif
