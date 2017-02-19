/*
 * Copyright (c) 2014-2017 gnome-mpv
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

#ifndef DEF_H
#define DEF_H

#define APP_ID "io.github.GnomeMpv"
#define ICON_NAME "gnome-mpv"
#define CONFIG_DIR "gnome-mpv"
#define CONFIG_ROOT APP_ID
#define CONFIG_WIN_STATE APP_ID".window-state"
#define ACTION_PREFIX "gmpv-action"
#define DEFAULT_LOG_LEVEL MPV_LOG_LEVEL_ERROR
#define MPRIS_TRACK_ID_PREFIX "/org/gnome_mpv/GmpvTrack/"
#define MPRIS_BUS_NAME "org.mpris.MediaPlayer2.gnome-mpv"
#define MPRIS_OBJ_ROOT_PATH "/org/mpris/MediaPlayer2"
#define PLAYLIST_DEFAULT_WIDTH 200
#define PLAYLIST_MIN_WIDTH 20
#define CSD_WIDTH_OFFSET 52
#define CSD_HEIGHT_OFFSET 99
#define WAYLAND_NOCSD_HEIGHT_OFFSET 60
#define MAIN_WINDOW_DEFAULT_WIDTH 625
#define MAIN_WINDOW_DEFAULT_HEIGHT 400
#define SEEK_BAR_UPDATE_INTERVAL 250
#define FS_CONTROL_HIDE_DELAY 1
#define KEYSTRING_MAX_LEN 16

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
	{	"Ctrl+o script-message gmpv-action show-open-dialog(false)",\
		"Ctrl+l script-message gmpv-action show-open-location-dialog(false)",\
		"Ctrl+Shift+s script-message gmpv-action save-playlist",\
		"Ctrl+q script-message gmpv-action quit",\
		"Ctrl+? script-message gmpv-action show-shortcuts-dialog",\
		"Ctrl+p script-message gmpv-action show-preferences-dialog",\
		"Ctrl+h script-message gmpv-action toggle-controls",\
		"F9 script-message gmpv-action toggle-playlist",\
		"F11 script-message gmpv-action toggle-fullscreen",\
		"f script-message gmpv-action toggle-fullscreen",\
		"ESC script-message gmpv-action leave-fullscreen",\
		"v osd-msg cycle sub-visibility",\
		"s osd-msg screenshot",\
		"S osd-msg screenshot video",\
		"j osd-msg cycle sub",\
		"J osd-msg cycle sub down",\
		"@ osd-msg cycle chapter",\
		"! osd-msg cycle chapter down",\
		"DEL script-message gmpv-action remove-selected-playlist-item",\
		"U stop",\
		"RIGHT no-osd seek 10",\
		"LEFT no-osd seek -10",\
		"UP no-osd seek 60",\
		"DOWN no-osd seek -60",\
		"Alt+0 script-message gmpv-action set-video-size(0.5)",\
		"Alt+1 script-message gmpv-action set-video-size(1.0)",\
		"Alt+2 script-message gmpv-action set-video-size(2.0)",\
		"MOUSE_BTN0_DBL script-message gmpv-action toggle-fullscreen",\
		"MOUSE_BTN3 add volume 2",\
		"MOUSE_BTN4 add volume -2",\
		"MOUSE_BTN5 no-osd seek -10",\
		"MOUSE_BTN6 no-osd seek 10",\
		NULL }

#define KEYSTRING_MAP	{	"<", "less",\
				">", "greater",\
				"PGUP", "Page_Up",\
				"PGDWN", "Page_Down",\
				"BS", "BackSpace",\
				".", "period",\
				",", "comma",\
				"`", "grave",\
				"~", "asciitilde",\
				"!", "exclam",\
				"@", "at",\
				"SHARP", "numbersign",\
				"$", "dollar",\
				"%", "percent",\
				"^", "caret",\
				"&", "ampersand",\
				"*", "asterisk",\
				"-", "minus",\
				"_", "underscore",\
				"=", "equal",\
				"+", "plus",\
				";", "semicolon",\
				":", "colon",\
				"'", "apostrophe",\
				"\"", "quotedbl",\
				"/", "slash",\
				"\\", "backslash",\
				"(", "parenleft",\
				")", "parenright",\
				"[", "bracketleft",\
				"]", "bracketright",\
				"?", "question",\
				"RIGHT", "Right",\
				"LEFT", "Left",\
				"UP", "Up",\
				"DOWN", "Down",\
				"ESC", "Escape",\
				"DEL", "Delete",\
				"ENTER", "Return",\
				"INS", "Insert",\
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

#define SUPPORTED_MIME_TYPES	{	"gmpv_application/ogg",\
					"gmpv_application/x-ogg",\
					"gmpv_application/sdp",\
					"gmpv_application/smil",\
					"gmpv_application/x-smil",\
					"gmpv_application/streamingmedia",\
					"gmpv_application/x-streamingmedia",\
					"gmpv_application/vnd.rn-realmedia",\
					"gmpv_application/vnd.rn-realmedia-vbr",\
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
