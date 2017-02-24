#!/bin/sh
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
# This scripts is called by USAS.sh only.
# 1. Arguments 1-3 are the catalog, schema, and table.
#    Argument 4 is the object type ('BT' or 'MV').
#    Argument 5 is a number.
# 2. Convert names from internal to external format: 
#    Double all double quotes and surround with double quotes.
# 3. Run update statistics against table with NECESSARY keyword.
# 4. Update USTAT_AUTO_TABLE with current time, status (0 if no errors), 
#    and error string.  When performing update, the SQL command must have all
#    single quotes within the internal format names doubled.

if [[ "$2" = "" || "$6" != "" ]]; then
  echo "Usage: $0 <cat> <sch> <tbl> <objtype> <id>"
  exit
fi

cat="$1"
sch="$2"
tbl="$3"
objtype=$4
id=$5

if [ `uname` != "Linux" ]; then
   if [[ $mxcidir != "" && $mxlibdir != "" ]]; then
     # This is test debug environment.  Set vars accordingly
     MXCI="$mxcidir/mxci"
     autoloc="$mxlibdir"
   else
     # This is a production environment.
     MXCI="/usr/tandem/sqlmx/bin/mxci"
     autoloc="/usr/tandem/mx_ustat"
   fi

   sys=$1          # The system name
else
   export SQLMX_TERMINAL_CHARSET=UTF8
   MXCI=sqlci
   autoloc=$TRAF_HOME/export/lib/mx_ustat
   alias print=echo
   sys=NSK
fi

AUTODIR="$autoloc/autodir"
runlog="$AUTODIR/USTAT_RUNLOG${id}"
progressDir="$AUTODIR/USTAT_AUTO_PROGRESS"

# Print process ID to run${id} file so that StopAutoStats can kill.
# (Note: this will not necessarily kill update stats).
print "$$" > ${progressDir}/run${id}

# 1. Incoming names must be correctly capitalized.
# 2. Convert names to external format.
extcat=$(print "$cat" | sed 's/"/""/g')
extsch=$(print "$sch" | sed 's/"/""/g')
exttbl=$(print "$tbl" | sed 's/"/""/g')
extName="\"$extcat\".\"$extsch\".\"$exttbl\""

print "Runlog${id}: Received args: cat=$1, sch=$2, tbl=$3, type=$4, id=$5" > $runlog
print "Runlog${id}: Running ustats for table (external format name):" >> $runlog
print "             $extName." >> $runlog
print "MXCI=$MXCI" >> $runlog
print "autoloc=$autoloc" >> $runlog
autoTable="MANAGEABILITY.HP_USTAT.USTAT_AUTO_TABLES"

# 3. Run update statistics
if [[ $objtype = "BT" ]]; then objtype="TABLE"; fi
query="CONTROL QUERY DEFAULT USTAT_AUTOMATION_INTERVAL '0';"
query="$query MAINTAIN $objtype $extName, UPDATE STATISTICS 'ON NECESSARY COLUMNS';"
errs=$(print "$query" | $MXCI | grep "\*\*\* ERROR" | sed -n '1p')

# 4. Update USTAT_AUTO_TABLE
# Get errors
errnum=0
errtext=""

if [[ "$errs" != "" ]]; then
  # Get errors.
  errnum=$(print "$errs" | cut -d" " -f2)  # Get rid of text
  errnum=${errnum#"ERROR["}             # Get rid of surrounding word and brackets.
  errnum=${errnum%"]"}
  errtext=${errs#"*** ERROR[$errnum]"}
  print "Runlog${id}: Ustat err: $errnum, $errtext" >> $runlog
else
  print "Runlog${id}: Ustat: No errors." >> $runlog
fi

# Internal name must have single quotes duped for queries
intcat=$(print "$cat" | sed "s/'/''/g") 
intsch=$(print "$sch" | sed "s/'/''/g")
inttbl=$(print "$tbl" | sed "s/'/''/g")

if [[ $mxcidir != "" && $mxlibdir != "" ]]; then
  # This is test debug environment.  Set time to a value for testing.
  time="1776-07-04 12:00:00"
else
  time=$(date +"%Y-%m-%d %H:%M:%S")
fi
update="UPDATE $autoTable SET 
         LAST_RUN_GMT = TIMESTAMP '$time',
         LAST_RUN_LCT = TIMESTAMP '$time',
          RUN_STATUS  = $errnum, 
          ERROR_TEXT  = _UCS2'$errtext' 
       WHERE CAT_NAME = _UCS2'$intcat' AND 
             SCH_NAME = _UCS2'$intsch' AND 
             TBL_NAME = _UCS2'$inttbl';"

# Update USTAT_AUTO_TABLE with update stats output and assign
# output to 'errs' variable.
errs=$(print "$update" | $MXCI | grep "\*\*\* ERROR" | sed -n '1p')

# Check for errors from update and log the result.
errnum=0
errtext=""
if [[ "$errs" != "" ]]; then
  # Get errors.
  errnum=$(print "$errs" | cut -d" " -f2)  # Get rid of text
  errnum=${errnum#"ERROR["}             # Get rid of surrounding word and brackets.
  errnum=${errnum%"]"}
  errtext=${errs#"*** ERROR[$errnum]"}
  print "Runlog${id}: Update auto table err: $errnum, $errtext" >> $runlog
else
  print "Runlog${id}: Update auto table: No errors." >> $runlog
fi
