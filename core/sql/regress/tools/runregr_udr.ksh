#!/bin/sh
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

#------------------------------------------------------------------------
# Test driver for SQL/MX regression tests. See the USAGE
# function for supported syntax and options. Currently this
# script is only being used for UDR tests but can probably
# be easily extended to work for other components as well.
#------------------------------------------------------------------------

unset HISTFILE

#
# PROG is the simple name of this script, with directories
# and file extension stripped off. COMPONENT is the name
# of the MX component being tested, e.g. UDR.
#
typeset PROG
PROG=${0##*/}            # strip directories
PROG=${PROG##*\\}
PROG=${PROG%%\.*}        # strip extension
typeset COMPONENT="${REGRBASDIR:-UDR}"
COMPONENT=`echo $COMPONENT | tr a-z A-Z`

LINUX=0
if [ `uname` = "Linux" ]; then
  LINUX=1
fi

USE_NDCS=0
if [ $NSK -eq 1 -o $LINUX -eq 1 ]; then
  USE_NDCS=1
fi

function USAGE
{
    cat <<EOF

 $PROG - Runs SQL/MX $COMPONENT regression tests

 USAGE

  $PROG [options] [files...]

   In no files are specified $PROG will run all tests
   in the current directory with names matching TEST???.

 OPTIONS

    -help              Display this help message
    -diff              Do diffs only
    -dircleanup        Remove all temporary files
    -env               Print all environment variables. Do not run any tests.
    -javachecks        Verify Java components. Do not run any tests.
EOF
}

#
# Function for console output and logging
#
LONG_LINE=\
-------------------------------------------------------------------------------
HEADER_LINE="[$COMPONENT] $LONG_LINE"
HEADER_LINE=`expr substr "$HEADER_LINE" 1 78`
function HEADER
{
    #
    # Print each argument on a separate line. If -log
    # is the first arg then also print to $rgrlog
    #

    typeset -i LOG=0
    if [ "$1" = "-log" ]; then
	LOG=1
	shift
    fi

    typeset TEXT="$(
	echo
	echo "$HEADER_LINE"
	while [ $# -gt 0 ]
	do
	    echo "[$COMPONENT]  $1"
	    shift
	done
	echo "$HEADER_LINE"
    )"

    if [ $LOG -ne 0 ]; then
	echo "$TEXT" | tee -a $rgrlog
    else
	echo "$TEXT"
    fi

}

function PRINT_TEST_FILES
{
    #
    # Function to print test file names. If -log is the first
    # arg then also print to $rgrlog.
    #

    typeset -i LOG=0
    if [ "$1" = "-log" ]; then
	LOG=1
	shift
    fi

    typeset TEXT="$(
	HEADER "Test files"
	printf '%-15s %-15s %-15s %-15s\n' $*
    )"

    if [ $LOG -ne 0 ]; then
	echo "$TEXT" | tee -a $rgrlog
    else
	echo "$TEXT"
    fi
}

function PRINT_SKIP_FILES
{
    #
    # Function to print skipped test file names. If -log is the
    # first arg then also print to $rgrlog.
    #

    typeset -i LOG=0
    if [ "$1" = "-log" ]; then
	LOG=1
	shift
    fi

    typeset TEXT="$(
	HEADER "Skipped files"
	printf '%-15s %-15s %-15s %-15s\n' $*
    )"

    if [ $LOG -ne 0 ]; then
	echo "$TEXT" | tee -a $rgrlog
    else
	echo "$TEXT"
    fi
}

function PRINT_JAVA_COMPONENTS
{
  # empty for now
  true
}

# TESTS that make a JDBC Connection. These tests need ODBC assoc server
# to be started.
#TESTS_NEED_ODBC="TEST100 TEST101 TEST102 TEST103 TEST104 TEST120 TEST300 \
#                 TEST401 TEST402 TEST403 TEST444 TEST470 TEST508 TEST980 "
TESTS_NEED_ODBC=""
typeset MXODBC_USABLE=0
function START_AND_TEST_MXODBC
{
  printf "\n*** INFO: Loging ODBC Startup messages to OdbcStartup.log ***\n"
  printf "*** INFO: Begin-odbc.ksh also logs to startodbc.log ***\n"
  printf "*** INFO: Check both of these log files for clues to startup issues ***\n"

  Begin-odbc.ksh  2>&1 | tee OdbcStartup.log
  BEGIN_ODBC_STATUS=${PIPESTATUS[0]}
  test $BEGIN_ODBC_STATUS -eq 0 && MXODBC_USABLE=1

  MXODBC_ENV_FILE=./odbcenv
  test "$REGRRUNDIR" != "" && MXODBC_ENV_FILE=$REGRRUNDIR/odbcenv
  test $MXODBC_USABLE = 1 &&  . $MXODBC_ENV_FILE
}

function STOP_MXODBC
{
  printf "\n*** INFO: Loging ODBC Shutdown messages to OdbcStop.log ***\n"
  printf "*** INFO: End-odbc.ksh also logs to stopodbc.log ***\n"
  printf "*** INFO: Check both of these log files for clues to startup issues ***\n"

  End-odbc.ksh 2>&1 | tee OdbcStop.log
  MXODBC_USABLE=0
}

#
# Process command-line options
#
typeset -i DIFFS_ONLY=0
typeset -i DIR_CLEANUP=0
typeset -i PRINT_ENV=0
typeset -i USER_WANTS_JAVA_CHECKS=0

while [ $# -gt 0 ] ; do
  case $1 in
    -h|-help)      USAGE
                   exit 1
                   ;;
    -diff)         DIFFS_ONLY=1
                   ;;
    -dircleanup)   DIR_CLEANUP=1
                   ;;
    -env)          PRINT_ENV=1
                   ;;
    -javachecks)   USER_WANTS_JAVA_CHECKS=1
                   ;;
    -*)            echo "\n*** Unknown option: $1"
                   USAGE
                   exit 1
                   ;;
    *)             break
                   ;;
  esac
  shift
done

#
# Cleanup and exit if -dircleanup was specified
#
if [ $DIR_CLEANUP -ne 0 ]; then
  echo "\n Cleaning up temporary files in $(pwd)...\n"
  rm -f *.class *.flt *.tflt *.srt *.bak *.dif 2>$NULL
  rm -f *.tmp *.data *.token *~ 2>$NULL
  rm -f *.pdb *.dll *.exp *.lib *.obj *.ilk *.exe *.is *.mdf *.pdb
  rm -f DIFF??? 2>$NULL
  exit 0
fi

#
# Print environment and exit if -env was specified
#
function WRITE_ENV
{
    typeset ENV_FILE="$(pwd)/$PROG.env"
    set >| "$ENV_FILE"

    PRINT_JAVA_COMPONENTS >> "$ENV_FILE"

    echo
    echo "Environment settings written to $ENV_FILE"
    echo

}
if [ $PRINT_ENV -ne 0 ]; then
  WRITE_ENV
  exit 0
fi

#
# Do Java checks and exit if the user specified -javachecks and did not
# specify any test names
#
if [ $USER_WANTS_JAVA_CHECKS -ne 0 -a $# -eq 0 ]; then
  PRINT_JAVA_COMPONENTS
  exit 0
fi

#
# Make sure certain environment variables are defined
#
function CHECK_ENV
{
    typeset -i OK=1
    typeset VARS="
	REGRBASDIR REGRTSTDIR REGRRUNDIR
	REGRBASDIR_Q REGRTSTDIR_Q REGRRUNDIR_Q
	NSK LOGSORT FILTER NULL
        mxci mxcmp mxudr rgrlog java javac
    "
    for NAME in $VARS
    do
	typeset VALUE
	eval VALUE="\$$NAME"
	if [ "$VALUE" = "" ]; then
	    echo "\n *** ERROR: Required variable $NAME is not defined"
	    OK=0
	fi
    done
    if [ $OK -eq 0 ]; then
	WRITE_ENV
	exit
    fi
}
CHECK_ENV




#
# Build a list of test files. The PRETTY_FILES function
# converts any abbreviated test names such as "100" to the
# full name, e.g. "TEST100", and stores the new list in the
# global variable PFILES. If no test files are given on
# the command line then use all files matching TEST???.
#
function PRETTY_FILES
{
    PFILES=
    cd "$REGRTSTDIR" 2>$NULL || return
    for F in $@
    do
	if [ "$(expr substr $F 1 4)" = "TEST" ]; then
	    PFILES="$PFILES $F"
	else
	    F="$(printf %03s $F)"
	    PFILES="$PFILES TEST$F"
	fi
    done
    PFILES=`echo $PFILES | tr a-z A-Z`
    return
}

typeset TESTFILES="$*"
TESTFILES=`echo $TESTFILES | tr a-z A-Z`
if [ "$TESTFILES" = "" ]; then
  TESTFILES="TEST???"
fi
PRETTY_FILES "$TESTFILES"
TESTFILES="$PFILES"

#
# The next several steps are to process skipped files. First we build
# a list of tests to skip in SKIPFILES. Then we rebuild TESTFILES and
# SKIPFILES, removing anything from TESTFILES that is in SKIPFILES and
# removing anything from SKIPFILES that was not originally in
# TESTFILES.
#
typeset SKIPFILES

#
# Build a list of files to skip. The list may be different for debug
# and release builds so we create an IS_RELEASE flag first.
#
typeset -i IS_RELEASE=0
if [ "$BUILD_FLAVOR" = "release" -o "$BUILD_FLAVOR" = "RELEASE" ]; then
  IS_RELEASE=1
fi

#
# Notes on skipped tests
#
#
# TEST402, TEST444, TEST810 - Debug build only
#
# TEST811 - Release build only. This test calls sections of TEST810
# under user identity sql.user1.
#
# TEST506 - Security Test. Skipped on NSK DEBUG.
#
#
# TEST981  - mxtool incorrectly reports crashopen on some files randomly
SKIPFILES="$SKIPFILES TEST165 TEST210 TEST800 TEST702 TEST703 TEST981"

if [ $IS_RELEASE -ne 0 ]; then
    SKIPFILES="$SKIPFILES TEST402 TEST444"
else
  if [ $NSK -eq 1 -o $LINUX -eq 1 ]; then
    SKIPFILES="$SKIPFILES TEST444"
  else
    SKIPFILES="$SKIPFILES"
  fi
fi

if [ $NSK -eq 1 -o $LINUX -eq 1 ]; then
    # TEST401 creates ZZSA files and they are not getting removed
    # after the test completes in official run. Disabling it until
    # that's resolved.
    SKIPFILES="$SKIPFILES TEST401 TEST700 TEST810 TEST811 TEST900"

    # Following tests are disabled for now. There are some issues with
    # ODBC/Type4 code and these tests are failing because of that.
    # These tests will be enabled once the ODBC/Type4 issue is resolved.
    SKIPFILES="$SKIPFILES TEST404 TEST450 TEST460 "
else
    SKIPFILES="$SKIPFILES TEST508 TEST103 TEST444 TEST750 TEST981 TEST982 TEST983"
fi

# TEST400 halts cpu. This problem is being fixed. Skip it until it's
# fixed.
if [ `uname -r` = H06 ]; then
    SKIPFILES="$SKIPFILES TEST400 "
fi

if [ $LINUX -eq 1 ]; then
  SKIPFILES="$SKIPFILES TEST150 TEST400 TEST402 TEST505 TEST508 TEST750 TEST982"
fi

#skip checkTest tests if they have already been run
if [ "$CHECK_TEST2" == "1" ]; then
    SKIPFILES="$SKIPFILES $udrCT"
fi

# 3/26/12 Security scrum:  Skip all tests on NT for now.  Changes for Secure JARs not compatible
#                          with the existing testware on NT.
if [ $LINUX -eq 0 ]; then
  SKIPFILES="$TESTFILES"
fi

#
# The name of an expected file for release build has an "R" appended
#
if [ $IS_RELEASE -ne 0 ]; then
    exp_suffix=R
else
    exp_suffix=
fi

#
# Now rebuild TESTFILES and SKIPFILES
#
TF=`echo "$TESTFILES" | tr a-z A-Z`
SF=`echo "$SKIPFILES" | tr a-z A-Z`
TESTFILES=
SKIPFILES=
for T in $TF
do
    typeset -i SKIP_IT=0
    for S in $SF
    do
	if [ "$S" = "$T" ]; then
	    SKIP_IT=1
	    break
	fi
    done
    if [ $SKIP_IT -eq 0 ]; then
	TESTFILES="$TESTFILES $T"
    else
	SKIPFILES="$SKIPFILES $T"
    fi
done

seabase="$SEABASE_REGRESS"
# sbtestfiles contains the list of tests to be run in seabase mode
if [ "$seabase" -ne 0 ]; then
  TESTFILES="TEST001 TEST002 TEST100 TEST101 TEST102 TEST103 TEST107 TEST108 TEST163"
  SBPFILES=
  for i in $PFILES; do
    for j in $TESTFILES; do
       if [ $i = $j ]; then
          SBPFILES="$SBPFILES $i";
       fi
    done
  done
  PFILES=$SBPFILES
  TESTFILES=$PFILES
fi

#
# Remove the old log file
#
mv -f $rgrlog $rgrlog.bak 2>$NULL

#
# Print the log header
#
HEADER "BEGIN $(date)" "Current directory: $(pwd)" \
            "Log file: $rgrlog"

HEADER "Executables"
ls -l "$sqlci" "$mxcmp" "$mxudr"
ls -lL "$java" "$javac"

PRINT_JAVA_COMPONENTS

#
# Print test file names
#
PRINT_TEST_FILES $TESTFILES
if [ "$SKIPFILES" != "" ]; then
    PRINT_SKIP_FILES $SKIPFILES
fi

#
# Now do the tests and diffs. Functions DO_DIFF, DO_FILTER,
# and DO_TEST do the real work. Below these functions is a
# simple loop that just walks through the list of test files
# and calls DO_TEST and/or DO_DIFF. DO_FILTER is a helper
# function called by DO_DIFF.
#

function DO_FILTER
{
    typeset TESTNUM="$1"
    typeset SORTED="$2"
    typeset BASE="${SORTED%.*}"
    rm -f $BASE.flt $BASE.tflt

    #
    # First see if there is a test-specific filter. Run it, and store
    # the output in a .tflt file
    #
    if [ -e "$REGRTSTDIR/FILTER$TESTNUM" -a -s "$REGRTSTDIR/FILTER$TESTNUM" ]; then
	echo "sh $REGRTSTDIR/FILTER$TESTNUM $SORTED > $BASE.tflt"
	sh $REGRTSTDIR/FILTER$TESTNUM $SORTED 2>&1 > $BASE.tflt
	SORTED="$BASE.tflt"
    fi

    echo "sh $FILTER $SORTED > $BASE.flt"
    sh $FILTER $SORTED 2>&1 > $BASE.flt
}

function DO_DIFF
{
    typeset T=$1
    cd $REGRRUNDIR

    HEADER "Starting diff for $T"

    #
    # Required files: EXPECTEDxxx, LOGxxx
    #

    typeset NUM=${T#TEST}
    typeset EXP=EXPECTED$NUM
    typeset LOG=LOG$NUM
    typeset DIF=DIFF$NUM
    typeset FILT=FILTER$NUM
    efile=""

    if [ $NSK -ne 0 ]; then
      if [ -r $REGRTSTDIR/$EXP.nsk$exp_suffix ]; then
        efile=$REGRTSTDIR/$EXP.nsk$exp_suffix
      elif [ -r $REGRTSTDIR/$EXP.nsk ]; then
        efile=$REGRTSTDIR/$EXP.nsk
      fi
    elif [ $SEABASE_REGRESS -ne 0 ]; then
      if [ -r $REGRTSTDIR/$EXP.SB$exp_suffix ]; then
          efile=$REGRTSTDIR/$EXP.SB$exp_suffix
      elif [ -r $REGRTSTDIR/$EXP.SB ]; then
          efile=$REGRTSTDIR/$EXP.SB
      fi
    elif [ `uname` = "Linux" ]; then
      if [ -r $REGRTSTDIR/$EXP.LINUX$exp_suffix ]; then
        efile=$REGRTSTDIR/$EXP.LINUX$exp_suffix
      elif [ -r $REGRTSTDIR/$EXP.LINUX ]; then
        efile=$REGRTSTDIR/$EXP.LINUX
      fi
    fi

    if [ -z "$efile" ]; then
      efile=$REGRTSTDIR/$EXP
    fi

    # If this test is executing concurrently with other tests, use
    # the parallel execution expected results file if one exists.
    if [ "$REGRCONCURRENT" -eq 1 ]; then
      if [ -r "${efile}-P" ]; then
        efile="${efile}-P"
      fi
    fi

    #
    # Required files: LOGxxx, EXPECTEDxxx
    #
    typeset -i OK=1
    if [ ! -r $LOG ]; then
	echo "\n *** ERROR: Log file $LOG not found"
	OK=0
    fi
    if [ ! -r "$efile" ]; then
      echo "\n *** ERROR: Expected file $efile not found"
      OK=0
    fi
    if [ $OK -eq 0 ]; then
	RESULT="### FAIL (missing files) ###"
	printf "\n$RESULT\n"
	printf "$(date '+%m/%d/%Y %R')  $T  $RESULT\n" >> $rgrlog
	return
    fi

    #
    # Sort the expected result file
    #
    rm -f $EXP.srt 2>$NULL
	echo "$LOGSORT $efile $EXP.srt >> $NULL"
	$LOGSORT $efile $EXP.srt >> $NULL

    #
    # Sort the actual result file
    #
    rm -f $LOG.srt 2>$NULL
    echo "$LOGSORT $LOG $LOG.srt >> $NULL"
    $LOGSORT $LOG $LOG.srt >> $NULL

    #
    # Filter the sorted files
    #
    DO_FILTER $NUM $EXP.srt
    DO_FILTER $NUM $LOG.srt

    #----------------------------------------------------------------
    #  Capture filtered files                              --
    #----------------------------------------------------------------
    echo "cp $LOG.flt $FilteredLogs/logfiles/LOG$NUM"
    cp $LOG.flt $FilteredLogs/logfiles/LOG$NUM
    echo "cp $EXP.flt $FilteredLogs/expfiles/LOG$NUM"
    cp $EXP.flt $FilteredLogs/expfiles/LOG$NUM

    #
    # Diff the filtered files
    #
    rm -f $DIF
    echo "diff $EXP.flt $LOG.flt > $DIF"
    if [ -z "$DUMA_FILL" ] ; then
      diff $EXP.flt $LOG.flt 2>&1 > $DIF
    else
      diff $EXP.flt $LOG.flt 2>/dev/null > $DIF
    fi

    #
    # Get the number of diff lines
    #
    typeset -i DIFF_SIZE="$(cat $DIF | wc -l)"

    #
    # Diff the diff output and the known diff file
    #
    typeset -i DIFFS_ARE_KNOWN=0
    typeset -i DIFF_SIZE2=0
    typeset KNOWN_FILE=$REGRTSTDIR/$DIF.known

    # For LINUX, look for linux specific known file
    if [ $LINUX -eq 1 ]; then
      if [ -r       $REGRTSTDIR/$DIF.KNOWN.LINUX ]; then
         KNOWN_FILE=$REGRTSTDIR/$DIF.KNOWN.LINUX
      fi
    fi

    # For SEABASE, look for SEABASE specific known file
    if [ $seabase -ne 0 ]; then
      if [ -r       $REGRTSTDIR/$DIF.KNOWN.SB ]; then
         KNOWN_FILE=$REGRTSTDIR/$DIF.KNOWN.SB
      fi
    fi

    # For YOS, look for yos specific known file
    if [ `uname -r` = H06 ]; then
      if [ $NSK -ne 0 -a -r $KNOWN_FILE.yos.release -a $IS_RELEASE -ne 0 ]; then
	  KNOWN_FILE=$KNOWN_FILE.yos.release
      elif
         [ $NSK -ne 0 -a -r $KNOWN_FILE.yos.debug -a $IS_RELEASE -eq 0 ]; then
          KNOWN_FILE=$KNOWN_FILE.yos.debug
      elif
         [ $NSK -ne 0 -a -r $KNOWN_FILE.yos ]; then
          KNOWN_FILE=$KNOWN_FILE.yos
      fi
    fi

    # For MIPS and YOS(if yos specific known file does not exist), look for
    # nsk specific known files.
    if [ `uname -r` != H06 -o $KNOWN_FILE = $REGRTSTDIR/$DIF.known ]; then
      if [ $NSK -ne 0 -a -r $KNOWN_FILE.nsk.release -a $IS_RELEASE -ne 0 ]; then
          KNOWN_FILE=$KNOWN_FILE.nsk.release
      elif
         [ $NSK -ne 0 -a -r $KNOWN_FILE.nsk.debug -a $IS_RELEASE -eq 0 ]; then
          KNOWN_FILE=$KNOWN_FILE.nsk.debug
      elif
         [ $NSK -ne 0 -a -r $KNOWN_FILE.nsk ]; then
          KNOWN_FILE=$KNOWN_FILE.nsk
      fi
    fi

    if [ $DIFF_SIZE -ne 0 -a -r $KNOWN_FILE ]; then
	DIFFS_ARE_KNOWN=1
	rm -f $DIF.dif
        echo "$FILTER $DIF > $DIF.flt 2>&1"
        $FILTER $DIF > $DIF.flt 2>&1
        # Filter known diff file to avoid schema differences
        known_filtered="$(basename $KNOWN_FILE).flt"
        echo "$FILTER $KNOWN_FILE > $known_filtered 2>&1"
        sh $FILTER $KNOWN_FILE > $known_filtered 2>&1
	echo "diff $DIF.flt $known_filtered > $DIF.dif"
	diff $DIF.flt $known_filtered > $DIF.dif 2>&1
	DIFF_SIZE2="$(cat $DIF.dif | wc -l)"
    fi

    #
    # Print the result
    #
    typeset result
    if [ $DIFFS_ARE_KNOWN -eq 0 ]; then
	if [ $DIFF_SIZE -eq 0 ]; then
	    RESULT="### PASS ###"
	else
	    RESULT="### FAIL ($DIFF_SIZE lines) ###"
	fi
    else
	if [ $DIFF_SIZE2 -eq 0 ]; then
	    RESULT="### PASS with known diffs ###"
	else
	    RESULT="### FAIL ($DIFF_SIZE lines vs. $DIFF_SIZE2 known) ###"
	fi
    fi

#    printf "$RESULT\n"
#    printf "$(date '+%m/%d/%Y %R')    $T\t  $RESULT\n" >> $rgrlog
    modtime=`stat --printf=%y $LOG | cut -d'.' -f1`
    printf "$modtime  $T  $RESULT\n" >> $rgrlog
}

function DO_TEST
{
    typeset T="$1"
    cd $REGRRUNDIR

    if [ "$REGRTSTDIR" != "$REGRRUNDIR" ]; then
       cp -f $REGRTSTDIR/$T $REGRRUNDIR/$T 2>$NULL
       if [ "$T" = "TEST811" ]; then
         HEADER "COPYING TEST810 over for you"
         cp -f $REGRTSTDIR/TEST810 $REGRRUNDIR/TEST810 2>$NULL
       fi
    fi

    # create a tempfile which is used as timestamp for deleting
    # saveabend files after the test completes
    touch TimestampFile

    HEADER "Starting test $T"
    if [ -r "$REGRTSTDIR/$T" ]; then
      if [ -r $rundir/tools/userdefs ]; then
        defsfile="$rundir/tools/userdefs"
      fi
      if [ "$REGRCONCURRENT" -eq 1 ]; then
        echo "create schema ${TEST_SCHEMA}; set schema ${TEST_SCHEMA};" \
          | cat $defsfile - $REGRTSTDIR/$T > $T.tmp
      else
        cat $defsfile $sbdefsfile $REGRTSTDIR/$T > $T.tmp
      fi
      $mxci -i$T.tmp
    else
	echo "\n *** ERROR: File $T not found"
    fi

    if [ "$REGRTSTDIR" != "$REGRRUNDIR" ]; then
      rm -f $REGRRUNDIR/$T 2>$NULL
      if [ "$T" = "TEST811" ]; then
        rm -f $REGRRUNDIR/TEST810 2>$NULL
      fi
    fi

    # TEST401 creates saveabend files purposefully. Remove them
    # after the test completes.
    if [ "$T" = "TEST401" ]; then
      find . -name "ZZSA*" -newer TimestampFile -exec rm -f {} \;
    fi
    rm -f TimestampFile
}

#
# This loop does all the real work...
#
cd $REGRRUNDIR 2>$NULL

if [ "$REGRCONCURRENT" -eq 1 ]; then
  test_suite=${REGRBASDIR:?'REGRBASDIR is undefined'}
  export TEST_CATALOG='cat'
  export TEST_SCHEMA_NAME="$test_suite"
else
  export TEST_CATALOG='cat'
  export TEST_SCHEMA_NAME='sch'
fi


sbdefsfile=
if [ "$SEABASE_REGRESS" -ne 0 ]; then
  export TEST_CATALOG='TRAFODION'
  seabase="$SEABASE_REGRESS"

  if [ -r $REGRTOOLSDIR/sbdefs ]; then
     sbdefsfile="$REGRTOOLSDIR/sbdefs"
  fi
fi

export TEST_SCHEMA="$TEST_CATALOG.$TEST_SCHEMA_NAME"
export QTEST_SCHEMA_NAME="'$TEST_SCHEMA_NAME'"
export QUOTEDLIBNAME="'$REGRRUNDIR/$T.dll'"; # For now include DLL name.

# Java properties file used by TEST403 and TEST404
echo "schema=$TEST_SCHEMA" >schema.prop

echo "$(date '+%m/%d/%Y %R')" >> $rgrlog

export JDBC_T4_URL="jdbc:t4jdbc://localhost:23400/:"
if [ -r $TRAF_HOME/sql/scripts/sw_env.sh ]; then
  # use a custom port for the JDBC Type 4 driver
  . $TRAF_HOME/sql/scripts/sw_env.sh
  export JDBC_T4_URL="jdbc:t4jdbc://localhost:${MY_DCS_MASTER_PORT}/:"
fi

# if the regressions are running on NSK, start the ODBC server if
# we are not running diffs_only
if [ $USE_NDCS -ne 0 -a $DIFFS_ONLY -eq 0 -a $SEABASE_REGRESS -eq 0 ]; then
  START_AND_TEST_MXODBC
fi

if [ $DIFFS_ONLY -eq 0 ]; then
   if [ "$REGRTSTDIR" != "$REGRRUNDIR" ]; then
      echo "copying FILTER_TIME.AWK to $REGRRUNDIR"
      cp -f $REGRTSTDIR/FILTER_TIME.AWK $REGRRUNDIR 2>$NULL
   fi
fi

for T in $TESTFILES
do
    if [ $DIFFS_ONLY -ne 0 ]; then
	DO_DIFF "$T"
    else
	# on NSK, make sure MXOAS is started for the tests
	# that need a JDBC type4 connection
        TEST_CAN_BE_RUN=1
	for S in $TESTS_NEED_ODBC
	do
           if [ "$T" = "$S" -a $USE_NDCS -ne 0 -a $MXODBC_USABLE -ne 1 -a $SEABASE_REGRESS -eq 0 ]; then
             TEST_CAN_BE_RUN=0
	     RESULT="### FAIL (!! Test SKIPPED because NDCS SETUP is NOT RIGHT !! ) ###"
	     printf "\n$RESULT\n"
	     printf "$(date '+%m/%d/%Y %R')  $T  $RESULT\n" >> $rgrlog
	     break;
           fi
        done

        if [ $TEST_CAN_BE_RUN -eq 1 ]; then
	  DO_TEST "$T"
	  DO_DIFF "$T"
        fi
    fi
done
#echo "$(date '+%m/%d/%Y %R')" >> $rgrlog

# stop the ODBC server
if [ $MXODBC_USABLE -eq 1 ]; then
  STOP_MXODBC
fi

PRINT_JAVA_COMPONENTS >> $rgrlog

#
# Print the summary of this test run
#
HEADER "$COMPONENT Test Summary"
if [ -r $rgrlog ]; then
    echo
    cat $rgrlog
    echo
else
    echo "\n *** WARNING: Log file $rgrlog not found\n"
fi

