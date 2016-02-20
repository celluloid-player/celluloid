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

#ifndef PLAYLIST_WIDGET_H
#define PLAYLIST_WIDGET_H

#include <gtk/gtk.h>

#include "playlist.h"

G_BEGIN_DECLS

#define PLAYLIST_WIDGET_TYPE (playlist_widget_get_type ())

#define	PLAYLIST_WIDGET(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
		((obj), PLAYLIST_WIDGET_TYPE, PlaylistWidget))

#define	PLAYLIST_WIDGET_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST \
		((klass), PLAYLIST_WIDGET_TYPE, PlaylistWidgetClass))

#define	IS_PLAYLIST_WIDGET(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), PLAYLIST_WIDGET_TYPE))

#define	IS_PLAYLIST_WIDGET_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), PLAYLIST_WIDGET_TYPE))

struct _PlaylistWidget
{
	GtkScrolledWindow parent_instance;
	GtkWidget *tree_view;
	Playlist *store;
	GtkTreeViewColumn *indicator_column;
	GtkTreeViewColumn *title_column;
	GtkCellRenderer *indicator_renderer;
	GtkCellRenderer *title_renderer;
};

struct _PlaylistWidgetClass
{
	GtkScrolledWindowClass parent_class;
};

typedef struct _PlaylistWidget PlaylistWidget;
typedef struct _PlaylistWidgetClass PlaylistWidgetClass;

GtkWidget *playlist_widget_new(void);
GType playlist_widget_get_type(void);
void playlist_widget_remove_selected(PlaylistWidget *wgt);

G_END_DECLS

#endif
