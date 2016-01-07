<!--
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
      http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the 
  License.
-->
This page describes how to perform manual installs of the required [Trafodion Build Tools](setup-build-environment.html#Install_Build_Tools).

In the sections below, the **```<tool installation directory>```** is the directory where you want the tool to be installed.

# MPICH
**Tested Version**: 3.0.4

**Download**: http://www.mpich.org/static/downloads/3.0.4/mpich-3.0.4.tar.gz (http://www.mpich.org/downloads)

**Considerations**: For more detailed instructions, see the README file that comes with the source.

**Install**:

    tar -xzf mpich-3.0.4.tar.gz
    cd mpich-3.0.4
    ./configure --prefix=<tool installation directory>/dest-mpich-3.0.4 --with-device=ch3:sock --disable-f77 --disable-fc
    make
    make check
    make install

**```<tool installation directory>```** is the directory where you want MPICH to be installed. If you do not specify the **```--prefix```** option, the default location is **```/usr/local``**.

# Bison
**Tested Version**: 3.0

**Download**: http://ftp.gnu.org/gnu/bison/bison-3.0.tar.gz (http://ftp.gnu.org/gnu/bison/)

**Considerations**: Refer to the bison INSTALL file for detailed instructions. 

**Determine Bison Version**:

    which bison
    bison --version

If the version is older than 3.0, then do the following:

    tar -xzf bison-3.0.tar.gz
    cd bison-3.0
    ./configure --prefix=<tool installation directory>/bison_3_linux
    make
    make check
    make install

**Note**: The **```make check```** step may return errors like the following that can be ignored:

    make[3]: Entering directory `<mydir>/bison-3.0'
      YACC     examples/calc++/calc++-parser.stamp
      CXX      examples/calc++/examples_calc___calc__-calc++-driver.o
      LEX      examples/calc++/calc++-scanner.cc
      CXX      examples/calc++/examples_calc___calc__-calc++-scanner.o
    g++: ./examples/calc++/calc++-scanner.cc: No such file or directory
    g++: no input file

Adjust your **```PATH```** to ensure that the correct version is chosen. Rerun the **```bison --version```** to verify.

# Udis86
**Tested Version**: 1.7.2

**Download**:  http://sourceforge.net/projects/udis86/files/udis86/1.7/udis86-1.7.2.tar.gz (http://udis86.sourceforge.net)

**Consideration**: Udis86 is a prerequisite to building the LLVM product.

**Install**:

    tar xzf udis86-1.7.2.tar.gz
    cd udis86-1.7.2
    ./configure --prefix=<tool installation directory>/udis86-1.7.2 --enable-shared
    make
    make install

# LLVM 
**Tested Version**: 3.2

**Download**: http://llvm.org/releases/3.2/llvm-3.2.src.tar.gz (http://llvm.org/releases/download.html)

**Consideration**: Udis86 must be installed on the system before LLVM is built and installed. Building LLVM takes some time to complete, be patient.

**Install**:

    # Set BASE_DIR to the top-level directory where the LLVM source will be
    # unpacked and the objects compiled.
    BASE_DIR=<your-base-dir>
    cd $BASE_DIR
    tar xzf llvm-3.2.src.tar.gz
     
    export MY_UDIS_INSTALL_DIR=<udis-installation-directory>/udis86-1.7.2
    export MY_LLVM_INSTALL_DIR=<llvm-installation-directory>/dest-llvm-3.2/
    export MY_LLVM_SRC_DIR=$BASE_DIR/llvm-3.2.src
    export MY_LLVM_OBJ_DIR=$BASE_DIR/llvm-3.2.obj/
    export LD_LIBRARY_PATH=$MY_UDIS_INSTALL_DIR/lib:$LD_LIBRARY_PATH
    export C_INCLUDE_PATH=$MY_UDIS_INSTALL_DIR/include
    export CPATH=$MY_UDIS_INSTALL_DIR/include
     
    mkdir -p $MY_LLVM_OBJ_DIR/release
    cd $MY_LLVM_OBJ_DIR/release
    
    $MY_LLVM_SRC_DIR/configure --prefix=$MY_LLVM_INSTALL_DIR/release \
    --enable-optimized --enable-jit \
    --enable-shared --enable-targets=x86,x86_64,cpp \
    --with-udis86=$MY_UDIS_INSTALL_DIR/lib \
    CFLAGS=-fgnu89-inline
    
    make libs-only
    make install-libs
     
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

# ICU
**Tested Version**: 4.4.0

**Download**: http://download.icu-project.org/files/icu4c/4.4/icu4c-4_4-src.tgz (http://site.icu-project.org/download)

**Install**:

    tar -xzf icu4c-4_4-src.tgz
    cd icu/source
    ./runConfigureICU Linux --with-library-suffix=Nv44 --prefix=<tool installation directory>/icu4.4/linux64
    make && make check
    make install

**Note**: The following **```make check```** errors can be ignored.

    [All tests passed successfully...]
    Elapsed Time: 00:00:12.126
    make[2]: Leaving directory `/home/centos/icu/source/test/cintltst'
    ---------------
    ALL TESTS SUMMARY:
    ok:  testdata iotest cintltst
    ===== ERRS:  intltest
    make[1]: *** [check-recursive] Error 1
    make[1]: Leaving directory `/home/centos/icu/source/test'
    make: *** [check-recursive] Error 2

# Zookeeper
**Tested Version**: 3.4.5

**Download**: https://archive.apache.org/dist/zookeeper/zookeeper-3.4.5/zookeeper-3.4.5.tar.gz

**Install**:

    tar -xzf zookeeper-3.4.5.tar.gz
    cd zookeeper-3.4.5/src/c
    ./configure --prefix=<tool installation directory>/zookeeper-3.4.5
    make
    make install

# Thrift
**Tested Version**: 0.9.0

**Download**: http://archive.apache.org/dist/thrift/0.9.0/

**Consideration**: Behind a firewall, you may need the ant flags to specify a proxy.

**Install**:

    tar -xzf thrift-0.9.0.tar.gz
    cd thrift-0.9.0
    ./configure --prefix=<tool installation dir>/thrift-0.9.0 --without-qt
    make
    make install

# Maven
**Tested Version**: 3.3.3

**Download**: http://archive.apache.org/dist/maven/maven-3/3.3.3/binaries/apache-maven-3.3.3-bin.tar.gz.

**Considerations**: Add Maven to your **```PATH```** once it has been installed.

**Install**:

    tar -xzf apache-maven-3.3.3-bin.tar.gz -C <tool installation directory>
 