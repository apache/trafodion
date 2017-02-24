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
INSTALL_SCRIPTSDIR := ${TRAF_HOME}/sql/scripts
CP :=/bin/cp -f
all:
	@$(MAVEN) package -DskipTests
	mkdir -p ${INSTALL_LIBDIR}
	${CP} target/lib_mgmt-${TRAFODION_VER}.jar ${INSTALL_LIBDIR}/lib_mgmt.jar
	${CP} src/main/resources/init_libmgmt.sh ${INSTALL_SCRIPTSDIR}/
	@chmod +x ${INSTALL_SCRIPTSDIR}/init_libmgmt.sh
clean:
	@rm -f ${INSTALL_LIBDIR}/lib_mgmt.jar
	@rm -f ${INSTALL_SCRIPTSDIR}/init_libmgmt.sh
	@$(MAVEN) clean
cleanall: clean
