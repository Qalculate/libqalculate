#
# src/libqalculate/Makefile.am for qalculate
#


lib_LTLIBRARIES = libqalculate.la

if COMPILED_DEFINITIONS
definitions.c: ${top_builddir}/data/currencies.xml ${top_builddir}/data/datasets.xml ${top_builddir}/data/elements.xml ${top_srcdir}/data/eurofxref-daily.xml ${top_builddir}/data/functions.xml ${top_builddir}/data/planets.xml ${top_builddir}/data/prefixes.xml ${top_builddir}/data/units.xml ${top_builddir}/data/variables.xml ${top_srcdir}/data/rates.json
	echo > $@ || (rm $@;exit 1)
	for FILE in $+; do \
		printf "const char * " >> $@ || (rm $@;exit 1); \
		basename -- $$FILE | sed 's/[.-]/_/g' >> $@ || (rm $@;exit 1); \
		printf " = " >> $@ || (rm $@;exit 1); \
		sed -e 's/^[ \t]*//' -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/\"/' -e 's/$$/\\n\"/' $$FILE >>$@ || (rm $@;exit 1); \
		printf ';\n\n' >>$@ || (rm $@;exit 1); \
	done

libqalculate_compiled_def_sources = definitions.c
libqalculate_compiled_def_headers = definitions.h
endif

libqalculateincludedir = $(includedir)/libqalculate

libqalculateinclude_HEADERS = \
	Function.h Calculator.h DataSet.h Variable.h \
	ExpressionItem.h Number.h MathStructure.h Prefix.h \
	util.h includes.h Unit.h BuiltinFunctions.h \
	QalculateDateTime.h qalculate.h

noinst_HEADERS = MathStructure-support.h Calculator_p.h ExpressionItem_p.h MathStructure_p.h bernoulli_numbers.h primes.h $(libqalculate_compiled_def_headers)

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

nodist_libqalculate_la_SOURCES = $(libqalculate_compiled_def_sources)

libqalculate_la_LDFLAGS = -version-info $(QALCULATE_CURRENT):$(QALCULATE_REVISION):$(QALCULATE_AGE) -no-undefined

libqalculate_la_LIBADD = \
	@LIBCURL_LIBS@ @LIBXML_LIBS@ @ICU_LIBS@ @LTLIBINTL@ @LTLIBICONV@

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	@LIBCURL_CFLAGS@  @LIBXML_CFLAGS@  @ICU_CFLAGS@
