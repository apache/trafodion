## Top-level Makefile with rules for main components

# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
#
## This version number is used by automated build procedures.
## Please don't change the version number unless you know what you are doing.
## Makefile Version: 8  -- SeaMonster
include macros.gmk

# Make Targets
.PHONY: all log4cpp dbsecurity foundation $(MPI_TARGET) ndcs ci jdbc_jar jdbc_type2_jar sqroot $(SEAMONSTER_TARGET) verhdr rest odb
.PHONY: package package-all pkg-product pkg-sql-regress

################
### Main targets
# Server-side only

# Default target (all components)
all: $(MPI_TARGET) log4cpp dbsecurity foundation jdbc_jar $(SEAMONSTER_TARGET) ndcs ci jdbc_type2_jar rest odb

package: pkg-product pkg-client

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

log4cpp: $(MPI_TARGET)
	cd log4cpp/$(LOG4CPP_VER)/src && $(MAKE) liblog4cpp.so 2>&1 | sed -e "s/$$/	##(Log4cpp)/";exit $${PIPESTATUS[0]}

dbsecurity: $(MPI_TARGET)
	cd dbsecurity && $(MAKE) all 2>&1 | sed -e "s/$$/	##(Security)/";exit $${PIPESTATUS[0]}

foundation: dbsecurity $(MPI_TARGET) $(SEAMONSTER_TARGET)
	cd sqf && $(MAKE) all

jdbc_jar: verhdr
	cd conn/jdbc_type4 && $(ANT) deploy 2>&1 | sed -e "s/$$/	##(JDBCT4)/";exit $${PIPESTATUS[0]}

ndcs: jdbc_jar foundation
	cd conn/odbc/src/odbc && $(MAKE) ndcs        2>&1 | sed -e "s/$$/	##(NDCS)/";exit $${PIPESTATUS[0]}
	cd conn/odbc/src/odbc && $(MAKE) bldlnx_drvr 2>&1 | sed -e "s/$$/	##(NDCS)/";exit $${PIPESTATUS[0]}

ci: trafci
trafci: jdbc_jar
	cd conn/trafci && $(ANT) dist 2>&1 | sed -e "s/$$/	##(TRAFCI)/" ; exit $${PIPESTATUS[0]}

jdbc_type2_jar: ndcs
	cd conn/jdbc_type2 && $(ANT)  2>&1 | sed -e "s/$$/	##(JDBC_TYPE2)/" ; exit $${PIPESTATUS[0]}
	cd conn/jdbc_type2 && $(MAKE) 2>&1 | sed -e "s/$$/	##(JDBC_TYPE2)/" ; exit $${PIPESTATUS[0]}
	
rest: verhdr 
	cd rest && $(MAKE) 2>&1 | sed -e "s/$$/  ##(REST)/" ; exit $${PIPESTATUS[0]}

odb: ndcs 
	cd conn/odb && $(MAKE) 2>&1 | sed -e "s/$$/	##(ODB)/" ; exit $${PIPESTATUS[0]}
    
clean: sqroot
	cd $(MPI_TARGET) &&		$(MAKE) clean-local
	cd $(SEAMONSTER_TARGET)/src &&	$(MAKE) clean
	cd log4cpp/$(LOG4CPP_VER)/src &&	$(MAKE) clean
	cd dbsecurity &&		$(MAKE) clean
	cd sqf &&			$(MAKE) clean
	cd conn/odbc/src/odbc &&	$(MAKE) clean
	cd conn/trafci        &&	$(ANT) clean
	cd conn/jdbc_type4    &&	$(ANT) clean
	cd conn &&			$(MAKE) clean
	cd conn/jdbc_type2 &&		$(ANT) clean && $(MAKE) clean
	cd rest &&			$(MAKE) clean
	cd conn/odb && 			$(MAKE) clean

cleanall: sqroot
	cd $(MPI_TARGET) &&		$(MAKE) clean-local
	cd log4cpp/$(LOG4CPP_VER)/src &&	$(MAKE) cleanall
	cd dbsecurity &&		$(MAKE) cleanall
	cd sqf &&			$(MAKE) cleanall
	cd conn/odbc/src/odbc &&	$(MAKE) cleanall
	cd conn/trafci        &&	$(ANT) clean
	cd conn/jdbc_type4    &&	$(ANT) clean
	cd conn &&			$(MAKE) clean
	cd conn/jdbc_type2 &&	        $(ANT) clean && $(MAKE) clean
	cd rest &&			$(MAKE) clean
	cd conn/odb &&			$(MAKE) clean

package-all: package pkg-sql-regress

pkg-product: all
	cd sqf && $(MAKE) package 2>&1 | sed -e "s/$$/	##(Package)/";exit $${PIPESTATUS[0]}

pkg-client: ci ndcs
	cd conn &&  make all 2>&1 | sed -e "s/$$/	##(Package clients)/" ; exit $${PIPESTATUS[0]}

# Package SQL regression tests (all target produces some regress/tool files so do that first)
pkg-sql-regress: all
	cd sqf && $(MAKE) package-regress 2>&1 | sed -e "s/$$/	##(Package)/";exit $${PIPESTATUS[0]}

version:
	@cd sqf; unset SQ_VERBOSE; source sqenv.sh ; echo "$${TRAFODION_VER}"

# Check that Environment variables are set correctly
sqroot:
	./bldenvchk.sh;

# Check for absolute filenames used as dynamic linked libraries
find-absolute-dlls:
	sqf/build-scripts/find-abs-dlls
