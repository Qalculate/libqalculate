#
# docs/reference/Makefile.am for qalculate
#

EXTRA_DIST = \
		$(srcdir)/Doxyfile

if HAVE_DOXYGEN

referencedir = $(docdir)/reference/html
reference_DATA = \
		$(builddir)/html/*.html \
		$(builddir)/html/*.png \
		$(builddir)/html/*.css \
		$(builddir)/html/*.js

.PHONY: docs
docs:
	top_srcdir=$(top_srcdir) $(DOXYGEN) $(srcdir)/Doxyfile

$(builddir)/html/*.html: $(srcdir)/Doxyfile
	top_srcdir=$(top_srcdir) $(DOXYGEN) $(srcdir)/Doxyfile

$(builddir)/html/*.png $(builddir)/html/*.css $(builddir)/html/*.js:

endif