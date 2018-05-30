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

testsToRun=
allTests=

#allTests have space separated fully qualified tests with format:
#  TEST001.core  TEST023.executor
function setupAllTests() {
    if [ "$2" == "" ]; then
        return
    fi

    for i in $2; do
        tnum=`echo $i | cut -c 5-` 

#        if [ "$1" == "1" ]; then
#            allTests="$allTests TEST$tnum.$3";
#            continue
#        fi

        testdir=$scriptsdir/$3
        testfile=$testdir/TEST$tnum
        expectedfile=$testdir/EXPECTED$tnum
        filterfile=
        if [ -r "$expectedfile.SB" ]; then
            expectedfile="$expectedfile.SB"
        elif [ -r "$expectedfile.LINUX" ]; then
            expectedfile="$expectedfile.LINUX"
        fi

        if [ "$BUILD_FLAVOR" = "RELEASE" ]; then
            if [ -r "$expectedfile.RELEASE" ]; then
                expectedfile="$expectedfile.RELEASE"
            fi
        fi

        knowndiff=
        if [ -r "$testdir/DIFF$tnum.KNOWN.LINUX" ]; then
            knowndiff=$testdir/DIFF$tnum.KNOWN.LINUX
        elif [ -r "$testdir/DIFF$tnum.KNOWN.SB" ]; then
            knowndiff=$testdir/DIFF$tnum.KNOWN.SB
        elif [ -r "$testdir/DIFF$tnum.KNOWN" ]; then
            knowndiff=$testdir/DIFF$tnum.KNOWN
        fi

        cp -f $testfile TEST$tnum.$3
        cp -f $expectedfile EXPECTED$tnum.$3
        if [ "$knowndiff" != "" ]; then
            cp -f $knowndiff DIFF$tnum.KNOWN.$3
        fi

        filterfile=
        if [ -r "$testdir/FILTER$tnum.SB" ]; then
            filterfile="$testdir/FILTER$tnum.SB"
        elif [ -r "$testdir/FILTER$tnum" ]; then
            filterfile="$testdir/FILTER$tnum"
        fi
        if [ "$filterfile" != "" ]; then
            cp -f $filterfile FILTER$tnum.$3
            chmod ugo+rwx FILTER$tnum.$3
        fi

#        if [ "$3" == "compGeneral" ]; then
#            cp -f $testdir/hqc_*.* .
#            cp -f $testdir/cache_*.* .
#        fi

        if [ "$3" == "udr" ]; then
            cp -f $testdir/Utils.java .

            export JDBC_T4_URL="jdbc:t4jdbc://localhost:23400/:"
            if [ -r $TRAF_HOME/sql/scripts/sw_env.sh ]; then
                # use a custom port for the JDBC Type 4 driver
                . $TRAF_HOME/sql/scripts/sw_env.sh
                export JDBC_T4_URL="jdbc:t4jdbc://localhost:${MY_DCS_MASTER_PORT}/:"
            fi
        fi

        allTests="$allTests TEST$tnum.$3";
    done
}

# $1 contains space separated entries with format:
#    core/TEST001 executor/test023 seabase/010 hive
function setupTestsToRun() {
    if [ "$1" == "" ]; then
        # run all tests
        testsToRun="$allTests"
        return
    fi

    for i in $1; do

        iDir=
        iTst=
        if [ `echo $i | grep -c "/" ` -eq 0 ]; then
            iDir=$i
        else
            iDir=$(echo $i | cut -d'/' -f 1)
            iTst=$(echo $i | cut -d'/' -f 2)
            iTst=`echo $iTst | tr a-z A-Z`
            if [ `expr substr $iTst 1 4` != "TEST" ]; then
                nlen=3
                test `expr match "$iTst" ".*[Uu]$"` -gt 0 && nlen=4
                if [ `expr length $iTst` -lt $nlen ]; then
                    iTst=0$iTst
	            if [ `expr length $iTst` -lt $nlen ]; then
	                iTst=0$iTst
	            fi
                fi                
                iTst=TEST$iTst
            fi
        fi

        for j in $allTests; do
            jTst=$(echo $j | cut -d'.' -f 1)
            jDir=$(echo $j | cut -d'.' -f 2)
            if [ "$iDir" == "$jDir" ]; then
                if [ "$iTst" == "" ]; then
                    rTst=$jTst
                    rDir=$jDir
                    testsToRun="$testsToRun $rTst.$rDir"
                elif [ "$iTst" == "$jTst" ]; then
                    rTst=$jTst
                    rDir=$jDir
                    testsToRun="$testsToRun $rTst.$rDir"
                else
                    continue
                fi
            fi
        done #for j
    done #for i
}

if [ "$1" = "-h" -o "$1" = "-help" -o "$1" = "-?" ]; then
  cat << END_HELP_TEXT

 Usage:
 $0 [-d] [-r]
    [-diff] 
    [files...]

 -f or -failuresOnly
   deletes empty (i.e. successful) DIFF files, leaving only failures.

 -diff
   do diffs only, do not run tests

-info
   show which tests will be run

 If no files are specified, all test scripts matching the pattern TEST???*
 are executed.  If specified, the test files should all start with the
 string "TEST" or be the three-digit test numbers.

END_HELP_TEXT
  exit 0
fi

failuresOnly=0
diffOnly=0
infoOnly=0
ct1=0
ct2=0

OK=-1
while [ $OK -ne 0 ]; do		# loop to allow options to appear in any order

  if [ $OK -gt 0 ]; then
    shift $OK
  fi
  OK=0

  if [ "$1" = "-f" -o "$1" = "-fail" -o "$1" = "-failuresOnly" ]; then
    failuresOnly=1
    OK=1
  fi

  if [ "$1" = "-diff" ]; then
    diffOnly=1
    OK=1
  fi

  if [ "$1" = "-info" ]; then
    infoOnly=1
    OK=1
  fi

  if [ "$1" = "-ct1" ]; then
    ct1=1
    OK=1
  fi

  if [ "$1" = "-ct2" ]; then
    ct2=1
    OK=1
  fi

done
# ---------------- end of parsing command line options ----------------

if [ $ct1 -eq 0 ] && [ $ct2 -eq 0 ]; then
    ct1=1
    ct2=1
fi

if [ "$*" == "" ] && [ "$diffOnly" == "0" ]; then
    #if not diff and all tests are to be run, then remove existing rgrStats
    rm -f $rgrStats
fi

export BUILD_FLAVOR=`echo $BUILD_FLAVOR | tr a-z A-Z`
bldFlvr=`grep "BuildFlavor:" $rgrStats`
if [ $diffOnly -eq 0 ]; then
    if [ "$bldFlvr" == "" ]; then
        echo "BuildFlavor:$BUILD_FLAVOR" >> $rgrStats
    fi
elif [ "$bldFlvr" != "" ]; then
    export BUILD_FLAVOR=`echo $bldFlvr | cut -d':' -f2`
    echo $BUILD_FLAVOR
fi

#CT test vars are defined and exported from 'tools/runregr' script.
#Any modification need to be done in 'runregr'
if [ $ct1 -eq 1 ]; then
    setupAllTests "$diffOnly" "$coreCT" "core"
    setupAllTests "$diffOnly" "$compGeneralCT" "compGeneral"
    setupAllTests "$diffOnly" "$charsetsCT" "charsets"
    setupAllTests "$diffOnly" "$executorCT" "executor"
    setupAllTests "$diffOnly" "$fullstack2CT" "fullstack2"
fi

if [ $ct2 -eq 1 ]; then
    setupAllTests "$diffOnly" "$hiveCT" "hive"
    setupAllTests "$diffOnly" "$seabaseCT" "seabase"
    setupAllTests "$diffOnly" "$privs1CT" "privs1"
    setupAllTests "$diffOnly" "$privs2CT" "privs2"
    setupAllTests "$diffOnly" "$udrCT" "udr"
fi

setupTestsToRun "$*"

export TEST_CATALOG='TRAFODION'
export TEST_SCHEMA_NAME='SCH'
seabase="$SEABASE_REGRESS"

sbdefsfile=
if [ -r $REGRTOOLSDIR/sbdefs ]; then
    sbdefsfile="$REGRTOOLSDIR/sbdefs"
fi

export TEST_SCHEMA="$TEST_CATALOG.$TEST_SCHEMA_NAME"

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

cat $rgrlog >> $TMP/`basename $rgrlog` 2>$NULL	# append elsewhere, for us who rm *.bak
mv -f $rgrlog $rgrlog.bak 2>$NULL
echo "`date +'%F %T'`	($BUILD_FLAVOR build)" > $rgrlog
echo " " >> $rgrlog
printf "%-21s%-21s%-10s%-20s\n" "dir/TEST" "StartTime" "Duration" "Status" >> $rgrlog
printf "%-21s%-21s%-10s%-20s\n" "=========" "==========" "=========" "=======" >> $rgrlog
echo " " >> $rgrlog

loopStartTime="`date +'%D %T'`"

testStartTime=
testEndTime=
testElapsedTime=
totalElapsedTime=0
for i in $testsToRun; do
    tnum=`expr substr $i 5 3`
    dir=`echo $i | cut -c 9-`
    test=TEST$tnum.$dir
    exp=EXPECTED$tnum.$dir
    diffknownfile=DIFF$tnum.KNOWN.$dir
    log=LOG$tnum.$dir
    diff=DIFF$tnum.$dir
    logtxt=
    filter=FILTER$tnum.$dir
    
    if [ $infoOnly -ne 0 ]; then
        echo $dir/TEST$tnum
        continue
    fi
    
    mv -f $diff $diff.BAK				2>$NULL
    if [ $diffOnly -eq 0 ]; then
        rm -f $log.BAK $diff.BAK
        mv -f $log  $log.BAK			2>$NULL
        
        echo "------------------------------------------------------------"
        echo "-- Starting test $dir/TEST$tnum: "
        echo "------------------------------------------------------------"
    else
        echo "------------------------------------------------------------"
        echo "-- Doing diff for test $dir/TEST$tnum: "
        echo "------------------------------------------------------------"
    fi
    echo
    
    #run the test
    defsfile=
    if [ $diffOnly -eq 0 ]; then
        if [ -r $scriptsdir/$dir/userdefs ]; then
            defsfile="$scriptsdir/$dir/userdefs"
        fi
        
        cat $sbdefsfile $defsfile $test > $test.tmp
        
        rm -f TEST$tnum 2>$NULL
        cp $test TEST$tnum 2>$NULL
        
        export REGRTSTDIR=$scriptsdir/$dir
        
        testStartTime="`date +'%F %T'`"
        testStartTimeInSecs="`date +'%s'`"

	$sqlci -i$test.tmp 

        testEndTimeInSecs="`date +'%s'`"
        testElapsedTime=$(($testEndTimeInSecs - $testStartTimeInSecs))
        #formattedET="`date -u -d @${testElapsedTime} +"%T"`"
        exists=`grep "$dir/TEST$tnum" $rgrStats`
        if [ "$exists" != "" ]; then
            sed -i "s/$dir\/TEST$tnum.*/$dir\/TEST$tnum|$testStartTime|$testElapsedTime/" $rgrStats 
        else
            echo "$dir/TEST$tnum|$testStartTime|$testElapsedTime" >> $rgrStats
        fi

        rm -f $log 2>$NULL
        cp LOG$tnum $log 2>$NULL 
    fi   
    rm -f $test.tmp 2>$NULL
    rm -f TEST$tnum 2>$NULL
    rm -f TEST$tnum.$dir 2>$NULL
    
    #---------------------------------------
    #  Sort log and expected result file. --
    #---------------------------------------
    # sort log and expected file
    if [ -x $LOGSORT ]; then
        expd=$exp.srt
        logd=$log.srt
        rm -f $expd $logd
        echo "SORTING EXPECTED AND LOG FILES"
        echo "$LOGSORT $exp $expd"
        echo "$LOGSORT $log $logd"
        echo
        $LOGSORT $exp $expd >> $NULL	# not to $diff, because
        $LOGSORT $log $logd >> $NULL	# logsort writes too much junk
    else
        expd=$exp
        logd=$log
        echo "Could not find $LOGSORT, comparing unsorted files"
    fi
    
    #------------------------------
    # filter result files        --
    #------------------------------
    
    if [ -r "$filter" ]; then
      # Mask out test-specific patterns (like timestamps,
      # generated identifiers, explain statistics) before doing the diff.
        expr=$exp.tflt
        logr=$log.tflt
        
        echo "RUNNING SPECIAL FILTER FOR TEST $test"
        $REGRRUNDIR/$filter $expd > $expr 2>&1
        $REGRRUNDIR/$filter $logd > $logr 2>&1
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
            echo "RUNNING STANDARD FILTER FOR TEST $test"
            echo "$FILTER $logr > $logt"
            echo "$FILTER $expr > $expt"
            echo
            $FILTER $expr > $expt 2>&1
            $FILTER $logr > $logt 2>&1
            echo "RUNNING DIFFs ON SORTED AND FILTERED LOGS/EXPECTED FILES"
            echo "diff $expt $logt >> $diff"
            diff $expt $logt >> $diff 2>&1
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
    
    # cleanup
    chmod ug+rw $exp.*srt $exp.*flt $log* $diff*		2>$NULL
    
    # see if there were differences (not safe to use "$?" status, if the
    # diff above pipes into some other command or some other cmd intervenes)
    diffsize=`wc -l $diff`; diffsize=`echo $diffsize | cut -f1 -d' '`
    
    #diff the diff and the diff.KNOWN files.
    diffsAreKnown=0
    knownsize=
    nskKnown=0
    
    if [ $diffsize -ne 0 ]; then
        if [ -r "$diffknownfile" ]; then
            echo
            echo "COMPARING KNOWN DIFFS FILE TO CURRENT DIFFS"
            echo "# ($diffsize lines different)"
            # Filter known diff file to avoid schema differences
            knownfiltered="$diffknownfile.flt"
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
            logtxt="### PASS ###"
        else
#            logtxt="### PASS with known diffs ###$logtxt"
            logtxt="### PASS (known diffs) ###"
        fi
    else
        ls -l $diff*
        logtxt="### FAIL ($diffsize lines$knownsize)     ###"
    fi
    echo $logtxt

    exists=`grep "$dir/TEST$tnum" $rgrStats`
    if [ "$exists" != "" ]; then
        start=`grep "$dir/TEST$tnum" $rgrStats | cut -d'|' -f2`
        elapsed=`grep "$dir/TEST$tnum" $rgrStats | cut -d'|' -f3`
        formattedET="`date -u -d @${elapsed} +"%T"`"
        printf "%-21s%-21s%-10s%s\n" "$dir/TEST$tnum" "$start" "$formattedET" "$logtxt" >> $rgrlog
        echo
    
        totalElapsedTime=$(($totalElapsedTime + $elapsed))
    fi
done

formattedTotalET="`date -u -d @${totalElapsedTime} +"%T"`"

echo " " >> $rgrlog
echo "TotalDuration:      " $formattedTotalET >> $rgrlog

echo "-------------------------------------------------------------------------"
echo
loopEndTime="`date +'%F %T'`"

if [ $infoOnly -ne 0 ]; then
    echo
else
    echo 'Regression Tests Summary'
    echo '========================'
    echo
    ls -l $sqlci $mxcmp | sed 's/\.exe$//'  # Summarize what we were testing with
    echo " " | tee -a $rgrlog
    echo
fi

cat $rgrlog

echo
