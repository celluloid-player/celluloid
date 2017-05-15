/*
 * Copyright (c) 2014-2017 gnome-mpv
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

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "gmpv_control_box.h"
#include "gmpv_seek_bar.h"

enum
{
	PROP_0,
	PROP_VOLUME,
	N_PROPERTIES
};

struct _GmpvControlBox
{
	GtkBox parent_instance;
	GtkWidget *play_button;
	GtkWidget *stop_button;
	GtkWidget *forward_button;
	GtkWidget *rewind_button;
	GtkWidget *next_button;
	GtkWidget *previous_button;
	GtkWidget *volume_button;
	GtkWidget *fullscreen_button;
	GtkWidget *seek_bar;
	gdouble volume;
};

struct _GmpvControlBoxClass
{
	GtkBoxClass parent_class;
};

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec );
static gboolean gtk_to_mpv_volume(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data );
static gboolean mpv_to_gtk_volume(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data );
static void seek_handler(	GmpvSeekBar *seek_bar,
				gdouble value,
				gpointer data );
static void volume_changed_handler(	GtkVolumeButton *button,
					gdouble value,
					gpointer data );
static void simple_signal_handler(GtkWidget *widget, gpointer data);
static void init_button(	GtkWidget *button,
				const gchar *icon_name,
				const gchar *tooltip_text );

G_DEFINE_TYPE(GmpvControlBox, gmpv_control_box, GTK_TYPE_BOX)

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvControlBox *self = GMPV_CONTROL_BOX(object);

	if(property_id == PROP_VOLUME)
	{
		self->volume = g_value_get_double(value);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec )
{
	GmpvControlBox *self = GMPV_CONTROL_BOX(object);

	if(property_id == PROP_VOLUME)
	{
		g_value_set_double(value, self->volume);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static gboolean gtk_to_mpv_volume(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data )
{
	gdouble from = g_value_get_double(from_value);
	g_value_set_double(to_value, from*100);

	return TRUE;
}

static gboolean mpv_to_gtk_volume(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data )
{
	gdouble from = g_value_get_double(from_value);
	g_value_set_double(to_value, from/100.0);

	return TRUE;
}

static void seek_handler(	GmpvSeekBar *seek_bar,
				gdouble value,
				gpointer data )
{
	g_signal_emit_by_name(data, "seek", value);
}

static void volume_changed_handler(	GtkVolumeButton *button,
					gdouble value,
					gpointer data )
{
	g_signal_emit_by_name(data, "volume-changed", value*100.0);
}

static void simple_signal_handler(GtkWidget *widget, gpointer data)
{
	GmpvControlBox *box = data;
	gint i;

	const struct
	{
		const GtkWidget *widget;
		const gchar *name;
	}
	signal_map[]
		= {	{box->play_button, "play-button-clicked"},
			{box->stop_button, "stop-button-clicked"},
			{box->forward_button, "forward-button-clicked"},
			{box->rewind_button, "rewind-button-clicked"},
			{box->previous_button, "previous-button-clicked"},
			{box->next_button, "next-button-clicked"},
			{box->fullscreen_button, "fullscreen-button-clicked"},
			{NULL, NULL} };

	for(i = 0; signal_map[i].name && signal_map[i].widget != widget; i++);

	if(signal_map[i].name)
	{
		g_signal_emit_by_name(data, signal_map[i].name);
	}
}

static void init_button(	GtkWidget *button,
				const gchar *icon_name,
				const gchar *tooltip_text )
{
	GtkWidget *play_icon;

	play_icon =	gtk_image_new_from_icon_name
			(icon_name, GTK_ICON_SIZE_BUTTON);

	gtk_widget_set_tooltip_text(button, _("Play"));
	g_object_set(button, "relief", GTK_RELIEF_NONE, NULL);
	gtk_widget_set_can_focus(button, FALSE);
	gtk_button_set_image(GTK_BUTTON(button), play_icon);
}

static void gmpv_control_box_class_init(GmpvControlBoxClass *klass)
{
	/* Names of signals that have no parameter and return nothing */
	const gchar *simple_signals[] = {	"play-button-clicked",
						"stop-button-clicked",
						"forward-button-clicked",
						"rewind-button-clicked",
						"previous-button-clicked",
						"next-button-clicked",
						"fullscreen-button-clicked",
						NULL };
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	object_class->set_property = set_property;
	object_class->get_property = get_property;

	pspec = g_param_spec_double
		(	"volume",
			"Volume",
			"The value of the volume button",
			0.0,
			G_MAXDOUBLE,
			0.0,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_VOLUME, pspec);

	for(gint i = 0; simple_signals[i]; i++)
	{
		g_signal_new(	simple_signals[i],
				G_TYPE_FROM_CLASS(klass),
				G_SIGNAL_RUN_FIRST,
				0,
				NULL,
				NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE,
				0 );
	}

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

static void gmpv_control_box_init(GmpvControlBox *box)
{
	box->play_button = gtk_button_new();
	box->stop_button = gtk_button_new();
	box->forward_button = gtk_button_new();
	box->rewind_button = gtk_button_new();
	box->next_button = gtk_button_new();
	box->previous_button = gtk_button_new();
	box->fullscreen_button = gtk_button_new();
	box->volume_button = gtk_volume_button_new();
	box->seek_bar = gmpv_seek_bar_new();

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
	init_button(	box->fullscreen_button,
			"view-fullscreen-symbolic",
			_("Toggle Fullscreen") );

	gtk_style_context_add_class
		(	gtk_widget_get_style_context(GTK_WIDGET(box)),
			GTK_STYLE_CLASS_BACKGROUND );

	gtk_widget_set_can_focus(box->volume_button, FALSE);
	gtk_widget_set_can_focus(box->seek_bar, FALSE);

	gtk_container_add(GTK_CONTAINER(box), box->previous_button);
	gtk_container_add(GTK_CONTAINER(box), box->rewind_button);
	gtk_container_add(GTK_CONTAINER(box), box->play_button);
	gtk_container_add(GTK_CONTAINER(box), box->stop_button);
	gtk_container_add(GTK_CONTAINER(box), box->forward_button);
	gtk_container_add(GTK_CONTAINER(box), box->next_button);
	gtk_box_pack_start(GTK_BOX(box), box->seek_bar, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(box), box->volume_button);
	gtk_container_add(GTK_CONTAINER(box), box->fullscreen_button);

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

GtkWidget *gmpv_control_box_new(void)
{
	return GTK_WIDGET(g_object_new(gmpv_control_box_get_type(), NULL));
}

void gmpv_control_box_set_enabled(GmpvControlBox *box, gboolean enabled)
{
	gtk_widget_set_sensitive(box->previous_button, enabled);
	gtk_widget_set_sensitive(box->rewind_button, enabled);
	gtk_widget_set_sensitive(box->stop_button, enabled);
	gtk_widget_set_sensitive(box->play_button, enabled);
	gtk_widget_set_sensitive(box->forward_button, enabled);
	gtk_widget_set_sensitive(box->next_button, enabled);
}

void gmpv_control_box_set_chapter_enabled(GmpvControlBox *box, gboolean enabled)
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

void gmpv_control_box_set_seek_bar_pos(GmpvControlBox *box, gdouble pos)
{
	gmpv_seek_bar_set_pos(GMPV_SEEK_BAR(box->seek_bar), pos);
}

void gmpv_control_box_set_seek_bar_duration(GmpvControlBox *box, gint duration)
{
	gmpv_seek_bar_set_duration(GMPV_SEEK_BAR(box->seek_bar), duration);
}

void gmpv_control_box_set_volume(GmpvControlBox *box, gdouble volume)
{
	g_signal_handlers_block_by_func(box, volume_changed_handler, box);

	gtk_scale_button_set_value
		(GTK_SCALE_BUTTON(box->volume_button), volume/100.0);

	g_signal_handlers_unblock_by_func(box, volume_changed_handler, box);
}

gdouble gmpv_control_box_get_volume(GmpvControlBox *box)
{
	return gtk_scale_button_get_value(GTK_SCALE_BUTTON(box->volume_button));
}

gboolean gmpv_control_box_get_volume_popup_visible(GmpvControlBox *box)
{
	GtkScaleButton *volume_button = GTK_SCALE_BUTTON(box->volume_button);

	return gtk_widget_is_visible(gtk_scale_button_get_popup(volume_button));
}

void gmpv_control_box_set_playing_state(GmpvControlBox *box, gboolean playing)
{
	GtkWidget *play_icon;
	const gchar *tooltip;

	play_icon = gtk_image_new_from_icon_name
			( 	playing
				?"media-playback-pause-symbolic"
				:"media-playback-start-symbolic",
				GTK_ICON_SIZE_BUTTON );
	tooltip = playing?_("Pause"):_("Play");

	gtk_button_set_image(GTK_BUTTON(box->play_button), play_icon);
	gtk_widget_set_tooltip_text(box->play_button, tooltip);
}

void gmpv_control_box_set_fullscreen_state(	GmpvControlBox *box,
						gboolean fullscreen )
{
	GtkWidget *fullscreen_icon;

	fullscreen_icon = gtk_image_new_from_icon_name
				(	fullscreen
					?"view-restore-symbolic"
					:"view-fullscreen-symbolic",
					GTK_ICON_SIZE_BUTTON );

	gtk_button_set_image(	GTK_BUTTON(box->fullscreen_button),
				fullscreen_icon );
}

void gmpv_control_box_set_fullscreen_btn_visible(	GmpvControlBox *box,
							gboolean value )
{
	gtk_widget_set_visible(box->fullscreen_button, value);
}

void gmpv_control_box_reset(GmpvControlBox *box)
{
	gmpv_control_box_set_seek_bar_pos(box, 0);
	gmpv_control_box_set_seek_bar_duration(box, 0);
	gmpv_control_box_set_playing_state(box, FALSE);
	gmpv_control_box_set_chapter_enabled(box, FALSE);
	gmpv_control_box_set_fullscreen_state(box, FALSE);
}
