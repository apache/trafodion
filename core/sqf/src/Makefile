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

DP2=se

all:
	cd seabed; $(MAKE)
	cd trafconf; $(MAKE)
	cd fs; $(MAKE)
	cd security; $(MAKE)
	cd stub; $(MAKE)
	cd tdm_logevent; $(MAKE)
	cd win; $(MAKE)
	cd nsk; $(MAKE)
	cd tm; $(MAKE)
	cd $(DP2); $(MAKE)
	cd stfs; $(MAKE) -f Makefile.stub
	cd tools; $(MAKE)

clean:
	cd seabed; $(MAKE) clean
	cd trafconf; $(MAKE) clean
	cd win; $(MAKE) clean
	cd tm; $(MAKE) clean
	cd stfs; $(MAKE) -f Makefile.stub clean
	cd tools; $(MAKE) clean
cleanall:
	cd seabed; $(MAKE) cleanall
	cd trafconf; $(MAKE) cleanall
	cd win; $(MAKE) cleanall
	cd tm; $(MAKE) cleanall
	cd stfs; $(MAKE) -f Makefile.stub cleanall
	cd tools; $(MAKE) cleanall
cleaner:
	cd seabed; $(MAKE) cleaner
	cd trafconf; $(MAKE) cleaner
	cd win; $(MAKE) cleaner
	cd tm; $(MAKE) cleaner
	cd stfs; $(MAKE) -f Makefile.stub cleaner
	cd tools; $(MAKE) cleaner
