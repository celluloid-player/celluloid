/*
 * Copyright (c) 2014 gnome-mpv
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

#ifndef DEF_H
#define DEF_H

#define ICON_NAME "gnome-mpv"
#define CONFIG_FILE "gnome-mpv.conf"
#define PLAYLIST_DEFAULT_WIDTH 200
#define MAIN_WINDOW_DEFAULT_WIDTH 400
#define MAIN_WINDOW_DEFAULT_HEIGHT 300
#define VID_AREA_BG_COLOR "#000000"
#define SEEK_BAR_UPDATE_INTERVAL 250
#define CURSOR_HIDE_DELAY 1

#define DEFAULT_KEYBINDS	{	"v osd-msg cycle sub-visibility",\
					"s osd-msg screenshot",\
					"S osd-msg screenshot video",\
					"j osd-msg cycle sub",\
					"j osd-msg cycle sub down",\
					"Q quit_watch_later",\
					NULL }

#endif
