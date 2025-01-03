/*
 * Copyright (c) 2014-2024 gnome-mpv
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

#include <adwaita.h>
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
	PROP_SHOW_PLAYLIST,
	PROP_TIME_POSITION,
	PROP_VOLUME,
	PROP_VOLUME_MAX,
	PROP_VOLUME_POPUP_VISIBLE,
	PROP_CHAPTER_LIST,
	PROP_CONTENT_TITLE,
	PROP_NARROW,
	N_PROPERTIES
};

struct _CelluloidControlBox
{
	AdwBin parent_instance;
	GtkGesture *click_gesture;
	GtkWidget *clamp;
	GtkWidget *title_widget;
	GtkWidget *controls_box;
	GtkWidget *inner_box;
	GtkWidget *play_button;
	GtkWidget *forward_button;
	GtkWidget *rewind_button;
	GtkWidget *next_button;
	GtkWidget *previous_button;
	GtkWidget *volume_button;
	GtkWidget *playlist_button;
	GtkWidget *seek_bar;
	GtkWidget *secondary_seek_bar;
	gboolean skip_enabled;
	gdouble duration;
	gboolean enabled;
	gboolean narrow;
	gboolean compact;
	gboolean fullscreened;
	gboolean pause;
	gboolean show_playlist;
	gdouble time_position;
	const gchar *content_title;
	gdouble volume;
	gdouble volume_max;
	gboolean volume_popup_visible;
	GPtrArray *chapter_list;
};

struct _CelluloidControlBoxClass
{
	AdwBinClass parent_class;
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
set_narrow(CelluloidControlBox *box, gboolean narrow);

static void
set_fullscreen_state(CelluloidControlBox *box, gboolean fullscreen);

static void
init_button(	GtkWidget *button,
		const gchar *icon_name,
		const gchar *tooltip_text );

G_DEFINE_TYPE(CelluloidControlBox, celluloid_control_box, ADW_TYPE_BIN)

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

		case PROP_NARROW:
		self->narrow = g_value_get_boolean(value);
		set_narrow(self, self->narrow);
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

		case PROP_SHOW_PLAYLIST:
		self->show_playlist = g_value_get_boolean(value);
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

		case PROP_VOLUME:
		self->volume = g_value_get_double(value);
		break;

		case PROP_VOLUME_MAX:
		self->volume_max = g_value_get_double(value);
		break;

		case PROP_VOLUME_POPUP_VISIBLE:
		self->volume_popup_visible = g_value_get_boolean(value);

		if(self->volume_popup_visible)
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

		case PROP_CONTENT_TITLE:
		self->content_title = g_value_get_string(value);
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

		case PROP_NARROW:
		g_value_set_boolean(value, self->narrow);
		break;

		case PROP_COMPACT:
		g_value_set_boolean(value, self->compact);
		break;

		case PROP_FULLSCREENED:
		g_value_set_boolean(value, self->fullscreened);
		break;

		case PROP_SHOW_PLAYLIST:
		g_value_set_boolean(value, self->show_playlist);
		break;

		case PROP_PAUSE:
		g_value_set_boolean(value, self->pause);
		break;

		case PROP_TIME_POSITION:
		g_value_set_double(value, self->time_position);
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

		case PROP_CONTENT_TITLE:
		g_value_set_string(value, self->content_title);
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
	gint offset = -6;
	graphene_rect_t bounds;

	if(gtk_widget_compute_bounds(self, box->seek_bar, &bounds))
	{
		offset = (gint)bounds.origin.y;
	}
	else
	{
		g_warning("Failed to calculate offset for timestamp popover.");
	}

	g_object_set(box->seek_bar, "popover-y-offset", offset, NULL);
	g_object_set(box->secondary_seek_bar, "popover-y-offset", offset, NULL);
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
			{box->forward_button, "button-clicked", "forward"},
			{box->rewind_button, "button-clicked", "rewind"},
			{box->previous_button, "button-clicked", "previous"},
			{box->next_button, "button-clicked", "next"},
			{box->playlist_button, "button-clicked", "playlist"},
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
		gtk_widget_set_visible(box->seek_bar, FALSE);
		gtk_widget_set_visible(box->secondary_seek_bar, TRUE);

		gtk_widget_add_css_class
			(GTK_WIDGET(box->controls_box), "docked");
		gtk_widget_remove_css_class
			(GTK_WIDGET(box->controls_box), "undocked");
	}
	else
	{
		gtk_widget_set_halign(box->inner_box, GTK_ALIGN_FILL);
		gtk_widget_set_visible(box->seek_bar, TRUE);
		gtk_widget_set_visible(box->secondary_seek_bar, FALSE);

		gtk_widget_add_css_class
			(GTK_WIDGET(box->controls_box), "undocked");
		gtk_widget_remove_css_class
			(GTK_WIDGET(box->controls_box), "docked");
	}
}

static void
set_narrow(CelluloidControlBox *box, gboolean narrow)
{
	if(box->narrow)
	{
		gtk_widget_add_css_class
			(GTK_WIDGET(box->controls_box), "docked");
		gtk_widget_remove_css_class
			(GTK_WIDGET(box->controls_box), "undocked");
	}
	else
	{
		gtk_widget_add_css_class
			(GTK_WIDGET(box->controls_box), "undocked");
		gtk_widget_remove_css_class
			(GTK_WIDGET(box->controls_box), "docked");
	}
}

static void
set_fullscreen_state(CelluloidControlBox *box, gboolean fullscreen)
{
}

static void
init_button(	GtkWidget *button,
		const gchar *icon_name,
		const gchar *tooltip_text )
{
	gtk_button_set_icon_name(GTK_BUTTON(button), icon_name);
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
		(	"show-playlist",
			"Playlist shown",
			"Whether the playlist panel is shown",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_SHOW_PLAYLIST, pspec);

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

	pspec = g_param_spec_string
		(	"content-title",
			"Content Title",
			"Title of the currently playing item",
			NULL,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_CONTENT_TITLE, pspec);

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
		(	"narrow",
			"Narrow",
			"Whether or not narrow mode is enabled",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_NARROW, pspec);

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
		{	"audio-volume-muted-symbolic",
			"audio-volume-high-symbolic",
			"audio-volume-low-symbolic",
			"audio-volume-medium-symbolic",
			NULL };

	box->clamp = adw_clamp_new();
	box->click_gesture = gtk_gesture_click_new();
	box->title_widget = gtk_label_new("");
	box->controls_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	box->inner_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	box->play_button = gtk_button_new();
	box->forward_button = gtk_button_new();
	box->rewind_button = gtk_button_new();
	box->next_button = gtk_button_new();
	box->previous_button = gtk_button_new();
	box->playlist_button = gtk_toggle_button_new();
	box->volume_button = gtk_scale_button_new(0.0, 1.0, 0.02, volume_icons);
	box->seek_bar = celluloid_seek_bar_new();
	box->secondary_seek_bar = celluloid_seek_bar_new();
	box->skip_enabled = FALSE;
	box->duration = 0.0;
	box->enabled = TRUE;
	box->narrow = FALSE;
	box->compact = FALSE;
	box->fullscreened = FALSE;
	box->pause = TRUE;
	box->show_playlist = FALSE;
	box->time_position = 0.0;
	box->volume = 0.0;
	box->content_title = "";
	box->volume_max = 100.0;
	box->volume_popup_visible = FALSE;
	box->chapter_list = NULL;

	init_button(	box->play_button,
			"media-playback-start-symbolic",
			_("Play") );
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
	init_button(	box->playlist_button,
			"sidebar-show-right-symbolic",
			_("Show Playlist") );

	gtk_widget_add_css_class(box->controls_box,"control-box");
	gtk_widget_add_css_class(box->controls_box, "toolbar");
	gtk_widget_add_css_class(box->title_widget, "heading");

	gtk_widget_add_controller
		(GTK_WIDGET(box), GTK_EVENT_CONTROLLER(box->click_gesture));

	gtk_label_set_max_width_chars
		(GTK_LABEL(box->title_widget), 20);
	gtk_label_set_ellipsize
		(GTK_LABEL(box->title_widget), PANGO_ELLIPSIZE_MIDDLE);
	gtk_widget_set_size_request
		(GTK_WIDGET(box->title_widget), 200, -1);

	gtk_widget_set_margin_end(box->seek_bar, 6);
	gtk_widget_set_margin_end(box->secondary_seek_bar, 6);

	gtk_widget_set_can_focus(box->volume_button, FALSE);
	gtk_widget_set_can_focus(box->seek_bar, FALSE);
	gtk_widget_set_can_focus(box->secondary_seek_bar, FALSE);

	gtk_widget_set_hexpand(box->seek_bar, TRUE);
	gtk_widget_set_hexpand(box->secondary_seek_bar, TRUE);

	g_object_set(box->secondary_seek_bar, "show-label", FALSE, NULL);

	gtk_widget_set_tooltip_text(box->volume_button, _("Volume"));
	set_narrow(box, box->narrow);
	set_compact(box, box->compact);

	// No need to set maximum size here. It will be set by
	// celluloid_control_box_set_floating().
	adw_bin_set_child(ADW_BIN(box), box->clamp);
	adw_clamp_set_child(ADW_CLAMP(box->clamp), box->controls_box);
	adw_clamp_set_tightening_threshold(ADW_CLAMP(box->clamp), 700);

	box->inner_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_append(GTK_BOX(box->controls_box), box->title_widget);
	gtk_box_append(GTK_BOX(box->controls_box), box->secondary_seek_bar);
	gtk_box_append(GTK_BOX(box->controls_box), box->inner_box);
	gtk_box_append(GTK_BOX(box->inner_box), box->previous_button);
	gtk_box_append(GTK_BOX(box->inner_box), box->rewind_button);
	gtk_box_append(GTK_BOX(box->inner_box), box->play_button);
	gtk_box_append(GTK_BOX(box->inner_box), box->forward_button);
	gtk_box_append(GTK_BOX(box->inner_box), box->next_button);
	gtk_box_append(GTK_BOX(box->inner_box), box->seek_bar);
	gtk_box_append(GTK_BOX(box->inner_box), box->volume_button);
	gtk_box_append(GTK_BOX(box->inner_box), box->playlist_button);

	popup =	gtk_scale_button_get_popup
		(GTK_SCALE_BUTTON(box->volume_button));
	adjustment =	gtk_scale_button_get_adjustment
			(GTK_SCALE_BUTTON(box->volume_button));

	gtk_popover_set_position(GTK_POPOVER(popup), GTK_POS_TOP);

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
	g_object_bind_property(	box, "show_playlist",
				box->playlist_button, "active",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property(	box, "enabled",
				box->secondary_seek_bar, "enabled",
				G_BINDING_DEFAULT );
	g_object_bind_property(	box, "content-title",
				box->title_widget, "label",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property(	box->title_widget, "label",
				box->title_widget, "tooltip-text",
				G_BINDING_DEFAULT );
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
	GtkWidget *toggle_button =
		gtk_widget_get_first_child(box->volume_button);

	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_button));
}

void
celluloid_control_box_set_floating(CelluloidControlBox *box, gboolean floating)
{
	if(floating)
	{
		gtk_widget_add_css_class
			(GTK_WIDGET(box->controls_box), "undocked");
		gtk_widget_add_css_class
			(GTK_WIDGET(box->controls_box), "osd");
		gtk_widget_set_visible
			(GTK_WIDGET(box->title_widget), TRUE);
		adw_clamp_set_maximum_size
			(ADW_CLAMP(box->clamp), 1000);
	}
	else
	{
		gtk_widget_remove_css_class
			(GTK_WIDGET(box->controls_box), "undocked");
		gtk_widget_remove_css_class
			(GTK_WIDGET(box->controls_box), "osd");
		gtk_widget_set_visible
			(GTK_WIDGET(box->title_widget), FALSE);
		adw_clamp_set_maximum_size
			(ADW_CLAMP(box->clamp), G_MAXINT);
	}
}

void
celluloid_control_box_set_title(CelluloidControlBox *box, gchar* title)
{
	gtk_label_set_label(GTK_LABEL(box->title_widget), title);
}

void
celluloid_control_box_reset(CelluloidControlBox *box)
{
	celluloid_control_box_set_seek_bar_pos(box, 0);
	set_fullscreen_state(box, FALSE);
}
