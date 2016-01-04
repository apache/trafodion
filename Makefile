## Top-level Makefile for building Trafodion components


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

.PHONY: all
SRCDIR = $(shell echo $(TRAFODION_VER_PROD) | sed -e 's/ /-/g' | tr 'A-Z' 'a-z')

all:
	@echo "Building all Trafodion components"
	cd core && $(MAKE) all 

package: 
	@echo "Packaging Trafodion components"
	cd core && $(MAKE) package 

package-all: 
	@echo "Packaging all Trafodion components"
	cd core && $(MAKE) package-all 

package-src:
	@echo "Packaging source for $(TRAFODION_VER_PROD) $(TRAFODION_VER)"
	mkdir -p distribution
	git archive --format tar --prefix $(SRCDIR)-${TRAFODION_VER}-incubating/ HEAD | gzip > distribution/$(SRCDIR)-${TRAFODION_VER}-incubating-src.tar.gz

eclipse: 
	@echo "Making eclipse projects for Trafodion components"
	cd core && $(MAKE) eclipse 

clean:
	@echo "Removing Trafodion objects"
	cd core && $(MAKE) clean 

cleanall:
	@echo "Removing all Trafodion objects"
	cd core && $(MAKE) cleanall 

trafinstall:
	@echo "Installing Trafodion components"
	cd core && $(MAKE) trafinstall
