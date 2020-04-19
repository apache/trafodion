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

# -----------------------------------------------------------------------------
# script: traf_tools_setup
#
# Helper script that downloads and installs dependent tools required by 
# Apache Trafodion to build from a source distribution
#
# Tools installed:
#
# MPICH: Implementation of the Message Passing Interface (MPI) standard.  For use 
#   in Trafodion, MPICH must be built to force sockets to be used in both 
#   internode and intranode message passing. 
# Bison: General-purpose parser generator.
# Udis86: Minimalistic disassembler library (libudis86) for the x86 class of 
#   instruction set architectures.
# LLVM: Collection of modular and reusable compiler and toolchain technologies.
# ICU: C/C++ and Java libraries providing Unicode and Globalization support for 
#   software applications.
# Zookeeper: Coordination service for distributed applications.  It exposes 
#   common services such as naming, configuration management, synchronization, 
#   and group services.
# Thrift: Communications and data serialization tool
# Maven: Build tool that is only installed if compatible version does not exist
# log4cxx: standard logging framework for C++
# hadoop: shared libraries for libhadoop, libhdfs, and hdfs header file
#
# Script can be modified to meet the needs of your environment
# May need root or SUDO access to install tools in desired location
# ----------------------------------------------------------------------------- 

function Usage {
   echo
   echo "Usage: $0 -d <downloaddir> -i <installdir>"
   echo
   echo " -d <downloaddir> - location of download directory"
   echo " -i <installdir>  - location of install directory"
   echo
   echo " -h - help"
   echo
   echo "example: traf_tools_setup.sh -d /home/userx/download -i /home/userx/tools"
   echo
}

# -----------------------------------------------------------------------------
# function: downloadSource - downloads and un-tars the requested file
#    $1 - tar file URL to download
#    $2 - directory where source is untarred (leave empty to skip untar)
#
# -----------------------------------------------------------------------------
function downloadSource
{
  URL="$1"
  SRCDIR="$2"
  TARFILE="${URL##*/}"

  if [ ! -e $BASEDIR/$TARFILE ]; then
    wget $URL  >>$LOGFILE 2>&1
    echo "INFO:   downloaded tar file: $TARFILE " | tee -a $LOGFILE
  else
    echo "INFO:   tar file already downloaded, step skipped" | tee -a $LOGFILE
  fi

  if [ ! -e $BASEDIR/$SRCDIR ]; then
    cd $BASEDIR
    tar -xzf $BASEDIR/$TARFILE
  else
    echo "INFO:   source tree already exists" | tee -a $LOGFILE
  fi
}

LOGDIR=$(pwd)
LOGFILE=$LOGDIR/traf_tools_setup.log
BLDLOG=$LOGDIR/traf_tools_bld

# -----------------------------------------------------------------------------
# execute build functions in background
# $1 - short name of component (single word, also used in logfile name)
# $2 - first make target (usually empty string for the default target)
# $3 - second make target
# $4 - dir/file to check for success
# -----------------------------------------------------------------------------
function bkgBuild
{
  LOG=${BLDLOG}-${1}.log
  echo "INFO:   Starting background make and install for $1" | tee -a $LOGFILE

 (make $2 > $LOG 2>&1
  echo "INFO:   make completed" > $LOG
  make $3 > $LOG 2>&1
  if [[ ! -e $4 ]]; then
    echo "ERROR:  failed to install $1" | tee -a $LOGFILE
    echo "  see details in $LOG" | tee -a $LOGFILE
    exit 2;
  fi
  echo "INFO:   $1 install complete at $(date), files placed in $TOOLSDIR" | tee -a $LOGFILE
 ) &
}

# main code

TOOLSDIR=
BASEDIR=
rm $LOGFILE 2>/dev/null
echo
echo "INFO: Starting tools build on $(date)" | tee -a $LOGFILE

    while getopts "d:i:hv" arg
      do
      case $arg in
          d)
              BASEDIR=${OPTARG};
              ;;
          h)
              Usage;
              exit 1;
              ;;
          i)
              TOOLSDIR=${OPTARG};
              ;;
          *)
              Usage;
              exit 1;
              ;;
      esac
    done


if [ "$BASEDIR" == "" ]; then
  echo
  echo "ERROR: download directory (-d) is not specified"
  Usage;
  exit 1;
fi
# handle relative path
if [[ ! $BASEDIR =~ ^/ ]]
then
  BASEDIR=$(pwd)/$BASEDIR
fi

if [ ! -d "$BASEDIR" ]; then
  echo
  echo "ERROR: download directory ($BASEDIR) does not exist"
  echo
  exit 1;
fi
 
if [ "$TOOLSDIR" == "" ]; then
  echo
  echo "ERROR: install directory (-t)  is not specified"
  Usage;
  exit 1;
fi
# handle relative path
if [[ ! $TOOLSDIR =~ ^/ ]]
then
  TOOLSDIR=$(pwd)/$TOOLSDIR
fi

if [ ! -d "$TOOLSDIR" ]; then                                                    
  read -p "Tools install directory $TOOLSDIR does not exist, do you want to to create it? y/n : " CREATEDIR
  if [ "$CREATEDIR" == "y" ]; then
     mkdir $TOOLSDIR
     if [ ! -d "$TOOLSDIR" ]; then
       echo
       echo "ERROR: unable to create directory $TOOLSDIR"
       echo
       exit 1
     else
       echo "INFO: Created directory $TOOLSDIR" | tee -a $LOGFILE
     fi
  else
    echo
    echo "ERROR: install directory ($TOOLSDIR) does not exist"
    echo
    exit 1;
  fi
fi

echo "INFO: Tar download location: $BASEDIR" | tee -a $LOGFILE
echo "INFO: Tool install directory location: $TOOLSDIR" | tee -a $LOGFILE
echo "INFO: LogFile location: $LOGFILE" | tee -a $LOGFILE
echo " *********************************************************** " | tee -a $LOGFILE


# -----------------------------------------------------------------------------
# install udis
cd $BASEDIR
echo
echo "INFO: Installing UDIS on $(date)" | tee -a $LOGFILE
if [ -d $TOOLSDIR/udis86-1.7.2/bin ]; then
  echo "INFO: UDIS is already installed, skipping to next tool" | tee -a $LOGFILE
else
  downloadSource http://sourceforge.net/projects/udis86/files/udis86/1.7/udis86-1.7.2.tar.gz udis86-1.7.2
  cd udis86-1.7.2
  ./configure --prefix=$TOOLSDIR/udis86-1.7.2 --enable-shared >>$LOGFILE 2>&1
  echo "INFO:   configure complete" | tee -a $LOGFILE
  bkgBuild "udis86" "" "install" "$TOOLSDIR/udis86-1.7.2/bin"
fi
echo " *********************************************************** " | tee -a $LOGFILE


# -----------------------------------------------------------------------------
#install LLVM
cd $BASEDIR
echo
echo "INFO: Installing LLVM on $(date)" | tee -a $LOGFILE
if [ -d $TOOLSDIR/dest-llvm-3.2/release/bin -a -d $TOOLSDIR/dest-llvm-3.2/debug/bin ]; then
  echo "INFO: LLVM is already installed, skipping to next step" | tee -a $LOGFILE
else
  downloadSource http://llvm.org/releases/3.2/llvm-3.2.src.tar.gz llvm-3.2.src
  
  # Depends on UDIS, so make sure it is done
  wait

  export MY_UDIS_INSTALL_DIR=$TOOLSDIR/udis86-1.7.2
  export MY_LLVM_INSTALL_DIR=$TOOLSDIR/dest-llvm-3.2/
  export MY_LLVM_SRC_DIR=$BASEDIR/llvm-3.2.src
  export MY_LLVM_OBJ_DIR=$BASEDIR/llvm-3.2.obj/
  export LD_LIBRARY_PATH=$MY_UDIS_INSTALL_DIR/lib:$LD_LIBRARY_PATH
  export C_INCLUDE_PATH=$MY_UDIS_INSTALL_DIR/include
  export CPATH=$MY_UDIS_INSTALL_DIR/include

  # Build release version
  if [ ! -d $TOOLSDIR/dest-llvm-3.2/release/bin ]; then 
    mkdir -p $MY_LLVM_OBJ_DIR/release
    cd $MY_LLVM_OBJ_DIR/release

    $MY_LLVM_SRC_DIR/configure --prefix=$MY_LLVM_INSTALL_DIR/release \
       --enable-shared --enable-targets=x86,x86_64,cpp \
       --with-udis86=$MY_UDIS_INSTALL_DIR/lib \
       CFLAGS=-fgnu89-inline >>$LOGFILE 2>&1
    echo "INFO:   release configure complete" | tee -a $LOGFILE

    bkgBuild "llvm-release" "libs-only" "install-libs" "$TOOLSDIR/dest-llvm-3.2/release/bin"
  fi

  # Build debug version
  if [ ! -d $TOOLSDIR/dest-llvm-3.2/debug/bin ]; then
    mkdir -p $MY_LLVM_OBJ_DIR/debug
    cd $MY_LLVM_OBJ_DIR/debug

    $MY_LLVM_SRC_DIR/configure --prefix=$MY_LLVM_INSTALL_DIR/debug \
       --enable-optimized --enable-jit \
       --enable-debug-runtime --enable-debug-symbols \
       --enable-shared --enable-targets=x86,x86_64,cpp \
       --with-udis86=$MY_UDIS_INSTALL_DIR/lib \
       CFLAGS=-fgnu89-inline >>$LOGFILE 2>&1
    echo "INFO:   debug configure complete" | tee -a $LOGFILE
    bkgBuild "llvm-debug" "libs-only" "install-libs" "$TOOLSDIR/dest-llvm-3.2/debug/bin"
  fi
fi
echo " *********************************************************** " | tee -a $LOGFILE


# -----------------------------------------------------------------------------
# install mpi
cd $BASEDIR
echo
echo "INFO: Installing MPI on $(date)" | tee -a $LOGFILE
if [ -d $TOOLSDIR/dest-mpich-3.0.4/bin ]; then
  echo "INFO: MPI is already installed, skipping to next tool" | tee -a $LOGFILE
else	
  downloadSource http://www.mpich.org/static/downloads/3.0.4/mpich-3.0.4.tar.gz mpich-3.0.4 
  cd mpich-3.0.4
  ./configure --prefix=$TOOLSDIR/dest-mpich-3.0.4 --with-device=ch3:sock --disable-f77 --disable-fc >>$LOGFILE 2>&1
  echo "INFO:   configure complete" | tee -a $LOGFILE
  bkgBuild "MPI" "" "install" "$TOOLSDIR/dest-mpich-3.0.4/bin"
fi
echo " *********************************************************** " | tee -a $LOGFILE


# -----------------------------------------------------------------------------
# install bison
cd $BASEDIR
echo
echo "INFO: Installing Bison on $(date)" | tee -a $LOGFILE
if [ -d $TOOLSDIR/bison_3_linux/bin ]; then
  echo "INFO: Bison is already installed, skipping to next tool" | tee -a $LOGFILE
else	
  downloadSource http://ftp.gnu.org/gnu/bison/bison-3.0.tar.gz bison-3.0
  cd bison-3.0
  ./configure --prefix=$TOOLSDIR/bison_3_linux >>$LOGFILE 2>&1
  echo "INFO:   configure complete" | tee -a $LOGFILE
  bkgBuild "bison" "" "install" "$TOOLSDIR/bison_3_linux/bin"
fi
echo " *********************************************************** " | tee -a $LOGFILE


# -----------------------------------------------------------------------------
# Build ICU
cd $BASEDIR
echo
echo "INFO: Installing ICU on $(date)" | tee -a $LOGFILE
if [ -d $TOOLSDIR/icu4c_4.4/linux64/bin ]; then
  echo "INFO: ICU is already installed, skipping to next tool" | tee -a $LOGFILE
else
  downloadSource https://sourceforge.net/projects/icu/files/ICU4C/4.4.2/icu4c-4_4_2-src.tgz icu4c-4_4-src
  cd icu/source
  ./configure --prefix=$TOOLSDIR/icu4c_4.4/linux64 >>$LOGFILE 2>&1
  echo "INFO:   configure complete" | tee -a $LOGFILE
  bkgBuild "ICU" "" "install" "$TOOLSDIR/icu4c_4.4/linux64/bin"
fi
echo " *********************************************************** " | tee -a $LOGFILE


# -----------------------------------------------------------------------------
# make zookeeper
cd $BASEDIR
echo
echo "INFO: Installing ZooKeeper on $(date)" | tee -a $LOGFILE
if [ -d $TOOLSDIR/zookeeper-3.4.5/bin ]; then
  echo "INFO: ZooKeeper is already installed, skipping to next tool" | tee -a $LOGFILE
else
  downloadSource https://archive.apache.org/dist/zookeeper/zookeeper-3.4.5/zookeeper-3.4.5.tar.gz zookeeper-3.4.5
  cd zookeeper-3.4.5/src/c
  ./configure --prefix=$TOOLSDIR/zookeeper-3.4.5 >>$LOGFILE 2>&1
  echo "INFO:   configure complete" | tee -a $LOGFILE
  bkgBuild "zookeeper" "" "install" "$TOOLSDIR/zookeeper-3.4.5/bin"
fi
echo " *********************************************************** " | tee -a $LOGFILE


# -----------------------------------------------------------------------------
# make thrift
cd $BASEDIR
echo
echo "INFO: Installing Thrift on $(date)" | tee -a $LOGFILE
if [ -d $TOOLSDIR/thrift-0.9.0/bin ]; then
  echo "INFO: Thrift is already installed, skipping to next tool" | tee -a $LOGFILE
else
  downloadSource http://archive.apache.org/dist/thrift/0.9.0/thrift-0.9.0.tar.gz thrift-0.9.0
  cd thrift-0.9.0
  ./configure --prefix=$TOOLSDIR/thrift-0.9.0 -without-qt >>$LOGFILE 2>&1
  echo "INFO:   configure complete" | tee -a $LOGFILE
  bkgBuild "thrift" "" "install" "$TOOLSDIR/thrift-0.9.0/bin"
fi
echo " *********************************************************** " | tee -a $LOGFILE


# -----------------------------------------------------------------------------
# install maven, if version 3 or greater not available already
MAVEN_VERSION=`mvn --version 2>/dev/null | grep 'Apache Maven' | cut -f 3 -d ' '`
if [[ ! "$MAVEN_VERSION" =~ "3." ]]; then
  cd $BASEDIR
  echo
  echo "INFO: Installing Maven on $(date)" | tee -a $LOGFILE
  if [ -d $TOOLSDIR/apache-maven-3.3.3/bin ]; then
    echo "INFO: Maven is already installed, skipping to next tool" | tee -a $LOGFILE
  else
    # there is no install, simply extract the files into $TOOLSDIR
    wget http://archive.apache.org/dist/maven/maven-3/3.3.3/binaries/apache-maven-3.3.3-bin.tar.gz >>$LOGFILE 2>&1
    cd $TOOLSDIR
    tar -xzf $BASEDIR/apache-maven-3.3.3-bin.tar.gz >>$LOGFILE 2>&1
    echo "INFO:   downloaded tar file:  apache-maven-3.3.3-bin.tar.gz" | tee -a $LOGFILE
  fi
    echo "INFO: Maven installation complete" | tee -a $LOGFILE
  echo " *********************************************************** " | tee -a $LOGFILE
else
  echo "INFO:  Maven is already installed, skipping to next tool" | tee -a $LOGFILE
fi

# -----------------------------------------------------------------------------
# install log4cxx, if not installed
if [[ !  -e /usr/lib64/liblog4cxx.so ]]; then
  cd $BASEDIR
  echo
  echo "INFO: Installing log4cxx on $(date)" | tee -a $LOGFILE
  if [ -d $TOOLSDIR/apache-log4cxx-0.10.0/lib ]; then
    echo "INFO: log4cxx is already installed, skipping to next tool" | tee -a $LOGFILE
  else
    downloadSource https://dist.apache.org/repos/dist/release/logging/log4cxx/0.10.0/apache-log4cxx-0.10.0.tar.gz apache-log4cxx-0.10.0
    cd apache-log4cxx-0.10.0
    echo "INFO:   headerfile patch - per LOG4CXX-360" | tee -a $LOGFILE
    sed -i '1 i#include <string.h>' src/main/cpp/inputstreamreader.cpp
    sed -i '1 i#include <string.h>' src/main/cpp/socketoutputstream.cpp
    sed -i '1 i#include <string.h>' src/examples/cpp/console.cpp
    sed -i '2 i#include <stdio.h>' src/examples/cpp/console.cpp
    ./configure --prefix=$TOOLSDIR/apache-log4cxx-0.10.0 >>$LOGFILE 2>&1
    echo "INFO:   configure complete" | tee -a $LOGFILE
    echo "INFO:   Be sure dependencies apr-devel and apr-util-devel are installed" | tee -a $LOGFILE
    bkgBuild "log4cxx" "" "install" "$TOOLSDIR/apache-log4cxx-0.10.0/lib"
  fi
  echo " *********************************************************** " | tee -a $LOGFILE
else
  echo "INFO:  log4cxx is already installed, skipping to next tool" | tee -a $LOGFILE
fi

# -----------------------------------------------------------------------------
# download hadoop/hdfs libs
echo
echo "INFO: Hadoop/HDFS libs on $(date)" | tee -a $LOGFILE
HVER="2.6.0"
if [ -d $TOOLSDIR/hadoop-${HVER} ]; then
  echo "INFO: Hadoop/HDFS is already installed, skipping to next tool" | tee -a $LOGFILE
else
  downloadSource http://archive.apache.org/dist/hadoop/common/hadoop-${HVER}/hadoop-${HVER}.tar.gz # no un-tar
  cd $TOOLSDIR
  tar -xzf $BASEDIR/hadoop-${HVER}.tar.gz \
    hadoop-${HVER}/lib/native/libhadoop\*so\* \
    hadoop-${HVER}/lib/native/libhdfs\*so\* \
    hadoop-${HVER}/include/hdfs.h
  echo "INFO:   extraction complete" | tee -a $LOGFILE
fi
echo " *********************************************************** " | tee -a $LOGFILE

# -----------------------------------------------------------------------------

echo
echo "INFO: Waiting for all background builds. This might take a while." | tee -a $LOGFILE
wait
echo "INFO: Completed tools builds on $(date)" | tee -a $LOGFILE
echo "INFO: List of tools directory: " | tee -a $LOGFILE
ls $TOOLSDIR | echo | tee -a $LOGFILE
echo "`ls $TOOLSDIR`" | tee -a $LOGFILE
echo
