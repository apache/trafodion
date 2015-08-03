
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
function GetSQcnfg {
# Get SQ Node configuration
 TempList=`grep -o 'node-name=.[^A-Za-z].[0-9]*' $MY_SQROOT/sql/scripts/sqconfig | cut -d "=" -f 2 | cut -d ";" -f 1 | sort -u`

 i=0
 for NODE in $TempList
   do
     SQNodeNames[$i]=$NODE
     ((i=i+1))

     done

 # Check that the Node names were corretly added
 NumberOfSQnodes=${#SQNodeNames[*]}
 ExNodeList="$(echo ${SQNodeNames[@]} | tr ' ' ',')"


 if [ ! -z ${ExNodeList[@]} ]; then   
     echo "${ExNodeList[@]}"
 
 else
     echo
     echo "Could not parse $MY_SQROOT/sql/scripts/sqconfig file."
     echo "Please ensure sqenv.sh has been sourced and the sqconfig file is valid.  Then, re-run sqgen."
     echo
     exit 1;
 fi

}

if [ -f $SQ_PDCP ]; then

    GetSQcnfg

fi

error_txt=$MY_SQROOT/export/include/sql/SqlciErrors.txt
error_cat=$MY_SQROOT/export/bin$SQ_MBTYPE/mxcierrors.cat

# generate catalog only if it doesn't exist or $error_txt is older
if [ $error_txt -nt $error_cat ]; then
rm -f *ci[Ee]rrors.[ghm]		\
	   *ci[Ee]rrors_msg.[ghm]

rm -f $error_cat

# versioning
./msgfileVrsn.ksh	 > SqlciErrors.g

# append error message
cat $error_txt	>> SqlciErrors.g

# reformating
awk -f makemsg.awk SqlciErrors.g > SqlciErrors.m

echo "generating sql message catalog: $error_cat ... "
# generate catalog
gencat $error_cat SqlciErrors.m
fi

# On a cluster, copy it to all the nodes
if [ -e $SQ_PDCP ]; then
   echo "Doing a $SQ_PDCP -p -w ${ExNodeList[@]} -x `uname -n` $error_cat `dirname $error_cat`"
   $SQ_PDCP -p -w ${ExNodeList[@]} -x `uname -n` $error_cat `dirname $error_cat`
fi
#exit 0

