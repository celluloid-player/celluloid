/*
 * Copyright (c) 2014-2022, 2024 gnome-mpv
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

#ifndef DEF_H
#define DEF_H

#define APP_ID "io.github.celluloid_player.Celluloid"
#define ICON_NAME APP_ID
#define BIN_NAME "celluloid"
#define CONFIG_DIR BIN_NAME
#define CONFIG_ROOT "io.github.celluloid-player.Celluloid"
#define CONFIG_WIN_STATE CONFIG_ROOT".window-state"
#define ACTION_PREFIX "celluloid-action"
#define DEFAULT_LOG_LEVEL MPV_LOG_LEVEL_ERROR
#define MPRIS_TRACK_LIST_BEFORE 10
#define MPRIS_TRACK_LIST_AFTER 10
#define MPRIS_TRACK_ID_NO_TRACK "/org/mpris/MediaPlayer2/TrackList/NoTrack"
#define MPRIS_TRACK_ID_PREFIX "/io/github/CelluloidPlayer/Celluloid/Track/"
#define MPRIS_BUS_NAME "org.mpris.MediaPlayer2."APP_ID
#define MPRIS_OBJ_ROOT_PATH "/org/mpris/MediaPlayer2"
#define MPV_OPTION_PREFIX "--mpv-"
#define PLAYLIST_DEFAULT_WIDTH 200
#define CSD_WIDTH_OFFSET 52
#define CSD_HEIGHT_OFFSET 99
#define COMPACT_THRESHOLD_OFFSET 100
#define WAYLAND_NOCSD_HEIGHT_OFFSET 60
#define MAIN_WINDOW_DEFAULT_WIDTH 625
#define MAIN_WINDOW_DEFAULT_HEIGHT 400
#define SEEK_BAR_UPDATE_INTERVAL 250
#define FS_CONTROL_HIDE_DELAY 1
#define KEYSTRING_MAX_LEN 16
#define MIN_MPV_MAJOR 0
#define MIN_MPV_MINOR 29
#define MIN_MPV_PATCH 0

#define SUBTITLE_EXTS	{	"utf",\
				"utf8",\
				"utf-8",\
				"idx",\
				"sub",\
				"srt",\
				"smi",\
				"rt",\
				"txt",\
				"ssa",\
				"aqt",\
				"jss",\
				"js",\
				"ass",\
				"mks",\
				"vtt",\
				"sup",\
				NULL }

#define PLAYLIST_EXTS	{	"m3u",\
				"m3u8",\
				"ini",\
				"pls",\
				"txt",\
				NULL }

#define DND_TARGETS	{	{.target = "PLAYLIST_PATH",\
				.flags = GTK_TARGET_SAME_APP,\
				.info = 0},\
				{.target = "text/uri-list",\
				.flags = 0,\
				.info = 0},\
				{.target = "text/plain",\
				.flags = 0,\
				.info = 0},\
				{.target = "STRING",\
				.flags = 0,\
				.info = 0} }

#define DEFAULT_KEYBINDS \
	{	"Alt+0 set window-scale 0.5",\
		"Alt+1 set window-scale 1.0",\
		"Alt+2 set window-scale 2.0",\
		"Ctrl+o script-message celluloid-action win.show-open-dialog((false, false))",\
		"Ctrl+l script-message celluloid-action win.show-open-location-dialog(false)",\
		"Ctrl+n script-message celluloid-action app.new-window",\
		"Ctrl+Shift+o script-message celluloid-action win.show-open-dialog((false, true))",\
		"Ctrl+Shift+l script-message celluloid-action win.show-open-location-dialog(true)",\
		"Ctrl+Shift+s script-message celluloid-action win.save-playlist",\
		"Ctrl+f script-message celluloid-action win.search-playlist",\
		"Ctrl+q quit",\
		"Ctrl+? script-message celluloid-action win.show-shortcuts-dialog",\
		"Ctrl+, script-message celluloid-action win.show-preferences-dialog",\
		"F9 script-message celluloid-action win.toggle-playlist",\
		"DEL script-message celluloid-action win.remove-selected-playlist-item",\
		"U stop",\
		"STOP stop",\
		"F10 script-message celluloid-action win.toggle-main-menu",\
		"F11 cycle fullscreen",\
		"WHEEL_UP add volume 2",\
		"WHEEL_DOWN add volume -2",\
		"WHEEL_LEFT no-osd seek -10",\
		"WHEEL_RIGHT no-osd seek 10",\
		NULL }

#define KEYSTRING_MAP	{	"PGUP", "Page_Up",\
				"PGDWN", "Page_Down",\
				"BS", "\b",\
				"SHARP", "#",\
				"UP", "KP_Up",\
				"DOWN", "KP_Down",\
				"RIGHT", "KP_Right",\
				"LEFT", "KP_Left",\
				"RIGHT", "Right",\
				"LEFT", "Left",\
				"UP", "Up",\
				"DOWN", "Down",\
				"ESC", "\x1b",\
				"DEL", "\x7f",\
				"ENTER", "\r",\
				"INS", "Insert",\
				"VOLUME_LOWER", "AudioLowerVolume",\
				"MUTE", "AudioMute",\
				"VOLUME_UP", "AudioRaiseVolume",\
				"PLAY", "AudioPlay",\
				"STOP", "AudioStop",\
				"PREV", "AudioPrev",\
				"NEXT", "AudioNext",\
				"FORWARD", "AudioForward",\
				"REWIND", "AudioRewind",\
				"MENU", "Menu",\
				"HOMEPAGE", "HomePage",\
				"MAIL", "Mail",\
				"FAVORITES", "Favorites",\
				"SEARCH", "Search",\
				"SLEEP", "Sleep",\
				"CANCEL", "Cancel",\
				"RECORD", "AudioRecord",\
				"", "Control_L",\
				"", "Control_R",\
				"", "Alt_L",\
				"", "Alt_R",\
				"", "Meta_L",\
				"", "Meta_R",\
				"", "Shift_L",\
				"", "Shift_R",\
				NULL }

#define SUPPORTED_PROTOCOLS	{	"cdda",\
					"rtmp",\
					"rtsp",\
					"http",\
					"https",\
					"mms",\
					"mmst",\
					"mmsh",\
					"mmshttp",\
					"rtp",\
					"httpproxy",\
					"hls",\
					"rtmpe",\
					"rtmps",\
					"rtmpt",\
					"rtmpte",\
					"rtmpts",\
					"srtp",\
					"lavf",\
					"ffmpeg",\
					"udp",\
					"ftp",\
					"tcp",\
					"tls",\
					"unix",\
					"sftp",\
					"md5",\
					"concat",\
					"avdevice",\
					"av",\
					"dvb",\
					"tv",\
					"pvr",\
					"smb",\
					"file",\
					"dvdread",\
					"dvd",\
					"dvdnav",\
					"bd",\
					"br",\
					"bluray",\
					"bdnav",\
					"brnav",\
					"bluraynav",\
					"memory",\
					"null",\
					"mf",\
					"edl",\
					"rar",\
					NULL }

#define SUPPORTED_MIME_TYPES	{	"application/ogg",\
					"application/x-ogg",\
					"application/sdp",\
					"application/smil",\
					"application/x-smil",\
					"application/streamingmedia",\
					"application/x-streamingmedia",\
					"application/vnd.rn-realmedia",\
					"application/vnd.rn-realmedia-vbr",\
					"audio/aac",\
					"audio/x-aac",\
					"audio/m4a",\
					"audio/x-m4a",\
					"audio/mp1",\
					"audio/x-mp1",\
					"audio/mp2",\
					"audio/x-mp2",\
					"audio/mp3",\
					"audio/x-mp3",\
					"audio/mpeg",\
					"audio/x-mpeg",\
					"audio/mpegurl",\
					"audio/x-mpegurl",\
					"audio/mpg",\
					"audio/x-mpg",\
					"audio/rn-mpeg",\
					"audio/ogg",\
					"audio/scpls",\
					"audio/x-scpls",\
					"audio/vnd.rn-realaudio",\
					"audio/wav",\
					"audio/x-pn-windows-pcm",\
					"audio/x-realaudio",\
					"audio/x-pn-realaudio",\
					"audio/x-ms-wma",\
					"audio/x-pls",\
					"audio/x-wav",\
					"video/mpeg",\
					"video/x-mpeg",\
					"video/x-mpeg2",\
					"video/mp4",\
					"video/msvideo",\
					"video/x-msvideo",\
					"video/ogg",\
					"video/quicktime",\
					"video/vnd.rn-realvideo",\
					"video/x-ms-afs",\
					"video/x-ms-asf",\
					"video/x-ms-wmv",\
					"video/x-ms-wmx",\
					"video/x-ms-wvxvideo",\
					"video/x-avi",\
					"video/x-fli",\
					"video/x-flv",\
					"video/x-theora",\
					"video/x-matroska",\
					"video/webm",\
					"audio/x-flac",\
					"audio/x-vorbis+ogg",\
					"video/x-ogm+ogg",\
					"audio/x-shorten",\
					"audio/x-ape",\
					"audio/x-wavpack",\
					"audio/x-tta",\
					"audio/AMR",\
					"audio/ac3",\
					"video/mp2t",\
					"audio/flac",\
					NULL }

#endif
