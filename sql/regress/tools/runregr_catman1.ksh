#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2012-2014 Hewlett-Packard Development Company, L.P.
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
################################################################################
#                    CATMAN1 REGRESSION TEST SUITE                             #
#                                                                              #
#  This script automates the running of tests which test SQLMX tables.         #
#  It is based on the optimizer and fullstack scripts.                         #
#                                                                              #
################################################################################

#! /bin/sh

#  The command set -x, makes the script echo everything.  Useful for debugging
#set -x

#------------------------------------------------------
#  User asked for help text (-h, -help or -? option) --
# print it out and exit.                             --
#------------------------------------------------------
if [ "$1" = "-h" -o "$1" = "-help" -o "$1" = "-?" ]; then
  cat << END_HELP_TEXT

 Usage:
 $0                -- run everything
    [-help]              -- this screen
    [-dircleanup]        -- cleanup directory
    [-diff]              -- do not run, only do diffs
    [<cat tests>]        -- run the specified tests

END_HELP_TEXT
  exit 0
fi

#----------------------------------------------------------------------
#  User specified -cleanup option.  Delete transient files and exit. --
#----------------------------------------------------------------------
if [ "$1" = "-dircleanup" ]; then
  rm -f core dumpfile *.srt *.flt *.tflt
  exit 0
fi

#-----------------------------------------------------------------
#  Each command line option has an associated boolean variable  --
# indicating whether or not the option is in effect.  Initalize --
# these booleans to false.                                      --
#-----------------------------------------------------------------
diffsonly=0

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

  if [ "$1" = "-diff" ]; then
    diffsonly=1;
    OK=1
  fi

done

#########################################################################
# E N D   O F   P A R S I N G   C O M M A N D   L I N E   O P T I O N S #
#########################################################################

#-----------------------------------------------------------------------
#  Initialize global variables.                                       --
#                                                                     --
#    sqlci   : full path of sqlci executable file                     --
#    CATDIR  : full path of catman regression test directory          --
#    LOGSORT : full path of log file sorting program                  --
#    FILTER  : full path of korn shell script for filtering log files --
#    NULL    : null output file                                       --
#    TMP     : full path of temp directory                            --
#    rgrlog  : full path name of output log file for this script      --
#-----------------------------------------------------------------------
CATDIR=${rundir}/catman1
cd $REGRTSTDIR 2>$NULL

# set the platform
# 1 = Linux
platform=1
echo "Platform is: LINUX"

echo ""

#  See if this is seabase
seabase=0
echo "seabase regress: $SEABASE_REGRESS"
if [ "$SEABASE_REGRESS" -ne 0 ]; then
  seabase="$SEABASE_REGRESS"
fi
echo "Value of seabase: $seabase"

# set up defaulat catalog and schema
export TEST_CATALOG='cat'
export TEST_SCHEMA_NAME='sch'
export TEST_SCHEMA="$TEST_CATALOG.$TEST_SCHEMA_NAME"
sbdefsfile=
if [ "$seabase" -gt 0 ]; then
  echo "setting catalog"
  export TEST_CATALOG='TRAFODION'
  if [ -r $REGRTOOLSDIR/sbdefs ]; then
     sbdefsfile="$REGRTOOLSDIR/sbdefs"
  fi
fi

echo "test catalog: $TEST_CATALOG"
echo "test schema: $TEST_SCHEMA"

#---------------------------------------------------------------------
# make all test, expected, filter, and diff files uppercase for NSK --
#---------------------------------------------------------------------
if [ $platform -eq 1 ]; then
  # upcase all test*, expected* and known diff files
  lctestfiles=`ls -1 test???* | sed -e /~$/d -e /.bak$/d | \
                 sort -fu | pr  -a -h "" -w 9999 -l 1` 2>$NULL
  for lcfile in $lctestfiles; do
    ucfile=`ls -1 $lcfile | tr a-z A-Z`
    cp -f $lcfile $ucfile 2>$NULL
  done

  lcxptdfiles=`ls -1 *.mx expected* | sed -e /~$/d -e /.bak$/d | \
                 sort -fu | pr  -a -h "" -w 9999 -l 1` 2>$NULL
  for lcfile in $lcxptdfiles; do
    ucfile=`ls -1 $lcfile | tr a-z A-Z`
    cp -f $lcfile $ucfile 2>$NULL
  done

  lcknownfiles=`ls -1 *.known* | sed -e /~$/d -e /.bak$/d | \
                 sort -fu | pr  -a -h "" -w 9999 -l 1` 2>$NULL
  for lcfile in $lcknownfiles; do
    ucfile=`ls -1 $lcfile | tr a-z A-Z`
    cp -f $lcfile $ucfile 2>$NULL
  done

  lcfilterfiles=`ls -1 filter???* | sed -e /~$/d -e /.bak$/d | \
                 sort -fu | pr  -a -h "" -w 9999 -l 1` 2>$NULL
  for lcfile in $lcfilterfiles; do
    ucfile=`ls -1 $lcfile | tr a-z A-Z`
    cp -f $lcfile $ucfile 2>$NULL
  done
  export BUILD_FLAVOR=`echo $BUILD_FLAVOR | tr a-z A-Z`
fi

#--------------------------------------------------
#  Based on the command lines options specified, --
# determine which test files to run.             --
#--------------------------------------------------
if [ -z "$1" ]; then
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

# if these are seabase tests, remove files from the list that are not
# functional on seabase
if [ "$seabase" -gt 0 ]; then
  sbtestfiles="TEST136"
  sbprettyfiles=
  for i in $prettyfiles; do
    for j in $sbtestfiles; do
       if [ $i = $j ]; then
          sbprettyfiles="$sbprettyfiles $i";
       fi
    done
  done
  prettyfiles=$sbprettyfiles;
fi

#-------------------------------------------------------
# For now, don't run these tests                      --
#-------------------------------------------------------
testfiles="$prettyfiles"
prettyfiles=
skipTheseTests=

# skip these tests for all platforms and flavours
#
#
skipTheseTests="$skipTheseTests TEST126 TEST135 TEST142 TEST145 TEST146 TEST147 TEST150 TEST153 TEST154 TEST155 TEST180"

if [ "$BUILD_FLAVOR" = "RELEASE" ]; then
  skipTheseTests="$skipTheseTests TEST103 TEST106 TEST108 TEST116 TEST135 TEST140 TEST142 TEST146 TEST147 TEST148 TEST154 TEST155 TEST156"
fi

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
echo "TESTS THAT WILL BE RUN:\n$testfiles\n"
echo
if [ "$skippedfiles" != "" ]; then
  echo "TESTS NOT RUN:\n$skippedfiles\n"
fi

#--------------------------------------------------
#  Set up environment variables for running SQL. --
#--------------------------------------------------
cd $REGRRUNDIR 2>$NULL

echo "--"
echo "-- Current work directory:"
pwd

echo "---------------------------------------------------------"
echo

echo "copying $MAKESCRIPT to $REGRRUNDIR"
cp $MAKESCRIPT $REGRRUNDIR>$NULL
echo "copying $scriptsdir/tools/runmxci.ksh to $REGRRUNDIR"
cp $scriptsdir/tools/runmxci.ksh $REGRRUNDIR >$NULL
echo "copying $scriptsdir/tools/runmxtool to $REGRRUNDIR"
cp $scriptsdir/tools/runmxtool.ksh $REGRRUNDIR>$NULL
echo "copying $scriptsdir/tools/java-compile.ksh to $REGRRUNDIR"
cp $scriptsdir/tools/java-compile.ksh $REGRRUNDIR >$NULL
echo "copying $scriptsdir/tools/xqt.ksh to $REGRRUNDIR"
cp $scriptsdir/tools/xqt.ksh $REGRRUNDIR >$NULL

if [ $diffsonly -eq 0 ]; then
   if [ "$REGRTSTDIR" != "$REGRRUNDIR" ]; then
      echo "copying FILTER_TIME.AWK to $REGRRUNDIR"
      cp -f $REGRTSTDIR/FILTER_TIME.AWK $REGRRUNDIR 2>$NULL
   fi
fi

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

  # set up for linux platform                             --
  if [ $platform -eq 1 ]; then

    echo "Setting up expected results file: $seabase"
    echo "looking for: $REGRTSTDIR/${exp}.SB"
    # if EXPECTED.SB exists, use that
    if [ "$seabase" -gt 0 -a -r "$REGRTSTDIR/${exp}.SB" ]; then
        exp="${exp}.SB"

    # if EXPECTED.LINUX exists, use that
    elif [ -r "$REGRTSTDIR/${exp}.LINUX" ]; then
      exp=$exp.LINUX

    # else use standard EXPECTED file already defined
    fi
    echo "expected result file: $exp"

    # If a special file is needed for the release version, use it
    if [ "$BUILD_FLAVOR" = "RELEASE" ]; then
      if [ -r $REGRTSTDIR/$exp.L$BUILD_FLAVOR ]; then
        exp=$exp.L$BUILD_FLAVOR
      fi
    fi
  fi

  # use the default expected file
  if [ -r "$REGRTSTDIR/$exp" ]; then
    efile=$REGRTSTDIR/$exp
  fi

  #--------------------------------------------------
  # Run test if the -diff option not specified     --
  #--------------------------------------------------
  #eval mainSMDLocation='$'SQLMX_SMD_LOCATION_$NSK_SYS
  #if [ -z "${mainSMDLocation}" ] ; then
  #  eval mainSMDLocation='$'SQLMX_SMD_LOCATION
  #fi
  if [ $diffsonly -eq 0 ]; then
    if [ $runTheTest -eq 1 ]; then
      rm -f $lfile.BAK $dfile.BAK
      mv -f $lfile $lfile.BAK
      mv -f $dfile $dfile.BAK

      if [ "$REGRTSTDIR" != "$REGRRUNDIR" ]; then
         cp -f $REGRTSTDIR/$ix $REGRRUNDIR/$ix 2>$NULL
         if test "$tnum" = "100" ; then
            cp $REGRTSTDIR/viewsel $REGRRUNDIR/viewsel
         fi
         if test "$tnum" = "108" ; then
            cp -f $REGRTSTDIR/echontnsk.ksh $REGRRUNDIR/echontnsk.ksh
         fi
         if test "$tnum" = "189" ; then
            cp -f $REGRTSTDIR/BINARYFILE189A.DAT $REGRRUNDIR/BINARYFILE189A.DAT
         fi
      fi

      if [ -r $rundir/tools/userdefs ]; then
        defsfile="$rundir/tools/userdefs"
      fi

      cat $REGRTSTDIR/cidefs $defsfile $tfile > $ix.tmp

      if test "$tnum" = "108" ; then
        cat $REGRTSTDIR/cidefs $defsfile > $REGRRUNDIR/ci108defs
      fi

      echo "${SQLMX_SMD_LOCATION}"


      if test "$tnum" = "149" ; then
        $REGRRUNDIR/TEST149
      else
        echo "$sqlci -i$ix.tmp"
        $sqlci -i$ix.tmp
        rm -f $ix.tmp 2>$NULL
      fi

      # Reset the SMD location
      #eval SQLMX_SMD_LOCATION='$'mainSMDLocation
      #eval SQLMX_SMD_LOCATION_$NSK_SYS='$'mainSMDLocation

      if [ "$REGRTSTDIR" != "$REGRRUNDIR" ]; then
         rm -f $REGRRUNDIR/$ix 2>$NULL
         if test "$tnum" = "100" ; then
            rm -f $REGRRUNDIR/viewsel 2>$NULL
         fi
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
    echo "$REGRTSTDIR/FILTER$tnum $efilesrt > $efiletflt"
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
  echo "$FILTER $efiletflt > $exp.flt"
  echo "$FILTER $lfiletflt > $lfile.flt"
  echo
  $FILTER $efiletflt > $exp.flt 2>&1
  $FILTER $lfiletflt > $lfile.flt 2>&1
  #----------------------------------------------------------------
  #  Capture filtered files                              --
  #----------------------------------------------------------------
      echo "cp $lfile.flt $FilteredLogs/logfiles/LOG$tnum"
      cp $lfile.flt $FilteredLogs/logfiles/LOG$tnum
      echo "cp $exp.flt $FilteredLogs/expfiles/LOG$tnum"
      cp $exp.flt $FilteredLogs/expfiles/LOG$tnum
  #----------------------------------------------------------------
  #  Compare filtered expected and actual files for differences. --
  #----------------------------------------------------------------
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
   nskKnown=0
   diffsAreKnown=0
   diffknownfile=
   knownsize=
   if [ $diffsize -ne 0 ]; then

     #on LINUX platform, if $diff.known.linux exists, use that.
     if [ $platform -eq 1 ]; then
       if [ -r         $REGRRUNDIR/$diff.KNOWN.LINUX.$BUILD_FLAVOR ]; then
         diffknownfile=$REGRRUNDIR/$diff.KNOWN.LINUX.$BUILD_FLAVOR
       elif [ -r       $REGRTSTDIR/$diff.KNOWN.LINUX ]; then
         diffknownfile=$REGRTSTDIR/$diff.KNOWN.LINUX
       fi
     fi

     test "$diffknownfile" != "" && nskKnown=1

     if [ "$diffknownfile" != "" ]; then
       if [ `cat $diffknownfile | wc -l` -eq 0 ]; then
         # known.nsk file is empty. Try known.
         nskKnown=0
       fi
     fi

     if [ $nskKnown -eq 0 ]; then
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
      echo "$FILTER $diffknownfile > $knownfiltered 2>&1"
      $FILTER $diffknownfile > $knownfiltered 2>&1
      echo "$FILTER $dfile > $dfilefiltered 2>&1"
      $FILTER $dfile > $dfilefiltered 2>&1
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
  fi

  #--------------------------
  #  Print result of test. --
  #--------------------------
  if [ $diffsize -eq 0 -o $diffsAreKnown -ne 0 ]; then
    if [ $diffsize -eq 0 ]; then
      logtxt="### PASS ###$logtxt"
    else
      if [ "$nskrel1known" = "1" ]; then
        logtxt="### PASS with known diffs on NSK ###"
      else
        logtxt="### PASS with known diffs ###"
      fi
    fi
  else
    logtxt="### FAIL ($diffsize lines$knownsize)     ###"
  fi


  #-------------------------------------------------------------
  #  Print date, time, test name and test result in log file. --
  #-------------------------------------------------------------
  echo $logtxt
  echo "`date +'%D %T'`	$ix  $logtxt" >> $rgrlog

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
#rm -f $CATDIR/`basename $MAKESCRIPT` 2>$NULL
rm -f $REGRRUNDIR/runmxci.ksh 2>$NULL
rm -f $REGRRUNDIR/runmxtool.ksh 2>$NULL
rm -f $REGRRUNDIR/java-compile.ksh 2>$NULL
rm -f $REGRRUNDIR/makefileall.ksh 2>$NULL
rm -f $REGRRUNDIR/xqt.ksh 2>$NULL
rm -f $REGRRUNDIR/tmpfile.*.log 2>$NULL
rm -f $REGRRUNDIR/TMP.T115* 2>$NULL
