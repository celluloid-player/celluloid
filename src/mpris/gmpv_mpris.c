/*
 * Copyright (c) 2015-2017 gnome-mpv
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

#include <unistd.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gmpv_mpris.h"
#include "gmpv_mpris_base.h"
#include "gmpv_mpris_player.h"
#include "gmpv_mpris_track_list.h"
#include "gmpv_def.h"

enum
{
	PROP_0,
	PROP_APP,
	N_PROPERTIES
};

struct _GmpvMpris
{
	GObject parent;
	GmpvApplication* app;
	GmpvMprisModule *base;
	GmpvMprisModule *player;
	GmpvMprisModule *track_list;
	guint name_id;
	guint player_reg_id;
	gulong *player_sig_id_list;
	gdouble pending_seek;
	GHashTable *player_prop_table;
	GDBusConnection *session_bus_conn;
};

struct _GmpvMprisClass
{
	GObjectClass parent_class;
};

G_DEFINE_TYPE(GmpvMpris, gmpv_mpris, G_TYPE_OBJECT)

static void constructed(GObject *object);
static void dispose(GObject *object);
static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec );
static void name_acquired_handler(	GDBusConnection *connection,
					const gchar *name,
					gpointer data );
static void name_lost_handler(	GDBusConnection *connection,
				const gchar *name,
				gpointer data );
static void unregister(GmpvMpris *mpris);

static void constructed(GObject *object)
{
	GmpvMpris *self = GMPV_MPRIS(object);
	gchar *name = NULL;

	/* sizeof(pid_t) can theoretically be larger than sizeof(gint64), but
	 * even then the chance of collision would be minimal.
	 */
	name =	g_strdup_printf
		(	MPRIS_BUS_NAME ".instance%" G_GINT64_FORMAT,
			ABS((gint64)getpid()) );

	self->name_id = g_bus_own_name(	G_BUS_TYPE_SESSION,
					name,
					G_BUS_NAME_OWNER_FLAGS_NONE,
					NULL,
					(GBusNameAcquiredCallback)
					name_acquired_handler,
					(GBusNameLostCallback)
					name_lost_handler,
					self,
					NULL );

	g_free(name);

	G_OBJECT_CLASS(gmpv_mpris_parent_class)->constructed(object);
}

static void dispose(GObject *object)
{
	GmpvMpris *self = GMPV_MPRIS(object);

	unregister(self);
	g_clear_object(&self->base);
	g_clear_object(&self->player);
	g_clear_object(&self->track_list);
	g_bus_unown_name(self->name_id);

	G_OBJECT_CLASS(gmpv_mpris_parent_class)->dispose(object);
}

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvMpris *self = GMPV_MPRIS(object);

	switch(property_id)
	{
		case PROP_APP:
		self->app = g_value_get_pointer(value);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}

}

static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec )
{
	GmpvMpris *self = GMPV_MPRIS(object);

	switch(property_id)
	{
		case PROP_APP:
		g_value_set_pointer(value, self->app);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}

}

static void name_acquired_handler(	GDBusConnection *connection,
					const gchar *name,
					gpointer data )
{
	GmpvMpris *self = data;

	self->session_bus_conn = connection;
	self->base = GMPV_MPRIS_MODULE(gmpv_mpris_base_new(	self->app,
								connection ));
	self->player = GMPV_MPRIS_MODULE(gmpv_mpris_player_new(	self->app,
								connection ));
	self->track_list = GMPV_MPRIS_MODULE(gmpv_mpris_track_list_new(	self->app,
									connection ));

	gmpv_mpris_module_register_interface(self->base);
	gmpv_mpris_module_register_interface(self->player);
	gmpv_mpris_module_register_interface(self->track_list);
}

static void name_lost_handler(	GDBusConnection *connection,
				const gchar *name,
				gpointer data )
{
	unregister(data);
}

static void unregister(GmpvMpris *mpris)
{
	gmpv_mpris_module_unregister_interface(mpris->base);
	gmpv_mpris_module_unregister_interface(mpris->player);
	gmpv_mpris_module_unregister_interface(mpris->track_list);
}

static void gmpv_mpris_init(GmpvMpris *mpris)
{
	mpris->app = NULL;
	mpris->base = NULL;
	mpris->player = NULL;
	mpris->track_list = NULL;
	mpris->name_id = 0;
	mpris->pending_seek = -1;
	mpris->session_bus_conn = NULL;
}

static void gmpv_mpris_class_init(GmpvMprisClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	object_class->constructed = constructed;
	object_class->set_property = set_property;
	object_class->get_property = get_property;
	object_class->dispose = dispose;

	pspec = g_param_spec_pointer
		(	"app",
			"App",
			"The GmpvApplication to use",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_APP, pspec);
}

GVariant *gmpv_mpris_build_g_variant_string_array(const gchar** list)
{
	GVariantBuilder builder;
	gint i;

	g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));

	for(i = 0; list[i]; i++)
	{
		g_variant_builder_add(&builder, "s", list[i]);
	}

	return g_variant_new("as", &builder);
}

GmpvMpris *gmpv_mpris_new(GmpvApplication *app)
{
	return GMPV_MPRIS(g_object_new(	gmpv_mpris_get_type(),
					"app", app,
					NULL ));
}
