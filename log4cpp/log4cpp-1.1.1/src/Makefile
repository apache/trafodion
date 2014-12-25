# Makefile for building log4cpp shared library on Linux
#
include $(MY_SQROOT)/macros.gmk # top level
include ./macros.gmk # current level

OBJS  = $(OUTDIR)/AbortAppender.o \
        $(OUTDIR)/Appender.o \
	$(OUTDIR)/AppenderSkeleton.o \
	$(OUTDIR)/AppendersFactory.o \
	$(OUTDIR)/BasicConfigurator.o \
	$(OUTDIR)/BasicLayout.o \
	$(OUTDIR)/BufferingAppender.o \
	$(OUTDIR)/Category.o \
	$(OUTDIR)/CategoryStream.o \
	$(OUTDIR)/Configurator.o \
	$(OUTDIR)/DllMain.o \
	$(OUTDIR)/DummyThreads.o \
	$(OUTDIR)/FactoryParams.o \
	$(OUTDIR)/FileAppender.o \
	$(OUTDIR)/Filter.o \
	$(OUTDIR)/FixedContextCategory.o \
	$(OUTDIR)/HierarchyMaintainer.o \
	$(OUTDIR)/IdsaAppender.o \
	$(OUTDIR)/LayoutAppender.o \
	$(OUTDIR)/LayoutsFactory.o \
	$(OUTDIR)/LevelEvaluator.o \
	$(OUTDIR)/Localtime.o \
	$(OUTDIR)/LoggingEvent.o \
	$(OUTDIR)/Manipulator.o \
	$(OUTDIR)/MSThreads.o \
	$(OUTDIR)/NDC.o \
	$(OUTDIR)/NTEventLogAppender.o \
	$(OUTDIR)/OmniThreads.o \
	$(OUTDIR)/OstreamAppender.o \
	$(OUTDIR)/PThreads.o \
	$(OUTDIR)/PassThroughLayout.o \
	$(OUTDIR)/PatternLayout.o \
	$(OUTDIR)/PortabilityImpl.o \
	$(OUTDIR)/Priority.o \
	$(OUTDIR)/Properties.o \
	$(OUTDIR)/PropertyConfigurator.o \
	$(OUTDIR)/PropertyConfiguratorImpl.o \
	$(OUTDIR)/RemoteSyslogAppender.o \
	$(OUTDIR)/RollingFileAppender.o \
	$(OUTDIR)/SimpleConfigurator.o \
	$(OUTDIR)/SimpleLayout.o \
	$(OUTDIR)/SmtpAppender.o \
	$(OUTDIR)/StringQueueAppender.o \
	$(OUTDIR)/SyslogAppender.o \
	$(OUTDIR)/StringUtil.o \
	$(OUTDIR)/TimeStamp.o \
	$(OUTDIR)/TriggeringEventEvaluatorFactory.o \
	$(OUTDIR)/Win32DebugAppender.o \
	$(OUTDIR)/verslog4cpplib.o


INCLUDES = -I. -I../include -I$(MY_SQROOT)/inc -I$(MY_SQROOT)/export/include -I/usr/include -I$(TOOLS_DIR)/dest-llvm-3.2/debug/include -I$(TOOLS_DIR)/dest-llvm-3.2/debug/include/llvm -I$(TOOLS_DIR)/udis86-1.7.2/include

DEFINES = -DNA_LINUX -DSQ_LINUX -DNGG -D_M_DG -D_NSKFS_ -D_FULL_BUILD -DARKFS_GENERATOR -D_DP2NT_ -D_LITTLE_ENDIAN_ -DARG_PRESENT_OMIT -DNSK_USE_MSGSYS_SHELLS -D_REENTRANT -D_GLIBCXX_DEPRECATED -D_TNS_R_TARGET -D_X86_ -DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x402 -DNA_ITOA_NOT_SUPPORTED -DMPI_ -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -DNA_CMPDLL  -DSQ_CPP_INTF -DSQ_NEW_PHANDLE -DUSE_NEW_PHANDLE -DSP_DIS -DNA_64BIT -D_DEBUG

CODEGEN = -fPIC

EXTRA_FLAGS = -std=c++0x -fshort-wchar -fno-strict-aliasing -msse2 -mfpmath=sse -g3 -ggdb -Wno-unknown-pragmas -Wreturn-type -Wformat -Werror

DEFINES += $(EXTRA_FLAGS)

# Produce liblog4cpp.so
liblog4cpp.so: $(OBJS)
	$(CXX) -shared $(GCCMODE) -o $@ $(DBG_FLGS) -L$(LIBEXPDIR) $(OBJS)
	cp -up $@ $(LIBEXPDIR)
	$(RM) liblog4cpp.so

#$(OUTDIR)/libmxocore_version.o: $(MY_SQROOT)/export/include/SCMBuildStr.h

clean:
	$(RM) liblog4cpp.so $(LIBEXPDIR)/liblog4cpp.so
	$(RM) -rf $(OUTDIR)

cleanall: clean
	$(RM) -rf $(BUILD_PLAT)
