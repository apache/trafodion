# @@@ START COPYRIGHT @@@
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# @@@ END COPYRIGHT @@@

include ../macros.gmk
include ./macros.gmk

# Default build flavor is debug
SQ_BUILD_TYPE ?= debug

ifeq ($(SQ_BUILD_TYPE),release)
  MTYPE_BTYPE=$(SQ_MTYPE)bit/release
else
  MTYPE_BTYPE=$(SQ_MTYPE)bit/debug
endif
ifeq ($(SQ_MTYPE),64)
  SQENV_MTYPE=sqenv.sh
else
  SQENV_MTYPE=sq32env.sh
endif
STUBLIBDIR=stublibs/lib/linux/$(MTYPE_BTYPE)
SQL_W=$(TRAF_HOME)/../sql

    DP2=se

MISC = win misc_sec
SQ_COMPONENTS := make_sqevlog seabed stfs trafconf $(MISC) tm $(SEAMONSTER_TARGET) make_sql make_monitor tools hbase_utilities

.PHONY: $(SQ_COMPONENTS)

buildinfo:
	@echo "Building $(TRAFODION_VER_PROD) Version $(TRAFODION_VER) $(SQ_BUILD_TYPE)"
	@echo ""

all: genverhdr $(SQ_COMPONENTS) 

win: make_sqevlog seabed
	cd src/win; $(MAKE) 2>&1 | sed -e "s/$$/	##(SQF)/" ; exit $${PIPESTATUS[0]}

seabed: make_sqevlog
	cd src/seabed; $(MAKE) 2>&1 | sed -e "s/$$/	##(SQF)/" ; exit $${PIPESTATUS[0]}

trafconf: make_sqevlog
	cd src/trafconf; $(MAKE) 2>&1 | sed -e "s/$$/	##(SQF)/" ; exit $${PIPESTATUS[0]}

tm: seabed make_sqevlog
	cd src/tm; $(MAKE) 2>&1 | sed -e "s/$$/	##(SQF)/" ; exit $${PIPESTATUS[0]}

seamonster: seabed
	cd ../seamonster/src; $(MAKE) 2>&1 | sed -e "s/$$/	##(SEAMONSTER)/" ; exit $${PIPESTATUS[0]}

smstub:
	cd ../smstub/src; $(MAKE) 2>&1 | sed -e "s/$$/	##(SMSTUB)/" ; exit $${PIPESTATUS[0]}


make_sql: $(SEAMONSTER_TARGET) win hbase_utilities
	cd sql; $(MAKE) WROOT=$(SQL_W) 2>&1 | sed -e "s/$$/	##(SQL)/" ; exit $${PIPESTATUS[0]}
	cd $(TRAF_HOME)/sql/scripts && ./makemsg.ksh 2>&1 | sed -e "s/$$/	##(SQL)/" ; exit $${PIPESTATUS[0]}

make_monitor: stfs make_sqevlog seabed trafconf
	cd monitor/linux; $(MAKE) 2>&1 | sed -e "s/$$/	##(SQF)/" ; exit $${PIPESTATUS[0]}

stfs: make_sqevlog
	-cd src/stfs; $(MAKE) -f Makefile.stub  2>&1 | sed -e "s/$$/	##(SQF)/" ; exit $${PIPESTATUS[0]}

monitor_ib:
	-cd monitor/linux && mv $(OUTDIR) $(OUTDIR).tcp && mkdir -p $(OUTDIR) 2>&1 | sed -e "s/$$/	##(SQF)/" ; exit $${PIPESTATUS[0]}
	cd monitor/linux; $(MAKE) 2>&1 | sed -e "s/$$/	##(SQF)/" ; exit $${PIPESTATUS[0]}

make_sqevlog: 
	cd sqevlog; $(MAKE) 2>&1 | sed -e "s/$$/	##(SQF)/" ; exit $${PIPESTATUS[0]}

hbase_utilities: tm
	cd hbase_utilities; $(MAKE) 2>&1 | sed -e "s/$$/  ##(HBASE_UTIL)/" ; exit $${PIPESTATUS[0]}

.PHONY: conn
conn:
	cd ../conn/odbc/src/odbc; $(MAKE) clean; $(MAKE) all 2>&1 | sed -e "s/$$/	##(NDCS)/" ; exit $${PIPESTATUS[0]}

.PHONY: ndcs
ndcs:
	cd ../conn/odbc/src/odbc; $(MAKE) cleanndcs; $(MAKE) ndcs 2>&1 | sed -e "s/$$/	##(NDCS)/" ; exit $${PIPESTATUS[0]}

.PHONY: wms
wms:
	cd ../conn/odbc/src/odbc; $(MAKE) cleanwms; $(MAKE) wms 2>&1 | sed -e "s/$$/	##(WMS)/" ; exit $${PIPESTATUS[0]}

clean: setupdir_clean
	-cd monitor/linux; $(MAKE) clean
	-cd src/seabed; $(MAKE) clean
	-cd src/stfs; $(MAKE) -f Makefile.stub clean
	-cd src/win; $(MAKE) clean
	-cd src/tm; $(MAKE) clean
	-cd sql; $(MAKE) WROOT=$(SQL_W) clean
	-cd sqevlog; $(MAKE) clean
	-cd src/trafconf; $(MAKE) clean
	-cd hbase_utilities; $(MAKE) clean
	# from genverhdr.ksh
	rm -f export/include/SCMBuildStr.h export/include/SCMBuildStr.java export/include/buildId
	# from makemsg.ksh
	rm -f sql/scripts/SqlciErrors.[gm] export/bin*/mxcierrors.cat

# sql uses clean
cleanall: setupdir_clean
	-cd monitor/linux; $(MAKE) cleanall
	-cd src/seabed; $(MAKE) cleanall
	-cd src/stfs; $(MAKE) -f Makefile.stub cleanall
	-cd src/win; $(MAKE) cleanall
	-cd src/tm; $(MAKE) cleanall
	-cd sql; $(MAKE) WROOT=$(SQL_W) clean
	-cd sqevlog; $(MAKE) cleanall
	-cd src/trafconf; $(MAKE) cleanall
	-cd hbase_utilities; $(MAKE) clean
	# from genverhdr.ksh
	rm -f export/include/SCMBuildStr.h export/include/SCMBuildStr.java export/include/buildId
	# from makemsg.ksh
	rm -f sql/scripts/SqlciErrors.[gm] export/bin*/mxcierrors.cat

cleaner:
	-cd monitor/linux; $(MAKE) cleaner
	-cd src/seabed; $(MAKE) cleaner
	-cd src/stfs; $(MAKE) -f Makefile.stub cleaner
	-cd src/win; $(MAKE) cleaner
	-cd src/tm; $(MAKE) cleaner
	-cd src/trafconf; $(MAKE) cleaner
	-cd sql; $(MAKE) WROOT=$(SQL_W) cleaner
	-cd sqevlog; $(MAKE) cleaner

cleanobjs:
	@TMP=/tmp/mc$$$$; \
	trap "rm -f $$TMP; exit 0" 1 2 13 15; \
	echo "echo /bin/rm -f" '"$$*"' > $$TMP; \
	echo "/bin/rm -f" '"$$*"' >> $$TMP; \
	chmod +x $$TMP; \
	find . \( -name '*.o' -o -name '*.so' \) -exec $$TMP '{}' \;
	$(RM) $$TMP


EXPORTBIN_DIR=export/bin$(SQ_MBTYPE)
EXPORTLIB_DIR=export/lib$(SQ_MBTYPE)
EXPORTLIBX_DIR=export/lib
EXPORTINCSQL_DIR=export/include/sql
EXPORTINCEVL_DIR=export/include

# SQL libs
EXPORTSQLLIBS= libarkcmplib.so libarkcmp.so libarkesp.so libarkfsindp2.so libcatman.so \
	libcli.so libcomexe.so libcommon.so libeh.so libexecutor.so libexeindp2.so \
	libfepipe.so libgenerator.so liblangman.so liblmcomp.so liboptimizer.so \
	libparser.so libsmdio.so libsort.so libsqlcat.so libsqlci.so libsqlcomp.so \
	libsqlc.so libsqlmsg.so libtdm_arkfsddl.so libtdm_arkfsdml.so \
	libtdm_arkfsinopen.so libtdm_sqlcli_globals.so libtdm_sqlcli.so \
	libtdm_sqlerrormsg.so libtdm_sqlexport.so libtdm_sqlexp.so libtdm_sqlmxevents.so \
	libtdm_sqlshare.so libustat.so libSqlCompilerDebugger.so libudr_predef.so
# SQL Utility Libraries
EXPORTUTILLIBS= libds.a libdmol.a libddol.a libuol.a libqol.a libdup.a libPaUtiParser.a \
	libuofs.a libpurgedata.a libtse_interface.so
# SQL system executables
EXPORTSQLEXE= tdm_sqlc tdm_arkcmp tdm_arkesp sqlci tdm_udrserv mxlobsrvr
# SQL Utilities executables
EXPORTUTILS= mxtool sup   mxexportddl mximportddl

setupdir:
	mkdir -p $(EXPORTBIN_DIR)
	mkdir -p $(EXPORTLIB_DIR)
	mkdir -p $(EXPORTINCSQL_DIR)
	mkdir -p $(EXPORTINCEVL_DIR)
	@# side-effect directory creation in these targets
	-cd monitor/linux; $(MAKE) setup
	-cd src/win; $(MAKE) setup
	-cd src/seabed; $(MAKE) setup
	-cd src/tm; $(MAKE) setup
	-cd src/trafconf; $(MAKE) setup
	-cd sqevlog; $(MAKE) setup
	@# builds/copies 3rd party dependencies as necessary
	cd sql; $(MAKE) WROOT=$(SQL_W) setup
	@# commands to export
	-ln -sf ../../mondump $(EXPORTBIN_DIR)
	-ln -sf ../../sqid $(EXPORTBIN_DIR)
	-ln -sf ../../sqinfo $(EXPORTBIN_DIR)
	-ln -sf ../../sqvers $(EXPORTBIN_DIR)
	@# SQL Messages .txt file
	-ln -sf $(SQL_W)/bin/SqlciErrors.txt $(EXPORTINCSQL_DIR)
	@# copy some header files
	-ln -sf $(SQL_W)/cli/sqlcli.h $(EXPORTINCSQL_DIR)
	-ln -sf $(SQL_W)/sqludr/sqludr.h $(EXPORTINCSQL_DIR)
	@#
	mkdir -p export/include/nsk
	-ln -sf $(TRAF_HOME)/inc/cextdecs/cextdecs.h export/include/nsk
	@# USTAT
	-mkdir -p export/lib/mx_ustat
	-ln -sf $(SQL_W)/ustat/USAS.sh export/lib/mx_ustat
	-ln -sf $(SQL_W)/ustat/FillAutoTbl.sh export/lib/mx_ustat
	-ln -sf $(SQL_W)/ustat/RunLogUstats.sh export/lib/mx_ustat
	-ln -sf $(SQL_W)/ustat/StopAutoStats.sh export/lib/mx_ustat
	-ln -sf $(SQL_W)/ustat/stats_profile export/lib/mx_ustat

# "setupdir" target clean up
setupdir_clean:
	rm -rf $(EXPORTBIN_DIR)/sqinfo
	rm -rf $(EXPORTLIB_DIR)/librwtlh.so
	rm -rf $(EXPORTLIB_DIR)/libdummy.so $(EXPORTLIB_DIR)/libtdm_measenty.so \
		$(EXPORTLIB_DIR)/libtdm_security.so $(EXPORTLIB_DIR)/libtdm_tfds.so \
		$(EXPORTLIB_DIR)/libtdm_tmflib.so $(EXPORTLIB_DIR)/libtdm_tmfuser.so \
	        $(EXPORTLIB_DIR)/libhdfs*.so $(EXPORTLIB_DIR)/libhadoop*.so \
	        $(EXPORTINCEVL_DIR)/hdfs.h
	@for f in $(EXPORTSQLLIBS); do \
	  echo rm -f $(EXPORTLIB_DIR)/$$f ; \
	       rm -f $(EXPORTLIB_DIR)/$$f ; done
	@for f in $(EXPORTUTILLIBS); do \
	  echo rm -f $(EXPORTLIB_DIR)/$$f ; \
	       rm -f $(EXPORTLIB_DIR)/$$f ; done
	@for f in $(EXPORTSQLEXE); do \
	  echo rm -f $(EXPORTBIN_DIR)/$$f ; \
	       rm -f $(EXPORTBIN_DIR)/$$f ; done
	@for f in $(EXPORTUTILS); do \
	  echo rm -f $(EXPORTBIN_DIR)/$$f ; \
	       rm -f $(EXPORTBIN_DIR)/$$f ; done
	rm -f $(EXPORTLIBX_DIR)/mxlangman.jar
	-rm -rf $(EXPORTINCSQL_DIR) export/include/nsk export/lib/mx_ustat

######
#
#  Automated Build and Release Targets
#
######

#	NOTE:	This is another target that could be a recursive
#		call into each subdir.  That would allow each
#		subdir to have it's own header / version info

.PHONY:	genverhdr

PV_BRANCH ?= $(shell build-scripts/build.branch)
PV_BUILDID ?= $(shell whoami)
PV_DATE ?= $(shell date +%d%b%y)
PV_FUNBRANCH ?= $(shell build-scripts/build.branch -f)
SQVFILE = sqenvcom.sh
PV_MAJOR ?=  $(shell grep TRAFODION_VER_MAJOR=  $(SQVFILE) | cut -f2 -d=)
PV_MINOR ?=  $(shell grep TRAFODION_VER_MINOR=  $(SQVFILE) | cut -f2 -d=)
PV_UPDATE ?= $(shell grep TRAFODION_VER_UPDATE= $(SQVFILE) | cut -f2 -d=)

genverhdr: buildinfo
	cd build-scripts && ./genverhdr.ksh -major "${PV_MAJOR}" \
                                        -minor "${PV_MINOR}" \
                                        -update "${PV_UPDATE}" \
                                        -build "${PV_BUILDID}" \
                                        -flavor "${SQ_BUILD_TYPE}" \
                                        -funbranch "${PV_FUNBRANCH}" \
                                        -branch "${PV_BRANCH}" \
                                        -date "${PV_DATE}"

PKG_PROD = apache-trafodion
PKG_PHX = phoenix
PKG_DCS = dcs
OS_TYPE = RH
OS_MAJOR ?=  $(shell lsb_release -rs | cut -f1 -d.)
P_TYPE = $(shell uname -p)

ifeq ($(SQ_BUILD_TYPE),release)
  PKG_TYPE="server-$(TRAFODION_VER)-${OS_TYPE}${OS_MAJOR}-${P_TYPE}"
else
  PKG_TYPE="server-$(TRAFODION_VER)-${OS_TYPE}${OS_MAJOR}-${P_TYPE}-debug"
endif

PKG_BIN ?= "${PKG_PROD}.bin"
PKG_TAR ?= "../../${DISTRIBUTION_DIR}/${PKG_PROD}_$(PKG_TYPE).tar.gz"
PKG_TAR_UN = "${PKG_PROD}_${SQ_BUILD_TYPE}.tar"
SRC_PKG_BIN ?= "../../${DISTRIBUTION_DIR}/${PKG_PROD}-src.bin"
SRC_PKG_TAR ?= "../../${DISTRIBUTION_DIR}/${PKG_PROD}-src.tgz"
PKG_PHX_TAR ?= "../../${DISTRIBUTION_DIR}/${PKG_PHX}-tests.tgz"
PKG_DCS_TEST_TAR ?= "../../${DISTRIBUTION_DIR}/${PKG_DCS}-tests.tgz"

PKG_BIN_OBJS ?= export/bin$(SQ_MBTYPE) export/include export/lib export/lib$(SQ_MBTYPE) export/share gdb sql/scripts sqenvcom.sh opt tools conf hbase_utilities/backup_and_restore

PKG_BIN_OBJS += sysinstall
PKG_BIN_OBJS += trafci
PKG_BIN_OBJS += samples
PKG_BIN_OBJS += export/limited-support-tools
PKG_BIN_OBJS += LICENSE NOTICE 
PKG_BIN_OBJS += rest-${TRAFODION_VER} dcs-${TRAFODION_VER}

PKG_BIN_DIRS ?= sql export
PKG_BIN_DIRS += trafci
PKG_BIN_DIRS += samples

# Package Regression variables (parser file is for compGeneral/TESTTOK2)
PKG_REGRESS_OBJS ?= $(SQL_W)/regress inc/seaquest/sqtypes.h inc/fs/feerrors.h $(SQL_W)/cli/sqlcli.h $(SQL_W)/cli/SQLCLIdev.h $(SQL_W)/parser/sqlparser.y
REGRESS_EXCLUDE_FILES = "--exclude=$(SQL_W)/regress/seabase"
REGRESS_EXCLUDE_FILES =

PKG_REGRESS_TAR ?= "../../${DISTRIBUTION_DIR}/${PKG_PROD}-regress.tgz"


# Package SeaMonster kernel module
SEAMONSTER_TAR ?= "seamonster_${SQ_BUILD_TYPE}-src.tar.gz"

#	Targets to tar/gzip the tree
package: pkglist-files pkglist-symlinks tmp/sqenv.sh conf/dcs conf/rest
	mkdir -p ../../${DISTRIBUTION_DIR}
	chmod o-rwx $(PKG_BIN_DIRS)
	chmod -R o-rwx $(PKG_BIN_OBJS)
	tar --mode o-rwx --no-recursion -chf $(PKG_TAR_UN) -X build-scripts/package.exclude -X build-scripts/package.exclude.$(SQ_MBTYPE) -T pkglist-files
	tar --mode o-rwx -rf $(PKG_TAR_UN) -X build-scripts/package.exclude -X build-scripts/package.exclude.$(SQ_MBTYPE) -T pkglist-symlinks
	cd tmp; tar --mode o-rwx -rf ../$(PKG_TAR_UN) sqenv.sh
	gzip -c $(PKG_TAR_UN) > $(PKG_TAR)
	rm -f $(PKG_TAR_UN) pkglist-files pkglist-symlinks tmp/sqenv.sh

LICENSE: ../../licenses/LICENSE-server
	cp -f $? $@

../../licenses/LICENSE-server:
	cd $(@D) && $(MAKE) $(@F)

NOTICE: ../../licenses/NOTICE-server
	cp -f $? $@

../../licenses/NOTICE-server:
	cd $(@D) && $(MAKE) $(@F)

rest-${TRAFODION_VER}:
	ln -sf ../rest/target/$@/$@ $@

conf/rest: rest-${TRAFODION_VER}
	mkdir -p $@
	cp -f rest-${TRAFODION_VER}/conf/* $@/

dcs-${TRAFODION_VER}:
	ln -sf ../../dcs/target/$@/$@ $@

conf/dcs: dcs-${TRAFODION_VER}
	mkdir -p $@
	cp -f dcs-${TRAFODION_VER}/conf/* $@/

tmp/sqenv.sh:
	@if [ ! -d tmp ]; then mkdir tmp; fi
	echo "export SQ_BUILD_TYPE=$(SQ_BUILD_TYPE)" > tmp/sqenv.sh
	echo "" >> tmp/sqenv.sh
	echo "# $(SQENV_MTYPE)" >> tmp/sqenv.sh
	cat $(SQENV_MTYPE) >> tmp/sqenv.sh

# regular directories and files, following symlinks, minus simple links
pkglist-files: pkglist-symlinks
	find -L $(PKG_BIN_OBJS) -type d -o -type f | grep -v -xf pkglist-symlinks > $@

# simple symlinks that point to filename (does not start with . or /)
pkglist-symlinks: LICENSE NOTICE rest-${TRAFODION_VER} dcs-${TRAFODION_VER}
	find $(PKG_BIN_OBJS) -lname '[^./]*' > $@

#	Targets to tar/gzip the self installer
packagebin:
	cat $(TRAF_HOME)/sql/scripts/sqinstall > $(PKG_BIN)
	echo "__ARCHIVE_BELOW__" >> $(PKG_BIN)
	tar chzf - -X  build-scripts/package.exclude $(PKG_BIN_OBJS) >> $(PKG_BIN)
	chmod +x $(PKG_BIN)

#	Targets to create self-installer with sqinstaller
packageinstaller:
	cat $(TRAF_HOME)/sql/scripts/sqinstaller > $(PKG_BIN)
	echo "__ARCHIVE_BELOW__" >> $(PKG_BIN)
	tar chzf - -X  build-scripts/package.exclude $(PKG_BIN_OBJS) >> $(PKG_BIN)
	chmod +x $(PKG_BIN)

srcpackage:
	-rm $(TRAF_VAR)/ms.env
	tar chzf ${SRC_PKG_TAR} -X build-scripts/package.exclude -X build-scripts/package.exclude.$(SQ_MBTYPE) .

#	Targets to tar/gzip the regression tree
package-regress:
	tar chzf $(PKG_REGRESS_TAR) -X build-scripts/package.exclude $(REGRESS_EXCLUDE_FILES) $(PKG_REGRESS_OBJS)

#	Targets to tar/gzip the phoenix tests
package-phx:
	tar chzf $(PKG_PHX_TAR) ../../tests/phx 

#	Targets to tar/gzip the dcs tests
package-dcs:
	tar chzf $(PKG_DCS_TEST_TAR) ../../dcs/src/test 

#	Targets to tar/gzip the self installer with sources
srcpackagebin:
	-rm $(TRAF_VAR)/ms.env
	cat $(TRAF_HOME)/sql/scripts/sqinstall > $(SRC_PKG_BIN)
	echo "__ARCHIVE_BELOW__" >> $(SRC_PKG_BIN)
	tar chzf - -X build-scripts/package.exclude -X build-scripts/package.exclude.$(SQ_MBTYPE) . >> $(SRC_PKG_BIN)
	chmod +x $(SRC_PKG_BIN)


#	Convenience target to clean / build / package
fullpackage: clean all package srcpackage

#	Convenience target to clean / build / binary package
fullpackagebin: clean all packagebin srcpackagebin

# doxygen html xref
#
doxygen:
	doxygen sqf_doxygen.cfg
