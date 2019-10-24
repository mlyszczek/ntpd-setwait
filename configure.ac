AC_INIT([ntpd-setwait], [0.1.1], [michal.lyszczek@bofc.pl])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([ntpd-setwait-config.h])
AC_PROG_CC
AC_CANONICAL_HOST
AC_CONFIG_FILES([Makefile www/Makefile])


###
# --enable-analyzer
#

AC_ARG_ENABLE([analyzer],
    AS_HELP_STRING([--enable-analyzer], [Enable static code analyzer]),
    [enable_analyzer="yes"], [enable_analyzer="no"])
AM_CONDITIONAL([ENABLE_ANALYZER], [test "x$enable_analyzer" = "xyes"])


AC_SEARCH_LIBS([socket], [socket])

AC_OUTPUT
