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

#ifndef HEADER_BAR_H
#define HEADER_BAR_H

#include <gtk/gtk.h>

#define GMPV_TYPE_HEADER_BAR (gmpv_header_bar_get_type ())

G_DECLARE_FINAL_TYPE(GmpvHeaderBar, gmpv_header_bar, GMPV, HEADER_BAR, GtkHeaderBar)

GtkWidget *gmpv_header_bar_new(void);
gboolean gmpv_header_bar_get_open_button_popup_visible(GmpvHeaderBar *hdr);
gboolean gmpv_header_bar_get_menu_button_popup_visible(GmpvHeaderBar *hdr);
void gmpv_header_bar_set_fullscreen_state(	GmpvHeaderBar *hdr,
						gboolean fullscreen );
void gmpv_header_bar_update_track_list(	GmpvHeaderBar *hdr,
					const GSList *audio_list,
					const GSList *video_list,
					const GSList *sub_list );

#endif
