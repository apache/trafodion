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

INSTALL_LIBDIR := ${TRAF_HOME}/export/lib
CP :=/bin/cp -f
java_files:=$(shell find src/main/java -name *.java*)
all: target/lib_mgmt-${TRAFODION_VER}.jar
target/lib_mgmt-${TRAFODION_VER}.jar: $(java_files)
	$(MAVEN) package -DskipTests | tee build_mgmt.log | grep --line-buffered -E -e '^\[[^WId]' -e '^\[INFO\] B[Uu][Ii][Ll][Dd]' -e 'to compile'
	mkdir -p ${INSTALL_LIBDIR}
	${CP} target/lib_mgmt-${TRAFODION_VER}.jar ${INSTALL_LIBDIR}/lib_mgmt.jar
mvn_clean:
	$(MAVEN) clean | grep -e '\[INFO\] Building' -e '\[INFO\] BUILD SUCCESS' -e 'ERROR' 
clean: $(shell if [ -d target ]; then echo "mvn_clean"; fi) 
	@rm -f build_mgmt.log
	@rm -f ${INSTALL_LIBDIR}/lib_mgmt.jar
cleanall: clean
