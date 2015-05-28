#! /bin/sh
#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2003-2014 Hewlett-Packard Development Company, L.P.
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


if [ "$1" = "-h" -o "$1" = "-help" -o "$1" = "-?" ]; then
  cat << END_HELP_TEXT

 Usage:
 $0                -- run everything (ddl, ustat, dml)
    [-help]              -- this screen
    [-cleanup]           -- cleanup directory
    [-genshape]          -- prepend CONTROL QUERY SHAPE stmt to qat* tests
                         -- and create qat*.shp test files
    [-runshape]          -- run .shp tests. Both genshape and runshape
                         -- options could be specified along with other
                         -- options
    [-ddb] [-dropdb]     -- drop qat database
    [-cdb] [-createdb]   -- create qat database
    [-ust] [-updstats]   -- update statistics
    [-dml] [-rundml]     -- run all dml queries
    [-diff]              -- do not run, only do diffs
    [-post cmdfile]      -- postprocessing file
    [<qat tests>]        -- run the specified tests

END_HELP_TEXT
  exit 0
fi

if [ "$1" = "-cleanup" ]; then
  rm -f core dumpfile *.srt *.tmp *.dif aqat* dqat?????
  exit 0
fi


createdb=0
diffsonly=0
dropdb=0
genshape=0
# nsk=0		-- commented out, since tools/runregr sets it now
post=0
rundml=0
runshape=0
updstats=0

OK=-1
while [ $OK -ne 0 ]; do

  if [ $OK -gt 0 ]; then
    shift $OK
  fi
  OK=0

  if [ "$1" = "-genshape" ]; then
    genshape=1;
    OK=1;
  fi

  if [ "$1" = "-runshape" ]; then
    runshape=1;
    OK=1;
  fi

  if [ "$1" = "-createdb" -o "$1" = "-cdb" ]; then
    createdb=1;
    OK=1
  fi

  if [ "$1" = "-dropdb" -o "$1" = "-ddb" ]; then
    dropdb=1;
    OK=1
  fi

  if [ "$1" = "-nsk" ]; then
    nsk=1;
    OK=1
  fi

  if [ "$1" = "-updstats" -o "$1" = "-ust" ]; then
    updstats=1;
    OK=1
  fi

  if [ "$1" = "-rundml" -o "$1" = "-dml" ]; then
    rundml=1;
    OK=1
  fi

  if [ "$1" = "-diff" ]; then
    diffsonly=1;
    OK=1
  fi

  if [ "$1" = "-post" ]; then
    shift
    post=$1;
    OK=1
  fi

done

# ---------------- end of parsing command line options ----------------

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

export BUILD_FLAVOR=`echo $BUILD_FLAVOR | tr a-z A-Z`

# enable NA memory overflow checking during test
if [ "$BUILD_FLAVOR" = "DEBUG" ]; then
  export MEMDEBUG=2
fi

cd $REGRTSTDIR 2>$NULL

testfiles=

if [ $dropdb -eq 1 ]; then
  testfiles=qatddl00
fi

if [ $createdb -eq 1 ]; then
  testfiles="qatddl??"
fi

if [ $updstats -eq 1 ]; then
  testfiles="$testfiles qatust??"
fi

if [ $rundml -eq 1 ]; then
  testfiles="$testfiles qatdml??"
fi

testfiles=`echo $testfiles $*`

if [ "$testfiles" = "" ]; then
  if [ "$seabase" -ne 0 ]; then
    #dont run upd stats tests until support for seabase tables is in
    testfiles=`echo qatddl?? qatdml??`
  else
    testfiles=`echo qatddl?? qatust?? qatdml??`
  fi
fi

echo "Tests:\n$testfiles\n"

cd $REGRRUNDIR 2>$NULL

test $diffsonly -eq 0 && mv -f $rgrlog $rgrlog.bak 2>$NULL
echo "`date +'%D %T'`" > $rgrlog

export SQL_MXCI_CASE_INSENSITIVE_LOG=lowercase

for i in $testfiles; do

  echo
  if [ $diffsonly -ne 0 ]; then
    echo "Diff for test $i"
  else
    echo "Running test $i"
  fi

  tfile=$i
  tfileshp=$REGRRUNDIR/$i.shp
  if [ `uname` = "Linux" -a -f "$REGRTSTDIR/e$i.SB" ]; then
    efile=$REGRTSTDIR/e$i.SB
  elif [ `uname` = "Linux" -a -f "$REGRTSTDIR/e$i.LINUX" ]; then
    efile=$REGRTSTDIR/e$i.LINUX
  else
    efile=$REGRTSTDIR/e$i
  fi
  efileshp=$REGRRUNDIR/e$i.shp
  afile=$REGRRUNDIR/a$i
  dfile=$REGRRUNDIR/d$i
  dfileknown=$REGRTSTDIR/d$i.known
  logtxt=

  # If this test is executing concurrently with other tests, use
  # the parallel execution expected results file if one exists.
  if [ "$REGRCONCURRENT" -eq 1 ]; then
    if [ -r "${efile}-P" ]; then
      efile="${efile}-P"
    fi
  fi

  if [ $diffsonly -eq 0 ]; then
    if [ $genshape -eq 1 ]; then
      rm -f $tfileshp 2>$NULL
      echo "set showshape infile $REGRTSTDIR/$tfile outfile $tfileshp;"
      echo "set showshape infile $REGRTSTDIR/$tfile outfile $tfileshp;" > shape.cmd
      $sqlci -ishape.cmd
      rm -f shape.cmd 2>$NULL
    else
      if [ $runshape -eq 1 ]; then
        echo "$sqlci -i$tfileshp"
        $sqlci -i$tfileshp
      else
        cidefsFile=
        if [ -r "$REGRTSTDIR/cidefs" ]; then
          cidefsFile="$REGRTSTDIR/cidefs"
        fi

        if [ "$REGRCONCURRENT" -eq 1 ]; then
          echo "create schema ${TEST_SCHEMA}; set schema ${TEST_SCHEMA};" \
            | cat $cidefsFile $sbdefsfile - $REGRTSTDIR/$tfile > $tfile.tmp
        else
          cat $cidefsFile $sbdefsfile $REGRTSTDIR/$tfile > $tfile.tmp
        fi
        echo "$sqlci -i$tfile.tmp"
        $sqlci -i$tfile.tmp
        rm -f $tfile.tmp
      fi
    fi
    # save off the LOG file for later use
    if [ $nsk -eq 1 ]; then
      if [ $SQLMX_REGRESS -eq 1 ]; then
        rm -f $afile.MX 2>$NULL
        cp $afile $afile.MX 2>$NULL
      else
        rm -f $afile.MP 2>$NULL
        cp $afile $afile.MP 2>$NULL
      fi
    fi
  fi

  # use MP or MX log file from now on
  if [ $nsk -eq 1 ]; then
    if [ $SQLMX_REGRESS -eq 1 ]; then
      afile=$afile.MX
    else
      afile=$afile.MP
    fi
  fi

  #sort expected result file
  rm -f $efile.srt 2>$NULL
  if [ $runshape -eq 1 ]; then
    echo "$LOGSORT $efile $efile.srt -i >> $NULL"
    $LOGSORT $efile $efile.srt -i >> $NULL
  else
    echo "$LOGSORT $efile $efile.srt -i >> $NULL"
    $LOGSORT $efile $efile.srt -i >> $NULL
  fi

  #sort actual result file
  rm -f $afile.srt 2>$NULL
  echo "$LOGSORT $afile $afile.srt -i >> $NULL"
  $LOGSORT $afile $afile.srt -i >> $NULL

  rm -f $dfile $efile.tmp $afile.tmp
  if [ $nsk -eq 0 ]; then
    echo "$FILTER $efile.srt > $efile.tmp 2>&1"
    echo "$FILTER $afile.srt > $afile.tmp 2>&1"
    $FILTER $efile.srt > $efile.tmp 2>&1
    $FILTER $afile.srt > $afile.tmp 2>&1

    if [ -x "$REGRTSTDIR/FILTERqat" ]; then
      echo "RUNNING SPECIAL FILTER FOR qat"
      echo "$REGRTSTDIR/FILTERqat $efile.tmp > $efile.tmp2"
      echo "$REGRTSTDIR/FILTERqat $afile.tmp > $afile.tmp2"
      $REGRTSTDIR/FILTERqat $efile.tmp > $efile.tmp2
      $REGRTSTDIR/FILTERqat $afile.tmp > $afile.tmp2
    else
      cp -f $efile.tmp $efile.tmp2
      cp -f $afile.tmp $afile.tmp2
    fi

    echo "diff $efile.tmp2 $afile.tmp2 >> $dfile"
    if [ -z "$DUMA_FILL" ] ; then
      diff $efile.tmp2 $afile.tmp2 >> $dfile 2>&1
    else
      diff $efile.tmp2 $afile.tmp2 >> $dfile 2>/dev/null
    fi
  else
    if [ -x "$REGRTSTDIR/FILTERqat" ]; then
      echo "RUNNING SPECIAL FILTER FOR qat"
      echo "$REGRTSTDIR/FILTERqat $efile.srt > $efile.tmp2"
      echo "$REGRTSTDIR/FILTERqat $afile.srt > $afile.tmp2"
      $REGRTSTDIR/FILTERqat $efile.srt > $efile.tmp2
      $REGRTSTDIR/FILTERqat $afile.srt > $afile.tmp2
    else
      cp -f $efile.srt $efile.tmp2
      cp -f $afile.srt $afile.tmp2
    fi

    echo "diff $efile.tmp2 $afile.tmp2 >> $dfile"
    if [ -z "$DUMA_FILL" ] ; then
      diff $efile.tmp2 $afile.tmp2 >> $dfile 2>&1
    else
      diff $efile.tmp2 $afile.tmp2 >> $dfile 2>/dev/null
    fi
  fi
#  rm -f $efile.tmp $afile.tmp $efile.tmp2 $afile.tmp2

  diffsize=`wc -l $dfile`; diffsize=`echo $diffsize | cut -f1 -d' '`

  #----------------------------------------------------------------
  #  Capture filtered files                              --
  #----------------------------------------------------------------
      echo "cp $afile.tmp2 $FilteredLogs/logfiles/$(basename $afile)"
      cp $afile.tmp2 $FilteredLogs/logfiles/$(basename $afile)
      echo "cp $efile.tmp2 $FilteredLogs/expfiles/$(basename $afile)"
      cp $efile.tmp2 $FilteredLogs/expfiles/$(basename $afile)

  # diff the diff and the diff.known files.
  if [ -r "$dfileknown" ]; then
    # Filter known diff file to avoid schema differences
    knownfiltered="$(basename $dfileknown).flt"
    $FILTER $dfileknown > $knownfiltered 2>&1
    dfileknown="$knownfiltered"
  fi
  rm -f $dfile.dif
  diff $dfile $dfileknown >> $dfile.dif 2>&1
  diffofdiffsize=`wc -l $dfile.dif`; diffofdiffsize=`echo $diffofdiffsize | cut -f1 -d' '`
  rm -f $dfile.dif

  #print result
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

  echo $logtxt
  echo "`date +'%D %T'`	$tfile  $logtxt" >> $rgrlog

  # save off the DIFF file for debugging
  if [ $nsk -eq 1 ]; then
    if [ $SQLMX_REGRESS -eq 1 ]; then
      rm -f $dfile.MX 2>$NULL
      cp $dfile $dfile.MX 2>$NULL
    else
      rm -f $dfile.MP 2>$NULL
      cp $dfile $dfile.MP 2>$NULL
    fi
  fi

done # for i in $prettyfiles


echo
echo
echo 'Test Summary'
echo '============'
echo
cat $rgrlog
echo
#test "$post" != "" && $post

