#
# docs/reference/Makefile.am for qalculate
#

EXTRA_DIST = \
		$(srcdir)/Doxyfile

if HAVE_DOXYGEN

referencedir = $(docdir)/reference/html
reference_DATA = \
		$(builddir)/html/index.html \
		$(builddir)/html/*

.PHONY: docs
docs:
	top_srcdir=$(top_srcdir) $(DOXYGEN) $(srcdir)/Doxyfile

$(builddir)/html/index.html: $(srcdir)/Doxyfile
	top_srcdir=$(top_srcdir) $(DOXYGEN) $(srcdir)/Doxyfile

$(builddir)/html/*:

DISTCLEANFILES = $(reference_DATA)

endif