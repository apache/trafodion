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
This page describes how you manage the Trafodion development test environment, which is intended for people that are contributing to the Trafodion source tree. Please refer to [Download](download.html) if you want to try the Trafodion product environment.

# Prerequisites
You must have created the [Trafodion Development Environment](create-dev-environment.html) before using the instructions on this page.

# Hadoop Enviroment
## Pre-Existing Hadoop
If you are doing Trafodion development on a pre-existing Hadoop distribution, then do the following:

* **Distribution**: Use the distribution management too. For example: Apache Ambari or Cloudera Manager.
* **Regular Hadoop**: Use the start/stop script for each Hadoop environment.

## Local Hadoop
Use the following commands to manage the Hadoop environment.

Command                            | Usage
-----------------------------------|-----------------------------------------
**```swstartall```**               | Start the complete Hadoop environment.
**```swstopall```**                | Stops the complete Hadoop environment.
**```swstatus```**                 | Checks the status of the Hadoop environment.
**```swuninstall_local_hadoop```** | Removes the Hadoop installation.

# Trafodion
Please refer to [Trafodion Management](management.html).

# New Source Download
You need to do the following each time you download new source code.

    cd <Trafodion source directory>
    source ./env.sh
    cd $MY_SQROOT/etc
    # delete ms.env, if it exists
    rm ms.env
    cd $MY_SQROOT/sql/scripts
    sqgen


