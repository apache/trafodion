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

include ../../macros.gmk #top level
java_files:=$(shell find src/main/java -name *.java*|grep -v Vproc.java)

all: build_all 

build_all: target/jdbcT4-${TRAFODION_VER}.jar 

target/jdbcT4-${TRAFODION_VER}.jar: $(java_files) LICENSE NOTICE
	echo "$(MAVEN) package -DskipTests"
	set -o pipefail && $(MAVEN) install -Dhttps.protocols=TLSv1,TLSv1.1,TLSv1.2 -DskipTests | tee build_jdbct4.log | grep --line-buffered -E -e '^\[[^WId]' -e '^\[INFO\] B[Uu][Ii][Ll][Dd]' -e 'to compile'
	cp target/jdbcT4-${TRAFODION_VER}.jar ${TRAF_HOME}/export/lib
	mkdir -p ../clients
	mv target/jdbcT4-${TRAFODION_VER}.zip ../clients
	#ln -sf ${TRAF_HOME}/export/lib/jdbcT4-${TRAFODION_VER}.jar ${TRAF_HOME}/export/lib/jdbcT4.jar
	`cd ${TRAF_HOME}/export/lib;ln -sf jdbcT4-${TRAFODION_VER}.jar jdbcT4.jar`

mvn_clean: 
	-$(MAVEN) clean | grep -e '\[INFO\] Building' -e '\[INFO\] BUILD SUCCESS' -e 'ERROR' 

clean: $(shell if [ -d target ]; then echo "mvn_clean"; fi) 
	$(RM) build_jdbct4.log ${TRAF_HOME}/export/lib/jdbcT4*.jar

LICENSE: ../../../licenses/LICENSE-clients
	cp -f $? $@

../../../licenses/LICENSE-clients:
	cd $(@D) && $(MAKE) $(@F)

NOTICE: ../../../NOTICE
	cp -f $? $@
