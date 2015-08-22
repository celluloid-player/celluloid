# GNOME MPV

GNOME MPV is a simple GTK+ frontend for mpv. GNOME MPV interacts with mpv via
the client API exported by libmpv, allowing access to mpv's powerful playback
capabilities.

## Dependencies

- autotools (build)
- pkg-config (build)
- gcc (build)
- intltool (build)
- python2 (build)
- glib2
- gtk3
- libmpv
- youtube-dl (optional)

## Installation

### GNU/Linux packages
- Arch Linux: https://aur.archlinux.org/packages/gnome-mpv
- Arch Linux (Git): https://aur.archlinux.org/packages/gnome-mpv-git
- Fedora (russianfedora): http://ru.fedoracommunity.org/repository
- Fedora/OpenSUSE: https://build.opensuse.org/package/show/home:mermoldy:multimedia/gnome-mpv
- Gentoo: http://gpo.zugaina.org/media-video/gnome-mpv
- Solus: https://packages.solus-project.com/v1/g/gnome-mpv/
- Ubuntu: https://launchpad.net/~xuzhen666/+archive/ubuntu/gnome-mpv

### Source code
Run the following command in the source code directory to build and install:

```sh
./autogen.sh && make && sudo make install
```

## Usage

### Opening files
There are 4 ways to open files in GNOME MPV.

1. Passing files and/or URIs as command line arguments.
2. Using the file chooser dialog box, accessible via the "Open" menu item.
3. Typing URI into the "Open Location" dialog box, accessible via the
   menu item with the same name.
4. Dragging and dropping files or URIs onto GNOME MPV.

### Manipulating playlist
The playlist is hidden by default. To show the playlist, click the "Playlist"
menu item or press F9. Files can be added by dragging and dropping files or URIs
onto the playlist. Dropping files or URIs onto the video area will replace the
content of the playlist. Playlist files or online playlists (eg. YouTube's
playlist) will be automatically expanded into individual items when loaded.

Items in the playlist can be reordered via drag-and-drop. To remove items from
the playlist, select the item by clicking on it then press the delete button on
your keyboard.

### Configuration
GNOME MPV can be configured using the preferences dialog box accessible via the
"Preferences" menu item. Playback-related settings can be loaded from an
external file using the same syntax as mpv's `mpv.conf`. It is possible to load
mpv's `mpv.conf` directly with no modification. GNOME MPV will not write to the
file, so it is not necessary to make a separate copy of the file. The file must
be specified and enabled in the preferences dialog under the "MPV Configuration"
section.

Options can also be passed to mpv using the "Extra MPV Options" text box in the
preferences dialog. The syntax used is the same as mpv's command line options.

### Default keybindings
Default GUI-related keybindings are defined as following:

|Keys		|Action			|
|---------------|-----------------------|
|Ctrl+O		|Open			|
|Ctrl+L		|Open location		|
|Ctrl+S		|Save playlist		|
|Ctrl+Q		|Quit			|
|Ctrl+P		|Preferences		|
|F9		|Toggle playlist	|
|F11 or F	|Toggle fullscreen mode	|
|ESC   		|Exit fullscreen mode	|
|Ctrl+1		|Normal size		|
|Ctrl+2		|Double size		|
|Ctrl+3		|Half size		|

"Normal size", "Double size", and "Half size" resizes GNOME MPV window such that
the video area is proportional to the size of the playing video. "Normal size"
will resize the window so that the video area will be the same size as the
video. "Double size" and "Half size" will resize the window so that the video
area will be double and half the actual size of the video respectively. Because
this requires information from the playing video, these actions will have no
effect if no file is loaded or the current file is an audio file.

There is also another set of default keybindings used for playback-related
actions. They are defined in the macro `DEFAULT_KEYBINDS`, which can be found
in [src/def.h](https://github.com/gnome-mpv/gnome-mpv/blob/master/src/def.h).
The syntax used is exactly the same as mpv's `input.conf`.

Additional keybindings can be defined in an external file using mpv's
`input.conf` syntax. Because the syntax is the same, it is possible to load
mpv's `input.conf` directly with no modification. It is not necessary to create
a separate copy of the file as GNOME MPV will not write to the file. The file
has to be loaded via the preferences dialog under the "Keybindings" section. In
case of conflict with default keybindings, keybindings defined in external file
will be given priority.
