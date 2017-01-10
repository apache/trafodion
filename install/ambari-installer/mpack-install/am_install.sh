#!/bin/bash
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

# generate new unique key local to ambari server
tf=/tmp/trafssh.$$
rm -f ${tf}*
/usr/bin/ssh-keygen -q -t rsa -N '' -f $tf

instloc="$1"

config="${instloc}/traf-mpack/common-services/TRAFODION/2.1/configuration/trafodion-env.xml"

chmod 0600 $config  # protect key
sed -i -e "/TRAFODION-GENERATED-SSH-KEY/r $tf" $config # add key to config properties

rm -f ${tf}*

# tar up the mpack, included generated key
tball="${instloc}/traf-mpack.tar.gz"

cd "${instloc}"
tar czf "$tball" traf-mpack

# install ambari mpack
ambari-server install-mpack --verbose --mpack="$tball"
ret=$?

exit $ret
