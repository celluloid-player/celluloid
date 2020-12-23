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

#include "celluloid-video-area.h"
#include "celluloid-control-box.h"
#include "celluloid-common.h"
#include "celluloid-def.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib-object.h>
#include <math.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

enum
{
	PROP_0,
	PROP_TITLE,
	N_PROPERTIES
};

struct _CelluloidVideoArea
{
	GtkOverlay parent_instance;
	GtkWidget *stack;
	GtkWidget *draw_area;
	GtkWidget *gl_area;
	GtkWidget *control_box;
	GtkWidget *header_bar;
	GtkWidget *control_box_revealer;
	GtkWidget *header_bar_revealer;
	guint32 last_motion_time;
	gdouble last_motion_x;
	gdouble last_motion_y;
	guint timeout_tag;
	gboolean fullscreen;
	gboolean fs_control_hover;
};

struct _CelluloidVideoAreaClass
{
	GtkOverlayClass parent_class;
};

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec );

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec );

static
void destroy(GtkWidget *widget);

static
void set_cursor_visible(CelluloidVideoArea *area, gboolean visible);

static
gboolean timeout_handler(gpointer data);

static
gboolean motion_notify_event(GtkWidget *widget, GdkEventMotion *event);

static gboolean
fs_control_crossing_handler(	GtkWidget *widget,
				GdkEventCrossing *event,
				gpointer data );

G_DEFINE_TYPE(CelluloidVideoArea, celluloid_video_area, GTK_TYPE_OVERLAY)

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidVideoArea *self = CELLULOID_VIDEO_AREA(object);

	switch(property_id)
	{
		case PROP_TITLE:
		g_object_set_property(	G_OBJECT(self->header_bar),
					pspec->name,
					value );
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec )
{
	CelluloidVideoArea *self = CELLULOID_VIDEO_AREA(object);

	switch(property_id)
	{
		case PROP_TITLE:
		g_object_get_property(	G_OBJECT(self->header_bar),
					pspec->name,
					value );
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
destroy(GtkWidget *widget)
{
	g_source_clear(&CELLULOID_VIDEO_AREA(widget)->timeout_tag);
	GTK_WIDGET_CLASS(celluloid_video_area_parent_class)->destroy(widget);
}

static void
set_cursor_visible(CelluloidVideoArea *area, gboolean visible)
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

static gboolean
timeout_handler(gpointer data)
{
	CelluloidVideoArea *area = data;
	CelluloidControlBox *control_box = CELLULOID_CONTROL_BOX(area->control_box);
	CelluloidHeaderBar *header_bar = CELLULOID_HEADER_BAR(area->header_bar);
	gboolean open_button_active = FALSE;
	gboolean menu_button_active = FALSE;

	g_object_get(	header_bar,
			"open-button-active", &open_button_active,
			"menu-button-active", &menu_button_active,
			NULL );

	if(control_box
	&& !area->fs_control_hover
	&& !celluloid_control_box_get_volume_popup_visible(control_box)
	&& !open_button_active
	&& !menu_button_active)
	{
		GSettings *settings;
		gboolean always_autohide;

		settings = g_settings_new(CONFIG_ROOT);
		always_autohide =	g_settings_get_boolean
					(settings, "always-autohide-cursor");

		gtk_revealer_set_reveal_child
			(GTK_REVEALER(area->control_box_revealer), FALSE);
		gtk_revealer_set_reveal_child
			(GTK_REVEALER(area->header_bar_revealer), FALSE);

		set_cursor_visible(area, !(always_autohide || area->fullscreen));
		area->timeout_tag = 0;

		g_object_unref(settings);
	}
	else if(!control_box)
	{
		area->timeout_tag = 0;
	}

	/* Try again later if timeout_tag has not been cleared. This means that
	 * either one of the popups is visible or the cursor is hovering over
	 * the control box, preventing it from being hidden.
	 */
	return (area->timeout_tag != 0);
}

static gboolean
motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
	GSettings *settings = g_settings_new(CONFIG_ROOT);
	CelluloidVideoArea *area = CELLULOID_VIDEO_AREA(widget);
	const gint height = gtk_widget_get_allocated_height(widget);

	const gdouble unhide_speed =
		g_settings_get_double(settings, "controls-unhide-cursor-speed");
	const gdouble dead_zone =
		g_settings_get_double(settings, "controls-dead-zone-size");

	const gdouble dist = sqrt(	pow(event->x - area->last_motion_x, 2) +
					pow(event->y - area->last_motion_y, 2) );
	const gdouble speed = dist / (event->time - area->last_motion_time);

	area->last_motion_time = event->time;
	area->last_motion_x = event->x;
	area->last_motion_y = event->y;

	if(	speed >= unhide_speed &&
		ABS((2 * event->y - height) / height) > dead_zone )
	{
		GdkCursor *cursor =	gdk_cursor_new_from_name
					(	gdk_display_get_default(),
						"default" );

		gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);

		if(area->control_box)
		{
			gtk_revealer_set_reveal_child
				(	GTK_REVEALER(area->control_box_revealer),
					TRUE );
			gtk_revealer_set_reveal_child
				(	GTK_REVEALER(area->header_bar_revealer),
					area->fullscreen );
		}

		g_source_clear(&area->timeout_tag);
		area->timeout_tag =	g_timeout_add_seconds
					(	FS_CONTROL_HIDE_DELAY,
						timeout_handler,
						area );
	}

	g_object_unref(settings);

	return	GTK_WIDGET_CLASS(celluloid_video_area_parent_class)
			->motion_notify_event(widget, event);
}

static gboolean
render_handler(GtkGLArea *gl_area, GdkGLContext *context, gpointer data)
{
	g_signal_emit_by_name(data, "render");

	return TRUE;
}

static void
popover_notify_handler(GObject *gobject, GParamSpec *pspec, gpointer data)
{
	gboolean value = FALSE;

	g_object_get(gobject, pspec->name, &value, NULL);

	if(value)
	{
		set_cursor_visible(CELLULOID_VIDEO_AREA(data), TRUE);
	}
}

static void
notify_handler(GObject *gobject, GParamSpec *pspec, gpointer data)
{
	/* Due to a GTK+ bug, the header bar isn't hidden completely if hidden
	 * by a GtkRevealer. Workaround this by manually hiding the revealer
	 * along with the header bar when the revealer completes its transition
	 * to hidden state.
	 */
	CelluloidVideoArea *area = data;
	gboolean reveal_child = TRUE;
	gboolean child_revealed = TRUE;

	g_object_get(	gobject,
			"reveal-child", &reveal_child,
			"child-revealed", &child_revealed,
			NULL );

	gtk_widget_set_visible(	GTK_WIDGET(area->header_bar_revealer),
				area->fullscreen &&
				(reveal_child || child_revealed) );
}

static gboolean
fs_control_crossing_handler(	GtkWidget *widget,
				GdkEventCrossing *event,
				gpointer data )
{
	CELLULOID_VIDEO_AREA(data)->fs_control_hover =
		(event->type == GDK_ENTER_NOTIFY);

	return FALSE;
}

static void
celluloid_video_area_class_init(CelluloidVideoAreaClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *wgt_class = GTK_WIDGET_CLASS(klass);
	GParamSpec *pspec = NULL;

	obj_class->set_property = set_property;
	obj_class->get_property = get_property;
	wgt_class->destroy = destroy;
	wgt_class->motion_notify_event = motion_notify_event;

	gtk_widget_class_set_css_name(wgt_class, "celluloid-video-area");

	g_signal_new(	"render",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );

	pspec = g_param_spec_string
		(	"title",
			"Title",
			"The title of the header bar",
			NULL,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_TITLE, pspec);
}

static void
celluloid_video_area_init(CelluloidVideoArea *area)
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
	area->control_box = celluloid_control_box_new();
	area->header_bar = celluloid_header_bar_new();
	area->control_box_revealer = gtk_revealer_new();
	area->header_bar_revealer = gtk_revealer_new();
	area->last_motion_time = 0;
	area->last_motion_x = -1;
	area->last_motion_y = -1;
	area->timeout_tag = 0;
	area->fullscreen = FALSE;
	area->fs_control_hover = FALSE;

	gtk_drag_dest_set(	GTK_WIDGET(area),
				GTK_DEST_DEFAULT_ALL,
				targets,
				G_N_ELEMENTS(targets),
				GDK_ACTION_COPY );

	gtk_widget_add_events(area->draw_area, extra_events);
	gtk_widget_add_events(area->gl_area, extra_events);

	gtk_widget_set_valign(area->control_box_revealer, GTK_ALIGN_END);
	gtk_revealer_set_reveal_child
		(GTK_REVEALER(area->control_box_revealer), FALSE);

	gtk_widget_set_valign(area->header_bar_revealer, GTK_ALIGN_START);
	gtk_revealer_set_reveal_child
		(GTK_REVEALER(area->header_bar_revealer), FALSE);

	gtk_widget_show_all(area->control_box);
	gtk_widget_hide(area->control_box_revealer);
	gtk_widget_set_no_show_all(area->control_box_revealer, TRUE);

	gtk_widget_show_all(area->header_bar);
	gtk_widget_hide(area->header_bar_revealer);
	gtk_widget_set_no_show_all(area->header_bar_revealer, TRUE);

	g_signal_connect(	area->gl_area,
				"render",
				G_CALLBACK(render_handler),
				area );
	g_signal_connect(	area->control_box,
				"notify::volume-popup-visible",
				G_CALLBACK(popover_notify_handler),
				area );
	g_signal_connect(	area->header_bar,
				"notify::open-button-active",
				G_CALLBACK(popover_notify_handler),
				area );
	g_signal_connect(	area->header_bar,
				"notify::menu-button-active",
				G_CALLBACK(popover_notify_handler),
				area );
	g_signal_connect(	area->header_bar_revealer,
				"notify::reveal-child",
				G_CALLBACK(notify_handler),
				area );
	g_signal_connect(	area->header_bar_revealer,
				"notify::child-revealed",
				G_CALLBACK(notify_handler),
				area );
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

	gtk_container_add(	GTK_CONTAINER(area->header_bar_revealer),
				area->header_bar );
	gtk_container_add(	GTK_CONTAINER(area->control_box_revealer),
				area->control_box );

	gtk_overlay_add_overlay(GTK_OVERLAY(area), area->control_box_revealer);
	gtk_overlay_add_overlay(GTK_OVERLAY(area), area->header_bar_revealer);
	gtk_container_add(GTK_CONTAINER(area), area->stack);
}

GtkWidget *
celluloid_video_area_new()
{
	return GTK_WIDGET(g_object_new(celluloid_video_area_get_type(), NULL));
}

void
celluloid_video_area_update_track_list(	CelluloidVideoArea *area,
					const GPtrArray *track_list )
{
	celluloid_header_bar_update_track_list
		(CELLULOID_HEADER_BAR(area->header_bar), track_list);
}

void
celluloid_video_area_update_disc_list(	CelluloidVideoArea *area,
					const GPtrArray *disc_list )
{
	celluloid_header_bar_update_disc_list
		(CELLULOID_HEADER_BAR(area->header_bar), disc_list);
}

void
celluloid_video_area_set_fullscreen_state(	CelluloidVideoArea *area,
						gboolean fullscreen )
{
	if(area->fullscreen != fullscreen)
	{
		area->fullscreen = fullscreen;

		gtk_widget_hide(area->header_bar_revealer);
		set_cursor_visible(area, !fullscreen);

		gtk_revealer_set_reveal_child
			(GTK_REVEALER(area->control_box_revealer), FALSE);
		gtk_revealer_set_reveal_child
			(GTK_REVEALER(area->header_bar_revealer), FALSE);

		celluloid_header_bar_set_fullscreen_state
			(CELLULOID_HEADER_BAR(area->header_bar), fullscreen);

		if(area->control_box)
		{
			celluloid_control_box_set_fullscreen_state
				(CELLULOID_CONTROL_BOX(area->control_box), fullscreen);
		}
	}
}

void
celluloid_video_area_set_control_box_visible(	CelluloidVideoArea *area,
						gboolean visible )
{
	gtk_widget_set_visible(area->control_box_revealer, visible);
}

void
celluloid_video_area_set_use_opengl(	CelluloidVideoArea *area,
					gboolean use_opengl )
{
	gtk_stack_set_visible_child
		(	GTK_STACK(area->stack),
			use_opengl?area->gl_area:area->draw_area );
}

void
celluloid_video_area_queue_render(CelluloidVideoArea *area)
{
	gtk_gl_area_queue_render(GTK_GL_AREA(area->gl_area));
}

GtkDrawingArea *
celluloid_video_area_get_draw_area(CelluloidVideoArea *area)
{
	return GTK_DRAWING_AREA(area->draw_area);
}

GtkGLArea *
celluloid_video_area_get_gl_area(CelluloidVideoArea *area)
{
	return GTK_GL_AREA(area->gl_area);
}

CelluloidHeaderBar *
celluloid_video_area_get_header_bar(CelluloidVideoArea *area)
{
	return CELLULOID_HEADER_BAR(area->header_bar);
}

CelluloidControlBox *
celluloid_video_area_get_control_box(CelluloidVideoArea *area)
{
	return CELLULOID_CONTROL_BOX(area->control_box);
}

gint64
celluloid_video_area_get_xid(CelluloidVideoArea *area)
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
