#!/bin/bash
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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

DATE=`date +%y%m%d`
BLDID=`../../../../../sqf/build-scripts/build.id`

#create_version_file function to update version string
# $1 should be filename
# $2 should be library version
# $3 should be timestamp
# $4 product name (ie TRAFODBC or TRAFODBC_DRVR)

create_version_file() {
echo "// $3 Version File generated on $2, bldId $BLDID" > $1
echo "extern char* versionString=\"$3 (Build Id [$BLDID])\";" >> $1
echo "extern \"C\" void $3_Build_Id_"$BLDID" ()" >> $1
echo "{ }" >> $1
}

create_version_file "trace/version.cpp" "$DATE" "TRAFODBC"
create_version_file "trace/version_drvr.cpp" "$DATE" "TRAFODBC_DRVR"
