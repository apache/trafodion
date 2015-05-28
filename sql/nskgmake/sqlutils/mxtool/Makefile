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
	goawayexpr.cpp  \
	infoexpr.cpp    \
	fixupexpr.cpp   \
	mxtool.cpp      \
	mxtoolexpr.cpp  \
	verifyexpr.cpp  \
	verifymsg.cpp

# need Utils.java and ansiname_decompose.java before others!
JSRC := Utils.java \
        ansiname_decompose.java \
        mxtool_verify_obj.java \
        mxtool_verify_sch.java \
        replicate_schema_ddl.java \
        mxtool_remove_obj.java \
        mxtool_repair_obj.java \
        ManageSystemDefaults.java \
        ManageAuditConfig.java \
        map_user.java \
        run_script.java \
        generate_file_list.java

PACKAGE := com/hp/sqlmx
JARPREFIX := spjsql
ifneq ($(TARGTYPE), nt)
  JARMANIFEST := $(TOPDIR)/sqlutils/mxtool/spjsql.jar.mf
endif
CLASSPATH := $(TARGOBJDIR)/java

DEP_LIBS := \
	tdm_ds \
	tdm_uofs \
	tdm_dmo \
	tdm_ddol \
	tdm_uol \
	tdm_purgedata \
	tdm_recover \
	tdm_popindex \
	tdm_refresh \
	tdm_PaUtiParser_mxtool \
	tdm_SchLevelOp \
	porting_layer

YINC := sqlutils/utiparser_mxtool \
	tdm_sqlmxmsg_intl

DEFS := -DNA_MXTOOL

ifeq ($(TARGTYPE), nt)
  DEFS += -DNA_NOPARSERDLL
endif

ifeq ($(TARGTYPE),yos)

  LDFLAGS := 	-L$(TOPDIR)/dll \
		-lzusrdll

  CPPSRC += stubs.cpp vprocmxtool.cpp
  CPPSRC += cleanupexpr.cpp

  ifeq ($(FLAVOR),release)
	DEP_LIBS +=	common tdm_arkfsddl tdm_sqlexp tdm_sqlexport \
			tdm_sqlmxevents tdm_sqlshare tdm_transform tdm_validate
  else
	DEP_LIBS +=	tdm_sqlcli tdm_transform tdm_validate 
  endif

  TARGET := mxtool

else
  DEP_LIBS += tdm_sqlcli arkcmp_dll	
endif

ifeq ($(TARGTYPE),linux)
  CPPSRC += cleanupexpr.cpp vprocmxtool.cpp
  DEP_LIBS += lmcomp tse_interface tdm_transform tdm_validate
endif

MXTOOL_UTIPARSER_OBJDIR := sqlutils/tdm_PaUtiParser_mxtool/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)
INCLUDE_DIRS := sqlutils/utiparser sqlutils/ddol
GMAKE_INC_DIRS := $(MXTOOL_UTIPARSER_OBJDIR)

$(TARGOBJDIR)/mxtool.$(OBJSUFFIX): $(MXTOOL_UTIPARSER_OBJDIR)/yypautiparser.h
$(TARGOBJDIR)/cleanupexpr.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/fixupexpr.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/goawayexpr.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/infoexpr.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/mxtool.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/mxtoolexpr.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/verifyexpr.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/verifymsg.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h

$(TARGOBJDIR)/java/$(PACKAGE)/mxtool_verify_obj.class :  $(TARGOBJDIR)/java/$(PACKAGE)/Utils.class  $(TARGOBJDIR)/java/$(PACKAGE)/ansiname_decompose.class
$(TARGOBJDIR)/java/$(PACKAGE)/mxtool_verify_sch.class :  $(TARGOBJDIR)/java/$(PACKAGE)/Utils.class $(TARGOBJDIR)/java/$(PACKAGE)/ansiname_decompose.class
$(TARGOBJDIR)/java/$(PACKAGE)/mxtool_remove_obj.class :  $(TARGOBJDIR)/java/$(PACKAGE)/Utils.class $(TARGOBJDIR)/java/$(PACKAGE)/ansiname_decompose.class
$(TARGOBJDIR)/java/$(PACKAGE)/mxtool_repair_obj.class :  $(TARGOBJDIR)/java/$(PACKAGE)/Utils.class $(TARGOBJDIR)/java/$(PACKAGE)/ansiname_decompose.class
$(TARGOBJDIR)/java/$(PACKAGE)/replicate_schema_ddl.class :  $(TARGOBJDIR)/java/$(PACKAGE)/Utils.class $(TARGOBJDIR)/java/$(PACKAGE)/ansiname_decompose.class
$(TARGOBJDIR)/java/$(PACKAGE)/ManageSystemDefaults.class :  $(TARGOBJDIR)/java/$(PACKAGE)/Utils.class 
$(TARGOBJDIR)/java/$(PACKAGE)/ManageAuditConfig.class : $(TARGOBJDIR)/java/$(PACKAGE)/Utils.class
$(TARGOBJDIR)/java/$(PACKAGE)/map_user.class : $(TARGOBJDIR)/java/$(PACKAGE)/Utils.class $(TARGOBJDIR)/java/$(PACKAGE)/ansiname_decompose.class
$(TARGOBJDIR)/java/$(PACKAGE)/run_script.class :  $(TARGOBJDIR)/java/$(PACKAGE)/Utils.class $(TARGOBJDIR)/java/$(PACKAGE)/ansiname_decompose.class
$(TARGOBJDIR)/java/$(PACKAGE)/generate_file_list.class :  $(TARGOBJDIR)/java/$(PACKAGE)/Utils.class $(TARGOBJDIR)/java/$(PACKAGE)/ansiname_decompose.class
ifneq ($(TARGTYPE), nt)
$(JARMANIFEST) : $(MY_SQROOT)/export/include/SCMBuildMan.mf $(TOPDIR)/sqlutils/mxtool/spjsql.jar.version
	cat  $(TOPDIR)/sqlutils/mxtool/spjsql.jar.version >$@
	cat $(MY_SQROOT)/export/include/SCMBuildMan.mf >>$@
endif
