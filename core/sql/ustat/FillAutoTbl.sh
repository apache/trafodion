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
# 1. Get row count of USTAT_AUTO_TABLES for 'USER' entries
# 2. If row count = 0, then repopulate with all tables in NEO catalog.

# Set up: ------------------------------------------------------------


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
   MXCI=$TRAF_HOME/export/bin32/sqlci
   autoloc=$TRAF_HOME/export/lib/mx_ustat
   alias print=echo
   sys=NSK
fi

MAX_READ_AGE=$2 # Max time since hist was read that it will be retrieved

tableCat="NEO"
autoCat="MANAGEABILITY"
autoSch="HP_USTAT"
autoTable="${autoCat}.${autoSch}.USTAT_AUTO_TABLES"
AUTODIR="$autoloc/autodir"
query10="${AUTODIR}/query10"
query12_13="${AUTODIR}/query12_13"
filllog="${AUTODIR}/USTAT_FILLLOG"
print "FillAutoTbl: MXCI=$MXCI" > $filllog
print "FillAutoTbl: autoloc=$autoloc" >> $filllog

# Functions: ---------------------------------------------------------
checkForErrs()
{
  errsOutput=$1
  errnum=0
  errtext=""
  if [[ "$errsOutput" != "" ]]; then # Errors
    # Get errors.
    let errcnt=$errcnt+1
    errnum=$(print "$errsOutput" | cut -d" " -f2)  # Get rid of text
    errnum=${errnum#"ERROR["}             # Get rid of surrounding word and brackets.
    errnum=${errnum%"]"}
    errtext=${errsOutput#"*** ERROR[$errnum]"}
    print "FillAutoTbl: Update auto table err: $errnum, $errtext" >> $filllog
    return 1
  fi
  return 0
}

# return the number of row field SQL cout output.
# The value should be contained in the 7th row. Valid only for SQ
alias getRow="sed '1,6 d' | sed '2,6 d' | sed '1,\$s/ //g'"

# Main code: ---------------------------------------------------------

query="SELECT COUNT(*) FROM $autoTable FOR READ UNCOMMITTED ACCESS;"
let rows=$(echo $query | $MXCI | getRow)

# 1. Get row count of USTAT_AUTO_TABLES for 'USER' entries
query="SELECT COUNT(*) FROM $autoTable WHERE ADDED_BY=_ISO88591'USER' FOR READ UNCOMMITTED ACCESS;"
let userRows=$(echo $query | $MXCI | getRow)

print "FillAutoTbl: Starting to search for tables." >> $filllog
print "FillAutoTbl: USTAT_AUTO_TABLE current sysRows=$rows, userRows=$userRows." >> $filllog

# Create timestamp to compare READ_TIME to.
currentTime=$(date +"%Y-%m-%d %H:%M:%S")
# Find maximum age in seconds for histogram read.  Allow 10^6 minutes max (about 2 yrs).
if [[ $MAX_READ_AGE < 1000000 ]]; then let MAX_READ_SECS=$MAX_READ_AGE*60
else  MAX_READ_SECS="345600"; fi # default of 4 days.

print "FillAutoTbl: Current time is $currentTime." >> $filllog
print "FillAutoTbl: Max age of READ_TIME = $MAX_READ_SECS (secs)." >> $filllog

# Create list of schema versions.
print "FillAutoTbl: Obtaining schema versions >= 2300." >> $filllog
query1="SELECT DISTINCT 'Version:', S.SCHEMA_VERSION
         FROM HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.CATSYS C,
              HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.SCHEMATA S
         WHERE C.CAT_UID=S.CAT_UID AND
               C.CAT_NAME=_UCS2'NEO' AND
               S.SCHEMA_VERSION >= 2300;"

print "$query1" | $MXCI >$AUTODIR/tmp
errs=$(cat $AUTODIR/tmp | grep "\*\*\* ERROR" | sed -n '1p')
checkForErrs "$errs"

versions=$(cat $AUTODIR/tmp | grep "Version:" | grep -v "SELECT" |\
           tr -s ' ' ' ' | cut -d" " -f2)
print "FillAutoTbl: Schema versions found: $versions" >> $filllog

errcnt=0
# When automation becomes default, replace following line with:
# if [[ $userRows = 0 ]]; then
if [[ $rows -ne 0 && $userRows = 0 ]]; then
  # USTAT AUTOMATION DYNAMIC TABLE LIST
  print "FillAutoTbl: USTAT_AUTO_TABLES is dynamic - recreating." >> $filllog

  if checkForErrs "$errs"; then for ver in $versions; do
    # No errors getting versions.  Loop for each version.
    # 1. Update USTAT_AUTO_TABLES

    print "FillAutoTbl: Updating tables in USTAT_AUTO_TABLE for vers=$ver" >> $filllog
    # -- Add tables and MVs in NEO catalog that are not in list. ------------------------
    query2="INSERT INTO $autoTable
        SELECT C.CAT_NAME, S.SCHEMA_NAME, O.OBJECT_NAME,
               TIMESTAMP '0001-01-01 00:00:00',
               TIMESTAMP '0001-01-01 00:00:00',
               0, _UCS2'', _ISO88591'SYSTEM'
          FROM HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.CATSYS C,
               HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.SCHEMATA S,
               NEO.HP_DEFINITION_SCHEMA.OBJECTS O
          WHERE C.CAT_UID=S.CAT_UID AND
               S.SCHEMA_UID=O.SCHEMA_UID AND
              (O.OBJECT_TYPE=_ISO88591'BT' OR O.OBJECT_TYPE=_ISO88591'MV') AND
               O.OBJECT_NAME_SPACE=_ISO88591'TA' AND
               C.CAT_NAME=_UCS2'NEO' AND
               S.SCHEMA_NAME<>_UCS2'HP_DEFINITION_SCHEMA' AND
               S.SCHEMA_NAME<>_UCS2'PUBLIC_ACCESS_SCHEMA' AND
               S.SCHEMA_NAME NOT LIKE _UCS2'HP\_%' ESCAPE _UCS2'\' AND
               S.SCHEMA_NAME NOT LIKE _UCS2'VOLATILE\_SCHEMA\_%' ESCAPE _UCS2'\' AND
               O.OBJECT_NAME<>_UCS2'HISTOGRAMS' AND
               O.OBJECT_NAME<>_UCS2'HISTOGRAM_INTERVALS' AND
               O.OBJECT_NAME<>_UCS2'HISTOGRAMS_FREQ_VALS' AND
               O.OBJECT_NAME<>_UCS2'MVS_TABLE_INFO_UMD' AND
               O.OBJECT_NAME<>_UCS2'MVS_UMD' AND
               O.OBJECT_NAME<>_UCS2'MVS_USED_UMD' AND
                (C.CAT_NAME, S.SCHEMA_NAME, O.OBJECT_NAME) NOT IN
                  (SELECT CAT_NAME, SCH_NAME, TBL_NAME FROM $autoTable);"

    # -- Delete tables and MVs that no longer exist in NEO catalog from list. -----------
    query3="DELETE FROM $autoTable WHERE ADDED_BY<>_ISO88591'EXCLUD' AND
       (CAT_NAME, SCH_NAME, TBL_NAME) NOT IN
       (SELECT C.CAT_NAME, S.SCHEMA_NAME, O.OBJECT_NAME
          FROM HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.CATSYS C,
               HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.SCHEMATA S,
               NEO.HP_DEFINITION_SCHEMA.OBJECTS O
          WHERE C.CAT_UID=S.CAT_UID AND
                S.SCHEMA_UID=O.SCHEMA_UID AND
              (O.OBJECT_TYPE=_ISO88591'BT' OR O.OBJECT_TYPE=_ISO88591'MV') AND
               O.OBJECT_NAME_SPACE=_ISO88591'TA' AND
               C.CAT_NAME=_UCS2'NEO' AND
               S.SCHEMA_NAME<>_UCS2'HP_DEFINITION_SCHEMA' AND
               S.SCHEMA_NAME<>_UCS2'PUBLIC_ACCESS_SCHEMA' AND
               S.SCHEMA_NAME NOT LIKE _UCS2'HP\_%' ESCAPE _UCS2'\' AND
               S.SCHEMA_NAME NOT LIKE _UCS2'VOLATILE\_SCHEMA\_%' ESCAPE _UCS2'\' AND
               O.OBJECT_NAME<>_UCS2'HISTOGRAMS' AND
               O.OBJECT_NAME<>_UCS2'HISTOGRAM_INTERVALS' AND
               O.OBJECT_NAME<>_UCS2'HISTOGRAMS_FREQ_VALS' AND
               O.OBJECT_NAME<>_UCS2'MVS_TABLE_INFO_UMD' AND
               O.OBJECT_NAME<>_UCS2'MVS_UMD' AND
               O.OBJECT_NAME<>_UCS2'MVS_USED_UMD');"

    errs=$(print "$query2 $query3" | $MXCI | grep "\*\*\* ERROR" | sed -n '1p')
    checkForErrs "$errs"
  done
  fi

  # Update 'rows' variable to new rowcount.
  query="SELECT COUNT(*) FROM $autoTable FOR READ UNCOMMITTED ACCESS;"
  print "$query" | $MXCI >$AUTODIR/tmp
  errs=$(cat $AUTODIR/tmp | grep "\*\*\* ERROR" | sed -n '1p')
  checkForErrs "$errs"
  let rows=$(cat $AUTODIR/tmp | getRow)
fi


if [[ $rows -ne 0 ]]; then
  # -- Get schemas from USTAT_AUTO_TABLES ------------------------------------------------
  query4="SELECT DISTINCT 'Schema:', SCH_NAME FROM $autoTable 
             WHERE SCH_NAME NOT LIKE _UCS2'@%' AND
                   SCH_NAME<>_UCS2'';"
  echo "$query4" | $MXCI >$AUTODIR/tmp
  errs=$(cat $AUTODIR/tmp | grep "\*\*\* ERROR" | sed -n '1p')
  # Create list of schemas.
  cat $AUTODIR/tmp | grep "Schema:" | grep -v "SELECT" |\
             tr -s ' ' ' ' | cut -d" " -f2- > $AUTODIR/USTAT_SCHEMAS

  # -- Find all tables with empty histograms for schemas found. --------------------------

  if checkForErrs "$errs"; then
    # Drop temp tables, but don't check for errors.
    query5="DROP TABLE NEO.PUBLIC_ACCESS_SCHEMA.USTAT_MISSING_STATS_TBLS;"
    query6="DROP TABLE NEO.PUBLIC_ACCESS_SCHEMA.USTAT_RECENT_HISTS_TBLS;"
    query7="DROP TABLE NEO.PUBLIC_ACCESS_SCHEMA.USTAT_AUTO_RUN_LIST;"
    print "$query5 $query6 $query7" | $MXCI > /dev/null

    query5="CREATE TABLE NEO.PUBLIC_ACCESS_SCHEMA.USTAT_MISSING_STATS_TBLS LIKE $autoTable;"
    query6="CREATE TABLE NEO.PUBLIC_ACCESS_SCHEMA.USTAT_RECENT_HISTS_TBLS  LIKE $autoTable;"
    query7="CREATE TABLE NEO.PUBLIC_ACCESS_SCHEMA.USTAT_AUTO_RUN_LIST      LIKE $autoTable;"
    errs=$(print "$query5 $query6 $query7" | $MXCI | grep "\*\*\* ERROR" | sed -n '1p')

    if checkForErrs "$errs"; then
      print "FillAutoTbl: Finding tables w/ empty or recently read histograms." >> $filllog
      rm -f $AUTODIR/findTblsWHistsToUpdate
      touch $AUTODIR/findTblsWHistsToUpdate # Create empty file.
      exec 4<$AUTODIR/USTAT_SCHEMAS
      read schema <&4;  # Get first schema name.

      while [[ "$schema" != "" ]]; do
        # Find all tables/MVs with empty histograms for a given schema.  Note that the 
        # ADDED_BY column is used here only to store the object type, which should be 
        # only BT (table) or MV.
        extsch=$(print "$schema" | sed 's/"/""/g')
        print "FillAutoTbl: Creating query for schema \"$extsch\"." >> $filllog
        intschstrlit=$(print "$schema" | sed "s/'/''/g")
        query8="INSERT INTO NEO.PUBLIC_ACCESS_SCHEMA.USTAT_MISSING_STATS_TBLS
          SELECT DISTINCT _UCS2'NEO', _UCS2'$intschstrlit', O.OBJECT_NAME,
                TIMESTAMP '0001-01-01 00:00:00',
                TIMESTAMP '0001-01-01 00:00:00',
                0, _UCS2'', O.OBJECT_TYPE
            FROM   NEO.HP_DEFINITION_SCHEMA.OBJECTS O
                 , NEO.HP_DEFINITION_SCHEMA.COLS C
                 , NEO.\"$extsch\".HISTOGRAMS H
            WHERE  O.OBJECT_UID = H.TABLE_UID
               AND O.OBJECT_UID = C.OBJECT_UID
               AND O.OBJECT_NAME_SPACE = 'TA'
               AND C.COLUMN_NUMBER = H.COLUMN_NUMBER
               AND H.REASON = _ISO88591' '
               AND O.OBJECT_NAME<>_UCS2'HISTOGRAMS'
               AND O.OBJECT_NAME<>_UCS2'HISTOGRAM_INTERVALS'
               AND O.OBJECT_NAME<>_UCS2'HISTOGRAMS_FREQ_VALS'
               AND O.OBJECT_NAME<>_UCS2'MVS_TABLE_INFO_UMD'
               AND O.OBJECT_NAME<>_UCS2'MVS_UMD'
               AND O.OBJECT_NAME<>_UCS2'MVS_USED_UMD'
            for read uncommitted access;"

        # Find all tables/MVs with READ_TIME in recent past.  Note that the ADDED_BY
        # column is used here only to store the object type, which should be only 
        # BT (table) or MV.
        query9="INSERT INTO NEO.PUBLIC_ACCESS_SCHEMA.USTAT_RECENT_HISTS_TBLS
          SELECT DISTINCT _UCS2'NEO', _UCS2'$intschstrlit', O.OBJECT_NAME,
                TIMESTAMP '0001-01-01 00:00:00',
                TIMESTAMP '0001-01-01 00:00:00',
                0, _UCS2'', O.OBJECT_TYPE
            FROM   NEO.HP_DEFINITION_SCHEMA.OBJECTS O
                 , NEO.HP_DEFINITION_SCHEMA.COLS C
                 , NEO.\"$extsch\".HISTOGRAMS H
            WHERE  O.OBJECT_UID = H.TABLE_UID
               AND O.OBJECT_UID = C.OBJECT_UID
               AND O.OBJECT_NAME_SPACE = 'TA'
               AND C.COLUMN_NUMBER = H.COLUMN_NUMBER
               AND O.OBJECT_NAME<>_UCS2'HISTOGRAMS'
               AND O.OBJECT_NAME<>_UCS2'HISTOGRAM_INTERVALS'
               AND O.OBJECT_NAME<>_UCS2'HISTOGRAMS_FREQ_VALS'
               AND O.OBJECT_NAME<>_UCS2'MVS_TABLE_INFO_UMD'
               AND O.OBJECT_NAME<>_UCS2'MVS_UMD'
               AND O.OBJECT_NAME<>_UCS2'MVS_USED_UMD'
               AND TIMESTAMP '$currentTime' - H.READ_TIME <
                   INTERVAL '$MAX_READ_SECS' SECOND(12,0)
            for read uncommitted access;"
        print "$query8 $query9" >> $AUTODIR/findTblsWHistsToUpdate
        # Get next schema name and version.
        read schema <&4;
      done
      # Run MXCI for all queries to find objects.
      print "FillAutoTbl: Searching schemas to fill USTAT_MISSING/RECENT_TBLS." >> $filllog
      errs=$($MXCI -i $AUTODIR/findTblsWHistsToUpdate | grep "\*\*\* ERROR" | sed -n '1p')
      checkForErrs "$errs"

      print "FillAutoTbl: List of tables with empty or recently read histograms:" >> $filllog
      print "FillAutoTbl: Note: Recently read hists may not be obsolete." >> $filllog
      queryP1="select _ISO88591'Empty hist: ',
                      SUBSTRING(SCH_NAME, 1, 30) || _UCS2' ' || SUBSTRING(TBL_NAME, 1, 30)
             from NEO.PUBLIC_ACCESS_SCHEMA.USTAT_MISSING_STATS_TBLS;"
      queryP2="select _ISO88591'Recent hist:',
                      SUBSTRING(SCH_NAME, 1, 30) || _UCS2' ' || SUBSTRING(TBL_NAME, 1, 30)
             from NEO.PUBLIC_ACCESS_SCHEMA.USTAT_RECENT_HISTS_TBLS;"
      print "              Schema                         Table" >> $filllog
      print "=================================================================" >> $filllog
      print "${queryP1}${queryP2}" | $MXCI | grep " hist:" | grep -v ">>select" >> $filllog

      # Fill AUTO_RUN_LIST with objects that are in both USTAT_AUTO_TABLES and
      # MISSING_STATS_TBLS or RECENT_HISTS_TBLS. Objects for which ADDED_BY = 
      # 'EXCLUD' in USTAT_AUTO_TABLES are not inserted to AUTO_RUN_LIST.  Instead,
      # the ADDED_BY column of MISSING_STATS_TBLS and RECENT_HISTS_TBLS is prop-
      # agated to AUTO_RUN_LIST which here stores the object type and is used by 
      # USAS.sh to issue correct MAINTAIN syntax.
      print "FillAutoTbl: Updating USTAT_AUTO_RUN_LIST with list of tables to update." >> $filllog

      # Send the query to file query10 first. This is to avoid * expansion
      # if the entire query is assigned to a shell variable.
      echo "INSERT INTO NEO.PUBLIC_ACCESS_SCHEMA.USTAT_AUTO_RUN_LIST
          SELECT DISTINCT M.CAT_NAME, M.SCH_NAME, M.TBL_NAME, 
                          M.LAST_RUN_GMT, M.LAST_RUN_LCT, M.RUN_STATUS,
                          M.ERROR_TEXT, T.ADDED_BY FROM $autoTable M,
                (SELECT * FROM NEO.PUBLIC_ACCESS_SCHEMA.USTAT_MISSING_STATS_TBLS
                 UNION
                 SELECT * FROM NEO.PUBLIC_ACCESS_SCHEMA.USTAT_RECENT_HISTS_TBLS) T
          WHERE (M.CAT_NAME=T.CAT_NAME AND M.SCH_NAME=T.SCH_NAME AND M.TBL_NAME=T.TBL_NAME
             AND M.ADDED_BY <> _ISO88591'EXCLUD')
          for read uncommitted access;" > $query10

      errs=$($MXCI -i $query10 | grep "\*\*\* ERROR" | sed -n '1p')
      checkForErrs "$errs"
    else
      print "FillAutoTbl: ERROR: Couldn't create USTAT_MISSING_STATS_TBLS, ..." >> $filllog
    fi
  else
    print "FillAutoTbl: ERROR: Couldn't obtain schemas from USTAT_AUTO_TABLES." >> $filllog
  fi
  if [[ $errcnt != 0 ]]; then
    print "FillAutoTbl: Because of errors, attempting to copy USTAT_AUTO_TABLES" >> $filllog
    print "             into USTAT_AUTO_RUN_LIST" >> $filllog
    errcnt=0   # reset error count
    query11="DROP TABLE NEO.PUBLIC_ACCESS_SCHEMA.USTAT_AUTO_RUN_LIST;"
    print "$query11" | $MXCI > /dev/null
    query12="CREATE TABLE NEO.PUBLIC_ACCESS_SCHEMA.USTAT_AUTO_RUN_LIST
             LIKE $autoTable;"
    query13="INSERT INTO NEO.PUBLIC_ACCESS_SCHEMA.USTAT_AUTO_RUN_LIST 
             SELECT CAT_NAME, SCH_NAME, TBL_NAME, 
                    LAST_RUN_GMT, LAST_RUN_LCT, RUN_STATUS,
                    ERROR_TEXT, 'BT' FROM $autoTable
                    WHERE ADDED_BY <> _ISO88591'EXCLUD';"
    print "$query12 $query13" | $MXCI >$AUTODIR/tmp
    errs=$(cat $AUTODIR/tmp | grep "\*\*\* ERROR" | sed -n '1p')
    checkForErrs "$errs"
  fi
else
  print "FillAutoTbl: There are no entries in USTAT_AUTO_TABLES." >> $filllog

  # Drop temp tables, but don't check for errors.
  query14="DROP TABLE NEO.PUBLIC_ACCESS_SCHEMA.USTAT_MISSING_STATS_TBLS;"
  query15="DROP TABLE NEO.PUBLIC_ACCESS_SCHEMA.USTAT_RECENT_HISTS_TBLS;"
  query16="DROP TABLE NEO.PUBLIC_ACCESS_SCHEMA.USTAT_AUTO_RUN_LIST;"
  print "$query14 $query15 $query16" | $MXCI > /dev/null  
fi

if [[ $errcnt != 0 ]]; then
  # An error occurred.
  print "FillAutoTbl: An error occurred while creating list of tables to run." >> $filllog
fi
print "FillAutoTbl: Processing completed." >> $filllog
