#!/bin/sh

#-- @@@ START COPYRIGHT @@@
#--
#-- Licensed to the Apache Software Foundation (ASF) under one
#-- or more contributor license agreements.  See the NOTICE file
#-- distributed with this work for additional information
#-- regarding copyright ownership.  The ASF licenses this file
#-- to you under the Apache License, Version 2.0 (the
#-- "License"); you may not use this file except in compliance
#-- with the License.  You may obtain a copy of the License at
#--
#--   http://www.apache.org/licenses/LICENSE-2.0
#--
#-- Unless required by applicable law or agreed to in writing,
#-- software distributed under the License is distributed on an
#-- "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#-- KIND, either express or implied.  See the License for the
#-- specific language governing permissions and limitations
#-- under the License.
#--
#-- @@@ END COPYRIGHT @@@

# process the parser through bison to get a list of shift/reduce and reduce/reduce conflicts
# in file sqlparser.output. Remove the directory name from the output.
topdir=$1
bisondir=${TOOLSDIR}/bison_3_linux/share/bison
bisonexedir=${TOOLSDIR}/bison_3_linux/bin
parserdir=$topdir/parser
toolsdir=$topdir/regress/tools

# m4 is a utility needed by bison
export M4=$bisonexedir/m4


export BISON_PKGDATADIR=$bisondir

$bisonexedir/bison -v $parserdir/sqlparser.y 2>&1 | sed -r 's/.+sqlparser\.y/sqlparser.y/' >LOGTOK;

# extract a list of conflicts from the sqlparser.output file
awk '/State [0-9]+ conflicts:/ { printf "%06d ", $2; print } ' sqlparser.output | grep State | sed -r 's/ State [0-9]+//' >LOGTOK_conflicts
# extract a list of parser states (state number and first descriptive line) from the parser output file
awk '/^State 0$/,/untilthelastline/ { print }' sqlparser.output | awk '/^State [0-9]+$/ { printf "%06d ", $2; getline; getline; print }'  >LOGTOK_gramm
# join the two extracted files on the state number (first 6 digits)
join LOGTOK_conflicts LOGTOK_gramm >LOGTOK_join
# replace state numbers with nnnn, so unrelated parser changes don't cause this test to fail
echo " " >>LOGTOK
cat LOGTOK_join | sed -r 's/^[0-9]+ conflicts/nnnn conflicts/' | sed -r 's/reduce [0-9]+/reduce nnnn/' >>LOGTOK

# delete some of the larger output files produced (uncomment for debugging)
rm sqlparser.output sqlparser.tab.c;
