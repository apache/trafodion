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

BLD_HBASE_TRX_JARNAME       =hbase-trx-$(TRAFODION_VER).jar

# We also build the same code again with Java 6, to be loaded into
# HBase distros that are compiled with Java6. Note that this jar, if
# used at all, is only used by the HBase region server, Trafodion
# objects are built with Java 7
JAVA6_VERSION               =1.6
JAVA6_SUFFIX                =-java6
BLD_HBASE_TRX_JARNAME_1_6   =hbase-trx-$(TRAFODION_VER)$(JAVA6_SUFFIX).jar

.NOTPARALLEL: all

all: build_all

jdk_1_7:
	mvn package -DskipTests | tee -a build_trx.log
	cp -pf target/$(BLD_HBASE_TRX_JARNAME) $(MY_SQROOT)/export/lib

jdk_1_6:
	JAVA_HOME=$(JAVAJDK16) mvn package -Djava.version=$(JAVA6_VERSION) -Dtrx-suffix=$(JAVA6_SUFFIX) -DskipTests | tee -a build_trx.log
	cp -pf target/$(BLD_HBASE_TRX_JARNAME_1_6) $(MY_SQROOT)/export/lib

build_all: jdk_1_6 jdk_1_7

clean:
	$(RM) $(MY_SQROOT)/export/lib/$(BLD_HBASE_TRX_JARNAME)
	$(RM) $(MY_SQROOT)/export/lib/$(BLD_HBASE_TRX_JARNAME_1_6)
	mvn clean
	$(RM) build_trx.log

