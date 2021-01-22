## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = po-defs po-defs/fixed data libqalculate src man po docs

EXTRA_DIST = config.rpath  \
	autogen.sh \
	README.translate \
	libqalculate.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libqalculate.pc

if HAVE_DOXYGEN

.PHONY: docs
docs:
	$(MAKE) -C $(top_srcdir)/docs/reference docs

endif
