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
This page describes how to create the test environment used for Trafodion development, which is intended for people that are contributing to the Trafodion source tree. Please refer to [Download](download.html) if you want to try the Trafodion product environment.

# Prerequisites
The following prerequisites need to be met in order to create a functional Trafodion test environment.

## Passwordless ```ssh```
Check to see if you have passwordless SSH setup correctly.

    ssh localhost
    Last login: Fri Nov  6 22:44:00 2015 from 192.168.1.9

If the **```ssh localhost```** command prompts for a password, then passwordless **```ssh```** is not set up correctly.

The following is an example of setting up passwordless **```ssh```** using **```id_rsa```** keys. You can choose the method that best represents your environment.

If you already have an existing set of **```ssh```** keys. Simply copy both the **```id_rsa.pub```** and **```id_rsa```** to your **```~/.ssh```** directory.

Then, do the following to modify your **```ssh```** environment.

    cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys
    chmod 600 ~/.ssh/id_rsa
    echo "NoHostAuthenticationForLocalhost=yes" >>~/.ssh/config
    chmod go-w ~/.ssh/config
    chmod 755 ~/.ssh; chmod 640 ~/.ssh/authorized_keys; cd ~/.ssh; chmod 700 ..

If you need to create your keys first, then do the following.

    rm -rf ~/.ssh
    ssh-keygen -t rsa -N "" -f ~/.ssh/id_rsa
    cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys
    chmod 600 ~/.ssh/id_rsa.pub 
    echo "NoHostAuthenticationForLocalhost=yes" >>~/.ssh/config
    chmod go-w ~/.ssh/config
    chmod 755 ~/.ssh; chmod 640 ~/.ssh/authorized_keys; cd ~/.ssh; chmod 700 ..

## System Limits
Please check that the system limits in your environment are appropriate for Apache Trafodion. If they are not, then you will need to increase the limits or Trafodion cannot start.

The recommended settings are as follows.

    $ ulimit –a
    core file size             (blocks, -c) 1000000
    data seg size              (kbytes, -d) unlimited
    scheduling priority        (-e) 0
    file size                  (blocks, -f) unlimited
    pending signals            (-i) 515196
    max locked memory          (kbytes, -l) 49595556
    max memory size            (kbytes, -m) unlimited
    open files                 (-n) 32000
    pipe size                  (512 bytes, -p) 8
    POSIX message queues       (bytes, -q) 819200
    real-time priority         (-r) 0
    stack size                 (kbytes, -s) 10240
    cpu time                   (seconds, -t) unlimited
    max user processes         (-u) 267263
    virtual memory             (kbytes, -v) unlimited
    file locks                 (-x) unlimited

Please refer to this [article](http://www.itworld.com/article/2693414/setting-limits-with-ulimit.html) for information on how you change system limits.

# Setup
You can create a Trafodion test environment using a:

* **Pre-Installed Hadoop**: Trafodion installation on a system that already has a compatible version of Hadoop installed
* **Local Hadoop**: You install a Hadoop environment using the **```install_local_hadoop```** script

Your installation approach depends on whether you already have installed Hadoop.

## Pre-Installed Hadoop
Use the following instructions if you're installing Trafodion on a pre-installed Hadoop environment. 
### Build Binary tar Files
Build the Trafodion binary tar files.

    cd <Trafodion source directory>
    make package

### Install Trafodion
Please refer to the installation instructions described in the [Installation](install.html) page.

## Local Hadoop
Use the following instructions if you need to install a local Hadoop environment.
### Run ```install_local_hadoop```
The **```install_local_hadoop```** script downloads compatible versions of Hadoop, HBase, Hive, and MySQL. Then, it starts Trafodion.

<div class="alert alert-dismissible alert-info">
  <button type="button" class="close" data-dismiss="alert">&close;</button>
  <p style="color:black"><strong>Time Saver</strong></p>
  <p style="color:black"><strong>```install_local_hadoop```</strong> downloads Hadoop, HBase, Hive, and MySql jar files from the Internet. To avoid this overhead, you can download the required files into a separate directory and set the environment variable <strong>```MY_LOCAL_SW_DIST```</strong> to point to this directory.</p>
</div>

Command                                        | Usage
-----------------------------------------------|--------------------------------------------------------
**```install_local_hadoop```**                 | Uses default ports for all services.
**```install_local_hadoop -p fromDisplay```**  | Start Hadoop with a port number range determined from the DISPLAY environment variable.
**```install_local_hadoop -p rand```**         | Start with any random port number range between 9000 and 49000.
**```install_local_hadoop -p <port>```**       | Start with the specified port number.

For a list of ports that get configured and their default values, please refer to [Port Assignments](port-assignment.html).

### Sample Procedure
Start a new **```ssh```** session and ensure that the Trafodion environmental variables are loaded.

    cd <Trafodion source directory>
    source ./env.sh

Install the Hadoop software.

    cd $MY_SQROOT/sql/scripts
    install_local_hadoop
    ./install_traf_components

Verify installation.

    $ swstatus
    6 java servers and 2 mysqld processes are running
    713   NameNode
    19513 HMaster
    1003  SecondaryNameNode
    838   DataNode
    1173  ResourceManager
    1298  NodeManager

Six java servers as shown above and two mysqld processes should be running.

# Install and Build Trafodion
Please refer to [Modify Code](code.html) for information on how to install and build Trafodion from its source code.

# New Source Download
You need to do the following each time you download new source code.

    cd <Trafodion source directory>
    source ./env.sh
    cd $MY_SQROOT/etc
    # delete ms.env, if it exists
    rm ms.env
    cd $MY_SQROOT/sql/scripts
    sqgen

# Manage
Please refer to [Manage Development Environment](manage-dev-environment.html) for instructions.
