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

all:Trafci_deploy
Trafci_deploy: ../clients/trafci.zip

java_files:=$(shell find src/main/java -name *.java*|grep -v Vproc.java)

target/trafciInstaller.jar target/trafciInstaller.tar: $(java_files)
	mvn install -DskipTests
	./getBuildInfo
	export buildId=`head -n 1 buildId`
	mvn -f installer_pom.xml -DbldId=${buildId} clean package -DskipTests

../clients/trafci.zip: target/trafciInstaller.jar
	zip -r ../clients/trafci.zip -j target/trafciInstaller.jar utils/README
	java -jar target/trafciInstaller.jar -silent -installDir ${TRAF_HOME}

mvn_clean:
	-@mvn -f pom.xml clean| grep -e '\[INFO\] Building' -e '\[INFO\] BUILD SUCCESS' -e 'ERROR'

clean: $(shell if [ -d target ]; then echo "mvn_clean"; fi) 
