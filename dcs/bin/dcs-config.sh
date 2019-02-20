#/**
## @@@ START COPYRIGHT @@@
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
# # @@@ END COPYRIGHT @@@
# */
#
# should not be executable directly
# also should not be passed any arguments, since we need original $*

# resolve links - "${BASH_SOURCE-$0}" may be a softlink

this="${BASH_SOURCE-$0}"
while [ -h "$this" ]; do
  ls=`ls -ld "$this"`
  link=`expr "$ls" : '.*-> \(.*\)$'`
  if expr "$link" : '.*/.*' > /dev/null; then
    this="$link"
  else
    this=`dirname "$this"`/"$link"
  fi
done

# convert relative path to absolute path
bin=`dirname "$this"`
script=`basename "$this"`
bin=`cd "$bin">/dev/null; pwd`
this="$bin/$script"

# the root of the Dcs installation
if [ -z "$DCS_HOME" ]; then
  export DCS_HOME=`dirname "$this"`/..
fi

foreground="false"
#check to see if the conf dir or dcs home are given as an optional arguments
while [ $# -gt 1 ]
do
  if [ "--config" = "$1" ]
  then
    shift
    confdir=$1
    shift
    DCS_CONF_DIR=$confdir
  elif [ "--hosts" = "$1" ]
  then
    shift
    hosts=$1
    shift
    DCS_SERVERS=$hosts
  elif [ "--foreground" = "$1" ]
  then
    shift
    foreground="true"
  else
    # Presume we are at end of options and break
    break
  fi
done
 
# Allow alternate DCS conf dir location.
DCS_CONF_DIR="${DCS_CONF_DIR:-$TRAF_CONF/dcs}"
# List of DCS servers.
DCS_SERVERS="${DCS_SERVERS:-$DCS_CONF_DIR/servers}"
#List of DCS masters
DCS_MASTERS="${DCS_MASTERS:-$DCS_CONF_DIR/masters}"

# Source the dcs-env.sh.  Will have JAVA_HOME defined.
if [ -f "${DCS_CONF_DIR}/dcs-env.sh" ]; then
  . "${DCS_CONF_DIR}/dcs-env.sh"
fi

# Source in sqenv.sh.
if [ -f "${TRAF_HOME}/sqenv.sh" ]; then
  savedir=`pwd`
  cd $TRAF_HOME
  if [[ -f /etc/trafodion/trafodion_config ]]; then
     . /etc/trafodion/trafodion_config
  fi
  . ./sqenv.sh
  cd $savedir
fi

# Newer versions of glibc use an arena memory allocator that causes virtual
# memory usage to explode. Tune the variable down to prevent vmem explosion.
export MALLOC_ARENA_MAX=${MALLOC_ARENA_MAX:-4}

if [ -z "$JAVA_HOME" ]; then
  for candidate in \
    /usr/lib/jvm/java-1.7* \
    /usr/lib/jvm/java-1.6.0* \
    /usr/lib/jvm/java-6-sun \
    /usr/lib/jvm/java-1.6.0-sun-1.6.0.*/jre \
    /usr/lib/jvm/java-1.6.0-sun-1.6.0.* \
    /usr/lib/j2sdk1.6-sun \
    /usr/java/jdk1.6* \
    /usr/java/jre1.6* \
    /Library/Java/Home ; do
    if [ -e $candidate/bin/java ]; then
      export JAVA_HOME=$candidate
      break
    fi
  done
  # if we didn't set it
  if [ -z "$JAVA_HOME" ]; then
    cat 1>&2 <<EOF
+======================================================================+
|      Error: JAVA_HOME is not set and Java could not be found         |
+----------------------------------------------------------------------+
| Please download the latest Sun JDK from the Sun Java web site        |
|       > http://java.sun.com/javase/downloads/ <                      |
|                                                                      |
| Dcs requires Java 1.6 or later.                                    |
| NOTE: This script will find Sun Java whether you install using the   |
|       binary or the RPM based installer.                             |
+======================================================================+
EOF
    exit 1
  fi
fi
