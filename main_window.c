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

#include "main_window.h"

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
	GdkRGBA black;
	GtkWidget *play_icon;
	GtkWidget *stop_icon;
	GtkWidget *forward_icon;
	GtkWidget *rewind_icon;
	GtkWidget *previous_icon;
	GtkWidget *next_icon;
	GtkWidget *fullscreen_icon;

	wnd->fullscreen = FALSE;
	wnd->seek_bar_length = -1;
	wnd->settings = gtk_settings_get_default();
	wnd->accel_group = gtk_accel_group_new();
	wnd->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	wnd->vid_area = gtk_drawing_area_new();
	wnd->control_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	wnd->fs_control = gtk_window_new(GTK_WINDOW_POPUP);
	wnd->menu_bar = gtk_menu_bar_new();
	wnd->file_menu = gtk_menu_new();
	wnd->file_menu_item = gtk_menu_item_new_with_mnemonic("_File");
	wnd->open_menu_item = gtk_menu_item_new_with_mnemonic("_Open");
	wnd->quit_menu_item = gtk_menu_item_new_with_mnemonic("_Quit");
	wnd->edit_menu = gtk_menu_new();
	wnd->edit_menu_item = gtk_menu_item_new_with_mnemonic("_Edit");
	wnd->pref_menu_item = gtk_menu_item_new_with_mnemonic("_Preferences");
	wnd->view_menu = gtk_menu_new();
	wnd->view_menu_item = gtk_menu_item_new_with_mnemonic("_View");
	wnd->help_menu = gtk_menu_new();
	wnd->help_menu_item = gtk_menu_item_new_with_mnemonic("_Help");
	wnd->about_menu_item = gtk_menu_item_new_with_mnemonic("_About");
	wnd->play_button = gtk_button_new_with_label(NULL);
	wnd->stop_button = gtk_button_new_with_label(NULL);
	wnd->forward_button = gtk_button_new_with_label(NULL);
	wnd->rewind_button = gtk_button_new_with_label(NULL);
	wnd->next_button = gtk_button_new_with_label(NULL);
	wnd->previous_button = gtk_button_new_with_label(NULL);
	wnd->fullscreen_button = gtk_button_new_with_label(NULL);
	wnd->volume_button = gtk_volume_button_new();

	wnd->open_loc_menu_item
		= gtk_menu_item_new_with_mnemonic("Open _Location");

	wnd->fullscreen_menu_item
		= gtk_menu_item_new_with_mnemonic("_Fullscreen");

	wnd->normal_size_menu_item
		= gtk_menu_item_new_with_mnemonic("_Normal Size");

	wnd->double_size_menu_item
		= gtk_menu_item_new_with_mnemonic("_Double Size");

	wnd->half_size_menu_item
		= gtk_menu_item_new_with_mnemonic("_Half Size");

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

	gdk_rgba_parse(&black, "#000000");

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

	gtk_window_set_default_size(GTK_WINDOW(wnd), 400, 300);
	gtk_window_set_title(GTK_WINDOW(wnd), g_get_application_name());

	gtk_window_add_accel_group(	GTK_WINDOW(wnd),
					wnd->accel_group );

	gtk_widget_override_background_color(	wnd->vid_area,
						GTK_STATE_NORMAL,
						&black);

	gtk_widget_add_accelerator(	wnd->open_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_o,
					GDK_CONTROL_MASK,
					GTK_ACCEL_VISIBLE );

	gtk_widget_add_accelerator(	wnd->open_loc_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_l,
					GDK_CONTROL_MASK,
					GTK_ACCEL_VISIBLE );

	gtk_widget_add_accelerator(	wnd->quit_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_q,
					GDK_CONTROL_MASK,
					GTK_ACCEL_VISIBLE );

	gtk_widget_add_accelerator(	wnd->pref_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_p,
					GDK_CONTROL_MASK,
					GTK_ACCEL_VISIBLE );

	gtk_widget_add_accelerator(	wnd->fullscreen_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_F11,
					(GdkModifierType)0,
					GTK_ACCEL_VISIBLE );

	gtk_widget_add_accelerator(	wnd->normal_size_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_1,
					GDK_CONTROL_MASK,
					GTK_ACCEL_VISIBLE );

	gtk_widget_add_accelerator(	wnd->double_size_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_2,
					GDK_CONTROL_MASK,
					GTK_ACCEL_VISIBLE );

	gtk_widget_add_accelerator(	wnd->half_size_menu_item,
					"activate",
					wnd->accel_group,
					GDK_KEY_3,
					GDK_CONTROL_MASK,
					GTK_ACCEL_VISIBLE );

	gtk_widget_set_can_focus(wnd->previous_button, FALSE);
	gtk_widget_set_can_focus(wnd->rewind_button, FALSE);
	gtk_widget_set_can_focus(wnd->play_button, FALSE);
	gtk_widget_set_can_focus(wnd->stop_button, FALSE);
	gtk_widget_set_can_focus(wnd->forward_button, FALSE);
	gtk_widget_set_can_focus(wnd->next_button, FALSE);
	gtk_widget_set_can_focus(wnd->seek_bar, FALSE);
	gtk_widget_set_can_focus(wnd->volume_button, FALSE);
	gtk_widget_set_can_focus(wnd->fullscreen_button, FALSE);

	gtk_menu_shell_append
		(GTK_MENU_SHELL(wnd->menu_bar), wnd->file_menu_item);

	gtk_menu_shell_append
		(GTK_MENU_SHELL(wnd->menu_bar), wnd->edit_menu_item);

	gtk_menu_shell_append
		(GTK_MENU_SHELL(wnd->menu_bar), wnd->view_menu_item);

	gtk_menu_shell_append
		(GTK_MENU_SHELL(wnd->menu_bar), wnd->help_menu_item);

	gtk_menu_item_set_submenu
		(GTK_MENU_ITEM(wnd->file_menu_item), wnd->file_menu);

	gtk_menu_item_set_submenu
		(GTK_MENU_ITEM(wnd->edit_menu_item), wnd->edit_menu);

	gtk_menu_item_set_submenu
		(GTK_MENU_ITEM(wnd->view_menu_item), wnd->view_menu);

	gtk_menu_item_set_submenu
		(GTK_MENU_ITEM(wnd->help_menu_item), wnd->help_menu);

	gtk_menu_shell_append
		(GTK_MENU_SHELL(wnd->file_menu), wnd->open_menu_item);

	gtk_menu_shell_append
		(GTK_MENU_SHELL(wnd->file_menu), wnd->open_loc_menu_item);

	gtk_menu_shell_append
		(GTK_MENU_SHELL(wnd->file_menu), gtk_separator_menu_item_new());

	gtk_menu_shell_append
		(GTK_MENU_SHELL(wnd->file_menu), wnd->quit_menu_item);

	gtk_menu_shell_append
		(GTK_MENU_SHELL(wnd->edit_menu), wnd->pref_menu_item);

	gtk_menu_shell_append
		(GTK_MENU_SHELL(wnd->view_menu), wnd->fullscreen_menu_item);

	gtk_menu_shell_append
		(GTK_MENU_SHELL(wnd->view_menu), gtk_separator_menu_item_new());

	gtk_menu_shell_append
		(GTK_MENU_SHELL(wnd->view_menu), wnd->normal_size_menu_item);

	gtk_menu_shell_append
		(GTK_MENU_SHELL(wnd->view_menu), wnd->double_size_menu_item);

	gtk_menu_shell_append
		(GTK_MENU_SHELL(wnd->view_menu), wnd->half_size_menu_item);

	gtk_menu_shell_append
		(GTK_MENU_SHELL(wnd->help_menu), wnd->about_menu_item);

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
		(GTK_CONTAINER(wnd->main_box), wnd->menu_bar);

	gtk_box_pack_start
		(GTK_BOX(wnd->main_box), wnd->vid_area, TRUE, TRUE, 0);

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
		gtk_widget_show(wnd->menu_bar);
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
		gtk_widget_hide(wnd->menu_bar);
		gtk_widget_show(wnd->fs_control);
		gtk_widget_set_opacity(wnd->fs_control, 0);

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

	gtk_range_set_value(GTK_RANGE(wnd->seek_bar), 0);
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
