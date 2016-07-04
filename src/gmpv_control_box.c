/*
 * Copyright (c) 2014-2016 gnome-mpv
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

#include <glib/gi18n.h>

#include "gmpv_control_box.h"

struct _GmpvControlBox
{
	GtkBox parent_instance;
	gint seek_bar_length;
	GtkWidget *play_button;
	GtkWidget *stop_button;
	GtkWidget *forward_button;
	GtkWidget *rewind_button;
	GtkWidget *next_button;
	GtkWidget *previous_button;
	GtkWidget *volume_button;
	GtkWidget *fullscreen_button;
	GtkWidget *seek_bar;
};

struct _GmpvControlBoxClass
{
	GtkBoxClass parent_class;
};

static gchar *seek_bar_format_handler(	GtkScale *scale,
					gdouble value,
					gpointer data );
static void seek_handler(	GtkWidget *widget,
				GtkScrollType scroll,
				gdouble value,
				gpointer data );
static void volume_changed_handler(	GtkVolumeButton *button,
					gdouble value,
					gpointer data );
static void simple_signal_handler(GtkWidget *widget, gpointer data);

G_DEFINE_TYPE(GmpvControlBox, gmpv_control_box, GTK_TYPE_BOX)

static gchar *seek_bar_format_handler(	GtkScale *scale,
					gdouble value,
					gpointer data )
{
	gint sec = (gint)value;
	gint length = GMPV_CONTROL_BOX(data)->seek_bar_length;
	char *output = NULL;

	if (length == 0)
		return g_strdup("");

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

static void seek_handler(	GtkWidget *widget,
				GtkScrollType scroll,
				gdouble value,
				gpointer data )
{
	g_signal_emit_by_name(data, "seek", value);
}


static void volume_changed_handler(	GtkVolumeButton *button,
					gdouble value,
					gpointer data )
{
	g_signal_emit_by_name(data, "volume-changed", value);
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
	GtkWidget *play_icon;
	GtkWidget *stop_icon;
	GtkWidget *forward_icon;
	GtkWidget *rewind_icon;
	GtkWidget *previous_icon;
	GtkWidget *next_icon;
	GtkWidget *fullscreen_icon;

	box->seek_bar_length = -1;
	box->play_button = gtk_button_new_with_label(NULL);
	box->stop_button = gtk_button_new_with_label(NULL);
	box->forward_button = gtk_button_new_with_label(NULL);
	box->rewind_button = gtk_button_new_with_label(NULL);
	box->next_button = gtk_button_new_with_label(NULL);
	box->previous_button = gtk_button_new_with_label(NULL);
	box->fullscreen_button = gtk_button_new_with_label(NULL);
	box->volume_button = gtk_volume_button_new();
	box->seek_bar = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, NULL);

	play_icon
		= gtk_image_new_from_icon_name
			("media-playback-start-symbolic", GTK_ICON_SIZE_BUTTON);
	stop_icon
		= gtk_image_new_from_icon_name
			("media-playback-stop-symbolic", GTK_ICON_SIZE_BUTTON);
	forward_icon
		= gtk_image_new_from_icon_name
			("media-seek-forward-symbolic", GTK_ICON_SIZE_BUTTON);
	rewind_icon
		= gtk_image_new_from_icon_name
			("media-seek-backward-symbolic", GTK_ICON_SIZE_BUTTON);
	next_icon
		= gtk_image_new_from_icon_name
			("media-skip-forward-symbolic", GTK_ICON_SIZE_BUTTON);
	previous_icon
		= gtk_image_new_from_icon_name
			("media-skip-backward-symbolic", GTK_ICON_SIZE_BUTTON);
	fullscreen_icon
		= gtk_image_new_from_icon_name
			("view-fullscreen-symbolic", GTK_ICON_SIZE_BUTTON);

	gtk_style_context_add_class
		(	gtk_widget_get_style_context(GTK_WIDGET(box)),
			GTK_STYLE_CLASS_BACKGROUND );
	gtk_range_set_increments(GTK_RANGE(box->seek_bar), 10, 10);

	gtk_widget_set_tooltip_text(box->play_button, _("Play"));
	gtk_widget_set_tooltip_text(box->stop_button, _("Stop"));
	gtk_widget_set_tooltip_text(box->forward_button, _("Forward"));
	gtk_widget_set_tooltip_text(box->rewind_button, _("Rewind"));
	gtk_widget_set_tooltip_text(box->next_button, _("Next Chapter"));
	gtk_widget_set_tooltip_text(box->previous_button, _("Previous Chapter"));
	gtk_widget_set_tooltip_text(box->fullscreen_button, _("Toggle Fullscreen"));

	g_object_set(box->play_button, "relief", GTK_RELIEF_NONE, NULL);
	g_object_set(box->stop_button, "relief", GTK_RELIEF_NONE, NULL);
	g_object_set(box->forward_button, "relief", GTK_RELIEF_NONE, NULL);
	g_object_set(box->rewind_button, "relief", GTK_RELIEF_NONE, NULL);
	g_object_set(box->next_button, "relief", GTK_RELIEF_NONE, NULL);
	g_object_set(box->previous_button, "relief", GTK_RELIEF_NONE, NULL);
	g_object_set(box->fullscreen_button, "relief", GTK_RELIEF_NONE, NULL);
	g_object_set(box->seek_bar, "value-pos", GTK_POS_RIGHT, NULL);
	g_object_set(box->seek_bar, "digits", 2, NULL);

	gtk_widget_set_can_focus(box->previous_button, FALSE);
	gtk_widget_set_can_focus(box->rewind_button, FALSE);
	gtk_widget_set_can_focus(box->play_button, FALSE);
	gtk_widget_set_can_focus(box->stop_button, FALSE);
	gtk_widget_set_can_focus(box->forward_button, FALSE);
	gtk_widget_set_can_focus(box->next_button, FALSE);
	gtk_widget_set_can_focus(box->seek_bar, FALSE);
	gtk_widget_set_can_focus(box->volume_button, FALSE);
	gtk_widget_set_can_focus(box->fullscreen_button, FALSE);

	gtk_button_set_image
		(GTK_BUTTON(box->play_button), play_icon);
	gtk_button_set_image
		(GTK_BUTTON(box->stop_button), stop_icon);
	gtk_button_set_image
		(GTK_BUTTON(box->forward_button), forward_icon);
	gtk_button_set_image
		(GTK_BUTTON(box->rewind_button), rewind_icon);
	gtk_button_set_image
		(GTK_BUTTON(box->next_button), next_icon);
	gtk_button_set_image
		(GTK_BUTTON(box->previous_button), previous_icon);
	gtk_button_set_image
		(GTK_BUTTON(box->fullscreen_button), fullscreen_icon);

	gtk_container_add
		(GTK_CONTAINER(box), box->previous_button);
	gtk_container_add
		(GTK_CONTAINER(box), box->rewind_button);
	gtk_container_add
		(GTK_CONTAINER(box), box->play_button);
	gtk_container_add
		(GTK_CONTAINER(box), box->stop_button);
	gtk_container_add
		(GTK_CONTAINER(box), box->forward_button);
	gtk_container_add
		(GTK_CONTAINER(box), box->next_button);
	gtk_box_pack_start
		(GTK_BOX(box), box->seek_bar, TRUE, TRUE, 0);
	gtk_container_add
		(GTK_CONTAINER(box), box->volume_button);
	gtk_container_add
		(GTK_CONTAINER(box), box->fullscreen_button);

	g_signal_connect(	box,
				"button-press-event",
				G_CALLBACK(gtk_true),
				NULL );
	g_signal_connect(	box->seek_bar,
				"format-value",
				G_CALLBACK(seek_bar_format_handler),
				box );
	g_signal_connect(	box->play_button,
				"clicked",
				G_CALLBACK(simple_signal_handler),
				box );
	g_signal_connect(	box->stop_button,
				"clicked",
				G_CALLBACK(simple_signal_handler),
				box );
	g_signal_connect(	box->seek_bar,
				"change-value",
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
	gtk_range_set_value(GTK_RANGE(box->seek_bar), pos);
}

static void gmpv_control_box_set_seekable(GmpvControlBox *box, gboolean seekable)
{
	if (seekable)
	{
		gtk_widget_show(box->forward_button);
		gtk_widget_show(box->rewind_button);
		g_signal_handlers_unblock_by_func(box->seek_bar, seek_handler, box);
	}
	else
	{
		gtk_widget_hide(box->forward_button);
		gtk_widget_hide(box->rewind_button);
		g_signal_handlers_block_by_func(box->seek_bar, seek_handler, box);
	}
}

void gmpv_control_box_set_seek_bar_length(GmpvControlBox *box, gint length)
{
	if ((length == 0 || box->seek_bar_length == 0) && box->seek_bar_length != length)
	{
		gmpv_control_box_set_seekable(box, length != 0);
	}
	box->seek_bar_length = length;

	gtk_range_set_range(GTK_RANGE(box->seek_bar), 0, length);
}

void gmpv_control_box_set_volume(GmpvControlBox *box, gdouble volume)
{
	g_signal_handlers_block_by_func(box, volume_changed_handler, box);

	gtk_scale_button_set_value
		(GTK_SCALE_BUTTON(box->volume_button), volume);

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
	gmpv_control_box_set_seek_bar_length(box, 0);
	gmpv_control_box_set_playing_state(box, FALSE);
	gmpv_control_box_set_chapter_enabled(box, FALSE);
	gmpv_control_box_set_fullscreen_state(box, FALSE);
}
