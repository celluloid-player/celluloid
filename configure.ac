AC_PREREQ([2.69])

AC_INIT(	[gnome-mpv],
		[0.3],
		[http://github.com/gnome-mpv/gnome-mpv/issues],
		[gnome-mpv],
		[http://github.com/gnome-mpv/gnome-mpv] )

AC_CONFIG_SRCDIR([src/main.c])
AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE

# i18n
IT_PROG_INTLTOOL([0.40.6])
GETTEXT_PACKAGE=gnome-mpv
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

AM_GLIB_GNU_GETTEXT

AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug], [turn on debugging]))

AS_IF(	[test "x$enable_debug" = "xyes"],
	[CFLAGS="-ansi -Wall -pedantic -g -fsanitize=address"],
	[CFLAGS="-O2 -fomit-frame-pointer -Wno-deprecated-declarations"] )

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES(DEPS, [gtk+-3.0 >= 3.10 glib-2.0 >= 2.32 mpv >= 1.0])

# Checks for header files.
AC_CHECK_HEADERS([string.h stdio.h stdlib.h locale.h ctype.h execinfo.h])
AC_CHECK_FUNCS([setlocale backtrace backtrace_symbols_fd exit])
AC_CONFIG_FILES([Makefile src/Makefile po/Makefile.in])
AC_OUTPUT
