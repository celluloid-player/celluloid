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

#include "def.h"
#include "playlist_widget.h"
#include "main_window.h"
#include "main_menu_bar.h"

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

static gchar *seekbar_format_handler(	GtkScale *scale,
					gdouble value,
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

static gchar *seekbar_format_handler(	GtkScale *scale,
					gdouble value,
					gpointer data )
{
	gint sec = value;
	gint length = ((MainWindow *)data)->seek_bar_length;
	char *output = NULL;

	/* Longer than one hour */
	if(length > 3600 && sec >= 0)
	{
		output = g_strdup_printf(	"%02d:%02d:%02d/"
						"%02d:%02d:%02d",
						sec/3600,
						(sec%3600)/60,
						sec%60,
						length/3600,
						(length%3600)/60,
						length%60 );
	}
	else
	{
		/* Set variable to 0 if the value is negative */
		length *= (length >= 0);
		sec *= (sec >= 0);

		output = g_strdup_printf(	"%02d:%02d/"
						"%02d:%02d",
						(sec%3600)/60,
						sec%60,
						(length%3600)/60,
						length%60 );
	}

	return output;
}

static void main_window_init(MainWindow *wnd)
{
	GdkRGBA vid_area_bg_color;
	MainMenuBar *menu;
	GtkWidget *play_icon;
	GtkWidget *stop_icon;
	GtkWidget *forward_icon;
	GtkWidget *rewind_icon;
	GtkWidget *previous_icon;
	GtkWidget *next_icon;
	GtkWidget *fullscreen_icon;

	wnd->fullscreen = FALSE;
	wnd->playlist_visible = FALSE;
	wnd->playlist_width = PLAYLIST_DEFAULT_WIDTH;
	wnd->seek_bar_length = -1;
	wnd->settings = gtk_settings_get_default();
	wnd->accel_group = gtk_accel_group_new();
	wnd->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	wnd->vid_area_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	wnd->vid_area = gtk_drawing_area_new();
	wnd->control_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	wnd->fs_control = gtk_window_new(GTK_WINDOW_POPUP);
	wnd->menu = main_menu_bar_new();
	wnd->play_button = gtk_button_new_with_label(NULL);
	wnd->stop_button = gtk_button_new_with_label(NULL);
	wnd->forward_button = gtk_button_new_with_label(NULL);
	wnd->rewind_button = gtk_button_new_with_label(NULL);
	wnd->next_button = gtk_button_new_with_label(NULL);
	wnd->previous_button = gtk_button_new_with_label(NULL);
	wnd->fullscreen_button = gtk_button_new_with_label(NULL);
	wnd->volume_button = gtk_volume_button_new();
	wnd->seek_bar = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, NULL);
	wnd->playlist = playlist_widget_new();

	play_icon
		= gtk_image_new_from_icon_name(	"media-playback-start",
						GTK_ICON_SIZE_BUTTON );

	stop_icon
		= gtk_image_new_from_icon_name(	"media-playback-stop",
						GTK_ICON_SIZE_BUTTON );

	forward_icon
		= gtk_image_new_from_icon_name(	"media-seek-forward",
						GTK_ICON_SIZE_BUTTON );

	rewind_icon
		= gtk_image_new_from_icon_name(	"media-seek-backward",
						GTK_ICON_SIZE_BUTTON );

	next_icon
		= gtk_image_new_from_icon_name(	"media-skip-forward",
						GTK_ICON_SIZE_BUTTON );

	previous_icon
		= gtk_image_new_from_icon_name(	"media-skip-backward",
						GTK_ICON_SIZE_BUTTON );

	fullscreen_icon
		= gtk_image_new_from_icon_name(	"view-fullscreen",
						GTK_ICON_SIZE_BUTTON );

	wnd->seek_bar = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, NULL);

	menu = MAIN_MENU(wnd->menu);

	gdk_rgba_parse(&vid_area_bg_color, VID_AREA_BG_COLOR);

	gtk_widget_add_events(	wnd->fs_control,
				GDK_ENTER_NOTIFY_MASK
				|GDK_LEAVE_NOTIFY_MASK );

	g_object_set(	wnd->settings,
			"gtk-application-prefer-dark-theme",
			TRUE,
			NULL );

	g_object_set(wnd->play_button, "relief", GTK_RELIEF_NONE, NULL);
	g_object_set(wnd->stop_button, "relief", GTK_RELIEF_NONE, NULL);
	g_object_set(wnd->forward_button, "relief", GTK_RELIEF_NONE, NULL);
	g_object_set(wnd->rewind_button, "relief", GTK_RELIEF_NONE, NULL);
	g_object_set(wnd->next_button, "relief", GTK_RELIEF_NONE, NULL);
	g_object_set(wnd->previous_button, "relief", GTK_RELIEF_NONE, NULL);
	g_object_set(wnd->fullscreen_button, "relief", GTK_RELIEF_NONE, NULL);
	g_object_set(wnd->seek_bar, "value-pos", GTK_POS_RIGHT, NULL);
	g_object_set(wnd->seek_bar, "digits", 2, NULL);

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

	gtk_widget_set_can_focus(wnd->previous_button, FALSE);
	gtk_widget_set_can_focus(wnd->rewind_button, FALSE);
	gtk_widget_set_can_focus(wnd->play_button, FALSE);
	gtk_widget_set_can_focus(wnd->stop_button, FALSE);
	gtk_widget_set_can_focus(wnd->forward_button, FALSE);
	gtk_widget_set_can_focus(wnd->next_button, FALSE);
	gtk_widget_set_can_focus(wnd->seek_bar, FALSE);
	gtk_widget_set_can_focus(wnd->volume_button, FALSE);
	gtk_widget_set_can_focus(wnd->fullscreen_button, FALSE);

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

	gtk_widget_add_accelerator(	menu->fullscreen_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_F11,
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

	gtk_button_set_image
		(GTK_BUTTON(wnd->play_button), play_icon);

	gtk_button_set_image
		(GTK_BUTTON(wnd->stop_button), stop_icon);

	gtk_button_set_image
		(GTK_BUTTON(wnd->forward_button), forward_icon);

	gtk_button_set_image
		(GTK_BUTTON(wnd->rewind_button), rewind_icon);

	gtk_button_set_image
		(GTK_BUTTON(wnd->next_button), next_icon);

	gtk_button_set_image
		(GTK_BUTTON(wnd->previous_button), previous_icon);

	gtk_button_set_image
		(GTK_BUTTON(wnd->fullscreen_button), fullscreen_icon);

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
		(GTK_CONTAINER(wnd->control_box), wnd->previous_button);

	gtk_container_add
		(GTK_CONTAINER(wnd->control_box), wnd->rewind_button);

	gtk_container_add
		(GTK_CONTAINER(wnd->control_box), wnd->play_button);

	gtk_container_add
		(GTK_CONTAINER(wnd->control_box), wnd->stop_button);

	gtk_container_add
		(GTK_CONTAINER(wnd->control_box), wnd->forward_button);

	gtk_container_add
		(GTK_CONTAINER(wnd->control_box), wnd->next_button);

	gtk_box_pack_start
		(GTK_BOX(wnd->control_box), wnd->seek_bar, TRUE, TRUE, 0);

	gtk_container_add
		(GTK_CONTAINER(wnd->control_box), wnd->volume_button);

	gtk_container_add
		(GTK_CONTAINER(wnd->control_box), wnd->fullscreen_button);

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

	g_signal_connect(	wnd->seek_bar,
				"format-value",
				G_CALLBACK(seekbar_format_handler),
				wnd );

	gtk_widget_show_all(GTK_WIDGET(wnd));
	gtk_widget_hide(wnd->playlist);
	main_window_hide_chapter_control(wnd);
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
	GtkWidget *icon;

	if(wnd->fullscreen)
	{
		icon = gtk_image_new_from_icon_name(	"view-fullscreen",
							GTK_ICON_SIZE_BUTTON );

		gtk_button_set_image(GTK_BUTTON(wnd->fullscreen_button), icon);
		gtk_window_unfullscreen(GTK_WINDOW(wnd));
		gtk_widget_reparent(wnd->control_box, wnd->main_box);
		gtk_widget_hide(wnd->fs_control);
		gtk_widget_show(wnd->menu);

		if(wnd->playlist_visible)
		{
			gtk_widget_show(wnd->playlist);
		}
	}
	else
	{
		GdkScreen *screen;
		gint width;
		gint height;

		screen = gtk_window_get_screen(GTK_WINDOW(wnd));
		width = gdk_screen_width()/2;

		icon = gtk_image_new_from_icon_name(	"view-restore",
							GTK_ICON_SIZE_BUTTON );

		gtk_button_set_image(	GTK_BUTTON(wnd->fullscreen_button),
					icon );

		gtk_window_fullscreen(GTK_WINDOW(wnd));
		gtk_window_set_screen(GTK_WINDOW(wnd->fs_control), screen);
		gtk_widget_reparent(wnd->control_box, wnd->fs_control);
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
	}
}

void main_window_reset_control(MainWindow *wnd)
{
	GtkWidget *play_icon;
	GtkWidget *fullscreen_icon;

	play_icon = gtk_image_new_from_icon_name(	"media-playback-start",
							GTK_ICON_SIZE_BUTTON );

	fullscreen_icon = gtk_image_new_from_icon_name(	"view-fullscreen",
							GTK_ICON_SIZE_BUTTON );

	main_window_set_seek_bar_length(wnd, 0);
	gtk_button_set_image(GTK_BUTTON(wnd->play_button), play_icon);

	gtk_button_set_image(	GTK_BUTTON(wnd->fullscreen_button),
				fullscreen_icon );
}

void main_window_set_seek_bar_length(MainWindow *wnd, gint length)
{
	wnd->seek_bar_length = length;

	gtk_range_set_range(GTK_RANGE(wnd->seek_bar), 0, length);
}

void main_window_set_control_enabled(MainWindow *wnd, gboolean enabled)
{
	gtk_widget_set_sensitive(wnd->previous_button, enabled);
	gtk_widget_set_sensitive(wnd->rewind_button, enabled);
	gtk_widget_set_sensitive(wnd->stop_button, enabled);
	gtk_widget_set_sensitive(wnd->play_button, enabled);
	gtk_widget_set_sensitive(wnd->forward_button, enabled);
	gtk_widget_set_sensitive(wnd->next_button, enabled);
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

void main_window_show_chapter_control(MainWindow *wnd)
{
	gtk_widget_show_all(wnd->previous_button);
	gtk_widget_show_all(wnd->next_button);
}

void main_window_hide_chapter_control(MainWindow *wnd)
{
	gtk_widget_hide(wnd->previous_button);
	gtk_widget_hide(wnd->next_button);
}
