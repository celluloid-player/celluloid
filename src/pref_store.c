/*
 * Copyright (c) 2015-2016 gnome-mpv
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

#include "pref_store.h"

pref_store *pref_store_new()
{
	pref_store *pref = g_malloc(sizeof(pref_store));

	pref->dark_theme_enable = TRUE;
	pref->csd_enable = TRUE;
	pref->last_folder_enable = FALSE;
	pref->mpv_input_config_enable = FALSE;
	pref->mpv_config_enable = FALSE;
	pref->mpv_input_config_file = NULL;
	pref->mpv_config_file = NULL;
	pref->mpv_options = NULL;

	return pref;
}

void pref_store_free(pref_store *pref)
{
	if(pref)
	{
		g_free(pref->mpv_input_config_file);
		g_free(pref->mpv_config_file);
		g_free(pref->mpv_options);
		g_free(pref);
	}
}
