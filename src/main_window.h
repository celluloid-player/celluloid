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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <gtk/gtk.h>

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
	GtkApplicationWindow parent_instance;
	gboolean fullscreen;
	gboolean playlist_visible;
	gint playlist_width;
	gint init_width;
	gint init_height;
	gint init_playlist_visible;
	guint timeout_tag;
	GtkSettings* settings;
	GtkWidget *header_bar;
	GtkWidget *open_hdr_btn;
	GtkWidget *fullscreen_hdr_btn;
	GtkWidget *menu_hdr_btn;
	GtkWidget *main_box;
	GtkWidget *vid_area_paned;
	GtkWidget *vid_area;
	GtkWidget *fs_control;
	GtkWidget *control_box;
	GtkWidget *playlist;
};

struct _MainWindowClass
{
	GtkApplicationWindowClass parent_class;
};

typedef struct _MainWindow MainWindow;
typedef struct _MainWindowClass MainWindowClass;


GtkWidget *main_window_new(GtkApplication *app);
GType main_window_get_type(void);
void main_window_toggle_fullscreen(MainWindow *wnd);
void main_window_reset(MainWindow *wnd);
void main_window_save_state(MainWindow *wnd);
void main_window_load_state(MainWindow *wnd);
gint main_window_get_width_margin(MainWindow *wnd);
gint main_window_get_height_margin(MainWindow *wnd);
void main_window_enable_csd(MainWindow *wnd);
gboolean main_window_get_csd_enabled(MainWindow *wnd);
void main_window_set_playlist_visible(MainWindow *wnd, gboolean visible);
gboolean main_window_get_playlist_visible(MainWindow *wnd);

G_END_DECLS

#endif
