#
# po-defs/Makefile.am for qalculate
#

LINK_FOR_BUILD.cpp = $(CXX_FOR_BUILD) $(CXXFLAGS_FOR_BUILD) $(CPPFLAGS_FOR_BUILD) $(TARGET_ARCH_FOR_BUILD) $(LDFLAGS_FOR_BUILD)

all: fixpo$(BUILD_EXEEXT)
install: fixpo$(BUILD_EXEEXT)

fixpo$(BUILD_EXEEXT): remove-untranslated.cc
	$(LINK_FOR_BUILD.cpp) $^ $(LOADLIBES_FOR_BUILD) $(LDLIBS_FOR_BUILD) -o $@

EXTRA_DIST = \
	remove-untranslated.cc POTFILES.in ca.po es.po fr.po nl.po ru.po sv.po zh_CN.po zh_TW.po

DISTCLEANFILES = fixpo$(BUILD_EXEEXT)