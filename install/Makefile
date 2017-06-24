
# @@@ START COPYRIGHT @@@
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
RELEASE_VER ?= ${TRAFODION_VER}-incubating
RELEASE_TYPE ?= $(shell echo $(TRAFODION_VER_PROD)| sed -e 's/ /-/g')
PYINSTALLER_TARNAME = $(shell echo ${RELEASE_TYPE}_pyinstaller-${RELEASE_VER}.tar.gz |tr '[A-Z]' '[a-z]')

all: pkg-pyinstaller pkg-ambari

pkg-ambari:
	cd ambari-installer && $(MAKE) all

create-dir:
	mkdir -p ../distribution

pkg-pyinstaller: create-dir python-installer/LICENSE python-installer/NOTICE python-installer/DISCLAIMER
	tar czf ${PYINSTALLER_TARNAME} python-installer
	mv ${PYINSTALLER_TARNAME} ../distribution

../licenses/LICENSE-pyinstall:
	cd $(@D) && $(MAKE) $(@F)

python-installer/LICENSE: ../licenses/LICENSE-pyinstall
	cp -f $? $@

python-installer/NOTICE: ../NOTICE
	cp -f $? $@

python-installer/DISCLAIMER: ../DISCLAIMER
	cp -f $? $@

version:
	@echo "$(RELEASE_VER)"

clean:
	rm -f ${INSTALLER_TARNAME}
	rm -f ${PYINSTALLER_TARNAME}
	cd ambari-installer && $(MAKE) clean

