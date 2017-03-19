#
# src/Makefile.am for qalculate
#

AM_CPPFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libqalculate \
	@GLIB_CFLAGS@ \
	@CLN_CFLAGS@

bin_PROGRAMS = @QALCULATE_TEXT@
noinst_PROGRAMS = @QALCULATE_DEFS2DOC@
EXTRA_PROGRAMS = qalc defs2doc

qalc_SOURCES = qalc.cc

qalc_LDADD = \
	@GLIB_LIBS@ \
	@CLN_LIBS@ \
	@READLINE_LIBS@ \
	../libqalculate/libqalculate.la

defs2doc_SOURCES = defs2doc.cc

defs2doc_LDADD = \
	@GLIB_LIBS@ \
	../libqalculate/libqalculate.la

#install-exec-local:
#	cd $(DESTDIR)$(bindir) && rm -f qalculate; $(LN_S) @LN_QALCULATE@ qalculate

