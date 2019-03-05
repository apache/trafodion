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
#
# Set environment variables here.

# This script sets variables multiple times over the course of starting an rest process,
# so try to keep things idempotent unless you want to take an even deeper look
# into the startup scripts (bin/rest, etc.)

# The java implementation to use.  Java 1.7 required.
# export JAVA_HOME=/usr/java/jdk1.7.0/

# Add Trafodion to the classpath
if [ "$TRAF_HOME" != "" ]; then
  if [ -d $TRAF_HOME ]; then
    export REST_CLASSPATH=${CLASSPATH}:
  fi
fi

# Extra Java CLASSPATH elements.  Optional.
# export REST_CLASSPATH=${REST_CLASSPATH}:

# The maximum amount of heap to use, in MB. Default is 128.
# export REST_HEAPSIZE=128

# Extra Java runtime options.
# Below are what we set by default.  May only work with SUN JVM.
# For more on why as well as other possible settings,
# see http://wiki.apache.org/hadoop/PerformanceTuning
export REST_OPTS="-XX:+UseConcMarkSweepGC"

# Uncomment below to enable java garbage collection logging for the server-side processes
# this enables basic gc logging for the server processes to the .out file
# export SERVER_GC_OPTS="-verbose:gc -XX:+PrintGCDetails -XX:+PrintGCDateStamps $REST_GC_OPTS"

# this enables gc logging using automatic GC log rolling. Only applies to jdk 1.6.0_34+ and 1.7.0_2+. Either use this set of options or the one above
# export SERVER_GC_OPTS="-verbose:gc -XX:+PrintGCDetails -XX:+PrintGCDateStamps -XX:+UseGCLogFileRotation -XX:NumberOfGCLogFiles=1 -XX:GCLogFileSize=512M $REST_GC_OPTS"

# Uncomment below to enable java garbage collection logging for the client processes in the .out file.
# export CLIENT_GC_OPTS="-verbose:gc -XX:+PrintGCDetails -XX:+PrintGCDateStamps $REST_GC_OPTS"

# Uncomment below (along with above GC logging) to put GC information in its own logfile (will set REST_GC_OPTS).
# This applies to both the server and client GC options above
# export REST_USE_GC_LOGFILE=true

# Uncomment below if you intend to use the EXPERIMENTAL off heap cache.
# export REST_OPTS="$REST_OPTS -XX:MaxDirectMemorySize="
# Set rest.offheapcache.percentage in rest-site.xml to a nonzero value.

# Uncomment and adjust to enable JMX exporting
# See jmxremote.password and jmxremote.access in $JRE_HOME/lib/management to configure remote password access.
# More details at: http://java.sun.com/javase/6/docs/technotes/guides/management/agent.html
# export REST_JMX_BASE="-Dcom.sun.management.jmxremote.ssl=false -Dcom.sun.management.jmxremote.authenticate=false"
# export REST_REST_OPTS="$REST_REST_OPTS $REST_JMX_BASE -Dcom.sun.management.jmxremote.port=10103"

# File naming hosts on which REST Servers will run.  $TRAF_CONF/rest/servers by default.
# export REST_SERVERS=${TRAF_CONF}/rest/servers

# Extra ssh options.  Empty by default.
# export REST_SSH_OPTS="-o ConnectTimeout=1 -o SendEnv=REST_CONF_DIR"

# Where log files are stored.  $TRAF_LOG/rest by default.
# export REST_LOG_DIR=$TRAF_LOG/rest

# Enable remote JDWP debugging of major rest processes. Meant for Core Developers 
# export REST_RESET_OPTS="$REST_RESET_OPTS -Xdebug -Xrunjdwp:transport=dt_socket,server=y,suspend=n,address=8072"

# A string representing this instance of rest. $USER by default.
# export REST_IDENT_STRING=$USER

# The scheduling priority for daemon processes.  See 'man nice'.
# export REST_NICENESS=10

# The directory where pid files are stored. $REST_HOME/tmp by default.
# export REST_PID_DIR=/var/rest/pids
