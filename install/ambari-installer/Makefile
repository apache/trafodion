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


# Makefile to build trafodion management pack for Ambari

RELEASE ?= 1
REPO_URL ?= http://no.such.server.org/repo

SPECFILE = traf_ambari.spec

RPMROOT=$(PWD)/RPMROOT
RPMDIR=$(RPMROOT)/RPMS
SRPMDIR=$(RPMROOT)/SRPMS
SOURCEDIR=$(RPMROOT)/SOURCES
BUILDDIR=$(RPMROOT)/BUILD
BUILDROOTDIR=$(RPMROOT)/BUILDROOT

all: rpmbuild

# need repoinfo file per release
# traf_ambari needs to support multiple releases so ambari can select 
# correct trafodion version for a given HDP stack
REPO_VER= 2.1

$(SOURCEDIR)/ambari_rpm.tar.gz: mpack-install/LICENSE repofiles
	rm -rf $(RPMROOT)
	mkdir -p $(SOURCEDIR)
	tar czf $@ traf-mpack mpack-install

# To do: needs enhancement when supporting RH6 & RH7 os
repofiles:
	for v in $(REPO_VER); do \
	   rdir=traf-mpack/custom-services/TRAFODION/$${v}/repos; \
	   mkdir -p $${rdir} ; \
	   sed -e "s#REPLACE_WITH_RH6_REPO_URL#$(REPO_URL)/RH6/$${v}#" repo.template > $${rdir}/repoinfo.xml ; \
	   echo $${rdir}/repoinfo.xml ; \
	done

mpack-install/LICENSE: ../../licenses/LICENSE-install
	cp -f $? $@

../../licenses/LICENSE-install:
	cd $(@D) && $(MAKE) $(@F)

rpmbuild: $(SOURCEDIR)/ambari_rpm.tar.gz
	mkdir -p $(RPMDIR)
	mkdir -p $(BUILDDIR)
	mkdir -p $(BUILDROOTDIR)
	mkdir -p $(SRPMDIR)
	rpmbuild -vv -bb \
	                --define "version $(TRAFODION_VER)" \
	                --define "release $(RELEASE)" \
	                --define "_builddir $(BUILDDIR)" \
	                --define "_buildrootdir $(BUILDROOTDIR)" \
	                --define "_sourcedir $(SOURCEDIR)" \
	                --define "_rpmdir $(RPMDIR)" \
	                --define "_topdir $(RPMROOT)" \
	                $(SPECFILE)
	mkdir -p ../../distribution
	mv -f $(RPMROOT)/RPMS/noarch/traf_ambari*.rpm ../../distribution/

clean:
	rm -rf $(RPMROOT)
	rm -rf mpack-install/LICENSE
	rm -rf traf-mpack/custom-services/TRAFODION/*/repos
