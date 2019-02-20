#/**
#* @@@ START COPYRIGHT @@@
#*
#* Licensed to the Apache Software Foundation (ASF) under one
#* or more contributor license agreements.  See the NOTICE file
#* distributed with this work for additional information
#* regarding copyright ownership.  The ASF licenses this file
#* to you under the Apache License, Version 2.0 (the
#* "License"); you may not use this file except in compliance
#* with the License.  You may obtain a copy of the License at
#*
#*   http://www.apache.org/licenses/LICENSE-2.0
#*
#* Unless required by applicable law or agreed to in writing,
#* software distributed under the License is distributed on an
#* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#* KIND, either express or implied.  See the License for the
#* specific language governing permissions and limitations
#* under the License.
#*
#* @@@ END COPYRIGHT @@@
# */

# Set environment variables here.

# This script sets variables multiple times over the course of starting an dcs process,
# so try to keep things idempotent unless you want to take an even deeper look
# into the startup scripts (bin/dcs, etc.)

# The java implementation to use.  Java 1.7 required.
# export JAVA_HOME=/usr/java/jdk1.7.0/

# Add Trafodion to the classpath
if [ "$TRAF_HOME" != "" ]; then
  if [ -d $TRAF_HOME ]; then
    export DCS_CLASSPATH=${CLASSPATH}:
  fi
fi

# Extra Java CLASSPATH elements.  Optional.
# export DCS_CLASSPATH=${DCS_CLASSPATH}:

# The maximum amount of heap to use, in MB. Default is 128.
# export DCS_HEAPSIZE=128

# Extra Java runtime options.
# Below are what we set by default.  May only work with SUN JVM.
# For more on why as well as other possible settings,
# see http://wiki.apache.org/hadoop/PerformanceTuning
export DCS_OPTS="-XX:+UseConcMarkSweepGC"

# Uncomment below to enable java garbage collection logging for the server-side processes
# this enables basic gc logging for the server processes to the .out file
# export SERVER_GC_OPTS="-verbose:gc -XX:+PrintGCDetails -XX:+PrintGCDateStamps -XX:+PrintGCTimeStamps -XX:+PrintTenuringDistribution $DCS_GC_OPTS"

# this enables gc logging using automatic GC log rolling. Only applies to jdk 1.6.0_34+ and 1.7.0_2+. Either use this set of options or the one above
# export SERVER_GC_OPTS="-verbose:gc -XX:+PrintGCDetails -XX:+PrintGCDateStamps -XX:+PrintGCTimeStamps -XX:+UseGCLogFileRotation -XX:NumberOfGCLogFiles=1 -XX:GCLogFileSize=512M $DCS_GC_OPTS"

# Uncomment below to enable java garbage collection logging for the client processes in the .out file.
# export CLIENT_GC_OPTS="-verbose:gc -XX:+PrintGCDetails -XX:+PrintGCDateStamps $DCS_GC_OPTS"

# Uncomment below (along with above GC logging) to put GC information in its own logfile (will set DCS_GC_OPTS).
# This applies to both the server and client GC options above
# export DCS_USE_GC_LOGFILE=true

# Uncomment below if you intend to use the EXPERIMENTAL off heap cache.
# export DCS_OPTS="$DCS_OPTS -XX:MaxDirectMemorySize="
# Set dcs.offheapcache.percentage in dcs-site.xml to a nonzero value.

export DCS_MASTER_OPTS=
export DCS_SERVER_OPTS=
export DCS_RESET_OPTS=
export DCS_ZOOKEEPER_OPTS=
# Uncomment and adjust to enable JMX exporting
# See jmxremote.password and jmxremote.access in $JRE_HOME/lib/management to configure remote password access.
# More details at: http://java.sun.com/javase/6/docs/technotes/guides/management/agent.html
# export DCS_JMX_BASE="-Dcom.sun.management.jmxremote.ssl=false -Dcom.sun.management.jmxremote.authenticate=false"
# export DCS_MASTER_OPTS="$DCS_MASTER_OPTS $DCS_JMX_BASE -Dcom.sun.management.jmxremote.port=10101"
# export DCS_SERVER_OPTS="$DCS_SERVER_OPTS $DCS_JMX_BASE -Dcom.sun.management.jmxremote.port=10102"
# export DCS_REST_OPTS="$DCS_REST_OPTS $DCS_JMX_BASE -Dcom.sun.management.jmxremote.port=10103"
# export DCS_ZOOKEEPER_OPTS="$DCS_ZOOKEEPER_OPTS $DCS_JMX_BASE -Dcom.sun.management.jmxremote.port=10104"

# File naming host on which DCS Primary Master is configured to run. $TRAF_HOME/dcs/master by default.
# export DCS_PRIMARY_MASTER=${TRAF_HOME}/dcs/master

# File naming hosts on which DCS Backup Masters is configured to run. $TRAF_CONF/dcs/backup-masters by default.
# export DCS_BACKUP_MASTERS=${TRAF_CONF}/dcs/backup-masters

# File naming hosts on which DCS Servers will run. $TRAF_CONF/dcs/servers by default.
# export DCS_SERVERS=${TRAF_CONF}/dcs/servers

# Extra ssh options.  Empty by default.
# export DCS_SSH_OPTS="-o ConnectTimeout=1 -o SendEnv=DCS_CONF_DIR"

# Where log files are stored.  $TRAF_LOG/dcs by default.
# export DCS_LOG_DIR=$TRAF_LOG/dcs

# Enable remote JDWP debugging of major dcs processes. Meant for Core Developers 
# export DCS_MASTER_OPTS="$DCS_MASTER_OPTS -Xdebug -Xrunjdwp:transport=dt_socket,server=y,suspend=n,address=8070"
# export DCS_SERVER_OPTS="$DCS_SERVER_OPTS -Xdebug -Xrunjdwp:transport=dt_socket,server=y,suspend=n,address=8071"
# export DCS_RESET_OPTS="$DCS_RESET_OPTS -Xdebug -Xrunjdwp:transport=dt_socket,server=y,suspend=n,address=8072"
# export DCS_ZOOKEEPER_OPTS="$DCS_ZOOKEEPER_OPTS -Xdebug -Xrunjdwp:transport=dt_socket,server=y,suspend=n,address=8073"

# A string representing this instance of dcs. $USER by default.
# export DCS_IDENT_STRING=$USER

# The scheduling priority for daemon processes.  See 'man nice'.
# export DCS_NICENESS=10

# The directory where pid files are stored. $DCS_HOME/tmp by default.
# export DCS_PID_DIR=/var/dcs/pids

# Tell DCS whether it should manage it's own instance of Zookeeper or not.
export DCS_MANAGES_ZK=false

# Tell DCS where the user program environment lives.
export DCS_USER_PROGRAM_HOME=$TRAF_HOME

# DCS master port (from dcs-site.xml)
export DCS_MASTER_PORT=23400

# DCS floating IP, if HA is enabled (from dcs-site.xml)
export DCS_MASTER_FLOATING_IP=""
