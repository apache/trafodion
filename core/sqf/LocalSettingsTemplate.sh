# Local Build/Run settings for Trafodion
#
#
# To have your local customizations work with the source code,
# copy this file to ~/.trafodion.sh and edit values there.
# See sqf/sqenvcom.sh
#
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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

#######################
# Build Dependencies

# Standard tools expected to be installed and found in PATH
# Change to explicit path as needed
ANT=ant
AR=ar
FLEX=flex
CXX=g++

# Non-standard or newer version tools
TOOLSDIR="/opt/home/tools"   # convenient to put them all in the same place

BISON="${TOOLSDIR}/bison_3_linux/bin/bison"
MAVEN="${TOOLSDIR}/apache-maven-3.0.5/bin/mvn"
LLVM="${TOOLSDIR}/dest-llvm-3.2"
UDIS86="${TOOLSDIR}/udis86-1.7.2"
ICU="${TOOLSDIR}/icu4.4"
MPICH_ROOT="$TOOLSDIR/dest-mpich-3.0.4"
ZOOKEEPER_DIR="$TOOLSDIR/zookeeper-3.4.5"
THRIFT_LIB_DIR="$TOOLSDIR/thrift-0.9.0/lib"
THRIFT_INC_DIR="$TOOLSDIR/thrift-0.9.0/include"
PROTOBUFS="/usr"
QT_TOOLKIT="$TOOLSDIR/Qt-4.8.5-64"
# Explicitly unset QT_TOOLKIT here if Qt is not installed and you don't want to build the SqlCompilerDebugger
# unset QT_TOOLKIT

# HBASE*, HIVE*, HADOOP* locations may be overridden here - see sqf/sqenvcom.sh
