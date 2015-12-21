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
This page describes how you set up the Trafodion build environment.

# Supported Platforms
Red Hat or Centos 6.x (6.4 or later) versions are supported as development and production platforms.

# Install Required Packages
You need to install the following packages before you can build Trafodion.

    sudo yum install epel-release
 
    sudo yum install alsa-lib-devel ant ant-nodeps boost-devel cmake \
             device-mapper-multipath dhcp flex gcc-c++ gd git glibc-devel \
             glibc-devel.i686 graphviz-perl gzip java-1.7.0-openjdk-devel \
             libX11-devel libXau-devel libaio-devel \
             libcurl-devel libibcm.i686 libibumad-devel libibumad-devel.i686 \
             libiodbc libiodbc-devel librdmacm-devel librdmacm-devel.i686 \
             libxml2-devel log4cxx log4cxx-devel lua-devel lzo-minilzo \
             net-snmp-devel net-snmp-perl openldap-clients openldap-devel \
             openldap-devel.i686 openmotif openssl-devel openssl-devel.i686 \
             openssl-static perl-Config-IniFiles perl-Config-Tiny \
             perl-DBD-SQLite perl-Expect perl-IO-Tty perl-Math-Calc-Units \
             perl-Params-Validate perl-Parse-RecDescent perl-TermReadKey \
             perl-Time-HiRes protobuf-compiler protobuf-devel python-qpid \
             python-qpid-qmf qpid-cpp-client \
             qpid-cpp-client-ssl qpid-cpp-server qpid-cpp-server-ssl \
             qpid-qmf qpid-tools readline-devel saslwrapper sqlite-devel \
             unixODBC unixODBC-devel uuid-perl wget xerces-c-devel xinetd

# Verify Java Version
The Java version must be 1.7.x. Check as following:

    $ java -version
    java version "1.7.0_85"
    OpenJDK Runtime Environment (rhel-2.6.1.3.el6_6-x86_64 u85-b01)
    OpenJDK 64-Bit Server VM (build 24.85-b03, mixed mode)

Ensure that the Java environment exists and points to your JDK installation. By default Java is located in **```/usr/lib/java-\<version\>```**.

    $ echo $JAVA_HOME
    $ export JAVA_HOME=/usr/lib/jvm/java-1.7.0-openjdk.x86_64

# Download and Install Source
The Trafodion source code contains tools that help you set up the build environment.
## Git
Please refer to [Making Changes](develop.html#making_changes) on the [Develop](develop.html) page.

## tar file
The source code for Apache Trafodion can be downloaded from [Apache Trafodion Incubator Release](https://dist.apache.org/repos/dist/release/incubator) as a tar file.  

* Download the source tar file to your **```<trafodion download directory>```**.
* Check the tar file validity by checking signatures, please refer to [Verify Signatures](release.html#Verify_Signatures). The Trafodion releases have been signed using The GNU Privacy Guard. 

**Unpack the tar file**
     
     cd <trafodion download directory>
     tar -xzf <tar file>

# Install Build Tools
Trafodion requires that several tools are installed in order to build. These tools are:

Tool                                   | Description
---------------------------------------|-----------------------------------------------------------------
**Bison**                              | General-purpose parser generator.
**ICU**                                | Set of C/C++ and Java libraries providing Unicode and Globalization support for software applications.
**LLVM**                               | Collection of modular and reusable compiler and tool-chain technologies.
**Maven**                              | Build tool that is only installed if compatible version does not exist.
**MPICH**                              | An implementation of the Message Passing Interface (MPI) standard.  For use in Trafodion, MPICH must be built to force sockets to be used in both internode and intranode message passing.
**Thrift**                             | Communications and data serialization tool.
**Udis86**                             | Minimalistic disassembler library (libudis86) for the x86 class of instruction set architectures.
**Zookeeper**                          | Coordination service for distributed applications.  It exposes common services such as naming, configuration management, synchronization, and group services.

You can perform the required installation using the Trafodion **```traf_tools_setup.sh```** script or by installing each tool manually.

## ```traf_tools_setup.sh```
**```traf_tools_setup.sh```** is a script that uses **```wget```** to download the appropriate tar file, build, and install the required tool into a directory of your choice for each tool required tools.  

The advantage of this method is that all the correct tools are downloaded and built in a single directory.  Before building, a single environment variable needs to be set: **```TOOLSDIR```**.

<div class="alert alert-dismissible alert-info">
  <button type="button" class="close" data-dismiss="alert">&close;</button>
  <p style="color:black"><strong>Note</strong></p>
  <p style="color:black">You may want to modify <strong><code>traf_tools_setup.sh</code></strong> for your specific environment. Example: if you already have Zoopkeeper installed, you may not want to re-install it.</p>
  <p style="color:black">You may need root or sudo access to installs the tools in desired locations.</p>
  <p style="color:black">In the sections below, <strong><code>incubator-trafodion</code></strong> represents the root directory where you installed the Trafodion source.</p>
</div>

**Usage**

    cd <Trafodion source directory>/install
    ./traf_tools_setup.sh -h
    Usage: ./traf_tools_setup.sh -d <downloaddir> -i <installdir>
    -d <downloaddir> - location of download directory
    -i <installdir>  - location of install directory
    -h - help
    example: traf_tools_setup.sh -d /home/userx/download -i /home/userx/tools

Run **```traf_tools_setup.sh```** to install all dependent tools.

**Example**

    $ mkdir ~/download
    $ ./traf_tools_setup.sh -d ~/download -i ~/tools
    INFO: Starting tools build on Fri Nov  6 21:33:53 PST 2015
    Tools install directory /home/centos/tools does not exist, do you want to to create it? y/n : y
    INFO: Created directory /home/centos/tools
    INFO: Tar download location: /home/centos/download
    INFO: Tool install directory location: /home/centos/tools
    INFO: LogFile location: /home/centos/incubator-trafodion/install/traf_tools_setup.log
    ***********************************************************
    INFO: Installing MPI on Fri Nov  6 21:34:00 PST 2015
    INFO:   downloaded tar file: mpich-3.0.4.tar.gz
    .
    .
    .
    INFO:   downloaded tar file:  apache-maven-3.3.3-bin.tar.gz
    INFO: Maven installation complete
    ***********************************************************
    INFO: Completed tools build on Fri Nov  6 22:23:22 PST 2015
    INFO: List of tools directory:
    apache-maven-3.3.3
    bison_3_linux
    dest-llvm-3.2
    dest-mpich-3.0.4
    icu4.4
    thrift-0.9.0
    udis86-1.7.2
    zookeeper-3.4.5

Export **```TOOLSDIR```** in **```.bashrc```** or **```.profile```**.

    export TOOLSDIR=~/tools
 
## Manual Installation
Please refer to [Build Tools Manual Installation](build-tools-manual.html).

# Verify Build Environment
## Verify Maven
Check that Maven is installed.

    mvn --version

If Maven is not found, then you should add Maven to your **```PATH```** environmental variable in **```.bashrc```** or **```.profile```**.

    PATH=$PATH:<tool installation directory>/apache-maven-3.3.3/bin

At this point, your build environment has been set up. You should now be able to [Build Trafodion](build.html).