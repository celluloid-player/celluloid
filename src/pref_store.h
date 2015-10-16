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

#ifndef PREF_H
#define PREF_H

#include <glib.h>

struct pref_store
{
	gboolean dark_theme_enable;
	gboolean csd_enable;
	gboolean last_folder_enable;
	gboolean mpv_msg_redir_enable;
	gboolean mpv_input_config_enable;
	gboolean mpv_config_enable;
	gchar *mpv_input_config_file;
	gchar *mpv_config_file;
	gchar *mpv_options;
};

typedef struct pref_store pref_store;

pref_store *pref_store_new(void);
void pref_store_free(pref_store *pref);

#endif
