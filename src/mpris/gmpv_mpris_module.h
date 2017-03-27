/*
 * Copyright (c) 2017 gnome-mpv
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

#ifndef MPRIS_MODULE_H
#define MPRIS_MODULE_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define GMPV_TYPE_MPRIS_MODULE gmpv_mpris_module_get_type()
G_DECLARE_DERIVABLE_TYPE(GmpvMprisModule, gmpv_mpris_module, GMPV, MPRIS_MODULE, GObject)

typedef struct _GmpvMprisProperty GmpvMprisProperty;

struct _GmpvMprisModuleClass
{
	GObjectClass parent_interface;
	void (*register_interface)(GmpvMprisModule *module);
	void (*unregister_interface)(GmpvMprisModule *module);
};

struct _GmpvMprisProperty
{
	gchar *name;
	GVariant *value;
};

void gmpv_mpris_module_emit_properties_changed(	GDBusConnection *conn,
						const gchar *iface_name,
						const GmpvMprisProperty *props );
void gmpv_mpris_module_register_interface(GmpvMprisModule *module);
void gmpv_mpris_module_unregister_interface(GmpvMprisModule *module);

G_END_DECLS

#endif
