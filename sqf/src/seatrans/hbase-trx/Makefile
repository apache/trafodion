# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
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

# This Makefile is just a thin shell to Maven, which is used to do the real build

BLD_HBASE_APACHE_TRX_JARNAME     =hbase-trx-hbase_98_4-$(TRAFODION_VER).jar
BLD_HBASE_CDH_TRX_JARNAME        =hbase-trx-cdh5_3-$(TRAFODION_VER).jar
BLD_HBASE_MAPR_TRX_JARNAME       =hbase-trx-mapr4_0-$(TRAFODION_VER).jar
BLD_HBASE_HDP_TRX_JARNAME        =hbase-trx-hdp2_2-$(TRAFODION_VER).jar

VFILE			    =hbase-trx.jar.versions
GENVERS			    =./genvers

.NOTPARALLEL: all

all: build_all

jdk_1_7_cdh:
	$(MAKE) build_chk_cdh
	set -o pipefail && $(MAVEN) -f pom.xml.cdh package -DskipTests | tee -a build_trx.log
	cp -pf target/$(BLD_HBASE_CDH_TRX_JARNAME) $(MY_SQROOT)/export/lib
	$(RM) $(VFILE)

jdk_1_7_mapr:
	$(MAKE) build_chk_mapr
	set -o pipefail && $(MAVEN) -f pom.xml.mapr package -DskipTests | tee -a build_trx.log
	cp -pf target/$(BLD_HBASE_MAPR_TRX_JARNAME) $(MY_SQROOT)/export/lib
	$(RM) $(VFILE)

jdk_1_7_hdp:
	$(MAKE) build_chk_hdp
	set -o pipefail && $(MAVEN) -f pom.xml.hdp package -DskipTests | tee -a build_trx.log
	cp -pf target/$(BLD_HBASE_HDP_TRX_JARNAME) $(MY_SQROOT)/export/lib
	$(RM) $(VFILE)

jdk_1_7_apache:
	$(MAKE) build_chk_apache
	echo "$(MAVEN) package -DskipTests"
	echo "### For full Maven output, see file build_trx.log"
	set -o pipefail && $(MAVEN) -f pom.xml.apache package -DskipTests | tee build_trx.log | grep --line-buffered -E -e '^\[[^WId]' -e '^\[INFO\] B[Uu][Ii][Ll][Dd]' -e 'to compile'
	cp -pf target/$(BLD_HBASE_APACHE_TRX_JARNAME) $(MY_SQROOT)/export/lib
	$(RM) $(VFILE)

build_all: jdk_1_7_apache jdk_1_7_cdh jdk_1_7_hdp jdk_1_7_mapr
#build_all: jdk_1_7_apache jdk_1_7_cdh

build_chk_apache:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_HBASE_APACHE_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_APACHE_TRX_JARNAME); fi
	@if [ $(MY_SQROOT)/export/include/SCMBuildStr.h -nt target/$(BLD_HBASE_APACHE_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_APACHE_TRX_JARNAME); fi

build_chk_cdh:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_HBASE_CDH_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_CDH_TRX_JARNAME); fi
	@if [ $(MY_SQROOT)/export/include/SCMBuildStr.h -nt target/$(BLD_HBASE_CDH_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_CDH_TRX_JARNAME); fi

build_chk_mapr:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_HBASE_MAPR_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_MAPR_TRX_JARNAME); fi
	@if [ $(MY_SQROOT)/export/include/SCMBuildStr.h -nt target/$(BLD_HBASE_MAPR_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_MAPR_TRX_JARNAME); fi

build_chk_hdp:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_HBASE_HDP_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_HDP_TRX_JARNAME); fi
	@if [ $(MY_SQROOT)/export/include/SCMBuildStr.h -nt target/$(BLD_HBASE_HDP_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_HDP_TRX_JARNAME); fi

clean:
	$(RM) $(MY_SQROOT)/export/lib/$(BLD_HBASE_APACHE_TRX_JARNAME)
	$(RM) $(MY_SQROOT)/export/lib/$(BLD_HBASE_CDH_TRX_JARNAME)
	$(RM) $(MY_SQROOT)/export/lib/$(BLD_HBASE_MAPR_TRX_JARNAME)
	$(RM) $(MY_SQROOT)/export/lib/$(BLD_HBASE_HDP_TRX_JARNAME)
	-$(MAVEN) -f pom.xml.apache clean | grep ERROR
	-$(MAVEN) -f pom.xml.cdh clean | grep ERROR
	-$(MAVEN) -f pom.xml.mapr clean | grep ERROR
	-$(MAVEN) -f pom.xml.hdp clean | grep ERROR
	$(RM) build_trx.log
	$(RM) $(VFILE)
