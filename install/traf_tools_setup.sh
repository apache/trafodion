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
#    $1 - tar file to download 
#    $2 - directory where source is untarred
#
# Suggestion:  instead use a single argument $1 and figure out the name of the
#              file to extract with basename $1
# -----------------------------------------------------------------------------
function downloadSource
{
  # currently only tar files ending in "tar.gz" and "tgz" are recognized
  TARSUFFIX="tar.gz"
  if [[ ! $1 == *$"$TARSUFFIX" ]]; then
    TARSUFFIX="tgz"
  fi

  if [ ! -e $BASEDIR/$2.$TARSUFFIX ]; then
    wget $1  >>$LOGFILE 2>&1
  else
    echo "INFO:   tar file already downloaded, step skipped" | tee -a $LOGFIL
  fi

  if [ ! -e $BASEDIR/$2 ]; then
    tar -xzf $BASEDIR/$2.$TARSUFFIX
    echo "INFO:   downloaded tar file: $2.$TARSUFFIX " | tee -a $LOGFILE
  else
    echo "INFO:   source tree already exists" | tee -a $LOGFILE
  fi
}


# main code

TOOLSDIR=
BASEDIR=
LOGDIR=`pwd`
LOGFILE=$LOGDIR/traf_tools_setup.log
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
  make  >>$LOGFILE 2>&1
  echo "INFO:   make completed" | tee -a $LOGFILE
  make install  >>$LOGFILE 2>&1
  if [ ! -d $TOOLSDIR/dest-mpich-3.0.4/bin ]; then
    echo "ERROR:  failed to install MPI" | tee -a $LOGFILE
    echo "  see details in $LOGFILE" | tee -a $LOGFILE
    exit 2;
  fi
  echo "INFO:   make install complete, files placed in $TOOLSDIR"
fi
echo "INFO: MPI installation complete" | tee -a $LOGFILE
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
  make >>$LOGFILE 2>&1
  echo "INFO:   make completed" | tee -a $LOGFILE
  make install >>$LOGFILE 2>&1
  if [ ! -d $TOOLSDIR/bison_3_linux/bin ]; then
    echo "ERROR:  failed to install Bison" | tee -a $LOGFILE
    echo "  see details in $LOGFILE" | tee -a $LOGFILE
    exit 2;
  fi
  echo "INFO:   make install complete, files placed in $TOOLSDIR"
fi
echo "INFO: Bison installation complete" | tee -a $LOGFILE
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
  make >>$LOGFILE 2>&1
  echo "INFO:   make completed" | tee -a $LOGFILE
  make install >>$LOGFILE 2>&1
  if [ ! -d $TOOLSDIR/udis86-1.7.2/bin ]; then
    echo "ERROR:  failed to install UDIS" | tee -a $LOGFILE
    echo "  see details in $LOGFILE" | tee -a $LOGFILE
    exit 2;
  fi
  echo "INFO:   make install complete, files placed in $TOOLSDIR"
fi
echo "INFO: UDIS installation complete" | tee -a $LOGFILE
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

    echo "INFO:   building release - this will take some time"
    make libs-only >>$LOGFILE 2>&1
    echo "INFO:   release make completed" | tee -a $LOGFILE
    make install-libs >>$LOGFILE 2>&1
    if [ ! -d $TOOLSDIR/dest-llvm-3.2/release/bin ]; then
      echo "ERROR:  failed to install release LLVM" | tee -a $LOGFILE
      echo "  see details in $LOGFILE" | tee -a $LOGFILE
      exit 2;
    fi
  fi
  echo "INFO:   release make install complete, files placed in $TOOLSDIR"

  # Build debug version
  if [ ! -d $TOOLSDIR/dest-llvm-3.2/debut/bin ]; then
    mkdir -p $MY_LLVM_OBJ_DIR/debug
    cd $MY_LLVM_OBJ_DIR/debug

    $MY_LLVM_SRC_DIR/configure --prefix=$MY_LLVM_INSTALL_DIR/debug \
       --enable-optimized --enable-jit \
       --enable-debug-runtime --enable-debug-symbols \
       --enable-shared --enable-targets=x86,x86_64,cpp \
       --with-udis86=$MY_UDIS_INSTALL_DIR/lib \
       CFLAGS=-fgnu89-inline >>$LOGFILE 2>&1
    echo "INFO:   debug configure complete" | tee -a $LOGFILE
    echo "INFO:   building debug - this will take some time"
    make libs-only >>$LOGFILE 2>&1
    echo "INFO:   debug make completed" | tee -a $LOGFILE
    make install-libs >>$LOGFILE 2>&1
    if [ ! -d $TOOLSDIR/dest-llvm-3.2/debug/bin ]; then
      echo "ERROR:  failed to install debug LLVM" | tee -a $LOGFILE
      echo "  see details in $LOGFILE" | tee -a $LOGFILE
      exit 2;
    fi
  fi
  echo "INFO:   debug make install complete, files placed in $TOOLSDIR"
fi
echo "INFO: LLVM installation complete" | tee -a $LOGFILE
echo " *********************************************************** " | tee -a $LOGFILE


# Build ICU
cd $BASEDIR
echo
echo "INFO: Installing ICU on $(date)" | tee -a $LOGFILE
if [ -d $TOOLSDIR/icu4.4/linux64/bin ]; then
  echo "INFO: ICU is already installed, skipping to next tool" | tee -a $LOGFILE
else
  downloadSource http://download.icu-project.org/files/icu4c/4.4/icu4c-4_4-src.tgz icu4c-4_4-src
  cd icu/source
  ./configure --with-library-suffix=Nv44 --prefix=$TOOLSDIR/icu4.4/linux64 >>$LOGFILE 2>&1
  echo "INFO:   configure complete" | tee -a $LOGFILE
  make >>$LOGFILE 2>&1
  echo "INFO:   make completed" | tee -a $LOGFILE
  make install  >>$LOGFILE 2>&1
  if [ ! -d $TOOLSDIR/icu4.4/linux64/bin ]; then
    echo "ERROR:  failed to install ICU" | tee -a $LOGFILE
    echo "  see details in $LOGFILE" | tee -a $LOGFILE
    exit 2;
  fi
  echo "INFO:   make install complete, files placed in $TOOLSDIR"
fi
echo "INFO: ICU installation complete" | tee -a $LOGFILE
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
  make >>$LOGFILE 2>&1
  echo "INFO:   make completed" | tee -a $LOGFILE
  make install  >>$LOGFILE 2>&1
  if [ ! -d $TOOLSDIR/zookeeper-3.4.5/bin ]; then
    echo "ERROR:  failed to install ZooKeeper" | tee -a $LOGFILE
    echo "  see details in $LOGFILE" | tee -a $LOGFILE
    exit 2;
  fi
  echo "INFO:   make install complete, files placed in $TOOLSDIR"
fi
echo "INFO: ZooKeeper installation complete" | tee -a $LOGFILE
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
  make >>$LOGFILE 2>&1
  echo "INFO:   make completed" | tee -a $LOGFILE
  make install  >>$LOGFILE 2>&1
  if [ ! -d $TOOLSDIR/thrift-0.9.0/bin ]; then
    echo "ERROR:  failed to install Thrift" | tee -a $LOGFILE
    echo "  see details in $LOGFILE" | tee -a $LOGFILE
    exit 2;
  fi
  echo "INFO:   make install complete, files placed in $TOOLSDIR"
fi
echo "INFO: Thrift installation complete" | tee -a $LOGFILE
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

echo
echo "INFO: Completed tools build on $(date)" | tee -a $LOGFILE
echo "INFO: List of tools directory: " | tee -a $LOGFILE
ls $TOOLSDIR | echo | tee -a $LOGFILE
echo "`ls $TOOLSDIR`" | tee -a $LOGFILE
echo
