#
# src/Makefile.am for qalculate
#

AM_CPPFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libqalculate \
	@LIBCURL_CFLAGS@ @LIBXML_CFLAGS@ @ICU_CFLAGS@ @GIO_CFLAGS@

bin_PROGRAMS = @QALCULATE_TEXT@
noinst_PROGRAMS = @QALCULATE_DEFS2DOC@ @QALCULATE_TEST@
EXTRA_PROGRAMS = qalc defs2doc test

qalc_SOURCES = qalc.cc

qalc_LDADD = \
	@LIBXML_LIBS@ \
	@LIBCURL_LIBS@ \
	@ICU_LIBS@ \
	@GIO_LIBS@ \
	@READLINE_LIBS@ \
	@LTLIBINTL@ \
	@LTLIBICONV@ \
	../libqalculate/libqalculate.la

defs2doc_SOURCES = defs2doc.cc

defs2doc_LDADD = \
	@LIBXML_LIBS@ \
	@LIBCURL_LIBS@ \
	@ICU_LIBS@ \
	@GIO_LIBS@ \
	@LTLIBINTL@ \
	@LTLIBICONV@ \
	../libqalculate/libqalculate.la

test_SOURCES = test.cc

test_LDADD = \
	@LIBXML_LIBS@ \
	@LIBCURL_LIBS@ \
	@ICU_LIBS@ \
	@GIO_LIBS@ \
	@LTLIBINTL@ \
	@LTLIBICONV@ \
	../libqalculate/libqalculate.la

#install-exec-local:
#	cd $(DESTDIR)$(bindir) && rm -f qalculate; $(LN_S) @LN_QALCULATE@ qalculate

