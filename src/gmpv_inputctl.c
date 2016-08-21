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

#include <gtk/gtk.h>
#include <string.h>

#include "gmpv_inputctl.h"
#include "gmpv_def.h"

static gchar *get_full_keystr(guint keyval, guint state);
static gboolean key_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gboolean key_release_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gboolean mouse_button_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gboolean mouse_move_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gboolean scroll_handler(	GtkWidget *widget,
				GdkEvent *event,
				gpointer data );

static gchar *get_full_keystr(guint keyval, guint state)
{
	/* strlen("Ctrl+Alt+Shift+Meta+")+1 == 21 */
	const gsize max_modstr_len = 21;
	gchar modstr[max_modstr_len];
	gboolean found = FALSE;
	const gchar *keystr = gdk_keyval_name(keyval);
	const gchar *keystrmap[] = KEYSTRING_MAP;
	modstr[0] = '\0';

	if((state&GDK_SHIFT_MASK) != 0)
	{
		g_strlcat(modstr, "Shift+", max_modstr_len);
	}

	if((state&GDK_CONTROL_MASK) != 0)
	{
		g_strlcat(modstr, "Ctrl+", max_modstr_len);
	}

	if((state&GDK_MOD1_MASK) != 0)
	{
		g_strlcat(modstr, "Alt+", max_modstr_len);
	}

	if((state&GDK_META_MASK) != 0 || (state&GDK_SUPER_MASK) != 0)
	{
		g_strlcat(modstr, "Meta+", max_modstr_len);
	}

	/* Translate GDK key name to mpv key name */
	for(gint i = 0; !found && keystrmap[i]; i += 2)
	{
		gint rc = g_ascii_strncasecmp(	keystr,
						keystrmap[i+1],
						KEYSTRING_MAX_LEN );

		if(rc == 0)
		{
			keystr = keystrmap[i];
			found = TRUE;
		}
	}

	return (strlen(keystr) > 0)?g_strconcat(modstr, keystr, NULL):NULL;
}

static gboolean key_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	const gchar *cmd[] = {"keydown", NULL, NULL};
	GmpvApplication *app = data;
	guint keyval = ((GdkEventKey*)event)->keyval;
	guint state = ((GdkEventKey*)event)->state;
	gchar *keystr = NULL;

	cmd[1] = keystr = get_full_keystr(keyval, state);

	if(keystr)
	{
		GmpvMpvObj *mpv = gmpv_application_get_mpv_obj(app);

		g_debug("Sent '%s' key down to mpv", keystr);
		gmpv_mpv_obj_command(mpv, cmd);

		g_free(keystr);
	}

	return FALSE;
}

static gboolean key_release_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	GmpvApplication *app = data;
	guint keyval = ((GdkEventKey*)event)->keyval;
	guint state = ((GdkEventKey*)event)->state;
	gchar *keystr = NULL;
	const gchar *cmd[] = {"keyup", NULL, NULL};

	cmd[1] = keystr = get_full_keystr(keyval, state);

	if(keystr)
	{
		GmpvMpvObj *mpv = gmpv_application_get_mpv_obj(app);

		g_debug("Sent '%s' key up to mpv", keystr);
		gmpv_mpv_obj_command(mpv, cmd);

		g_free(keystr);
	}

	return FALSE;
}

static gboolean mouse_button_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	GdkEventButton *btn_event = (GdkEventButton *)event;

	if(btn_event->type == GDK_BUTTON_PRESS
	|| btn_event->type == GDK_BUTTON_RELEASE
	|| btn_event->type == GDK_SCROLL)
	{
		GmpvApplication *app = data;
		GmpvMpvObj *mpv = gmpv_application_get_mpv_obj(app);
		gchar *btn_str =	g_strdup_printf
					("MOUSE_BTN%u", btn_event->button-1);
		const gchar *type_str =	(btn_event->type == GDK_SCROLL)?
					"keypress":
					(btn_event->type == GDK_BUTTON_PRESS)?
					"keydown":"keyup";
		const gchar *key_cmd[] = {type_str, btn_str, NULL};

		g_debug(	"Sent %s event for button %s to mpv",
				type_str, btn_str );

		gmpv_mpv_obj_command(mpv, key_cmd);

		g_free(btn_str);
	}

	return TRUE;
}

static gboolean mouse_move_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	GmpvApplication *app = data;
	GmpvMpvObj *mpv = gmpv_application_get_mpv_obj(app);
	GdkEventMotion *motion_event = (GdkEventMotion *)event;
	gchar *x_str = g_strdup_printf("%d", (gint)motion_event->x);
	gchar *y_str = g_strdup_printf("%d", (gint)motion_event->y);
	const gchar *cmd[] = {"mouse", x_str, y_str, NULL};

	gmpv_mpv_obj_command(mpv, cmd);

	g_free(x_str);
	g_free(y_str);

	return FALSE;
}

static gboolean scroll_handler(	GtkWidget *widget,
				GdkEvent *event,
				gpointer data )
{
	GdkEventScroll *scroll_event = (GdkEventScroll *)event;
	guint button = 0;
	gint count = 0;

	/* Only one axis will be used at a time to prevent accidental activation
	 * of commands bound to buttons associated with the other axis.
	 */
	if(ABS(scroll_event->delta_x) > ABS(scroll_event->delta_y))
	{
		count = (gint)ABS(scroll_event->delta_x);

		if(scroll_event->delta_x <= -1)
		{
			button = 6;
		}
		else if(scroll_event->delta_x >= 1)
		{
			button = 7;
		}
	}
	else
	{
		count = (gint)ABS(scroll_event->delta_y);

		if(scroll_event->delta_y <= -1)
		{
			button = 4;
		}
		else if(scroll_event->delta_y >= 1)
		{
			button = 5;
		}
	}

	if(button > 0)
	{
		GdkEventButton btn_event;

		btn_event.type = scroll_event->type;
		btn_event.window = scroll_event->window;
		btn_event.send_event = scroll_event->send_event;
		btn_event.time = scroll_event->time;
		btn_event.x = scroll_event->x;
		btn_event.y = scroll_event->y;
		btn_event.axes = NULL;
		btn_event.state = scroll_event->state;
		btn_event.button = button;
		btn_event.device = scroll_event->device;
		btn_event.x_root = scroll_event->x_root;
		btn_event.y_root = scroll_event->y_root;

		for(gint i = 0; i < count; i++)
		{
			/* Not used */
			gboolean rc;

			g_signal_emit_by_name
				(widget, "button-press-event", &btn_event, &rc);
		}
	}

	return TRUE;
}

void gmpv_inputctl_connect_signals(GmpvApplication *app)
{
	GmpvMainWindow *wnd = gmpv_application_get_main_window(app);
	GmpvVideoArea *video_area = gmpv_main_window_get_video_area(wnd);

	g_signal_connect(	wnd,
				"key-press-event",
				G_CALLBACK(key_press_handler),
				app );
	g_signal_connect(	wnd,
				"key-release-event",
				G_CALLBACK(key_release_handler),
				app );
	g_signal_connect(	video_area,
				"button-press-event",
				G_CALLBACK(mouse_button_handler),
				app );
	g_signal_connect(	video_area,
				"button-release-event",
				G_CALLBACK(mouse_button_handler),
				app );
	g_signal_connect(	video_area,
				"motion-notify-event",
				G_CALLBACK(mouse_move_handler),
				app );
	g_signal_connect(	video_area,
				"scroll-event",
				G_CALLBACK(scroll_handler),
				app );
}
