#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
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
#######################################################################

ifeq ($(TARGTYPE),nt)
$(error Build of NT version not supported)
endif

CPPSRC := security_sql_cli.cpp globalstubs.cpp

CXXFLAGS += -Wsrl -Wnowarn=1303

#DEP_LIBS := ems

INCLUDE_DIRS := cli \
                common \
                export \
                security/shared/ems \
                security/shared/security_sql_cli/inc \
                sqlcli \
                sqlmxevents

SRCPATH := security/shared/security_sql_cli/src cli
