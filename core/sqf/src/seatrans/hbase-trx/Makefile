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

# This Makefile is just a thin shell to Maven, which is used to do the real build

BLD_HBASE_APACHE10_TRX_JARNAME     =hbase-trx-apache1_0-$(TRAFODION_VER).jar
BLD_HBASE_APACHE11_TRX_JARNAME     =hbase-trx-apache1_1-$(TRAFODION_VER).jar
BLD_HBASE_APACHE12_TRX_JARNAME     =hbase-trx-apache1_2-$(TRAFODION_VER).jar
BLD_HBASE_CDH54_TRX_JARNAME        =hbase-trx-cdh5_4-$(TRAFODION_VER).jar
BLD_HBASE_CDH55_TRX_JARNAME        =hbase-trx-cdh5_5-$-$(TRAFODION_VER).jar
BLD_HBASE_CDH57_TRX_JARNAME        =hbase-trx-cdh5_7-$-$(TRAFODION_VER).jar
BLD_HBASE_MAPR_TRX_JARNAME         =hbase-trx-mapr4_0-$(TRAFODION_VER).jar
BLD_HBASE_HDP_TRX_JARNAME          =hbase-trx-hdp2_3-$(TRAFODION_VER).jar
BLD_HBASE_SSCC_TRX_JARNAME         =hbase-trx-sscc-$(TRAFODION_VER).jar

VFILE			    =hbase-trx.jar.versions
GENVERS			    =./genvers
UNCOMMENT_STRING	    =./uncomment_string
REINSTATE_ORIG   	    =./reinstate_orig

.NOTPARALLEL: all

all: build_all

jdk_1_7_cdh54: build_chk_cdh54
	$(REINSTATE_ORIG)
	$(UNCOMMENT_STRING) CDH5.4
	echo "$(MAVEN) package -DskipTests"
	echo "### For full Maven output, see file build_trx.log"
	set -o pipefail && $(MAVEN) -f pom.xml.cdh54 package install -DskipTests | tee -a build_trx.log
	mkdir -p $(TRAF_HOME)/export/lib
	cp -pf target/$(BLD_HBASE_CDH54_TRX_JARNAME) $(TRAF_HOME)/export/lib
	$(RM) $(VFILE)

jdk_1_7_cdh55: build_chk_cdh55
	$(REINSTATE_ORIG)
	$(UNCOMMENT_STRING) CDH5.5
	set -o pipefail && $(MAVEN) -f pom.xml.cdh55 package install -DskipTests | tee -a build_trx.log
	mkdir -p $(TRAF_HOME)/export/lib
	cp -pf target/$(BLD_HBASE_CDH55_TRX_JARNAME) $(TRAF_HOME)/export/lib
	$(RM) $(VFILE)

jdk_1_7_cdh57: build_chk_cdh57
	$(REINSTATE_ORIG)
	$(UNCOMMENT_STRING) CDH5.7
	set -o pipefail && $(MAVEN) -f pom.xml.cdh57 package install -DskipTests | tee -a build_trx.log
	mkdir -p $(TRAF_HOME)/export/lib
	cp -pf target/$(BLD_HBASE_CDH57_TRX_JARNAME) $(TRAF_HOME)/export/lib
	$(RM) $(VFILE)

jdk_1_7_mapr: build_chk_mapr
	set -o pipefail && $(MAVEN) -f pom.xml.mapr package -DskipTests | tee -a build_trx.log
	mkdir -p $(TRAF_HOME)/export/lib
	cp -pf target/$(BLD_HBASE_MAPR_TRX_JARNAME) $(TRAF_HOME)/export/lib
	$(RM) $(VFILE)

jdk_1_7_hdp: build_chk_hdp
	$(REINSTATE_ORIG)
	$(UNCOMMENT_STRING) HDP2.3 
	set -o pipefail && $(MAVEN) -f pom.xml.hdp package -DskipTests | tee -a build_trx.log
	mkdir -p $(TRAF_HOME)/export/lib
	cp -pf target/$(BLD_HBASE_HDP_TRX_JARNAME) $(TRAF_HOME)/export/lib
	$(RM) $(VFILE)

jdk_1_7_apache10: build_chk_apache10
	$(REINSTATE_ORIG)
	$(UNCOMMENT_STRING) APACHE1.0
	set -o pipefail && $(MAVEN) -f pom.xml.apache10 package -DskipTests | tee build_trx.log
	mkdir -p $(TRAF_HOME)/export/lib
	cp -pf target/$(BLD_HBASE_APACHE10_TRX_JARNAME) $(TRAF_HOME)/export/lib
	$(RM) $(VFILE)

jdk_1_7_apache11: build_chk_apache11
	$(REINSTATE_ORIG)
	$(UNCOMMENT_STRING) APACHE1.1
	set -o pipefail && $(MAVEN) -f pom.xml.apache11 package -DskipTests | tee build_trx.log
	mkdir -p $(TRAF_HOME)/export/lib
	cp -pf target/$(BLD_HBASE_APACHE11_TRX_JARNAME) $(TRAF_HOME)/export/lib
	$(RM) $(VFILE)

jdk_1_7_apache12: build_chk_apache12
	$(REINSTATE_ORIG)
	$(UNCOMMENT_STRING) APACHE1.2
	set -o pipefail && $(MAVEN) -f pom.xml.apache12 package -DskipTests | tee build_trx.log
	mkdir -p $(TRAF_HOME)/export/lib
	cp -pf target/$(BLD_HBASE_APACHE12_TRX_JARNAME) $(TRAF_HOME)/export/lib
	$(RM) $(VFILE)

build_all: jdk_1_7_cdh54 jdk_1_7_cdh55 jdk_1_7_cdh57 jdk_1_7_hdp jdk_1_7_apache10 jdk_1_7_apache11 jdk_1_7_apache12

build_chk_apache10:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_HBASE_APACHE10_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_APACHE10_TRX_JARNAME); fi
	@if [ $(TRAF_HOME)/export/include/SCMBuildStr.h -nt target/$(BLD_HBASE_APACHE10_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_APACHE10_TRX_JARNAME); fi

build_chk_apache11:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_HBASE_APACHE11_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_APACHE11_TRX_JARNAME); fi
	@if [ $(TRAF_HOME)/export/include/SCMBuildStr.h -nt target/$(BLD_HBASE_APACHE11_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_APACHE11_TRX_JARNAME); fi

build_chk_apache12:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_HBASE_APACHE12_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_APACHE12_TRX_JARNAME); fi
	@if [ $(TRAF_HOME)/export/include/SCMBuildStr.h -nt target/$(BLD_HBASE_APACHE12_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_APACHE12_TRX_JARNAME); fi

build_chk_cdh54:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_HBASE_CDH54_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_CDH54_TRX_JARNAME); fi
	@if [ $(TRAF_HOME)/export/include/SCMBuildStr.h -nt target/$(BLD_HBASE_CDH54_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_CDH54_TRX_JARNAME); fi

build_chk_cdh55:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_HBASE_CDH55_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_CDH55_TRX_JARNAME); fi
	@if [ $(TRAF_HOME)/export/include/SCMBuildStr.h -nt target/$(BLD_HBASE_CDH55_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_CDH55_TRX_JARNAME); fi

build_chk_cdh57:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_HBASE_CDH57_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_CDH57_TRX_JARNAME); fi
	@if [ $(TRAF_HOME)/export/include/SCMBuildStr.h -nt target/$(BLD_HBASE_CDH57_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_CDH57_TRX_JARNAME); fi

build_chk_mapr:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_HBASE_MAPR_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_MAPR_TRX_JARNAME); fi
	@if [ $(TRAF_HOME)/export/include/SCMBuildStr.h -nt target/$(BLD_HBASE_MAPR_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_MAPR_TRX_JARNAME); fi

build_chk_hdp:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_HBASE_HDP_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_HDP_TRX_JARNAME); fi
	@if [ $(TRAF_HOME)/export/include/SCMBuildStr.h -nt target/$(BLD_HBASE_HDP_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_HDP_TRX_JARNAME); fi

clean:
	$(RM) $(TRAF_HOME)/export/lib/$(BLD_HBASE_APACHE10_TRX_JARNAME)
	$(RM) $(TRAF_HOME)/export/lib/$(BLD_HBASE_APACHE11_TRX_JARNAME)
	$(RM) $(TRAF_HOME)/export/lib/$(BLD_HBASE_APACHE12_TRX_JARNAME)
	$(RM) $(TRAF_HOME)/export/lib/$(BLD_HBASE_CDH54_TRX_JARNAME)
	$(RM) $(TRAF_HOME)/export/lib/$(BLD_HBASE_CDH55_TRX_JARNAME)
	$(RM) $(TRAF_HOME)/export/lib/$(BLD_HBASE_CDH57_TRX_JARNAME)
	$(RM) $(TRAF_HOME)/export/lib/$(BLD_HBASE_MAPR_TRX_JARNAME)
	$(RM) $(TRAF_HOME)/export/lib/$(BLD_HBASE_HDP_TRX_JARNAME)
	$(REINSTATE_ORIG)
	-$(MAVEN) -f pom.xml.apache10 clean | grep ERROR
	-$(MAVEN) -f pom.xml.apache11 clean | grep ERROR
	-$(MAVEN) -f pom.xml.apache12 clean | grep ERROR
	-$(MAVEN) -f pom.xml.cdh54 clean | grep ERROR
	-$(MAVEN) -f pom.xml.cdh55 clean | grep ERROR
	-$(MAVEN) -f pom.xml.cdh57 clean | grep ERROR
	-$(MAVEN) -f pom.xml.mapr clean | grep ERROR
	-$(MAVEN) -f pom.xml.hdp clean | grep ERROR
	$(RM) build_trx.log
	$(RM) $(VFILE)
