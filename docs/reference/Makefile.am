#
# docs/reference/Makefile.am for qalculate
#

EXTRA_DIST = \
		$(top_srcdir)/docs/reference/Doxyfile

if HAVE_DOXYGEN

referencedir = $(docdir)/reference/html
reference_DATA = \
		$(top_srcdir)/docs/reference/html/*.html \
		$(top_srcdir)/docs/reference/html/*.png \
		$(top_srcdir)/docs/reference/html/*.css \
		$(top_srcdir)/docs/reference/html/*.js

.PHONY: docs
docs:
	$(DOXYGEN) Doxyfile

$(top_srcdir)/docs/reference/html/*.html:
	$(DOXYGEN) Doxyfile

$(top_srcdir)/docs/reference/html/*.png $(top_srcdir)/docs/reference/html/*.css $(top_srcdir)/docs/reference/html/*.js:

endif