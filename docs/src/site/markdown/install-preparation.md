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
This page describes the steps you need to perform before installing Trafodion.

# Cluster Requirements
The following configuration settings have been tested and are known to work with the Trafodion installation:

Setting                       | Value
------------------------------|-------------------------------------------------
**Hardware Platform**         | x86-64
**Operating System**          | RedHat Enterprise Linux, 6.x kernel (64 bit) or CentOS, 6.x kernel (64 bit)
**Install User IDs**          | A user ID with passwordless and sudo access. Please refer to [Install User ID](#Install User ID) below.
**Cluster Size**              | A cluster consisting of 1 to N nodes. There is currently no upper limit. Two nodes is the recommended minimum, but a single node cluster should work.
**Disk Space**                | Minimum of 20 GB. Please refer to [Checking Disk Space](#Checking_Disk_Space) below.
**Memory**                    | Minimal 1 GB * number of connectivity servers (MXOSRVR processes) configured per cluster.

## Checking Disk Space
### Cloudera Manager
This step is required for Cloudera distributions only.

Before installing Cloudera Manager, a minimum of 20 GB is needed to support the Cloudera management database. Default location: **```/var/lib/cloudera-scm-server-db```**. 

Check the available space for **```/var```** as follows:

* Start a putty or VNC terminal window against the node where Cloudera Manager will be installed.
* As a user with **```root```** privileges, verify that 20 GB of space is available to **```/var```**.

    cd /var
    df -hP

If the **```/var```** space is insufficient, then you can provide a soft link to another drive for your Cloudera Manager database.

* Locate a drive with sufficient disk space.
* Create the soft link.

```
cd <new drive>  # (e.g. cd /DATA)
mkdir cloudera-scm-server-db
chmod 777 cloudera-scm-server-db
cd /var/lib
ln -s <new drive>/cloudera-scm-server-db .
```
 
**Considerations**

* If **```/var```** is a sub-directory in your cluster's root filesystem, then the Cloudera Manager database should have sufficient space available.
* If Cloudera Manager has already been installed and it is showing red for log directories, it means that Cloudera Manager was installed using **```/var/lib```** and may not have a large enough **```/var```** file system. In this case, there is an unsupported script that can be used to move the directories. Please refer to the **```clouderaMoveDB.sh```** script in the **```installer/tools```** directory, which is created when the installer tar file is unpacked. Run **```clouderaMoveDB.sh```** without syntax to display help information.

# Security Requirements
## Install User ID
The Trafodion installation requires a user ID (referred to as **```<sudo-user>```**) with these attributes on all nodes where Trafodion is installed:

* **```/etc/sudoers```* on all nodes:
    * **```sudo```** access — allows the install scripts to run privileged commands.
    * **requiretty** disabled — allows Trafodion install scripts to use embedded **```sudo```** commands
* passwordless ssh to all nodes — allows Trafodion install scripts to communicate with remote nodes without prompting for password.
 
Please create a the **```<sudo-user>```** per the practices of your environment. For example, you may need to request permission from your cluster-management team to obtain this type of access. [Passwordless SSH(passwordless-ssh.html) describes how to set up passwordless ssh.

## Configure LDAP Identity Store
If you plan to enable Trafodion authentication, then you need to have an LDAP identity store available to perform authentication. 

The Trafodion installer prompts you to set up an authentication configuration file that points to an LDAP server (or servers), which enables security (that is, authentication and authorization) in the Trafodion system.

If you wish to manually set up the authentication configuration file and enable security, then please refer to [Enabling Security Features in Trafodion](enable-secure-trafodion.html).

# PC Tools
If you are using a Windows PC, then additional PC software may be needed throughout the installation process. It is recommended that you pre-install the PC software that you need before continuing with the Trafodion installation.

Install the following software:

* putty and puttygen (download from the [PuTTY website](http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html))
* VNC client (download from http://www.realvnc.com)
* Firefox or Chrome browser
* SFTP client to transfer files from your PC to the server: WinSCP or FileZilla

# Install Hadoop Distribution
Trafodion requires that the following Hadoop services are installed:

* HDFS
* MapReduce
* ZooKeeper
* HBase
* Hive

Please ensure that these services are installed for one of the following Hadoop distributions.

<table><tr><td><strong>IMPORTANT</strong><br />Trafodion does not support a Hadoop server running on a node that is not part of the Trafodion cluster. You must specify the Hadoop nodes to be part of the Trafodion list of nodes during the installation of Trafodion (using the <strong><code>--nodes</code></strong> parameter). The Trafodion installer needs to run from the same node where you installed the Hadoop distribution).</td></tr></table>

<table>
  <tr>
    <th>Distribution</th>
    <th>Version</th>
    <th>HBase Version</th>
    <th>Installation</th>
    <th>Notes</th>
  </tr>
  <tr>
    <td><strong>Cloudera</strong></td>
    <td>CDH 5.2 or 5.3</td>
    <td>0.98.6</td>
    <td>Refer to <a href="http://www.cloudera.com/content/cloudera/en/documentation/core/latest/topics/installation.html" target="_blank">CDH 5.3.x Installation and Upgrade Guide</a> for installation instructions.</td>
    <td>
      <ul>
        <li>Select <strong>Cloudera Standard</strong> edition.</li>
        <li>Install using <strong>packages</strong>. Do not install using tarballs and parcels. Trafodion does not currently support tarball/parcel installation.</li>
        <li>CDH 5.2 is supported but has not been officially tested with Trafodion Release 1.3.</li>
      </ul>
    </td>
  </tr>
  <tr>
     <td><strong>Hortonworks Data Platform (HDP)</strong></td>
     <td>HDP 2.2</td>
     <td>0.98.4</td>
     <td>
       <ul>
         <li><strong>Install</strong>: <a href="http://docs.hortonworks.com/HDPDocuments/Ambari-1.7.0.0/Ambari_Install_v170/Ambari_Install_v170.pdf">Ambari 1.7.0 Install Guide</a></li>
         <li><strong>Upgrade:</strong> <a href="http://docs.hortonworks.com/HDPDocuments/Ambari-1.7.0.0/Ambari_Upgrade_v170/Ambari_Upgrade_v170.pdf">Ambari 1.7.0 Upgrade Guide</a></li>
       </ul>
     </td>
     <td></td>
   </tr>
</table>

# Validation
Once the Hadoop distribution has been installed and started, please ensure that the required services are running. At this point, you're ready to [Install Trafodion](install.html).