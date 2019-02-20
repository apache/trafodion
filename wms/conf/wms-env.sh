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

# Set environment variables here.

# This script sets variables multiple times over the course of starting an wms process,
# so try to keep things idempotent unless you want to take an even deeper look
# into the startup scripts (bin/wms, etc.)

# The java implementation to use.  Java 1.7 required.
# export JAVA_HOME=/usr/java/jdk1.7.0/

# Add Trafodion to the classpath
if [ "$TRAF_HOME" != "" ]; then
  if [ -d $TRAF_HOME ]; then
    export WMS_CLASSPATH=${CLASSPATH}:
  fi
fi


# Extra Java CLASSPATH elements.  Optional.
# export WMS_CLASSPATH=${WMS_CLASSPATH}:

# The maximum amount of heap to use, in MB. Default is 128.
# export WMS_HEAPSIZE=128

# Extra Java runtime options.
# Below are what we set by default.  May only work with SUN JVM.
# For more on why as well as other possible settings,
# see http://wiki.apache.org/hadoop/PerformanceTuning
export WMS_OPTS="-XX:+UseConcMarkSweepGC"

# Uncomment below to enable java garbage collection logging for the server-side processes
# this enables basic gc logging for the server processes to the .out file
# export SERVER_GC_OPTS="-verbose:gc -XX:+PrintGCDetails -XX:+PrintGCDateStamps $WMS_GC_OPTS"

# this enables gc logging using automatic GC log rolling. Only applies to jdk 1.6.0_34+ and 1.7.0_2+. Either use this set of options or the one above
# export SERVER_GC_OPTS="-verbose:gc -XX:+PrintGCDetails -XX:+PrintGCDateStamps -XX:+UseGCLogFileRotation -XX:NumberOfGCLogFiles=1 -XX:GCLogFileSize=512M $WMS_GC_OPTS"

# Uncomment below to enable java garbage collection logging for the client processes in the .out file.
# export CLIENT_GC_OPTS="-verbose:gc -XX:+PrintGCDetails -XX:+PrintGCDateStamps $WMS_GC_OPTS"

# Uncomment below (along with above GC logging) to put GC information in its own logfile (will set WMS_GC_OPTS).
# This applies to both the server and client GC options above
# export WMS_USE_GC_LOGFILE=true


# Uncomment below if you intend to use the EXPERIMENTAL off heap cache.
# export WMS_OPTS="$WMS_OPTS -XX:MaxDirectMemorySize="
# Set wms.offheapcache.percentage in wms-site.xml to a nonzero value.


# Uncomment and adjust to enable JMX exporting
# See jmxremote.password and jmxremote.access in $JRE_HOME/lib/management to configure remote password access.
# More details at: http://java.sun.com/javase/6/docs/technotes/guides/management/agent.html
#
# export WMS_JMX_BASE="-Dcom.sun.management.jmxremote.ssl=false -Dcom.sun.management.jmxremote.authenticate=false"
# export WMS_MASTER_OPTS="$WMS_MASTER_OPTS $WMS_JMX_BASE -Dcom.sun.management.jmxremote.port=10101"
# export WMS_SERVER_OPTS="$WMS_SERVER_OPTS $WMS_JMX_BASE -Dcom.sun.management.jmxremote.port=10102"
# export WMS_THRIFT_OPTS="$WMS_THRIFT_OPTS $WMS_JMX_BASE -Dcom.sun.management.jmxremote.port=10103"
# export WMS_ZOOKEEPER_OPTS="$WMS_ZOOKEEPER_OPTS $WMS_JMX_BASE -Dcom.sun.management.jmxremote.port=10104"

# File naming hosts on which WMS Servers will run.  $TRAF_CONF/wms/servers by default.
# export WMS_SERVERS=${TRAF_CONF}/wms/servers

# Extra ssh options.  Empty by default.
# export WMS_SSH_OPTS="-o ConnectTimeout=1 -o SendEnv=WMS_CONF_DIR"

# Where log files are stored.  $TRAF_LOG/wms by default.
# export WMS_LOG_DIR=$TRAF_LOG/wms

# Enable remote JDWP debugging of major wms processes. Meant for Core Developers 
# export WMS_MASTER_OPTS="$WMS_MASTER_OPTS -Xdebug -Xrunjdwp:transport=dt_socket,server=y,suspend=n,address=8070"
# export WMS_SERVER_OPTS="$WMS_SERVER_OPTS -Xdebug -Xrunjdwp:transport=dt_socket,server=y,suspend=n,address=8071"
# export WMS_THRIFT_OPTS="$WMS_THRIFT_OPTS -Xdebug -Xrunjdwp:transport=dt_socket,server=y,suspend=n,address=8072"
# export WMS_ZOOKEEPER_OPTS="$WMS_ZOOKEEPER_OPTS -Xdebug -Xrunjdwp:transport=dt_socket,server=y,suspend=n,address=8073"

# A string representing this instance of wms. $USER by default.
# export WMS_IDENT_STRING=$USER

# The scheduling priority for daemon processes.  See 'man nice'.
# export WMS_NICENESS=10

# The directory where pid files are stored. /tmp by default.
# export WMS_PID_DIR=/var/wms/pids

# Seconds to sleep between slave commands.  Unset by default.  This
# can be useful in large clusters, where, e.g., slave rsyncs can
# otherwise arrive faster than the master can service them.
# export WMS_SLAVE_SLEEP=0.1

# Tell WMS whether it should manage it's own instance of Zookeeper or not.
# export WMS_MANAGES_ZK=true

# Tell WMS where the user program environment lives.
 export WMS_USER_PROGRAM_HOME=$TRAF_HOME

