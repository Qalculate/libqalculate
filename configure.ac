dnl
dnl configure.in for libqalculate
dnl

dnl  ----------------------
dnl | initialize autotools |---------------------------------------------------
dnl  ----------------------

AC_INIT([libqalculate], [3.3.0])
AC_CONFIG_SRCDIR(libqalculate/Calculator.cc)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_CONFIG_MACRO_DIRS([m4])

AC_ARG_ENABLE(textport, 
              [  --disable-textport	  Disable compiling the text port], 
              enable_textport=$enableval, 
              enable_textport="yes")

AC_ARG_ENABLE(defs2doc, 
              [  --enable-defs2doc	  Enable compiling the definitions documentation generator], 
              enable_defs2doc=$enableval, 
              enable_defs2doc="no")

AC_ARG_ENABLE(tests, 
              [  --enable-tests	  Enable compiling test programs], 
              enable_tests=$enableval, 
              enable_tests="no")


dnl  -------------------------------
dnl | check for neccessary programs |------------------------------------------
dnl  -------------------------------

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_STDC
AC_PROG_LN_S
AC_HEADER_STDC
AC_PROG_INTLTOOL
AM_PROG_LIBTOOL

dnl  ------------------------------------
dnl | check for compiler characteristics |-------------------------------------
dnl  ------------------------------------

dnl Replace -Os with -O2 to stop segfault on startup
if test "x$GCC" = "xyes"; then
	case $CFLAGS in
	*-Os*)
		CFLAGS="$CFLAGS -O2"
		;;
	esac
	case $CXXFLAGS in
	*-Os*)
		CXXFLAGS="$CXXFLAGS -O2"
		;;
	esac
fi


dnl Use -Wall if we have gcc.
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
    *[\ \	]-Wall[\ \	]*) ;;
    *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl

dnl check for threading model
dnl TODO
AC_DEFINE([HAVE_PTHREADS],1,
        [Define if pthreads are present.])

dnl libtool versioning for libqalculate

dnl increment if the interface has additions, changes, removals.
QALCULATE_CURRENT=23

dnl increment any time the source changes; set to 
dnl  0 if you increment CURRENT
QALCULATE_REVISION=0

dnl increment if any interfaces have been added; set to 0
dnl  if any interfaces have been removed. removal has 
dnl  precedence over adding, so set to 0 if both happened.
QALCULATE_AGE=2

AC_SUBST(QALCULATE_CURRENT)
AC_SUBST(QALCULATE_REVISION)
AC_SUBST(QALCULATE_AGE)


dnl  --------------------------------
dnl | check for neccessary libraries |-----------------------------------------
dnl  --------------------------------

AC_CHECK_LIB(pthread, pthread_create, [LIBS="$LIBS -lpthread"])

AC_CHECK_HEADERS(mpfr.h, [LIBS="$LIBS -lmpfr -lgmp"], [AC_MSG_ERROR([Unable to find mpfr header])])

AC_LANG([C++])

AC_CHECK_HEADERS([unordered_map])
AC_CHECK_FUNCS([pipe2])

AC_ARG_WITH([libcurl], AS_HELP_STRING([--with-libcurl], [support for built-in retrieval of exchange rates]), [], [with_libcurl=yes])
AS_IF([test "x$with_libcurl" = "xyes"], [
	PKG_CHECK_MODULES([LIBCURL], [libcurl])
	AC_DEFINE([HAVE_LIBCURL], [1], [Use libcurl])
	AC_SUBST(LIBCURL_CFLAGS)
	AC_SUBST(LIBCURL_LIBS)
])

AC_ARG_WITH([icu], AS_HELP_STRING([--with-icu], [support for case-insensitive unicode strings]), [], [with_icu=yes])
AS_IF([test "x$with_icu" = "xyes"], [
	PKG_CHECK_MODULES([ICU], [icu-uc])
	AC_DEFINE([HAVE_ICU], [1], [Use icu])
	AC_SUBST(ICU_CFLAGS)
	AC_SUBST(ICU_LIBS)
])

PKG_CHECK_MODULES(LIBXML, [libxml-2.0 >= 2.3.8])
AC_SUBST(LIBXML_CFLAGS)
AC_SUBST(LIBXML_LIBS)

AM_ICONV_LINK()

dnl  --------------------------------
dnl | check for readline            |-----------------------------------------
dnl  --------------------------------

QALCULATE_TEXT=""
LN_QALCULATE=""
USE_READLINE="yes"
READLINE_LIBS=""
if test "x$enable_textport" = "xyes" ; then
  QALCULATE_TEXT="qalc$EXEEXT"
  LN_QALCULATE="qalc"

  AC_MSG_CHECKING([whether to use readline])
  AC_ARG_WITH(readline,
  [  --with-readline         turn on readline [default=yes, default tries -lncurses, -lcurses, -ltermcap]],
    [case "${withval}" in
      yes) USE_READLINE="yes" ; AC_MSG_RESULT([yes]) ;;
      no) USE_READLINE="no" ; AC_MSG_RESULT([no]) ;;
      only) USE_READLINE="yes"
            EXTRA_LIBREADLINE_DEPS=" "
            AC_MSG_RESULT([yes (using only readline)]) ;;
      *) USE_READLINE="yes"
         EXTRA_LIBREADLINE_DEPS="${withval}"
         AC_MSG_RESULT([yes (using extra libraries ${withval})]) ;;
    esac],[AC_MSG_RESULT([${USE_READLINE}])])

  dnl Checks for libraries.
  dnl When checking readline, check using extra libraries first.
  dnl We want to protect against the link somehow succeeding, but only
  dnl failing at runtime, as seems to happen on some BSD systems.
  if test "$USE_READLINE" = yes ; then
   AC_CHECK_HEADERS(readline/readline.h readline/history.h)
   if test "x${ac_cv_header_readline_readline_h}" != "xyes" -o "x${ac_cv_header_readline_history_h}" != "xyes"; then
     AC_MSG_WARN([Could not find the headers for libreadline.])
   else
    if test "$EXTRA_LIBREADLINE_DEPS" = "" ; then
      unset ac_cv_lib_readline_readline
      AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
                   HAVE_LIBREADLINE=false, -lncurses)
      if test x${HAVE_LIBREADLINE} = xtrue ; then
        echo "  Using -lreadline -lncurses"
        EXTRA_LIBREADLINE_DEPS=-lncurses
        AC_DEFINE(HAVE_LIBREADLINE, [1], [Define if the text port uses readline])
      else
        unset ac_cv_lib_readline_readline
        AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
                     HAVE_LIBREADLINE=false, -lcurses)
        if test x${HAVE_LIBREADLINE} = xtrue ; then
          echo "  Using -lreadline -lcurses"
          EXTRA_LIBREADLINE_DEPS=-lcurses
          AC_DEFINE(HAVE_LIBREADLINE, [1], [Define if the text port uses readline])
        else
          unset ac_cv_lib_readline_readline
          AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
                       HAVE_LIBREADLINE=false, -ltermcap)
          if test x${HAVE_LIBREADLINE} = xtrue ; then
            echo "  Using -lreadline -ltermcap"
            EXTRA_LIBREADLINE_DEPS=-ltermcap
            AC_DEFINE(HAVE_LIBREADLINE, [1], [Define if the text port uses readline])
          else
            unset ac_cv_lib_readline_readline
            AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
                         HAVE_LIBREADLINE=false)
            if test x${HAVE_LIBREADLINE} = xtrue ; then
              AC_DEFINE(HAVE_LIBREADLINE, [1], [Define if the text port uses readline])
            fi
          fi
        fi
      fi
    else
      unset ac_cv_lib_readline_readline
      AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
                   HAVE_LIBREADLINE=false, ${EXTRA_LIBREADLINE_DEPS})
      if test x${HAVE_LIBREADLINE} = xtrue ; then
        echo "  Using -lreadline ${EXTRA_LIBREADLINE_DEPS}"
        AC_DEFINE(HAVE_LIBREADLINE, [1], [Define if the text port uses readline])
      fi
    fi
    if test x${HAVE_LIBREADLINE} = xtrue; then
      READLINE_LIBS="-lreadline -lhistory ${EXTRA_LIBREADLINE_DEPS}"
    else
      AC_MSG_WARN([Could not find libreadline.])
    fi
   fi
  fi
fi

AC_SUBST(READLINE_LIBS)
AC_SUBST(QALCULATE_TEXT)

AC_SUBST(LN_QALCULATE)

QALCULATE_DEFS2DOC=""
if test "x$enable_defs2doc" = "xyes" ; then
	QALCULATE_DEFS2DOC="defs2doc$EXEEXT"
fi
AC_SUBST(QALCULATE_DEFS2DOC)

QALCULATE_TEST=""
if test "x$enable_tests" = "xyes" ; then
	QALCULATE_TEST="test$EXEEXT"
fi
AC_SUBST(QALCULATE_TEST)

AC_SEARCH_LIBS([nanosleep],[rt posix4])

dnl  -------------------------------------
dnl | internationalization (i18n) support |------------------------------------
dnl  -------------------------------------

GETTEXT_PACKAGE=libqalculate
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[The package name for gettext])

dnl Please keep this in alphabetical order
ALL_LINGUAS="fr nl sv zh_CN"
AM_GNU_GETTEXT(external)
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl AM_GLIB_GNU_GETTEXT sets $DATADIRNAME
AC_MSG_CHECKING(locale directory)
if test "x$prefix" = "xNONE"; then
  PACKAGE_LOCALE_DIR=$ac_default_prefix/share/locale
else
  PACKAGE_LOCALE_DIR=$prefix/share/locale
fi

dnl lconv
AC_CHECK_MEMBERS([struct lconv.int_p_cs_precedes,] dnl
[struct lconv.int_n_cs_precedes],[],[],[AC_INCLUDES_DEFAULT
#include <locale.h>])

AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "$PACKAGE_LOCALE_DIR",
	[The directory in which qalculate's locale data will be stored])
	 AC_MSG_RESULT("$PACKAGE_LOCALE_DIR")

INTLTOOL_QALCULATE_DEFINITIONS_RULE='%.xml:       %.xml.in       $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po-defs/*.po) ; LC_ALL=C $(INTLTOOL_MERGE) -x -u -c $(top_builddir)/po-defs/.intltool-merge-cache $(top_srcdir)/po-defs $< [$]@' 
AC_SUBST(INTLTOOL_QALCULATE_DEFINITIONS_RULE)

dnl  --------
dnl | output |-----------------------------------------------------------------
dnl  --------

AC_OUTPUT([
  Makefile
  src/Makefile
  libqalculate/Makefile
  data/Makefile
  po/Makefile.in
  po-defs/Makefile
  docs/Makefile
  docs/reference/Makefile
  libqalculate.pc
])

