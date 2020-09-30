#
# src/libqalculate/Makefile.am for qalculate
#


lib_LTLIBRARIES = libqalculate.la

if COMPILED_DEFINITIONS_GIO
definitions.c : $(top_srcdir)/data/definitions.gresource.xml $(top_srcdir)/data/*.xml
	glib-compile-resources --generate-source --target=$@ --sourcedir=$(top_srcdir)/data $(top_srcdir)/data/definitions.gresource.xml

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
	MathStructure-convert.cc MathStructure-decompose.cc MathStructure-limit.cc \
	definitions.c
else
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
endif

libqalculateincludedir = $(includedir)/libqalculate

if COMPILED_DEFINITIONS
prefixes_xml.h: ${top_srcdir}/data/prefixes.xml
	echo "const char * prefixes_xml = " >$@ || (rm $@;exit 1)
	sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/\"/' -e 's/$$/\\n\"/' $< >>$@ || (rm $@;exit 1)
	echo ';' >>$@ || (rm $@;exit 1)

currencies_xml.h: ${top_srcdir}/data/currencies.xml
	echo "const char * currencies_xml = " >$@ || (rm $@;exit 1)
	sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/\"/' -e 's/$$/\\n\"/' $< >>$@ || (rm $@;exit 1)
	echo ';' >>$@ || (rm $@;exit 1)

functions_xml.h: ${top_srcdir}/data/functions.xml
	echo "const char * functions_xml = " >$@ || (rm $@;exit 1)
	sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/\"/' -e 's/$$/\\n\"/' $< >>$@ || (rm $@;exit 1)
	echo ';' >>$@ || (rm $@;exit 1)

variables_xml.h: ${top_srcdir}/data/variables.xml
	echo "const char * variables_xml = " >$@ || (rm $@;exit 1)
	sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/\"/' -e 's/$$/\\n\"/' $< >>$@ || (rm $@;exit 1)
	echo ';' >>$@ || (rm $@;exit 1)

units_xml.h: ${top_srcdir}/data/units.xml
	echo "const char * units_xml = " >$@ || (rm $@;exit 1)
	sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/\"/' -e 's/$$/\\n\"/' $< >>$@ || (rm $@;exit 1)
	echo ';' >>$@ || (rm $@;exit 1)

datasets_xml.h: ${top_srcdir}/data/datasets.xml
	echo "const char * datasets_xml = " >$@ || (rm $@;exit 1)
	sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/\"/' -e 's/$$/\\n\"/' $< >>$@ || (rm $@;exit 1)
	echo ';' >>$@ || (rm $@;exit 1)

planets_xml.h: ${top_srcdir}/data/planets.xml
	echo "const char * planets_xml = " >$@ || (rm $@;exit 1)
	sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/\"/' -e 's/$$/\\n\"/' $< >>$@ || (rm $@;exit 1)
	echo ';' >>$@ || (rm $@;exit 1)

elements_xml.h: ${top_srcdir}/data/elements.xml
	echo "const char * elements_xml = " >$@ || (rm $@;exit 1)
	sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/\"/' -e 's/$$/\\n\"/' $< >>$@ || (rm $@;exit 1)
	echo ';' >>$@ || (rm $@;exit 1)

eurofxref_daily_xml.h: ${top_srcdir}/data/eurofxref-daily.xml
	echo "const char * eurofxref_daily_xml = " >$@ || (rm $@;exit 1)
	sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/\"/' -e 's/$$/\\n\"/' $< >>$@ || (rm $@;exit 1)
	echo ';' >>$@ || (rm $@;exit 1)

rates_json.h: ${top_srcdir}/data/rates.json
	echo "const char * rates_json = " >$@ || (rm $@;exit 1)
	sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/\"/' -e 's/$$/\\n\"/' $< >>$@ || (rm $@;exit 1)
	echo ';' >>$@ || (rm $@;exit 1)

libqalculateinclude_HEADERS = \
	Function.h Calculator.h DataSet.h Variable.h \
	ExpressionItem.h Number.h MathStructure.h Prefix.h \
	util.h includes.h Unit.h BuiltinFunctions.h \
	QalculateDateTime.h qalculate.h \
	prefixes_xml.h units_xml.h currencies_xml.h functions_xml.h variables_xml.h datasets_xml.h planets_xml.h elements_xml.h eurofxref_daily_xml.h rates_json.h
else
libqalculateinclude_HEADERS = \
	Function.h Calculator.h DataSet.h Variable.h \
	ExpressionItem.h Number.h MathStructure.h Prefix.h \
	util.h includes.h Unit.h BuiltinFunctions.h \
	QalculateDateTime.h qalculate.h
endif

noinst_HEADERS = MathStructure-support.h Calculator_p.h bernoulli_numbers.h

libqalculate_la_LDFLAGS = -version-info $(QALCULATE_CURRENT):$(QALCULATE_REVISION):$(QALCULATE_AGE) -no-undefined

libqalculate_la_LIBADD = \
	@LIBCURL_LIBS@ @LIBXML_LIBS@ @ICU_LIBS@ @GIO_LIBS@ @LTLIBINTL@ @LTLIBICONV@

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	@LIBCURL_CFLAGS@  @LIBXML_CFLAGS@  @ICU_CFLAGS@ @GIO_CFLAGS@
