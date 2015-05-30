-- @@@ START COPYRIGHT @@@
--
-- (C) Copyright 2014-2015 Hewlett-Packard Development Company, L.P.
--
--  Licensed under the Apache License, Version 2.0 (the "License");
--  you may not use this file except in compliance with the License.
--  You may obtain a copy of the License at
--
--      http://www.apache.org/licenses/LICENSE-2.0
--
--  Unless required by applicable law or agreed to in writing, software
--  distributed under the License is distributed on an "AS IS" BASIS,
--  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--  See the License for the specific language governing permissions and
--  limitations under the License.
--
-- @@@ END COPYRIGHT @@@
#######################################################################
# @@@ START COPYRIGHT @@@
#
#        HP CONFIDENTIAL: NEED TO KNOW ONLY
#
#        Copyright 2002
#        Hewlett-Packard Development Company, L.P.
#        Protected as an unpublished work.
#
#  The computer program listings, specifications and documentation 
#  herein are the property of Hewlett-Packard Development Company,
#  L.P., or a third party supplier and shall not be reproduced, 
#  copied, disclosed, or used in whole or in part for any reason 
#  without the prior express written permission of Hewlett-Packard 
#  Development Company, L.P.
#
# @@@ END COPYRIGHT @@@
# +++ Copyright added on 2003/12/3
# +++ Code modified on 2002/7/24
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

cat files_list | perl AlterTable.pl

attrib -r files_list
