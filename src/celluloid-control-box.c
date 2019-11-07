/*
 * Copyright (c) 2014-2019 gnome-mpv
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
	PROP_PAUSE,
	PROP_SHOW_FULLSCREEN_BUTTON,
	PROP_TIME_POSITION,
	PROP_LOOP,
	PROP_VOLUME,
	PROP_VOLUME_MAX,
	PROP_VOLUME_POPUP_VISIBLE,
	N_PROPERTIES
};

struct _CelluloidControlBox
{
	GtkBox parent_instance;
	GtkWidget *play_button;
	GtkWidget *stop_button;
	GtkWidget *forward_button;
	GtkWidget *rewind_button;
	GtkWidget *next_button;
	GtkWidget *previous_button;
	GtkWidget *loop_button;
	GtkWidget *volume_button;
	GtkWidget *fullscreen_button;
	GtkWidget *seek_bar;
	gboolean skip_enabled;
	gdouble duration;
	gboolean enabled;
	gboolean pause;
	gboolean show_fullscreen_button;
	gdouble time_position;
	gboolean loop;
	gdouble volume;
	gdouble volume_max;
	gboolean volume_popup_visible;
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

static void
simple_signal_handler(GtkWidget *widget, gpointer data);

static void
set_enabled(CelluloidControlBox *box, gboolean enabled);

static void
set_skip_enabled(CelluloidControlBox *box, gboolean enabled);

static void
set_playing_state(CelluloidControlBox *box, gboolean playing);

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

		celluloid_seek_bar_set_duration
			(CELLULOID_SEEK_BAR(self->seek_bar), self->duration);
		break;

		case PROP_ENABLED:
		self->enabled = g_value_get_boolean(value);
		set_enabled(self, self->enabled);
		break;

		case PROP_PAUSE:
		self->pause = g_value_get_boolean(value);
		set_playing_state(self, !self->pause);
		break;

		case PROP_SHOW_FULLSCREEN_BUTTON:
		self->show_fullscreen_button = g_value_get_boolean(value);
		break;

		case PROP_TIME_POSITION:
		self->time_position = g_value_get_double(value);

		celluloid_seek_bar_set_pos
			(	CELLULOID_SEEK_BAR(self->seek_bar),
				self->time_position );
		break;

		case PROP_LOOP:
		self->loop = g_value_get_boolean(value);
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

		case PROP_VOLUME:
		g_value_set_double(value, self->volume);
		break;

		case PROP_VOLUME_MAX:
		g_value_set_double(value, self->volume_max);
		break;

		case PROP_VOLUME_POPUP_VISIBLE:
		g_value_set_boolean(value, self->volume_popup_visible);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
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
		gtk_widget_show_all(box->previous_button);
		gtk_widget_show_all(box->next_button);
	}
	else
	{
		gtk_widget_hide(box->previous_button);
		gtk_widget_hide(box->next_button);
	}
}

static void
set_playing_state(CelluloidControlBox *box, gboolean playing)
{
	GtkWidget *image = gtk_button_get_image(GTK_BUTTON(box->play_button));
	const gchar *tooltip = playing?_("Pause"):_("Play");

	gtk_image_set_from_icon_name( 	GTK_IMAGE(image),
					playing?
					"media-playback-pause-symbolic":
					"media-playback-start-symbolic",
					GTK_ICON_SIZE_BUTTON );

	gtk_widget_set_tooltip_text(box->play_button, tooltip);
}

static void
init_button(	GtkWidget *button,
		const gchar *icon_name,
		const gchar *tooltip_text )
{
	GtkWidget *icon =	gtk_image_new_from_icon_name
				(icon_name, GTK_ICON_SIZE_BUTTON);

	gtk_widget_set_tooltip_text(button, tooltip_text);
	g_object_set(button, "relief", GTK_RELIEF_NONE, NULL);
	gtk_widget_set_can_focus(button, FALSE);
	gtk_button_set_image(GTK_BUTTON(button), icon);
}

static void
celluloid_control_box_class_init(CelluloidControlBoxClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	object_class->set_property = set_property;
	object_class->get_property = get_property;

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

	box->play_button = gtk_button_new();
	box->stop_button = gtk_button_new();
	box->forward_button = gtk_button_new();
	box->rewind_button = gtk_button_new();
	box->next_button = gtk_button_new();
	box->previous_button = gtk_button_new();
	box->fullscreen_button = gtk_button_new();
	box->loop_button = gtk_toggle_button_new();
	box->volume_button = gtk_volume_button_new();
	box->seek_bar = celluloid_seek_bar_new();
	box->skip_enabled = FALSE;
	box->duration = 0.0;
	box->enabled = TRUE;
	box->pause = TRUE;
	box->show_fullscreen_button = FALSE;
	box->time_position = 0.0;
	box->loop = FALSE;
	box->volume = 0.0;
	box->volume_max = 100.0;

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
	init_button(	box->fullscreen_button,
			"view-fullscreen-symbolic",
			_("Toggle Fullscreen") );

	gtk_style_context_add_class
		(	gtk_widget_get_style_context(GTK_WIDGET(box)),
			GTK_STYLE_CLASS_BACKGROUND );

	gtk_widget_set_margin_end(box->seek_bar, 6);

	gtk_widget_set_can_focus(box->volume_button, FALSE);
	gtk_widget_set_can_focus(box->seek_bar, FALSE);

	gtk_container_add(GTK_CONTAINER(box), box->previous_button);
	gtk_container_add(GTK_CONTAINER(box), box->rewind_button);
	gtk_container_add(GTK_CONTAINER(box), box->play_button);
	gtk_container_add(GTK_CONTAINER(box), box->stop_button);
	gtk_container_add(GTK_CONTAINER(box), box->forward_button);
	gtk_container_add(GTK_CONTAINER(box), box->next_button);
	gtk_box_pack_start(GTK_BOX(box), box->seek_bar, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(box), box->loop_button);
	gtk_container_add(GTK_CONTAINER(box), box->volume_button);
	gtk_container_add(GTK_CONTAINER(box), box->fullscreen_button);

	popup =	gtk_scale_button_get_popup
		(GTK_SCALE_BUTTON(box->volume_button));
	adjustment =	gtk_scale_button_get_adjustment
			(GTK_SCALE_BUTTON(box->volume_button));

	g_object_bind_property(	popup, "visible",
				box, "volume-popup-visible",
				G_BINDING_DEFAULT );
	g_object_bind_property(	box, "volume-max",
				adjustment, "upper",
				G_BINDING_DEFAULT );
	g_object_bind_property(	box, "loop",
				box->loop_button, "active",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property_full(	box->volume_button, "value",
					box, "volume",
					G_BINDING_BIDIRECTIONAL,
					gtk_to_mpv_volume,
					mpv_to_gtk_volume,
					NULL,
					NULL );

	g_signal_connect(	box,
				"button-press-event",
				G_CALLBACK(gtk_true),
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
}

void
celluloid_control_box_set_seek_bar_duration(	CelluloidControlBox *box,
						gint duration )
{
	celluloid_seek_bar_set_duration(CELLULOID_SEEK_BAR(box->seek_bar), duration);
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
celluloid_control_box_set_fullscreen_state(	CelluloidControlBox *box,
						gboolean fullscreen )
{
	GtkWidget *image =
		gtk_button_get_image(GTK_BUTTON(box->fullscreen_button));

	gtk_image_set_from_icon_name(	GTK_IMAGE(image),
					fullscreen?
					"view-restore-symbolic":
					"view-fullscreen-symbolic",
					GTK_ICON_SIZE_BUTTON );

	gtk_widget_set_visible(	box->fullscreen_button,
				!fullscreen && box->show_fullscreen_button );
}

void
celluloid_control_box_reset(CelluloidControlBox *box)
{
	celluloid_control_box_set_seek_bar_pos(box, 0);
	celluloid_control_box_set_seek_bar_duration(box, 0);
	set_playing_state(box, FALSE);
	set_skip_enabled(box, FALSE);
	celluloid_control_box_set_fullscreen_state(box, FALSE);
}
