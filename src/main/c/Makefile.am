# need this for Doxygen integration
#include $(top_srcdir)/aminclude.am

AM_CPPFLAGS = -I${srcdir}/include -I${srcdir}/tests -I${srcdir}/generated
AM_CFLAGS = -Wall -Werror 
AM_CXXFLAGS = -Wall 

LIB_LDFLAGS = -no-undefined -version-info 2

pkginclude_HEADERS = include/Wms.h generated/WmsService_types.h
EXTRA_DIST=LICENSE

COMMON_SRC = include/GlobalData.h include/GlobalHeader.h include/Wms.h include/WmsException.h include/WmsZookeeper.h \
    src/Wms.cpp src/GlobalData.cpp src/WmsException.cpp src/WmsZookeeper.cpp \
    generated/WmsService_constants.cpp generated/WmsService_constants.h generated/WmsService_types.cpp generated/WmsService_types.h \
    generated/WmsService.cpp generated/WmsService.h

noinst_LTLIBRARIES = libwms.la
libwms_la_SOURCES = $(COMMON_SRC) 
libwms_la_LIBADD = -lm

lib_LTLIBRARIES = libwms_st.la
libwms_st_la_SOURCES =
libwms_st_la_LIBADD=libwms.la -lzookeeper_mt -lthrift 
libwms_st_la_DEPENDENCIES=libwms.la  
libwms_st_la_LDFLAGS = $(LIB_LDFLAGS) 

#########################################################################
# build and run unit tests

EXTRA_DIST+=$(wildcard ${srcdir}/tests/*.cpp) $(wildcard ${srcdir}/tests/*.h)

TEST_SOURCES = tests/wms_test.cpp

check_PROGRAMS = wmstest-st
nodist_wmstest_st_SOURCES = $(TEST_SOURCES)
wmstest_st_LDADD = libwms_st.la 
wmstest_st_CXXFLAGS = -DUSE_STATIC_LIB 
wmstest_st_LDFLAGS = -static-libtool-libs 

run-check: check
	./wmstest-st $(TEST_OPTIONS)

clean-local: clean-check
	$(RM) $(DX_CLEANFILES)

clean-check:
	$(RM) $(nodist_wmstest_st_OBJECTS) $(nodist_wmstest_mt_OBJECTS)
