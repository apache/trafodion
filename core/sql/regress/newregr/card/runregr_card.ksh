-- @@@ START COPYRIGHT @@@
--
-- Licensed to the Apache Software Foundation (ASF) under one
-- or more contributor license agreements.  See the NOTICE file
-- distributed with this work for additional information
-- regarding copyright ownership.  The ASF licenses this file
-- to you under the Apache License, Version 2.0 (the
-- "License"); you may not use this file except in compliance
-- with the License.  You may obtain a copy of the License at
--
--   http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing,
-- software distributed under the License is distributed on an
-- "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
-- KIND, either express or implied.  See the License for the
-- specific language governing permissions and limitations
-- under the License.
--
-- @@@ END COPYRIGHT @@@
#! /bin/sh
################################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2003-2007 Hewlett-Packard Development Company, L.P.
#
# @@@ END COPYRIGHT @@@
################################################################################

################################################################################
#              CARDINALITY REGRESSION TEST SUITE                  
#                                                                              
#  This script automates the running of tests which verify cardinalities.      
#  It is based on the runqat script which automates the OPT tests.  
#                                                                              
################################################################################
 
#  The command set -x, makes the script echo everything.  Useful for debugging
# set -x

#------------------------------------------------------
#  User asked for help text (-h, -help or -? option) --
# print it out and exit.                             --
#------------------------------------------------------
if [ "$1" = "-h" -o "$1" = "-help" -o "$1" = "-?" ]; then
  cat << END_HELP_TEXT

 Usage:
 $0                  -- run everything (ddl, dml) 
    [-help] [-h] [-?]    -- this screen
    [-cdb] [-createdb]   -- create all databases
    [-dml] [-rundml]     -- run all dml queries

END_HELP_TEXT
  exit 0
fi

#-----------------------------------------------------------------
#  Each command line option has an associated boolean variable  --
# indicating whether or not the option is in effect; initialize --
# these booleans to false.                                      --
#-----------------------------------------------------------------
createdb=0
rundml=0
#-----------------------------------------------------------------------
#  Initialize global variables (MOSTLY DONE IN ../tools/runregr !!)
#
#    sqlci        : full path of sqlci executable file                --
#    OPTSCRIPTDIR : full path of optimizer regression test directory  --
#    OPTRUNDIR    : full path of current directory to run this script --
#    LOGSORT      : full path of log file sorting program             --
#    FILTER       : full path of shell script for filtering log files --
#    NULL         : null output file                                  --
#    TEMP, TMP    : full path of temp directory                       --
#-----------------------------------------------------------------------

OPTSCRIPTDIR=$REGRTSTDIR
OPTRUNDIR=$REGRRUNDIR

#--------------------------------------------------------------
#  Determine which command line arguments the user specified --
# and set the corresponding boolean variable to TRUE.        --
#--------------------------------------------------------------
OK=-1
while [ $OK -ne 0 ]; do		

  if [ $OK -gt 0 ]; then
    shift $OK
  fi
  OK=0

  if [ "$1" = "-createdb" -o "$1" = "-cdb" ]; then
    createdb=1;
    OK=1
  fi

  if [ "$1" = "-rundml" -o "$1" = "-dml" ]; then
    rundml=1;
    OK=1
  fi

done

#########################################################################
# E N D   O F   P A R S I N G   C O M M A N D   L I N E   O P T I O N S #
#########################################################################

if [ "$REGRCONCURRENT" -eq 1 ]; then 
  test_suite=${REGRBASDIR:?'REGRBASDIR is undefined'}
  export TEST_CATALOG='cat'
  export TEST_SCHEMA_NAME="$test_suite"
else
  export TEST_CATALOG='cat'
  export TEST_SCHEMA_NAME='sch'
fi
export TEST_SCHEMA="$TEST_CATALOG.$TEST_SCHEMA_NAME"

#--------------------------------------------------
#  Based on the command lines options specified, --
# determine which test files to run.             --
#--------------------------------------------------
testfiles=

#---------------------------------------
#  User wants to create the database, --
# so run the ddl tests.               --
#---------------------------------------
if [ $createdb -eq 1 ]; then
  testfiles="testddl??"
fi

#---------------------------------
#  User wants to run dml tests. --
#---------------------------------
if [ $rundml -eq 1 ]; then
  testfiles="$testfiles testdml??"
fi

#------------------------------------------------
#  Force any wildcard expansion of test files. --
#------------------------------------------------
testfiles=`echo $testfiles $*`

#----------------------------------------------------------
#  No test files specified, so by default, run them all. --
#----------------------------------------------------------
if [ "$testfiles" = "" ]; then
  testfiles="testddl?? testdml??"
fi


#-----------------------------------------
#  Inform user which tests will be run. --
#-----------------------------------------
echo "Tests:\n  $testfiles\n"

#--------------------------------------------------
# rename the old log file to have a .bak suffix. --
# Write date and time to new log file.           --
#--------------------------------------------------
mv -f $rgrlog $rgrlog.bak 2>$NULL
echo "`date +'%D %T'`" >> $rgrlog

#---------------------------------------
#  For each test file in the list.... --
#---------------------------------------
for i in $testfiles; do

  #---------------------------------------
  #  Tell user whether we're just doing --
  # diffs or actually running a test.   --
  #---------------------------------------
  echo
    echo "Running test $i"

  #---------------------------------------------------------
  #  Set up the test file, expected file, output file and --
  # difference file names for this iteration.  Also clear --
  # out the log text for the current iteration.           --
  #---------------------------------------------------------
  tfile=$OPTSCRIPTDIR/$i
  tfileshp=$OPTRUNDIR/$i.shp
  efileshp=$OPTRUNDIR/E$i.shp
  convertToUpper=`echo $i | tr a-z A-Z`
  afile=$OPTRUNDIR/A$convertToUpper
  dfile=$OPTRUNDIR/D$i
  logtxt=

#  if [ $nsk -eq 0 ]; then
    efile=$OPTSCRIPTDIR/E$i
#  else
#    efile=$OPTSCRIPTDIR/E$i.nsk
#  fi

  # If this test is executing concurrently with other tests, use
  # the parallel execution expected results file if one exists.
  if [ "$REGRCONCURRENT" -eq 1 ]; then
    if [ -r "${efile}-P" ]; then
      efile="${efile}-P"
    fi
  fi

#-------------------------------------------
# No special options.  Just run the test. --
#-------------------------------------------
if [ "$REGRCONCURRENT" -eq 1 ]; then
    echo "create schema ${TEST_SCHEMA}; set schema ${TEST_SCHEMA};" \
          | cat cidefs - $tfile > $tfile.tmp
  else
       cat cidefs $tfile > $tfile.tmp
fi

 echo "$sqlci -i$tfile -l"
 $sqlci -i$tfile.tmp -l
  rm $tfile.tmp

  #-------------------------------
  #  Sort expected result file. --
  #-------------------------------
  rm -f $efile.srt 2>$NULL
  if [ $runshape -eq 1 ]; then
    echo "$LOGSORT $efileshp $efile.srt -i >> $NULL"
    $LOGSORT $efile $efile.srt -i >> $NULL
  else
    echo "$LOGSORT $efile $efile.srt -i >> $NULL"
    $LOGSORT $efile $efile.srt -i >> $NULL
  fi

  #-----------------------------
  #  Sort actual result file. --
  #-----------------------------
  rm -f $afile.srt 2>$NULL
  echo "$LOGSORT $afile $afile.srt -i >> $NULL"
  $LOGSORT $afile $afile.srt -i >> $NULL

  #-------------------------------------------------
  #  Filter the sorted expected and actual files. --
  #-------------------------------------------------
  rm -f $dfile $efile.flt $afile.flt
  $FILTER $efile.srt > $efile.flt 2>&1
  $FILTER $afile.srt > $afile.flt 2>&1
  #----------------------------------------------------------------
  #  Capture filtered files                              --
  #----------------------------------------------------------------
      echo "cp $afile.flt $FilteredLogs/logfiles/$(basename $afile)"
      cp $afile.flt $FilteredLogs/logfiles/$(basename $afile)
      echo "cp $efile.flt $FilteredLogs/expfiles/$(basename $afile)"
      cp $efile.flt $FilteredLogs/expfiles/$(basename $afile)
  #----------------------------------------------------------------
  #  Compare filtered expected and actual files for differences. --
  #----------------------------------------------------------------
  echo "diff $efile $afile >> $dfile"
  diff $efile.flt $afile.flt >> $dfile 2>&1

  #  Leave filter files around for now.
  #rm -f $efile.flt $afile.flt

  #------------------------------------------
  #  Determine how many lines differed     --
  # between the expected and actual files. --
  #------------------------------------------
  diffsize=`wc -l $dfile`; diffsize=`echo $diffsize | cut -f1 -d' '`

  #----------------------------------------
  #  Diff the diff and the diff.known    --
  # files and See how many lines differ. --
  #----------------------------------------
  rm -f $dfile.dif
  # Filter known diff file to avoid schema differences
  knownfiltered="$(basename $dfile.known).flt"
  $FILTER $dfile.known > $knownfiltered 2>&1 
  diff $dfile $knownfiltered >> $dfile.dif 2>&1
  diffofdiffsize=`wc -l $dfile.dif`; diffofdiffsize=`echo $diffofdiffsize | cut -f1 -d' '`
  rm -f $dfile.dif

  #--------------------------
  #  Store result of test. --
  #--------------------------
  if [ $diffsize -eq 0 -o $diffofdiffsize -eq 0 ]; then
    if [ $diffsize -eq 0 ]; then
      logtxt="### PASS ###"
    else
      logtxt="### PASS with known diffs ###"
    fi
    rm -f $dfile
  else
    logtxt="### FAIL ($diffsize lines) ###"
  fi 

  #-------------------------------------------------------------
  #  Print date, time, test name and test result in log file. --
  #-------------------------------------------------------------
  echo $logtxt
  echo "`date +'%D %T'`	$tfile  $logtxt" >> $rgrlog

done

#----------------------------------------
#  Print log file to screen and do any --
# post processing if any is specified. --
#----------------------------------------

echo
echo
echo 'Test Summary'
echo '============'
echo
cat $rgrlog
echo
test "$post" != "" && $post

