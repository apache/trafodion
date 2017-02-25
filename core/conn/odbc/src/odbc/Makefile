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


.PHONY: all
all: ndcs bldlnx_drvr

# NDCS targets
.PHONY:	ndcs
ndcs: nsksrvrcore nsksrvr

.PHONY:	nsksrvrcore
nsksrvrcore:  
	cd nsksrvrcore; $(MAKE)

.PHONY:	nsksrvr
nsksrvr: nsksrvrcore 
	cd nsksrvr; $(MAKE)

# NDCS clean targets
.PHONY:	cleanndcs
cleanndcs: cleannsksrvrcore cleannsksrvr

.PHONY: cleannsksrvrcore
cleannsksrvrcore:
	cd nsksrvrcore; $(MAKE) clean

.PHONY: cleannsksrvr
cleannsksrvr:
	cd nsksrvr; $(MAKE) clean

# NDCS cleanall targets
.PHONY: cleanallndcs
cleanallndcs:  cleanallnsksrvrcore cleanallnsksrvr

.PHONY: cleanallnsksrvrcore
cleanallnsksrvrcore:
	cd nsksrvrcore; $(MAKE) cleanall

.PHONY: cleanallnsksrvr
cleanallnsksrvr:
	cd nsksrvr; $(MAKE) cleanall

.PHONY: clean
clean: cleanndcs cleanlnx_drvr

.PHONY: cleanall
cleanall: cleanallndcs cleanlnx_drvr
	
# Linux ODBC Driver targets
.PHONY: bldlnx_drvr
include $(TRAF_HOME)/macros.gmk
bldlnx_drvr:
	cd $(TRAF_HOME)/../conn/unixodbc/odbc/odbcclient/unixcli;$(MAKE) -f makefile.lnx linux64_$(SQ_BUILD_TYPE)
	cd $(TRAF_HOME)/../conn/unixodbc/odbc/odbcclient/unixcli/package;rm -rf *.gz $(LIBEXPDIR)/libicu* $(LIBEXPDIR)/libtrafodbc*; ./mklnxpkg.sh -f; cp -p trafodbc_Linux64_pkg.tar.gz $(TRAF_HOME)/../conn/clients/TRAF_ODBC_Linux_Driver_64.tar.gz; cp -Pf relfiles/linux64/lib* $(LIBEXPDIR);cp -p relfiles/linux64/{connect_test.cpp,TRAFDSN,runconnect_test.sh} $(TRAF_HOME)/samples
	cd $(LIBEXPDIR);ln -sf libicudata.so.44 libicudata.so;ln -sf libicuuc.so.44 libicuuc.so;ln -sf libtrafodbc_l64.so libtrafodbc64.so;ln -sf libtrafodbc_l64_drvr.so libtrafodbc_drvr64.so

# Linux ODBC driver clean targets	
.PHONY: cleanlnx_drvr
cleanlnx_drvr:
	cd $(TRAF_HOME)/../conn/unixodbc/odbc/odbcclient/unixcli;$(MAKE) -f makefile.lnx clean; 
	/bin/rm -f $(LIBEXPDIR)/libtrafodbc* $(LIBEXPDIR)/libicu*
	cd $(TRAF_HOME)/../conn/unixodbc/odbc/odbcclient/unixcli/package;rm -rf *.gz relfiles
	cd $(TRAF_HOME)/samples;rm -rf connect_test* runconnect_test.sh TRAFDSN
