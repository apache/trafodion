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
This page describes how you download and install a Trafodion evaluation environment. Other information:

* [Trafodion Provision Guide](http://trafodion.incubator.apache.org/docs/provisioning_guide/index.html): Complete provisioning documentation.
* [Trafodion Contributor Guide](https://cwiki.apache.org/confluence/display/TRAFODION/Trafodion+Contributor+Guide): 
How to set up a Trafodion development environment.

# Download
The Trafodion end-user environment is installed using the Trafodion Installer, which operates on Trafodion binaries only.

## 1.3.0 Binaries

* [log4c++ RPM](http://traf-builds.esgyn.com/downloads/trafodion/publish/release/1.3.0/log4cxx-0.10.0-13.el6.x86_64.rpm)
* [Trafodion Installer](http://traf-builds.esgyn.com/downloads/trafodion/publish/release/1.3.0/apache-trafodion-installer-1.3.0-incubating-bin.tar.gz)
* [Trafodion Server](http://traf-builds.esgyn.com/downloads/trafodion/publish/release/1.3.0/apache-trafodion-1.3.0-incubating-bin.tar.gz)
* [Trafodion Clients](http://traf-builds.esgyn.com/downloads/trafodion/publish/release/1.3.0/apache-trafodion-clients-1.3.0-incubating-bin.tar.gz)

# Install

The following steps installs a Trafodion evaluation on a 64-bit RHEL/CentOS Hadoop environment running on of the following
Hadoop distributions:

## Prerequisites
Trafodion is installed as an add-on to an existing Hadoop environment. Currently, one of the following Hadoop distributions is **required**:

* Cloudera CDH 5.2
* Cloudera CDH 5.3
* Hortonworks HDP 2.2

The following Hadoop services **must be** in a STARTED state and fully functional.

* HDFS
* YARN with MRv2
* ZooKeeper
* HBase
* Hive (optional)

## Create Provisioning User

Create a Linux user on the Hadoop system that you're installing Trafodion on with the following characteristics:

* Privileges to run **`sudo`** with **`root`** access levels.
* Passwordless ssh access from the Provisioning Node to all other nodes in the cluster.
* Not named **`trafodion`**.

Perform the remaining steps in this procedure using the Provisioning User.

## Download Binaries

Download all binaries listed above except the **Trafodion Clients** into a diretory of your choosing

**Example**

```
mkdir $HOME/trafodion
mkdir $HOME/trafodion/downloads
cd $HOME/trafodion/downloads
wget <log4c++ RPM>
wget <Trafodion Installer>
wget <Trafodion Server>
```

## Install log4c++

Use **`yum install`** to install the `log4c++` RPM on all nodes on all nodes in your Hadoop cluster.

## Unpack Trafodion Installer

Use **`tar -zxf`** to unpack the Trafodion Installer.

**Example**

```
cd $HOME/trafodion/downloads
tar -zxf apache-trafodion-installer-1.3.0-incubating-bin.tar.gz -C $HOME/trafodion
```

## Create Configuration File

The Trafodion Installer uses a configuration file that describes your Hadoop environment.

**Copy Configuration Template File**

```
cd $HOME/trafodion/installer
cp trafodion_config_default my_config
```

**Edit Configuration File**

Use the embedded guidance in the configuration file to define each of these settings:

|-------------------|----------------------------------------------|-------------------|--------------------------------|--------------------------------
**`LOCAL_WORKDIR`** | **`TRAF_USER_PASSWORD`** (change if desired) | **`NODE_LIST`**   | **`node_count`**               | **`MY_NODES`**
**`JAVA_HOME`**     | **`TRAF_PACKAGE`**                           | **`HADOOP_TYPE`** | **`URL`**                      | **`ADMIN`**
**`PASSWORD`**      | **`CLUSTER_NAME`**                           | **`SQ_ROOT`**     | **`START`** (set to **`"Y"`**) | **`INIT_TRAFODION`** (set to **`"Y"`**)

## Run `trafodion_install`

You'll use the Trafodion Installer (**`trafodion_install`**) to create the Trafodion Runtime User (**`trafodion`**), 
modify your system and Hadoop environment, restart ZooKeeper, HDFS, and HBAse to activate configuration changes, 
and install/starts Trafodion.

**Example**

```
cd $HOME/trafodion/installer
./trafodion_install --accept_license --config_file my_config
```

The Trafodion Installer performs all the operations required to install and start Trafodion. Wait for it to complete before
continuing to the next step.

## Verify Installation

Perform this step as the **`trafodion`** user.

First, check the status of the Trafodion environment.

**Example**

```
sudo su trafodion
swstatus
```

Second, run a simple smoke test using **`sqlci`**. Use the following commands:

```
sqlci
>> get schemas;
>> create table table1 (a int);
>> invoke table1;
>> insert into table1 values (1), (2), (3), (4);
>> select * from table1;
>> drop table table1;
>> exit;
```

The Trafodion installation is complete. Next, follow the steps in [Quick Start](quickstart.html) to get started with Trafodion.
