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
This page describes how you download and install the Trafodion product environment.  Please refer to [Create Development Environment](create-dev-environment.html) if you want to set up a Trafodion development environment (which includes the installation of a stand-alone Hadoop environment) based on the Trafodion source code.

# Prerequisite
Trafodion is installed onto an existing Hadoop environment. Currently, one of the following environments are required:

* Cloudera CDH 5.2
* Cloudera CDH 5.3
* Hortonworks HDP 2.2.

We're actively working on removing this restriction.

# Download
The Trafodion product environment is installed using the Trafodion Installer, which operates on Trafodion binaries only.

## Binaries
The Trafodion binaries are available as a tar file. 

Please download from: https://dist.apache.org/repos/dist/release/incubator/trafodion/apache-trafodion-1.3.0-incubating/

## Source
Build your own binaries from the Trafodion source code as follows:

1. [Setup Build Environment](setup-build-environment.html).
2. [Build Trafodion](build.html) — use **```make package```**.

Git site: git@github.com:apache/incubator-trafodion

The source tar file has been signed with pgp key A44C5A05 which is included in the download location’s KEYS file:
https://dist.apache.org/repos/dist/release/incubator/trafodion/KEYS

# Install
Please refer to the [Install](install.html) instructions.



