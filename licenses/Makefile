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

all: LICENSE-src LICENSE-server LICENSE-pyinstall LICENSE-install LICENSE-clients NOTICE-server DISCLAIMER-server

# All source code included in Trafodion source
LICENSE-src:
	cat Apache lic-components-src lic-server-src lic-test-src lic-pyinstall-src > $@

# Binary licenses for each separately packaged component
# Must include source licenses as well as software bundled in at build time
LICENSE-server:
	cat Apache lic-server-src lic-server-bin > $@

NOTICE-server:
	cat ../NOTICE note-server-bin > $@

DISCLAIMER-server:
	cat ../DISCLAIMER > $@

LICENSE-install:
	cat Apache > $@

LICENSE-pyinstall:
	cat Apache lic-pyinstall-src > $@

LICENSE-clients:
	cat Apache lic-clients-bin > $@

clean:
	rm -f LICENSE* NOTICE*
