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
SHELL := /bin/bash

VFILE			    =hbase-trx.jar.versions
GENVERS			    =./genvers
UNCOMMENT_STRING	    =./uncomment_string
REINSTATE_ORIG   	    =./reinstate_orig
COPY_PROTO   	            =./copy_proto
java_files:=$(shell find src/main/java -name *.java* >original.diff.log;find src/main/java -name "*.java.tmpl" | sed -e "s/\.tmpl$$//" >generated.diff.log;  grep -F -v -f generated.diff.log original.diff.log)

.NOTPARALLEL: all
all: target/hbase-trx-apache1_2-$(TRAFODION_VER).jar \
     target/hbase-trx-apache1_1-$(TRAFODION_VER).jar \
     target/hbase-trx-apache1_0-$(TRAFODION_VER).jar \
     target/hbase-trx-hdp263-$(TRAFODION_VER).jar \
     target/hbase-trx-hdp2_3-$(TRAFODION_VER).jar \
     target/hbase-trx-cdh5_7-$(TRAFODION_VER).jar \
     target/hbase-trx-cdh5_5-$(TRAFODION_VER).jar \
     target/hbase-trx-cdh5_4-$(TRAFODION_VER).jar 

define trx
	$(GENVERS) > $(VFILE)
	jar_nm=target/hbase-trx-$(1)-$(TRAFODION_VER).jar
	if [ $(GENVERS) -nt target/hbase-trx-$(1)-$(TRAFODION_VER).jar ]; then echo "update manifest"; $(RM) -f target/hbase-trx-$(1)-$(TRAFODION_VER).jar; fi
	if [ $(TRAF_HOME)/export/include/SCMBuildStr.h -nt target/hbase-trx-$(1)-$(TRAFODION_VER).jar ]; then echo "update manifest"; $(RM) -f target/hbase-trx-$(1)-$(TRAFODION_VER).jar; fi
	echo $(3)
	$(REINSTATE_ORIG)
	$(UNCOMMENT_STRING) $(3)
	$(COPY_PROTO) $(4)
	echo "$(MAVEN) -f pom.xml.$(2) install -DskipTests"
	echo "### For full Maven output, see file build_trx_$(1).log"
	set -o pipefail && $(MAVEN) -f pom.xml.$(2) install -DskipTests | tee -a build_trx_$(1).log| grep --line-buffered -E -e '^\[[^WId]' -e '^\[INFO\] B[Uu][Ii][Ll][Dd]' -e 'to compile' -e 'Compiling'
	mkdir -p $(TRAF_HOME)/export/lib
	cp -pf target/hbase-trx-$(1)-$(TRAFODION_VER).jar $(TRAF_HOME)/export/lib
	$(RM) $(VFILE)
endef

target/hbase-trx-apache1_2-$(TRAFODION_VER).jar: $(java_files) 
	$(call trx,apache1_2,apache12,APACHE1.2,nopb)

target/hbase-trx-apache1_1-$(TRAFODION_VER).jar: $(java_files) 
	$(call trx,apache1_1,apache11,APACHE1.1,nopb)

target/hbase-trx-apache1_0-$(TRAFODION_VER).jar: $(java_files) 
	$(call trx,apache1_0,apache10,APACHE1.0,nopb)

target/hbase-trx-hdp2_3-$(TRAFODION_VER).jar: $(java_files) 
	$(call trx,hdp2_3,hdp,HDP2.3,nopb)

target/hbase-trx-hdp263-$(TRAFODION_VER).jar: $(java_files) 
	$(call trx,hdp263,hdp263,HDP2.3,pb)

target/hbase-trx-cdh5_7-$(TRAFODION_VER).jar: $(java_files) 
	$(call trx,cdh5_7,cdh57,CDH5.7,nopb)

target/hbase-trx-cdh5_5-$(TRAFODION_VER).jar: $(java_files) 
	$(call trx,cdh5_5,cdh55,CDH5.5,nopb)

target/hbase-trx-cdh5_4-$(TRAFODION_VER).jar: $(java_files) 
	$(call trx,cdh5_4,cdh54,CDH5.4,nopb)

depcheck:
	$(MAVEN) -f pom.xml.cdh54 org.owasp:dependency-check-maven:check
	$(MAVEN) -f pom.xml.cdh55 org.owasp:dependency-check-maven:check
	$(MAVEN) -f pom.xml.cdh57 org.owasp:dependency-check-maven:check
	#$(MAVEN) -f pom.xml.mapr org.owasp:dependency-check-maven:check
	$(MAVEN) -f pom.xml.hdp org.owasp:dependency-check-maven:check
	$(MAVEN) -f pom.xml.hdp263 org.owasp:dependency-check-maven:check
	$(MAVEN) -f pom.xml.apache10 org.owasp:dependency-check-maven:check
	$(MAVEN) -f pom.xml.apache11 org.owasp:dependency-check-maven:check
	$(MAVEN) -f pom.xml.apache12 org.owasp:dependency-check-maven:check

mvn_clean:
	-$(MAVEN) -f pom.xml.apache10 clean| grep -e '\[INFO\] Building' -e '\[INFO\] BUILD SUCCESS' -e 'ERROR' 

clean: $(shell if [ -d target ]; then echo "mvn_clean"; fi) 
	$(RM) $(TRAF_HOME)/export/lib/hbase-trx-*-$(TRAFODION_VER).jar
	$(REINSTATE_ORIG)
	$(RM) *.log
	$(RM) $(VFILE)
