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

#include "gmpv_video_area.h"
#include "gmpv_control_box.h"
#include "gmpv_def.h"

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

struct _GmpvVideoArea
{
	GtkOverlay parent_instance;
	GtkWidget *stack;
	GtkWidget *draw_area;
	GtkWidget *gl_area;
	GtkWidget *control_box;
	GtkWidget *fs_revealer;
	guint timeout_tag;
	gboolean fullscreen;
	gboolean fs_control_hover;
};

struct _GmpvVideoAreaClass
{
	GtkOverlayClass parent_class;
};

static void set_cursor_visible(GmpvVideoArea *area, gboolean visible);
static gboolean timeout_handler(gpointer data);
static gboolean motion_notify_handler(GtkWidget *widget, GdkEventMotion *event);
static gboolean fs_control_crossing_handler(	GtkWidget *widget,
						GdkEventCrossing *event,
						gpointer data );

G_DEFINE_TYPE(GmpvVideoArea, gmpv_video_area, GTK_TYPE_OVERLAY)

static void set_cursor_visible(GmpvVideoArea *area, gboolean visible)
{
	GdkWindow *window;
	GdkCursor *cursor;

	window = gtk_widget_get_window(GTK_WIDGET(area));

	if(visible)
	{
		cursor =	gdk_cursor_new_from_name
				(gdk_display_get_default(), "default");
	}
	else
	{
		cursor =	gdk_cursor_new_for_display
				(gdk_display_get_default(), GDK_BLANK_CURSOR);
	}

	gdk_window_set_cursor(window, cursor);
	g_object_unref(cursor);
}

static gboolean timeout_handler(gpointer data)
{
	GmpvVideoArea *area = data;
	GmpvControlBox *control_box = GMPV_CONTROL_BOX(area->control_box);

	if(area->fullscreen
	&& !area->fs_control_hover
	&& !gmpv_control_box_get_volume_popup_visible(control_box))
	{
		set_cursor_visible(area, FALSE);
		gtk_revealer_set_reveal_child
			(GTK_REVEALER(area->fs_revealer), FALSE);
	}

	area->timeout_tag = 0;

	return FALSE;
}

static gboolean motion_notify_handler(GtkWidget *widget, GdkEventMotion *event)
{
	GmpvVideoArea *area = GMPV_VIDEO_AREA(widget);
	GdkCursor *cursor;

	cursor = gdk_cursor_new_from_name(gdk_display_get_default(), "default");
	gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);

	if(area->fullscreen)
	{
		gtk_revealer_set_reveal_child
			(GTK_REVEALER(area->fs_revealer), TRUE);
	}

	if(area->timeout_tag > 0)
	{
		g_source_remove(area->timeout_tag);
	}

	area->timeout_tag = g_timeout_add_seconds(	FS_CONTROL_HIDE_DELAY,
							timeout_handler,
							area );

	return	GTK_WIDGET_CLASS(gmpv_video_area_parent_class)
		->motion_notify_event(widget, event);
}

static gboolean fs_control_crossing_handler(	GtkWidget *widget,
						GdkEventCrossing *event,
						gpointer data )
{
	GMPV_VIDEO_AREA(data)->fs_control_hover = (event->type == GDK_ENTER_NOTIFY);

	return FALSE;
}

static void gmpv_video_area_class_init(GmpvVideoAreaClass *klass)
{
	GtkWidgetClass *wgt_class = GTK_WIDGET_CLASS(klass);

	wgt_class->motion_notify_event = motion_notify_handler;
}

static void gmpv_video_area_init(GmpvVideoArea *area)
{
	GtkTargetEntry targets[] = DND_TARGETS;

	/* GDK_BUTTON_RELEASE_MASK is needed so that GtkMenuButtons can
	 * hide their menus when vid_area is clicked.
	 */
	const gint extra_events =	GDK_BUTTON_PRESS_MASK|
					GDK_BUTTON_RELEASE_MASK|
					GDK_POINTER_MOTION_MASK|
					GDK_SMOOTH_SCROLL_MASK|
					GDK_SCROLL_MASK;

	area->stack = gtk_stack_new();
	area->draw_area = gtk_drawing_area_new();
	area->gl_area = gtk_gl_area_new();
	area->control_box = NULL;
	area->fs_revealer = gtk_revealer_new();
	area->timeout_tag = 0;
	area->fullscreen = FALSE;
	area->fs_control_hover = FALSE;

	gtk_style_context_add_class
		(	gtk_widget_get_style_context(area->draw_area),
			"gmpv-vid-area" );

	gtk_drag_dest_set(	GTK_WIDGET(area),
				GTK_DEST_DEFAULT_ALL,
				targets,
				G_N_ELEMENTS(targets),
				GDK_ACTION_COPY );

	gtk_widget_add_events(area->draw_area, extra_events);
	gtk_widget_add_events(area->gl_area, extra_events);

	gtk_widget_set_valign(area->fs_revealer, GTK_ALIGN_END);
	gtk_widget_show(area->fs_revealer);
	gtk_revealer_set_reveal_child(GTK_REVEALER(area->fs_revealer), FALSE);

	g_signal_connect(	area,
				"enter-notify-event",
				G_CALLBACK(fs_control_crossing_handler),
				area );
	g_signal_connect(	area,
				"leave-notify-event",
				G_CALLBACK(fs_control_crossing_handler),
				area );

	gtk_stack_add_named(GTK_STACK(area->stack), area->draw_area, "draw");
	gtk_stack_add_named(GTK_STACK(area->stack), area->gl_area, "gl");
	gtk_stack_set_visible_child(GTK_STACK(area->stack), area->draw_area);

	gtk_overlay_add_overlay(GTK_OVERLAY(area), area->fs_revealer);
	gtk_container_add(GTK_CONTAINER(area), area->stack);
}

GtkWidget *gmpv_video_area_new()
{
	return GTK_WIDGET(g_object_new(gmpv_video_area_get_type(), NULL));
}

void gmpv_video_area_set_fullscreen_state(	GmpvVideoArea *area,
						gboolean fullscreen )
{
	if(area->fullscreen != fullscreen)
	{
		area->fullscreen = fullscreen;

		gtk_widget_set_visible(area->fs_revealer, fullscreen);
		set_cursor_visible(area, !fullscreen);

		if(area->control_box)
		{
			gmpv_control_box_set_fullscreen_state
				(GMPV_CONTROL_BOX(area->control_box), fullscreen);
		}
	}
}

void gmpv_video_area_set_control_box(	GmpvVideoArea *area,
					GtkWidget *control_box )
{
	if(area->control_box)
	{
		gtk_container_remove
			(GTK_CONTAINER(area->fs_revealer), area->control_box);
	}

	area->control_box = control_box;

	if(control_box)
	{
		gtk_container_add
			(GTK_CONTAINER(area->fs_revealer), control_box);
	}
}

void gmpv_video_area_set_use_opengl(GmpvVideoArea *area, gboolean use_opengl)
{
	gtk_stack_set_visible_child
		(	GTK_STACK(area->stack),
			use_opengl?area->gl_area:area->draw_area );
}

GtkDrawingArea *gmpv_video_area_get_draw_area(GmpvVideoArea *area)
{
	return GTK_DRAWING_AREA(area->draw_area);
}

GtkGLArea *gmpv_video_area_get_gl_area(GmpvVideoArea *area)
{
	return GTK_GL_AREA(area->gl_area);
}

gint64 gmpv_video_area_get_xid(GmpvVideoArea *area)
{
#ifdef GDK_WINDOWING_X11
	if(GDK_IS_X11_DISPLAY(gdk_display_get_default()))
	{
		GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(area));
		GdkWindow *window = NULL;

		if(parent && !gtk_widget_get_realized(area->draw_area))
		{
			gtk_widget_realize(area->draw_area);
		}

		window = gtk_widget_get_window(area->draw_area);

		if(!window)
		{
			g_critical("Failed to get XID of video area");
		}

		return window?(gint64)gdk_x11_window_get_xid(window):-1;
	}
#endif

	return -1;
}
