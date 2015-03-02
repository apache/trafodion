# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# @@@ END COPYRIGHT @@@

# This Makefile is just a thin shell to Maven, which is used to do the real build

BLD_TRAFODION_REST_TARNAME           =rest-$(TRAFODION_VER).tar.gz

VFILE			    =trafodion-rest.jar.versions
GENVERS			    =./genvers

.NOTPARALLEL: all

all: build_all

build_all:	 
	$(MAKE) build_chk
	echo "$(MAVEN) package -DskipTests"
	echo "### For full Maven output, see file build_rest.log"
	set -o pipefail && $(MAVEN) package -DskipTests | tee build_rest.log | grep --line-buffered -E -e '^\[[^WId]' -e '^\[INFO\] B[Uu][Ii][Ll][Dd]' -e 'to compile'
	cp -pf target/$(BLD_TRAFODION_REST_TARNAME) $(MY_SQROOT)/export/lib
	$(RM) $(VFILE)

build_chk:
	$(GENVERS) > $(VFILE)
	@if [ $(GENVERS) -nt target/$(BLD_TRAFODION_REST_TARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_TRAFODION_REST_TARNAME); fi
	@if [ $(MY_SQROOT)/export/include/SCMBuildStr.h -nt target/$(BLD_TRAFODION_REST_TARNAME) ]; then echo "update manifest"; $(RM) -f target/$(BLD_TRAFODION_REST_TARNAME); fi

clean:
	$(RM) $(MY_SQROOT)/export/lib/$(BLD_TRAFODION_REST_TARNAME)
	-$(MAVEN) clean | grep ERROR
	$(RM) build_rest.log
	$(RM) $(VFILE)
