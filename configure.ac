AC_PREREQ([2.60])

# See http://www.gnu.org/software/libtool/manual/libtool.html#Updating-version-info
m4_define([v_maj], [2])
m4_define([v_min], [6])
m4_define([v_mic], [0])
m4_define([project_version], [v_maj.v_min.v_mic])

m4_define([lt_cur], [m4_eval(v_maj + v_min)])
m4_define([lt_rev], [v_mic])
m4_define([lt_age], [v_min])

AC_INIT([bitset], [project_version], [http://github.com/chriso/bitset])
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

AC_CHECK_HEADERS([limits.h stdint.h stdlib.h string.h])

AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove])
AX_CC_MAXOPT

version_info="lt_cur:lt_rev:lt_age"
version="lt_cur.lt_rev.lt_age"
AC_SUBST([version_info])
AC_SUBST([version])

AC_CONFIG_FILES([Makefile include/Makefile lib/Makefile test/Makefile bitset.pc])
AC_OUTPUT
