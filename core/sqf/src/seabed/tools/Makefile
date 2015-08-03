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

JAVAPROGS	= $(EXPROOT)/lib/sqmanvers.jar

include ../../../macros.gmk # top level
include ../../macros.gmk
include ../macros.gmk

OUTSQMANVERS	= $(OUTDIR)/sqmanvers
DIRSQMANVERS	= $(OUTSQMANVERS)/com/hp/sq/sqmanvers
DIRSCMVERS	= $(OUTSQMANVERS)/com/hp/sq/scm

all: progs

progs: $(JAVAPROGS)

$(EXPROOT)/lib/sqmanvers.jar: $(DIRSQMANVERS)/sqmanvers.class $(DIRSQMANVERS)/sqmanvers.mf
	cd $(OUTSQMANVERS); $(JAR) cvfm $(EXPROOT)/lib/sqmanvers.jar com/hp/sq/sqmanvers/sqmanvers.mf com/hp/sq/sqmanvers/sqmanvers.class

$(DIRSQMANVERS)/sqmanvers.class: sqmanvers.java
	$(JAVAC) -cp $(OUTSQMANVERS) -d $(OUTSQMANVERS) sqmanvers.java

$(DIRSQMANVERS)/sqmanvers.mf: sqmanvers $(INCEXPDIR)/SCMBuildMan.mf
	echo "Main-Class: com.hp.sq.sqmanvers.sqmanvers" > $(DIRSQMANVERS)/sqmanvers.mf
	./sqmanvers >> $(DIRSQMANVERS)/sqmanvers.mf

clean:
	$(RM) $(DIRSCMVERS)/*.class $(DIRSQMANVERS)/*.class $(DIRSQMANVERS)/*.mf

cleanall: clean
	$(RM) -fr $(BUILD_PLAT)

cleaner: clean
	$(RM) *~

_dummy := $(shell if [ ! -d $(DIRSQMANVERS) ]; then mkdir -p $(DIRSQMANVERS); fi)

