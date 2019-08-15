#!/bin/bash
#
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

chkext() {
	for f in $chkfiles; do
		echo "#include \"seabed/$f\"" > $tmpi
		echo 'int main() { return 0; }' >> $tmpi
		flags="-I$incl"
		g++ $flags $tmpi -o $tmpo > /dev/null 2>/dev/null
		if [ $? != 0 ]; then
			echo "problem with $incl/$f"
			echo g++ $flags $tmpi -o $tmpo
			g++ $flags $tmpi -o $tmpo
			ret=1
		else
			echo "$incl/$f ok"
		fi
	done
}

chkint() {
	for f in $chkfiles; do
		echo "#include \"$f\"" > $tmpi
		flags="-c -I. -I$mpiincl -I$monincl -I$incl"
		g++ $flags $tmpi -o $tmpo > /dev/null 2>/dev/null
		if [ $? != 0 ]; then
			echo "problem with $incl/$f"
			echo g++ $flags $tmpi -o $tmpo
			g++ $flags $tmpi -o $tmpo
			ret=1
		else
			echo "$incl/$f ok"
		fi
	done
}

# main
ret=0
tmpi=/tmp/zchk$$.cpp
tmpo=/tmp/zchk$$.o
incl="../../../export/include"
monincl="../../../monitor/linux"
mpiincl="$MPI_ROOT/include"

pushd $incl/seabed
chkfiles=`ls *.h`
popd
chkext

pushd $incl/seabed
chkfiles=`ls int/*.h | grep -v 'int\/da.h'`
popd
chkext

chkfiles=`ls *.h`
chkint

rm -f $tmpi $tmpo
exit $ret
