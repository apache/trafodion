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
#parameters:
#1. initial primary key value
#2. square root of the desire number of rows that should be inserted
#3. the initial value of the ordinal value
#4. (Optional) the desired value for A3

rm exec_insert_into_t1
prim_index=$1
ordinal_counter=$3
n_val=`expr $1 + $2`
if [ $# -eq 4 ]; then
	third_col=$4
fi
while [ ! $prim_index -eq $n_val ];
do
	sec_index=$1
	while [ ! $sec_index -eq $n_val ];
	do
		if [ $# -eq 3 ]; then
			third_col=`expr $prim_index '*' $sec_index`
		fi
		sql_command="'insert into CATMVS.MVSCHM.T1 values($sec_index,$prim_index,$third_col,0,2,2)'"
		echo 'execute insert_into_t1 using 0,1,'$ordinal_counter','$sql_command',1,5,0;' >> exec_insert_into_t1
		sec_index=`expr $sec_index + 1`
		ordinal_counter=`expr $ordinal_counter + 1`
	done
	prim_index=`expr $prim_index + 1`
done
sql_command="'insert into CATMVS.MVSCHM.T1 values($sec_index,$prim_index,$third_col,0,0,2)'"
echo 'execute insert_into_t1 using 0,1,'$ordinal_counter','$sql_command',1,5,1;' >> exec_insert_into_t1


