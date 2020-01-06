#
# src/libqalculate/Makefile.am for qalculate
#


lib_LTLIBRARIES = libqalculate.la

libqalculate_la_SOURCES = \
	Function.cc Calculator.cc Calculator-plot.cc Calculator-parse.cc \
	Calculator-definitions.cc Calculator-calculate.cc Calculator-convert.cc \
	DataSet.cc Variable.cc ExpressionItem.cc Number.cc MathStructure.cc \
	Prefix.cc support.h util.cc Unit.cc QalculateDateTime.cc \
	BuiltinFunctions-util.cc BuiltinFunctions-trigonometry.cc BuiltinFunctions-matrixvector.cc \
	BuiltinFunctions-datetime.cc BuiltinFunctions-statistics.cc \
	BuiltinFunctions-explog.cc BuiltinFunctions-special.cc BuiltinFunctions-algebra.cc \
	BuiltinFunctions-calculus.cc BuiltinFunctions-number.cc \
	BuiltinFunctions-combinatorics.cc BuiltinFunctions-logical.cc \
	MathStructure-factor.cc MathStructure-gcd.cc MathStructure-integrate.cc \
	MathStructure-isolatex.cc MathStructure-polynomial.cc \
	MathStructure-matrixvector.cc MathStructure-print.cc \
	MathStructure-differentiate.cc MathStructure-calculate.cc MathStructure-eval.cc \
	MathStructure-convert.cc MathStructure-decompose.cc MathStructure-limit.cc

libqalculateincludedir = $(includedir)/libqalculate

libqalculateinclude_HEADERS = \
	Function.h Calculator.h DataSet.h Variable.h \
	ExpressionItem.h Number.h MathStructure.h Prefix.h \
	util.h includes.h Unit.h BuiltinFunctions.h \
	QalculateDateTime.h qalculate.h

noinst_HEADERS = MathStructure-support.h Calculator_p.h bernoulli_numbers.h

libqalculate_la_LDFLAGS = -version-info $(QALCULATE_CURRENT):$(QALCULATE_REVISION):$(QALCULATE_AGE) -no-undefined

libqalculate_la_LIBADD = \
	@LIBCURL_LIBS@ @LIBXML_LIBS@ @ICU_LIBS@ @LTLIBINTL@ @LTLIBICONV@

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	@LIBCURL_CFLAGS@  @LIBXML_CFLAGS@  @ICU_CFLAGS@
