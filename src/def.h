/*
 * Copyright (c) 2014-2015 gnome-mpv
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

#define APP_ID "org.gnome-mpv"
#define ICON_NAME "gnome-mpv"
#define CONFIG_DIR "gnome-mpv"
#define CONFIG_FILE "gnome-mpv.conf"
#define CONFIG_ROOT APP_ID
#define CONFIG_WIN_STATE APP_ID".window-state"
#define CONFIG_ROOT_PATH "/apps/gnome-mpv/"
#define CONFIG_ROOT_GROUP "main"
#define DEFAULT_LOG_LEVEL MPV_LOG_LEVEL_ERROR
#define MPRIS_TRACK_ID_PREFIX "/org/gnome_mpv/Track/"
#define MPRIS_BUS_NAME "org.mpris.MediaPlayer2.gnome-mpv"
#define MPRIS_OBJ_ROOT_PATH "/org/mpris/MediaPlayer2"
#define PLAYLIST_DEFAULT_WIDTH 200
#define PLAYLIST_CSD_WIDTH_OFFSET 52
#define MAIN_WINDOW_DEFAULT_WIDTH 625
#define MAIN_WINDOW_DEFAULT_HEIGHT 400
#define VID_AREA_BG_COLOR "#000000"
#define SEEK_BAR_UPDATE_INTERVAL 250
#define CURSOR_HIDE_DELAY 1
#define KEYSTRING_MAX_LEN 8

#define DEFAULT_KEYBINDS	{	"SPACE cycle pause",\
					"p cycle pause",\
					"v osd-msg cycle sub-visibility",\
					"s osd-msg screenshot",\
					"S osd-msg screenshot video",\
					"j osd-msg cycle sub",\
					"J osd-msg cycle sub down",\
					"@ osd-msg cycle chapter",\
					"! osd-msg cycle chapter down",\
					"< playlist_prev",\
					"> playlist_next",\
					"U stop",\
					"RIGHT seek 10",\
					"LEFT seek -10",\
					"UP seek 60",\
					"DOWN seek -60",\
					"Q quit_watch_later",\
					"MOUSE_BTN0_DBL cycle fullscreen",\
					"MOUSE_BTN2 cycle pause",\
					NULL }

#define KEYSTRING_MAP	{	"<", "less",\
				">", "greater",\
				"PGUP", "Page_Up",\
				"PGDWN", "Page_Down",\
				"BS", "BackSpace",\
				"SPACE", "space",\
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
				"RIGHT", "Right",\
				"LEFT", "Left",\
				"UP", "Up",\
				"DOWN", "Down",\
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
