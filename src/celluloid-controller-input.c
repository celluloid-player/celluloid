/*
 * Copyright (c) 2016-2020 gnome-mpv
 *
 * This file is part of Celluloid.
 *
 * Celluloid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Celluloid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Celluloid.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <glib-object.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <string.h>

#include "celluloid-controller-private.h"
#include "celluloid-controller.h"
#include "celluloid-controller-input.h"
#include "celluloid-mpv.h"
#include "celluloid-mpv-wrapper.h"
#include "celluloid-main-window.h"
#include "celluloid-video-area.h"
#include "celluloid-def.h"

static gchar *
keyval_to_keystr(guint keyval);

static gchar *
get_modstr(guint state);

static gchar *
get_full_keystr(GdkEventKey *event);

static gboolean
key_press_handler(GtkWidget *widget, GdkEvent *event, gpointer data);

static gboolean
key_release_handler(GtkWidget *widget, GdkEvent *event, gpointer data);

static gboolean
mouse_button_handler(GtkWidget *widget, GdkEvent *event, gpointer data);

static gboolean
mouse_move_handler(GtkWidget *widget, GdkEvent *event, gpointer data);

static gboolean
scroll_handler(GtkWidget *widget, GdkEvent *event, gpointer data);

static gchar *
keyval_to_keystr(guint keyval)
{
	const gchar *keystrmap[] = KEYSTRING_MAP;
	gboolean found = FALSE;
	gchar key_utf8[7] = {0}; // 6 bytes for output and 1 for NULL terminator
	const gchar *result = key_utf8;

	g_unichar_to_utf8(gdk_keyval_to_unicode(keyval), key_utf8);
	result = result[0] ? result : gdk_keyval_name(keyval);
	found = !result;

	for(gint i = 0; !found && keystrmap[i]; i += 2)
	{
		const gint rc = g_ascii_strncasecmp
				(result, keystrmap[i+1], KEYSTRING_MAX_LEN);

		if(rc == 0)
		{
			result = keystrmap[i][0] ? keystrmap[i] : NULL;
			found = TRUE;
		}
	}

	return result ? g_strdup(result) : NULL;
}

static gchar *
get_modstr(guint state)
{
	const struct
	{
		guint mask;
		gchar *str;
	}
	mod_map[] = {	{GDK_SHIFT_MASK, "Shift+"},
			{GDK_CONTROL_MASK, "Ctrl+"},
			{GDK_MOD1_MASK, "Alt+"},
			{GDK_SUPER_MASK, "Meta+"}, // Super is Meta in mpv
			{0, NULL} };

	const gsize max_len = 21; // strlen("Ctrl+Alt+Shift+Meta")+1
	gchar *result = g_malloc0(max_len);

	for(gint i = 0; mod_map[i].str; i++)
	{
		if(state & mod_map[i].mask)
		{
			g_strlcat(result, mod_map[i].str, max_len);
		}
	}

	return result;
}

static gchar *
get_full_keystr(GdkEventKey *event)
{
	gchar *modstr = get_modstr(event->state);
	gchar *keystr = keyval_to_keystr(event->keyval);
	char *result = keystr ? g_strconcat(modstr, keystr, NULL) : NULL;

	g_free(keystr);
	g_free(modstr);

	return result;
}

static gboolean
key_press_handler(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	CelluloidController *controller = data;
	gchar *keystr = get_full_keystr((GdkEventKey *)event);
	gboolean searching = FALSE;

	g_object_get(controller->view, "searching", &searching, NULL);

	if(keystr && !searching)
	{
		celluloid_model_key_down(controller->model, keystr);
		g_free(keystr);
	}

	return FALSE;
}

static gboolean
key_release_handler(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	CelluloidController *controller = data;
	gchar *keystr = get_full_keystr((GdkEventKey *)event);
	gboolean searching = FALSE;

	g_object_get(controller->view, "searching", &searching, NULL);

	if(keystr && !searching)
	{
		celluloid_model_key_up(controller->model, keystr);
		g_free(keystr);
	}

	return FALSE;
}

static gboolean
mouse_button_handler(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	GdkEventButton *btn_event = (GdkEventButton *)event;

	if(btn_event->type == GDK_BUTTON_PRESS
	|| btn_event->type == GDK_BUTTON_RELEASE
	|| btn_event->type == GDK_SCROLL)
	{
		CelluloidController *controller = data;
		gchar *btn_str =	g_strdup_printf
					("MOUSE_BTN%u", btn_event->button-1);
		void (*func)(CelluloidModel *, const gchar *)
			=	(btn_event->type == GDK_SCROLL)?
				celluloid_model_key_press:
				(btn_event->type == GDK_BUTTON_PRESS)?
				celluloid_model_key_down:celluloid_model_key_up;

		func(controller->model, btn_str);

		g_free(btn_str);
	}

	return TRUE;
}

static gboolean
mouse_move_handler(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	CelluloidController *controller = data;
	GdkEventMotion *motion_event = (GdkEventMotion *)event;

	if(controller->model)
	{
		celluloid_model_mouse(	controller->model,
					(gint)motion_event->x,
					(gint)motion_event->y );
	}

	return FALSE;
}

static gboolean
scroll_handler(GtkWidget *widget, GdkEvent *event, gpointer data)
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

void
celluloid_controller_input_connect_signals(CelluloidController *controller)
{
	CelluloidMainWindow *wnd =	celluloid_view_get_main_window
					(controller->view);
	CelluloidVideoArea *video_area =	celluloid_main_window_get_video_area
						(wnd);

	g_signal_connect(	controller->view,
				"key-press-event",
				G_CALLBACK(key_press_handler),
				controller );
	g_signal_connect(	controller->view,
				"key-release-event",
				G_CALLBACK(key_release_handler),
				controller );
	g_signal_connect(	video_area,
				"button-press-event",
				G_CALLBACK(mouse_button_handler),
				controller );
	g_signal_connect(	video_area,
				"button-release-event",
				G_CALLBACK(mouse_button_handler),
				controller );
	g_signal_connect(	controller->view,
				"motion-notify-event",
				G_CALLBACK(mouse_move_handler),
				controller );
	g_signal_connect(	video_area,
				"scroll-event",
				G_CALLBACK(scroll_handler),
				controller );
}
