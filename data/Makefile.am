#
# data/Makefile.am for qalculate
#

xmldir = $(datadir)/qalculate

xml_in_files = currencies.xml.in datasets.xml.in elements.xml.in functions.xml.in planets.xml.in prefixes.xml.in units.xml.in variables.xml.in
if COMPILED_DEFINITIONS
noinst_DATA = $(xml_in_files:.xml.in=.xml) eurofxref-daily.xml rates.json
else
xml_DATA = $(xml_in_files:.xml.in=.xml) eurofxref-daily.xml rates.json
endif

if ENABLE_NLS
%.xml: %.xml.in
	$(AM_V_GEN)GETTEXTDATADIRS=$(srcdir) GETTEXTDATADIR=$(srcdir) $(MSGFMT) --xml -L QALCULATE --template $< -d $(top_builddir)/po-defs-fixed/fixed -o $@
else
%.xml: %.xml.in
	cp $< $@
endif

EXTRA_DIST = $(xml_in_files) eurofxref-daily.xml rates.json its/qalculate.its its/qalculate.loc

DISTCLEANFILES = $(xml_in_files:.xml.in=.xml)

