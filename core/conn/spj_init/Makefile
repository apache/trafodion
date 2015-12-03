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

LIBDIR := lib
JAVAC := $(JAVA_HOME)/bin/javac
INSTALL_LIBDIR := ${MY_SQROOT}/export/lib
INSTALL_SCRIPTSDIR := ${MY_SQROOT}/sql/scripts
CP :=/bin/cp -f
INSTALL_INCLUDEDIR := ${MY_SQROOT}/export/include
all:
	@$(MAVEN) package
	${CP} target/spj_init-*.jar ${INSTALL_LIBDIR}/spj_init.jar
	${CP} src/main/resources/init_spj.sql ${INSTALL_SCRIPTSDIR}/
	@chmod +x ${INSTALL_SCRIPTSDIR}/init_spj.sql
clean:
	@rm ${INSTALL_LIBDIR}/spj_init.jar
	@rm -f ${INSTALL_SCRIPTSDIR}/init_spj.sql
	@$(MAVEN) clean
cleanall: clean
