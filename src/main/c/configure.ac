#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT([WMS C client],0.1.0,[mattbrown@hp.com],[wms])
AC_CONFIG_SRCDIR([src/Wms.cpp])

# Save initial CFLAGS and CXXFLAGS values before AC_PROG_CC and AC_PROG_CXX
init_cflags="$CFLAGS"
init_cxxflags="$CXXFLAGS"

# initialize automake
AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_HEADER([config.h])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S

# AC_DISABLE_SHARED
AC_PROG_LIBTOOL

AC_ARG_ENABLE([debug],
 [AS_HELP_STRING([--enable-debug],[enable debug build [default=no]])],
 [],[enable_debug=no])

if test "x$enable_debug" = xyes; then
    if test "x$init_cflags" = x; then
        CFLAGS=""
    fi
    CFLAGS="$CFLAGS -g -O0 -D_GNU_SOURCE"
else
    if test "x$init_cflags" = x; then
        CFLAGS="-g -O2 -D_GNU_SOURCE"
    fi
fi

if test "x$enable_debug" = xyes; then
    if test "x$init_cxxflags" = x; then
        CXXFLAGS=""
    fi
    CXXFLAGS="$CXXFLAGS -g -O0"
else
    if test "x$init_cxxflags" = x; then
        CXXFLAGS="-g -O2"
    fi
fi

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_mutex_lock],[have_pthread=yes],[have_pthread=no])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h sys/utsname.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_HEADER_TIME
AC_CHECK_TYPE([nfds_t],
    [AC_DEFINE([POLL_NFDS_TYPE],[nfds_t],[poll() second argument type])],
    [AC_DEFINE([POLL_NFDS_TYPE],[unsigned int],[poll() second argument type])],
    [#include <poll.h>])

AC_MSG_CHECKING([whether to enable ipv6])

AC_TRY_RUN([ /* is AF_INET6 available? */
#include <sys/types.h>
#include <sys/socket.h>
main()
{
 if (socket(AF_INET6, SOCK_STREAM, 0) < 0)
   exit(1);
 else
   exit(0);
}
], AC_MSG_RESULT(yes) 
   ipv6=yes, 
   AC_MSG_RESULT(no) 
   ipv6=no, 
   AC_MSG_RESULT(no) 
   ipv6=no)

if test x"$ipv6" = xyes; then
  USEIPV6="-DZOO_IPV6_ENABLED"
  AC_SUBST(USEIPV6)
fi

# Checks for library functions.
AC_CHECK_FUNCS([getcwd gethostbyname gethostname getlogin getpwuid_r gettimeofday getuid memmove memset poll socket strchr strdup strerror strtol])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
