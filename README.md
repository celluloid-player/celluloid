# GNOME MPV

GNOME MPV is a simple GTK+ frontend for MPV.

## Dependencies

- autotools (build)
- pkg-config (build)
- gcc (build)
- glib2
- gtk3
- libmpv
- youtube-dl (optional)

## Installation

Run the following command to build and install:

    $ autoreconf -sfi && ./configure && make && sudo make install
