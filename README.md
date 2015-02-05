# GNOME MPV

GNOME MPV is a simple GTK+ frontend for MPV.

## Dependencies

- autotools (build)
- pkg-config (build)
- gcc (build)
- intltool (build)
- glib2
- gtk3
- libmpv
- youtube-dl (optional)

## Installation

Run the following command to build and install:

    $ autoreconf -sfi && intltoolize -c --automake && ./configure && make && sudo make install
