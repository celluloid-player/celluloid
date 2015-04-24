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

#ifndef KEYBIND_H
#define KEYBIND_H

#include <gdk/gdkx.h>

#include "common.h"

struct keybind
{
	gboolean mouse;
	gint modifier;
	gint keyval;
	gchar **command;
};

typedef struct keybind keybind;

keybind *keybind_parse_config_line(const gchar *line, gboolean *propexp);
GSList *keybind_parse_config(const gchar *config_path, gboolean *propexp);
gchar **keybind_get_command(	gmpv_handle *ctx,
				gboolean mouse,
				gint modifier,
				gint keyval );
GSList *keybind_parse_config_with_defaults(	const gchar *config_path,
						gboolean *propexp );

#endif
