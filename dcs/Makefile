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

include ../core/macros.gmk #top level
BLD_TRAFODION_DCS_TARNAME           =dcs-$(TRAFODION_VER).tar.gz

VFILE			    =trafodion-dcs.jar.versions
GENVERS			    =./genvers

.NOTPARALLEL: all

all: build_all

build_all: build_chk
	echo "$(MAVEN) site package -DskipTests"
	echo "### For full Maven output, see file build_dcs.log"
	set -o pipefail && $(MAVEN) site package -DskipTests | tee build_dcs.log | grep --line-buffered -E -e '^\[[^WId]' -e '^\[INFO\] B[Uu][Ii][Ll][Dd]' -e 'to compile'
	$(RM) $(VFILE)

build_chk:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_TRAFODION_DCS_TARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_TRAFODION_DCS_TARNAME); fi
	@if [ $(TRAF_HOME)/export/include/SCMBuildStr.h -nt target/$(BLD_TRAFODION_DCS_TARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_TRAFODION_DCS_TARNAME); fi

clean:
	-$(MAVEN) clean | tee build_dcs.log | grep --line-buffered -E -e '^\[[^WId]' -e '^\[INFO\] B[Uu][Ii][Ll][Dd]' -e 'to compile' 
	$(RM) build_dcs.log
	$(RM) $(VFILE)
	$(RM) ../${DISTRIBUTION_DIR}/$(BLD_TRAFODION_DCS_TARNAME)
