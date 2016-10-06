AC_PREREQ(2.61)
AC_INIT([bt-app], 1.0.0)
AM_INIT_AUTOMAKE([-Wall gnu foreign])
AM_MAINTAINER_MODE
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CXX
AM_PROG_AS
AC_PROG_LIBTOOL
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
PKG_PROG_PKG_CONFIG

# Library configs
AC_ARG_WITH([common_includes],
      AC_HELP_STRING([--with-common-includes=@<:@dir@:>@],
         [Specify the location of the common headers]),
      [common_incdir=$withval],
      with_common_includes=no)

if test "x$with_common_includes" != "xno"; then
   CFLAGS="${CFLAGS} -I${common_incdir}"
fi

AC_ARG_WITH([lib_path],
      AC_HELP_STRING([--with-lib-path=@<:@dir@:>@],
         [Specify the location of the libraries]),
      [std_libdir=$withval],
      with_lib_path=no)

if test "x$with_lib_path" != "xno"; then
   SYS_LIB=${std_libdir}
fi

AC_ARG_WITH([btobex],
     AC_HELP_STRING([--with-btobex],
        [enable btobex, building BT-APP which uses BTOBEX]))

AM_CONDITIONAL(USE_BT_OBEX, test "x${with_btobex}" = "xyes")

AC_SUBST(SYS_LIB)
AC_SUBST([CFLAGS])
AC_SUBST([CC])
AC_CONFIG_FILES([Makefile\
    osi/Makefile\
    main/Makefile])
AC_OUTPUT
