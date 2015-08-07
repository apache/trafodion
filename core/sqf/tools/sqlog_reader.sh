#!/bin/bash
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

export MY_LOGROOT=$PWD

# head of the monitor.map files
function lmaph {
    l_nl=1
    if [ $# != 0 ]; then
	l_nl=$1
    fi
    
    head --quiet --lines $l_nl $MY_LOGROOT/logs/monitor.map.[0-9]*.* | sort -k1
}

# tail of the monitor.map.[0-9]*.* files
function lmapt {
    l_nl=1
    if [ $# != 0 ]; then
	l_nl=$1
    fi
    
    tail --quiet --lines $l_nl $MY_LOGROOT/logs/monitor.map.[0-9]*.* | sort -k1
}

# lookup the monitor.map.[0-9]*.* files
function lmapl {
    if [ $# == 0 ]; then
	echo "Enter a string to lookup in all the monitor.map.[0-9]*.* files"
	return 1;
    fi
    l_lookup=$*
    grep "$l_lookup" $MY_LOGROOT/logs/monitor.map.[0-9]*.*
}

function lmaplc {

    lmapl $* | wc -l
}

function lmaplbc {

    lmapl $* | grep BEGIN | wc -l
}

function lmaplec {

    lmapl $* | grep END | wc -l
}

function lmaplbec {
    l_lookup=$*

    lv_all=`lmaplc $*`
    lv_begin=`lmaplbc $*`
    lv_end=`lmaplec $*`

    echo "$l_lookup,$lv_all,$lv_begin,$lv_end"
    
}

# ===== BEGIN ======= lookup the monitor.map.[0-9]*.* files for ESPs 
function lmapls {
    if [ $# == 0 ]; then
	echo "Enter a string to lookup in all the monitor.map.[0-9]*.* files"
	return 1;
    fi
    l_lookup=$*
    grep "$l_lookup" $MY_LOGROOT/logs/monitor.map.[0-9]*.* | grep tdm_arkesp
}

function lmaplsc {

    lmapls $* | wc -l
}

function lmaplsbc {

    lmapls $* | grep BEGIN | wc -l
}

function lmaplsec {

    lmapls $* | grep END | wc -l
}

function lmaplsbec {
    l_lookup=$*

    lv_all=`lmaplsc $*`
    lv_begin=`lmaplsbc $*`
    lv_end=`lmaplsec $*`

    echo "$l_lookup,$lv_all,$lv_begin,$lv_end"
    
}
# =======  END ======= lookup the monitor.map.[0-9]*.* files for ESPs

# Count the number of processes started (in the lifetime of this SQ env) for a particular program string - defaults to arkesp
function lmappc {
    l_nl=arkesp
    if [ $# != 0 ]; then
	l_nl=$1
    fi
    
    grep -h $l_nl $MY_LOGROOT/logs/monitor.map.[0-9]*.* | grep BEGIN | wc -l | sort -nk2
}


function lmapc {
    
    grep $1 $MY_LOGROOT/logs/monitor.map* | grep $2 | wc -l
}

function lpdsh_counter {
    awk 'BEGIN {cnt=0} {cnt=cnt+$1} END {print cnt;}'
}

function ltnbprog {
    if [ $# != 0 ]; then
        lv_prog=$1
    else
        lv_prog=arkesp
    fi
    lmapc BEGIN $lv_prog | lpdsh_counter
}

function ltneprog {
    if [ $# != 0 ]; then
        lv_prog=$1
    else
        lv_prog=arkesp
    fi
    lmapc END $lv_prog | lpdsh_counter
}

function ldp2stats {
   lv_nbegin=`ltnbprog dp2`
   lv_nend=`ltneprog dp2`
   echo "`date`;Total started: $lv_nbegin;  Total exitted: $lv_nend;"
}

function lespstats {
   lv_nbegin=`ltnbprog tdm_arkesp`
   lv_nend=`ltneprog tdm_arkesp`
   echo "`date`; Total started: $lv_nbegin; Total exitted: $lv_nend;"
}

function lsqlcistats {
   lv_nbegin=`ltnbprog sqlci`
   lv_nend=`ltneprog sqlci`
   echo "`date`; SQLCIs running: $lv_ncurr; Total started: $lv_nbegin; Total exitted: $lv_nend;"
}

function lcmpstats {
   lv_nbegin=`ltnbprog tdm_arkcmp`
   lv_nend=`ltneprog tdm_arkcmp`
   echo "`date`; Total started: $lv_nbegin; Total exitted: $lv_nend;"
}

function lmxosstats {
   lv_nbegin=`ltnbprog mxosrvr`
   lv_nend=`ltneprog mxosrvr`
   echo "`date`; Total started: $lv_nbegin; Total exitted: $lv_nend;"
}

# check the startup log and sort the interesting even chronologically
function lsqchksl {
    
    cd $MY_LOGROOT/logs; grep Executing startup*.log 2>/dev/null | sort -k4 -k5
    cd -
}

function lsqchkmpi {
    cd $MY_LOGROOT/logs; egrep -i '(mpi bug|ibv_create)' *.log
}

#The following allows the above functions to be used from some other shell scripts spawned from this shell
export -f lmaph
export -f lmapt
export -f lmapl
export -f lmaplc
export -f lmaplbc
export -f lmaplec
export -f lmaplbec

# for ESPs
export -f lmapls
export -f lmaplsc
export -f lmaplsbc
export -f lmaplsec
export -f lmaplsbec

export -f lmappc

export -f lsqchkmpi

export -f lmapc
export -f lpdsh_counter
export -f ltnbprog
export -f ltneprog
export -f lespstats
export -f lsqlcistats
export -f lcmpstats
export -f lmxosstats
export -f ldp2stats

export -f lsqchksl

export PATH=$PATH:~/bin
