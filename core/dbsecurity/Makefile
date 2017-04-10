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
#
# File:         Makefile
# Description:  Makefile for building security library 
#

include $(TRAF_HOME)/macros.gmk #top level
include ./macros.gmk

RM	= /bin/rm
CP	= /bin/cp


.PHONY: all
all: verhdr auth cert dist


.PHONY: verhdr
verhdr:
	cd $(TRAF_HOME); $(MAKE) genverhdr;

.PHONY: auth
auth: verhdr
	cd auth; $(MAKE)

.PHONY: cert
cert: verhdr auth
	cd cert; $(MAKE)

.PHONY: dist
dist:
	$(CP) -p scripts/sqcertgen $(TRAF_HOME)/sql/scripts
	$(CP) -p scripts/update_auth $(TRAF_HOME)/sql/scripts


.PHONY: clean
clean: cleanauth cleancert
	$(RM) -f $(TRAF_HOME)/sql/scripts/sqcertgen
	$(RM) -f $(TRAF_HOME)/sql/scripts/update_auth

.PHONY: cleanall
cleanall: cleanallauth cleanallcert

.PHONY: cleanauth
cleanauth:
	cd auth; $(MAKE) clean

.PHONY: cleancert
cleancert:
	cd cert; $(MAKE) clean

.PHONY: cleanallauth
cleanallauth:
	cd auth; $(MAKE) cleanall

.PHONY: cleanallcert
cleanallcert:
	cd cert; $(MAKE) cleanall

