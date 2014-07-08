# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
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

all:
	cd src; $(MAKE)
	cd tools; $(MAKE)

buildall:
	cd evsim; $(MAKE) all
	cd mpisim; $(MAKE) all
	cd src; $(MAKE) all

setup:
	@# do nothing

clean:
	cd src; $(MAKE) clean
	cd tools; $(MAKE) clean

cleaner:
	cd src; $(MAKE) cleaner
	cd tools; $(MAKE) cleaner

cleanall:
	cd evsim; $(MAKE) cleanall
	cd mpisim; $(MAKE) cleanall
	cd src; $(MAKE) cleanall
	cd tools; $(MAKE) cleanall

