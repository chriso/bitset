AC_PREREQ([2.60])

# See http://www.gnu.org/software/libtool/manual/libtool.html#Updating-version-info
m4_define([ver], [2.6.0])
m4_define([l_ver], [2:6:0])

AC_INIT([bitset], [ver], [http://github.com/chriso/bitset])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_AUX_DIR(.)
AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CC
AC_PROG_MAKE_SET

AM_INIT_AUTOMAKE([foreign 1.11])

define([AC_LIBTOOL_LANG_CXX_CONFIG], [:])dnl
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])dnl
LT_INIT([dlopen])
LT_INIT([dlopen disable-static])

AC_CHECK_HEADERS([limits.h stdint.h stdlib.h string.h])

AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove])
AX_CC_MAXOPT

version="ver"
library_version="l_ver"

AC_SUBST([library_version])
AC_SUBST([version])

AC_CONFIG_FILES([Makefile include/Makefile lib/Makefile test/Makefile bitset.pc])
AC_OUTPUT
