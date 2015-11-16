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

#include "track.h"

Track *track_new(void)
{
	Track *entry = g_malloc(sizeof(Track));

	entry->type = TRACK_TYPE_INVALID;
	entry->title = NULL;
	entry->lang = NULL;
	entry->id = 0;

	return entry;
}

void track_free(Track *entry)
{
	if(entry)
	{
		g_free(entry->title);
		g_free(entry->lang);
	}
}
