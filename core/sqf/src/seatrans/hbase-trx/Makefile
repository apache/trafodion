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

VFILE			    =hbase-trx.jar.versions
GENVERS			    =./genvers
UNCOMMENT_STRING	    =./uncomment_string
REINSTATE_ORIG   	    =./reinstate_orig
#When add one more hadoop version, please addthe names in below 4 lines in order
VERSION_NAMES		=0       1       2       3        4          5          6
JAR_NAMES		=cdh5_4  cdh5_5  cdh5_7  hdp2_3   apache1_0  apache1_1  apache1_2 
POM_NAMES		=cdh54   cdh55   cdh57   hdp      apache10   apache11   apache12 
UNCOMMENT_NAMES		=CDH5.4  CDH5.5  CDH5.7  HDP      APACHE1.0  APACHE1.1  APACHE1.2

.NOTPARALLEL: all

all: 
	@pom_nms=($(shell echo $(POM_NAMES))); \
	uncomment_nms=($(shell echo $(UNCOMMENT_NAMES))); \
	jar_nms=($(shell echo $(JAR_NAMES))); \
	version_nms=($(shell echo $(VERSION_NAMES))); \
	length=$${#version_nms[@]}; \
	let 'length=length-1'; \
	$(GENVERS) > $(VFILE); \
	for i in `echo $${version_nms[@]}` ; do \
		jar_nm=target/hbase-trx-$${jar_nms[$${i}]}-$(TRAFODION_VER).jar ; \
		if [ ! -f $${jar_nm} ]; then \
			if [ $(GENVERS) -nt $${jar_nm} ]; then echo "update manifest"; $(RM) -f $${jar_nm}; fi ; \
			if [ $(TRAF_HOME)/export/include/SCMBuildStr.h -nt $${jar_nm} ]; then echo "update manifest"; $(RM) -f $${jar_nm}; fi ; \
			echo $${uncomment_nms[$${i}]}; \
			$(REINSTATE_ORIG); \
			$(UNCOMMENT_STRING) $${uncomment_nms[$${i}]}; \
			echo "$${i}/$${length}: $(MAVEN) -f pom.xml.$${pom_nms[$${i}]} package -DskipTests"; \
			echo "### For full Maven output, see file build_trx.log"; \
			set -o pipefail && $(MAVEN) -f pom.xml.$${pom_nms[$${i}]} package install -DskipTests | tee -a build_trx.log; \
		fi ; \
		mkdir -p $(TRAF_HOME)/export/lib; \
		cp -pf $${jar_nm}  $(TRAF_HOME)/export/lib; \
	done ;  \
	$(RM) $(VFILE)

clean:
	$(RM) $(TRAF_HOME)/export/lib/hbase-trx-*-$(TRAFODION_VER).jar
	$(REINSTATE_ORIG)
	-@if [ -d target ]; then \
		$(MAVEN) -f pom.xml.apache10 clean | grep ERROR 2>/dev/null ; \
	fi
	$(RM) build_trx.log
	$(RM) $(VFILE)
