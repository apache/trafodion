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
#1. initial primary index value
#2. the desired number of rows that should be inserted
#3. the initial value of the ordinal counter
#4. the group id
#5. the proccess id
#6. (optional) the desired value for column B2

rm exec_insertt2
prim_index=$1
ordinal_counter=$3
sec_val=10
max_prim=`expr $1 + $2`
if [ $# -eq 5 ];then
	sec_val=$6
fi
while [ ! $prim_index -eq $max_prim ];
do
	if [ $# -eq 4 ]; then
		sec_val=`expr $prim_index '*' 2`
	fi
	echo 'execute insert_into_t2 using $4,$5,'$ordinal_counter','insert into CATMVS.MVSCHM.T1 values('$sec_index','$prim_index','0,0,20)',1,5,0;' >> exec_insert_into_t1
	ordinal_counter=`expr $ordinal_counter + 1`
	prim_index=`expr $prim_index + 1`
done
sec_val=`expr $prim_index '*' 2`

echo 'execute insert_into_t2 using $4,$5,'$ordinal_counter','insert into CATMVS.MVSCHM.T1 values('$sec_index','$prim_index','0,0,20)',1,5,1;' >> exec_insert_into_t1
