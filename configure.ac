#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([gnome-mpv], [0.2], [jdoe@example.com])
AC_CONFIG_SRCDIR([src/main.c])
AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE

IT_PROG_INTLTOOL([0.40.6])
GETTEXT_PACKAGE=gnome-mpv
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

AM_GLIB_GNU_GETTEXT

AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug], [turn on debugging]))

AS_IF(	[test "x$enable_debug" = "xyes"],
	[CFLAGS="-ansi -Wall -pedantic -g -fsanitize=address"],
	[CFLAGS="-O2 -fomit-frame-pointer"] )

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES(GTK, [gtk+-3.0])

AC_CHECK_LIB(	[mpv],
		[mpv_command],
		[],
		[echo "libmpv is missing. Aborting..."; exit -1;] )

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([	fcntl.h inttypes.h limits.h locale.h mach/mach.h \
			malloc.h stddef.h stdint.h stdlib.h string.h strings.h \
			sys/ioctl.h sys/mount.h sys/param.h sys/time.h \
			sys/vfs.h termios.h unistd.h wchar.h ])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_FSEEKO
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRCOLL
AC_FUNC_STRNLEN
AC_FUNC_STRTOD
AC_CHECK_FUNCS([	atexit clock_gettime dup2 floor ftruncate getcwd \
			gettimeofday memchr memmove memset mkdir munmap pow \
			putenv rint select setlocale sqrt strcasecmp strchr \
			strcspn strdup strerror strncasecmp strpbrk strrchr \
			strspn strstr strtol strtoul ])

AC_CONFIG_FILES([Makefile src/Makefile po/Makefile.in])
AC_OUTPUT
