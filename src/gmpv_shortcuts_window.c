/*
 * Copyright (c) 2016 gnome-mpv
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

#include <glib/gi18n.h>

#include "gmpv_shortcuts_window.h"

struct _GmpvShortcutsWindow
{
	GtkShortcutsWindow parent;
};

struct _GmpvShortcutsWindowClass
{
	GtkShortcutsWindowClass parent_class;
};

struct ShortcutEntry
{
	const gchar *accel;
	const gchar *title;
};

struct ShortcutGroup
{
	const gchar *title;
	const struct ShortcutEntry *entries;
};

typedef struct ShortcutEntry ShortcutEntry;
typedef struct ShortcutGroup ShortcutGroup;

G_DEFINE_TYPE(GmpvShortcutsWindow, gmpv_shortcuts_window, GTK_TYPE_SHORTCUTS_WINDOW)

static void gmpv_shortcuts_window_init(GmpvShortcutsWindow *wnd)
{
	const ShortcutEntry general[]
		= {	{"<Ctrl>O", _("Open file")},
			{"<Ctrl>L", _("Open location")},
			{"<Ctrl>Q", _("Quit")},
			{"<Ctrl>P", _("Show preferences dialog")},
			{"<Ctrl>H", _("Toggle controls")},
			{"F9", _("Toggle playlist")},
			{"<Ctrl>S", _("Save playlist")},
			{"Delete", _("Remove selected playlist item")},
			{"F11", _("Toggle fullscreen mode")},
			{"Escape", _("Leave fullscreen mode")},
			{NULL, NULL} };
	const ShortcutEntry mpv_seeking[]
		= {	{"leftarrow rightarrow", _("Seek backward/forward 5 seconds")},
			{"<Shift>leftarrow <Shift>rightarrow", _("Exact seek backward/forward 1 second")},
			{"downarrow uparrow", _("Seek backward/forward 1 minute")},
			{"<Shift>downarrow <Shift>uparrow", _("Exact seek backward/forward 5 seconds")},
			{"<Ctrl>leftarrow <Ctrl>rightarrow", _("Seek to previous/next subtitle")},
			{"comma period", _("Step backward/forward a single frame")},
			{"Page_Up Page_Down", _("Seek to the beginning of the previous/next chapter")},
			{NULL, NULL} };
	const ShortcutEntry mpv_playback_speed[]
		= {	{"bracketleft bracketright", _("Decrease/increase playback speed by 10%")},
			{"braceleft braceright", _("Halve/double current playback speed")},
			{"BackSpace", _("Reset playback speed to normal")},
			{NULL, NULL} };
	const ShortcutEntry mpv_playback[]
		= {	{"less greater", _("Go backward/forward in the playlist")},
			{"l", _("Set/clear A-B loop points")},
			{"<Shift>l", _("Toggle infinite looping")},
			{"p space", _("Pause or unpause")},
			{"q", _("Quit")},
			{"<Shift>q", _("Save current playback position and quit")},
			{NULL, NULL} };
	const ShortcutEntry mpv_audio[]
		= {	{"numbersign", _("Cycle through audio tracks")},
			{"slash asterisk", _("Decrease/increase volume")},
			{"9 0", _("Decrease/increase volume")},
			{"m", _("Mute or unmute")},
			{"<Ctrl>plus <Ctrl>minus", _("Adjust audio delay by +/- 0.1 seconds")},
			{NULL, NULL} };
	const ShortcutEntry mpv_osd[]
		= {	{"<Shift>o", _("Toggle OSD states between normal and playback time/duration")},
			{"<Shift>I", _("Show filename on the OSD")},
			{"o <Shift>p", _("Show progress, elapsed time, and duration on the OSD")},
			{NULL, NULL} };
	const ShortcutEntry mpv_subtitle[]
		= {	{"v", _("Toggle subtitle visibility")},
			{"i j", _("Cycle through available subtitles")},
			{"x z", _("Adjust subtitle delay by +/- 0.1 seconds")},
			{"u", _("Toggle SSA/ASS subtitles style override")},
			{"r t", _("Move subtitles up/down")},
			{"<Shift>v", _("Toggle VSFilter aspect compatibility mode")},
			{NULL, NULL} };
	const ShortcutEntry mpv_video[]
		= {	{"underscore", _("Cycle through video tracks")},
			{"w e", _("Decrease/increase pan-and-scan range")},
			{"<Alt>0", _("Resize video to half its original size")},
			{"<Alt>1", _("Resize video to its original size")},
			{"<Alt>2", _("Resize video to double its original size")},
			{"1 2", _("Adjust contrast")},
			{"3 4", _("Adjust brightness")},
			{"5 6", _("Adjust gamma")},
			{"7 8", _("Adjust saturation")},
			{"d", _("Activate or deactivate deinterlacer")},
			{"<Shift>a", _("Cycle aspect ratio override")},
			{NULL, NULL} };
	const ShortcutEntry mpv_misc[]
		= {	{"f", _("Toggle fullscreen mode")},
			{"Escape", _("Exit fullscreen mode")},
			{"s", _("Take a screenshot")},
			{"<Shift>s", _("Take a screenshot, without subtitles")},
			{"<Ctrl>s", _("Take a screenshot, as the window shows it")},
			{NULL, NULL} };
	const ShortcutGroup mpv_groups[]
		= {	{_("Seeking"), mpv_seeking},
			{_("Playback Speed"), mpv_playback_speed},
			{_("Playback"), mpv_playback},
			{_("Audio"), mpv_audio},
			{_("On-Screen Display"), mpv_osd},
			{_("Subtitle"), mpv_subtitle},
			{_("Video"), mpv_video},
			{_("Miscellaneous"), mpv_misc},
			{NULL, NULL} };
	GtkWidget *general_section;
	GtkWidget *mpv_section;
	GtkWidget *general_group;

	general_section = g_object_new(	gtk_shortcuts_section_get_type(),
					"section-name", "shortcuts",
					"title", _("GNOME MPV"),
					"visible", TRUE,
					NULL );
	mpv_section = g_object_new(	gtk_shortcuts_section_get_type(),
					"section-name", "mpv_shortcuts",
					"title", _("MPV"),
					"visible", FALSE,
					NULL );
	general_group = g_object_new(	gtk_shortcuts_group_get_type(),
					"title", _("General"),
					NULL );

	for(gint i  = 0; general[i].accel; i++)
	{
		GtkWidget *entry;

		entry = g_object_new(	gtk_shortcuts_shortcut_get_type(),
					"accelerator", general[i].accel,
					"title", general[i].title,
					NULL );

		gtk_container_add(GTK_CONTAINER(general_group), entry);
	}

	gtk_container_add(GTK_CONTAINER(general_section), general_group);

	for(gint i  = 0; mpv_groups[i].title; i++)
	{
		const ShortcutEntry *entries = mpv_groups[i].entries;
		GtkWidget *mpv_group =	g_object_new
					(	gtk_shortcuts_group_get_type(),
						"title", mpv_groups[i].title,
						NULL );

		for(gint j  = 0; entries[j].accel; j++)
		{
			GtkWidget *mpv_entry;

			mpv_entry =	g_object_new
				(	gtk_shortcuts_shortcut_get_type(),
					"accelerator", entries[j].accel,
					"title", entries[j].title,
					NULL );

			gtk_container_add(GTK_CONTAINER(mpv_group), mpv_entry);
		}

		gtk_container_add(GTK_CONTAINER(mpv_section), mpv_group);
	}

	gtk_container_add(GTK_CONTAINER(wnd), general_section);
	gtk_container_add(GTK_CONTAINER(wnd), mpv_section);
}

static void gmpv_shortcuts_window_class_init(GmpvShortcutsWindowClass *klass)
{
}

GtkWidget *gmpv_shortcuts_window_new(GtkWindow *parent)
{
	return GTK_WIDGET(g_object_new(	gmpv_shortcuts_window_get_type(),
					"transient-for", parent,
					"modal", TRUE,
					NULL));
}
