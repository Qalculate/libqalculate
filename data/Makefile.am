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

%.xml: %.xml.in
	$(AM_V_GEN)GETTEXTDATADIRS=$(srcdir) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po-defs -o $@

EXTRA_DIST = $(xml_in_files) eurofxref-daily.xml rates.json

DISTCLEANFILES = $(xml_in_files:.xml.in=.xml)

