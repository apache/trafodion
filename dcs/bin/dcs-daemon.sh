#!/usr/bin/env bash
#/**
# @@@ START COPYRIGHT @@@
#
#Licensed to the Apache Software Foundation (ASF) under one
#or more contributor license agreements.  See the NOTICE file
#distributed with this work for additional information
#regarding copyright ownership.  The ASF licenses this file
#to you under the Apache License, Version 2.0 (the
#"License"); you may not use this file except in compliance
#with the License.  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#Unless required by applicable law or agreed to in writing,
#software distributed under the License is distributed on an
#"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#KIND, either express or implied.  See the License for the
#specific language governing permissions and limitations
#under the License.
#
# @@@ END COPYRIGHT @@@
# */
# Runs a DCS command as a daemon.
#
# Environment Variables
#
#   DCS_CONF_DIR   Alternate DCS conf dir. Default is ${DCS_HOME}/conf.
#   DCS_LOG_DIR    Where log files are stored.  PWD by default.
#   DCS_PID_DIR    The pid files are stored. ${DCS_HOME}/tmp by default.
#   DCS_IDENT_STRING   A string representing this instance. $USER by default
#   DCS_NICENESS The scheduling priority for daemons. Defaults to 0.
#

usage="Usage: dcs-daemon.sh [--config <conf-dir>]\
 (start|stop|restart) <dcs-command> \
 <args...>"

# if no args specified, show usage
if [ $# -le 1 ]; then
  echo $usage
  exit 1
fi

bin=`dirname "${BASH_SOURCE-$0}"`
bin=`cd "$bin">/dev/null; pwd`

. "$bin"/dcs-config.sh

# get arguments
startStop=$1
shift

command=$1
shift

instance=$1
case $instance in
    ''|*[!0-9]*) instance=1 ;; #bad
    *) ;; #good
esac

dcs_rotate_log ()
{
    log=$1;
    num=5;
    if [ -n "$2" ]; then
    num=$2
    fi
    if [ -f "$log" ]; then # rotate logs
    while [ $num -gt 1 ]; do
        prev=`expr $num - 1`
        [ -f "$log.$prev" ] && mv -f "$log.$prev" "$log.$num"
        num=$prev
    done
    mv -f "$log" "$log.$num";
    fi
}

wait_until_done ()
{
    p=$1
    cnt=${DCS_SLAVE_TIMEOUT:-300}
    origcnt=$cnt
    while kill -0 $p > /dev/null 2>&1; do
      if [ $cnt -gt 1 ]; then
        cnt=`expr $cnt - 1`
        sleep 1
      else
        echo "Process did not complete after $origcnt seconds, killing."
        kill -9 $p
        exit 1
      fi
    done
    return 0
}

# get log directory
if [ "$DCS_LOG_DIR" = "" ]; then
  export DCS_LOG_DIR="$DCS_HOME/logs"
fi
mkdir -p "$DCS_LOG_DIR"

if [ "$DCS_PID_DIR" = "" ]; then
  DCS_PID_DIR="$DCS_HOME/tmp"
fi

#if [ "$DCS_IDENT_STRING" = "" ]; then
  export DCS_IDENT_STRING="$USER-$instance"
#fi

# Some variables
# Work out java location so can print version into log.
if [ "$JAVA_HOME" != "" ]; then
  #echo "run java in $JAVA_HOME"
  JAVA_HOME=$JAVA_HOME
fi
if [ "$JAVA_HOME" = "" ]; then
  echo "Error: JAVA_HOME is not set."
  exit 1
fi
JAVA=$JAVA_HOME/bin/java
export DCS_LOG_PREFIX=dcs-$DCS_IDENT_STRING-$command-$HOSTNAME
export DCS_LOGFILE=$DCS_LOG_PREFIX.log
export DCS_ROOT_LOGGER="INFO,DRFA"
export DCS_SECURITY_LOGGER="INFO,DRFAS"
logout=$DCS_LOG_DIR/$DCS_LOG_PREFIX.out  
loggc=$DCS_LOG_DIR/$DCS_LOG_PREFIX.gc
loglog="${DCS_LOG_DIR}/${DCS_LOGFILE}"
pid=$DCS_PID_DIR/dcs-$DCS_IDENT_STRING-$command.pid

if [ "$DCS_USE_GC_LOGFILE" = "true" ]; then
  export DCS_GC_OPTS=" -Xloggc:${loggc}"
fi

# Set default scheduling priority
if [ "$DCS_NICENESS" = "" ]; then
    export DCS_NICENESS=0
fi

case $startStop in

  (start)
    mkdir -p "$DCS_PID_DIR"
    if [ -f $pid ]; then
      if kill -0 `cat $pid` > /dev/null 2>&1; then
        echo $command  `cat $pid`.  Stop it first.
        exit 1
      fi
    fi

    dcs_rotate_log $logout
    dcs_rotate_log $loggc
    echo starting $command, logging to $logout
    # Add to the command log file vital stats on our environment.
    # echo "`date` Starting $command on `hostname`" >> $loglog
    # echo "`ulimit -a`" >> $loglog 2>&1
    nohup nice -n $DCS_NICENESS "$DCS_HOME"/bin/dcs \
        --config "${DCS_CONF_DIR}" \
        $command "$@" $startStop > "$logout" 2>&1 < /dev/null &
    echo $! > $pid
    sleep 1; head "$logout"
    ;;

  (stop)
    if [ -f $pid ]; then
      # kill -0 == see if the PID exists 
      if kill -0 `cat $pid` > /dev/null 2>&1; then
        echo -n stopping $command
        # echo "`date` Terminating $command" >> $loglog
        kill `cat $pid` > /dev/null 2>&1
        while kill -0 `cat $pid` > /dev/null 2>&1; do
          echo -n "."
          sleep 1;
        done
        rm $pid
        echo
      else
        retval=$?
        echo no $command to stop because kill -0 of pid `cat $pid` failed with status $retval
      fi
    else
      echo no $command to stop because no pid file $pid
    fi
    ;;

  (restart)
    thiscmd=$0
    args=$@
    # stop the command
    $thiscmd --config "${DCS_CONF_DIR}" stop $command $args &
    wait_until_done $!
    # wait a user-specified sleep period
    sp=${DCS_RESTART_SLEEP:-3}
    if [ $sp -gt 0 ]; then
      sleep $sp
    fi
    # start the command
    $thiscmd --config "${DCS_CONF_DIR}" start $command $args &
    wait_until_done $!
    ;;

  (*)
    echo $usage
    exit 1
    ;;

esac
