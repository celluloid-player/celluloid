/*
 * Copyright (c) 2014-2023 gnome-mpv
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
#include <glib/gi18n.h>

#include "celluloid-control-box.h"
#include "celluloid-seek-bar.h"

enum
{
	PROP_0,
	PROP_SKIP_ENABLED,
	PROP_DURATION,
	PROP_ENABLED,
	PROP_COMPACT,
	PROP_FULLSCREENED,
	PROP_PAUSE,
	PROP_SHOW_FULLSCREEN_BUTTON,
	PROP_TIME_POSITION,
	PROP_LOOP,
	PROP_SHUFFLE,
	PROP_VOLUME,
	PROP_VOLUME_MAX,
	PROP_VOLUME_POPUP_VISIBLE,
	PROP_CHAPTER_LIST,
	N_PROPERTIES
};

struct _CelluloidControlBox
{
	GtkBox parent_instance;

	GtkGesture *click_gesture;

	GtkWidget *inner_box;
	GtkWidget *play_button;
	GtkWidget *stop_button;
	GtkWidget *forward_button;
	GtkWidget *rewind_button;
	GtkWidget *next_button;
	GtkWidget *previous_button;
	GtkWidget *loop_button;
	GtkWidget *shuffle_button;
	GtkWidget *volume_button;
	GtkWidget *playlist_button;
	GtkWidget *fullscreen_button;
	GtkWidget *seek_bar;
	GtkWidget *secondary_seek_bar;
	gboolean skip_enabled;
	gdouble duration;
	gboolean enabled;
	gboolean compact;
	gboolean fullscreened;
	gboolean pause;
	gboolean show_fullscreen_button;
	gdouble time_position;
	gboolean loop;
	gboolean shuffle;
	gdouble volume;
	gdouble volume_max;
	gboolean volume_popup_visible;
	GPtrArray *chapter_list;
};

struct _CelluloidControlBoxClass
{
	GtkBoxClass parent_class;
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
css_changed(GtkWidget *self, GtkCssStyleChange *change);

static gboolean
gtk_to_mpv_volume(	GBinding *binding,
			const GValue *from_value,
			GValue *to_value,
			gpointer data );

static gboolean
mpv_to_gtk_volume(	GBinding *binding,
			const GValue *from_value,
			GValue *to_value,
			gpointer data );

static void
seek_handler(CelluloidSeekBar *seek_bar, gdouble value, gpointer data);

static void
volume_changed_handler(GtkVolumeButton *button, gdouble value, gpointer data);

static gboolean
button_pressed_handler(	GtkEventControllerKey *controller,
			guint keyval,
			guint keycode,
			GdkModifierType state,
			gpointer data );

static void
simple_signal_handler(GtkWidget *widget, gpointer data);

static void
set_enabled(CelluloidControlBox *box, gboolean enabled);

static void
set_skip_enabled(CelluloidControlBox *box, gboolean enabled);

static void
set_playing_state(CelluloidControlBox *box, gboolean playing);

static void
set_compact(CelluloidControlBox *box, gboolean compact);

static void
set_fullscreen_state(CelluloidControlBox *box, gboolean fullscreen);

static void
init_button(	GtkWidget *button,
		const gchar *icon_name,
		const gchar *tooltip_text );

G_DEFINE_TYPE(CelluloidControlBox, celluloid_control_box, GTK_TYPE_BOX)

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidControlBox *self = CELLULOID_CONTROL_BOX(object);

	switch(property_id)
	{
		case PROP_SKIP_ENABLED:
		self->skip_enabled = g_value_get_boolean(value);
		set_skip_enabled(self, self->skip_enabled);
		break;

		case PROP_DURATION:
		self->duration = g_value_get_double(value);
		break;

		case PROP_ENABLED:
		self->enabled = g_value_get_boolean(value);
		set_enabled(self, self->enabled);
		break;

		case PROP_COMPACT:
		self->compact = g_value_get_boolean(value);
		set_compact(self, self->compact);
		break;

		case PROP_FULLSCREENED:
		self->fullscreened = g_value_get_boolean(value);
		set_fullscreen_state(self, self->fullscreened);
		break;

		case PROP_PAUSE:
		self->pause = g_value_get_boolean(value);
		set_playing_state(self, !self->pause);
		break;

		case PROP_SHOW_FULLSCREEN_BUTTON:
		self->show_fullscreen_button = g_value_get_boolean(value);
		gtk_widget_set_visible
			(	self->fullscreen_button,
				self->show_fullscreen_button );
		break;

		case PROP_TIME_POSITION:
		self->time_position = g_value_get_double(value);

		celluloid_seek_bar_set_pos
			(	CELLULOID_SEEK_BAR(self->seek_bar),
				self->time_position );
		celluloid_seek_bar_set_pos
			(	CELLULOID_SEEK_BAR(self->secondary_seek_bar),
				self->time_position );
		break;

		case PROP_LOOP:
		self->loop = g_value_get_boolean(value);
		break;

		case PROP_SHUFFLE:
		self->shuffle = g_value_get_boolean(value);
		break;

		case PROP_VOLUME:
		self->volume = g_value_get_double(value);
		break;

		case PROP_VOLUME_MAX:
		self->volume_max = g_value_get_double(value);
		break;

		case PROP_VOLUME_POPUP_VISIBLE:
		self->volume_popup_visible = g_value_get_boolean(value);

		if(	self->volume_popup_visible &&
			gtk_widget_is_visible(GTK_WIDGET(self)) )
		{
			g_signal_emit_by_name(self->volume_button, "popup");
		}
		else if(!self->volume_popup_visible)
		{
			g_signal_emit_by_name(self->volume_button, "popdown");
		}
		break;

		case PROP_CHAPTER_LIST:
		self->chapter_list = g_value_get_pointer(value);
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
	CelluloidControlBox *self = CELLULOID_CONTROL_BOX(object);

	switch(property_id)
	{
		case PROP_SKIP_ENABLED:
		g_value_set_boolean(value, self->skip_enabled);
		break;

		case PROP_DURATION:
		g_value_set_double(value, self->duration);
		break;

		case PROP_ENABLED:
		g_value_set_boolean(value, self->enabled);
		break;

		case PROP_COMPACT:
		g_value_set_boolean(value, self->compact);
		break;

		case PROP_FULLSCREENED:
		g_value_set_boolean(value, self->fullscreened);
		break;

		case PROP_PAUSE:
		g_value_set_boolean(value, self->pause);
		break;

		case PROP_SHOW_FULLSCREEN_BUTTON:
		g_value_set_boolean(value, self->show_fullscreen_button);
		break;

		case PROP_TIME_POSITION:
		g_value_set_double(value, self->time_position);
		break;

		case PROP_LOOP:
		g_value_set_boolean(value, self->loop);
		break;

		case PROP_SHUFFLE:
		g_value_set_boolean(value, self->shuffle);
		break;

		case PROP_VOLUME:
		g_value_set_double(value, self->volume);
		break;

		case PROP_VOLUME_MAX:
		g_value_set_double(value, self->volume_max);
		break;

		case PROP_VOLUME_POPUP_VISIBLE:
		g_value_set_boolean(value, self->volume_popup_visible);
		break;

		case PROP_CHAPTER_LIST:
		g_value_set_pointer(value, self->chapter_list);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
css_changed(GtkWidget *self, GtkCssStyleChange *change)
{
	GTK_WIDGET_CLASS(celluloid_control_box_parent_class)
		->css_changed(self, change);

	CelluloidControlBox *box = CELLULOID_CONTROL_BOX(self);
	GtkStyleContext *ctx = gtk_widget_get_style_context(self);
	GtkBorder padding = {0};

	gtk_style_context_get_padding(ctx, &padding);
	g_object_set(box->seek_bar, "popover-y-offset", -padding.top, NULL);
	g_object_set(box->secondary_seek_bar, "popover-y-offset", -padding.top, NULL);
}

static gboolean
gtk_to_mpv_volume(	GBinding *binding,
			const GValue *from_value,
			GValue *to_value,
			gpointer data )
{
	gdouble from = g_value_get_double(from_value);
	g_value_set_double(to_value, from*100);

	return TRUE;
}

static gboolean
mpv_to_gtk_volume(	GBinding *binding,
			const GValue *from_value,
			GValue *to_value,
			gpointer data )
{
	gdouble from = g_value_get_double(from_value);
	g_value_set_double(to_value, from/100.0);

	return TRUE;
}

static void
seek_handler(CelluloidSeekBar *seek_bar, gdouble value, gpointer data)
{
	g_signal_emit_by_name(data, "seek", value);
}

static void
volume_changed_handler(GtkVolumeButton *button, gdouble value, gpointer data)
{
	g_signal_emit_by_name(data, "volume-changed", value*100.0);
}

static gboolean
button_pressed_handler(	GtkEventControllerKey *controller,
			guint keyval,
			guint keycode,
			GdkModifierType state,
			gpointer data )
{
	return TRUE;
}

static void
simple_signal_handler(GtkWidget *widget, gpointer data)
{
	CelluloidControlBox *box = data;
	gint i;

	const struct
	{
		const GtkWidget *widget;
		const gchar *name;
		const gchar *button;
	}
	signal_map[]
		= {	{box->play_button, "button-clicked", "play"},
			{box->stop_button, "button-clicked", "stop"},
			{box->forward_button, "button-clicked", "forward"},
			{box->rewind_button, "button-clicked", "rewind"},
			{box->previous_button, "button-clicked", "previous"},
			{box->next_button, "button-clicked", "next"},
			{box->playlist_button, "button-clicked", "playlist"},
			{box->fullscreen_button, "button-clicked", "fullscreen"},
			{NULL, NULL, NULL} };

	for(i = 0; signal_map[i].name && signal_map[i].widget != widget; i++);

	if(signal_map[i].name)
	{
		g_signal_emit_by_name
			(data, signal_map[i].name, signal_map[i].button);
	}
}

static void
set_enabled(CelluloidControlBox *box, gboolean enabled)
{
	gtk_widget_set_sensitive(box->previous_button, enabled);
	gtk_widget_set_sensitive(box->rewind_button, enabled);
	gtk_widget_set_sensitive(box->stop_button, enabled);
	gtk_widget_set_sensitive(box->play_button, enabled);
	gtk_widget_set_sensitive(box->forward_button, enabled);
	gtk_widget_set_sensitive(box->next_button, enabled);
}

static void
set_skip_enabled(CelluloidControlBox *box, gboolean enabled)
{
	if(enabled)
	{
		gtk_widget_set_visible(box->previous_button, TRUE);
		gtk_widget_set_visible(box->next_button, TRUE);
	}
	else
	{
		gtk_widget_set_visible(box->previous_button, FALSE);
		gtk_widget_set_visible(box->next_button, FALSE);
	}
}

static void
set_playing_state(CelluloidControlBox *box, gboolean playing)
{
	const gchar *tooltip = playing ? _("Pause") : _("Play");
	const gchar *icon_name =
		playing ?
		"media-playback-pause-symbolic" :
		"media-playback-start-symbolic";

	gtk_widget_set_tooltip_text(box->play_button, tooltip);
	gtk_button_set_icon_name(GTK_BUTTON(box->play_button), icon_name);
}

static void
set_compact(CelluloidControlBox *box, gboolean compact)
{
	if(box->compact)
	{
		gtk_widget_set_halign(box->inner_box, GTK_ALIGN_CENTER);
		gtk_widget_set_margin_start(box->loop_button, 12);
		gtk_widget_set_visible(box->seek_bar, FALSE);
		gtk_widget_set_visible(box->secondary_seek_bar, TRUE);
	}
	else
	{
		gtk_widget_set_halign(box->inner_box, GTK_ALIGN_FILL);
		gtk_widget_set_margin_start(box->loop_button, 0);
		gtk_widget_set_visible(box->seek_bar, TRUE);
		gtk_widget_set_visible(box->secondary_seek_bar, FALSE);
	}
}

static void
set_fullscreen_state(CelluloidControlBox *box, gboolean fullscreen)
{
	const gchar *icon_name =
		fullscreen ?
		"view-restore-symbolic" :
		"view-fullscreen-symbolic";

	gtk_button_set_icon_name
		(GTK_BUTTON(box->fullscreen_button), icon_name);
}

static void
init_button(	GtkWidget *button,
		const gchar *icon_name,
		const gchar *tooltip_text )
{
	gtk_button_set_icon_name(GTK_BUTTON(button), icon_name);

	g_object_set(button, "has-frame", FALSE, NULL);
	gtk_widget_set_tooltip_text(button, tooltip_text);
	gtk_widget_set_can_focus(button, FALSE);
}

static void
celluloid_control_box_class_init(CelluloidControlBoxClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
	GParamSpec *pspec = NULL;

	object_class->set_property = set_property;
	object_class->get_property = get_property;
	widget_class->css_changed = css_changed;

	pspec = g_param_spec_boolean
		(	"skip-enabled",
			"Skip enabled",
			"Whether the skip buttons are shown",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property
		(object_class, PROP_SKIP_ENABLED, pspec);

	pspec = g_param_spec_double
		(	"duration",
			"Duration",
			"Duration of the file",
			0.0,
			G_MAXDOUBLE,
			0.0,
			G_PARAM_READWRITE );
	g_object_class_install_property
		(object_class, PROP_DURATION, pspec);

	pspec = g_param_spec_boolean
		(	"enabled",
			"Enabled",
			"Whether the controls are enabled",
			TRUE,
			G_PARAM_READWRITE );
	g_object_class_install_property
		(object_class, PROP_ENABLED, pspec);

	pspec = g_param_spec_boolean
		(	"compact",
			"Compact",
			"Whether or not compact mode is enabled",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_COMPACT, pspec);

	pspec = g_param_spec_boolean
		(	"fullscreened",
			"Fullscreened",
			"Whether the control box is in fullscreen configuration",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_FULLSCREENED, pspec);

	pspec = g_param_spec_boolean
		(	"pause",
			"Pause",
			"Whether there is a file playing",
			TRUE,
			G_PARAM_READWRITE );
	g_object_class_install_property
		(object_class, PROP_PAUSE, pspec);

	pspec = g_param_spec_boolean
		(	"show-fullscreen-button",
			"Show fullscreen button",
			"Whether to show fullscreen button when applicable",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property
		(object_class, PROP_SHOW_FULLSCREEN_BUTTON, pspec);

	pspec = g_param_spec_double
		(	"time-position",
			"Time position",
			"The current timr position in the current file",
			0.0,
			G_MAXDOUBLE,
			0.0,
			G_PARAM_READWRITE );
	g_object_class_install_property
		(object_class, PROP_TIME_POSITION, pspec);

	pspec = g_param_spec_boolean
		(	"loop",
			"Loop",
			"Whether or not the loop button is active",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property
		(object_class, PROP_LOOP, pspec);

	pspec = g_param_spec_boolean
		(	"shuffle",
			"Shuffle",
			"Whether or not the the shuffle button is active",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property
		(object_class, PROP_SHUFFLE, pspec);

	pspec = g_param_spec_double
		(	"volume",
			"Volume",
			"The value of the volume button",
			0.0,
			G_MAXDOUBLE,
			0.0,
			G_PARAM_READWRITE );
	g_object_class_install_property
		(object_class, PROP_VOLUME, pspec);

	pspec = g_param_spec_double
		(	"volume-max",
			"Volume Max",
			"The maximum amplification level",
			0.0,
			G_MAXDOUBLE,
			100.0,
			G_PARAM_READWRITE );
	g_object_class_install_property
		(object_class, PROP_VOLUME_MAX, pspec);

	pspec = g_param_spec_pointer
		(	"chapter-list",
			"Chapter list",
			"The list of chapters for the current file",
			G_PARAM_READWRITE );
	g_object_class_install_property
		(object_class, PROP_CHAPTER_LIST, pspec);

	pspec = g_param_spec_boolean
		(	"volume-popup-visible",
			"Volume popup visible",
			"Whether or not the volume popup is visible",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property
		(object_class, PROP_VOLUME_POPUP_VISIBLE, pspec);

	g_signal_new(	"button-clicked",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING );
	g_signal_new(	"seek",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__DOUBLE,
			G_TYPE_NONE,
			1,
			G_TYPE_DOUBLE );
	g_signal_new(	"volume-changed",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__DOUBLE,
			G_TYPE_NONE,
			1,
			G_TYPE_DOUBLE );
}

static void
celluloid_control_box_init(CelluloidControlBox *box)
{
	GtkWidget *popup = NULL;
	GtkAdjustment *adjustment = NULL;
	const gchar *volume_icons[] =
		{	"audio-volume-muted",
			"audio-volume-high",
			"audio-volume-low",
			"audio-volume-medium",
			NULL };

	box->click_gesture = gtk_gesture_click_new();

	box->inner_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	box->play_button = gtk_button_new();
	box->stop_button = gtk_button_new();
	box->forward_button = gtk_button_new();
	box->rewind_button = gtk_button_new();
	box->next_button = gtk_button_new();
	box->previous_button = gtk_button_new();
	box->playlist_button = gtk_button_new();
	box->fullscreen_button = gtk_button_new();
	box->loop_button = gtk_toggle_button_new();
	box->shuffle_button = gtk_toggle_button_new();
	box->volume_button = gtk_scale_button_new(0.0, 1.0, 0.02, volume_icons);
	box->seek_bar = celluloid_seek_bar_new();
	box->secondary_seek_bar = celluloid_seek_bar_new();
	box->skip_enabled = FALSE;
	box->duration = 0.0;
	box->enabled = TRUE;
	box->compact = FALSE;
	box->fullscreened = FALSE;
	box->pause = TRUE;
	box->show_fullscreen_button = FALSE;
	box->time_position = 0.0;
	box->loop = FALSE;
	box->shuffle = FALSE;
	box->volume = 0.0;
	box->volume_max = 100.0;
	box->volume_popup_visible = FALSE;
	box->chapter_list = NULL;

	init_button(	box->play_button,
			"media-playback-start-symbolic",
			_("Play") );
	init_button(	box->stop_button,
			"media-playback-stop-symbolic",
			_("Stop") );
	init_button(	box->forward_button,
			"media-seek-forward-symbolic",
			_("Forward") );
	init_button(	box->rewind_button,
			"media-seek-backward-symbolic",
			_("Rewind") );
	init_button(	box->next_button,
			"media-skip-forward-symbolic",
			_("Next Chapter") );
	init_button(	box->previous_button,
			"media-skip-backward-symbolic",
			_("Previous Chapter") );
	init_button(	box->loop_button,
			"media-playlist-repeat-symbolic",
			_("Loop Playlist") );
	init_button(	box->shuffle_button,
			"media-playlist-shuffle-symbolic",
			_("Shuffle Playlist") );
	init_button(	box->playlist_button,
			"sidebar-show-right-symbolic",
			_("Toggle Playlist") );
	init_button(	box->fullscreen_button,
			"view-fullscreen-symbolic",
			_("Toggle Fullscreen") );

	gtk_widget_add_controller
		(GTK_WIDGET(box), GTK_EVENT_CONTROLLER(box->click_gesture));

	gtk_widget_add_css_class(GTK_WIDGET(box), "toolbar");

	gtk_widget_set_margin_end(box->seek_bar, 6);
	gtk_widget_set_margin_end(box->secondary_seek_bar, 6);

	gtk_widget_set_can_focus(box->volume_button, FALSE);
	gtk_widget_set_can_focus(box->seek_bar, FALSE);
	gtk_widget_set_can_focus(box->secondary_seek_bar, FALSE);

	gtk_widget_set_hexpand(box->seek_bar, TRUE);
	gtk_widget_set_hexpand(box->secondary_seek_bar, TRUE);

	g_object_set(box->secondary_seek_bar, "show-label", FALSE, NULL);

	set_compact(box, box->compact);

	g_object_set(box, "orientation", GTK_ORIENTATION_VERTICAL, NULL);

	gchar css_data[] = "box {border-spacing: 0px;}";
	GtkCssProvider *css = gtk_css_provider_new();
	gtk_css_provider_load_from_data(css, css_data, -1);

	gtk_style_context_add_provider_for_display
		(	gtk_widget_get_display(GTK_WIDGET(box)),
			GTK_STYLE_PROVIDER(css),
			GTK_STYLE_PROVIDER_PRIORITY_USER );

	box->inner_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	gtk_box_append(GTK_BOX(box), box->secondary_seek_bar);
	gtk_box_append(GTK_BOX(box), box->inner_box);

	gtk_box_append(GTK_BOX(box->inner_box), box->previous_button);
	gtk_box_append(GTK_BOX(box->inner_box), box->rewind_button);
	gtk_box_append(GTK_BOX(box->inner_box), box->play_button);
	gtk_box_append(GTK_BOX(box->inner_box), box->stop_button);
	gtk_box_append(GTK_BOX(box->inner_box), box->forward_button);
	gtk_box_append(GTK_BOX(box->inner_box), box->next_button);
	gtk_box_append(GTK_BOX(box->inner_box), box->seek_bar);
	gtk_box_append(GTK_BOX(box->inner_box), box->loop_button);
	gtk_box_append(GTK_BOX(box->inner_box), box->shuffle_button);
	gtk_box_append(GTK_BOX(box->inner_box), box->volume_button);
	gtk_box_append(GTK_BOX(box->inner_box), box->playlist_button);
	gtk_box_append(GTK_BOX(box->inner_box), box->fullscreen_button);

	popup =	gtk_scale_button_get_popup
		(GTK_SCALE_BUTTON(box->volume_button));
	adjustment =	gtk_scale_button_get_adjustment
			(GTK_SCALE_BUTTON(box->volume_button));

	gtk_popover_set_position(GTK_POPOVER(popup), GTK_POS_TOP);

	g_object_bind_property(	popup, "visible",
				box, "volume-popup-visible",
				G_BINDING_DEFAULT );
	g_object_bind_property(	box, "duration",
				box->seek_bar, "duration",
				G_BINDING_DEFAULT );
	g_object_bind_property(	box, "chapter-list",
				box->seek_bar, "chapter-list",
				G_BINDING_DEFAULT );
	g_object_bind_property(	box, "pause",
				box->seek_bar, "pause",
				G_BINDING_DEFAULT );
	g_object_bind_property(	box, "enabled",
				box->seek_bar, "enabled",
				G_BINDING_DEFAULT );
	g_object_bind_property(	box, "chapter-list",
				box->secondary_seek_bar, "chapter-list",
				G_BINDING_DEFAULT );
	g_object_bind_property(	box, "duration",
				box->secondary_seek_bar, "duration",
				G_BINDING_DEFAULT );
	g_object_bind_property(	box, "pause",
				box->secondary_seek_bar, "pause",
				G_BINDING_DEFAULT );
	g_object_bind_property(	box, "enabled",
				box->secondary_seek_bar, "enabled",
				G_BINDING_DEFAULT );
	g_object_bind_property(	box, "loop",
				box->loop_button, "active",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property(	box, "shuffle",
				box->shuffle_button, "active",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property_full(	box->volume_button, "value",
					box, "volume",
					G_BINDING_BIDIRECTIONAL,
					gtk_to_mpv_volume,
					mpv_to_gtk_volume,
					NULL,
					NULL );
	g_object_bind_property_full(	box, "volume-max",
					adjustment, "upper",
					G_BINDING_DEFAULT|G_BINDING_SYNC_CREATE,
					mpv_to_gtk_volume,
					gtk_to_mpv_volume,
					NULL,
					NULL );

	g_signal_connect(	box->click_gesture,
				"pressed",
				G_CALLBACK(button_pressed_handler),
				NULL );

	g_signal_connect(	box->play_button,
				"clicked",
				G_CALLBACK(simple_signal_handler),
				box );
	g_signal_connect(	box->stop_button,
				"clicked",
				G_CALLBACK(simple_signal_handler),
				box );
	g_signal_connect(	box->seek_bar,
				"seek",
				G_CALLBACK(seek_handler),
				box );
	g_signal_connect(	box->secondary_seek_bar,
				"seek",
				G_CALLBACK(seek_handler),
				box );
	g_signal_connect(	box->forward_button,
				"clicked",
				G_CALLBACK(simple_signal_handler),
				box );
	g_signal_connect(	box->rewind_button,
				"clicked",
				G_CALLBACK(simple_signal_handler),
				box );
	g_signal_connect(	box->previous_button,
				"clicked",
				G_CALLBACK(simple_signal_handler),
				box );
	g_signal_connect(	box->next_button,
				"clicked",
				G_CALLBACK(simple_signal_handler),
				box );
	g_signal_connect(	box->playlist_button,
				"clicked",
				G_CALLBACK(simple_signal_handler),
				box );
	g_signal_connect(	box->fullscreen_button,
				"clicked",
				G_CALLBACK(simple_signal_handler),
				box );
	g_signal_connect(	box->volume_button,
				"value-changed",
				G_CALLBACK(volume_changed_handler),
				box );
}

GtkWidget *
celluloid_control_box_new(void)
{
	return GTK_WIDGET(g_object_new(celluloid_control_box_get_type(), NULL));
}

void
celluloid_control_box_set_seek_bar_pos(CelluloidControlBox *box, gdouble pos)
{
	celluloid_seek_bar_set_pos(CELLULOID_SEEK_BAR(box->seek_bar), pos);
	celluloid_seek_bar_set_pos(CELLULOID_SEEK_BAR(box->secondary_seek_bar), pos);
}

void
celluloid_control_box_set_seek_bar_duration(	CelluloidControlBox *box,
						gint duration )
{
	celluloid_seek_bar_set_duration(CELLULOID_SEEK_BAR(box->seek_bar), duration);
	celluloid_seek_bar_set_duration(CELLULOID_SEEK_BAR(box->secondary_seek_bar), duration);
}

void
celluloid_control_box_set_volume(CelluloidControlBox *box, gdouble volume)
{
	g_signal_handlers_block_by_func(box, volume_changed_handler, box);

	gtk_scale_button_set_value
		(GTK_SCALE_BUTTON(box->volume_button), volume/100.0);

	g_signal_handlers_unblock_by_func(box, volume_changed_handler, box);
}

gdouble
celluloid_control_box_get_volume(CelluloidControlBox *box)
{
	return gtk_scale_button_get_value(GTK_SCALE_BUTTON(box->volume_button));
}

gboolean
celluloid_control_box_get_volume_popup_visible(CelluloidControlBox *box)
{
	return box->volume_popup_visible;
}

void
celluloid_control_box_set_floating(CelluloidControlBox *box, gboolean floating)
{
	if(floating)
	{
		gtk_widget_add_css_class(GTK_WIDGET(box), "osd");

		gtk_widget_set_margin_start(GTK_WIDGET(box), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(box), 12);
		gtk_widget_set_margin_top(GTK_WIDGET(box), 12);
		gtk_widget_set_margin_bottom(GTK_WIDGET(box), 12);
	}
	else
	{
		gtk_widget_remove_css_class(GTK_WIDGET(box), "osd");

		gtk_widget_set_margin_start(GTK_WIDGET(box), 0);
		gtk_widget_set_margin_end(GTK_WIDGET(box), 0);
		gtk_widget_set_margin_top(GTK_WIDGET(box), 0);
		gtk_widget_set_margin_bottom(GTK_WIDGET(box), 0);
	}
}

void
celluloid_control_box_reset(CelluloidControlBox *box)
{
	celluloid_control_box_set_seek_bar_pos(box, 0);
	set_fullscreen_state(box, FALSE);
}
