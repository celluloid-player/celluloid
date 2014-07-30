/*
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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <gtk/gtk.h>

typedef struct main_window_t
{
	gint fullscreen;
	gint seek_bar_length;
	GtkSettings* settings;
	GtkAccelGroup *accel_group;
	GtkWidget *window;
	GtkWidget *main_box;
	GtkWidget *vid_area;
	GtkWidget *control_box;
	GtkWidget *fs_control;
	GtkWidget *menu_bar;
	GtkWidget *file_menu;
	GtkWidget *file_menu_item;
	GtkWidget *open_menu_item;
	GtkWidget *open_loc_menu_item;
	GtkWidget *quit_menu_item;
	GtkWidget *edit_menu;
	GtkWidget *edit_menu_item;
	GtkWidget *pref_menu_item;
	GtkWidget *view_menu;
	GtkWidget *view_menu_item;
	GtkWidget *fullscreen_menu_item;
	GtkWidget *normal_size_menu_item;
	GtkWidget *double_size_menu_item;
	GtkWidget *half_size_menu_item;
	GtkWidget *help_menu;
	GtkWidget *help_menu_item;
	GtkWidget *about_menu_item;
	GtkWidget *play_button;
	GtkWidget *stop_button;
	GtkWidget *forward_button;
	GtkWidget *rewind_button;
	GtkWidget *next_button;
	GtkWidget *previous_button;
	GtkWidget *volume_button;
	GtkWidget *fullscreen_button;
	GtkWidget *seek_bar;
}
main_window_t;

void main_window_init(main_window_t *wnd);
void main_window_toggle_fullscreen(main_window_t *wnd);
void main_window_reset_control(main_window_t *wnd);
void main_window_set_seek_bar_length(main_window_t *wnd, gint length);
void main_window_set_control_enabled(main_window_t *wnd, gboolean enabled);
void main_window_show_chapter_control(main_window_t *wnd);
void main_window_hide_chapter_control(main_window_t *wnd);

#endif
