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
   echo " -v               - verbose mode"
   echo
   echo " -h - help"
   echo
   echo "example: traf_tools_setup.sh -d /home/userx/download -i /home/userx/tools"
   echo
}

  VERBOSE=0
  TOOLSDIR=
  BASEDIR=

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
          v)
             VERBOSE=1;
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
       echo "Created directory $TOOLSDIR"
     fi
  else
    echo
    echo "ERROR: install directory ($TOOLSDIR) does not exist"
    echo
    exit 1;
  fi
fi

if [ $VERBOSE -eq 1 ]; then
  echo "basedir is: $BASDIR";
  echo "toolsdir is: $TOOLSDIR":
fi

#install mpi
cd $BASEDIR
wget http://www.mpich.org/static/downloads/3.0.4/mpich-3.0.4.tar.gz
tar -xzf mpich-3.0.4.tar.gz
cd mpich-3.0.4
./configure --prefix=$TOOLSDIR/dest-mpich-3.0.4 --with-device=ch3:sock --disable-f77 --disable-fc
make
make check
make install
if [ $VERBOSE -eq 1 ]; then
  echo "STEP completed:  installation of MPI"
fi

# See if need to install protobufs tested with version 2.3.0
protoc --version

# install bison
cd $BASEDIR
wget http://ftp.gnu.org/gnu/bison/bison-3.0.tar.gz
tar -xzf bison-3.0.tar.gz
cd bison-3.0
./configure --prefix=$TOOLSDIR/bison_3_linux
make
make check
make install
if [ $VERBOSE -eq 1 ]; then
  echo "STEP completed:  installation of BISON"
fi

# install udis
cd $BASEDIR
wget http://sourceforge.net/projects/udis86/files/udis86/1.7/udis86-1.7.2.tar.gz
tar -xzf udis86-1.7.2.tar.gz
cd udis86-1.7.2
./configure --prefix=$TOOLSDIR/udis86-1.7.2 --enable-shared
make
make check
make install
if [ $VERBOSE -eq 1 ]; then
  echo "STEP completed:  installation of UDIS"
fi

#install LLVM

cd $BASEDIR
wget http://llvm.org/releases/3.2/llvm-3.2.src.tar.gz
tar xzf llvm-3.2.src.tar.gz
  
export MY_UDIS_INSTALL_DIR=$TOOLSDIR/udis86-1.7.2
export MY_LLVM_INSTALL_DIR=$TOOLSDIR/dest-llvm-3.2/
export MY_LLVM_SRC_DIR=$BASEDIR/llvm-3.2.src
export MY_LLVM_OBJ_DIR=$BASEDIR/llvm-3.2.obj/
export LD_LIBRARY_PATH=$MY_UDIS_INSTALL_DIR/lib:$LD_LIBRARY_PATH
export C_INCLUDE_PATH=$MY_UDIS_INSTALL_DIR/include
export CPATH=$MY_UDIS_INSTALL_DIR/include

# Build release version
mkdir -p $MY_LLVM_OBJ_DIR/release
cd $MY_LLVM_OBJ_DIR/release

$MY_LLVM_SRC_DIR/configure --prefix=$MY_LLVM_INSTALL_DIR/release \
   --enable-shared --enable-targets=x86,x86_64,cpp \
   --with-udis86=$MY_UDIS_INSTALL_DIR/lib \
   CFLAGS=-fgnu89-inline

make libs-only
make install-libs
if [ $VERBOSE -eq 1 ]; then
  echo "STEP completed:  installation of LLVM release"
fi

# Build debug version
mkdir -p $MY_LLVM_OBJ_DIR/debug
cd $MY_LLVM_OBJ_DIR/debug

$MY_LLVM_SRC_DIR/configure --prefix=$MY_LLVM_INSTALL_DIR/debug \
   --enable-optimized --enable-jit \
   --enable-debug-runtime --enable-debug-symbols \
   --enable-shared --enable-targets=x86,x86_64,cpp \
   --with-udis86=$MY_UDIS_INSTALL_DIR/lib \
   CFLAGS=-fgnu89-inline

make libs-only
make install-libs
if [ $VERBOSE -eq 1 ]; then
  echo "STEP completed:  installation of LLVM debug"
fi

# Build ICU
cd $BASEDIR
wget http://download.icu-project.org/files/icu4c/4.4/icu4c-4_4-src.tgz
tar -xzf icu4c-4_4-src.tgz
cd icu/source
./configure --with-library-suffix=Nv44 --prefix=$TOOLSDIR/icu4.4/linux64
make
make check
make install
if [ $VERBOSE -eq 1 ]; then
  echo "STEP completed:  installation of ICU"
fi

# make zookeeper
cd $BASEDIR
wget https://archive.apache.org/dist/zookeeper/zookeeper-3.4.5/zookeeper-3.4.5.tar.gz 
tar -zxf zookeeper-3.4.5.tar.gz
cd zookeeper-3.4.5/src/c
./configure --prefix=$TOOLSDIR/zookeeper-3.4.5
make
make install
if [ $VERBOSE -eq 1 ]; then
  echo "STEP completed:  installation of ZOOKEEPER"
fi

# make thrift
cd $BASEDIR
wget http://archive.apache.org/dist/thrift/0.9.0/thrift-0.9.0.tar.gz
tar -xzf thrift-0.9.0.tar.gz
cd thrift-0.9.0
./configure --prefix=$TOOLSDIR/thrift-0.9.0 -without-qt
make
make install
if [ $VERBOSE -eq 1 ]; then
  echo "STEP completed:  installation of THRIFT"
fi
