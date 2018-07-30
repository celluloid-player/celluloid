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

#include <string.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>

#include "gmpv_player.h"
#include "gmpv_player_options.h"
#include "gmpv_marshal.h"
#include "gmpv_metadata_cache.h"
#include "gmpv_mpv_wrapper.h"
#include "gmpv_def.h"

typedef struct _GmpvLogLevel GmpvLogLevel;

enum
{
	PROP_0,
	PROP_PLAYLIST,
	PROP_METADATA,
	PROP_TRACK_LIST,
	N_PROPERTIES
};

struct _GmpvLogLevel
{
	const gchar *prefix;
	mpv_log_level level;
};

struct _GmpvPlayer
{
	GmpvMpv parent;
	GmpvMetadataCache *cache;
	GPtrArray *playlist;
	GPtrArray *metadata;
	GPtrArray *track_list;
	GHashTable *log_levels;
	gboolean loaded;
	gboolean new_file;
	gboolean init_vo_config;
	gchar *tmp_input_config;
};

struct _GmpvPlayerClass
{
	GmpvMpvClass parent_class;
};

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec );
static void finalize(GObject *object);
static void mpv_event_notify(GmpvMpv *mpv, gint event_id, gpointer event_data);
static void mpv_log_message(	GmpvMpv *mpv,
				mpv_log_level log_level,
				const gchar *prefix,
				const gchar *text );
static void mpv_property_changed(	GmpvMpv *mpv,
					const gchar *name,
					gpointer value );
static void observe_properties(GmpvMpv *mpv);
static void apply_default_options(GmpvMpv *mpv);
static void initialize(GmpvMpv *mpv);
static gint apply_options_array_string(GmpvMpv *mpv, gchar *args);
static void apply_extra_options(GmpvMpv *mpv);
static void load_file(GmpvMpv *mpv, const gchar *uri, gboolean append);
static void reset(GmpvMpv *mpv);
static void load_input_conf(GmpvPlayer *player, const gchar *input_conf);
static void load_config_file(GmpvMpv *mpv);
static void load_input_config_file(GmpvPlayer *player);
static void load_scripts(GmpvPlayer *player);
static GmpvTrack *parse_track_entry(mpv_node_list *node);
static void add_file_to_playlist(GmpvPlayer *player, const gchar *uri);
static void load_from_playlist(GmpvPlayer *player);
static GmpvPlaylistEntry *parse_playlist_entry(mpv_node_list *node);
static void update_playlist(GmpvPlayer *player);
static void update_metadata(GmpvPlayer *player);
static void update_track_list(GmpvPlayer *player);
static void cache_update_handler(	GmpvMetadataCache *cache,
					const gchar *uri,
					gpointer data );

G_DEFINE_TYPE(GmpvPlayer, gmpv_player, GMPV_TYPE_MPV)

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	switch(property_id)
	{
		case PROP_PLAYLIST:
		case PROP_METADATA:
		case PROP_TRACK_LIST:
		g_critical("Attempted to set read-only property");
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
	GmpvPlayer *self = GMPV_PLAYER(object);

	switch(property_id)
	{
		case PROP_PLAYLIST:
		g_value_set_pointer(value, self->playlist);
		break;

		case PROP_METADATA:
		g_value_set_pointer(value, self->metadata);
		break;

		case PROP_TRACK_LIST:
		g_value_set_pointer(value, self->track_list);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void finalize(GObject *object)
{
	GmpvPlayer *player = GMPV_PLAYER(object);

	if(player->tmp_input_config)
	{
		g_unlink(player->tmp_input_config);
	}

	g_free(player->tmp_input_config);
	g_ptr_array_free(player->playlist, TRUE);
	g_ptr_array_free(player->metadata, TRUE);
	g_ptr_array_free(player->track_list, TRUE);

	G_OBJECT_CLASS(gmpv_player_parent_class)->finalize(object);
}

static void mpv_event_notify(GmpvMpv *mpv, gint event_id, gpointer event_data)
{
	GmpvPlayer *player = GMPV_PLAYER(mpv);

	if(event_id == MPV_EVENT_START_FILE)
	{
		gboolean vo_configured = FALSE;

		gmpv_mpv_get_property(	mpv,
					"vo-configured",
					MPV_FORMAT_FLAG,
					&vo_configured );

		/* If the vo is not configured yet, save the content of mpv's
		 * playlist. This will be loaded again when the vo is
		 * configured.
		 */
		if(!vo_configured)
		{
			update_playlist(GMPV_PLAYER(mpv));
		}
	}
	else if(event_id == MPV_EVENT_END_FILE)
	{
		if(player->loaded)
		{
			player->new_file = FALSE;
		}
	}
	else if(event_id == MPV_EVENT_IDLE)
	{
		player->loaded = FALSE;
	}
	else if(event_id == MPV_EVENT_FILE_LOADED)
	{
		player->loaded = TRUE;
	}
	else if(event_id == MPV_EVENT_VIDEO_RECONFIG)
	{
		if(player->new_file)
		{
			g_signal_emit_by_name(player, "autofit");
		}
	}

	GMPV_MPV_CLASS(gmpv_player_parent_class)
		->mpv_event_notify(mpv, event_id, event_data);
}

static void mpv_log_message(	GmpvMpv *mpv,
				mpv_log_level log_level,
				const gchar *prefix,
				const gchar *text )
{
	GmpvPlayer *player = GMPV_PLAYER(mpv);
	gsize prefix_len = strlen(prefix);
	gboolean found = FALSE;
	const gchar *iter_prefix;
	mpv_log_level iter_level;
	GHashTableIter iter;

	g_hash_table_iter_init(&iter, player->log_levels);

	while(	!found &&
		g_hash_table_iter_next(	&iter,
					(gpointer *)&iter_prefix,
					(gpointer *)&iter_level) )
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

	if(!found || (log_level <= iter_level))
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

	GMPV_MPV_CLASS(gmpv_player_parent_class)
		->mpv_log_message(mpv, log_level, prefix, text);
}

static void mpv_property_changed(	GmpvMpv *mpv,
					const gchar *name,
					gpointer value )
{
	GmpvPlayer *player = GMPV_PLAYER(mpv);

	if(g_strcmp0(name, "pause") == 0)
	{
		gboolean idle_active = FALSE;
		gboolean pause = value?*((int *)value):TRUE;

		gmpv_mpv_get_property(	mpv,
					"idle-active",
					MPV_FORMAT_FLAG,
					&idle_active );

		if(idle_active && !pause && !player->init_vo_config)
		{
			load_from_playlist(player);
		}
	}
	else if(g_strcmp0(name, "playlist") == 0)
	{
		gint64 playlist_count = 0;
		gboolean idle_active = FALSE;
		gboolean was_empty = FALSE;

		gmpv_mpv_get_property(	mpv,
					"playlist-count",
					MPV_FORMAT_INT64,
					&playlist_count );
		gmpv_mpv_get_property(	mpv,
					"idle-active",
					MPV_FORMAT_FLAG,
					&idle_active );

		was_empty = (player->playlist->len == 0);

		if(!idle_active)
		{
			update_playlist(player);
		}

		/* Check if we're transitioning from empty playlist to non-empty
		 * playlist.
		 */
		if(was_empty && player->playlist->len > 0)
		{
			gmpv_mpv_set_property_flag(mpv, "pause", FALSE);
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
		if(player->init_vo_config)
		{
			player->init_vo_config = FALSE;
			load_scripts(player);
			load_from_playlist(player);
		}
	}

	GMPV_MPV_CLASS(gmpv_player_parent_class)
		->mpv_property_changed(mpv, name, value);
}

static void observe_properties(GmpvMpv *mpv)
{
	gmpv_mpv_observe_property(mpv, 0, "aid", MPV_FORMAT_STRING);
	gmpv_mpv_observe_property(mpv, 0, "vid", MPV_FORMAT_STRING);
	gmpv_mpv_observe_property(mpv, 0, "sid", MPV_FORMAT_STRING);
	gmpv_mpv_observe_property(mpv, 0, "chapters", MPV_FORMAT_INT64);
	gmpv_mpv_observe_property(mpv, 0, "core-idle", MPV_FORMAT_FLAG);
	gmpv_mpv_observe_property(mpv, 0, "idle-active", MPV_FORMAT_FLAG);
	gmpv_mpv_observe_property(mpv, 0, "fullscreen", MPV_FORMAT_FLAG);
	gmpv_mpv_observe_property(mpv, 0, "pause", MPV_FORMAT_FLAG);
	gmpv_mpv_observe_property(mpv, 0, "loop", MPV_FORMAT_STRING);
	gmpv_mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);
	gmpv_mpv_observe_property(mpv, 0, "media-title", MPV_FORMAT_STRING);
	gmpv_mpv_observe_property(mpv, 0, "metadata", MPV_FORMAT_NODE);
	gmpv_mpv_observe_property(mpv, 0, "playlist", MPV_FORMAT_NODE);
	gmpv_mpv_observe_property(mpv, 0, "playlist-count", MPV_FORMAT_INT64);
	gmpv_mpv_observe_property(mpv, 0, "playlist-pos", MPV_FORMAT_INT64);
	gmpv_mpv_observe_property(mpv, 0, "speed", MPV_FORMAT_DOUBLE);
	gmpv_mpv_observe_property(mpv, 0, "track-list", MPV_FORMAT_NODE);
	gmpv_mpv_observe_property(mpv, 0, "vo-configured", MPV_FORMAT_FLAG);
	gmpv_mpv_observe_property(mpv, 0, "volume", MPV_FORMAT_DOUBLE);
	gmpv_mpv_observe_property(mpv, 0, "window-scale", MPV_FORMAT_DOUBLE);
}

static void apply_default_options(GmpvMpv *mpv)
{
	gchar *config_dir = get_config_dir_path();
	gchar *watch_dir = get_watch_dir_path();

	const struct
	{
		const gchar *name;
		const gchar *value;
	}
	options[] = {	{"vo", "opengl,vdpau,vaapi,xv,x11,opengl-cb,"},
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
			{"config", "no"},
			{"config-dir", config_dir},
			{"watch-later-directory", watch_dir},
			{"screenshot-template", "gnome-mpv-shot%n"},
			{NULL, NULL} };

	for(gint i = 0; options[i].name; i++)
	{
		g_debug(	"Applying default option --%s=%s",
				options[i].name,
				options[i].value );

		gmpv_mpv_set_option_string(	mpv,
						options[i].name,
						options[i].value );
	}

	g_free(config_dir);
	g_free(watch_dir);
}

static void initialize(GmpvMpv *mpv)
{

	GSettings *win_settings = g_settings_new(CONFIG_WIN_STATE);
	gdouble volume = g_settings_get_double(win_settings, "volume")*100;

	apply_default_options(mpv);
	load_config_file(mpv);
	load_input_config_file(GMPV_PLAYER(mpv));
	apply_extra_options(mpv);
	observe_properties(mpv);
	gmpv_player_options_init(GMPV_PLAYER(mpv));

	GMPV_MPV_CLASS(gmpv_player_parent_class)->initialize(mpv);

	g_debug("Setting volume to %f", volume);
	gmpv_mpv_set_property(mpv, "volume", MPV_FORMAT_DOUBLE, &volume);

	gmpv_player_options_init(GMPV_PLAYER(mpv));

	g_object_unref(win_settings);
}

static gint apply_options_array_string(GmpvMpv *mpv, gchar *args)
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

		if(gmpv_mpv_set_option_string(mpv, option, value) < 0)
		{
			fail_count++;

			g_warning("Failed to apply option: --%s\n", tokens[i]);
		}

		g_strfreev(parts);
	}

	g_strfreev(tokens);

	return fail_count*(-1);
}

static void apply_extra_options(GmpvMpv *mpv)
{
	GSettings *settings = g_settings_new(CONFIG_ROOT);
	gchar *extra_options = g_settings_get_string(settings, "mpv-options");

	g_debug("Applying extra mpv options: %s", extra_options);

	/* Apply extra options */
	if(apply_options_array_string(mpv, extra_options) < 0)
	{
		const gchar *msg = _("Failed to apply one or more MPV options.");
		g_signal_emit_by_name(mpv, "error", msg);
	}

	g_free(extra_options);
	g_object_unref(settings);
}

static void load_file(GmpvMpv *mpv, const gchar *uri, gboolean append)
{
	GmpvPlayer *player = GMPV_PLAYER(mpv);
	gboolean ready = FALSE;
	gboolean idle_active = FALSE;

	g_object_get(mpv, "ready", &ready, NULL);
	gmpv_mpv_get_property(	mpv,
				"idle-active",
				MPV_FORMAT_FLAG,
				&idle_active );

	if(idle_active || !ready)
	{
		if(!append)
		{
			player->new_file = TRUE;
			g_ptr_array_set_size(player->playlist, 0);
		}

		add_file_to_playlist(player, uri);
	}
	else
	{
		if(!append)
		{
			player->new_file = TRUE;
			player->loaded = FALSE;
		}

		GMPV_MPV_CLASS(gmpv_player_parent_class)
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

static void reset(GmpvMpv *mpv)
{
	gboolean idle_active = FALSE;
	gint64 playlist_pos = 0;

	gmpv_mpv_get_property(	mpv,
				"idle-active",
				MPV_FORMAT_FLAG,
				&idle_active );
	gmpv_mpv_get_property(	mpv,
				"playlist-pos",
				MPV_FORMAT_INT64,
				&playlist_pos );

	GMPV_MPV_CLASS(gmpv_player_parent_class)->reset(mpv);

	if(!idle_active)
	{
		load_from_playlist(GMPV_PLAYER(mpv));
	}

	if(playlist_pos > 0)
	{
		gmpv_mpv_set_property(	mpv,
					"playlist-pos",
					MPV_FORMAT_INT64,
					&playlist_pos );
	}
}

static void load_input_conf(GmpvPlayer *player, const gchar *input_conf)
{
	const gchar *default_keybinds[] = DEFAULT_KEYBINDS;
	gchar *tmp_path;
	FILE *tmp_file;
	gint tmp_fd;

	if(player->tmp_input_config)
	{
		g_unlink(player->tmp_input_config);
		g_free(player->tmp_input_config);
	}

	tmp_fd = g_file_open_tmp("."BIN_NAME"-XXXXXX", &tmp_path, NULL);
	tmp_file = fdopen(tmp_fd, "w");
	player->tmp_input_config = tmp_path;

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
	gmpv_mpv_set_option_string(GMPV_MPV(player), "input-conf", tmp_path);

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

static void load_config_file(GmpvMpv *mpv)
{
	GSettings *settings = g_settings_new(CONFIG_ROOT);

	if(g_settings_get_boolean(settings, "mpv-config-enable"))
	{
		gchar *mpv_conf =	g_settings_get_string
					(settings, "mpv-config-file");

		g_info("Loading config file: %s", mpv_conf);
		gmpv_mpv_load_config_file(mpv, mpv_conf);

		g_free(mpv_conf);
	}

	g_object_unref(settings);
}

static void load_input_config_file(GmpvPlayer *player)
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

static void load_scripts(GmpvPlayer *player)
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
				gmpv_mpv_command(GMPV_MPV(player), cmd);
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

static GmpvTrack *parse_track_entry(mpv_node_list *node)
{
	GmpvTrack *entry = gmpv_track_new();

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

static void add_file_to_playlist(GmpvPlayer *player, const gchar *uri)
{
	GmpvPlaylistEntry *entry = gmpv_playlist_entry_new(uri, NULL);

	g_ptr_array_add(player->playlist, entry);
}

static void load_from_playlist(GmpvPlayer *player)
{
	GmpvMpv *mpv = GMPV_MPV(player);
	GPtrArray *playlist = player->playlist;

	for(guint i = 0; playlist && i < playlist->len; i++)
	{
		GmpvPlaylistEntry *entry = g_ptr_array_index(playlist, i);

		/* Do not append on first iteration */
		GMPV_MPV_CLASS(gmpv_player_parent_class)
			->load_file(mpv, entry->filename, i != 0);
	}
}

static GmpvPlaylistEntry *parse_playlist_entry(mpv_node_list *node)
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

	return gmpv_playlist_entry_new(filename, title);
}

static void update_playlist(GmpvPlayer *player)
{
	GSettings *settings;
	gboolean prefetch_metadata;
	const mpv_node_list *org_list;
	mpv_node playlist;

	settings = g_settings_new(CONFIG_ROOT);
	prefetch_metadata = g_settings_get_boolean(settings, "prefetch-metadata");

	g_ptr_array_set_size(player->playlist, 0);

	gmpv_mpv_get_property
		(GMPV_MPV(player), "playlist", MPV_FORMAT_NODE, &playlist);

	org_list = playlist.u.list;

	if(playlist.format == MPV_FORMAT_NODE_ARRAY)
	{
		for(gint i = 0; i < org_list->num; i++)
		{
			GmpvPlaylistEntry *entry;

			entry = parse_playlist_entry(org_list->values[i].u.list);

			if(!entry->title && prefetch_metadata)
			{
				GmpvMetadataCacheEntry *cache_entry;

				cache_entry =	gmpv_metadata_cache_lookup
						(player->cache, entry->filename);
				entry->title =	g_strdup(cache_entry->title);
			}

			g_ptr_array_add(player->playlist, entry);
		}

		mpv_free_node_contents(&playlist);
		g_object_notify(G_OBJECT(player), "playlist");
	}


	if(prefetch_metadata)
	{
		gmpv_metadata_cache_load_playlist
			(player->cache, player->playlist);
	}

	g_object_unref(settings);
}

static void update_metadata(GmpvPlayer *player)
{
	mpv_node_list *org_list = NULL;
	mpv_node metadata;

	g_ptr_array_set_size(player->metadata, 0);
	gmpv_mpv_get_property(	GMPV_MPV(player),
				"metadata",
				MPV_FORMAT_NODE,
				&metadata );
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
				GmpvMetadataEntry *entry;

				entry =	gmpv_metadata_entry_new
					(key, value.u.string);

				g_ptr_array_add(player->metadata, entry);
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

static void update_track_list(GmpvPlayer *player)
{
	mpv_node_list *org_list = NULL;
	mpv_node track_list;

	g_ptr_array_set_size(player->track_list, 0);
	gmpv_mpv_get_property(	GMPV_MPV(player),
				"track-list",
				MPV_FORMAT_NODE,
				&track_list );
	org_list = track_list.u.list;

	if(track_list.format == MPV_FORMAT_NODE_ARRAY)
	{
		for(gint i = 0; i < org_list->num; i++)
		{
			GmpvTrack *entry =	parse_track_entry
						(org_list->values[i].u.list);

			g_ptr_array_add(player->track_list, entry);
		}

		mpv_free_node_contents(&track_list);
		g_object_notify(G_OBJECT(player), "track-list");
	}
}

static void cache_update_handler(	GmpvMetadataCache *cache,
					const gchar *uri,
					gpointer data )
{
	GmpvPlayer *player = data;

	for(guint i = 0; i < player->playlist->len; i++)
	{
		GmpvPlaylistEntry *entry = g_ptr_array_index(player->playlist, i);

		if(g_strcmp0(entry->filename, uri) == 0)
		{
			GmpvMetadataCacheEntry *cache_entry;

			cache_entry = gmpv_metadata_cache_lookup(cache, uri);
			g_free(entry->title);
			entry->title = g_strdup(cache_entry->title);

			g_signal_emit_by_name(data, "metadata-update", (gint64)i);
		}
	}
}

static void gmpv_player_class_init(GmpvPlayerClass *klass)
{
	GmpvMpvClass *mpv_class = GMPV_MPV_CLASS(klass);
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

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

static void gmpv_player_init(GmpvPlayer *player)
{
	player->cache =		gmpv_metadata_cache_new();
	player->playlist =	g_ptr_array_new_with_free_func
				((GDestroyNotify)gmpv_playlist_entry_free);
	player->metadata =	g_ptr_array_new_with_free_func
				((GDestroyNotify)gmpv_metadata_entry_free);
	player->track_list =	g_ptr_array_new_with_free_func
				((GDestroyNotify)gmpv_track_free);
	player->log_levels =	g_hash_table_new_full(	g_str_hash,
							g_str_equal,
							g_free,
							NULL );
	player->loaded = FALSE;
	player->new_file = TRUE;
	player->init_vo_config = TRUE;
	player->tmp_input_config = NULL;

	g_signal_connect(	player->cache,
				"update",
				G_CALLBACK(cache_update_handler),
				player );
}

GmpvPlayer *gmpv_player_new(gint64 wid)
{
	return GMPV_PLAYER(g_object_new(	gmpv_player_get_type(),
						"wid", wid,
						NULL ));
}

void gmpv_player_set_playlist_position(GmpvPlayer *player, gint64 position)
{
	GmpvMpv *mpv = GMPV_MPV(player);
	gint64 playlist_pos = 0;

	gmpv_mpv_get_property(	mpv,
				"playlist-pos",
				MPV_FORMAT_INT64,
				&playlist_pos );

	if(position != playlist_pos)
	{
		gmpv_mpv_set_property(	GMPV_MPV(player),
					"playlist-pos",
					MPV_FORMAT_INT64,
					&position );
	}
}

void gmpv_player_remove_playlist_entry(GmpvPlayer *player, gint64 position)
{
	GmpvMpv *mpv = GMPV_MPV(player);
	gboolean idle_active = gmpv_mpv_get_property_flag(mpv, "idle-active");

	if(idle_active)
	{
		g_ptr_array_remove_index(player->playlist, (guint)position);
		g_object_notify(G_OBJECT(player), "playlist");
	}
	else
	{
		const gchar *cmd[] = {"playlist_remove", NULL, NULL};
		gchar *index_str = g_strdup_printf("%" G_GINT64_FORMAT, position);

		cmd[1] = index_str;

		gmpv_mpv_command(GMPV_MPV(player), cmd);
		g_free(index_str);
	}
}

void gmpv_player_move_playlist_entry(GmpvPlayer *player, gint64 src, gint64 dst)
{
	GmpvMpv *mpv = GMPV_MPV(player);
	gboolean idle_active = gmpv_mpv_get_property_flag(mpv, "idle-active");

	if(idle_active)
	{
		GPtrArray *playlist;
		GmpvPlaylistEntry **entry;

		playlist = player->playlist;
		entry = (GmpvPlaylistEntry **)&g_ptr_array_index(playlist, src);

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

		gmpv_mpv_command(GMPV_MPV(player), cmd);

		g_free(src_str);
		g_free(dst_str);
	}
}

void gmpv_player_set_log_level(	GmpvPlayer *player,
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
	mpv_log_level iter_level = DEFAULT_LOG_LEVEL;
	mpv_log_level max_level = DEFAULT_LOG_LEVEL;
	gboolean found = FALSE;
	gint i = 0;

	for(i = 0; level_map[i].name && !found; i++)
	{
		if(g_strcmp0(level, level_map[i].name) == 0)
		{
			found = TRUE;
		}
	}

	if(found && g_strcmp0(prefix, "all") != 0)
	{
		g_hash_table_replace(	player->log_levels,
					g_strdup(prefix),
					GINT_TO_POINTER(level_map[i].level) );
	}

	max_level = level_map[i].level;

	g_hash_table_iter_init(&iter, player->log_levels);

	while(g_hash_table_iter_next(&iter, NULL, (gpointer *)&iter_level))
	{
		if(iter_level > max_level)
		{
			iter_level = max_level;
		}
	}

	for(i = 0; level_map[i].level != max_level; i++)
	gmpv_mpv_request_log_messages(GMPV_MPV(player), level_map[i].name);
}

