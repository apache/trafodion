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
This page describes how to install the Trafodion product environment. [Create Development Environment](create-dev-environment.html) describes how to create the Trafodion Development Test Environment.

The Trafodion product environment is installed using the Trafodion Installer, which operates on Trafodion binaries only. Refer to the [Download](download.html) page for instructions about how you download/create the Trafodion binaries.

# Preparation
The Trafodion Installer assumes that you've performed the following steps before a Trafodion install:

1. **Cluster Requirements**: Ensure that your cluster meets the Trafodion requirements.
2. **Security Requirements**: Create the user ID that is used to run the Trafodion Installer. Review and configure LDAP, if desired.
3. **PC Tools**: Install software used when installing Trafodion on your PC.
4. **Install Hadoop**: Install Hadoop software supported by Trafodion.
5. **Validate Hadoop**: Validate that required Hadoop services are running.

Please refer to [Install Preparation](install-preparation.html) for information about how to perform the steps above.

# Security Considerations 
You use the following user IDs and passwords during the Trafodion installation.

Logon                              | User ID                    | Password                    | Description                                         
-----------------------------------|----------------------------|-----------------------------|--------------------------------------------------------
Cloudera Manager Web GUI logon | **```admin (default)```**  | **```admin (default)```**   | Cloudera only. After installing Cloudera, you will be instructed to log on to the Cloudera Manager Web GUI. Use the default user ID and password (**```admin```**, **```admin```**). If you already had Cloudera installed, please use your previously defined user ID and password.
Ambari Web GUI logon           | **```admin (default)```**  | **```admin (default)```**   | Hortonworks only. After downloading and setting up the Ambari repository, you will be instructed to log on to the Ambari Web GUI. Use the default user ID and password (**```admin```**, **```admin```**). If you already had Ambari installed, please use your previously defined user ID and password.
User ID with sudo Access       | **```<sudo-username>```**  | **```<password>```**        | In the installation steps, you may be instructed to use **```sudo```** or **```sudo userid```** access. You will be using your user ID, which has been enabled with **```sudo```** access and [passwordless ssh](passwordless-ssh.html) to all nodes of the cluster.
Trafodion Logon                | **```trafodion```**        | **```traf123 (default)```** | The Trafodion installer **automatically creates** this user ID when Trafodion is installed. Do not create this user ID manually.
VNC Server Logon               | **```trafodion```**        | **```traf123 (default)```** | After installing Trafodion, you will be instructed to log on to the VNC server at port 1. Use the **```trafodion```** user ID and **```password```**.

# Install Procedure
## Upgrade
Follow these steps if you have an older version of Trafodion that you want to upgrade:
 
1. *Shutdown Trafodion*: Please refer to [Manage Trafodion](management.html).
2. *Backup Data*: Backup metadata and user objects. Please refer to [Backup/Restore](backuprestore.html).
3. *Install Trafodion*: The **```trafodion_install```** script automatically detects the previous version of Trafodion and performs an upgrade operation, including initializing Trafodion, upgrading the metadata, and restarting all processes. Please refer to [Installation](#Installation) below for more information about **```trafodion_install```**.

    If something goes wrong during the installation, restore your data from the backed up objects and repeat the previous step.

<table><tr><td><strong>NOTE</strong>
  <p>If you are doing an upgrade outside of the installation environment, then you will need to restart your services and processes, such as sqlci, the DCS server, and the qms servers. If you do not restart those services and processes, your system will not work properly.</p>
</td></tr></table>

## Installation
At a high level, the following instructions consist of a single step:

Command                     | User ID                   | Comments
----------------------------|---------------------------|---------------------------------------------------------------------------------------------------------
**```trafodion_install```** | **```<sudo-username>```** | Installs necessary RPMs, creates the **```trafodion```** user ID, sets up passwordless ssh for the **```trafodion```** user ID, copies the Trafodion distribution files across the cluster, generates startup files, and starts Trafodion, including Database Connectivity Services (DCS) (the ODBC/JDBC server), among other things.


<table><tr><td>
  <strong>IMPORTANT</strong>
  <p>The Trafodion commands should all be done from the first node of the cluster. Do not run them from a machine that is not part of the Trafodion cluster. Trafodion must be installed on all nodes that host an HBase RegionServer (that is, where a supported Hadoop distribution is installed).</p>
  <p>For example, if Cloudera or Hortonworks is installed on nodes <strong><code>n001</code></strong>, <strong><code>n002</code></strong>, and <strong><code>n003</code></strong>, then you must install Trafodion on those same nodes (<strong><code>--nodes "n001 n002 n003"</code></strong>).</p>
</td></tr></table>

<table>
  <tr>
    <th width="5%">Step</th>
    <th width="35%">Description</th>
    <th width="60%">Commands</th>
  </tr>
  <tr>
     <td><strong>Download</strong></td>
     <td>If you haven't done so already, then please download the Trafodion software.</td>
     <td>Please refer to <a href="download.html">Download</a>.</td>
  </tr>
  <tr>
     <td><strong>Upgrade</strong></td>
     <td>
       <p>If you are upgrading to a newer version of Trafodion, copy the installation files for the previous version to a directory outside of <strong><code>$HOME/trafodion_downloads</code></strong>.</p>
       <p>For example:</p>
       <pre>mkdir $HOME/trafodion_downloads_v1.0</pre>
     </td>
     <td><pre>cp –r $HOME/trafodion_downloads/* $HOME/trafodion_downloads_v1.0
rm –rf $HOME/trafodion_downloads/*</pre> 
     </td>
  </tr>
  <tr>
     <td><strong>Create <code>trafodion_downloads</code></strong></td>
     <td>Create the <strong><code>trafodion_downloads</code></strong> directory if you have not done so already and place the downloaded <strong><code>trafodion-1.1.&lt;n&gt;.tar.gz</code></strong> and <strong><code>installer-1.1.&lt;n&gt;_v&lt;num&gt;.tar.gz</code></strong> tar files into it.</td>
     <td>
       <pre>
mkdir $HOME/trafodion_downloads
mv &lt;your-download-path&gt;/trafodion-1.1.&lt;n&gt;.tar.gz $HOME/trafodion_downloads
mv &lt;your-download-path&gt;/installer-1.1.&lt;n&gt;_v&lt;num&gt;.tar.gz $HOME/trafodion_downloads
cd $HOME/trafodion_downloads
       </pre>
     </td>
  </tr>
  <tr>
     <td><strong>Unpack tar File</strong></td>
     <td>Untar the downloaded installer file.</td>
     <td><pre>tar -xzf installer-1.1.&lt;n&gt;_v&lt;num&gt;.tar.gz</pre></td>
  </tr>
  <tr>
     <td><strong>Run <code>trafodion_install</code></strong></td>
     <td>
        <p>The scripts is run without parameters. Instead, you'll be prompted for information.</p>
        <p><strong>NOTE</strong></p>
        <p>The <strong><code>trafodion_install</code></strong> script automatically checks the cluster for attributes that commonly cause installation issues, runtime performance problems, and so on. Also, various system information is collected for future reference, such as to aid with troubleshooting. The installation will abort if any configuration errors are discovered. You will need to correct such errors before re-running the <strong><code>trafodion_install</code></strong> script.</p>
     </td>
     <td>
       <pre>
cd installer
./trafodion_install
       </pre>
       <p>Type "accept" to accept the license agreement when prompted.</p>
       <p>Provide the requested information according to the prompts.</p>
       <p><strong>NOTE</strong></p>
       <p>If you choose not to start Trafodion after the installation (that is, if you enter <strong><code>N</code></strong> for <strong><code>Start Trafodion after install (Y/N)</code></strong>), you will need to manually start and initialize Trafodion after <strong><code>trafodion_install</code></strong> completes.</p> 
     </td>
  </tr>
  <tr>
     <td><strong><code>trafodion_install</code> Completion</strong></td>
     <td>Wait for <strong><code>trafodion_install</code></strong> to complete. </td>
     <td><p>You will see the following message:</p><pre>***INFO: Installation completed successfully.</pre></td>
  </tr>
  <tr>
     <td><strong>Connect</strong></td>
     <td>Once <strong><code>trafodion_install</code></strong> completes without error, your Trafodion system should be up and running.</td>
     <td>
        <p>Start a new VNC session using port :1.</p>
        <p>Log in as the <strong><code>trafodion</code></strong> user with the password for the VNC, which is <strong><code>traf123</code></strong> by default unless you changed it.</p>
     </td>
  </tr>
</table>

## Start and Initialize
If you chose not to start Trafodion after the installation, start and initialize Trafodion as follows:

    cds
    sqstart
    
    [trafodion@n001 ~]$ sqlci
    Trafodion Conversational Interface 1.1.0
    (c) Copyright 2015 Apache Software Foundation
    >>initialize trafodion ;
    .
    .
    .

## Validate Installation
Perform a quick sanity check using the Trafodion Conversational Interface (**```sqlci```**). Create a table with a couple of records. 

**Example**

    [trafodion@n001 ~]$ sqlci
    Trafodion Conversational Interface 1.1.0
    (c) Copyright 2015 Apache Software Foundation
    >> create table test1 (f1 int, f2 int);
    
    --- SQL operation complete.
    >> insert into test1 values(1,1);
    
    --- 1 row(s) inserted.
    >> insert into test1 values(2,2);
    
    --- 1 row(s) inserted.
    >> select * from test1;
    
    F1            F2
    -----------   -----------
              1             1
              2             2
    
    --- 2 row(s) selected.
    >> get tables;
    
    Tables in Schema TRAFODION.SEABASE
    ==================================
    
    TEST1
    
    --- SQL operation complete.
    >> drop table test1;
    >> exit;

The SQL commands above should run successfully.

## Install Client Software
Download and install the Trafodion JDBC and/or ODBC drivers on your client workstation to be able to connect to Trafodion from a client application. Please refer to the [Trafodion Client Installation Guide](docs/Trafodion_Client_Installation_Guide.html), which describes how to install the JDBC and ODBC drivers, how to connect to Trafodion, and how to run sample programs to test the connection.

# Troubleshooting
Please refer to [Troubleshoot Trafodion Installation](install-troubleshoot.html).

# Uninstall
Please refer to [Unistall Trafodion](uninstall.html).

