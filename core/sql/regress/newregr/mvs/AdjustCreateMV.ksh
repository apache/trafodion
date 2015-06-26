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
if [ $# -eq 0 ]; then
   	echo At least one parameter is needed, bye.
   	exit
fi
attrib -r files_list

rm files_list
    
for arg in $*
do
   	echo $arg >> files_list
done
echo EOF>>files_list

attrib +r files_list

cat files_list | perl AdjustCreateMV.pl

attrib -r files_list
