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

#include <gio/gsettingsbackend.h>
#include <glib/gi18n.h>

#include "def.h"
#include "common.h"
#include "playlist_widget.h"
#include "main_window.h"
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
static gboolean motion_notify_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gboolean configure_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gboolean hide_cursor(gpointer data);
static gboolean finalize_load_state(gpointer data);
static GMenu *menu_btn_build_menu();
static GMenu *open_btn_build_menu();
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

static gboolean motion_notify_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	MainWindow *wnd;
	GdkCursor *cursor;

	wnd = data;
	cursor = gdk_cursor_new_for_display(	gdk_display_get_default(),
						GDK_ARROW );

	gdk_window_set_cursor
		(gtk_widget_get_window(GTK_WIDGET(wnd->vid_area)), cursor);

	if(wnd->timeout_tag > 0)
	{
		g_source_remove(wnd->timeout_tag);
	}

	wnd->timeout_tag
		= g_timeout_add_seconds(CURSOR_HIDE_DELAY, hide_cursor, data);

	return FALSE;
}

static gboolean configure_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	MainWindow *wnd = data;
	GdkEventConfigure *conf_event = (GdkEventConfigure *)event;
	guint signal_id = g_signal_lookup("configure-event", MAIN_WINDOW_TYPE);

	if(wnd->init_width == conf_event->width)
	{
		g_signal_handlers_disconnect_matched(	wnd,
							G_SIGNAL_MATCH_ID
							|G_SIGNAL_MATCH_DATA,
							signal_id,
							0,
							0,
							NULL,
							data);

		g_idle_add((GSourceFunc)finalize_load_state, data);
	}

	return FALSE;
}

static gboolean hide_cursor(gpointer data)
{
	MainWindow *wnd;
	GdkCursor *cursor;

	wnd = data;
	cursor = gdk_cursor_new_for_display(	gdk_display_get_default(),
						GDK_BLANK_CURSOR );

	gdk_window_set_cursor
		(gtk_widget_get_window(GTK_WIDGET(wnd->vid_area)), cursor);

	wnd->timeout_tag = 0;

	return FALSE;
}

static gboolean finalize_load_state(gpointer data)
{
	MainWindow *wnd = data;

	gtk_paned_set_position(	GTK_PANED(wnd->vid_area_paned),
				wnd->init_width-wnd->playlist_width );

	if(wnd->init_playlist_visible != main_window_get_playlist_visible(wnd))
	{
		wnd->playlist_visible = wnd->init_playlist_visible;

		gtk_widget_set_visible(	wnd->playlist,
					wnd->init_playlist_visible);
	}

	return FALSE;
}

static GMenu *menu_btn_build_menu()
{
	GMenu *menu;
	GMenu *playlist_submenu;
	GMenu *sub_submenu;
	GMenu *view_submenu;
	GMenuItem *playlist_section;
	GMenuItem *sub_section;
	GMenuItem *view_section;
	GMenuItem *load_sub_menu_item;
	GMenuItem *playlist_toggle_menu_item;
	GMenuItem *playlist_save_menu_item;
	GMenuItem *fullscreen_menu_item;
	GMenuItem *normal_size_menu_item;
	GMenuItem *double_size_menu_item;
	GMenuItem *half_size_menu_item;

	menu = g_menu_new();
	playlist_submenu = g_menu_new();
	sub_submenu = g_menu_new();
	view_submenu = g_menu_new();

	playlist_section
		= g_menu_item_new_section(NULL, G_MENU_MODEL(playlist_submenu));

	sub_section
		= g_menu_item_new_section(NULL, G_MENU_MODEL(sub_submenu));

	view_section
		= g_menu_item_new_section(NULL, G_MENU_MODEL(view_submenu));

	playlist_toggle_menu_item
		= g_menu_item_new(_("_Toggle Playlist"), "app.playlist_toggle");

	playlist_save_menu_item
		= g_menu_item_new(_("_Save Playlist"), "app.playlist_save");

	load_sub_menu_item
		= g_menu_item_new(_("_Load Subtitle"), "app.loadsub");

	fullscreen_menu_item
		= g_menu_item_new(_("_Fullscreen"), "app.fullscreen");

	normal_size_menu_item
		= g_menu_item_new(_("_Normal Size"), "app.normalsize");

	double_size_menu_item
		= g_menu_item_new(_("_Double Size"), "app.doublesize");

	half_size_menu_item
		= g_menu_item_new(_("_Half Size"), "app.halfsize");

	g_menu_append_item(playlist_submenu, playlist_toggle_menu_item);
	g_menu_append_item(playlist_submenu, playlist_save_menu_item);
	g_menu_append_item(sub_submenu, load_sub_menu_item);
	g_menu_append_item(view_submenu, fullscreen_menu_item);
	g_menu_append_item(view_submenu, normal_size_menu_item);
	g_menu_append_item(view_submenu, double_size_menu_item);
	g_menu_append_item(view_submenu, half_size_menu_item);

	g_menu_append_item(menu, playlist_section);
	g_menu_append_item(menu, sub_section);
	g_menu_append_item(menu, view_section);

	return menu;
}

static GMenu *open_btn_build_menu()
{
	GMenu *menu;
	GMenuItem *open_menu_item;
	GMenuItem *open_loc_menu_item;

	menu = g_menu_new();

	open_menu_item = g_menu_item_new(_("_Open"), "app.open");

	open_loc_menu_item
		= g_menu_item_new(_("Open _Location"), "app.openloc");

	g_menu_append_item(menu, open_menu_item);
	g_menu_append_item(menu, open_loc_menu_item);

	return menu;
}

void main_window_save_state(MainWindow *wnd)
{
	GSettingsBackend *config_backend;
	GSettings *config;
	gint width;
	gint height;
	gint handle_pos;

	config_backend = g_keyfile_settings_backend_new
				(	get_config_file_path(),
					CONFIG_ROOT_PATH,
					CONFIG_ROOT_GROUP );

	config = g_settings_new_with_backend(CONFIG_WIN_STATE, config_backend);
	handle_pos = gtk_paned_get_position(GTK_PANED(wnd->vid_area_paned));

	gtk_window_get_size(GTK_WINDOW(wnd), &width, &height);

	g_settings_set_int(config, "width", width);
	g_settings_set_int(config, "height", height);

	if(main_window_get_playlist_visible(wnd))
	{
		g_settings_set_int(	config,
					"playlist-width",
					width-handle_pos );
	}
	else
	{
		g_settings_set_int(	config,
					"playlist-width",
					wnd->playlist_width );
	}

	g_settings_set_boolean(	config,
				"show-playlist",
				wnd->playlist_visible );
}

void main_window_load_state(MainWindow *wnd)
{
	GSettingsBackend *config_backend;
	GSettings *config;

	config_backend = g_keyfile_settings_backend_new
				(	get_config_file_path(),
					CONFIG_ROOT_PATH,
					CONFIG_ROOT_GROUP );

	config = g_settings_new_with_backend(CONFIG_WIN_STATE, config_backend);
	wnd->init_width = g_settings_get_int(config, "width");
	wnd->init_height = g_settings_get_int(config, "height");

	wnd->init_playlist_visible
		= g_settings_get_boolean(config, "show-playlist");

	g_signal_connect(	wnd,
				"configure-event",
				G_CALLBACK(configure_handler),
				wnd );

	/* This is needed even when show_playlist==false to initialize some
	 * internal variables.
	 */
	main_window_set_playlist_visible(wnd, TRUE);

	wnd->playlist_width = g_settings_get_int(config, "playlist-width");

	gtk_window_resize(GTK_WINDOW(wnd), wnd->init_width, wnd->init_height);
}

static void main_window_init(MainWindow *wnd)
{
	GdkRGBA vid_area_bg_color;

	wnd->fullscreen = FALSE;
	wnd->playlist_visible = FALSE;
	wnd->playlist_width = PLAYLIST_DEFAULT_WIDTH;
	wnd->timeout_tag = 0;
	wnd->init_width = MAIN_WINDOW_DEFAULT_WIDTH;
	wnd->init_height = MAIN_WINDOW_DEFAULT_HEIGHT;
	wnd->settings = gtk_settings_get_default();
	wnd->header_bar = gtk_header_bar_new();
	wnd->open_hdr_btn = NULL;
	wnd->fullscreen_hdr_btn = NULL;
	wnd->menu_hdr_btn = NULL;
	wnd->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	wnd->vid_area_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	wnd->vid_area = gtk_drawing_area_new();
	wnd->fs_control = gtk_window_new(GTK_WINDOW_POPUP);
	wnd->control_box = control_box_new();
	wnd->playlist = playlist_widget_new();

	gdk_rgba_parse(&vid_area_bg_color, VID_AREA_BG_COLOR);

	gtk_widget_add_events(	wnd->fs_control,
				GDK_ENTER_NOTIFY_MASK
				|GDK_LEAVE_NOTIFY_MASK );

	gtk_widget_add_events(wnd->vid_area, GDK_BUTTON_PRESS_MASK);

	gtk_header_bar_set_show_close_button(	GTK_HEADER_BAR(wnd->header_bar),
						TRUE );

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

	g_signal_connect(	wnd,
				"motion-notify-event",
				G_CALLBACK(motion_notify_handler),
				wnd );
}

GtkWidget *main_window_new(GtkApplication *app)
{
	return GTK_WIDGET(g_object_new(	main_window_get_type(),
					"application", app,
					NULL ));
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

		wnd_type = g_type_register_static
				(	GTK_TYPE_APPLICATION_WINDOW,
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

		if(main_window_get_csd_enabled(wnd))
		{
			control_box_set_fullscreen_btn_visible
				(CONTROL_BOX(wnd->control_box), FALSE);
		}
		else
		{
			gtk_application_window_set_show_menubar
				(GTK_APPLICATION_WINDOW(wnd), TRUE);
		}

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
		gtk_widget_show(wnd->fs_control);
		gtk_widget_set_opacity(wnd->fs_control, 0);

		if(main_window_get_csd_enabled(wnd))
		{
			control_box_set_fullscreen_btn_visible
				(CONTROL_BOX(wnd->control_box), TRUE);
		}
		else
		{
			gtk_application_window_set_show_menubar
				(GTK_APPLICATION_WINDOW(wnd), FALSE);
		}

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

gint main_window_get_width_margin(MainWindow *wnd)
{
	return	gtk_widget_get_allocated_width(GTK_WIDGET(wnd))
		- gtk_widget_get_allocated_width(wnd->vid_area);
}

gint main_window_get_height_margin(MainWindow *wnd)
{
	return	gtk_widget_get_allocated_height(GTK_WIDGET(wnd))
		- gtk_widget_get_allocated_height(wnd->vid_area);
}

void main_window_enable_csd(MainWindow *wnd)
{
	GIcon *open_icon;
	GIcon *fullscreen_icon;
	GIcon *menu_icon;

	open_icon = g_themed_icon_new_with_default_fallbacks
				("list-add-symbolic");

	fullscreen_icon = g_themed_icon_new_with_default_fallbacks
				("view-fullscreen-symbolic");

	menu_icon = g_themed_icon_new_with_default_fallbacks
				("view-list-symbolic");

	wnd->playlist_width = PLAYLIST_DEFAULT_WIDTH+PLAYLIST_CSD_WIDTH_OFFSET;
	wnd->open_hdr_btn = gtk_menu_button_new();
	wnd->fullscreen_hdr_btn = gtk_button_new();
	wnd->menu_hdr_btn = gtk_menu_button_new();

	gtk_widget_set_can_focus(wnd->open_hdr_btn, FALSE);
	gtk_widget_set_can_focus(wnd->fullscreen_hdr_btn, FALSE);
	gtk_widget_set_can_focus(wnd->menu_hdr_btn, FALSE);

	gtk_button_set_image
		(	GTK_BUTTON(wnd->fullscreen_hdr_btn),
			gtk_image_new_from_gicon
				(fullscreen_icon, GTK_ICON_SIZE_MENU ));

	gtk_button_set_image
		(	GTK_BUTTON(wnd->open_hdr_btn),
			gtk_image_new_from_gicon
				(open_icon, GTK_ICON_SIZE_MENU ));

	gtk_button_set_image
		(	GTK_BUTTON(wnd->menu_hdr_btn),
			gtk_image_new_from_gicon
				(menu_icon, GTK_ICON_SIZE_MENU ));

	gtk_menu_button_set_menu_model
		(	GTK_MENU_BUTTON(wnd->open_hdr_btn),
			G_MENU_MODEL(open_btn_build_menu()) );

	gtk_menu_button_set_menu_model
		(	GTK_MENU_BUTTON(wnd->menu_hdr_btn),
			G_MENU_MODEL(menu_btn_build_menu()) );

	gtk_header_bar_pack_start
		(GTK_HEADER_BAR(wnd->header_bar), wnd->open_hdr_btn);

	gtk_header_bar_pack_end
		(GTK_HEADER_BAR(wnd->header_bar), wnd->menu_hdr_btn);

	gtk_header_bar_pack_end
		(GTK_HEADER_BAR(wnd->header_bar), wnd->fullscreen_hdr_btn);

	gtk_actionable_set_action_name
		(GTK_ACTIONABLE(wnd->fullscreen_hdr_btn), "app.fullscreen");

	gtk_paned_set_position(	GTK_PANED(wnd->vid_area_paned),
				MAIN_WINDOW_DEFAULT_WIDTH
				-PLAYLIST_DEFAULT_WIDTH
				-PLAYLIST_CSD_WIDTH_OFFSET );

	gtk_window_set_titlebar(GTK_WINDOW(wnd), wnd->header_bar);
	gtk_window_set_title(GTK_WINDOW(wnd), g_get_application_name());
}

gboolean main_window_get_csd_enabled(MainWindow *wnd)
{
	return	wnd->open_hdr_btn &&
		wnd->fullscreen_hdr_btn &&
		wnd->menu_hdr_btn;
}

void main_window_set_playlist_visible(MainWindow *wnd, gboolean visible)
{
	gint offset;
	gint handle_pos;
	gint width;
	gint height;

	offset = main_window_get_csd_enabled(wnd)?PLAYLIST_CSD_WIDTH_OFFSET:0;
	handle_pos = gtk_paned_get_position(GTK_PANED(wnd->vid_area_paned));

	gtk_window_get_size(GTK_WINDOW(wnd), &width, &height);

	if(!visible && wnd->playlist_visible)
	{
		wnd->playlist_width = width-handle_pos;
	}

	wnd->playlist_visible = visible;

	gtk_widget_set_visible(wnd->playlist, visible);

	/* For some unknown reason, width needs to be adjusted by some offset
	 * (50px) when CSD is enabled for the resulting size to be correct.
	 */
	gtk_window_resize(	GTK_WINDOW(wnd),
				visible
				?width+wnd->playlist_width-offset
				:handle_pos+offset,
				height );
}

gboolean main_window_get_playlist_visible(MainWindow *wnd)
{
	return gtk_widget_get_visible(GTK_WIDGET(wnd->playlist));
}
