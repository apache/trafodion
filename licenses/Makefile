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

all: LICENSE-src LICENSE-dcs LICENSE-server LICENSE-install LICENSE-rest LICENSE-clients

# All source code included in Trafodion source
LICENSE-src:
	cat Apache lic-components-src lic-dcs-src lic-server-src lic-test-src > $@

# Binary licenses for each separately packaged component
# Must include source licenses as well as software bundled in at build time
LICENSE-dcs:
	cat Apache lic-dcs-src lic-dcs-bin > $@

LICENSE-server:
	cat Apache lic-server-src lic-server-bin > $@

LICENSE-install:
	cat Apache > $@

LICENSE-rest:
	cat Apache lic-rest-bin > $@

LICENSE-clients:
	cat Apache lic-clients-bin > $@

clean:
	rm LICENSE*
