/*
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

enum PlaylistStoreColumn
{
	PLAYLIST_INDICATOR_COLUMN,
	PLAYLIST_NAME_COLUMN,
	PLAYLIST_URI_COLUMN,
	PLAYLIST_N_COLUMNS
};

typedef struct PlaylistWidget PlaylistWidget;
typedef struct PlaylistWidgetClass PlaylistWidgetClass;
typedef enum PlaylistStoreColumn PlaylistStoreColumn;

struct PlaylistWidget
{
	GtkScrolledWindow scrolled_window;
	GtkWidget *tree_view;
	GtkListStore *list_store;
	GtkTreeViewColumn *indicator_column;
	GtkTreeViewColumn *title_column;
	GtkCellRenderer *indicator_renderer;
	GtkCellRenderer *title_renderer;
};

struct PlaylistWidgetClass
{
	GtkScrolledWindowClass parent_class;
};

GtkWidget *playlist_widget_new(void);
GType playlist_widget_get_type(void);
void playlist_widget_clear(PlaylistWidget *wgt);
void playlist_widget_get_iter_first(PlaylistWidget *wgt, GtkTreeIter *iter);
void playlist_widget_iter_next(PlaylistWidget *wgt, GtkTreeIter *iter);
void playlist_widget_get_uri(PlaylistWidget *wgt, gint pos);
void playlist_widget_set_indicator_pos(PlaylistWidget *wgt, gint pos);
void playlist_widget_remove(PlaylistWidget *wgt, gint pos);

void playlist_widget_append(	PlaylistWidget *wgt,
				const gchar *name,
				const gchar *uri );

G_END_DECLS

#endif
