#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2007-2014 Hewlett-Packard Development Company, L.P.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# @@@ END COPYRIGHT @@@
#######################################################################

CPPSRC := \
	JavaSplitter.cpp \
	LmAssert.cpp \
	LmComQueue.cpp \
	LmCommon.cpp \
	LmConnection.cpp\
	LmContManager.cpp \
	LmDebug.cpp \
	LmLangManager.cpp \
	LmLangManagerJava.cpp \
	LmJavaHooks.cpp \
	LmJavaOptions.cpp \
	LmJavaSignatureHelpers.cpp \
	LmJavaSignature.cpp \
	LmJavaType.cpp \
	LmJavaExceptionReporter.cpp \
	LmParameter.cpp \
	LmResultSetJava.cpp \
	LmRoutineJava.cpp \
	LmUtility.cpp \
	Measure.cpp \
	vproclangman.cpp \
	LmLangManagerC.cpp \
	LmRoutine.cpp \
	LmRoutineC.cpp \
	LmRoutineCSql.cpp \
	LmRoutineCSqlRow.cpp \
	LmRoutineCSqlRowTM.cpp \
	vers_libtdm_sqllangman.cpp

SRCPATH := langman bin

ifneq (,$(findstring $(TARGTYPE), linux yos))
  SPECIAL_JSRC := vproc.java
endif

JSRC := MethodValidationFailedException.java \
	LmClassLoader.java \
	LmUtility.java \
	LmCharsetCoder.java \
	test.java

# January 2013. The LmSQLMXDriver class depends on JDK 1.6 or higher
# and many Windows build machines are still on JDK 1.4. We omit
# LmSQLMXDriver from the Windows build. This is not a problem because
# the class is only used on platforms where we run our JDBC T4 driver
# inside the UDR server and Windows is not one of these platforms.

JSRC += LmT2Driver.java
PACKAGE := org/trafodion/sql/udr
JARPREFIX := trafodion-UDR-$(TRAFODION_VER)
JSRC_DIR := $(MY_SQROOT)/../sql/langman
CLASSPATH := $(TARGOBJDIR)/java:$(JSRC_DIR):$(ENV_CLASSPATH)

DEFS := -DLANGMAN -D_CMA_NOWRAPPERS_

INCLUDE_DIRS += sqludr

ifneq (,$(findstring $(TARGTYPE), oss yos))

  CPPSRC+= I3EAdaptor.cpp

  JVM ?= JVM_1_4

  ifeq ($(JVM), JVM_1_4)
    DEFS+= -DJVM_1_4
    INCLUDE_DIRS += jdk/oss/1_4/include
  endif

endif # ifneq (,$(findstring $(TARGTYPE), oss yos))

ifeq ($(TARGTYPE), nt)
  INCLUDE_DIRS += jdk/include jdk/include/win32
  DEFS += -DLM_DLL -DJVM_1_4
  SYS_LIBS := /libpath:"$(JAVALIBDIR)" $(IMAGEHELP) kernel32.lib \
	user32.lib advapi32.lib jvm.lib

  DEP_LIBS := common tdm_sqlexport tdm_sqlcli tdm_sqlexp tdm_sqlshare \
              tdm_arkfsinopen

  EXTERN_OBJS := \
	$(YLIBDIR)/tdm_nsklib.lib \
	$(YLIBDIR)/tdm_fs.lib \
	$(YLIBDIR)/tdm_security.lib \
	$(YLIBDIR)/tdm_tmfuser.lib \
	$(YLIBDIR)/tdm_fs.lib
endif

ifeq ($(FLAVOR), debug)
  DEFS += -DLM_DEBUG
else
  DEFS += -DNDEBUG
endif

$(TARGOBJDIR)/java/$(PACKAGE)/vproc.class: $(TOPDIR)/bin/vproc.h
	rm -f $(TARGOBJDIR)/java/vproc.class $(TARGOBJDIR)/vproc.java
	sh $(TOPDIR)/langman/create_vproc.ksh > $(TARGOBJDIR)/vproc.java
	$(JAVAC) -d $(TARGOBJDIR)/java -classpath $(CLASSPATH) $(TARGOBJDIR)/vproc.java

# Set up explicit dependencies (necessary for parallel builds... and for correctness).
$(TARGOBJDIR)/java/$(PACKAGE)/LmUtility.class : $(TARGOBJDIR)/java/$(PACKAGE)/LmClassLoader.class
$(TARGOBJDIR)/java/$(PACKAGE)/LmUtility.class : $(TARGOBJDIR)/java/$(PACKAGE)/MethodValidationFailedException.class
$(TARGOBJDIR)/java/$(PACKAGE)/test.class : $(TARGOBJDIR)/java/$(PACKAGE)/LmUtility.class
$(TARGOBJDIR)/java/$(PACKAGE)/test.class : $(TARGOBJDIR)/java/$(PACKAGE)/LmClassLoader.class
$(TARGOBJDIR)/java/$(PACKAGE)/test.class : $(TARGOBJDIR)/java/$(PACKAGE)/MethodValidationFailedException.class

ifneq ($(TARGTYPE), nt)
$(JARMANIFEST) : $(MY_SQROOT)/export/include/SCMBuildMan.mf $(TOPDIR)/langman/mxlangman.jar.version
	cat  $(TOPDIR)/langman/mxlangman.jar.version >$@
	cat $(MY_SQROOT)/export/include/SCMBuildMan.mf >>$@
endif
