#
# src/libqalculate/Makefile.am for qalculate
#


lib_LTLIBRARIES = libqalculate.la

libqalculate_la_SOURCES = \
	Function.cc Calculator.cc DataSet.cc \
	Variable.cc ExpressionItem.cc Number.cc	MathStructure.cc \
	Prefix.cc support.h util.cc Unit.cc BuiltinFunctions.cc

libqalculateincludedir = $(includedir)/libqalculate

libqalculateinclude_HEADERS = \
	Function.h Calculator.h DataSet.h Variable.h \
	ExpressionItem.h Number.h MathStructure.h Prefix.h \
	util.h includes.h Unit.h BuiltinFunctions.h \
	qalculate.h

libqalculate_la_LDFLAGS = -version-info $(QALCULATE_CURRENT):$(QALCULATE_REVISION):$(QALCULATE_AGE) -no-undefined

libqalculate_la_LIBADD = \
	@LIBCURL_LIBS@ @LIBXML_LIBS@ @ICU_LIBS@ @LTLIBINTL@ @LTLIBICONV@

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	@LIBCURL_CFLAGS@  @LIBXML_CFLAGS@  @ICU_CFLAGS@
