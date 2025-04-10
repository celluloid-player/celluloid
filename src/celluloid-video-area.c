/*
 * Copyright (c) 2016-2025 gnome-mpv
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
#include "celluloid-marshal.h"
#include "celluloid-common.h"
#include "celluloid-def.h"

#include <adwaita.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <math.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#endif

enum
{
	PROP_0,
	PROP_FULLSCREENED,
	N_PROPERTIES
};

struct _CelluloidVideoArea
{
	AdwBreakpointBin parent_instance;
	GtkWidget *toast_overlay;
	GtkWidget *stack;
	GtkWidget *gl_area;
	GtkWidget *graphics_offload;
	GtkWidget *toolbar_view;
	GtkWidget *initial_page;
	GtkWidget *control_box;
	GtkWidget *header_bar;
	GtkEventController *area_motion_controller;
	CelluloidVideoAreaStatus status;
	gint compact_threshold;
	guint32 last_motion_time;
	gdouble last_motion_x;
	gdouble last_motion_y;
	guint timeout_tag;
	gboolean fullscreened;
	gboolean fs_control_hover;
	gboolean use_floating_header_bar;
	gboolean use_floating_controls;
	AdwBreakpoint *wide;
	AdwBreakpoint *narrow;
	AdwBreakpoint *compacted;
};

struct _CelluloidVideoAreaClass
{
	AdwBreakpointBinClass parent_class;
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

static void
set_fullscreen_state(CelluloidVideoArea *area, gboolean fullscreen);

static void
set_cursor_visible(CelluloidVideoArea *area, gboolean visible);

static void
reveal_controls(CelluloidVideoArea *area);

static void
destroy_handler(GtkWidget *widget, gpointer data);

static gboolean
timeout_handler(gpointer data);

static void
motion_handler(	GtkEventControllerMotion *controller,
		gdouble x,
		gdouble y,
		gpointer data );

static void
pressed_handler(	GtkGestureClick *self,
			gint n_press,
			gdouble x,
			gdouble y,
			gpointer data );

static void
enter_handler(	GtkEventControllerMotion *controller,
		gdouble x,
		gdouble y,
		gpointer data );

static void
leave_handler(GtkEventControllerMotion *controller, gpointer data);

G_DEFINE_TYPE(CelluloidVideoArea, celluloid_video_area, ADW_TYPE_BREAKPOINT_BIN)

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidVideoArea *self = CELLULOID_VIDEO_AREA(object);

	switch(property_id)
	{
		case PROP_FULLSCREENED:
		self->fullscreened = g_value_get_boolean(value);
		set_fullscreen_state(self, self->fullscreened);
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
		case PROP_FULLSCREENED:
		g_value_set_boolean(value, self->fullscreened);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
set_fullscreen_state(CelluloidVideoArea *area, gboolean fullscreen)
{
	area->fs_control_hover = FALSE;

	set_cursor_visible(area, !fullscreen);

	celluloid_video_area_set_use_floating_header_bar
		(area, area->use_floating_controls || fullscreen);
	celluloid_video_area_set_control_box_floating
		(area, TRUE);

	adw_toolbar_view_set_reveal_top_bars
		(ADW_TOOLBAR_VIEW(area->toolbar_view), TRUE);
	adw_toolbar_view_set_reveal_bottom_bars
		(ADW_TOOLBAR_VIEW(area->toolbar_view), TRUE);
}

static void
reveal_controls(CelluloidVideoArea *area)
{
	GdkCursor *cursor = gdk_cursor_new_from_name("default", NULL);
	GdkSurface *surface = gtk_widget_get_surface(GTK_WIDGET(area));

	gdk_surface_set_cursor(surface, cursor);

	adw_toolbar_view_set_reveal_top_bars
		(ADW_TOOLBAR_VIEW(area->toolbar_view), TRUE);
	adw_toolbar_view_set_reveal_bottom_bars
		(ADW_TOOLBAR_VIEW(area->toolbar_view), TRUE);

	g_source_clear(&area->timeout_tag);
	area->timeout_tag =	g_timeout_add_seconds
				(	FS_CONTROL_HIDE_DELAY,
					timeout_handler,
					area );
}

static void
destroy_handler(GtkWidget *widget, gpointer data)
{
	g_source_clear(&CELLULOID_VIDEO_AREA(widget)->timeout_tag);
}

static void
set_cursor_visible(CelluloidVideoArea *area, gboolean visible)
{
	GdkSurface *surface = gtk_widget_get_surface(area);
	GdkCursor *cursor = NULL;

	const gboolean hovering =
		gtk_event_controller_motion_contains_pointer
		(GTK_EVENT_CONTROLLER_MOTION(area->area_motion_controller));

	if(visible || !hovering)
	{
		cursor = gdk_cursor_new_from_name("default", NULL);
	}
	else
	{
               GdkPixbuf *pixbuf =
                       gdk_pixbuf_new
                       (GDK_COLORSPACE_RGB, TRUE, 8, 1, 1);
               gdk_pixbuf_fill(pixbuf, 0x00000000);

               GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
               cursor = gdk_cursor_new_from_texture(texture, 0, 0, NULL);

               g_object_unref(texture);
               g_object_unref(pixbuf);
	}

	gdk_surface_set_cursor(surface, cursor);
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
	gboolean paused = FALSE;

	g_object_get(	header_bar,
			"open-button-active", &open_button_active,
			"menu-button-active", &menu_button_active,
			NULL );
	g_object_get(	control_box,
			"pause", &paused,
			NULL );

	if(control_box
	&& !area->fs_control_hover
	&& !celluloid_control_box_get_volume_popup_visible(control_box)
	&& !open_button_active
	&& !menu_button_active
	&& !paused)
	{
		GSettings *settings;
		gboolean always_autohide;

		settings = g_settings_new(CONFIG_ROOT);
		always_autohide =	g_settings_get_boolean
					(settings, "always-autohide-cursor");

		if(area->use_floating_header_bar)
		{
			adw_toolbar_view_set_reveal_top_bars
				(ADW_TOOLBAR_VIEW(area->toolbar_view), FALSE);
		}

		if(area->use_floating_controls)
		{
			adw_toolbar_view_set_reveal_bottom_bars
				(ADW_TOOLBAR_VIEW(area->toolbar_view), FALSE);
		}

		set_cursor_visible(area, !(always_autohide || area->fullscreened));
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

static void
motion_handler(	GtkEventControllerMotion *controller,
		gdouble x,
		gdouble y,
		gpointer data )
{
	GSettings *settings = g_settings_new(CONFIG_ROOT);
	GtkWidget *widget = GTK_WIDGET(data);
	CelluloidVideoArea *area = CELLULOID_VIDEO_AREA(data);
	const gint height = gtk_widget_get_height(widget);

	const gdouble unhide_speed =
		g_settings_get_double(settings, "controls-unhide-cursor-speed");
	const gdouble dead_zone =
		g_settings_get_double(settings, "controls-dead-zone-size");

	const guint32 time =	gtk_event_controller_get_current_event_time
				(GTK_EVENT_CONTROLLER(controller));
	const gdouble dist = sqrt(	pow(x - area->last_motion_x, 2) +
					pow(y - area->last_motion_y, 2) );
	const gdouble speed = dist / (time - area->last_motion_time);

	area->last_motion_time = time;
	area->last_motion_x = x;
	area->last_motion_y = y;

	if(	speed >= unhide_speed &&
		area->control_box &&
		ABS((2 * y - height) / height) > dead_zone )
	{
		reveal_controls(area);
	}

	g_object_unref(settings);
}

static void
pressed_handler(	GtkGestureClick *self,
			gint n_press,
			gdouble x,
			gdouble y,
			gpointer data )
{
	CelluloidVideoArea *area = CELLULOID_VIDEO_AREA(data);
	GSettings *settings = g_settings_new(CONFIG_ROOT);

	const gboolean hovering =
		area->fs_control_hover;
	const gboolean draggable =
		g_settings_get_boolean(settings, "draggable-video-area-enable");

	reveal_controls(area);

	if(draggable && !hovering)
	{
		GdkSurface *surface = gtk_widget_get_surface(GTK_WIDGET(data));
		GdkToplevel *toplevel = GDK_TOPLEVEL(surface);
		GdkDevice *device = gtk_gesture_get_device(GTK_GESTURE(self));
		GtkGestureSingle *single = GTK_GESTURE_SINGLE(self);
		gint button = (gint)gtk_gesture_single_get_button(single);

		gdk_toplevel_begin_move
			(toplevel, device, button, x, y, GDK_CURRENT_TIME);
	}
}

static gboolean
render_handler(GtkGLArea *gl_area, GdkGLContext *context, gpointer data)
{
	g_signal_emit_by_name(data, "render");

	return TRUE;
}

static void
resize_handler(GtkWidget *widget, gint width, gint height, gpointer data)
{
	g_signal_emit_by_name(data, "resize", width, height);
}

static void
enter_handler(	GtkEventControllerMotion *controller,
		gdouble x,
		gdouble y,
		gpointer data )
{
	CELLULOID_VIDEO_AREA(data)->fs_control_hover = TRUE;
}

static void
leave_handler(GtkEventControllerMotion *controller, gpointer data)
{
	CELLULOID_VIDEO_AREA(data)->fs_control_hover = FALSE;
}

static void
celluloid_video_area_class_init(CelluloidVideoAreaClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *wgt_class = GTK_WIDGET_CLASS(klass);
	GParamSpec *pspec = NULL;

	obj_class->set_property = set_property;
	obj_class->get_property = get_property;

	gtk_widget_class_set_css_name(wgt_class, "celluloid-video-area");

	pspec = g_param_spec_boolean
		(	"fullscreened",
			"Fullscreened",
			"Whether the video area is in fullscreen configuration",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_FULLSCREENED, pspec);

	g_signal_new(	"render",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
	g_signal_new(	"resize",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_gen_marshal_VOID__INT_INT,
			G_TYPE_NONE,
			2,
			G_TYPE_INT,
			G_TYPE_INT );
}

static void
celluloid_video_area_init(CelluloidVideoArea *area)
{
	GSettings *settings = g_settings_new(CONFIG_ROOT);

	area->toast_overlay = adw_toast_overlay_new();
	area->stack = gtk_stack_new();
	area->gl_area = gtk_gl_area_new();
	area->graphics_offload = gtk_graphics_offload_new(area->gl_area);
	area->toolbar_view = adw_toolbar_view_new();
	area->initial_page = adw_status_page_new();
	area->control_box = celluloid_control_box_new();
	area->header_bar = celluloid_header_bar_new();
	area->area_motion_controller = gtk_event_controller_motion_new();
	area->status = CELLULOID_VIDEO_AREA_STATUS_LOADING;
	area->compact_threshold = -1;
	area->last_motion_time = 0;
	area->last_motion_x = -1;
	area->last_motion_y = -1;
	area->timeout_tag = 0;
	area->fullscreened = FALSE;
	area->fs_control_hover = FALSE;
	area->use_floating_header_bar = FALSE;
	area->use_floating_controls = FALSE;

	const gboolean enable_graphics_offload =
		g_settings_get_boolean(settings, "graphics-offload-enable");
	gtk_graphics_offload_set_enabled
		(	GTK_GRAPHICS_OFFLOAD(area->graphics_offload),
			enable_graphics_offload );

	AdwBreakpointCondition *wide_condition =
		adw_breakpoint_condition_new_length
		(ADW_BREAKPOINT_CONDITION_MIN_WIDTH, 700, ADW_LENGTH_UNIT_SP);
	area->wide = adw_breakpoint_new(wide_condition);

	AdwBreakpointCondition *narrow_condition =
		adw_breakpoint_condition_new_length
		(ADW_BREAKPOINT_CONDITION_MAX_WIDTH, 700, ADW_LENGTH_UNIT_SP);
	area->narrow = adw_breakpoint_new(narrow_condition);

	AdwBreakpointCondition *compacted_condition =
		adw_breakpoint_condition_new_length
		(ADW_BREAKPOINT_CONDITION_MAX_WIDTH, 400, ADW_LENGTH_UNIT_SP);
	area->compacted = adw_breakpoint_new(compacted_condition);

	adw_toolbar_view_set_reveal_top_bars
		(	ADW_TOOLBAR_VIEW(area->toolbar_view),
			area->use_floating_header_bar );
	adw_toolbar_view_set_reveal_bottom_bars
		(	ADW_TOOLBAR_VIEW(area->toolbar_view),
			area->use_floating_controls );

	gtk_widget_set_visible(GTK_WIDGET(area->header_bar), FALSE);

	adw_breakpoint_bin_add_breakpoint
		(ADW_BREAKPOINT_BIN(area), area->wide);
	adw_breakpoint_bin_add_breakpoint
		(ADW_BREAKPOINT_BIN(area), area->narrow);
	adw_breakpoint_bin_add_breakpoint
		(ADW_BREAKPOINT_BIN(area), area->compacted);

	g_object_bind_property(	area, "fullscreened",
				area->control_box, "fullscreened",
				G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE );
	g_object_bind_property(	area, "fullscreened",
				area->header_bar, "fullscreened",
				G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE );

	GtkEventController *area_motion_controller =
		area->area_motion_controller;
	gtk_widget_add_controller
		(GTK_WIDGET(area), area_motion_controller);

	g_signal_connect(	area_motion_controller,
				"motion",
				G_CALLBACK(motion_handler),
				area );

	GtkEventController *area_click_gesture =
		GTK_EVENT_CONTROLLER(gtk_gesture_click_new());
	gtk_widget_add_controller
		(GTK_WIDGET(area), area_click_gesture);

	g_signal_connect(	area_click_gesture,
				"pressed",
				G_CALLBACK(pressed_handler),
				area );

	GtkEventController *control_box_motion_controller =
		gtk_event_controller_motion_new();
	gtk_widget_add_controller
		(GTK_WIDGET(area->control_box), control_box_motion_controller);

	g_signal_connect(	control_box_motion_controller,
				"enter",
				G_CALLBACK(enter_handler),
				area );
	g_signal_connect(	control_box_motion_controller,
				"leave",
				G_CALLBACK(leave_handler),
				area );

	GtkEventController *header_bar_motion_controller =
		gtk_event_controller_motion_new();
	gtk_widget_add_controller
		(GTK_WIDGET(area->header_bar), header_bar_motion_controller);

	g_signal_connect(	header_bar_motion_controller,
				"enter",
				G_CALLBACK(enter_handler),
				area );
	g_signal_connect(	header_bar_motion_controller,
				"leave",
				G_CALLBACK(leave_handler),
				area );
	g_signal_connect(	area,
				"destroy",
				G_CALLBACK(destroy_handler),
				area );
	g_signal_connect(	area->gl_area,
				"render",
				G_CALLBACK(render_handler),
				area );
	g_signal_connect(	area->gl_area,
				"resize",
				G_CALLBACK(resize_handler),
				area );
	adw_status_page_set_icon_name
		(	ADW_STATUS_PAGE(area->initial_page),
			"io.github.celluloid_player.Celluloid" );

	gtk_stack_add_child(GTK_STACK(area->stack), area->graphics_offload);
	gtk_stack_add_child(GTK_STACK(area->stack), area->initial_page);

	celluloid_video_area_set_status
		(area, CELLULOID_VIDEO_AREA_STATUS_LOADING);

	gtk_graphics_offload_set_black_background(GTK_GRAPHICS_OFFLOAD(area->graphics_offload), TRUE);

	gtk_widget_set_hexpand(area->stack, TRUE);

	adw_toast_overlay_set_child
		(ADW_TOAST_OVERLAY(area->toast_overlay), area->stack);

	adw_toolbar_view_add_top_bar
		(ADW_TOOLBAR_VIEW(area->toolbar_view), area->header_bar);
	adw_toolbar_view_add_bottom_bar
		(ADW_TOOLBAR_VIEW(area->toolbar_view), area->control_box);
	adw_toolbar_view_set_content
		(ADW_TOOLBAR_VIEW(area->toolbar_view), area->toast_overlay);
	adw_breakpoint_bin_set_child
		(ADW_BREAKPOINT_BIN(area), area->toolbar_view);

	adw_toolbar_view_set_extend_content_to_top_edge
		(	ADW_TOOLBAR_VIEW(area->toolbar_view),
			area->use_floating_header_bar );
	adw_toolbar_view_set_extend_content_to_bottom_edge
		(	ADW_TOOLBAR_VIEW(area->toolbar_view),
			area->use_floating_controls );

	celluloid_header_bar_set_floating
		(	CELLULOID_HEADER_BAR(area->header_bar),
			area->use_floating_header_bar || area->fullscreened);
	celluloid_control_box_set_floating
		(	CELLULOID_CONTROL_BOX(area->control_box),
			area->use_floating_controls || area->fullscreened);

	adw_toolbar_view_set_reveal_top_bars
		(ADW_TOOLBAR_VIEW(area->toolbar_view), TRUE);
	adw_toolbar_view_set_reveal_bottom_bars
		(ADW_TOOLBAR_VIEW(area->toolbar_view), TRUE);

	//TODO: Reduce spacing for headerbar when in compact mode
	adw_breakpoint_add_setters
		(	area->compacted,
			G_OBJECT (area->control_box), "compact", TRUE,
			NULL );
	adw_breakpoint_add_setters
		(	area->narrow,
			G_OBJECT (area->control_box), "narrow", TRUE,
			NULL );

	gtk_widget_set_size_request (GTK_WIDGET(area), 200, 200);
	gtk_widget_set_size_request (GTK_WIDGET(area->toolbar_view), 200, 200);
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
celluloid_video_area_show_toast_message(	CelluloidVideoArea *area,
						const gchar *msg)
{
	AdwToast *toast = adw_toast_new(msg);
	AdwToastOverlay *toast_overlay = ADW_TOAST_OVERLAY(area->toast_overlay);

	adw_toast_overlay_add_toast(toast_overlay, toast);
}

void
celluloid_video_area_set_reveal_control_box(	CelluloidVideoArea *area,
						gboolean reveal )
{
	adw_toolbar_view_set_reveal_bottom_bars
		(ADW_TOOLBAR_VIEW(area->toolbar_view), reveal);

	g_source_clear(&area->timeout_tag);

	if(reveal)
	{
		area->timeout_tag =
			g_timeout_add_seconds
			(	FS_CONTROL_HIDE_DELAY,
				timeout_handler,
				area );
	}
}

void
celluloid_video_area_set_control_box_visible(	CelluloidVideoArea *area,
						gboolean visible )
{
	gtk_widget_set_visible(area->control_box, visible);
}

void
celluloid_video_area_set_status(	CelluloidVideoArea *area,
					CelluloidVideoAreaStatus status )
{
	switch(status)
	{
		case CELLULOID_VIDEO_AREA_STATUS_LOADING:
		adw_status_page_set_title
			(	ADW_STATUS_PAGE(area->initial_page),
				_("Loading…") );
		adw_status_page_set_description
			(	ADW_STATUS_PAGE(area->initial_page),
				NULL );
		gtk_stack_set_visible_child
			(GTK_STACK(area->stack), area->initial_page);
		break;

		case CELLULOID_VIDEO_AREA_STATUS_IDLE:
		adw_status_page_set_title
			(	ADW_STATUS_PAGE(area->initial_page),
				_("Welcome") );
		adw_status_page_set_description
			(	ADW_STATUS_PAGE(area->initial_page),
				_("Click the <b>Open</b> button or drag and drop videos here") );
		gtk_stack_set_visible_child
			(GTK_STACK(area->stack), area->initial_page);
		break;

		case CELLULOID_VIDEO_AREA_STATUS_PLAYING:
		gtk_stack_set_visible_child
			(GTK_STACK(area->stack), area->graphics_offload);
		break;
	}



	area->status = status;
}

gboolean
celluloid_video_area_get_control_box_visible(CelluloidVideoArea *area)
{
	return gtk_widget_get_visible(area->control_box);
}

void
celluloid_video_area_set_use_floating_header_bar(	CelluloidVideoArea *area,
							gboolean floating )
{
	area->use_floating_header_bar = floating;

	celluloid_header_bar_set_floating
		(CELLULOID_HEADER_BAR(area->header_bar), floating);
	adw_toolbar_view_set_extend_content_to_top_edge
		(ADW_TOOLBAR_VIEW(area->toolbar_view), floating);

	if(floating)
	{
		adw_toolbar_view_set_top_bar_style(ADW_TOOLBAR_VIEW(area->toolbar_view), ADW_TOOLBAR_FLAT);
	}
	else
	{
		adw_toolbar_view_set_top_bar_style(ADW_TOOLBAR_VIEW(area->toolbar_view), ADW_TOOLBAR_RAISED_BORDER);
	}
}

void
celluloid_video_area_set_control_box_floating(	CelluloidVideoArea *area,
						gboolean floating )
{
	area->use_floating_controls = floating;

	celluloid_control_box_set_floating
		(CELLULOID_CONTROL_BOX(area->control_box), floating);

	adw_toolbar_view_set_extend_content_to_bottom_edge
		(ADW_TOOLBAR_VIEW(area->toolbar_view), floating);

	if(floating)
	{
		adw_toolbar_view_set_bottom_bar_style(ADW_TOOLBAR_VIEW(area->toolbar_view), ADW_TOOLBAR_FLAT);
	}
	else
	{
		adw_toolbar_view_set_bottom_bar_style(ADW_TOOLBAR_VIEW(area->toolbar_view), ADW_TOOLBAR_RAISED_BORDER);
	}
}

void
celluloid_video_area_queue_render(CelluloidVideoArea *area)
{
	gtk_gl_area_queue_render(GTK_GL_AREA(area->gl_area));
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
		GdkSurface *surface = NULL;

		if(parent && !gtk_widget_get_realized(area->gl_area))
		{
			gtk_widget_realize(area->gl_area);
		}

		surface = gtk_widget_get_surface(area);

		if(!surface)
		{
			g_critical("Failed to get XID of video area");
		}

		return surface?(gint64)gdk_x11_surface_get_xid(surface):-1;
	}
#endif

	return -1;
}

void
celluloid_video_area_get_offset(	CelluloidVideoArea *area,
					gint *width,
					gint *height )
{
	const gint area_width =
		gtk_widget_get_width(GTK_WIDGET(area));
	const gint area_height =
		gtk_widget_get_height(GTK_WIDGET(area));
	const gint stack_width =
		gtk_widget_get_width(GTK_WIDGET(area->stack));
	const gint stack_height =
		gtk_widget_get_height(GTK_WIDGET(area->stack));

	*width = area_width - stack_width;
	*height = area_height - stack_height;
}
