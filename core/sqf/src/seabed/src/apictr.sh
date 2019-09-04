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
#
#
# compare apictr.cpp with apictr.h
#

x=0
e=0
TMPCPP=/tmp/zapicpp$$
TMPH=/tmp/zapih$$
rm -f $TMPCPP $TMPH

while read line; do
	if [ "$line" = '"AFIRST",' ]; then
		e=$x
	fi
	if [ $e != 0 ]; then
		echo $line | sed -e 's|"||g' -e 's|,||' >> $TMPCPP
	fi
	if [ "$line" = '"ZLAST"' ]; then
		e=0
	fi
	x=`expr $x + 1`
done < apictr.cpp

x=0
e=0
while read line; do
	if [ "$line" = 'SB_ACTR_AFIRST,' ]; then
		e=$x
	fi
	if [ $e != 0 ]; then
		echo $line | sed -e 's|,||' -e 's|SB_ACTR_||' >> $TMPH
	fi
	if [ "$line" = 'SB_ACTR_ZLAST' ]; then
		e=0
	fi
	x=`expr $x + 1`
done < apictr.h

diff -iw $TMPCPP $TMPH
rm -f $TMPCPP $TMPH
