## Top-level Makefile with rules for main components

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
#
## This version number is used by automated build procedures.
## Please don't change the version number unless you know what you are doing.
## Makefile Version: 8  -- SeaMonster
include macros.gmk

# Make Targets
.PHONY: all dbsecurity foundation $(MPI_TARGET) ndcs ci jdbc_jar jdbc_type2_jar sqroot $(SEAMONSTER_TARGET) verhdr dcs rest odb
.PHONY: package package-all pkg-product pkg-sql-regress pkg-phx-tests pkg-dcs-tests check-copyrights pkg-installer

################
### Main targets
# Server-side only

# Default target (all components)
all: $(MPI_TARGET) dbsecurity foundation jdbc_jar $(SEAMONSTER_TARGET) ndcs ci jdbc_type2_jar dcs rest odb lib_mgmt

package: pkg-product pkg-client pkg-installer

#############
# Components

mpi: sqroot verhdr
	echo "Building MPI"
	cd mpi && $(MAKE) sq-local 2>&1 | sed -e "s/$$/	##(MPI)/";exit $${PIPESTATUS[0]}

mpistub: sqroot verhdr
	echo "Building MPI stub"
	cd mpistub && $(MAKE) sq-local 2>&1 | sed -e "s/$$/	##(MPISTUB)/";exit $${PIPESTATUS[0]}

seamonster: mpi
	echo "Building SM"
	cd seamonster/src; $(MAKE) all 2>&1 | sed -e "s/$$/	##(SEAMONSTER)/" ; exit $${PIPESTATUS[0]}

smstub: mpistub
	echo "Building SM stub"
	cd smstub/src; $(MAKE) all 2>&1 | sed -e "s/$$/	##(SMSTUB)/" ; exit $${PIPESTATUS[0]}

verhdr:
	cd sqf && $(MAKE) genverhdr

dbsecurity: $(MPI_TARGET)
	cd dbsecurity && $(MAKE) all 2>&1 | sed -e "s/$$/	##(Security)/";exit $${PIPESTATUS[0]}

foundation: sqroot dbsecurity $(MPI_TARGET) $(SEAMONSTER_TARGET)
	cd sqf && $(MAKE) all

jdbc_jar: verhdr
	cd conn/jdbcT4 && $(MAKE) 2>&1 | sed -e "s/$$/	##(JDBCT4)/";exit $${PIPESTATUS[0]}

dcs: jdbc_jar jdbc_type2_jar
	cd ../dcs && $(MAKE) 2>&1 | sed -e "s/$$/  ##(DCS)/" ; exit $${PIPESTATUS[0]}

ndcs: jdbc_jar foundation
	cd conn/odbc/src/odbc && $(MAKE) ndcs        2>&1 | sed -e "s/$$/	##(NDCS)/";exit $${PIPESTATUS[0]}
	cd conn/odbc/src/odbc && $(MAKE) bldlnx_drvr 2>&1 | sed -e "s/$$/	##(NDCS)/";exit $${PIPESTATUS[0]}

ci: trafci
trafci: jdbc_jar
	cd conn/trafci && $(MAKE) 2>&1 | sed -e "s/$$/	##(TRAFCI)/" ; exit $${PIPESTATUS[0]}

jdbc_type2_jar: verhdr foundation
	cd conn/jdbc_type2 && $(MAKE) 2>&1 | sed -e "s/$$/	##(JDBC_TYPE2)/" ; exit $${PIPESTATUS[0]}

rest: verhdr jdbc_jar
	cd rest && $(MAKE) 2>&1 | sed -e "s/$$/  ##(REST)/" ; exit $${PIPESTATUS[0]}

odb: ndcs
	cd conn/odb && $(MAKE) 2>&1 | sed -e "s/$$/	##(ODB)/" ; exit $${PIPESTATUS[0]}

cleantests:
	cd ../dcs/src/test/pytests && $(RM) -r odbc* tox.ini config.ini .tox .testrep* env.sh test_p2.pyc 
	cd ../dcs/src/test/jdbc_test && $(RM) -r jdbcprop pom.xml  target
	cd ../tests/phx && $(RM) -r jdbcprop *.log pom.xml target

clean: 
	cd $(MPI_TARGET) &&		$(MAKE) clean-local
	cd $(SEAMONSTER_TARGET)/src &&	$(MAKE) clean
	cd dbsecurity &&		$(MAKE) clean
	cd sqf &&			$(MAKE) clean
	cd conn/odbc/src/odbc &&	$(MAKE) clean
	cd conn/trafci        &&	$(MAKE) clean
	cd conn/jdbcT4        &&	$(MAKE) clean
	cd conn &&			$(MAKE) clean
	cd conn/jdbc_type2 &&		$(MAKE) clean
	cd rest &&			$(MAKE) clean
	cd conn/odb &&			$(MAKE) clean
	cd ../dcs &&			$(MAKE) clean 
	cd sql/lib_mgmt &&		$(MAKE) clean
	cd ../install &&                $(MAKE) clean
	$(RM) -r ../${DISTRIBUTION_DIR}

cleanall: eclipseclean cleantests
	cd $(MPI_TARGET) &&		$(MAKE) clean-local
	cd dbsecurity &&		$(MAKE) cleanall
	cd sqf &&			$(MAKE) cleanall
	cd conn/odbc/src/odbc &&	$(MAKE) cleanall
	cd conn/trafci        &&	$(MAKE) clean
	cd conn/jdbcT4        &&	$(MAKE) clean
	cd conn &&			$(MAKE) clean
	cd conn/jdbc_type2 && $(MAKE) clean
	cd rest &&			$(MAKE) clean
	cd conn/odb &&			$(MAKE) clean
	cd ../dcs &&			$(MAKE) clean
	cd sql/lib_mgmt &&		$(MAKE) cleanall
	cd ../install &&                $(MAKE) clean
	$(RM) -r ../${DISTRIBUTION_DIR}

package-all: rpmbuild package pkg-sql-regress pkg-phx-tests pkg-dcs-tests 

pkg-product: all dcs rest 
	cd sqf && $(MAKE) package 2>&1 | sed -e "s/$$/	##(Package)/";exit $${PIPESTATUS[0]}

pkg-client: ci ndcs odb
	cd conn &&  make all 2>&1 | sed -e "s/$$/	##(Package clients)/" ; exit $${PIPESTATUS[0]}

# Package SQL regression tests (all target produces some regress/tool files so do that first)
pkg-sql-regress: all
	cd sqf && $(MAKE) package-regress 2>&1 | sed -e "s/$$/	##(Package sql regress)/";exit $${PIPESTATUS[0]}

# Package Phoenix test
pkg-phx-tests: all 
	cd sqf && $(MAKE) package-phx 2>&1 | sed -e "s/$$/	##(Package phoenix)/";exit $${PIPESTATUS[0]}

# Package dcs tests
pkg-dcs-tests: all
	cd sqf && $(MAKE) package-dcs 2>&1 | sed -e "s/$$/	##(Package dcs tests)/";exit $${PIPESTATUS[0]}

# Package installer
pkg-installer: 
	cd ../install && $(MAKE) 2>&1 | sed -e "s/$$/	##(Package installer)/";exit $${PIPESTATUS[0]}

version:
	@cd sqf; unset SQ_VERBOSE; source sqenv.sh ; echo "$${TRAFODION_VER}"

# Check that Environment variables are set correctly and
# create links and files needed as prerequisite for the build
sqroot:
	./bldenvchk.sh;
	cd sqf && $(MAKE) setupdir 2>&1 | sed -e "s/$$/	##(setupdir)/";exit $${PIPESTATUS[0]}

# Check for absolute filenames used as dynamic linked libraries
find-absolute-dlls:
	sqf/build-scripts/find-abs-dlls

check-copyrights:
	python ./updateCopyrightCheck.py

##############
# RPM defines
PKG_PROD=apache-trafodion

OS_TYPE = RH
OS_MAJOR ?=  $(shell lsb_release -rs | cut -f1 -d.)
P_TYPE = $(shell uname -p)

ifeq ($(SQ_BUILD_TYPE),release)
  RELEASE ?= 1
  SERVERTAR="$(PKG_PROD)_server-$(TRAFODION_VER)-${OS_TYPE}${OS_MAJOR}-${P_TYPE}.tar.gz"
else
  RELEASE ?= devel
  SERVERTAR="$(PKG_PROD)_server-$(TRAFODION_VER)-${OS_TYPE}${OS_MAJOR}-${P_TYPE}-debug.tar.gz"
endif

ARCH=$(P_TYPE)

SPECFILE = trafodion.spec

RPMROOT=$(shell cd ..; /bin/pwd)/RPMROOT
RPMDIR=$(RPMROOT)/RPMS
SRPMDIR=$(RPMROOT)/SRPMS
SOURCEDIR=$(RPMROOT)/SOURCES
BUILDDIR=$(RPMROOT)/BUILD
BUILDROOTDIR=$(RPMROOT)/BUILDROOT

rpmpackage: pkg-product
	/bin/rm -rf $(RPMROOT)
	mkdir -p $(RPMDIR)
	mkdir -p $(SOURCEDIR)
	mkdir -p $(BUILDDIR)
	mkdir -p $(BUILDROOTDIR)
	mkdir -p $(SRPMDIR)
	ln -L ../distribution/$(SERVERTAR) $(SOURCEDIR)/$(PKG_PROD)_server-$(TRAFODION_VER).tar.gz

rpmbuild: rpmpackage
	rpmbuild -vv -bb \
	                --define "name $(PKG_PROD)_server" \
	                --define "version $(TRAFODION_VER)" \
	                --define "release $(RELEASE)" \
	                --define "_builddir $(BUILDDIR)" \
	                --define "_buildrootdir $(BUILDROOTDIR)" \
	                --define "_sourcedir $(SOURCEDIR)" \
	                --define "_rpmdir $(RPMDIR)" \
	                --define "_arch $(ARCH)" \
	                --define "_topdir $(RPMROOT)" \
	                $(SPECFILE)
	mkdir -p ../distribution/${OS_TYPE}${OS_MAJOR}
	mv $(RPMROOT)/RPMS/$(ARCH)/$(PKG_PROD)*.rpm ../distribution/${OS_TYPE}${OS_MAJOR}/


eclipse: rest/.project dcs/.project sql/.project sqf/src/seatrans/hbase-trx/.project .project
	@echo
	@echo
	@echo "  Now you can import 5 projects in Eclipse:"
	@echo "      $^"
	@echo
	@echo "  File->Import->General->Existing Projects into Workspace"
	@echo "  Select root directory $(@D)"
	@echo "  Search for nested projects"
	@echo "  Select all or only those you want"
	@echo

rest/.project:
	cd rest && $(MAVEN) eclipse:eclipse

dcs/.project:
	cd ../dcs && $(MAVEN) eclipse:eclipse

sql/.project:
	cd sql  && $(MAVEN) eclipse:eclipse

sqf/src/seatrans/hbase-trx/.project:
	cd sqf/src/seatrans/hbase-trx && $(MAVEN) eclipse:eclipse

.project:
	cp sqf/export/limited-support-tools/eclipse.project .project
	cp sqf/export/limited-support-tools/eclipse.cproject .cproject

eclipseclean:
	$(RM) rest/.classpath rest/.project
	$(RM) ../dcs/.classpath ../dcs/.project
	$(RM) sql/.classpath sql/.project
	$(RM) sqf/src/seatrans/hbase-trx/.classpath sqf/src/seatrans/hbase-trx/.project
	$(RM) .cproject .project

trafinstall:
	cd $(TRAF_HOME)/sql/scripts && install_traf_components

lib_mgmt: foundation
	cd sql/lib_mgmt && $(MAKE) all
