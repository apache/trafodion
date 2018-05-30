#######################################################################
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
#######################################################################


################################################################################
#                     PRIVS1 REGRESSION TEST SUITE                             #
#                                                                              #
# This script automates the running of privilege security tests                #
#                                                                              #
################################################################################

#! /bin/sh

#  The command set -x, makes the script echo everything.  Useful for debugging
#set -x

#------------------------------------------------------
#  User asked for help text (-h, -help or -? option) --
# print it out and exit.                             --
#------------------------------------------------------
function usage {
  echo ""
  echo "Description: executes privilege tests"
  echo ""
  echo "  [-help | -h | -?]    -- this text "
  echo "  [-dircleanup]        -- cleanup directory "
  echo "  [-diff]              -- do not run, only do diffs "
  echo "  [<priv tests>]       -- run the specified tests "
  echo ""
  echo "If no options specified, run all tests and perform diffs"
  echo " " 
}

#-----------------------------------------------------------------
#  Each command line option has an associated boolean variable  --
# indicating whether or not the option is in effect.  Initalize --
# these booleans to false.                                      --
#-----------------------------------------------------------------
diffsonly=0
dircleanup=0

echo ""
echo "RUNNING PRIVS1 TESTS"
echo ""
#--------------------------------------------------------------
#  Determine which command line arguments the user specified --
# and set the corresponding boolean variable to TRUE.        --
#--------------------------------------------------------------
OK=-1
while [ $OK -ne 0 ]; do		# loop to allow options to appear in any order

  if [ $OK -gt 0 ]; then
    shift $OK
  fi
  OK=0

  if [ "$1" = "-dircleanup" ]; then
    dircleanup=1
    OK=1
  fi

  if [ "$1" = "-diff" ]; then
    diffsonly=1
    OK=1
  fi

  if [ "$1" = "-help" -o "$1" =  "-h" ]; then
    usage
    exit 0
    OK=1 
  fi
done

#########################################################################
# E N D   O F   P A R S I N G   C O M M A N D   L I N E   O P T I O N S #
#########################################################################


#----------------------------------------------------------------------
#  User specified -dircleanup option.  Delete transient files and exit. --
#----------------------------------------------------------------------
if [ $dircleanup -eq 1 ]; then
  echo "Cleaning up directory and exiting"
  cd $REGRTSTDIR 2>$NULL
  rm -f core dumpfile *.srt *.flt *.tflt
  cd $REGRRUNDIR 2>$NULL
  rm -f core dumpfile *.srt *.flt *.tflt
  exit 0
fi

#---------------------------------------------------------------------
# Initialize variables used in the test                             --
#---------------------------------------------------------------------

PRIVSDIR=${rundir}/privs1
sbdefsfile="$REGRTOOLSDIR/sbdefs"
export BUILD_FLAVOR=`echo $BUILD_FLAVOR | tr a-z A-Z`
echo "build flavor: $BUILD_FLAVOR"

seabase="$SEABASE_REGRESS"

# set up default catalog and schema
export TEST_CATALOG='TRAFODION'
export TEST_SCHEMA_NAME='sch'
export TEST_SCHEMA="$TEST_CATALOG.$TEST_SCHEMA_NAME"
echo "test catalog and schema: $TEST_SCHEMA"

cd $REGRTSTDIR 2>$NULL
echo "current work directory: `pwd`"

#---------------------------------------------------------------------
# uppercase all test, expected, known, filter, and diff files       --
#---------------------------------------------------------------------
lctestfiles=`ls -1 test???* 2>$NULL | sed -e /~$/d -e /.bak$/d | \
               sort -fu | pr  -a -h "" -w 9999 -l 1` 2>$NULL
for lcfile in $lctestfiles; do
  ucfile=`ls -1 $lcfile | tr a-z A-Z`
  cp -f $lcfile $ucfile 2>$NULL
done

lcxptdfiles=`ls -1 expected???* 2>$NULL | sed -e /~$/d -e /.bak$/d | \
               sort -fu | pr  -a -h "" -w 9999 -l 1` 2>$NULL
for lcfile in $lcxptdfiles; do
  ucfile=`ls -1 $lcfile | tr a-z A-Z`
  cp -f $lcfile $ucfile 2>$NULL
done

lcknownfiles=`ls -1 *.known* 2>$NULL | sed -e /~$/d -e /.bak$/d | \
               sort -fu | pr  -a -h "" -w 9999 -l 1` 2>$NULL
for lcfile in $lcknownfiles; do
  ucfile=`ls -1 $lcfile | tr a-z A-Z`
  cp -f $lcfile $ucfile 2>$NULL
done

lcfilterfiles=`ls -1 filter???* 2>$NULL | sed -e /~$/d -e /.bak$/d | \
               sort -fu | pr  -a -h "" -w 9999 -l 1` 2>$NULL
for lcfile in $lcfilterfiles; do
  ucfile=`ls -1 $lcfile | tr a-z A-Z`
  cp -f $lcfile $ucfile 2>$NULL
done

#--------------------------------------------------
# Based on the command lines options specified,  --
# determine which test files to run.             --
# Grab either the list of specified tests or all --
# the tests beginnning with "TEST".              --
# If only the number is specified, prepend TEST. --
#--------------------------------------------------
if [ "$1" = "" ]; then
  testfiles=`ls -1 TEST???* | tr a-z A-Z | sed -e /~$/d -e /.bak$/d | sort -fu`
  prettyfiles=$testfiles
else
  testfiles=`echo $* | tr a-z A-Z`
  prettyfiles=
  for i in $testfiles; do
    if [ `expr substr $i 1 4` = "TEST" ]; then
      prettyfiles="$prettyfiles $i"
    else
      prettyfiles="$prettyfiles TEST$i"
    fi
  done
fi

#-------------------------------------------------------
# For now, don't run these tests                      --
# Add list of tests to script in "skipTheseTests"     --
#-------------------------------------------------------
skipTheseTests="TEST133"

#skip checkTest tests if they have already been run
if [ "$CHECK_TEST2" == "1" ]; then
    skipTheseTests="$skipTheseTests $privs1CT"
fi

testfiles="$prettyfiles"
prettyfiles=
skippedfiles=
for i in $testfiles; do
  skipthis=0
  for j in $skipTheseTests; do
    if [ "$i" = "$j" ]; then
      skipthis=1
    fi
  done

  if [ $skipthis -eq 0 ]; then
    prettyfiles="$prettyfiles $i"
  else
    skippedfiles="$skippedfiles $i"
  fi
done
testfiles=$prettyfiles


#-----------------------------------------
#  Inform user which tests will be run. --
#-----------------------------------------
echo "TESTS TO BE RUN: $testfiles"
if [ "$skippedfiles" != "" ]; then
  echo "TESTS NOT RUN: $skippedfiles"
fi

#--------------------------------------------------
#  Set up environment variables for running SQL. --
#--------------------------------------------------
cd $REGRRUNDIR 2>$NULL


#--------------------------------------------------
#  If the user did not choose the -diff option,  --
# rename the old log file to have a .bak suffix. --
# Write date and time to new log file.           --
#--------------------------------------------------
test $diffsonly -eq 0 && mv -f $rgrlog $rgrlog.bak 2>$NULL
echo "`date +'%D %T'`" > $rgrlog

#---------------------------------------
#  For each test file in the list.... --
#---------------------------------------
for ix in $testfiles; do

  #---------------------------------------
  #  Tell user whether we're just doing --
  # diffs or actually running a test.   --
  #---------------------------------------
  echo
  if [ $diffsonly -eq 1 ]; then
    echo "------------------------------------------------------------"
    echo "-- Doing diff for test $ix: "
    echo "------------------------------------------------------------"
  else
    echo "------------------------------------------------------------"
    echo "-- Starting test $ix: "
    echo "------------------------------------------------------------"
  fi
  echo

  #---------------------------------------------------------
  #  Set up the test file, expected file, output file and --
  # difference file names for this iteration.  Also clear --
  # out the log text for the current iteration.           --
  #---------------------------------------------------------
  tnum=`echo $ix | cut -c 5-`
  exp=EXPECTED$tnum
  diff=DIFF$tnum
  tfile=$REGRTSTDIR/$ix
  dfile=$REGRRUNDIR/$diff
  lfile=$REGRRUNDIR/LOG$tnum
  logtxt=
  runTheTest=1

  # --------------------------------------------------------
  # set up expected results file                          __
  #---------------------------------------------------------

  # if EXPECTED.SB exists, use that
  if [ -r "$REGRTSTDIR/${exp}.SB" ]; then
      exp="${exp}.SB"
  fi
  echo "Using expected result file: $exp"

  # If a special file is needed for the release version, use it
  if [ "$BUILD_FLAVOR" = "RELEASE" ]; then
    if [ -r $REGRTSTDIR/$exp.L$BUILD_FLAVOR ]; then
      exp=$exp.L$BUILD_FLAVOR
    fi
  fi

  # use the default expected file
  if [ -r "$REGRTSTDIR/$exp" ]; then
    efile=$REGRTSTDIR/$exp
  fi

  if [ $diffsonly -eq 0 ]; then
    sqlci -i $scriptsdir/tools/reg_users.sql;
    echo "Authorization has been enabled"
  fi

  #--------------------------------------------------
  # Run test if the -diff option not specified     --
  #--------------------------------------------------
  if [ $diffsonly -eq 0 ]; then
    if [ $runTheTest -eq 1 ]; then
      rm -f $lfile.BAK $dfile.BAK 2>$NULL
      mv -f $lfile $lfile.BAK 2>$NULL
      mv -f $dfile $dfile.BAK 2>$NULL

      if [ "$REGRTSTDIR" != "$REGRRUNDIR" ]; then
         cp -f $REGRTSTDIR/$ix $REGRRUNDIR/$ix 2>$NULL
      fi

      echo "Executing: $sqlci -i$tfile"
      $sqlci -i$tfile

      if [ "$REGRTSTDIR" != "$REGRRUNDIR" ]; then
         rm -f $REGRRUNDIR/$ix 2>$NULL
      fi
    fi
  else
    rm -f $dfile.BAK
    mv -f $dfile $dfile.BAK
  fi

  #---------------------------------------
  #  Sort log and expected result file. --
  #---------------------------------------
  if [ -x $LOGSORT ]; then
    efilesrt=$exp.srt
    lfilesrt=$lfile.srt
    rm -f $efilesrt $lfilesrt
    echo
    echo "SORTING EXPECTED AND LOG FILES"
    echo "$LOGSORT $efile $efilesrt"
    echo "$LOGSORT $lfile $lfilesrt"
    echo
    $LOGSORT $efile $efilesrt >> $NULL    # not to $diff, because
    $LOGSORT $lfile $lfilesrt >> $NULL    # logsort writes too much junk
  else
    efilesrt=$efile
    lfilesrt=$lfile
    echo "Could not find $LOGSORT, comparing unsorted files"
  fi

  #------------------------------
  # filter result files        --
  #------------------------------

  # If special filter file exist for test, run it
  if [ -x "$REGRTSTDIR/FILTER$tnum" -a -s "$REGRTSTDIR/FILTER$tnum" ]; then
    efiletflt=$exp.tflt
    lfiletflt=$lfile.tflt
    rm -f $efiletflt $lfiletflt
    echo "RUNNING SPECIAL FILTER FOR TEST $tnum"
    echo "Special filtering expected file"
    echo "$REGRTSTDIR/FILTER$tnum $efilesrt > $efiletflt"
    echo "Special filtering log file"
    echo "$REGRTSTDIR/FILTER$tnum $lfilesrt > $lfiletflt"
    echo
    $REGRTSTDIR/FILTER$tnum $efilesrt > $efiletflt 2>&1
    $REGRTSTDIR/FILTER$tnum $lfilesrt > $lfiletflt 2>&1
  else
    efiletflt=$efilesrt
    lfiletflt=$lfilesrt
  fi

  # Run general filter file common for all tests
  echo "RUNNING STANDARD FILTER FILE ON SORTED OUTPUT"
  rm -f $exp.flt $lfile.flt
  echo "Filtering expected file:"
  echo "$FILTER $efiletflt > $exp.flt"
  echo "Filtering log file:"
  echo "$FILTER $lfiletflt > $lfile.flt"
  echo
  $FILTER $efiletflt > $exp.flt 2>&1
  $FILTER $lfiletflt > $lfile.flt 2>&1

  #----------------------------------------------------------------
  #  Capture filtered files                              --
  #----------------------------------------------------------------
  echo
  echo "RETAINING RESULTS OF FILTER OPERATION"
  echo "Saving filtered expected files:"
  echo "cp $exp.flt $FilteredLogs/expfiles/LOG$tnum"
  cp $exp.flt $FilteredLogs/expfiles/LOG$tnum
  echo "Saving filtered log files:"
  echo "cp $lfile.flt $FilteredLogs/logfiles/LOG$tnum"
  cp $lfile.flt $FilteredLogs/logfiles/LOG$tnum

  #----------------------------------------------------------------
  #  Compare filtered expected and actual files for differences. --
  #----------------------------------------------------------------
  echo
  echo "RUNNING DIFFs ON SORTED AND FILTERED LOGS/EXPECTED FILES"
  echo "diff $exp.flt $lfile.flt >> $dfile"
  echo
  if [ -z "$DUMA_FILL" ] ; then
    diff $exp.flt $lfile.flt >> $dfile 2>&1
  else
    diff $exp.flt $lfile.flt >> $dfile 2>/dev/null
  fi


  #rm -f $efilesrt $lfilesrt $efile.flt $lfile.flt $efiletflt $lfiletflt 2>$NULL

  #------------------------------------------
  #  Determine how many lines differed     --
  # between the expected and actual files. --
  #------------------------------------------
  diffsize=`wc -l $dfile`; diffsize=`echo $diffsize | cut -f1 -d' '`

  #-----------------------------------------------------------------
  # Compare with the known diff file to see if differences are ok --
  #-----------------------------------------------------------------
   diffsAreKnown=0
   diffknownfile=
   knownsize=
   if [ $diffsize -ne 0 ]; then
       if [ -r         $REGRRUNDIR/$diff.KNOWN.$BUILD_FLAVOR ]; then
         diffknownfile=$REGRRUNDIR/$diff.KNOWN.$BUILD_FLAVOR
       elif [ -r       $REGRTSTDIR/$diff.KNOWN ]; then
         diffknownfile=$REGRTSTDIR/$diff.KNOWN
       fi
   fi

    #  Diff the diff and the diff.known files and see how many lines differ
    if [ "$diffknownfile" != "" ]; then
      echo
      echo "COMPARING KNOWN DIFFS FILE TO CURRENT DIFFS"
      echo "Size of diff file: $diffsize"
      # Filter known diff file to avoid schema differences
      knownfiltered="$(dirname $dfile)/$(basename $diffknownfile).flt"
      dfilefiltered="$dfile.flt"
      echo "Filtering known diff file: "
      echo "$FILTER $diffknownfile > $knownfiltered 2>&1"
      $FILTER $diffknownfile > $knownfiltered 2>&1
      echo "Filtering diff file: "
      echo "$FILTER $dfile > $dfilefiltered 2>&1"
      $FILTER $dfile > $dfilefiltered 2>&1
      echo "Performing the DIFF: "
      echo "diff $dfilefiltered $knownfiltered 2>&1"
      diff $dfilefiltered $knownfiltered > $NULL
      if [ $? -eq 0 ]; then
        diffsAreKnown=1
      else
        ktmp=`wc -l $knownfiltered`; ktmp=`echo $ktmp | cut -f1 -d' '`
        test "$ktmp" != "" &&
        test $ktmp -ne 0 && knownsize=" (vs. $ktmp known)"
      fi
    fi

  #--------------------------
  #  Print result of test. --
  #--------------------------
  if [ $diffsize -eq 0 -o $diffsAreKnown -ne 0 ]; then
    if [ $diffsize -eq 0 ]; then
      logtxt="### PASS ###$logtxt"
    else
      logtxt="### PASS with known diffs ###"
    fi
  else
    logtxt="### FAIL ($diffsize lines$knownsize)     ###"
  fi


  #-------------------------------------------------------------
  #  Print date, time, test name and test result in log file. --
  #-------------------------------------------------------------
  echo $logtxt
  modtime=`stat --printf=%y $lfile | cut -d'.' -f1`
  echo "$modtime  $ix  $logtxt" >> $rgrlog

done

#----------------------------------------
#  Print log file to screen            --
#----------------------------------------
echo
echo
echo 'Test Summary'
echo '============'
echo
cat $rgrlog
echo

#------------------------
# Clean up dump files. --
#------------------------
rm -f core dumpfile 2>$NULL
#rm -f $REGRRUNDIR/runmxci.ksh 2>$NULL
rm -f $REGRRUNDIR/tmpfile.*.log 2>$NULL
