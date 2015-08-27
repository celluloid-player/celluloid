/*
 * Copyright (c) 2015 gnome-mpv
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

#include "volume_widget.h"
#include "common.h"

G_DEFINE_TYPE(VolumeWidget, volume_widget, GTK_TYPE_BOX)

static void volume_widget_init(VolumeWidget *wgt)
{
	wgt->ebox = gtk_event_box_new();

	wgt->icon = gtk_image_new_from_icon_name
		("multimedia-volume-control-symbolic", GTK_ICON_SIZE_BUTTON);

	/* Use the same range and stepping as GtkVolumeButton */
	wgt->scale = gtk_scale_new_with_range
		(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.02);

	gtk_scale_set_draw_value(GTK_SCALE(wgt->scale), FALSE);
	set_margin_start(wgt->icon, 10);
	gtk_widget_set_size_request(GTK_WIDGET(wgt), 128, -1);

	gtk_box_pack_end(GTK_BOX(wgt), wgt->scale, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(wgt), wgt->ebox, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(wgt->ebox), wgt->icon);

	gtk_widget_show_all(wgt->ebox);
	gtk_widget_show(wgt->scale);
}

static void volume_widget_class_init(VolumeWidgetClass *klass)
{
}

GtkWidget *volume_widget_new()
{
	return GTK_WIDGET(g_object_new(volume_widget_get_type(), NULL));
}
