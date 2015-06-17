#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2002-2015 Hewlett-Packard Development Company, L.P.
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
# Parameters:
# 1. Squeare root of the number of rows to insert
# 2. Max value
rm exec_fillup_t1
prim_index=1
sec_index=1
n_val=$1
while [ ! $prim_index -gt $n_val ];
do
	while [ ! $sec_index -gt $n_val ];
	do
		third_row=`expr $prim_index '*' $sec_index`
		echo 'insert into T1 values ('$sec_index','$prim_index','$third_row',0,"aaa");' >> exec_fillup_t1
		sec_index=`expr $sec_index + 1`
	done
	sec_index=1;
	prim_index=`expr $prim_index + 1`
done

third_row=`expr $2 + $2`
echo 'insert into T1 values ('$2','$2','$third_row',0,"aaa");' >> exec_fillup_t1
