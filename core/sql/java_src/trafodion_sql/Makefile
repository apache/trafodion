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
all: mavenbuild

# Java files get built through Maven
mavenbuild:
	# create a jar manifest file with the correct version information
	# run maven
	$(MAVEN) -Pproduction install -DskipTests 2>&1 | sed -e "s/$$/  ##(trafodion-sql)/";exit $${PIPESTATUS[0]} 
	cp -pf target/trafodion-sql-$(TRAFODION_VER).jar $(TRAF_HOME)/export/lib
	@cd $(TRAF_HOME)/export/lib; ln -sf trafodion-sql-$(TRAFODION_VER).jar trafodion-sql-apache-$(TRAFODION_VER).jar
	@cd $(TRAF_HOME)/export/lib; ln -sf trafodion-sql-$(TRAFODION_VER).jar trafodion-sql-cdh-$(TRAFODION_VER).jar 
	@cd $(TRAF_HOME)/export/lib; ln -sf trafodion-sql-$(TRAFODION_VER).jar trafodion-sql-hdp-$(TRAFODION_VER).jar 

clean:
	@rm -f ${INSTALL_LIBDIR}/trafodion-sql-*.jar
	@$(MAVEN) -Pproduction clean
cleanall: clean
