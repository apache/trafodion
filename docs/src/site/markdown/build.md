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
This page describes how to build the Trafodion source code.

# Prerequisites
You need to [Setup Build Environment](setup-build-environment.html) before trying to build the Trafodion source code.

# Download Source
You should already have downloaded the source code when setting up the build environment.

## Git
Please refer to [Making Changes](develop.html#making_changes) on the [Develop](develop.html) page.

## tar file
The source code for Apache Trafodion can be downloaded from [Apache Trafodion Incubator Release](https://dist.apache.org/repos/dist/release/incubator) as a tar file.  

* Download the source tar file to your **```<trafodion download directory>```**.
* Check the tar file validity by checking signatures, please refer to [Verify Signatures](release.html#Verify_Signatures). The Trafodion releases have been signed using The GNU Privacy Guard. 

**Unpack the tar file**
     
     cd <trafodion download directory>
     tar -xzf <tar file>

# Set Up Environmental Variables
Start a new **```ssh```** session. Use the following commands to set up the Trafodion environmental variables.

    cd <Trafodion source directory>
    export TOOLSDIR=<tools installation directory>
    source ./env.sh

* **```<Trafodion source directory>```**: Source tree base for Trafodion.
* **```<tools installation directory>```**:  where Trafodion required tools are located. The following example assumes that you installed all the required tools in a single location. If you installed or used pre-installed tools in different directories, then you need to export the location of each tool as described in [Build Tools Manual Installation](build-tools-manual.html) prior to sourcing in **```env.sh```**.

# Build Commands
Build a debug version of Trafodion using one of the following options.

Command                             | What It Builds
------------------------------------|----------------------------------------------------------------------------------
**```make all```**                  | Trafodion, DCS, and REST.
**```make package```**              | Trafodion, DCS, REST, and Client Drivers.
**```make package-all```**          | Trafodion, DCS, REST, Client Drivers, and tests for all components.

If the build fails, you might want to rerun the **```make```** step. Trafodion downloads many dependencies and sometimes one of the download operations fail. Rerunning the build generally works.

# Verify Build
Use **```sqvers -u```** to verify the build.

    $ sqvers -u
    MY_SQROOT=/home/centos/apache-trafodion-1.3.0-incubating/core/sqf
    who@host=centos@mysystem
    JAVA_HOME=/usr/lib/jvm/java-1.7.0-openjdk-1.7.0.91.x86_64
    SQ_MBTYPE=64d (64-debug)
    linux=2.6.32-504.1.3.el6.x86_64
    redhat=6.7
    NO patches
    Most common Apache_Trafodion Release 1.3.0 (Build debug [centos], branch -, date 06Nov15)
    UTT count is 1
    [6]     Release 1.3.0 (Build debug [centos], branch -, date 06Nov15)
              export/lib/hbase-trx-cdh5_3-1.3.0.jar
              export/lib/hbase-trx-hbase_98_4-1.3.0.jar
              export/lib/hbase-trx-hdp2_2-1.3.0.jar
              export/lib/sqmanvers.jar
              export/lib/trafodion-dtm-1.3.0.jar
              export/lib/trafodion-sql-1.3.0.jar

The output from the **```sqvers -u```** commands should show several jar files. The number of files differs based on the version of Trafodion you downloaded.

