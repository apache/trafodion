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

BLD_HBASE_TRX_JARNAME           =hbase-trx-$(TRAFODION_VER).jar
BLD_HBASE_HDP2_1_TRX_JARNAME    =hbase-trx-hdp2_1-$(TRAFODION_VER).jar

# We also build the same code again with Java 6, to be loaded into
# HBase distros that are compiled with Java6. Note that this jar, if
# used at all, is only used by the HBase region server, Trafodion
# objects are built with Java 7
JAVA6_VERSION               =1.6
JAVA6_SUFFIX                =-java6
BLD_HBASE_TRX_JARNAME_1_6   =hbase-trx-$(TRAFODION_VER)$(JAVA6_SUFFIX).jar

VFILE			    =hbase-trx.jar.versions
GENVERS			    =./genvers

.NOTPARALLEL: all

all: build_all

jdk_1_7_hdp2_1:
	$(MAKE) build_chk_1_7_hdp2_1
	set -o pipefail && $(MAVEN) -f pom.xml.hdp2_1 package -DskipTests | tee -a build_trx.log
	cp -pf target/$(BLD_HBASE_HDP2_1_TRX_JARNAME) $(MY_SQROOT)/export/lib
	$(RM) $(VFILE)

jdk_1_7:
	$(MAKE) build_chk_1_7
	echo "$(MAVEN) package -DskipTests"
	echo "### For full Maven output, see file build_trx.log"
	set -o pipefail && $(MAVEN) package -DskipTests | tee build_trx.log | grep --line-buffered -E -e '^\[[^WId]' -e '^\[INFO\] B[Uu][Ii][Ll][Dd]' -e 'to compile'
	cp -pf target/$(BLD_HBASE_TRX_JARNAME) $(MY_SQROOT)/export/lib
	$(RM) $(VFILE)

jdk_1_6:
	$(MAKE) build_chk_1_6
	echo "$(MAVEN) package -DskipTests"
	echo "### For full Maven output, see file build_trx_1_6.log"
	set -o pipefail && JAVA_HOME=$(JAVAJDK16) $(MAVEN) package -Djava.version=$(JAVA6_VERSION) -Dtrx-suffix=$(JAVA6_SUFFIX) -DskipTests | tee build_trx_1_6.log | grep --line-buffered -E -e '^\[[^WId]' -e '^\[INFO\] B[Uu][Ii][Ll][Dd]' -e 'to compile'
	cp -pf target/$(BLD_HBASE_TRX_JARNAME_1_6) $(MY_SQROOT)/export/lib
	$(RM) $(VFILE)

build_all: jdk_1_6 jdk_1_7 jdk_1_7_hdp2_1

build_chk_1_6:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_HBASE_TRX_JARNAME_1_6) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_TRX_JARNAME_1_6); fi
	@if [ $(MY_SQROOT)/export/include/SCMBuildStr.h -nt target/$(BLD_HBASE_TRX_JARNAME_1_6) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_TRX_JARNAME_1_6); fi

build_chk_1_7:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_HBASE_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_TRX_JARNAME); fi
	@if [ $(MY_SQROOT)/export/include/SCMBuildStr.h -nt target/$(BLD_HBASE_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_TRX_JARNAME); fi

build_chk_1_7_hdp2_1:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_HBASE_HDP2_1_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_HDP2_1_TRX_JARNAME); fi
	@if [ $(MY_SQROOT)/export/include/SCMBuildStr.h -nt target/$(BLD_HBASE_HDP2_1_TRX_JARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_HBASE_HDP2_1_TRX_JARNAME); fi

clean:
	$(RM) $(MY_SQROOT)/export/lib/$(BLD_HBASE_TRX_JARNAME)
	$(RM) $(MY_SQROOT)/export/lib/$(BLD_HBASE_TRX_JARNAME_1_6)
	$(RM) $(MY_SQROOT)/export/lib/$(BLD_HBASE_HDP2_1_TRX_JARNAME)
	-$(MAVEN) clean | grep ERROR
	$(RM) build_trx.log
	$(RM) $(VFILE)
