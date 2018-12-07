/*
 * Copyright (c) 2017 gnome-mpv
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

#ifndef MPRIS_MODULE_H
#define MPRIS_MODULE_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define GMPV_TYPE_MPRIS_MODULE gmpv_mpris_module_get_type()
G_DECLARE_DERIVABLE_TYPE(GmpvMprisModule, gmpv_mpris_module, GMPV, MPRIS_MODULE, GObject)

struct _GmpvMprisModuleClass
{
	GObjectClass parent_interface;
	void (*register_interface)(GmpvMprisModule *module);
	void (*unregister_interface)(GmpvMprisModule *module);
};

void gmpv_mpris_module_connect_signal(	GmpvMprisModule *module,
					gpointer instance,
					const gchar *detailed_signal,
					GCallback handler,
					gpointer data );
#define gmpv_mpris_module_set_properties(module, ...)\
	gmpv_mpris_module_set_properties_full(module, TRUE, __VA_ARGS__)
void gmpv_mpris_module_get_properties(GmpvMprisModule *module, ...);
void gmpv_mpris_module_set_properties_full(	GmpvMprisModule *module,
						gboolean send_new_value,
						... );
void gmpv_mpris_module_register(GmpvMprisModule *module);
void gmpv_mpris_module_unregister(GmpvMprisModule *module);

G_END_DECLS

#endif
