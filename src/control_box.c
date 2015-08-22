/*
 * Copyright (c) 2014-2015 gnome-mpv
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

#include "control_box.h"

static gchar *seek_bar_format_handler(	GtkScale *scale,
					gdouble value,
					gpointer data );
static void control_box_init(ControlBox *box);

G_DEFINE_TYPE(ControlBox, control_box, GTK_TYPE_BOX)

static gchar *seek_bar_format_handler(	GtkScale *scale,
					gdouble value,
					gpointer data )
{
	gint sec = value;
	gint length = ((ControlBox *)data)->seek_bar_length;
	char *output = NULL;

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

static void control_box_class_init(ControlBoxClass *klass)
{
}

static void control_box_init(ControlBox *box)
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
		= gtk_image_new_from_icon_name(	"media-playback-start-symbolic",
						GTK_ICON_SIZE_BUTTON );

	stop_icon
		= gtk_image_new_from_icon_name(	"media-playback-stop-symbolic",
						GTK_ICON_SIZE_BUTTON );

	forward_icon
		= gtk_image_new_from_icon_name(	"media-seek-forward-symbolic",
						GTK_ICON_SIZE_BUTTON );

	rewind_icon
		= gtk_image_new_from_icon_name(	"media-seek-backward-symbolic",
						GTK_ICON_SIZE_BUTTON );

	next_icon
		= gtk_image_new_from_icon_name(	"media-skip-forward-symbolic",
						GTK_ICON_SIZE_BUTTON );

	previous_icon
		= gtk_image_new_from_icon_name(	"media-skip-backward-symbolic",
						GTK_ICON_SIZE_BUTTON );

	fullscreen_icon
		= gtk_image_new_from_icon_name(	"view-fullscreen-symbolic",
						GTK_ICON_SIZE_BUTTON );

	gtk_range_set_increments(GTK_RANGE(box->seek_bar), 10, 10);

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

	g_signal_connect(	box->seek_bar,
				"format-value",
				G_CALLBACK(seek_bar_format_handler),
				box );
}

GtkWidget *control_box_new(void)
{
	return GTK_WIDGET(g_object_new(control_box_get_type(), NULL));
}

void control_box_set_enabled(ControlBox *box, gboolean enabled)
{
	gtk_widget_set_sensitive(box->previous_button, enabled);
	gtk_widget_set_sensitive(box->rewind_button, enabled);
	gtk_widget_set_sensitive(box->stop_button, enabled);
	gtk_widget_set_sensitive(box->play_button, enabled);
	gtk_widget_set_sensitive(box->forward_button, enabled);
	gtk_widget_set_sensitive(box->next_button, enabled);
}

void control_box_set_chapter_enabled(ControlBox *box, gboolean enabled)
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

void control_box_set_seek_bar_length(ControlBox *box, gint length)
{
	box->seek_bar_length = length;

	gtk_range_set_range(GTK_RANGE(box->seek_bar), 0, length);
}

void control_box_set_volume(ControlBox *box, gdouble volume)
{
	gtk_scale_button_set_value
		(GTK_SCALE_BUTTON(box->volume_button), volume);
}

void control_box_set_playing_state(ControlBox *box, gboolean playing)
{
	GtkWidget *play_icon;

	play_icon = gtk_image_new_from_icon_name
			( 	playing
				?"media-playback-pause-symbolic"
				:"media-playback-start-symbolic",
				GTK_ICON_SIZE_BUTTON );

	gtk_button_set_image(GTK_BUTTON(box->play_button), play_icon);
}

void control_box_set_fullscreen_state(ControlBox *box, gboolean fullscreen)
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

void control_box_set_fullscreen_btn_visible(ControlBox *box, gboolean value)
{
	gtk_widget_set_visible(box->fullscreen_button, value);
}

void control_box_reset_control(ControlBox *box)
{
	control_box_set_seek_bar_length(box, 0);
	control_box_set_playing_state(box, FALSE);
	control_box_set_fullscreen_state(box, FALSE);
}
