/*
 * Copyright (c) 2017-2019 gnome-mpv
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

#include <string.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>

#include "celluloid-player.h"
#include "celluloid-player-private.h"
#include "celluloid-player-options.h"
#include "celluloid-marshal.h"
#include "celluloid-metadata-cache.h"
#include "celluloid-mpv-wrapper.h"
#include "celluloid-def.h"

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec );

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec );

static void
finalize(GObject *object);

static void
autofit(CelluloidPlayer *player);

static void
metadata_update(CelluloidPlayer *player, gint64 pos);

static void
mpv_event_notify(CelluloidMpv *mpv, gint event_id, gpointer event_data);

static void
mpv_log_message(	CelluloidMpv *mpv,
			mpv_log_level log_level,
			const gchar *prefix,
			const gchar *text );

static void
mpv_property_changed(CelluloidMpv *mpv, const gchar *name, gpointer value);

static void
observe_properties(CelluloidMpv *mpv);

static void
apply_default_options(CelluloidMpv *mpv);

static void
initialize(CelluloidMpv *mpv);

static gint
apply_options_array_string(CelluloidMpv *mpv, gchar *args);

static void
apply_extra_options(CelluloidPlayer *player);

static void
load_file(CelluloidMpv *mpv, const gchar *uri, gboolean append);

static void
reset(CelluloidMpv *mpv);

static void
load_input_conf(CelluloidPlayer *player, const gchar *input_conf);

static void
load_config_file(CelluloidMpv *mpv);

static void
load_input_config_file(CelluloidPlayer *player);

static void
load_scripts(CelluloidPlayer *player);

static CelluloidTrack *
parse_track_entry(mpv_node_list *node);

static void
add_file_to_playlist(CelluloidPlayer *player, const gchar *uri);

static void
load_from_playlist(CelluloidPlayer *player);

static CelluloidPlaylistEntry *
parse_playlist_entry(mpv_node_list *node);

static void
update_playlist(CelluloidPlayer *player);

static void
update_metadata(CelluloidPlayer *player);

static void
update_track_list(CelluloidPlayer *player);

static void
cache_update_handler(	CelluloidMetadataCache *cache,
			const gchar *uri,
			gpointer data );

static void
mount_added_handler(GVolumeMonitor *monitor, GMount *mount, gpointer data);

static void
volume_removed_handler(GVolumeMonitor *monitor, GVolume *volume, gpointer data);

static void
guess_content_handler(GMount *mount, GAsyncResult *res, gpointer data);

G_DEFINE_TYPE_WITH_PRIVATE(CelluloidPlayer, celluloid_player, CELLULOID_TYPE_MPV)

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidPlayerPrivate *priv = get_private(object);

	switch(property_id)
	{
		case PROP_PLAYLIST:
		case PROP_METADATA:
		case PROP_TRACK_LIST:
		case PROP_DISC_LIST:
		g_critical("Attempted to set read-only property");
		break;

		case PROP_EXTRA_OPTIONS:
		g_free(priv->extra_options);
		priv->extra_options = g_value_dup_string(value);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec )
{
	CelluloidPlayerPrivate *priv = get_private(object);

	switch(property_id)
	{
		case PROP_PLAYLIST:
		g_value_set_pointer(value, priv->playlist);
		break;

		case PROP_METADATA:
		g_value_set_pointer(value, priv->metadata);
		break;

		case PROP_TRACK_LIST:
		g_value_set_pointer(value, priv->track_list);
		break;

		case PROP_DISC_LIST:
		g_value_set_pointer(value, priv->disc_list);
		break;

		case PROP_EXTRA_OPTIONS:
		g_value_set_string(value, priv->extra_options);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
finalize(GObject *object)
{
	CelluloidPlayerPrivate *priv = get_private(object);

	if(priv->tmp_input_config)
	{
		g_unlink(priv->tmp_input_config);
	}

	g_free(priv->tmp_input_config);
	g_free(priv->extra_options);
	g_ptr_array_free(priv->playlist, TRUE);
	g_ptr_array_free(priv->metadata, TRUE);
	g_ptr_array_free(priv->track_list, TRUE);
	g_ptr_array_free(priv->disc_list, TRUE);

	G_OBJECT_CLASS(celluloid_player_parent_class)->finalize(object);
}

static void
autofit(CelluloidPlayer *player)
{
}

static void
metadata_update(CelluloidPlayer *player, gint64 pos)
{
}

static void
mpv_event_notify(CelluloidMpv *mpv, gint event_id, gpointer event_data)
{
	CelluloidPlayerPrivate *priv = get_private(mpv);

	if(event_id == MPV_EVENT_START_FILE)
	{
		gboolean vo_configured = FALSE;

		celluloid_mpv_get_property(	mpv,
					"vo-configured",
					MPV_FORMAT_FLAG,
					&vo_configured );

		/* If the vo is not configured yet, save the content of mpv's
		 * playlist. This will be loaded again when the vo is
		 * configured.
		 */
		if(!vo_configured)
		{
			update_playlist(CELLULOID_PLAYER(mpv));
		}
	}
	else if(event_id == MPV_EVENT_END_FILE)
	{
		if(priv->loaded)
		{
			priv->new_file = FALSE;
		}
	}
	else if(event_id == MPV_EVENT_IDLE)
	{
		priv->loaded = FALSE;
	}
	else if(event_id == MPV_EVENT_FILE_LOADED)
	{
		priv->loaded = TRUE;
	}
	else if(event_id == MPV_EVENT_VIDEO_RECONFIG)
	{
		if(priv->new_file)
		{
			g_signal_emit_by_name(CELLULOID_PLAYER(mpv), "autofit");
		}
	}

	CELLULOID_MPV_CLASS(celluloid_player_parent_class)
		->mpv_event_notify(mpv, event_id, event_data);
}

static void
mpv_log_message(	CelluloidMpv *mpv,
			mpv_log_level log_level,
			const gchar *prefix,
			const gchar *text )
{
	CelluloidPlayerPrivate *priv = get_private(mpv);
	gsize prefix_len = strlen(prefix);
	gboolean found = FALSE;
	const gchar *iter_prefix;
	gpointer iter_level;
	GHashTableIter iter;

	g_hash_table_iter_init(&iter, priv->log_levels);

	while(	!found &&
		g_hash_table_iter_next(	&iter,
					(gpointer *)&iter_prefix,
					&iter_level) )
	{
		gsize iter_prefix_len = strlen(iter_prefix);
		gint cmp = strncmp(	iter_prefix,
					prefix,
					(prefix_len < iter_prefix_len)?
					prefix_len:iter_prefix_len );

		/* Allow both exact match and prefix match */
		if(cmp == 0
		&& (iter_prefix_len == prefix_len
		|| (iter_prefix_len < prefix_len
		&& prefix[iter_prefix_len] == '/')))
		{
			found = TRUE;
		}
	}

	if(!found || (gint)log_level <= GPOINTER_TO_INT(iter_level))
	{
		gchar *buf = g_strdup(text);
		gsize len = strlen(buf);

		if(len > 1)
		{
			/* g_message() automatically adds a newline
			 * character when using the default log handler,
			 * but log messages from mpv already come
			 * terminated with a newline character so we
			 * need to take it out.
			 */
			if(buf[len-1] == '\n')
			{
				buf[len-1] = '\0';
			}

			g_message("[%s] %s", prefix, buf);
		}

		g_free(buf);
	}

	CELLULOID_MPV_CLASS(celluloid_player_parent_class)
		->mpv_log_message(mpv, log_level, prefix, text);
}

static void
mpv_property_changed(CelluloidMpv *mpv, const gchar *name, gpointer value)
{
	CelluloidPlayer *player = CELLULOID_PLAYER(mpv);
	CelluloidPlayerPrivate *priv = get_private(mpv);

	if(g_strcmp0(name, "pause") == 0)
	{
		gboolean idle_active = FALSE;
		gboolean pause = value?*((int *)value):TRUE;

		celluloid_mpv_get_property
			(mpv, "idle-active", MPV_FORMAT_FLAG, &idle_active);

		if(idle_active && !pause && !priv->init_vo_config)
		{
			load_from_playlist(player);
		}
	}
	else if(g_strcmp0(name, "playlist") == 0)
	{
		gboolean idle_active = FALSE;
		gboolean was_empty = FALSE;

		celluloid_mpv_get_property
			(mpv, "idle-active", MPV_FORMAT_FLAG, &idle_active);

		was_empty =	priv->init_vo_config ||
				priv->playlist->len == 0;

		if(!idle_active && !priv->init_vo_config)
		{
			update_playlist(player);
		}

		/* Check if we're transitioning from empty playlist to non-empty
		 * playlist.
		 */
		if(was_empty && priv->playlist->len > 0)
		{
			celluloid_mpv_set_property_flag(mpv, "pause", FALSE);
		}
	}
	else if(g_strcmp0(name, "metadata") == 0)
	{
		update_metadata(player);
	}
	else if(g_strcmp0(name, "track-list") == 0)
	{
		update_track_list(player);
	}
	else if(g_strcmp0(name, "vo-configured") == 0)
	{
		if(priv->init_vo_config)
		{
			priv->init_vo_config = FALSE;
			load_scripts(player);
			load_from_playlist(player);
		}
	}

	CELLULOID_MPV_CLASS(celluloid_player_parent_class)
		->mpv_property_changed(mpv, name, value);
}

static void
observe_properties(CelluloidMpv *mpv)
{
	celluloid_mpv_observe_property(mpv, 0, "aid", MPV_FORMAT_STRING);
	celluloid_mpv_observe_property(mpv, 0, "vid", MPV_FORMAT_STRING);
	celluloid_mpv_observe_property(mpv, 0, "sid", MPV_FORMAT_STRING);
	celluloid_mpv_observe_property(mpv, 0, "chapters", MPV_FORMAT_INT64);
	celluloid_mpv_observe_property(mpv, 0, "core-idle", MPV_FORMAT_FLAG);
	celluloid_mpv_observe_property(mpv, 0, "idle-active", MPV_FORMAT_FLAG);
	celluloid_mpv_observe_property(mpv, 0, "border", MPV_FORMAT_FLAG);
	celluloid_mpv_observe_property(mpv, 0, "fullscreen", MPV_FORMAT_FLAG);
	celluloid_mpv_observe_property(mpv, 0, "pause", MPV_FORMAT_FLAG);
	celluloid_mpv_observe_property(mpv, 0, "loop", MPV_FORMAT_STRING);
	celluloid_mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);
	celluloid_mpv_observe_property(mpv, 0, "media-title", MPV_FORMAT_STRING);
	celluloid_mpv_observe_property(mpv, 0, "metadata", MPV_FORMAT_NODE);
	celluloid_mpv_observe_property(mpv, 0, "playlist", MPV_FORMAT_NODE);
	celluloid_mpv_observe_property(mpv, 0, "playlist-count", MPV_FORMAT_INT64);
	celluloid_mpv_observe_property(mpv, 0, "playlist-pos", MPV_FORMAT_INT64);
	celluloid_mpv_observe_property(mpv, 0, "speed", MPV_FORMAT_DOUBLE);
	celluloid_mpv_observe_property(mpv, 0, "track-list", MPV_FORMAT_NODE);
	celluloid_mpv_observe_property(mpv, 0, "vo-configured", MPV_FORMAT_FLAG);
	celluloid_mpv_observe_property(mpv, 0, "volume", MPV_FORMAT_DOUBLE);
	celluloid_mpv_observe_property(mpv, 0, "volume-max", MPV_FORMAT_DOUBLE);
	celluloid_mpv_observe_property(mpv, 0, "window-scale", MPV_FORMAT_DOUBLE);
}

static void
apply_default_options(CelluloidMpv *mpv)
{
	gchar *config_dir = get_config_dir_path();
	gchar *watch_dir = get_watch_dir_path();
	const gchar *screenshot_dir =	g_get_user_special_dir
					(G_USER_DIRECTORY_PICTURES);

	const struct
	{
		const gchar *name;
		const gchar *value;
	}
	options[] = {	{"vo", "opengl,vdpau,vaapi,xv,x11,libmpv,"},
			{"osd-level", "1"},
			{"softvol", "yes"},
			{"force-window", "immediate"},
			{"input-default-bindings", "yes"},
			{"audio-client-name", ICON_NAME},
			{"title", "${media-title}"},
			{"autofit-larger", "75%"},
			{"window-scale", "1"},
			{"pause", "yes"},
			{"ytdl", "yes"},
			{"ytdl-raw-options", "yes-playlist="},
			{"load-scripts", "no"},
			{"osd-bar", "no"},
			{"input-cursor", "no"},
			{"cursor-autohide", "no"},
			{"softvol-max", "100"},
			{"config-dir", config_dir},
			{"watch-later-directory", watch_dir},
			{"screenshot-directory", screenshot_dir},
			{"screenshot-template", "celluloid-shot%n"},
			{NULL, NULL} };

	for(gint i = 0; options[i].name; i++)
	{
		g_debug(	"Applying default option --%s=%s",
				options[i].name,
				options[i].value );

		celluloid_mpv_set_option_string
			(mpv, options[i].name, options[i].value);
	}

	g_free(config_dir);
	g_free(watch_dir);
}

static void
initialize(CelluloidMpv *mpv)
{
	CelluloidPlayer *player = CELLULOID_PLAYER(mpv);
	GSettings *win_settings = g_settings_new(CONFIG_WIN_STATE);
	gdouble volume = g_settings_get_double(win_settings, "volume")*100;

	apply_default_options(mpv);
	load_config_file(mpv);
	load_input_config_file(player);
	apply_extra_options(player);
	observe_properties(mpv);

	CELLULOID_MPV_CLASS(celluloid_player_parent_class)->initialize(mpv);

	g_debug("Setting volume to %f", volume);
	celluloid_mpv_set_property(mpv, "volume", MPV_FORMAT_DOUBLE, &volume);

	g_object_unref(win_settings);
}

static gint
apply_options_array_string(CelluloidMpv *mpv, gchar *args)
{
	gint fail_count = 0;
	gchar **tokens = g_regex_split_simple(	"(^|\\s+)--",
						args,
						G_REGEX_NO_AUTO_CAPTURE,
						0 );

	/* Skip the first token if it's non-NULL, since it is just going to be
	 * empty string for any valid args.
	 */
	for(gint i = tokens[0]?1:0; tokens[i]; i++)
	{
		gchar **parts = g_strsplit(g_strchomp(tokens[i]), "=", 2);
		const gchar *option = parts[0];
		const gchar *value = (option?parts[1]:NULL)?:"";

		g_debug("Applying option: --%s", tokens[i]);

		if(celluloid_mpv_set_option_string(mpv, option, value) < 0)
		{
			fail_count++;

			g_warning("Failed to apply option: --%s\n", tokens[i]);
		}

		g_strfreev(parts);
	}

	g_strfreev(tokens);

	return fail_count*(-1);
}

static void
apply_extra_options(CelluloidPlayer *player)
{
	CelluloidPlayerPrivate *priv = get_private(player);
	CelluloidMpv *mpv = CELLULOID_MPV(player);
	gchar *extra_options = priv->extra_options;

	g_debug("Applying extra mpv options: %s", extra_options);

	/* Apply extra options */
	if(extra_options && apply_options_array_string(mpv, extra_options) < 0)
	{
		const gchar *msg = _("Failed to apply one or more MPV options.");
		g_signal_emit_by_name(mpv, "error", msg);
	}
}

static void
load_file(CelluloidMpv *mpv, const gchar *uri, gboolean append)
{
	CelluloidPlayer *player = CELLULOID_PLAYER(mpv);
	CelluloidPlayerPrivate *priv = get_private(mpv);
	gboolean ready = FALSE;
	gboolean idle_active = FALSE;

	g_object_get(mpv, "ready", &ready, NULL);

	celluloid_mpv_get_property
		(mpv, "idle-active", MPV_FORMAT_FLAG, &idle_active);

	if(idle_active || !ready)
	{
		if(!append)
		{
			priv->new_file = TRUE;
			g_ptr_array_set_size(priv->playlist, 0);
		}

		add_file_to_playlist(player, uri);
	}
	else
	{
		if(!append)
		{
			priv->new_file = TRUE;
			priv->loaded = FALSE;
		}

		CELLULOID_MPV_CLASS(celluloid_player_parent_class)
			->load_file(mpv, uri, append);
	}

	/* Playlist items added when mpv is idle doesn't get added directly to
	 * its internal playlist, so the property change signal won't be fired.
	 * We need to emit notify signal here manually to ensure that the
	 * playlist widget gets updated.
	 */
	if(idle_active)
	{
		g_object_notify(G_OBJECT(player), "playlist");
	}
}

static void
reset(CelluloidMpv *mpv)
{
	gboolean idle_active = FALSE;
	gint64 playlist_pos = 0;
	gdouble volume = 0;

	celluloid_mpv_get_property
		(mpv, "idle-active", MPV_FORMAT_FLAG, &idle_active);
	celluloid_mpv_get_property
		(mpv, "playlist-pos", MPV_FORMAT_INT64, &playlist_pos);
	celluloid_mpv_get_property
		(mpv, "volume", MPV_FORMAT_DOUBLE, &volume);

	CELLULOID_MPV_CLASS(celluloid_player_parent_class)->reset(mpv);

	load_scripts(CELLULOID_PLAYER(mpv));

	if(!idle_active)
	{
		load_from_playlist(CELLULOID_PLAYER(mpv));
	}

	if(playlist_pos > 0)
	{
		celluloid_mpv_set_property
			(mpv, "playlist-pos", MPV_FORMAT_INT64, &playlist_pos);
	}

	celluloid_mpv_set_property(mpv, "volume", MPV_FORMAT_DOUBLE, &volume);
}

static void
load_input_conf(CelluloidPlayer *player, const gchar *input_conf)
{
	CelluloidPlayerPrivate *priv = get_private(player);
	const gchar *default_keybinds[] = DEFAULT_KEYBINDS;
	gchar *tmp_path;
	FILE *tmp_file;
	gint tmp_fd;

	if(priv->tmp_input_config)
	{
		g_unlink(priv->tmp_input_config);
		g_free(priv->tmp_input_config);
	}

	tmp_fd = g_file_open_tmp("."BIN_NAME"-XXXXXX", &tmp_path, NULL);
	tmp_file = fdopen(tmp_fd, "w");
	priv->tmp_input_config = tmp_path;

	if(!tmp_file)
	{
		g_error("Failed to open temporary input config file");
	}

	for(gint i = 0; default_keybinds[i]; i++)
	{
		const gsize len = strlen(default_keybinds[i]);
		gsize write_size = fwrite(default_keybinds[i], len, 1, tmp_file);
		gint rc = fputc('\n', tmp_file);

		if(write_size != 1 || rc != '\n')
		{
			g_error(	"Error writing default keybindings to "
					"temporary input config file" );
		}
	}

	g_debug("Using temporary input config file: %s", tmp_path);

	celluloid_mpv_set_option_string
		(CELLULOID_MPV(player), "input-conf", tmp_path);

	if(input_conf && strlen(input_conf) > 0)
	{
		const gsize buf_size = 65536;
		void *buf = g_malloc(buf_size);
		FILE *src_file = g_fopen(input_conf, "r");
		gsize read_size = buf_size;

		if(!src_file)
		{
			g_warning(	"Cannot open input config file: %s",
					input_conf );
		}

		while(src_file && read_size == buf_size)
		{
			read_size = fread(buf, 1, buf_size, src_file);

			if(read_size != fwrite(buf, 1, read_size, tmp_file))
			{
				g_error(	"Error writing requested input "
						"config file to temporary "
						"input config file" );
			}
		}

		g_info("Loaded input config file: %s", input_conf);

		g_free(buf);
	}

	fclose(tmp_file);
}

static void
load_config_file(CelluloidMpv *mpv)
{
	GSettings *settings = g_settings_new(CONFIG_ROOT);

	if(g_settings_get_boolean(settings, "mpv-config-enable"))
	{
		gchar *mpv_conf =	g_settings_get_string
					(settings, "mpv-config-file");

		g_info("Loading config file: %s", mpv_conf);
		celluloid_mpv_load_config_file(mpv, mpv_conf);

		g_free(mpv_conf);
	}

	g_object_unref(settings);
}

static void
load_input_config_file(CelluloidPlayer *player)
{
	GSettings *settings = g_settings_new(CONFIG_ROOT);
	gchar *input_conf = NULL;

	if(g_settings_get_boolean(settings, "mpv-input-config-enable"))
	{
		input_conf =	g_settings_get_string
				(settings, "mpv-input-config-file");

		g_info("Loading input config file: %s", input_conf);
	}

	load_input_conf(player, input_conf);

	g_free(input_conf);
	g_object_unref(settings);
}

static void
load_scripts(CelluloidPlayer *player)
{
	gchar *path = get_scripts_dir_path();
	GDir *dir = g_dir_open(path, 0, NULL);

	if(dir)
	{
		const gchar *name;

		do
		{
			gchar *full_path;

			name = g_dir_read_name(dir);
			full_path = g_build_filename(path, name, NULL);

			if(g_file_test(full_path, G_FILE_TEST_IS_REGULAR))
			{
				const gchar *cmd[]
					= {"load-script", full_path, NULL};

				g_info("Loading script: %s", full_path);
				celluloid_mpv_command
					(CELLULOID_MPV(player), cmd);
			}

			g_free(full_path);
		}
		while(name);

		g_dir_close(dir);
	}
	else
	{
		g_warning("Failed to open scripts directory: %s", path);
	}

	g_free(path);
}

static CelluloidTrack *
parse_track_entry(mpv_node_list *node)
{
	CelluloidTrack *entry = celluloid_track_new();

	for(gint i = 0; i < node->num; i++)
	{
		if(g_strcmp0(node->keys[i], "type") == 0)
		{
			const gchar *type = node->values[i].u.string;

			if(g_strcmp0(type, "audio") == 0)
			{
				entry->type = TRACK_TYPE_AUDIO;
			}
			else if(g_strcmp0(type, "video") == 0)
			{
				entry->type = TRACK_TYPE_VIDEO;
			}
			else if(g_strcmp0(type, "sub") == 0)
			{
				entry->type = TRACK_TYPE_SUBTITLE;
			}
		}
		else if(g_strcmp0(node->keys[i], "title") == 0)
		{
			entry->title = g_strdup(node->values[i].u.string);
		}
		else if(g_strcmp0(node->keys[i], "lang") == 0)
		{
			entry->lang = g_strdup(node->values[i].u.string);
		}
		else if(g_strcmp0(node->keys[i], "id") == 0)
		{
			entry->id = node->values[i].u.int64;
		}
	}

	return entry;
}

static void
add_file_to_playlist(CelluloidPlayer *player, const gchar *uri)
{
	CelluloidPlaylistEntry *entry = celluloid_playlist_entry_new(uri, NULL);

	g_ptr_array_add(get_private(player)->playlist, entry);
}

static void load_from_playlist(CelluloidPlayer *player)
{
	CelluloidMpv *mpv = CELLULOID_MPV(player);
	GPtrArray *playlist = get_private(player)->playlist;

	for(guint i = 0; playlist && i < playlist->len; i++)
	{
		CelluloidPlaylistEntry *entry = g_ptr_array_index(playlist, i);

		/* Do not append on first iteration */
		CELLULOID_MPV_CLASS(celluloid_player_parent_class)
			->load_file(mpv, entry->filename, i != 0);
	}
}

static CelluloidPlaylistEntry *
parse_playlist_entry(mpv_node_list *node)
{
	const gchar *filename = NULL;
	const gchar *title = NULL;

	for(gint i = 0; i < node->num; i++)
	{
		if(g_strcmp0(node->keys[i], "filename") == 0)
		{
			filename = node->values[i].u.string;
		}
		else if(g_strcmp0(node->keys[i], "title") == 0)
		{
			title = node->values[i].u.string;
		}
	}

	return celluloid_playlist_entry_new(filename, title);
}

static void
update_playlist(CelluloidPlayer *player)
{
	CelluloidPlayerPrivate *priv;
	GSettings *settings;
	gboolean prefetch_metadata;
	const mpv_node_list *org_list;
	mpv_node playlist;

	priv = get_private(player);
	settings = g_settings_new(CONFIG_ROOT);
	prefetch_metadata = g_settings_get_boolean(settings, "prefetch-metadata");

	g_ptr_array_set_size(priv->playlist, 0);

	celluloid_mpv_get_property
		(CELLULOID_MPV(player), "playlist", MPV_FORMAT_NODE, &playlist);

	org_list = playlist.u.list;

	if(playlist.format == MPV_FORMAT_NODE_ARRAY)
	{
		for(gint i = 0; i < org_list->num; i++)
		{
			CelluloidPlaylistEntry *entry;

			entry = parse_playlist_entry(org_list->values[i].u.list);

			if(!entry->title && prefetch_metadata)
			{
				CelluloidMetadataCacheEntry *cache_entry;

				cache_entry =	celluloid_metadata_cache_lookup
						(priv->cache, entry->filename);
				entry->title =	g_strdup(cache_entry->title);
			}

			g_ptr_array_add(priv->playlist, entry);
		}

		mpv_free_node_contents(&playlist);
		g_object_notify(G_OBJECT(player), "playlist");
	}


	if(prefetch_metadata)
	{
		celluloid_metadata_cache_load_playlist
			(priv->cache, priv->playlist);
	}

	g_object_unref(settings);
}

static void
update_metadata(CelluloidPlayer *player)
{
	CelluloidPlayerPrivate *priv = get_private(player);
	mpv_node_list *org_list = NULL;
	mpv_node metadata;

	g_ptr_array_set_size(priv->metadata, 0);

	celluloid_mpv_get_property
		(CELLULOID_MPV(player), "metadata", MPV_FORMAT_NODE, &metadata);

	org_list = metadata.u.list;

	if(metadata.format == MPV_FORMAT_NODE_MAP && org_list->num > 0)
	{
		for(gint i = 0; i < org_list->num; i++)
		{
			const gchar *key;
			mpv_node value;

			key = org_list->keys[i];
			value = org_list->values[i];

			if(value.format == MPV_FORMAT_STRING)
			{
				CelluloidMetadataEntry *entry;

				entry =	celluloid_metadata_entry_new
					(key, value.u.string);

				g_ptr_array_add(priv->metadata, entry);
			}
			else
			{
				g_warning(	"Ignored metadata field %s "
						"with unexpected format %d",
						key,
						value.format );
			}
		}

		mpv_free_node_contents(&metadata);
		g_object_notify(G_OBJECT(player), "metadata");
	}
}

static void
update_track_list(CelluloidPlayer *player)
{
	CelluloidPlayerPrivate *priv = get_private(player);
	mpv_node_list *org_list = NULL;
	mpv_node track_list;

	g_ptr_array_set_size(priv->track_list, 0);
	celluloid_mpv_get_property(	CELLULOID_MPV(player),
					"track-list",
					MPV_FORMAT_NODE,
					&track_list );

	org_list = track_list.u.list;

	if(track_list.format == MPV_FORMAT_NODE_ARRAY)
	{
		for(gint i = 0; i < org_list->num; i++)
		{
			CelluloidTrack *entry =	parse_track_entry
						(org_list->values[i].u.list);

			g_ptr_array_add(priv->track_list, entry);
		}

		mpv_free_node_contents(&track_list);
		g_object_notify(G_OBJECT(player), "track-list");
	}
}

static void
cache_update_handler(	CelluloidMetadataCache *cache,
			const gchar *uri,
			gpointer data )
{
	CelluloidPlayerPrivate *priv = get_private(data);

	for(guint i = 0; i < priv->playlist->len; i++)
	{
		CelluloidPlaylistEntry *entry =
			g_ptr_array_index(priv->playlist, i);

		if(g_strcmp0(entry->filename, uri) == 0)
		{
			CelluloidMetadataCacheEntry *cache_entry =
				celluloid_metadata_cache_lookup(cache, uri);

			g_free(entry->title);
			entry->title = g_strdup(cache_entry->title);

			g_signal_emit_by_name
				(data, "metadata-update", (gint64)i);
		}
	}
}

static void
mount_added_handler(GVolumeMonitor *monitor, GMount *mount, gpointer data)
{
	GAsyncReadyCallback callback
		= (GAsyncReadyCallback)guess_content_handler;
	gchar *name = g_mount_get_name(mount);

	g_debug("Mount %s added", name);
	g_mount_guess_content_type(mount, FALSE, NULL, callback, data);

	g_free(name);
}

static void
volume_removed_handler(GVolumeMonitor *monitor, GVolume *volume, gpointer data)
{
	GPtrArray *disc_list = get_private(data)->disc_list;
	gboolean found = FALSE;
	gchar *path = g_volume_get_identifier(volume, "unix-device");

	for(guint i = 0; !found && path && i < disc_list->len; i++)
	{
		CelluloidDisc *disc = g_ptr_array_index(disc_list, i);

		if(g_str_has_suffix(disc->uri, path))
		{
			g_ptr_array_remove_index(disc_list, i);

			found = TRUE;
		}
	}

	g_object_notify(G_OBJECT(data), "disc-list");
}

static void
guess_content_handler(GMount *mount, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	gchar **types = g_mount_guess_content_type_finish(mount, res, &error);

	if(error)
	{
		gchar *name = g_mount_get_name(mount);

		g_warning(	"Failed to guess content type for mount %s: %s",
				name,
				error->message );

		g_free(name);
	}
	else
	{
		CelluloidPlayer *player = data;
		const gchar *protocol = NULL;
		GVolume *volume = g_mount_get_volume(mount);
		gchar *path = 	volume ?
				g_volume_get_identifier(volume, "unix-device") :
				NULL;

		for(gint i = 0; path && !protocol && types && types[i]; i++)
		{
			if(g_strcmp0(types[i], "x-content/video-blueray") == 0)
			{
				protocol = "bd";
			}
			else if(g_strcmp0(types[i], "x-content/video-dvd") == 0)
			{
				protocol = "dvd";
			}
			else if(g_strcmp0(types[i], "x-content/audio-cdda") == 0)
			{
				protocol = "cdda";
			}
		}

		if(path && protocol)
		{
			CelluloidDisc *disc = celluloid_disc_new();

			disc->uri = g_strdup_printf("%s:///%s", protocol, path);
			disc->label = g_mount_get_name(mount);

			g_ptr_array_add(get_private(player)->disc_list, disc);
			g_object_notify(G_OBJECT(player), "disc-list");
		}

		g_free(path);
		g_clear_object(&volume);
	}

	g_strfreev(types);
}

static void
celluloid_player_class_init(CelluloidPlayerClass *klass)
{
	CelluloidMpvClass *mpv_class = CELLULOID_MPV_CLASS(klass);
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	klass->autofit = autofit;
	klass->metadata_update = metadata_update;
	mpv_class->mpv_event_notify = mpv_event_notify;
	mpv_class->mpv_log_message = mpv_log_message;
	mpv_class->mpv_property_changed = mpv_property_changed;
	mpv_class->initialize = initialize;
	mpv_class->load_file = load_file;
	mpv_class->reset = reset;
	obj_class->set_property = set_property;
	obj_class->get_property = get_property;
	obj_class->finalize = finalize;

	pspec = g_param_spec_pointer
		(	"playlist",
			"Playlist",
			"The playlist",
			G_PARAM_READABLE );
	g_object_class_install_property(obj_class, PROP_PLAYLIST, pspec);

	pspec = g_param_spec_pointer
		(	"metadata",
			"Metadata",
			"The metadata tags of the current file",
			G_PARAM_READABLE );
	g_object_class_install_property(obj_class, PROP_METADATA, pspec);

	pspec = g_param_spec_pointer
		(	"track-list",
			"Track list",
			"Audio, video, and subtitle tracks of the current file",
			G_PARAM_READABLE );
	g_object_class_install_property(obj_class, PROP_TRACK_LIST, pspec);

	pspec = g_param_spec_pointer
		(	"disc-list",
			"Disc list",
			"List of mounted discs",
			G_PARAM_READABLE );
	g_object_class_install_property(obj_class, PROP_DISC_LIST, pspec);

	pspec = g_param_spec_string
		(	"extra-options",
			"Extra options",
			"Extra options to pass to mpv",
			NULL,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_EXTRA_OPTIONS, pspec);

	g_signal_new(	"autofit",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
	g_signal_new(	"metadata-update",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_gen_marshal_VOID__INT64,
			G_TYPE_NONE,
			1,
			G_TYPE_INT64 );
}

static void
celluloid_player_init(CelluloidPlayer *player)
{
	CelluloidPlayerPrivate *priv = get_private(player);

	priv->cache =		celluloid_metadata_cache_new();
	priv->monitor =		g_volume_monitor_get();
	priv->playlist =	g_ptr_array_new_with_free_func
				((GDestroyNotify)celluloid_playlist_entry_free);
	priv->metadata =	g_ptr_array_new_with_free_func
				((GDestroyNotify)celluloid_metadata_entry_free);
	priv->track_list =	g_ptr_array_new_with_free_func
				((GDestroyNotify)celluloid_track_free);
	priv->disc_list =	g_ptr_array_new_with_free_func
				((GDestroyNotify)celluloid_disc_free);
	priv->log_levels =	g_hash_table_new_full
				(g_str_hash, g_str_equal, g_free, NULL);

	priv->loaded = FALSE;
	priv->new_file = TRUE;
	priv->init_vo_config = TRUE;
	priv->tmp_input_config = NULL;
	priv->extra_options = NULL;

	g_signal_connect(	priv->cache,
				"update",
				G_CALLBACK(cache_update_handler),
				player );
	g_signal_connect(	priv->monitor,
				"mount-added",
				G_CALLBACK(mount_added_handler),
				player );

	// We need to connect to volume-removed instead of mount-removed because
	// we need to use the path of the volume to figure out which entry in
	// disc_list to remove. However, by the time mount-removed fires, the
	// mount would no longer be associated with its volume. This works fine
	// since we only add mounts with associated volumes to disc_list.
	g_signal_connect(	priv->monitor,
				"volume-removed",
				G_CALLBACK(volume_removed_handler),
				player );

	// Emit mount-added to fill disc_list using mounts that already exist.
	GList *mount_list = g_volume_monitor_get_mounts(priv->monitor);

	for(GList *cur = mount_list; cur; cur = g_list_next(cur))
	{
		g_signal_emit_by_name(priv->monitor, "mount-added", cur->data);
	}

	g_list_free_full(mount_list, g_object_unref);
}

CelluloidPlayer *
celluloid_player_new(gint64 wid)
{
	return CELLULOID_PLAYER(g_object_new(	celluloid_player_get_type(),
						"wid", wid,
						NULL ));
}

void
celluloid_player_set_playlist_position(CelluloidPlayer *player, gint64 position)
{
	CelluloidMpv *mpv = CELLULOID_MPV(player);
	gint64 playlist_pos = 0;

	celluloid_mpv_get_property
		(mpv, "playlist-pos", MPV_FORMAT_INT64, &playlist_pos);

	if(position != playlist_pos)
	{
		celluloid_mpv_set_property
			(mpv, "playlist-pos", MPV_FORMAT_INT64, &position);
	}
}

void
celluloid_player_remove_playlist_entry(CelluloidPlayer *player, gint64 position)
{
	CelluloidMpv *mpv = CELLULOID_MPV(player);
	gboolean idle_active =	celluloid_mpv_get_property_flag
				(mpv, "idle-active");
	gint64 playlist_count = 0;

	celluloid_mpv_get_property
		(mpv, "playlist-count", MPV_FORMAT_INT64, &playlist_count);

	/* mpv doesn't send playlist change signal before going idle when the
	 * last playlist entry is removed, so the internal playlist needs to be
	 * directly updated.
	 */
	if(idle_active || playlist_count == 1)
	{
		g_ptr_array_remove_index(	get_private(player)->playlist,
						(guint)position );
		g_object_notify(G_OBJECT(player), "playlist");
	}

	if(!idle_active)
	{
		const gchar *cmd[] = {"playlist_remove", NULL, NULL};
		gchar *index_str =	g_strdup_printf
					("%" G_GINT64_FORMAT, position);

		cmd[1] = index_str;

		celluloid_mpv_command(CELLULOID_MPV(player), cmd);
		g_free(index_str);
	}
}

void
celluloid_player_move_playlist_entry(	CelluloidPlayer *player,
					gint64 src,
					gint64 dst )
{
	CelluloidMpv *mpv = CELLULOID_MPV(player);
	gboolean idle_active =	celluloid_mpv_get_property_flag
				(mpv, "idle-active");

	if(idle_active)
	{
		GPtrArray *playlist;
		CelluloidPlaylistEntry **entry;

		playlist = get_private(player)->playlist;
		entry = (CelluloidPlaylistEntry **)
			&g_ptr_array_index(playlist, src);

		g_ptr_array_insert(playlist, (gint)dst, *entry);

		/* Prevent the entry from being freed */
		*entry = NULL;
		g_ptr_array_remove_index(playlist, (guint)((src > dst)?--src:src));
	}
	else
	{
		const gchar *cmd[] =	{"playlist_move", NULL, NULL, NULL};
		gchar *src_str =	g_strdup_printf
					("%" G_GINT64_FORMAT, (src > dst)?--src:src);
		gchar *dst_str =	g_strdup_printf
					("%" G_GINT64_FORMAT, dst);

		cmd[1] = src_str;
		cmd[2] = dst_str;

		celluloid_mpv_command(CELLULOID_MPV(player), cmd);

		g_free(src_str);
		g_free(dst_str);
	}
}

void
celluloid_player_set_log_level(	CelluloidPlayer *player,
				const gchar *prefix,
				const gchar *level )
{
	const struct
	{
		gchar *name;
		mpv_log_level level;
	}
	level_map[] = {	{"no", MPV_LOG_LEVEL_NONE},
			{"fatal", MPV_LOG_LEVEL_FATAL},
			{"error", MPV_LOG_LEVEL_ERROR},
			{"warn", MPV_LOG_LEVEL_WARN},
			{"info", MPV_LOG_LEVEL_INFO},
			{"v", MPV_LOG_LEVEL_V},
			{"debug", MPV_LOG_LEVEL_DEBUG},
			{"trace", MPV_LOG_LEVEL_TRACE},
			{NULL, MPV_LOG_LEVEL_NONE} };

	GHashTableIter iter;
	gpointer iter_level = GINT_TO_POINTER(DEFAULT_LOG_LEVEL);
	CelluloidPlayerPrivate *priv = get_private(player);
	mpv_log_level max_level = DEFAULT_LOG_LEVEL;
	gboolean found = FALSE;
	gint i = 0;

	do
	{
		found = (g_strcmp0(level, level_map[i].name) == 0);
	}
	while(!found && level_map[++i].name);

	if(found && g_strcmp0(prefix, "all") != 0)
	{
		g_hash_table_replace(	priv->log_levels,
					g_strdup(prefix),
					GINT_TO_POINTER(level_map[i].level) );
	}

	max_level = level_map[i].level;

	g_hash_table_iter_init(&iter, priv->log_levels);

	while(g_hash_table_iter_next(&iter, NULL, &iter_level))
	{
		if(GPOINTER_TO_INT(iter_level) > (gint)max_level)
		{
			max_level = GPOINTER_TO_INT(iter_level);
		}
	}

	for(i = 0; level_map[i].level != max_level; i++);

	celluloid_mpv_request_log_messages
		(CELLULOID_MPV(player), level_map[i].name);
}
