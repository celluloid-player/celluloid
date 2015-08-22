#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

(test -f $srcdir/configure.ac) || {
	echo -n "**Error**: Directory "\`$srcdir\'" does not look like the top-level directory"
	exit 1
}

aclocal --install -I m4 || exit 1
intltoolize --copy --automake || exit 1
autoreconf --install -Wno-portability || exit 1

if [ "$NOCONFIGURE" = "" ]; then
        $srcdir/configure "$@" || exit 1

        if [ "$1" = "--help" ]; then exit 0 else
                echo "Now type \`make\' to compile" || exit 1
        fi
else
        echo "Skipping configure process."
fi

set +x
