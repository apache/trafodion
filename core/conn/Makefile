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
# Makefile for packaging client products

include $(MY_SQROOT)/macros.gmk #top level

RM      = /bin/rm
ifeq ($(SQ_BUILD_TYPE),release)
  CLIENT_TAR	?= $(MY_SQROOT)/../trafodion_clients-$(TRAFODION_VER).tgz
else
  CLIENT_TAR	?= $(MY_SQROOT)/../trafodion_clients-$(TRAFODION_VER)-debug.tgz
endif


.PHONY: all
all: pkg-clients

pkg-clients: 
	tar -zcvf $(CLIENT_TAR) clients
clean:	
	$(RM) -rf $(MY_SQROOT)/../conn/clients 
	$(RM) -f $(CLIENT_TAR)
