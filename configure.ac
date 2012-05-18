AC_PREREQ([2.63])

AC_INIT([guacamayo-watchdog], [0.1], [http://github.com/Guacamayo/gmayo-watchdog])

AC_CONFIG_SRCDIR([src/gmayo-watchdog.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 foreign -Wno-portability no-define no-dist-gzip dist-bzip2])

AM_SILENT_RULES([yes])

# Checks for programs.
AM_PROG_CC_C_O
AC_HEADER_STDC

AM_PATH_GLIB_2_0([2.30.0], [], [AC_MSG_ERROR([glib-2.0 not found])])

GMAYOW_CFLAGS="$GLIB_CFLAGS"
GMAYOW_LIBS="$GLIB_LIBS"

AC_SUBST(GMAYOW_CFLAGS)
AC_SUBST(GMAYOW_LIBS)

AC_CONFIG_FILES([
	Makefile
	src/Makefile
])

AC_OUTPUT

echo "
    guacamayo-watchdog

    Prefix: ${prefix}
"