# Local Build/Run settings for Trafodion
#
#
# To have your local customizations work with the source code,
# copy this file to ~/.trafodion.sh and edit values there.
# See sqf/sqenvcom.sh
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

#######################
# Build Dependencies
#
#  This file needs to be copied to ~/.trafodion and updated to override
#    build tool locations
#  .trafodion is sourced in when running $TRAF_HOME/sqenvcom.sh
#    sqenvcom.sh is sourced in by $TRAF_HOME/sqenv.sh
#    sqenv.sh is sourced in by: 
#      $TRAF_HOME/../../env.sh or
#      $TRAF_HOME/sqenvd.sh (debug build) or
#      $TRAF_HOME/sqenvr.sh (release build)
#
#  sqenvcom.sh sets up the environment in preparation for building and
#    running Trafodion.
########################

# Standard tools expected to be installed and found in PATH
# Change to explicit path as needed
ANT=ant

AR=ar
FLEX=flex
CXX=g++

# Non-standard or newer version tools
TOOLSDIR="/opt/home/tools" # convenient to put dependent tools in the same place

BISON="${TOOLSDIR}/bison_3_linux/bin/bison"
LLVM="${TOOLSDIR}/dest-llvm-3.2"
UDIS86="${TOOLSDIR}/udis86-1.7.2"
ICU="${TOOLSDIR}/icu4c_4.4"
MPICH_ROOT="$TOOLSDIR/dest-mpich-3.0.4"
ZOOKEEPER_DIR="$TOOLSDIR/zookeeper-3.4.5"
THRIFT_LIB_DIR="$TOOLSDIR/thrift-0.9.0/lib"
THRIFT_INC_DIR="$TOOLSDIR/thrift-0.9.0/include"
LOG4CXX_LIB_DIR="$TOOLSDIR/apache-log4cxx-0.10.0/lib"
LOG4CXX_INC_DIR="$TOOLSDIR/apache-log4cxx-0.10.0/include"


# Explicitly set QT_TOOLKIT here if Qt is installed and you want to build the SqlCompilerDebugger
# QT_TOOLKIT="$TOOLSDIR/Qt-4.8.5-64"
# SqlCompilerDebugger library will be dynamically linked with Qt libs.
# Note that Qt uses LGPL license. It is not to be re-distributed under Apache license.

# HBASE*, HIVE*, HADOOP* locations may be overridden here
# uncomment the following lines to describe where your Hadoop locations exist.
# This is needed if you are building with an existing installation
#HADOOP_PREFIX=/usr/hadoop-2.5.2
#HBASE_HOME=/usr/hbase-0.98.6-hadoop2
#HIVE_HOME=/usr/apache-hive-0.13.1-bin

