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
all: mavenbuild mavenbuild_apache mavenbuild_hdp

# Java files get built through Maven
mavenbuild:
	# create a jar manifest file with the correct version information
	mkdir -p ../src/main/resources
	#$(TRAF_HOME)/export/include/SCMBuildJava.sh 1.0.1 >../src/main/resources/trafodion-sql.jar.mf
	# run maven
	$(MAVEN) package -DskipTests 2>&1 | sed -e "s/$$/  ##(trafodion-sql-cdh)/";exit $${PIPESTATUS[0]} 
	cp -pf target/trafodion-sql-cdh*.jar $(TRAF_HOME)/export/lib

# Java files get built through Maven
mavenbuild_apache:
	$(MAVEN) -f pom.xml.apache package -DskipTests 2>&1 | sed -e "s/$$/  ##(trafodion-sql-apache)/";exit $${PIPESTATUS[0]}  
	cp -pf target/trafodion-sql-apache*.jar $(TRAF_HOME)/export/lib

# Java files get built through Maven
mavenbuild_hdp:
	$(MAVEN) -f pom.xml.hdp package -DskipTests 2>&1 | sed -e "s/$$/  ##(trafodion-sql-hdp)/";exit $${PIPESTATUS[0]}  
	cp -pf target/trafodion-sql-hdp*.jar $(TRAF_HOME)/export/lib
clean:
	@rm -f ${INSTALL_LIBDIR}/trafodion-sql-*.jar
	@$(MAVEN) clean
	@$(MAVEN) -f pom.xml.apache clean
	@$(MAVEN) -f pom.xml.hdp clean
cleanall: clean
