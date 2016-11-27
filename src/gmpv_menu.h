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

#ifndef MENU_H
#define MENU_H

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

void gmpv_menu_build_full(	GMenu *gmpv_menu,
			const GSList *audio_list,
			const GSList *video_list,
			const GSList *sub_list );
void gmpv_menu_build_menu_btn(	GMenu *gmpv_menu,
				const GSList *audio_list,
				const GSList *video_list,
				const GSList *sub_list );
void gmpv_menu_build_open_btn(GMenu *gmpv_menu);
void gmpv_menu_build_app_menu(GMenu *gmpv_menu);

G_END_DECLS

#endif
