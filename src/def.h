/*
 * Copyright (c) 2014-2015 gnome-mpv
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

#define APP_ID "org.gnome-mpv"
#define ICON_NAME "gnome-mpv"
#define CONFIG_FILE "gnome-mpv.conf"
#define PLAYLIST_DEFAULT_WIDTH 200
#define MAIN_WINDOW_DEFAULT_WIDTH 400
#define MAIN_WINDOW_DEFAULT_HEIGHT 400
#define VID_AREA_BG_COLOR "#000000"
#define SEEK_BAR_UPDATE_INTERVAL 250
#define CURSOR_HIDE_DELAY 1
#define KEYSTRING_MAX_LEN 8

#define DEFAULT_KEYBINDS	{	"SPACE cycle pause",\
					"p cycle pause",\
					"v osd-msg cycle sub-visibility",\
					"s osd-msg screenshot",\
					"S osd-msg screenshot video",\
					"j osd-msg cycle sub",\
					"J osd-msg cycle sub down",\
					"@ osd-msg cycle chapter",\
					"! osd-msg cycle chapter down",\
					"< playlist_prev",\
					"> playlist_next",\
					"U stop",\
					"RIGHT seek 10",\
					"LEFT seek -10",\
					"UP seek 60",\
					"DOWN seek -60",\
					"Q quit_watch_later",\
					NULL }

#define KEYSTRING_MAP	{	"<", "less",\
				">", "greater",\
				"PGUP", "Page_Up",\
				"PGDWN", "Page_Down",\
				"BS", "BackSpace",\
				"SPACE", "space",\
				".", "period",\
				",", "comma",\
				"`", "grave",\
				"~", "asciitilde",\
				"!", "exclam",\
				"@", "at",\
				"SHARP", "numbersign",\
				"$", "dollar",\
				"%", "percent",\
				"^", "caret",\
				"&", "ampersand",\
				"*", "asterisk",\
				"-", "minus",\
				"_", "underscore",\
				"=", "equal",\
				"+", "plus",\
				";", "semicolon",\
				":", "colon",\
				"'", "apostrophe",\
				"\"", "quotedbl",\
				"/", "slash",\
				"\\", "backslash",\
				"(", "parenleft",\
				")", "parenright",\
				"[", "bracketleft",\
				"]", "bracketright",\
				"RIGHT", "Right",\
				"LEFT", "Left",\
				"UP", "Up",\
				"DOWN", "Down",\
				NULL }

#endif
