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

#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <gtk/gtk.h>

#include "def.h"

G_BEGIN_DECLS

#define MAIN_WINDOW_TYPE (main_window_get_type ())

#define	MAIN_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), MAIN_WINDOW_TYPE, MainWindow))

#define	MAIN_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), MAIN_WINDOW_TYPE, MainWindowClass))

#define	IS_MAIN_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), MAIN_WINDOW_TYPE))

#define	IS_MAIN_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), MAIN_WINDOW_TYPE))

struct _MainWindow
{
	GtkWindow window;
	gboolean fullscreen;
	gboolean playlist_visible;
	gint playlist_width;
	gint seek_bar_length;
	GtkSettings* settings;
	GtkAccelGroup *accel_group;
	GtkWidget *main_box;
	GtkWidget *vid_area_paned;
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
	GtkWidget *playlist_menu_item;
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
	GtkWidget *playlist;
};

struct _MainWindowClass
{
	GtkWindowClass parent_class;
};

typedef struct _MainWindow MainWindow;
typedef struct _MainWindowClass MainWindowClass;

GtkWidget *main_window_new(void);
GType main_window_get_type(void);
void main_window_toggle_fullscreen(MainWindow *wnd);
void main_window_reset_control(MainWindow *wnd);
void main_window_set_seek_bar_length(MainWindow *wnd, gint length);
void main_window_set_control_enabled(MainWindow *wnd, gboolean enabled);
void main_window_set_playlist_visible(MainWindow *wnd, gboolean visible);
gboolean main_window_get_playlist_visible(MainWindow *wnd);
void main_window_show_chapter_control(MainWindow *wnd);
void main_window_hide_chapter_control(MainWindow *wnd);

G_END_DECLS

#endif
