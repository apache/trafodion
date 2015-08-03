#!/bin/bash
###############################################################################
#
# File:         bldFunctions.sh
# Description:  Common functions used in build scripts for Seaquest
#
# Created:      07 Mar 2007
# Language:     bash
#
#
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
#
###############################################################################

#---
# usage()
#---

if [[ $(basename $0) == "bldFunctions.sh" ]]; then
   echo ""
   echo "$0 - this is not intended as an executable script - it contains"
   echo "     support functions only"
   echo ""

   exit 0
fi

###############################################################################
# Helper function to log operations performed by the nightly script to a
# log file and history file.
###############################################################################

function log_it {
  echo -e "=== $*" | tee -a $LOGFILE
}

###############################################################################
# Helper function to log operations performed by the nightly script to a log file
# and history file.  This avoids the character processing done by echo in log_it.
###############################################################################
function log_it_r {
  echo -e "=== $*" | tee -a $LOGFILE $historyFilename
}	# function-end log_it_r

###############################################################################
# Helper function to run a tool appending standard out and standard error to
# the log file and history files, and return the tool's return code.
# This does not show output on the standard out of the shell which started this.
###############################################################################
function run_tool_quiet {
    #
    # Next lines send back $1's return code, not tee's.
    #
    typeset EXIT_STATUS_FILE=${LOGDIR}/exit_status_$$
    typeset STATUS=1
    rm -f $EXIT_STATUS_FILE
    ($* 2>&1 ; echo $? > $EXIT_STATUS_FILE) >> $LOGFILE
    if [[ -s $EXIT_STATUS_FILE ]]; then
	STATUS=$(< $EXIT_STATUS_FILE)
    fi
    rm -f $EXIT_STATUS_FILE
    return $STATUS
}	# function-end run_tool_quiet

###############################################################################
# Helper function to run a tool appending standard out and standard error to
# the log file and history files, and return the tools return code.
###############################################################################

function run_tool {
    #
    # Next lines send back $1's return code, not tee's.
    #
    typeset EXIT_STATUS_FILE=${LOGDIR}/exit_status_$$
    typeset STATUS=1
    rm -f $EXIT_STATUS_FILE

    ($* 2>&1 ; echo $? > $EXIT_STATUS_FILE) | tee -a $LOGFILE $EXTRA_OUT
    if [[ -s $EXIT_STATUS_FILE ]]; then
	STATUS=$(< $EXIT_STATUS_FILE)
    fi
    rm -f $EXIT_STATUS_FILE
    return $STATUS
}

###############################################################################
# Helper function to run $1, showing the actual command and logs standard out
# and standard error in the build log file, and returns the tool's return code.
###############################################################################

function run_and_log {
    log_it "$*"

    run_tool $*
    return $?
}


###############################################################################
# Helper function to put a visible warning message in the log without stopping.
###############################################################################
function bold_warning {
    log_it
    log_it_r '**************************************************'
    log_it_r '*'
    log_it_r '*' $*
    log_it_r '*'
    log_it_r '**************************************************'
    log_it
    sleep 5
}	# function-end bold_warning


###############################################################################
# Helper function to put a visible warning message in the log without stopping.
# This is more general purpose than bold_warning().
###############################################################################
function bold_message {
    typeset CTR=0
    typeset ARGS_SEEN=$#
    log_it
    log_it_r '**************************************************'
    log_it_r '*'
    while [[ $CTR -lt $ARGS_SEEN ]]; do
	log_it_r '*' $1
	CTR=$(( CTR + 1 ))
	shift 1
    done
    log_it_r '*'
    log_it_r '**************************************************'
    log_it
}	# function-end bold_message

