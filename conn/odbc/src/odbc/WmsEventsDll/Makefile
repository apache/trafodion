# Makefile for building libzqsevents.so shared library on Linux
#
include $(MY_SQROOT)/macros.gmk # top level
include ../macros.gmk       # odbc level

OBJS = $(OUTDIR)/WmsEventsExportDll.o \
       $(OUTDIR)/WmsEventsTokenizedDll.o \
       $(OUTDIR)/WmsEventsLogDll.o \
       $(OUTDIR)/WmsEventsUtil.o \
       $(OUTDIR)/versinfo.o	
       
INCLUDES =  -I. \
 -I./linux \
 -I../WmsCom \
 -I../WmsSync \
 -I../Common \
 -I../Common/Linux \
 -I../EventMsgs \
 -I../dependencies/include \
 -I../dependencies/linux \
 -I../Krypton/generated_incs \
 -I$(MY_SQROOT)/export/include/sql \
 -I$(MY_SQROOT)/inc/tmf_tipapi \
 -I$(MY_SQROOT)/inc \
 -I$(MY_SQROOT)/export/include \
 -I$(MY_SQROOT)/export/include/common \
 -I$(MY_SQROOT)/sql/nq_w/common \
 -I$(PROTOBUFS_INC) \
 -I$(MY_SQROOT)/seapilot/export/include/common \
 -I$(MY_SQROOT)/seapilot/export/include \

ifeq ($(COMPILATION_WARNING_NOT_ALLOWED),1)
WMS_DBG_FLGS = -Werror -Wno-deprecated
else
WMS_DBG_FLGS = -w
endif

CXXFLAGS += -fPIC
DEFINES =  -DNA_LINUX -DSIZEOF_LONG_INT=4 -DUSE_NEW_PHANDLE -DSQ_GUARDIAN_CALL -DZQSEVENTSDLL -DNSK_COM $(WMS_DBG_FLGS) 


libzqsevents.so:	$(OBJS)
	$(CXX) -shared $(GCCMODE) -o $@ $(DBG_FLGS) -lqpidclient -L$(PROTOBUFS_LIB) -lprotobuf -L$(SP_EXPORT_LIB) -lwrappersq -lpublications $(OBJS)
	cp -up $@ $(LIBEXPDIR)
	
$(OUTDIR)/versinfo.o: $(MY_SQROOT)/export/include/SCMBuildStr.h

.PHONY : clean
clean:
	$(RM) -rf $(OUTDIR)
	$(RM) ./libzqsevents.so
	$(RM)  $(LIBEXPDIR)/libzqsevents.so

.PHONY : cleanall
cleanall: clean
	$(RM) -rf $(BUILD_PLAT)

