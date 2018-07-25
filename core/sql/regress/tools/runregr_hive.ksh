#! /bin/sh
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

if [ "$1" = "-h" -o "$1" = "-help" -o "$1" = "-?" ]; then
  cat << END_HELP_TEXT

 Usage:
 $0 [-d] [-r]
    [-errlog] [-f]
    [-cleanup file]
    [-diff]
    [files...]

 -d or -debug or -Debug
   specifies DEBUG version of sqlci to test. This is the default. Right
   now we don't allow expected files with a "D" suffix (see -r option below).

 -r or -release or -Release
   specifies RELEASE version of sqlci to test. If expected files with
   an "R" suffix exist they will be used instead of the "regular" file.

 -cleanup bat_or_ksh_file
   specifies additional DOS or Unix commands you want to execute after all
   specified regression tests have been run.

 -errlog
   places a complete log file (stdout and stderr) for each script into
   an ERRLOGxxx file.  This can be useful to find diagnostics messages
   from sqlci.

 -f or -failuresOnly
   deletes empty (i.e. successful) DIFF files, leaving only failures.

 -diff
   do diffs only, do not run tests

 If no files are specified, all test scripts matching the pattern TEST???*
 are executed.  If specified, the test files should all start with the
 string "TEST" or be the three-digit test numbers.

END_HELP_TEXT
  exit 0
fi

cfg=Debug
clnpfile=
errlog=
failuresOnly=0
xptdsuffix=
diffOnly=0
stopHadoopWhenDone=0

test "$REGR_FAILONLY" != "" && failuresOnly=1

OK=-1
while [ $OK -ne 0 ]; do		# loop to allow options to appear in any order

  if [ $OK -gt 0 ]; then
    shift $OK
  fi
  OK=0

  if [ "$1" = "-d" -o "$1" = "-debug" -o "$1" = "-Debug" ]; then
    cfg="Debug"
    # For now, since there are only two flavors we assume that the
    # debug file has no suffix while the release file has an "R"
    # suffix. This can easily be changed later.
    xptdsuffix=
    OK=1
  fi

  if [ "$1" = "-r" -o "$1" = "-release" -o "$1" = "-Release" ]; then
    cfg="Release"
    xptdsuffix="R"
    OK=1
  fi

  if [ "$1" = "-cleanup" ]; then
    test "$2" = "" && echo Argument required for -cleanup option && exit 1
    clnpfile=$2
    OK=2
  fi

  if [ "$1" = "-errlog" ]; then
    errlog=ERRLOG
    OK=1
  fi

  if [ "$1" = "-f" -o "$1" = "-fail" -o "$1" = "-failuresOnly" ]; then
    failuresOnly=1
    OK=1
  fi

  if [ "$1" = "-diff" ]; then
    diffOnly=1
    OK=1
  fi

done
# ---------------- end of parsing command line options ----------------

echo "------------------------------------------------------------"

cd $REGRTSTDIR 2>$NULL

if [ "$REGRCONCURRENT" -ne 0 ]; then
  # concurrent execution
  test_suite=${REGRBASDIR:?'REGRBASDIR is undefined'}
  export TEST_CATALOG='cat'
  export TEST_SCHEMA_NAME="$test_suite"
else
  # sequential execution
  export TEST_CATALOG='cat'
  export TEST_SCHEMA_NAME='sch'
fi

seabase=0
sbdefsfile=
if [[ -z "$SEABASE_REGRESS" ]]; then
  export SEABASE_REGRESS=0
fi
if [ "$SEABASE_REGRESS" -ne 0 ]; then
  export TEST_CATALOG='TRAFODION'
  seabase="$SEABASE_REGRESS"

  if [ -r $REGRTOOLSDIR/sbdefs ]; then
     sbdefsfile="$REGRTOOLSDIR/sbdefs"
  fi
fi

export TEST_SCHEMA="$TEST_CATALOG.$TEST_SCHEMA_NAME"

UTEST_SCHEMA_NAME=$(echo $TEST_SCHEMA_NAME | tr a-z A-Z)
export QTEST_SCHEMA_NAME="'$UTEST_SCHEMA_NAME'"

# Tests that should not be executed concurrently are skipped during the
# concurrent execution phase and are executed sequentially after the
# concurrent execution phase has completed.
exclusiveTests=''

if [ $nsk -ne 0 -o `uname` = "Linux" ]; then
  # upcase all test*, expected*, filters and known diff files
  lctestfiles=`ls -1 test???* 2>$NULL | sed -e /~$/d -e /.bak$/d | sort -fu` 2>$NULL

  for lcfile in $lctestfiles; do
    ucfile=`echo $lcfile | tr a-z A-Z`
    cp -f $lcfile $ucfile 2>$NULL
  done
  lcxptdfiles=`ls -1 expected???* 2>$NULL | sed -e /~$/d -e /.bak$/d | sort -fu` 2>$NULL
  for lcfile in $lcxptdfiles; do
    ucfile=`echo $lcfile | tr a-z A-Z`
    cp -f $lcfile $ucfile 2>$NULL
  done
  lcfiltfiles=`ls -1 filter???* 2>$NULL | sed -e /~$/d -e /.bak$/d | sort -fu` 2>$NULL
  for lcfile in $lcfiltfiles; do
    ucfile=`echo $lcfile | tr a-z A-Z`
    cp -f $lcfile $ucfile 2>$NULL
  done
  lcknownfiles=`ls -1 *.known* 2>$NULL | sed -e /~$/d -e /.bak$/d | sort -fu` 2>$NULL
  for lcfile in $lcknownfiles; do
    ucfile=`echo $lcfile | tr a-z A-Z`
    cp -f $lcfile $ucfile 2>$NULL
  done
  export BUILD_FLAVOR=`echo $BUILD_FLAVOR | tr a-z A-Z`
fi

if [ "$REGREXCLUSIVE" -ne 0 ]; then
  # Sequential execution of exclusive tests
  prettyfiles="$exclusiveTests"
elif [ -z "$1" ]; then
  # default is to run TEST??? scripts (TEST001, ..., TEST399, etc)
  testfiles=`ls -1 TEST??? |  sort -fu`
  prettyfiles=$testfiles
else
  testfiles=`echo $* | tr a-z A-Z`
  prettyfiles=
  for i in $testfiles; do
    if [ `expr substr $i 1 4` = "TEST" ]; then
      prettyfiles="$prettyfiles $i"
    else
      nlen=3
      test `expr match "$i" ".*[Uu]$"` -gt 0 && nlen=4
      if [ `expr length $i` -lt $nlen ]; then
        i=0$i
	if [ `expr length $i` -lt $nlen ]; then
	  i=0$i
	fi
      fi
      prettyfiles="$prettyfiles TEST$i"
    fi
  done
fi

skippedfiles=

##############################################################
# Following tests are not run on all platforms:
#    None
#
# Following tests are being skipped for now since they inconsistently fail
#
#
################################################################
testfiles="$prettyfiles"
prettyfiles=

# skip these tests
skipTheseTests="TEST020"

# Skip exclusive tests during concurrent execution
if [ "$REGRCONCURRENT" -ne 0 ]; then
  skipTheseTests="$skipTheseTests $exclusiveTests"
fi

#skip checkTest tests if they have already been run
if [ "$CHECK_TEST2" == "1" ]; then
    skipTheseTests="$skipTheseTests $hiveCT"
fi

for i in $testfiles; do
  skipthis=0
  for j in $skipTheseTests; do
    if [ "$i" = "$j" ]; then
      skipthis=1
    fi
  done

  if [ $nsk -eq 0 ]; then
    #skip all tests with a "."  -- e.g. test001.contrib
    s=`expr substr $i 8 1`
    t=`expr substr $i 9 1`
    if [ "$s" = "." -o "$t" = "." ]; then
      skipthis=1
    fi
  fi

  if [ $skipthis -eq 0 ]; then
    prettyfiles="$prettyfiles $i"
  else
    skippedfiles="$skippedfiles $i"
  fi
done

# print out all test files that will be skipped
if [ "$skippedfiles" != "" ]; then
  echo "-- Skipped testfiles:"
  ls $skippedfiles | sort -fu | pr -5 -a -h "" |
				grep -v '.*:.*Page' |
				grep -v '^ *$'
  echo ""
fi

# print out all test files that will be run
echo "-- Testfiles:"
if [ "$prettyfiles" != "" ]; then
  ls $prettyfiles  | sort -fu | pr -5 -a -h "" |
				grep -v '.*:.*Page' |
				grep -v '^ *$'
fi

stopHadoopWhenDone=0
if [ $diffOnly -eq 0 ]; then
  echo "--"
  echo "-- Executables:"
  ls -l $sqlci $mxcmp				# YES, do this in two steps, for
  ls    $sqlci $mxcmp >$NULL 2>&1 || exit 1	# those of us who have written
  env | grep -i _DEBUG				# our own ls.ksh command...

  # Check whether we should install a local Linux instance for this run
  if [ `uname` = "Linux" -a -n "$MY_LOCAL_SW_DIST" ]; then
    which swstartall >$NULL 2>&1
    missing_sqstart=$?
    echo
    if [[ ( $missing_sqstart -ne 0 ) || ( ! -d $TRAF_HOME/sql/local_hadoop ) ]]; then
      echo "Local hadoop instance is not installed, invoking install script : install_local_hadoop -y"
      echo
      install_local_hadoop -y
      install_result=$?
      if [[ $install_result -ne 0 ]]; then
        echo "ERROR: Failure in install_local_hadoop, return code $install_result"
        exit $install_result
      fi
      stopHadoopWhenDone=1
    else
      echo "Local hadoop instance is installed, no extra install needed."
      echo
      # start local hadoop instance if we don't see 2 mysql processes,
      # NameNode and SecondaryNameNode
      numMatches=`swstatus | grep -e '2 mysql' | wc -l`
      if [ $numMatches -lt 1 ]; then
        echo "Hadoop NameNode or MySQL instance not running, starting it..."
        swstartall
        stopHadoopWhenDone=1
      fi
    fi
    # source in sw_env.sh file that defines HBase Thrift port as MY_HBASE_THRIFT_PORT_NUM*
    . $(dirname $(which swhadoop))/sw_env.sh
  fi
fi

cd $REGRRUNDIR 2>$NULL

wDir=../..

echo "--"
echo "-- Current work directory:"
pwd

echo "------------------------------------------------------------"
echo

unset  HISTFILE

rm -f etest*.ilk etest*.lst etest*.?db

cat $rgrlog >> $TMP/`basename $rgrlog` 2>$NULL	# append elsewhere, for us who rm *.bak
mv -f $rgrlog $rgrlog.bak 2>$NULL
echo "`date +'%D %T'`	$BUILD_FLAVOR_TEXT" > $rgrlog

loopStartTime="`date +'%D %T'`"

# main loop over all test files
for i in $prettyfiles; do
  # compute test number
  tnum=$i
  tnum=`basename $tnum`
  if [ `expr substr $tnum 1 4 | tr a-z A-Z` = "TEST" ]; then
    # tnum=`expr substr $tnum 5 99`  -- this doesn't work right in ksh
    tnum=`echo $tnum | cut -c 5-`
  fi

  # prepare name of log, diff and expected files
  export SQL_CMP_LOG=CMPLOG$tnum
  rm -f $SQL_CMP_LOG

  #########################################################
  #diff:    name of the DIFF file
  #exp:     name of the EXPECTED file
  #expfile: location & name of the EXPECTED file
  #log:     location & name of LOG file
  #test:    name of the TEST file
  #testrun: location & name of the TEST file
  #########################################################
  diff=DIFF$tnum
  exp=EXPECTED$tnum
  expfile=
  log=$REGRRUNDIR/LOG$tnum
  test=TEST$tnum
  testrun=$REGRTSTDIR/TEST$tnum
  logtxt=


  if [ -r "$REGRTSTDIR/$exp" ]; then
    expfile=$REGRTSTDIR/$exp
  fi

  # If a special file is needed for the release version, use it
  if [ -r $expfile$xptdsuffix ]; then
    # special expected file for debug/release exists
    exp=$exp$xptdsuffix
    expfile=$expfile$xptdsuffix
  fi

  # If this test is executing concurrently with other tests, use
  # the parallel execution expected results file if one exists.
  if [ "$REGRCONCURRENT" -eq 1 ] && [ -n "$expfile" ]; then
    if [ -r "${expfile}-P" ]; then
      exp="${exp}-P"
      expfile="${expfile}-P"
    fi
  fi

  if [ "$expfile" = "" ]; then
    echo "Missing expected file for $tnum"
    continue
  fi

  if [ $diffOnly -eq 1 ]; then
    if [ -r "$log" -a -r "$expfile" ]; then
      dummy=
    else
      if [ -r "$log" ]; then
        echo "$expfile missing."
      else
        echo "$log missing."
      fi
      continue
    fi
  fi

  echo
  mv -f $diff $diff.BAK				2>$NULL
  if [ $diffOnly -eq 0 ]; then
    rm -f $log.BAK $diff.BAK
    mv -f $log  $log.BAK			2>$NULL
    rm -f etest$tnum.exe etest$tnum.is

    echo "------------------------------------------------------------"
    echo "-- Starting test $test: "
    echo "------------------------------------------------------------"
  else
    echo "------------------------------------------------------------"
    echo "-- Doing diff for test $test: "
    echo "------------------------------------------------------------"
  fi
  echo

  # run the test

  if [ $diffOnly -eq 0 ]; then
    if [ "$REGRTSTDIR" != "$REGRRUNDIR" ]; then
       cp -f $REGRTSTDIR/$test $REGRRUNDIR/$test 2>$NULL
    fi

    if [ -r $rundir/tools/userdefs ]; then
      defsfile="$rundir/tools/userdefs"
    fi

    if [ "$REGRCONCURRENT" -eq 1 ]; then
      echo "create schema ${TEST_SCHEMA}; set schema ${TEST_SCHEMA};" \
        | cat $sbdefsfile $defsfile - $testrun > $test.tmp
    else
      cat $sbdefsfile $defsfile $testrun > $test.tmp
    fi


    if [ "$errlog" = "" ]; then
      $sqlci -i$test.tmp
    else
      ( $sqlci -i$test.tmp 2>&1 ) | tee ERRLOG$tnum
    fi

    rm -f $test.tmp >$NULL

    if [ -r $SQL_CMP_LOG ]; then
      if [ `cat $SQL_CMP_LOG | wc -l` -eq 0 ]; then
        rm -f $SQL_CMP_LOG
      fi
    fi
  fi

  if [ "$REGRTSTDIR" != "$REGRRUNDIR" ]; then
    rm -f $REGRRUNDIR/$test 2>$NULL
  fi

  #---------------------------------------
  #  Sort log and expected result file. --
  #---------------------------------------
  # sort log and expected file
  if [ -x $LOGSORT ]; then
    expd=$exp.srt
    logd=$log.srt
    rm -f $expd $logd
    echo "SORTING EXPECTED AND LOG FILES"
    echo "$LOGSORT $expfile $expd"
    echo "$LOGSORT $log $logd"
    echo
    $LOGSORT $expfile $expd >> $NULL	# not to $diff, because
    $LOGSORT $log $logd >> $NULL	# logsort writes too much junk
  else
    expd=$expfile
    logd=$log
    echo "Could not find $LOGSORT, comparing unsorted files"
  fi

  #------------------------------
  # filter result files        --
  #------------------------------

  if [ -x "$REGRTSTDIR/FILTER$tnum" -a -s "$REGRTSTDIR/FILTER$tnum" ]; then
    # Mask out test-specific patterns (like timestamps,
    # generated identifiers, explain statistics) before doing the diff.
    expr=$exp.tflt
    logr=$log.tflt

    dofilter=1

    if [ $dofilter -eq 1 ]; then
       echo "RUNNING SPECIAL FILTER FOR TEST $tnum"
       echo "$REGRTSTDIR/FILTER$tnum $expd > $expr"
       echo "$REGRTSTDIR/FILTER$tnum $logd > $logr"
       echo
       $REGRTSTDIR/FILTER$tnum $expd > $expr 2>&1
       $REGRTSTDIR/FILTER$tnum $logd > $logr 2>&1
    else
      expr=$expd
      logr=$logd
    fi
  else
    expr=$expd
    logr=$logd
  fi

  # Run general filter common for all tests
  if [ "$FILTER" != "" ]; then
    if [ -x "$FILTER" ]; then
      # Mask out commonly changing patterns (like timestamps,
      # generated identifiers, explain statistics) before doing the diff.
      expt=$exp.flt
      logt=$log.flt
      echo "RUNNING STANDARD FILTER ON SORTED OUTPUT"
      echo "$FILTER $logr > $logt"
      echo "$FILTER $expr > $expt"
      echo
      $FILTER $expr > $expt 2>&1
      $FILTER $logr > $logt 2>&1
      echo "RUNNING DIFFs ON SORTED AND FILTERED LOGS/EXPECTED FILES"
      echo "diff $expt $logt >> $diff"
      if [ -z "$DUMA_FILL" ] ; then
        diff $expt $logt >> $diff 2>&1
      else
        diff $expt $logt >> $diff 2>/dev/null
      fi

    else
      echo "Could not find ${FILTER:-FILTER program}"
      echo "diff $expr $logr >> $diff"
      echo "RUNNING DIFFs ON SORTED AND FILTERED LOGS/EXPECTED FILES"
      if [ -z "$DUMA_FILL" ] ; then
        diff $expr $logr >> $diff 2>&1
      else
        diff $expr $logr >> $diff 2>/dev/null
      fi

    fi
  else
    echo "diff $expr $logr >> $diff"
    if [ -z "$DUMA_FILL" ] ; then
      diff $expr $logr >> $diff 2>&1
    else
      diff $expr $logr >> $diff 2>/dev/null
    fi
  fi

  #----------------------------------------------------------------
  #  Capture filtered files                                    --
  #----------------------------------------------------------------
  echo "cp $logt $FilteredLogs/logfiles/LOG$i.flt"
  cp $logt $FilteredLogs/logfiles/LOG$i.flt
  echo "cp $expt $FilteredLogs/expfiles/LOG$i.flt"
  cp $expt $FilteredLogs/expfiles/LOG$i.flt

  # cleanup
  chmod ug+rw $exp.*srt $exp.*flt $log* $diff*		2>$NULL
#  mv -f $exp.*flt $log.*flt			$TMP	2>$NULL
#  rm -f $exp.*flt $log.*flt

  # see if there were differences (not safe to use "$?" status, if the
  # diff above pipes into some other command or some other cmd intervenes)
  diffsize=`wc -l $diff`; diffsize=`echo $diffsize | cut -f1 -d' '`

  #diff the diff and the diff.KNOWN files.
  diffsAreKnown=0
  knownsize=
  diffknownfile=

  if [ $diffsize -ne 0 ]; then
    if [ -r          $REGRTSTDIR/$diff.KNOWN.SB ]; then
      diffknownfile=$REGRTSTDIR/$diff.KNOWN.SB
    elif [ -r        $REGRTSTDIR/$diff.KNOWN.$BUILD_FLAVOR ]; then
      diffknownfile=$REGRTSTDIR/$diff.KNOWN.$BUILD_FLAVOR
    elif [ -r       $REGRTSTDIR/$diff.KNOWN.$BUILD_FLAVOR ]; then
      diffknownfile=$REGRTSTDIR/$diff.KNOWN.$BUILD_FLAVOR
    elif [ -r       $REGRTSTDIR/$diff.KNOWN ]; then
      diffknownfile=$REGRTSTDIR/$diff.KNOWN
    elif [ -r       $REGRTSTDIR/$diff.KNOWN ]; then
      diffknownfile=$REGRTSTDIR/$diff.KNOWN
    fi

    #on LINUX platform, if $diff.known.linux exists, use that.
    if [ `uname` = "Linux" ]; then
      if [ -r       $REGRTSTDIR/$diff.KNOWN.LINUX ]; then
        diffknownfile=$REGRTSTDIR/$diff.KNOWN.LINUX
      fi
    fi

    if [ "$diffknownfile" != "" ]; then
      echo
      echo "COMPARING KNOWN DIFFS FILE TO CURRENT DIFFS"
      echo "# ($diffsize lines different)"
      # Filter known diff file to avoid schema differences
      knownfiltered="$(basename $diffknownfile).flt"
      dfilefiltered="$diff.flt"
      echo "$FILTER $diffknownfile > $knownfiltered 2>&1"
      $FILTER $diffknownfile > $knownfiltered 2>&1
      echo "$FILTER $diff > $dfilefiltered 2>&1"
      $FILTER $diff > $dfilefiltered 2>&1
      echo "diff $dfilefiltered $knownfiltered"
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
  # print result
  if [ $diffsize -eq 0 -o $diffsAreKnown -ne 0 ]; then
    if [ $failuresOnly -ne 0 ]; then
      rm -f $diff $diff.BAK $exp.srt $log.srt $log.BAK
    fi
    if [ $diffsize -eq 0 ]; then
      logtxt="### PASS ###$logtxt"
    else
      logtxt="### PASS with known diffs ###$logtxt"
    fi
  else
    ls -l $diff*
    logtxt="### FAIL ($diffsize lines$knownsize)     ###$logtxt"
  fi
  echo $logtxt
  modtime=`stat --printf=%y $log | cut -d'.' -f1`
  echo "$modtime  $test  $logtxt" >> $rgrlog
  echo

  # save off the DIFF file for debugging
  if [ $nsk -eq 1 ]; then
    if [ $SQLMX_REGRESS -eq 1 ]; then
      rm -f $diff.MX 2>$NULL
      cp $diff $diff.MX 2>$NULL
    else
      rm -f $diff.MP 2>$NULL
      cp $diff $diff.MP 2>$NULL
    fi
  fi

done # for i in $prettyfiles


echo "------------------------------------------------------------------------------"
echo
loopEndTime="`date +'%T'`"

if [ $stopHadoopWhenDone -eq 1 ]; then
  swstopall
fi

if [ $diffOnly -eq 0 ]; then
  echo 'Regression Tests Summary'
  echo '========================'
  echo
  ls -l $sqlci $mxcmp | sed 's/\.exe$//'  # Summarize what we were testing with
  echo "$loopStartTime - $loopEndTime	$BUILD_FLAVOR_TEXT" | tee -a $rgrlog
  echo
fi

if [ $failuresOnly -eq 0 ]; then
  grep '#' $rgrlog
else
  egrep '(^$| FAIL )' $rgrlog			# or just the failures as logged
  sed -n -e '/ FAIL /d' -e '/^$/,$p' $rgrlog
fi

# ls -l CMPLOG* 2>$NULL
echo

if [ "$clnpfile" != "" ]; then
  $clnpfile
fi

