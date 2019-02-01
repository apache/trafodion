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

# if [ `uname` = "Windows_NT" ]; then
#   checkBuild -fast $cfg &
# fi

cd $REGRTSTDIR 2>$NULL

if [ "$REGRCONCURRENT" -eq 1 ]; then 
  test_suite=${REGRBASDIR:?'REGRBASDIR is undefined'}
  export TEST_CATALOG='cat'
  export TEST_SCHEMA_NAME="$test_suite"
else
  export TEST_CATALOG='cat'
  export TEST_SCHEMA_NAME='sch'
fi

seabase=0
sbdefsfile=
if [ "$SEABASE_REGRESS" -ne 0 ]; then
  export TEST_CATALOG='TRAFODION'
  seabase="$SEABASE_REGRESS"

  if [ -r $REGRTOOLSDIR/sbdefs ]; then
     sbdefsfile="$REGRTOOLSDIR/sbdefs"
  fi
fi

export TEST_SCHEMA="$TEST_CATALOG.$TEST_SCHEMA_NAME"

#Tests that diddle with the SYSTEM_DEFAULTS table must run exclusive.
exclusiveTests='TEST011 TEST030 TEST031 TEST032 TEST037 TEST038 TEST039 TEST050 TEST051 TEST052 TEST055 TEST300 TEST301'

if [ "$REGREXCLUSIVE" -ne 0 ]; then
  # Sequential execution of exclusive tests
  prettyfiles="$exclusiveTests"
elif [ -z "$1" ]; then
  # default is to run TEST??? scripts (TEST001, ..., TEST399, etc)
  # This complicated pipe lists all the TEST??? files, uppercases them,
  # eliminates backup files, sorts, and puts everything in one line.
  testfiles=`ls -1 TEST??? | tr a-z A-Z | sed -e /~$/d -e /.bak$/d | sort -fu`
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

# sbtestfiles contains the list of tests to be run in seabase mode
if [ "$seabase" -ne 0 ]; then
  sbtestfiles="TEST001 TEST002 TEST003 TEST004 TEST008 TEST009 TEST010 TEST012 TEST014 TEST310 TEST311 TEST312 TEST313 TEST314 TEST315 TEST316 TEST3265" 
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

skippedfiles=

##############################################################
# Following tests are not run on all platforms:
#    
#
# Following tests do not work yet for SQLMX tables on NSK:
#
#
# Following tests are not run for SQLMP tables on NSK:
#  
# 
# Following tests are not run for release code on NSK:
#  (They require special debug-only code)
#
# Following tests are not run for release code on NSK for SQLMP tables
#
################################################################
testfiles="$prettyfiles"
prettyfiles=

# skip these tests on all platforms.
# For now, lets not run test050 on all platforms.  There are problems with
# partially checked in changes that will be fixed later (rsm - 5/20/02)

# Aug 29, 2008: Disabled TEST032 for now, it creates incompatible schemas in the shared catalog CAT.
#                             The test must be modified to create those schemas in its own catalog which must be
#                             dropped at the end of the test. Also, the test will not be able to run in release mode.
# Adding TEST502 to list of skipped tests (12/11/09) until the test is made
# more robust and deterministic.
skipTheseTests="TEST032 TEST008 TEST038 TEST502"
# T008 test Kanji characters. We do not support it any more since R2.4 (BY)


# skip these tests for MX format objects
if [ $SQLMX_REGRESS -eq 1 ]; then
  skipTheseTests="$skipTheseTests TEST007"
fi

# skip these tests for release run on NSK
if [ "$BUILD_FLAVOR" = "RELEASE" ]; then
  skipTheseTests="$skipTheseTests TEST011 TEST014"
fi

# Skip exclusive tests during concurrent execution
if [ "$REGRCONCURRENT" -ne 0 ]; then
  skipTheseTests="$skipTheseTests $exclusiveTests"
fi

# skip these embedded tests
# Skip ISO_MAPING={SJIS|UTF8} tests: TEST037 TEST038 TEST039 TEST050 TEST051 TEST052
# skip Czech collation tests (Czech collation is not supported by SeaQuest SQL software)
if [ `uname` = "Linux" ]; then 
  skipTheseTests="$skipTheseTests TEST007 TST008 TEST009 TEST031 TEST037 TEST038 TEST039 TEST051 TEST052"
  skipTheseTests="$skipTheseTests TEST500 TEST503"
fi

# Must skip 005 & 013 if the NAWinNT.h file is not available to embedded tests to compile
if [ `uname` = "Linux" ]; then 
  if [ -e $scriptsdir/../common/NAWinNT.h ]; then
     skipTheseTests="$skipTheseTests"
  else
     skipTheseTests="$skipTheseTests TEST005 TEST013"
  fi
fi

# skip test009 (MP KANJI and KSC5601) on NT
# skip TEST037 (Japanese configuration not supported by Seaquest SQL software)
# skip Czech collation tests (Czech collation is not supported by SeaQuest SQL software)
# skip TEST137 (key length > 255 bytes - not supported on NT platform)
if [ `uname` = "Windows_NT" ]; then
  # ...old... skipTheseTests="$skipTheseTests TEST009 TEST037"
  skipTheseTests="$skipTheseTests TEST009 TEST030 TEST031 TEST037 TEST038 TEST039 TEST051 TEST052"
  skipTheseTests="$skipTheseTests TEST111 TEST132 TEST137 TEST500 TEST503"
fi

#skip checkTest tests if they have already been run
if [ "$CHECK_TEST1" == "1" ]; then
    skipTheseTests="$skipTheseTests $charsetsCT"
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

if [ $diffOnly -eq 0 ]; then
  echo "--"
  echo "-- Executables:"
  ls -l $sqlci $mxcmp 				# YES, do this in two steps,for
  ls    $sqlci $mxcmp >$NULL 2>&1 || exit 1	# those of us who have written
  env | grep -i _DEBUG				# our own ls.ksh command...
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

cp -f $MAKESCRIPT $REGRRUNDIR 2>$NULL 
echo "copying $MAKESCRIPT to $REGRRUNDIR"
cp -f $scriptsdir/tools/runmxcmp.ksh $REGRRUNDIR 2>$NULL
cp -f $scriptsdir/tools/runmxci.ksh $REGRRUNDIR 2>$NULL
cp $scriptsdir/charsets/*.sh $REGRRUNDIR 2>$NULL

if [ $diffOnly -eq 0 ]; then
   if [ "$REGRTSTDIR" != "$REGRRUNDIR" ]; then
      echo "copying FILTER_TIME.AWK to $REGRRUNDIR"
      cp -f $REGRTSTDIR/FILTER_TIME.AWK $REGRRUNDIR 2>$NULL
   fi
fi

for i in etest*.sql test*_sql sql_text.mdf *.sh wstring.h \
         rowsetsetup rowsetcleanup test*.txt TEST*.txt
do
   cp -f $REGRTSTDIR/$i $REGRRUNDIR 2>$NULL
done

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


  ###########################################################
  # Determine suffix for expected results.
  ###########################################################
  if [ `uname` = "Linux" ]; then
     if [ "$seabase" -ne 0 -a -r "$REGRTSTDIR/${exp}.SB" ]; then
        exp="${exp}.SB"
     elif [ -r "$REGRTSTDIR/${exp}.LINUX" ]; then
        exp="${exp}.LINUX"
     else
        exp="${exp}"
     fi

     if [ $BUILD_FLAVOR = "RELEASE" ]; then
          if [ "$seabase" -ne 0 -a -r "$REGRTSTDIR/EXPECTED$tnum.SB.RELEASE" ]; then
             exp="EXPECTED$tnum.SB.RELEASE"
          elif [ -r "$REGRTSTDIR/EXPECTED$tnum.LINUX.RELEASE" ]; then
             exp="EXPECTED$tnum.LINUX.RELEASE"
          fi
     fi
  fi

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

  if [ "$tnum" != "MEM" ]; then
    if [ -z "$expfile" ]; then
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
       if [ $test = "TEST006" ]; then
          cp -f $REGRTSTDIR/etest010.sql $REGRRUNDIR 2>$NULL
       elif [ $test = "TEST055" ]; then
          cp -f $REGRTSTDIR/TEST055.TXT $REGRRUNDIR 2>$NULL
          cp -f $REGRTSTDIR/TEST05X_CHANGE_ISOMAPPING.SH $REGRRUNDIR 2>$NULL
          cp -f $REGRTSTDIR/TEST05X_RUN.SH $REGRRUNDIR 2>$NULL
          chmod a+x $REGRRUNDIR/TEST05X_CHANGE_ISOMAPPING.SH 2>$NULL
          chmod a+x $REGRRUNDIR/TEST05X_RUN.SH 2>$NULL
       fi
    fi

    if [ $tnum = "TOK" ]; then
      $(wDir)/parserToks.ksh $(wDir)/parser/SqlParser.y NO-POPUP 2>&1 | tee LOGTOK;
    else
      if [ -r $rundir/tools/userdefs ]; then
        defsfile="$rundir/tools/userdefs"
      fi

      if [ "$REGRCONCURRENT" -eq 1 ]; then
        echo "create schema ${TEST_SCHEMA}; set schema ${TEST_SCHEMA};" \
          | cat $sbdefsfile $defsfile - $testrun > $test.tmp
      else
        cat $sbdefsfile $defsfile $testrun > $test.tmp
      fi

      #Special case for test100 or test101
      if [ "$tnum" = "100" -o "$tnum" = "101" ] ; then
        if [ "$REGRTSTDIR" != "$REGRRUNDIR" ]; then
          cp $REGRTSTDIR/SCRIPT${tnum} $REGRRUNDIR
          cp $REGRTSTDIR/SCRIPT${tnum}I $REGRRUNDIR
          cp $REGRTSTDIR/TEST${tnum}SAVED.XML $REGRRUNDIR
          cp $REGRTSTDIR/t${tnum}.java $REGRRUNDIR
        fi
      fi

      if [ $tnum = "014" ]; then
         export UCS2_SQL_TEXT_DEBUG=1
      fi

      if [ "$errlog" = "" ]; then
        $sqlci -i$test.tmp
      else
	( $sqlci -i$test.tmp 2>&1 ) | tee ERRLOG$tnum
      fi

      if [ $tnum = "014" ]; then
         unset UCS2_SQL_TEXT_DEBUG
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

   if [ `uname` = "Linux" ]; then
      if [ "$seabase" -ne 0 ]; then
         rm -f $log.SB 2>$NULL
         cp $log $log.SB 2>$NULL
      else
         rm -f $log.LINUX 2>$NULL
         cp $log $log.LINUX 2>$NULL
      fi
    fi
 
  fi

  if [ `uname` = "Linux" ]; then
    if [ "$seabase" -ne 0 ]; then
       log=$log.SB
    else
       log=$log.LINUX
    fi
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

  # Tests using env var NO_VOL_CAT_FILTER
  # must check in the FILTERED version of the EXPECTEDnnn.NSK file!
  case $test in
    TEST062)	export NO_VOL_CAT_FILTER=1	;;
    *)		unset  NO_VOL_CAT_FILTER	;;
  esac

  # Tests using env var NO_KARAT_FILTER
  case $test in
    TEST075)    export  NO_KARAT_FILTER=1        ;;
    *)          unset   NO_KARAT_FILTER          ;;
  esac

  #------------------------------
  # filter result files        --
  #------------------------------

  if [ -x "$REGRTSTDIR/FILTER$tnum" -a -s "$REGRTSTDIR/FILTER$tnum" ]; then
    # Mask out test-specific patterns (like timestamps,
    # generated identifiers, explain statistics) before doing the diff.
    expr=$exp.tflt
    logr=$log.tflt

    # Special test case for 018.  Only use the special filter if running
    # in the NSK environment.
    dofilter=1
    if [ $tnum -eq "018" -a $nsk -eq 0 ]; then
      dofilter=0
    fi
 
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
      diff $expr $logr >> $diff 2>&1
    fi
  else
    echo "diff $expr $logr >> $diff"
    diff $expr $logr >> $diff 2>&1
  fi

  #----------------------------------------------------------------
  #  Capture filtered files                                --
  #----------------------------------------------------------------
      echo "cp $logt $FilteredLogs/logfiles/LOG$tnum"
      cp $logt $FilteredLogs/logfiles/LOG$tnum
      echo "cp $expt $FilteredLogs/expfiles/LOG$tnum"
      cp $expt $FilteredLogs/expfiles/LOG$tnum
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
  nskKnown=0

  if [ $diffsize -ne 0 ]; then
    if [ $nskKnown -eq 0 ]; then
       if [ "$seabase" -eq 2 ] && [ -r $REGRTSTDIR/$diff.KNOWN.SB.OS ]; then
         diffknownfile=$REGRTSTDIR/$diff.KNOWN.SB.OS
       elif [ "$seabase" -ne 0 ] && [ -r $REGRTSTDIR/$diff.KNOWN.SB ]; then
         diffknownfile=$REGRTSTDIR/$diff.KNOWN.SB
      elif [ -r         $REGRTSTDIR/$diff.KNOWN.$BUILD_FLAVOR ]; then
	diffknownfile=$REGRTSTDIR/$diff.KNOWN.$BUILD_FLAVOR
      elif [ -r       $REGRTSTDIR/$diff.KNOWN.$BUILD_FLAVOR ]; then
	diffknownfile=$REGRTSTDIR/$diff.KNOWN.$BUILD_FLAVOR
      elif [ -r       $REGRTSTDIR/$diff.KNOWN ]; then
	diffknownfile=$REGRTSTDIR/$diff.KNOWN
      elif [ -r       $REGRTSTDIR/$diff.KNOWN ]; then
	diffknownfile=$REGRTSTDIR/$diff.KNOWN
      fi
    fi

    if [ "$diffknownfile" != "" ]; then
      echo
      echo "COMPARING KNOWN DIFFS FILE TO CURRENT DIFFS"
      echo "# ($diffsize lines different)"
      # Filter known diff file to avoid schema differences
      knownfiltered="$(basename $diffknownfile).flt"
      dfilefiltered="$diff.flt"v
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

#rm -f $MAKESCRIPT 2>$NULL
rm -f $REGRRUNDIR/runmxcmp.ksh 2>$NULL
rm -f $REGRRUNDIR/runmxci.ksh 2>$NULL
rm -f etest*.ilk etest*.lst etest*.?db	2>$NULL

echo "------------------------------------------------------------------------------"
echo
loopEndTime="`date +'%T'`"

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
