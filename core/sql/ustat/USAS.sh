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
# This is the usas program which runs periodically for update statistics
# and reorg automation.
#
# Before running maintain/reorg or update stats tasks, this program divides
# the total time between the 2 operations based on user input.
# It then does reorgs followed by update statistics tasks (including
# persistent sample table cleanup).
#
# Pre-automation steps:
# ---------------------
# 1. Compute time allocated for reorg and ustat based on user input.
# 2. Check if USAS is already running.  Don't run if it is.
# 3. Create progress directory if not already running.
# 4. Create USTAT_AUTO_TABLES if it doesn't exist:
#    If USTAT_AUTO_TABLES does not exist, create it and populate it with all tables
#    in the NEO catalog, setting the ADDED_BY column in every entry to 'SYSTEM'
#    (NOTE: This table is created by T0725, so this step is unnecessary now.)
# 5. If all entries in USTAT_AUTO_TABLES have ADDED_BY = 'SYSTEM', then make sure
#    all tables inserted into it.
# 6. Create a list of tables from every entry in the USTAT_AUTO_TABLES.
#
# REORG automation:
# -----------------
# 1. For each table in the list, run maintain 'reorg if needed'. Keep track
#    of time taken for each operation. Stop if accumulated time exceeds the
#    allocated reorg time.
# 2. Call MAINTAIN and pass in the allocated maxruntime for reorgs.
#    MAINTAIN fill find out the tables that need to be reorged and
#    reorg as many as could be done in the allocated time.
# 3. Add any leftover time to updStats window time.
#
# UPDATE STATISTICS automation/persistent sample cleanup:
# ------------------------------------------------------
# 7. Cleanup persistent samples tables.  Remove any persistent sample tables
#    older than USTAT_MAX_SAMPLE_AGE days old in NEO.PUBLIC_ACCESS_SCHEMA.
# 8. Automation: For each table in the list, run the following via the ODBC
#    interface:
#       UPDATE STATISTICS FOR TABLE <table> ON NECESSARY COLUMNS;
# 9. Automation: Run UPDATE STATISTICS at priority level as specified on
#    different CPUs.

# Constants:
# START_CPU:     The number of the first CPU on which ustats runs.
# END_CPU:       The number of the last CPU on which ustats can run.
# PARALLEL_RUNS: The number of ustat commands to run in parallel.
# Note: It doesn't matter if START_CPU+PARALLEL_RUNS > END_CPU.

#========================================================================
# Functions
#========================================================================
log()
{
  # This function should be invoked as: log "<message>" ["<text to override 'USAS'>"]
  # This function prints the message passed as the first argument to the log.
  # "USAS" is prepended to line before printing, unless a second argument is
  # is passed, in which case this argument is prepended.
  typeset time=$(date +"%H:%M:%S")

  # if $2 is passed in, then print that instead of "USAS" as the logged
  # row prefix.
  if [[ $2 != "" ]]; then
    echo "$2" [$time]:"$1" >> $LOG
  else
    echo USAS [$time]:"$1" >> $LOG
  fi
}


usasCleanup()
{
  $pdsh_a rm -fr ${AUTODIR}/USTAT_AUTO_PROGRESS

  rm $AUTODIR/STOP_AUTO_STATS 2>/dev/null
}

getCQD()
{
  # This function should be invoked as:  $(getCQD "<cqd name>")
  # This function obtains and returns the value of a CQD (passed as first argument).
  getCQDquery="control query default SHOWCONTROL_UNEXTERNALIZED_ATTRS 'ON';"
  getCQDquery="$getCQDquery showcontrol default $1;"

  getCQDvalue=$(echo $(echo "$getCQDquery" | $MXCI | grep $1 | grep -v settings | grep -v showcontrol))
  getCQDvalue=$(echo $(echo "$getCQDvalue" | tr -s ' ' ' ' | cut -d" " -f2))
  echo $getCQDvalue # return value.
}

isANum()
{
  # This function should be invoked as:  $(isANum <number>)
  # Returns "TRUE" if the first argument to the function is a number. "FALSE" otherwise.
  expr $1 + 0 >/dev/null 2>&1
  if [ $? -ne 0 ]; then echo "FALSE"; else echo "TRUE"; fi
}

runQuery()
{
  # This function should be invoked as:  $(runQuery "<query>")
  # runQuery will run the passed query via SQL.  If it detects that
  # an error occurred, it will log this and return 1.  Otherwise
  # it will return 0.
  rQquery=$*
  echo "$rQquery" | $MXCI >$AUTODIR/tmp
  rQerrsOutput=$(cat $AUTODIR/tmp | grep "\*\*\* ERROR" | sed -n '1p')

  rQerrnum=0
  rQerrtext=""
  if [[ "$rQerrsOutput" != "" ]]; then # Errors
    # Get errors.
    rQerrnum=$(echo "$rQerrsOutput" | cut -d" " -f2) # Get rid of text
    rQerrnum=${rQerrnum#"ERROR["}                     # Get rid of surrounding word and brackets.
    rQerrnum=${rQerrnum%"]"}
    rQerrtext=${rQerrsOutput#"*** ERROR[$rQerrnum]"}
    log "Error $rQerrnum, $rQerrtext with query: $rQquery "
    return 1
  fi
  return 0
}

waitFor()
{
  # Wait for the text, $1, to appear in file $2 or $timeout seconds (whichever occurs first).
  # If the wait times out ($timeout secs pass), the text "ERROR" will be inserted into file $2.
  text=$1
  file=$2
  timeout=180  # 3 minutes
  log "Waiting for $text in $file."
  let cnt=0
  while [[ $(grep "$text" $file) = "" && $cnt -le $timeout ]]; do
    let cnt=$cnt+1
    sleep 1
    log "Waited $cnt seconds."
  done
  if [[ $(grep "$text" $file) = "" && $cnt -gt $timeout ]]; then
    log "Timed out waiting for $text in $file."
    echo "ERROR" >> $file
  fi
}

#========================================================================
# Task Functions 
#========================================================================
cleanupPersSamples()
{
  typeset maxSampleDays=$1
  typeset maxSampleSecs  
  # Convert days to seconds.
  let maxSampleSecs=$maxSampleDays*86400
  
  log "Finding persistent sample tables older than $maxSampleDays days old."
  # Get current time in GMT.
  currentDate=$(date -u "+%Y-%m-%d %H:%M:%S")
  pred="where CREATION_DATE < (TIMESTAMP '$currentDate' - $maxSampleSecs)"
  query1="SELECT _UCS2'PerSample: ' || TABLE_NAME FROM NEO.PUBLIC_ACCESS_SCHEMA.PERSISTENT_SAMPLES $pred;"
  query2="DELETE FROM NEO.PUBLIC_ACCESS_SCHEMA.PERSISTENT_SAMPLES $pred;"
  echo "$query1 $query2" | $MXCI | grep "PerSample:" |\
     grep -v "SELECT" | cut -d" " -f2- > $PERSLIST
  exec 4<$PERSLIST # Use file descriptor 4 to read list.
  read table <&4;
  query3="CONTROL QUERY DEFAULT CAT_PERMIT_OFFLINE_ACCESS 'ON';"
  while [[ $table != "" ]]; do
    log "Dropping persistent sample table $table."
    query3="$query3 ALTER TABLE NEO.PUBLIC_ACCESS_SCHEMA.$table DROPPABLE;"
    query3="$query3 DROP TABLE NEO.PUBLIC_ACCESS_SCHEMA.$table;"
    read table <&4;
  done
  exec 4<&- # Close file descriptor 4
  echo "$query3" | $MXCI > /dev/null
}

cleanupSamples()
{
  # Cleanup regular sample tables.  Only cleanup samples that are older 
  # than that allowed for persistent sample tables, because the naming 
  # convention is the same.
  typeset maxSampleDays=$1
  typeset maxSampleSecs  

  let maxSampleDays=$maxSampleDays+1
  let maxSampleSecs=$maxSampleDays*86400
  log "Finding sample tables older than $maxSampleDays days ($maxSampleSecs secs) old."
  
  query1="SELECT DISTINCT _ISO88591'Version:', S.SCHEMA_VERSION
           FROM HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.CATSYS C,
                HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.SCHEMATA S
           WHERE C.CAT_UID=S.CAT_UID AND
                 C.CAT_NAME=_UCS2'NEO' AND
                 S.SCHEMA_VERSION >= 2300;"
  versions=$(echo "$query1" | $MXCI | grep "Version:" | grep -v "SELECT" |\
             tr -s ' ' ' ' | cut -d" " -f2)
  # Get current seconds as GMT.
  let yrsSince1970=$(date -u "+%Y")-1970
  let currSecs=$(expr $yrsSince1970*31556926+10#$(date -u "+%j")*86400+10#$(date -u "+%H")*3600)
  let currSecs=$currSecs+$(expr 10#$(date -u "+%M")*60+10#$(date -u "+%S"))
  for version in $versions; do
     log "Checking for sample tables with schema version $version."
     query2="SET CATALOG NEO;
             SELECT _ISO88591'Sample: ', O.OBJECT_NAME
              FROM HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.CATSYS C, 
                   HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.SCHEMATA S,
                   HP_DEFINITION_SCHEMA.OBJECTS O 
              WHERE C.CAT_UID=S.CAT_UID AND 
                    S.SCHEMA_UID=O.SCHEMA_UID AND
                    S.SCHEMA_NAME=_UCS2'PUBLIC_ACCESS_SCHEMA' AND 
                    O.OBJECT_TYPE=_ISO88591'BT' AND
                    O.OBJECT_NAME LIKE _UCS2'SQLMX\_%' ESCAPE _UCS2'\';"
     echo "$query2" | $MXCI | grep "Sample:" |\
           grep -v "SELECT" | cut -d" " -f2- > $SAMPLIST
     exec 4<$SAMPLIST # Use file descriptor 4 to read list.
     read table <&4;
     query3="CONTROL QUERY DEFAULT CAT_PERMIT_OFFLINE_ACCESS 'ON';"
     while [[ $table != "" ]]; do
       log "Checking sample table $table."
       sampSecs=$(echo $table | cut -d"_" -f3)
       log "currSecs=$currSecs, sampSecs=$sampSecs"
       let diffSecs=$currSecs-$sampSecs
       if [[ $diffSecs -gt $maxSampleSecs ]]; then 
         log "Dropping sample table $table."
         query3="$query3 ALTER TABLE NEO.PUBLIC_ACCESS_SCHEMA.$table DROPPABLE;"
         query3="$query3 DROP TABLE NEO.PUBLIC_ACCESS_SCHEMA.$table;"
       fi
       read table <&4;
     done
     exec 4<&- # Close file descriptor 4
     echo "$query3" | $MXCI > /dev/null
  done
}

removeOldHists()
{
  # Automation must be ON.
  typeset maxHistAge=$(getCQD "USTAT_AUTO_MAX_HIST_AGE")
  if [[ $(isANum $maxHistAge) = "TRUE" ]] &&
     [[ $maxHistAge -ne 0 ]]; then

    log "USTAT_AUTO_MAX_HIST_AGE > 0 (=$maxHistAge).  Removing old histograms"  
  # Remove histograms with a STATS_TIME or READ_TIME older than 
  # USTAT_AUTO_MAX_HIST_AGE days.  For a single column histogram 
  # with REASON not empty, if a READ_TIME is old (but not 0), 
  # then we assume the histogram has not been used recently.  
  # If a STATS_TIME is old and has a READ_TIME=0, then we also 
  # assume the histogram has not been used recently. Multi-column
  # histograms that have a column for which the single column
  # histogram is old, is also assumed to not be recently used.
  # This latter is because update statistics with NECESSARY will
  # update a multi-column histogram only if all single column 
  # histograms for the MC histograms need to be updated.  The 
  # maximum age allowed for histograms is obtained from CQD
  # USTAT_AUTO_MAX_HIST_AGE (in days).

  # Determine GMT time difference.
  let yrsSince1970=$(date "+%Y")-1970
  let currLclSecs=$(expr $yrsSince1970*31556926+10#$(date "+%j")*86400+10#$(date "+%H")*3600)
  let currLclSecs=$(expr $currLclSecs+10#$(date "+%M")*60+10#$(date "+%S"))
  let yrsSince1970=$(date -u "+%Y")-1970
  let currGMTSecs=$(expr $yrsSince1970*31556926+10#$(date -u "+%j")*86400+10#$(date -u "+%H")*3600)
  let currGMTSecs=$(expr $currGMTSecs+10#$(date -u "+%M")*60+10#$(date -u "+%S"))
  let secsOffset=$currLclSecs-$currGMTSecs

  typeset maxHistDays=$(getCQD "USTAT_AUTO_MAX_HIST_AGE")
  if [[ $(isANum $maxHistDays) = "FALSE" ]]; then maxHistDays=30; fi 
    # Set to 30 days if cannot obtain from MX.
  log "maxHistDays=$maxHistDays"
  log "Deleting histograms that have not been used for $maxHistDays days."
  log "Number of seconds difference from GMT = $secsOffset."
  
  # Convert days to seconds.
  typeset maxHistSecs
  let maxHistSecs=$maxHistDays*86400+$secsOffset
  let maxHistHrs=$maxHistSecs/3600
  log "Max age of READ_TIME (or STATS_TIME with 0 READ_TIME):"
  log "Max age in secs (GMT adjusted)= $maxHistSecs ($maxHistHrs hours)."

  # Find all schemas with version >= 2300 (when automation was first 
  # implemented - and therefore have READ_TIME, ...)  
  typeset query1 query2 query3 query4 query5 query6 ver schema
  query1="SELECT 'Schema:', s.schema_version, rtrim(s.schema_name)
           FROM HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.CATSYS C,
                HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.SCHEMATA S
           WHERE C.CAT_UID=S.CAT_UID
             AND C.CAT_NAME= _UCS2'NEO'
             AND S.SCHEMA_VERSION >= 2300
             AND S.SCHEMA_NAME <>  _UCS2'PUBLIC_ACCESS_SCHEMA'
             AND S.SCHEMA_NAME NOT LIKE _UCS2'HP\_%' ESCAPE _UCS2'\' 
             AND S.SCHEMA_NAME NOT LIKE _UCS2'VOLATILE\_SCHEMA\_%' ESCAPE _UCS2'\'
             AND S.SCHEMA_NAME NOT LIKE _UCS2'@%'
             AND S.SCHEMA_NAME NOT LIKE _UCS2'DEFINITION\_SCHEMA\_VERSION\_%' ESCAPE _UCS2'\';"
  runQuery "$query1"
  # runQuery puts output in $AUTODIR/tmp
  # Create list of schemas.
  cat $AUTODIR/tmp | grep "Schema:" | grep -v "SELECT" |\
             tr -s ' ' ' ' | cut -d" " -f2- > $AUTODIR/USTAT_SCHEMAS

  exec 4<$AUTODIR/USTAT_SCHEMAS
  read ver schema <&4;  # Get first schema name and version.

  while [[ "$schema" != "" && "$ver" -ge 2300 ]]; do
    extsch=$(print "$schema" | sed 's/"/""/g')
    log "Checking schema \"$extsch\"."
    query1="SET SCHEMA NEO.\"$extsch\";"
    query2="CREATE TABLE HISTSTODEL (
              TABLE_UID LARGEINT NOT NULL NOT DROPPABLE,
              HISTOGRAM_ID INT UNSIGNED NOT NULL NOT DROPPABLE,
              COLCOUNT INT,
              PRIMARY KEY (TABLE_UID, HISTOGRAM_ID))
            NO PARTITION;"
    # Select and insert histograms to delete.
    query3="INSERT INTO HISTSTODEL
           (SELECT DISTINCT TABLE_UID, HISTOGRAM_ID, COLCOUNT -- Find single and multi-col histograms
              FROM HISTOGRAMS H
              WHERE (TABLE_UID, COLUMN_NUMBER) IN 
                (SELECT DISTINCT TABLE_UID, COLUMN_NUMBER
                   FROM HISTOGRAMS H, NEO.HP_DEFINITION_SCHEMA.PARTITIONS P
                   WHERE
                   (-- Histograms that have not been recently read.
                       (H.READ_TIME <> TIMESTAMP '0001-01-01 00:00:00' AND
                        CURRENT_TIMESTAMP - H.READ_TIME > INTERVAL '$maxHistSecs' SECOND(12,0)) 
                    -- Histograms that have not been recently updated and never read.
                    OR (H.READ_TIME = TIMESTAMP '0001-01-01 00:00:00' AND
                        CURRENT_TIMESTAMP - H.STATS_TIME > INTERVAL '$maxHistSecs' SECOND(12,0))
                   )
                   AND (H.REASON <> ' ' AND 
                        H.COLCOUNT = 1 )  -- Find single column hists that have been generated.
                 FOR READ UNCOMMITTED ACCESS)
            FOR READ UNCOMMITTED ACCESS);"
    # Perform delete of histograms.
    query4="DELETE FROM HISTOGRAMS WHERE TABLE_UID, HISTOGRAM_ID IN 
              (SELECT TABLE_UID, HISTOGRAM_ID FROM HISTSTODEL);"
    query5="DELETE FROM HISTOGRAM_INTERVALS WHERE TABLE_UID, HISTOGRAM_ID IN
              (SELECT TABLE_UID, HISTOGRAM_ID FROM HISTSTODEL);"
    query6="SHOWDDL SCHEMA \"$extsch\";" # Used as a marker to wait on.
    query7="DROP TABLE HISTSTODEL;"
  
    runQuery "$query1 $query2" # Set schema, create table.  Do not check for errors.
    log "Checking for unused histograms and inserting to HISTSTODEL table."

    runQuery "$query1 $query3" # Set schema, insert hists to delete into table.
    if [[ $? = 0 ]]; then 
      # No errors occured.  Delete histograms.
      # Run MXCI interactively.  Use 'tail -f' command to send interactive commands
      # to MXCI via file DelOldHistsCmds.  Output from MXCI goes to DelOldHistsOut.
      echo "BEGIN WORK; $query1 $query4 $query5 $query6" > $AUTODIR/DelOldHistsCmds
      tail -f -n1000 $AUTODIR/DelOldHistsCmds | $MXCI > $AUTODIR/DelOldHistsOut &
      # Save pid (can't use 'echo $!')
      pid=$(ps -ef | grep "tail -f" | grep "DelOldHistsCmds" | tr -s ' ' ' ' | cut -d" " -f2) 
      # Wait for SHOWDDL to execute.
      waitFor "CREATE SCHEMA" $AUTODIR/DelOldHistsOut  # Will wait up to 3 minutes.
      log "MXCI background processes: pid=$pid"
      if [[ $(grep "ERROR" $AUTODIR/DelOldHistsOut) != "" ]]; then
        log "An error occurred attempting to delete from the HISTOGRAMS/INTERVALS tables."
        echo "ROLLBACK WORK; EXIT;" >> $AUTODIR/DelOldHistsCmds
      else 
        log "Deleted unused histograms from HISTOGRAMS/HISTOGRAM_INTERVALS tables."
        echo "COMMIT WORK; EXIT;" >> $AUTODIR/DelOldHistsCmds
      fi
      waitFor "End of MXCI Session" $AUTODIR/DelOldHistsOut  # Will wait up to 3 minutes.
      kill -9 $pid # End 'tail -f' process - which will cause MXCI to exit.
    else
      log "An error occurred attempting to INSERT/SELECT into HISTSTODEL."
      log "Unable to delete old histograms."
    fi

    runQuery "$query1 select 'HIST:', * from HISTSTODEL;"
    # runQuery puts output in $AUTODIR/tmp
    log "Histograms that will be deleted include (TableUID, HistID, HistSize):"
    cat $AUTODIR/tmp | grep "HIST:" >> $LOG

    log "Dropping HISTSTODEL table."
    runQuery "$query1 $query7"

    # Get next schema name and version.
    read ver schema <&4;
  done
  else
    log "USTAT_AUTO_MAX_HIST_AGE = 0.  Will not remove old histograms"  
  fi # If automation is ON.
}

#========================================================================
# Start of USAS main code.
#========================================================================
# return the number of row field SQL standard output.
# The value should be contained in the 7th row. Valid only for SQ
alias getRow="sed '1,6 d' | sed '2,6 d' | sed '1,\$s/ //g'"

  # no priority setup for SQ
  #setup pdsh 
  pdsh=`which pdsh 2>/dev/null`
  
  if [ "$pdsh" = "" ]; then
    # SQ workstation
    pdsh_a=""
    pdcp_cmd="cp"
  else
    # SQ cluster
    pdsh_a="pdsh -a"
    pdcp_cmd="pdcp -a"
  fi
  
  
  alias print=echo
  export SQLMX_TERMINAL_CHARSET=UTF8
  MXCI="$TRAF_HOME/export/bin32/sqlci"
  autoloc="$TRAF_HOME/export/lib/mx_ustat"
  this_node=$(uname -n)


#========================================================================

AUTODIR="$autoloc/autodir"
PREV="$autoloc/autoprev"

LOG="$AUTODIR/USTAT_AUTO_LOG"
MXCIOUT="$AUTODIR/USTAT_AUTO_MXCI"
CATLIST="$AUTODIR/USTAT_AUTO_CATS"
SCHLIST="$AUTODIR/USTAT_AUTO_SCHS"
TBLLIST="$AUTODIR/USTAT_AUTO_TBLS"
OBJTYPE="$AUTODIR/USTAT_AUTO_TYPE"
PERSLIST="$AUTODIR/USTAT_PERS_LIST"
SAMPLIST="$AUTODIR/USTAT_SAMP_LIST"
CQDFILE="$AUTODIR/USTAT_CQDS_SET"
RUN_LOG_USTAT="$autoloc/RunLogUstats.sh"
FILL_AUTO_TBL="$autoloc/FillAutoTbl.sh"

# output of maintain/reorg command will be in this file.
REORGOUT="$AUTODIR/REORG_AUTO_MXCI"

TableCat="NEO"
AutoCat="MANAGEABILITY"
AutoSch="HP_USTAT"
AutoTable=$AutoCat"."$AutoSch".USTAT_AUTO_TABLES"
AutoTblList="NEO.PUBLIC_ACCESS_SCHEMA.USTAT_AUTO_RUN_LIST"
AutoTempTable="NEO.PUBLIC_ACCESS_SCHEMA.USTAT_AUTO_TMP_TABLES"


  # First find the total number of SQL storage nodes.
  # this line greps for the node-id text line in sqconfig file, looking for
  # storage nodes. The logical node names are stored in $nodes.
  nodes=$(grep "^node-id=" $TRAF_CONF/sqconfig | grep storage | cut -d ';' -f 2 | cut -d '=' -f 2)


  # grep the number of cores per SQ node
  cores=$(grep "^node-id=" $TRAF_CONF/sqconfig | grep storage | cut -d ';' -f 3 | cut -d '=' -f 2 | cut -d '-' -f 2)

  #
  # Uncomment and modify the following two variables to unit test special cases. 
  # The following assignment assumes there are two nodes in the cluster, each
  # with 8 cores.
  #
  #cores="7 7"
  #nodes="n3 n4"

  headnodes=$(grep "^node-id=" $TRAF_CONF/sqconfig | grep connection | cut -d ';' -f 2 | cut -d '=' -f 2)

  # If no storage text line exists as in workstation environment, grep for
  # _virtualnodes.
  if [ "$nodes" = "" ]; then
    nodes=$(grep "^_virtualnodes" $TRAF_CONF/sqconfig)
  
    # We can not find node text line. Bail out.
    if [ "$nodes" = "" ]; then
       log "Ill-formatted sqconfig file at $TRAF_CONF"
       log "Exit the USAS.sh script."
      
       # In the future, we can hook up this exit with SQ version of EMS log.
       exit 1;
    fi
  
    # Virtual node configuration. Set one CPU, one node.
    nodes="NSK"
    headnode=""
  fi
  
  segs="NSK"
  numnodes=$(echo $nodes | wc -w)

  START_CPU=1
  END_CPU=1 

  TOT_CPUS=$numnodes

  SYSTEM_NAME="NSK"

  $pdsh_a mkdir $AUTODIR 2> /dev/null


time=$(date)
# PreAuto Step #2. Check if USAS is already running.  Don't run if it is.
if [[ -a $AUTODIR/USTAT_AUTO_PROGRESS ]]; then
   # Check if the progress directory has not been changed in 1 days
   isOlder=$(find $AUTODIR -name USTAT_AUTO_PROGRESS -mtime +1)
   if [[ $isOlder != "" ]]; then
     # USAS.sh must not be running, remove progress dir and continue.
     usasCleanup;
   else
     print "USAS failed to start at $time." > $AUTODIR/USTAT_AUTO_CANT_START
     print "Progress dir exists - already running, exiting..." >> $AUTODIR/USTAT_AUTO_CANT_START
     exit
   fi
fi

# Remove old PREV, move old AUTODIR to PREV, and then make new AUTODIR.
# PreAuto Step #3. Create progress directory if not already running.
# This will keep another USAS program from running at the same time.

  doPreAutoStep3="$TRAF_VAR/doPreAutoStep3"

  echo "rm -fr $PREV" > $doPreAutoStep3
  echo "mv $AUTODIR $PREV" >> $doPreAutoStep3
  echo "mkdir $AUTODIR" >> $doPreAutoStep3
  echo "mkdir $AUTODIR/USTAT_AUTO_PROGRESS" >> $doPreAutoStep3
  echo "if [[ -a $PREV/USTAT_CQDS_SET ]]; then" >> $doPreAutoStep3
  echo " touch $CQDFILE  # Recreate this file if it was set in prev autodir." >> $doPreAutoStep3
  echo "fi" >> $doPreAutoStep3
  $pdsh_a mkdir $autoloc 2> /dev/null
  $pdcp_cmd $doPreAutoStep3 $autoloc/doPreAutoStep3

  $pdsh_a sh $autoloc/doPreAutoStep3 2> /dev/null


print "Starting at $time" > $LOG
if [[ $isOlder != "" ]]; then
  log "Removed progress directory older than 2 days and continuing."
fi
log "Created progress directories."

##########################################################################
#
# Input is passed in as: totalWindowTime*1000+<reorgPercentage>.
# Extract totalwindow time and reorg/updStats percentages.
#
# Passed windowTime is time in minutes to run all operations.
##########################################################################

# PreAuto: Step #1
if [[ $1 != "" ]]; then
   let totalWindowTime=$1/1000
   let reorgPercentage=$1-$totalWindowTime*1000

   # can either give an error if reorgpercentage is greater than 100
   # or round it down to 100. Right now, we will round it down. This
   # could be changed to an error, if that what we think is the right thing.
   if [ $reorgPercentage -gt 100 ]; then
     let reorgPercentage=100
   fi

   let ustatPercentage=100-$reorgPercentage
else
   let totalWindowTime=360
   let reorgPercentage=0
   let ustatPercentage=100
fi

let reorgWindowTime=$totalWindowTime*$reorgPercentage/100
let ustatWindowTime=$totalWindowTime*$ustatPercentage/100
if [ $totalWindowTime -eq 0 ]; then
  log "Total window time = 0, stopping."
  usasCleanup
  exit
fi

# Set the number of update statistics that will run in parallel at one time.
  # We let Linux deal with process priority. There is no need to
  # fetch CQD USTAT_AUTO_PRIORITY.

  # Since we do parallel update stats cross all SQ nodes, the total number of
  # jobs run in parallel is the sum of (number of cores per SQL node + 1) 
  # divided by #cores_per_node.

  if [ "$nodes" = "NSK" ]; then
    # Virtual node configuration. Set # of jobs in a parallel run to 1.
    let PARALLEL_RUNS=1
  else
    let PARALLEL_RUNS=0

    cores_per_job=8     # 8 cores per update stats job. 

    for cores_in_node in $cores; do
      let cores_in_node="($cores_in_node + 1) / $cores_per_job"
      let PARALLEL_RUNS="$PARALLEL_RUNS + $cores_in_node"
    done
  fi


# log total, reorg, ustat times and percentages based on user input.
log "totalWindowTime: $totalWindowTime minutes" "USAS"
log "ustatPercentage: $ustatPercentage%"        "USAS"
log "reorgPercentage: $reorgPercentage%"        "REORG"
log "ustatWindowTime: $ustatWindowTime minutes" "USAS"
log "reorgWindowTime: $reorgWindowTime minutes" "REORG"

#PreAuto Steps #4,5,6
#    Create a list of tables that may need to have histograms regenerated.
#    This list is the set of tables in USTAT_AUTO_TABLES that have either
#    empty histograms or histograms with a recent READ_TIME.   Catalog,
#    schema, and table names are separated by the '/' character since this is
#    not a valid character for delimited names.
MAX_READ_AGE=$(getCQD "USTAT_MAX_READ_AGE_IN_MIN")
# If unassigned, then there was an error, so use current default setting.
if [[ $(isANum $MAX_READ_AGE) = "FALSE" ]]; then
  log "Error obtaining USTAT_MAX_READ_AGE_IN_MIN, using default of 5760."
  MAX_READ_AGE=5760;
fi
log "USTAT_MAX_READ_AGE_IN_MIN=$MAX_READ_AGE"

if [[ $reorgWindowTime > 0 ]] || [[ $ustatWindowTime > 0 ]]; then
  log "Calling $FILL_AUTO_TBL."
  sh $FILL_AUTO_TBL $SYSTEM_NAME $MAX_READ_AGE > /dev/null
  
  log "Creating list of tables to update."
    query1="SELECT _UCS2'@USAS_AutoTblList@CAT_NAME@: ' || CAT_NAME || _UCS2' :@USAS_AutoTblList@SCH_NAME@: ' || SCH_NAME || _UCS2' :@USAS_AutoTblList@TBL_NAME@: ' || TBL_NAME || _UCS2' :@USAS_AutoTblList@ADDED_BY@: ' || TRANSLATE(ADDED_BY USING ISO88591TOUCS2) || _UCS2' :@USAS_AutoTblList@THE_END@' FROM $AutoTblList"
    query1="$query1 WHERE CAT_NAME<>_UCS2'' AND ADDED_BY <> _ISO88591'EXCLUD' ORDER BY LAST_RUN_GMT;"

    print "$query1" | $MXCI | grep '^@USAS_AutoTblList@CAT_NAME@: ' > $MXCIOUT

    # Put catalog, schema, and table names in separate files (and handle '\' character in names)
    cat $MXCIOUT | sed -e 's/^@USAS_AutoTblList@CAT_NAME@: \(.*\) :@USAS_AutoTblList@SCH_NAME@: .*$/\1/' | sed 's/\\/\\\\/g' > $CATLIST 
    cat $MXCIOUT | sed -e 's/^@USAS_AutoTblList@CAT_NAME@: .* :@USAS_AutoTblList@SCH_NAME@: \(.*\) :@USAS_AutoTblList@TBL_NAME@: .*$/\1/' | sed 's/\\/\\\\/g' > $SCHLIST 
    cat $MXCIOUT | sed -e 's/^@USAS_AutoTblList@CAT_NAME@: .* :@USAS_AutoTblList@SCH_NAME@: .* :@USAS_AutoTblList@TBL_NAME@: \(.*\) :@USAS_AutoTblList@ADDED_BY@: .*$/\1/' | sed 's/\\/\\\\/g' > $TBLLIST 
    cat $MXCIOUT | sed -e 's/^@USAS_AutoTblList@CAT_NAME@: .* :@USAS_AutoTblList@SCH_NAME@: .* :@USAS_AutoTblList@TBL_NAME@: .* :@USAS_AutoTblList@ADDED_BY@: [ ]*\([^ ][^ ]*\)[ ]* :@USAS_AutoTblList@THE_END@[ ]*$/\1/' > $OBJTYPE
      # The object type (table or MV) is in the ADDED_BY column of $AutoTblList.
  fi

# REORG: Step #1
# first, reorg all the tables which are to be 'update statistics'ed.
# This step will reorg only if needed, that is, if reorg has not already
# been done on the table.
# This step is needed as we want to reorg before update stats to improve
# updStats performance.
if [ $reorgWindowTime -gt 0 ]; then

  log "Started for tables in $AutoTable" "REORG"

  let cpu=$START_CPU
  let reorgElapsedTime=0

  ustatsReorgTotal=$(echo $(cat $TBLLIST | wc -l))

  if [ $ustatsReorgTotal -gt 0 ]; then
    exec 4<$CATLIST  # Read catalog     using file descriptor 4.
    exec 5<$SCHLIST  # Read schema      using file descriptor 5.
    exec 6<$TBLLIST  # Read table       using file descriptor 6.
    exec 7<$OBJTYPE  # Read object type using file descriptor 7.

    ustatsReorgNum=0
    while [[ $ustatsReorgNum -lt $ustatsReorgTotal ]] &&
          [[ $reorgElapsedTime -lt $reorgWindowTime ]] &&
          [[ ! -a $AUTODIR/STOP_AUTO_STATS ]]; do
      read cat     <&4;
      read sch     <&5;
      read tbl     <&6;
      read objtype <&7;
       

      # Convert names to external format.
      extcat=$(print "$cat" | sed 's/"/""/g')
      extsch=$(print "$sch" | sed 's/"/""/g')
      exttbl=$(print "$tbl" | sed 's/"/""/g')
      extName="\"$extcat\".\"$extsch\".\"$exttbl\""

      if [[ $objtype = "BT" ]]; then objtype="table"; fi
      maintainReorgQuery="maintain $objtype $extName, reorg, if needed, run;"

      #run maintain reorg
      let startTime=$(date "+%Y")*525600+10#$(date "+%j")*1440+10#$(date "+%H")*60+10#$(date "+%M")


        # Ignore priority now.  "nice -n -1 cmd" will not run 
        # if the user is not root.
        echo "$maintainReorgQuery" | $MXCI >> $REORGOUT


      let currentTime=$(date "+%Y")*525600+10#$(date "+%j")*1440+10#$(date "+%H")*60+10#$(date "+%M")
      let currReorgElapsedTime=$currentTime-$startTime
      log "Elapsed time for $extName: $currReorgElapsedTime minutes" "REORG"

      let reorgElapsedTime=$reorgElapsedTime+$currReorgElapsedTime

      let ustatsReorgNum=$ustatsReorgNum+1;
    done # end while
  fi

  # Manage time window
  let reorgWindowTime=$reorgWindowTime-$reorgElapsedTime;

  log "Completed for tables in $AutoTable" "REORG"
fi


# REORG: Step #2
# Now run maintain/reorg on non-upd-stats'ed tables.
# Do this if reorg window time allocated for it is greater than reorg time already used up.
# If this step runs in less than the max allocated time, add remainder of time to
# ustat's max window time.  Note that $reorgWindowTime is the reorg time remaining.
if [ $reorgWindowTime -gt 0 ]; then
  log "Started for other tables" "REORG"

  maintainReorgQuery="maintain database, reorg, if needed, run for maxruntime $reorgWindowTime minutes;"

  #run maintain reorg
  let startTime=$(date "+%Y")*525600+10#$(date "+%j")*1440+10#$(date "+%H")*60+10#$(date "+%M")



    # Ignore the priority for now
    echo "$maintainReorgQuery" | $MXCI >> $REORGOUT


  let currentTime=$(date "+%Y")*525600+10#$(date "+%j")*1440+10#$(date "+%H")*60+10#$(date "+%M")
  let reorgElapsedTime=$currentTime-$startTime
  log "Elapsed time: $reorgElapsedTime minutes" "REORG"
  log "Completed for other tables" "REORG"

  # REORG: Step #3
  # Manage time window - substract elapsed time from reorg window again, so it can 
  # be used to adjust ustat time.
  let reorgWindowTime=$reorgWindowTime-$reorgElapsedTime;
fi

# Adjust ustat window time based on remaining or deficit of time from reorg.
if [ $ustatPercentage -gt 0 ]; then
  let ustatWindowTime=$ustatWindowTime+$reorgWindowTime
  log "ustatWindowTime updated to $ustatWindowTime minutes" "USAS"
fi


#***************************************************************
# Insert USTAT_AUTOMATION_INTERVAL into SYSTEM_DEFAULTS table with
# non-zero value to turn on automation if USTAT_AUTO_TABLE has rows in it.
# This is done even if the ustat window time is 0 or less.
if [[ ! -a $CQDFILE ]]; then
  # Get # rows in USTAT_AUTO_TABLE
  query="SELECT COUNT(*) FROM $AutoTable where CAT_NAME!=_UCS2'' and "
  query="$query SCH_NAME!=_UCS2'' and TBL_NAME!=_UCS2'' "
  query="$query FOR READ UNCOMMITTED ACCESS;"

   rows=$(echo $query | $MXCI | getRow)

  if [[ $rows -ne 0 ]]; then
    # Set USTAT_AUTOMATION_INTERVAL to non-zero on all segments.
    log "Setting USTAT_AUTOMATION_INTERVAL CQD in SYSTEM_DEFAULTS table."
    log "Segments on this system: $segs"
    for segment in $segs; do
      query="INSERT INTO
             HP_SYSTEM_CATALOG.SYSTEM_DEFAULTS_SCHEMA.SYSTEM_DEFAULTS
             VALUES (default, _ISO88591'USTAT_AUTOMATION_INTERVAL', _ISO88591'1440', default);"
      errs=$(echo "$query" | $MXCI | grep "\*\*\* ERROR" | sed -n '1p')

      if [[ "$errs" != "" ]]; then
        # Get errors.
        errnum=$(echo "$errs" | cut -d" " -f2)  # Get rid of text
        errnum=${errnum#"ERROR["}             # Get rid of surrounding word and brackets.
        errnum=${errnum%"]"}
        errtext=${errs#"*** ERROR[$errnum]"}
        log "Ustat err on $segment: $errnum, $errtext"
      else
        log "Updated USTAT_AUTOMATION_INTERVAL on $segment."
      fi
        touch $CQDFILE
    done

    # Move this log information to permanently save it and restart log.
    mv $LOG $PREV/USTAT_AUTO_LOG_CQD 2> /dev/null
    time=$(date)
    print "Restarting log at $time" > $LOG
  else
    log "USTAT_AUTOMATION_INTERVAL not set/will not set (0 rows in USTAT_AUTO_TABLES)."
  fi
fi
#***************************************************************


# UPDATE STATS tasks: Step #6
maxSampleDays=$(getCQD "USTAT_MAX_SAMPLE_AGE")
if [[ $(isANum $maxSampleDays) = "FALSE" ]]; then maxSampleDays=10; fi 
  # Set to 10 days if cannot obtain from MX.
log "USTAT_MAX_SAMPLE_AGE=$maxSampleDays"

# Cleanup persistent sample tables.
cleanupPersSamples $maxSampleDays
# Cleanup sample tables.
cleanupSamples $maxSampleDays
# Cleanup old histograms.
removeOldHists

# if no time allocated for Update Stats, exit.
if [ $ustatWindowTime -le 0 ]; then
  log "Ustat window time = $ustatWindowTime, stopping."
  usasCleanup
  exit
fi

# UPDATE STATS tasks: Steps 7-8.
# A. Create USTAT_AUTO_TABLES if it doesn't exist:
#    If USTAT_AUTO_TABLES does not exist, create it and populate it with all tables
#    in the NEO catalog, setting the ADDED_BY column in every entry to 'SYSTEM'
#    (NOTE: This table is created by T0725, so this step is unnecessary now.)
# B. If all entries in USTAT_AUTO_TABLES have the ADDED_BY column set to 'SYSTEM',
#    find all tables in the NEO catalog and add any that are missing, setting the
#    ADDED_BY column to 'SYSTEM'.
# C. For each table in the list, run the following via the ODBC interface:
#       UPDATE STATISTICS FOR TABLE <table> ON NECESSARY COLUMNS;
# D. Run UPDATE STATISTICS at priority level $PRIORITY (only on NSK) via ODBC.  If possible, usas will
#    run UPDATE STATISTICS for different tables simultaneously on different CPUs.
#    Note: In the future, the use of ODBC ensures that these update statistics runs
#    are under the constraints of WMS (workload management system) and a priority
#    level will not be set.  In addition, this interface can run these commands in
#    parallel on multiple CPUs based on the WMS and service level defined for update
#    statistics.
ustatsTotal=$(echo $(cat $TBLLIST | wc -l))
ustatsStarted=0
ustatsRunning=0
ustatsFinished=0
cpu=$START_CPU


if [[ $ustatsTotal = 0 ]]; then
  log "No tables on which to perform update statistics.  Exiting."
  usasCleanup
  exit
fi

log "There are $numnodes storage nodes on this system."

log "Will update statistics on $ustatsTotal tables."
log "Window time (in minutes) = $ustatWindowTime."
log "MXCI=$MXCI"
log "AUTODIR=$AUTODIR"
log "Segments on this system: $segs"

exec 4<$CATLIST  # Read catalog     using file descriptor 4.
exec 5<$SCHLIST  # Read schema      using file descriptor 5.
exec 6<$TBLLIST  # Read table       using file descriptor 6.
exec 7<$OBJTYPE  # Read object type using file descriptor 7.

# We distribute |$nodes_to_run| many of update stats jobs over SQL nodes
# in the cluster. Wait until these jobs are done. If there are more jobs left, 
# we distribute another |$nodes_to_run| jobs. If the number of left-over is less
# than |$nodes_to_run|, we distribute them among the first n (= left-over jobs)
# SQL nodes.
#
# Here we set $nodes_to_run to a list of multiples of nodes in $nodes, where 
# each node in the sequence logically represents $cores_per_job cores to be 
# used per job. 

   let i=0

   if [ "$nodes" = "NSK" ]; then
     nodes_to_run="NSK"
   else
     for node in $nodes; do
     
        # Get the # of cores for the ith node. Since there is no way to 
        # initialize a shell array with a shell variable containing a list,
        # we do it the old way.
        let j=0;
        for core in $cores; do
          if [ $j -eq $i ]; then
            break;
          fi
          let j=$j+1;
        done
     
        # Convert $core to a sequence of SQL node numbers. Some of SQL node 
        # numbers can be repeated if they can take more than one job at a time.
        # The number of cores in these nodes should be multiple of 
        # $cores_per_job.
        #
        # For example, for the following configuration,
        #   nodes="n1 n2 n3 n4"
        #   cores="15 15 7 7"
        # we will assign sequence "n1 n1 n2 n2 n3 n4" to $nodes_to_run.
        #
        let core="($core + 1) / $cores_per_job"
        for ((k = 1; k <= $core; k++))  do
          nodes_to_run="$nodes_to_run $node"
        done
     
        let i=$i+1
     done
   fi
   
# echo "nodes_to_run=" $nodes_to_run
# echo "PARALLEL_RUNS=" $PARALLEL_RUNS
#

# In the while loop below, each iteration within IF will take one node off 
# the $nodes_to_run and run a job on it. We assign $nodes_to_run to
# $segs as it is one of the control variables in the loop.

  segs=$nodes_to_run

# Start USAS loop to run update statistics and wait for results.
# This loop will complete either when all tables from list have completed or
# the elapsed time matches the window time.
let startTime=$(date "+%Y")*525600+10#$(date "+%j")*1440+10#$(date "+%H")*60+10#$(date "+%M")
elapsedTime=0

    

while [[ $ustatsFinished != $ustatsTotal ]] &&
      [[ $elapsedTime -lt $ustatWindowTime ]] &&
      [[ ! -a $AUTODIR/STOP_AUTO_STATS ]]; do

  if [[ $ustatsStarted -lt $ustatsTotal ]] && [[ $ustatsRunning -lt $PARALLEL_RUNS ]] && [[ $cpu -le $END_CPU ]]  && [[ $segs != "" ]];
  then


    #
    # Run update statistics in parallel for up to 'PARALLEL_RUNS' jobs.
    # 'PARALLEL_RUNS' is set to the number of SQ nodes. So each
    # parallel run will run one command per SQ node. 
    #
    # Note: this code will be removed once the Workload Management System (WMS)
    #       supports update statistics.
    read cat     <&4;
    read sch     <&5;
    read tbl     <&6;
    read objtype <&7;

    extcat=$(print "$cat" | sed 's/"/""/g')
    extsch=$(print "$sch" | sed 's/"/""/g')
    exttbl=$(print "$tbl" | sed 's/"/""/g')
    extName="\"$extcat\".\"$extsch\".\"$exttbl\""

    seg=$(print $segs | cut -d" " -f1)
    log "Table $ustatsStarted: Seg=$seg, CPU=$cpu: $extName."

      # determine whether we want to invoke the ustat command locally or
      # remotely. $nodes = "NSK" when it is a SQ workstation.
      if [ "$nodes" = "NSK" -o "$seg" = "$this_node" ]; then
        pdsh_w=""
        $pdsh_w sh $RUN_LOG_USTAT "$cat" "$sch" "$tbl" "$objtype" $ustatsStarted > $AUTODIR/USTAT_scriptout_$ustatsRunning 2>&1 &
      else
        pdsh_w="pdsh -w $seg"
        # Must escape the '\', '`', and '"' characters for remote scripts
        cat=$(print "$cat" | sed 's/`/\\`/g' | sed 's/"/\\"/g')
        sch=$(print "$sch" | sed 's/`/\\`/g' | sed 's/"/\\"/g')
        tbl=$(print "$tbl" | sed 's/`/\\`/g' | sed 's/"/\\"/g')
        $pdsh_w sh $RUN_LOG_USTAT \"$cat\" \"$sch\" \"$tbl\" \"$objtype\" $ustatsStarted > $AUTODIR/USTAT_scriptout_$ustatsRunning 2>&1 &
      fi

      echo $pdsh_w sh $RUN_LOG_USTAT "$cat" "$sch" "$tbl" "$objtype" $ustatsStarted > $AUTODIR/USTAT_scriptout_$ustatsRunning > $AUTODIR/USTAT_scriptout_$ustatsRunning_$tbl.cmd &
  
    # Save pid of the update stats job.
    pid=$(echo $!)
    print $pid > $AUTODIR/USTAT_AUTO_PROGRESS/run${ustatsStarted}

    # Update CPU and segment.  Increment CPU through END_CPU, then go to next segment.
    if [[ $cpu = $END_CPU ]]; then
      cpu=$START_CPU
      segs=$(print $segs | cut -d" " -f2-)
    else
      let cpu=$cpu+1;
    fi

    let ustatsStarted=$ustatsStarted+1;  # Increment # of started ustats
    let ustatsRunning=$ustatsRunning+1;  #       and # of running ustats


    # Always increment cpu, so next cpu is tried for next ustats attempt.
  else
    # Check for completed update statistics commands.  Note that this method
    # waits for all runs to complete in a set before starting another set.
    # This mechanism will be removed once workload management is implemented
    # for ustats.  Wait for all started ustats to complete or the window time
    # to expire.
    log "ustatsRunning=$ustatsRunning"

    # Wait for spawned processes to complete.
    wait

    let currentTime=$(date "+%Y")*525600+10#$(date "+%j")*1440+10#$(date "+%H")*60+10#$(date "+%M")
    let elapsedTime=$currentTime-$startTime
    log "All ustat cmds for this set completed. Elapsed time $elapsedTime minutes."

    # Remove files indicating completion (run*)

      $pdsh_a rm $AUTODIR/USTAT_AUTO_PROGRESS/run* 2>/dev/null

    # Update the count of already finished ustat cmds.
    let ustatsFinished=$ustatsFinished+$ustatsRunning;
    log "ustatsFinished=$ustatsFinished"

    cpu=$START_CPU;       # Start next update statistics cmd on starting CPU.

    ustatsRunning=0;      # Reset number of running ustat cmds.

      # The first iteration should assign # of storage nodes worth of
      # updatestats commands to these nodes. If there are more commands, we will
      # make another iteration and so on. All jobs in each iteration are done
      # in parallel.
      segs=$nodes_to_run

  fi # end if/else.
done # end while

exec 4<&- # Close file descriptor 4

if [[ $elapsedTime -ge $ustatWindowTime ]]; then
  log "Stopping because elapsed time has exceeded window of $ustatWindowTime."
fi
if [[ -a $AUTODIR/STOP_AUTO_STATS ]]; then
  log "Stopping because requested by STOP_AUTOMATED_STATS()."
fi


# Drop progress directory.
log "All ustats commands have completed. Dropping progress directories and scripts."
usasCleanup
