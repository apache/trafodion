# Top level HPDCS & WMS make file

# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
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


.PHONY: all
all: ndcs bldlnx_drvr

# NDCS targets
.PHONY:	ndcs
ndcs: wms nsksrvrcore nsksrvr

.PHONY:	nsksrvrcore
nsksrvrcore:  
	cd nsksrvrcore; $(MAKE)

.PHONY:	nsksrvr
nsksrvr: nsksrvrcore 
	cd nsksrvr; $(MAKE)

# NDCS clean targets
.PHONY:	cleanndcs
cleanndcs: cleannsksrvrcore cleannsksrvr cleanwmseventsdll

.PHONY: cleannsksrvrcore
cleannsksrvrcore:
	cd nsksrvrcore; $(MAKE) clean

.PHONY: cleannsksrvr
cleannsksrvr:
	cd nsksrvr; $(MAKE) clean

# NDCS cleanall targets
.PHONY: cleanallndcs
cleanallndcs:  cleanallnsksrvrcore cleanallnsksrvr cleanallwmseventsdll

.PHONY: cleanallnsksrvrcore
cleanallnsksrvrcore:
	cd nsksrvrcore; $(MAKE) cleanall

.PHONY: cleanallnsksrvr
cleanallnsksrvr:
	cd nsksrvr; $(MAKE) cleanall

# WMS targets
.PHONY: wms
wms: wmseventsdll 
 
.PHONY: wmseventsdll
wmseventsdll:
	cd WmsEventsDll; $(MAKE)
	
.PHONY: clean
clean: cleanndcs cleanwms cleanlnx_drvr

.PHONY: cleanall
cleanall: cleanallndcs cleanallwms cleanlnx_drvr
	
# WMS clean targets
.PHONY: cleanwms
cleanwms: cleanwmseventsdll 
	
.PHONY: cleanwmseventsdll
cleanwmseventsdll:
	cd WmsEventsDll; $(MAKE) clean

# WMS cleanall targets
.PHONY: cleanallwms
cleanallwms:  cleanallwmseventsdll 
	
.PHONY: cleanallwmseventsdll
cleanallwmseventsdll:
	cd WmsEventsDll; $(MAKE) cleanall
	
# Linux ODBC Driver targets
.PHONY: bldlnx_drvr
include $(MY_SQROOT)/macros.gmk
bldlnx_drvr:
	cd $(MY_SQROOT)/../conn/unixodbc/odbc/odbcclient/unixcli;$(MAKE) -f makefile.lnx clean; $(MAKE) -f makefile.lnx linux64_$(SQ_BUILD_TYPE)
	cd $(MY_SQROOT)/../conn/unixodbc/odbc/odbcclient/unixcli/package;rm -rf *.gz; ./mklnxpkg.sh -f; cp -p trafodbc_Linux64_pkg.tar.gz $(MY_SQROOT)/../conn/clients/TRAF_ODBC_Linux_Driver_64.tar.gz

# Linux ODBC driver clean targets	
.PHONY: cleanlnx_drvr
cleanlnx_drvr:
	cd $(MY_SQROOT)/../conn/unixodbc/odbc/odbcclient/unixcli;$(MAKE) -f makefile.lnx clean; 
	/bin/rm -f $(LIBEXPDIR)/libtrafodbc*

