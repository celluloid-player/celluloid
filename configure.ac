AC_PREREQ([2.69])

AC_INIT(	[gnome-mpv],
		[0.6],
		[http://github.com/gnome-mpv/gnome-mpv/issues],
		[gnome-mpv],
		[http://github.com/gnome-mpv/gnome-mpv] )

AC_CONFIG_SRCDIR([src/main.c])
AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE([1.12 tar-pax dist-xz no-dist-gzip subdir-objects foreign -Wall -Wno-portability])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([enable])

# i18n
IT_PROG_INTLTOOL([0.40.6])
GETTEXT_PACKAGE=gnome-mpv
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT

GLIB_GSETTINGS

AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug], [turn on debugging]))

AC_DEFUN([GM_CHECK_AX_MACRO],
	[m4_ifndef([$1], [m4_fatal($1[ is not defined. Is autoconf-archive installed?])])])

GM_CHECK_AX_MACRO([AX_APPEND_COMPILE_FLAGS])
GM_CHECK_AX_MACRO([AX_APPEND_LINK_FLAGS])

AS_IF([test "x$enable_debug" = "xyes"], [
	AX_APPEND_COMPILE_FLAGS([-O0 -g -fsanitize=address])
], [
	AX_APPEND_COMPILE_FLAGS([-O2])
])

AX_APPEND_COMPILE_FLAGS([ \
	-std=gnu99 \
	-funsigned-char \
	-fstack-protector-strong \
	-fPIE \
	-fPIC \
	-Wall \
	-Wextra \
	-Wconversion \
	-Winline \
	-Wno-padded \
	-Wno-unused-parameter \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Werror=implicit-function-declaration \
	-Werror=pointer-arith \
	-Werror=init-self \
	-Werror=format-security \
	-Werror=format=2 \
	-Werror=missing-include-dirs \
	-Werror=date-time \
])

AX_APPEND_LINK_FLAGS([ \
	-pie \
	-Wl,-z,relro \
	-Wl,-z,now \
])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_STDC

# Checks for libraries.
PKG_CHECK_MODULES(DEPS, [gtk+-3.0 >= 3.16 glib-2.0 >= 2.40 mpv >= 1.16 epoxy])
AC_DEFINE([GLIB_VERSION_MIN_REQUIRED], [GLIB_VERSION_2_40], [Dont warn using older APIs])
AC_DEFINE([GLIB_VERSION_MAX_ALLOWED], [GLIB_VERSION_2_40], [Prevents using newer APIs])

# Checks for header files.
AC_CHECK_HEADERS([string.h stdio.h stdlib.h locale.h ctype.h execinfo.h])
AC_CHECK_FUNCS([setlocale backtrace backtrace_symbols_fd exit])
AC_CONFIG_FILES([Makefile src/Makefile data/Makefile po/Makefile.in])
AC_OUTPUT
