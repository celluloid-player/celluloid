/*
 * Copyright (c) 2026 gnome-mpv
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

#include "celluloid-file.h"

#include <stdio.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

// URI from GFile can have the password stripped or not depending on the URI
// scheme, e.g. the password will stripped for rtsp://. The purpose of
// CelluloidFile is to keep track of the password in case it gets stripped and
// put it back as needed.

struct _CelluloidFile
{
	GObject parent;
	GFile* gfile;
	gchar *password;
};

struct _CelluloidFileClass
{
	GObjectClass parent_class;
};

G_DEFINE_TYPE(CelluloidFile, celluloid_file, G_TYPE_OBJECT)

static void
dispose(GObject *object)
{
	CelluloidFile *file = CELLULOID_FILE(object);

	g_clear_object(&file->gfile);
	g_clear_pointer(&file->password, g_free);

	G_OBJECT_CLASS(celluloid_file_parent_class)->dispose(object);
}

static void
celluloid_file_class_init(CelluloidFileClass *klass)
{
	G_OBJECT_CLASS(klass)->dispose = dispose;
}

static void
celluloid_file_init(CelluloidFile *file)
{
	file->gfile = NULL;
	file->password = NULL;
}

CelluloidFile *
celluloid_file_new_for_uri(const gchar *uri)
{
	GObject *object = g_object_new(celluloid_file_get_type(), NULL);
	CelluloidFile *file = CELLULOID_FILE(object);
	GUri *guri = g_uri_parse(uri, G_URI_FLAGS_HAS_PASSWORD, NULL);

	file->gfile = g_file_new_for_uri(uri);

	if(guri)
	{
		file->password = g_strdup(g_uri_get_password(guri));
		g_uri_unref(guri);
	}

	return file;
}

CelluloidFile *
celluloid_file_new_for_gfile(GFile *gfile)
{
	GObject *object = g_object_new(celluloid_file_get_type(), NULL);
	CelluloidFile *file = CELLULOID_FILE(object);

	file->gfile = g_object_ref(gfile);

	return file;
}

GFile *
celluloid_file_get_gfile(CelluloidFile *file)
{
	return g_object_ref(file->gfile);
}

gchar *
celluloid_file_get_path(CelluloidFile *file)
{
	return g_file_get_path(file->gfile);
}

gchar *
celluloid_file_get_uri(CelluloidFile *file)
{
	gchar *gfile_uri = g_file_get_uri(file->gfile);
	gchar *uri = gfile_uri;

	if(file->password)
	{
		GUri *old_guri = g_uri_parse
			(	gfile_uri,
				G_URI_FLAGS_HAS_PASSWORD,
				NULL );

		GUri *new_guri = g_uri_build_with_user
			(	G_URI_FLAGS_HAS_PASSWORD,
				g_uri_get_scheme(old_guri),
				g_uri_get_user(old_guri),
				file->password,
				g_uri_get_auth_params(old_guri),
				g_uri_get_host(old_guri),
				g_uri_get_port(old_guri),
				g_uri_get_path(old_guri),
				g_uri_get_query(old_guri),
				g_uri_get_fragment(old_guri) );

		g_free(uri);
		uri = g_uri_to_string(new_guri);

		g_uri_unref(new_guri);
		g_uri_unref(old_guri);
	}

	return uri;
}
