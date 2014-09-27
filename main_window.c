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

#include "def.h"
#include "playlist_widget.h"
#include "main_window.h"
#include "main_menu_bar.h"
#include "control_box.h"

static gboolean focus_in_handler(	GtkWidget *widget,
					GtkDirectionType direction,
					gpointer data );

static gboolean focus_out_handler(	GtkWidget *widget,
					GtkDirectionType direction,
					gpointer data );

static gboolean fs_control_enter_handler(	GtkWidget *widget,
						GdkEvent *event,
						gpointer data );

static gboolean fs_control_leave_handler(	GtkWidget *widget,
						GdkEvent *event,
						gpointer data );

static void main_window_init(MainWindow *wnd);

static gboolean focus_in_handler(	GtkWidget *widget,
					GtkDirectionType direction,
					gpointer data )
{
	MainWindow *wnd = data;

	if(wnd->fullscreen)
	{
		gint width;
		gint height;

		gtk_window_get_size(	GTK_WINDOW(wnd->fs_control),
					&width,
					&height );

		gtk_window_move(	GTK_WINDOW(wnd->fs_control),
					(gdk_screen_width()/2)-(width/2),
					gdk_screen_height()-height );

		gtk_widget_show_all(wnd->fs_control);
	}

	return TRUE;
}

static gboolean focus_out_handler(	GtkWidget *widget,
					GtkDirectionType direction,
					gpointer data )
{
	MainWindow *wnd = data;

	if(wnd->fullscreen)
	{
		gtk_widget_hide(wnd->fs_control);
	}

	return TRUE;
}

static gboolean fs_control_enter_handler(	GtkWidget *widget,
						GdkEvent *event,
						gpointer data )
{
	gtk_widget_set_opacity(data, 1);

	return FALSE;
}

static gboolean fs_control_leave_handler(	GtkWidget *widget,
						GdkEvent *event,
						gpointer data )
{
	gtk_widget_set_opacity(data, 0);

	return FALSE;
}

static void main_window_init(MainWindow *wnd)
{
	GdkRGBA vid_area_bg_color;
	MainMenuBar *menu;
	GtkBin *fullscreen_menu_item_bin;
	GtkAccelLabel *fullscreen_accel_label;

	wnd->fullscreen = FALSE;
	wnd->playlist_visible = FALSE;
	wnd->playlist_width = PLAYLIST_DEFAULT_WIDTH;
	wnd->settings = gtk_settings_get_default();
	wnd->accel_group = gtk_accel_group_new();
	wnd->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	wnd->vid_area_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	wnd->vid_area = gtk_drawing_area_new();
	wnd->fs_control = gtk_window_new(GTK_WINDOW_POPUP);
	wnd->control_box = control_box_new();
	wnd->menu = main_menu_bar_new();
	wnd->playlist = playlist_widget_new();

	menu = MAIN_MENU(wnd->menu);

	gdk_rgba_parse(&vid_area_bg_color, VID_AREA_BG_COLOR);

	gtk_widget_add_events(	wnd->fs_control,
				GDK_ENTER_NOTIFY_MASK
				|GDK_LEAVE_NOTIFY_MASK );

	g_object_set(	wnd->settings,
			"gtk-application-prefer-dark-theme",
			TRUE,
			NULL );

	gtk_window_set_title(GTK_WINDOW(wnd), g_get_application_name());

	gtk_paned_set_position(	GTK_PANED(wnd->vid_area_paned),
				MAIN_WINDOW_DEFAULT_WIDTH
				-PLAYLIST_DEFAULT_WIDTH );

	gtk_window_set_default_size(	GTK_WINDOW(wnd),
					MAIN_WINDOW_DEFAULT_WIDTH,
					MAIN_WINDOW_DEFAULT_HEIGHT );

	gtk_widget_override_background_color(	wnd->vid_area,
						GTK_STATE_NORMAL,
						&vid_area_bg_color);

	gtk_window_add_accel_group(	GTK_WINDOW(wnd),
					wnd->accel_group );

	gtk_widget_add_accelerator(	menu->open_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_o,
					GDK_CONTROL_MASK,
					GTK_ACCEL_VISIBLE );

	gtk_widget_add_accelerator(	menu->open_loc_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_l,
					GDK_CONTROL_MASK,
					GTK_ACCEL_VISIBLE );

	gtk_widget_add_accelerator(	menu->quit_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_q,
					GDK_CONTROL_MASK,
					GTK_ACCEL_VISIBLE );

	gtk_widget_add_accelerator(	menu->pref_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_p,
					GDK_CONTROL_MASK,
					GTK_ACCEL_VISIBLE );

	gtk_widget_add_accelerator(	menu->playlist_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_F9,
					(GdkModifierType)0,
					GTK_ACCEL_VISIBLE );

	gtk_widget_add_accelerator(	menu->normal_size_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_1,
					GDK_CONTROL_MASK,
					GTK_ACCEL_VISIBLE );

	gtk_widget_add_accelerator(	menu->double_size_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_2,
					GDK_CONTROL_MASK,
					GTK_ACCEL_VISIBLE );

	gtk_widget_add_accelerator(	menu->half_size_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_3,
					GDK_CONTROL_MASK,
					GTK_ACCEL_VISIBLE );

	fullscreen_menu_item_bin = GTK_BIN(menu->fullscreen_menu_item);

	fullscreen_accel_label
		= GTK_ACCEL_LABEL(gtk_bin_get_child(fullscreen_menu_item_bin));

	gtk_accel_label_set_accel(	fullscreen_accel_label,
					GDK_KEY_F11,
					(GdkModifierType)0 );

	gtk_container_add
		(GTK_CONTAINER(wnd->main_box), wnd->menu);

	gtk_box_pack_start
		(GTK_BOX(wnd->main_box), wnd->vid_area_paned, TRUE, TRUE, 0);

	gtk_paned_pack1
		(GTK_PANED(wnd->vid_area_paned), wnd->vid_area, TRUE, TRUE);

	gtk_paned_pack2
		(GTK_PANED(wnd->vid_area_paned), wnd->playlist, FALSE, TRUE);

	gtk_container_add
		(GTK_CONTAINER(wnd->main_box), wnd->control_box);

	gtk_container_add
		(GTK_CONTAINER(wnd), wnd->main_box);

	g_signal_connect(	wnd,
				"focus-in-event",
				G_CALLBACK(focus_in_handler),
				wnd );

	g_signal_connect(	wnd,
				"focus-out-event",
				G_CALLBACK(focus_out_handler),
				wnd );

	g_signal_connect(	wnd->fs_control,
				"enter-notify-event",
				G_CALLBACK(fs_control_enter_handler),
				wnd->fs_control );

	g_signal_connect(	wnd->fs_control,
				"leave-notify-event",
				G_CALLBACK(fs_control_leave_handler),
				wnd->fs_control );

	gtk_widget_show_all(GTK_WIDGET(wnd));
	gtk_widget_hide(wnd->playlist);
	control_box_set_chapter_enabled(CONTROL_BOX(wnd->control_box), FALSE);
}

GtkWidget *main_window_new()
{
	return GTK_WIDGET(g_object_new(main_window_get_type(), NULL));
}

GType main_window_get_type()
{
	static GType wnd_type = 0;

	if(wnd_type == 0)
	{
		const GTypeInfo wnd_info
			= {	sizeof(MainWindowClass),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				sizeof(MainWindow),
				0,
				(GInstanceInitFunc)main_window_init };

		wnd_type = g_type_register_static(	GTK_TYPE_WINDOW,
							"MainWindow",
							&wnd_info,
							0 );
	}

	return wnd_type;
}

void main_window_toggle_fullscreen(MainWindow *wnd)
{
	ControlBox *control_box = CONTROL_BOX(wnd->control_box);
	GtkContainer* main_box = GTK_CONTAINER(wnd->main_box);
	GtkContainer* fs_control = GTK_CONTAINER(wnd->fs_control);

	if(wnd->fullscreen)
	{
		g_object_ref(wnd->control_box);
		gtk_container_remove(fs_control, wnd->control_box);
		gtk_container_add(main_box, wnd->control_box);
		g_object_unref(wnd->control_box);

		control_box_set_fullscreen_state(control_box, FALSE);
		gtk_window_unfullscreen(GTK_WINDOW(wnd));
		gtk_widget_hide(wnd->fs_control);
		gtk_widget_show(wnd->menu);

		if(wnd->playlist_visible)
		{
			gtk_widget_show(wnd->playlist);
		}

		wnd->fullscreen = FALSE;
	}
	else
	{
		GdkScreen *screen;
		gint width;
		gint height;

		screen = gtk_window_get_screen(GTK_WINDOW(wnd));
		width = gdk_screen_width()/2;

		g_object_ref(wnd->control_box);
		gtk_container_remove(main_box, wnd->control_box);
		gtk_container_add(fs_control, wnd->control_box);
		g_object_unref(wnd->control_box);

		control_box_set_fullscreen_state(control_box, TRUE);
		gtk_window_fullscreen(GTK_WINDOW(wnd));
		gtk_window_set_screen(GTK_WINDOW(wnd->fs_control), screen);
		gtk_widget_hide(wnd->menu);
		gtk_widget_show(wnd->fs_control);
		gtk_widget_set_opacity(wnd->fs_control, 0);

		if(wnd->playlist_visible)
		{
			gtk_widget_hide(wnd->playlist);
		}

		gtk_window_get_size(	GTK_WINDOW(wnd->fs_control),
					NULL,
					&height );

		gtk_window_resize(	GTK_WINDOW(wnd->fs_control),
					width,
					height );

		gtk_window_move(	GTK_WINDOW(wnd->fs_control),
					(gdk_screen_width()/2)-(width/2),
					gdk_screen_height()-height );

		gtk_window_set_transient_for(	GTK_WINDOW(wnd->fs_control),
						GTK_WINDOW(wnd) );

		wnd->fullscreen = TRUE;
	}
}

void main_window_reset(MainWindow *wnd)
{
	gtk_window_set_title(GTK_WINDOW(wnd), g_get_application_name());
	control_box_reset_control(CONTROL_BOX(wnd->control_box));
}

void main_window_set_playlist_visible(MainWindow *wnd, gboolean visible)
{
	gint handle_pos;
	gint width;
	gint height;

	handle_pos = gtk_paned_get_position(GTK_PANED(wnd->vid_area_paned));

	gtk_window_get_size(GTK_WINDOW(wnd), &width, &height);

	if(!visible && wnd->playlist_visible)
	{
		wnd->playlist_width = width-handle_pos;
	}

	wnd->playlist_visible = visible;

	gtk_widget_set_visible(wnd->playlist, visible);

	gtk_window_resize(	GTK_WINDOW(wnd),
				visible?width+wnd->playlist_width:handle_pos,
				height );
}

gboolean main_window_get_playlist_visible(MainWindow *wnd)
{
	return gtk_widget_get_visible(GTK_WIDGET(wnd->playlist));
}
