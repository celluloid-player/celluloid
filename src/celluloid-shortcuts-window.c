/*
 * Copyright (c) 2016-2022, 2025 gnome-mpv
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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <adwaita.h>

#include "celluloid-shortcuts-window.h"

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
typedef AdwShortcutsDialog CelluloidShortcutsWindow;

static void
celluloid_shortcuts_window_init(CelluloidShortcutsWindow *wnd)
{
	const ShortcutEntry general[]
		= {	{"<Ctrl>o", _("Open file")},
			{"<Ctrl>l", _("Open location")},
			{"<Ctrl><Shift>o", _("Add file to playlist")},
			{"<Ctrl><Shift>l", _("Add location to playlist")},
			{"<Ctrl>comma", _("Show preferences dialog")},
			{"F9", _("Toggle playlist")},
			{"F10", _("Show main menu")},
			{"F11 f", _("Toggle fullscreen mode")},
			{"Escape", _("Leave fullscreen mode")},
			{"<Shift>o", _("Toggle OSD states between normal and playback time/duration")},
			{"<Shift>i", _("Show filename on the OSD")},
			{"o <Shift>p", _("Show progress, elapsed time, and duration on the OSD")},
			{NULL, NULL} };
	const ShortcutEntry seeking[]
		= {	{"Left Right", _("Seek backward/forward 5 seconds")},
			{"<Shift>Left <Shift>Right", _("Exact seek backward/forward 1 second")},
			{"downarrow uparrow", _("Seek backward/forward 1 minute")},
			{"<Shift>downarrow <Shift>uparrow", _("Exact seek backward/forward 5 seconds")},
			{"<Ctrl>Left <Ctrl>Right", _("Seek to previous/next subtitle")},
			{"comma period", _("Step backward/forward a single frame")},
			{"Page_Down Page_Up", _("Seek to the beginning of the previous/next chapter")},
			{NULL, NULL} };
	const ShortcutEntry playback[]
		= {	{"bracketleft bracketright", _("Decrease/increase playback speed by 10%")},
			{"braceleft braceright", _("Halve/double current playback speed")},
			{"BackSpace", _("Reset playback speed to normal")},
			{"less greater", _("Go backward/forward in the playlist")},
			{"Delete", _("Remove selected playlist item")},
			{"<Ctrl><Shift>s", _("Save playlist")},
			{"l", _("Set/clear A-B loop points")},
			{"<Shift>l", _("Toggle infinite looping")},
			{"p space", _("Pause or unpause")},
			{"<Ctrl>q q", _("Quit")},
			{"<Shift>q", _("Save current playback position and quit")},
			{NULL, NULL} };
	const ShortcutEntry playlist[]
		= {	{"<Ctrl>f", _("Enter search mode")},
			{"<Ctrl>g", _("Jump to next match")},
			{"<Ctrl><Shift>g", _("Jump to previous match")},
			{"Escape", _("Exit search mode")},
			{NULL, NULL} };
	const ShortcutEntry audio[]
		= {	{"numbersign", _("Cycle through audio tracks")},
			{"slash asterisk", _("Decrease/increase volume")},
			{"9 0", _("Decrease/increase volume")},
			{"m", _("Mute or unmute")},
			{"<Ctrl>plus <Ctrl>minus", _("Adjust audio delay by +/- 0.1 seconds")},
			{NULL, NULL} };
	const ShortcutEntry subtitle[]
		= {	{"v", _("Toggle subtitle visibility")},
			{"j <Shift>j", _("Cycle through available subtitles")},
			{"z <Shift>z", _("Adjust subtitle delay by +/- 0.1 seconds")},
			{"u", _("Toggle SSA/ASS subtitles style override")},
			{"r <Shift>r", _("Move subtitles up/down")},
			{"<Shift>v", _("Toggle VSFilter aspect compatibility mode")},
			{NULL, NULL} };
	const ShortcutEntry video[]
		= {	{"underscore", _("Cycle through video tracks")},
			{"w <Shift>w", _("Decrease/increase pan-and-scan range")},
			{"s", _("Take a screenshot")},
			{"<Shift>s", _("Take a screenshot, without subtitles")},
			{"<Ctrl>s", _("Take a screenshot, as the window shows it")},
			{"<Alt>0", _("Resize video to half its original size")},
			{"<Alt>1", _("Resize video to its original size")},
			{"<Alt>2", _("Resize video to double its original size")},
			{"1 2", _("Adjust contrast")},
			{"3 4", _("Adjust brightness")},
			{"5 6", _("Adjust gamma")},
			{"7 8", _("Adjust saturation")},
			{"d", _("Activate or deactivate deinterlacer")},
			{"<Shift>a", _("Cycle aspect ratio override")},

			{"<Alt>Left <Alt>Right <Alt>Up <Alt>Down", _("Pan the video")},
			{"<Alt>plus <Alt>minus", _("Zoom in/out")},
			{"<Alt>BackSpace", _("Reset pan/zoom settings")},

			{NULL, NULL} };
	const ShortcutGroup groups[]
		= {	{_("User Interface"), general},
			{_("Video"), video},
			{_("Audio"), audio},
			{_("Subtitle"), subtitle},
			{_("Playback"), playback},
			{_("Seeking"), seeking},
			{_("Playlist"), playlist},
			{NULL, NULL} };

	for(gint i  = 0; groups[i].title; i++)
	{
		const ShortcutEntry *entries =
			groups[i].entries;
		AdwShortcutsSection *section =
			adw_shortcuts_section_new(groups[i].title);

		for(gint j  = 0; entries[j].accel; j++)
		{
			AdwShortcutsItem *item =
				adw_shortcuts_item_new
				(entries[j].title, entries[j].accel);

			adw_shortcuts_section_add(section, item);
		}

		AdwShortcutsDialog *dialog = ADW_SHORTCUTS_DIALOG(wnd);
		adw_shortcuts_dialog_add(dialog, section);
	}
}

GtkWidget *
celluloid_shortcuts_window_new(GtkWindow *parent)
{
	GObject *result = g_object_new(adw_shortcuts_dialog_get_type(), NULL);

	celluloid_shortcuts_window_init(CELLULOID_SHORTCUTS_WINDOW(result));

	return GTK_WIDGET(result);
}
