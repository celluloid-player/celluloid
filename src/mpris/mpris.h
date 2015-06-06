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

#ifndef MPRIS_H
#define MPRIS_H

#include "common.h"

typedef struct mpris mpris;
typedef struct mpris_prop_val_pair mpris_prop_val_pair;

struct mpris
{
	gmpv_handle* gmpv_ctx;
	guint name_id;
	guint base_reg_id;
	guint player_reg_id;
	guint shutdown_sig_id;
	guint *base_sig_id_list;
	guint *player_sig_id_list;
	gdouble pending_seek;
	GHashTable *base_prop_table;
	GHashTable *player_prop_table;
	GDBusConnection *session_bus_conn;
};

struct mpris_prop_val_pair
{
	gchar *name;
	GVariant *value;
};

void mpris_emit_prop_changed(mpris *inst, const mpris_prop_val_pair *prop_list);
GVariant *mpris_build_g_variant_string_array(const gchar** list);
void mpris_init(gmpv_handle *gmpv_ctx);

#endif
