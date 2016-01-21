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
This page describes how to remove Trafodion and/or a Hadoop distribution.

# Remove Trafodion
Use these instructions to remove Trafodion. 

<table><tr><td><p><strong>NOTE</strong></p><p>You do not need to use the <strong><code>trafodion_uninstaller</code></strong> script if upgrading Trafodion. Instead, use the <strong><code>trafodion_install</code></strong> script, which automatically upgrades the version of Trafodion. Please refer to <a href="install.html">Install</a> for further information.</p></td></tr></table>

Run the commands from the first node of the cluster. Do not run them from a machine that is not part of the Trafodion cluster.

## Step 1: Stop Trafodion
Do the following:

    su trafodion
    cd $MY_SQROOT/sql/scripts or cds
    sqstop

## Step 2: Run **```trafodion_uninstaller```**
**```trafodion_uninstaller```** accepts the following command options.

Option                        | Description 
------------------------------|------------------------------------------------------------------------
**```instance <dir-name>```** | \<dir-name\> represents the directory path of the Trafodion instance to remove. Run this option from the **```trafodion```** user ID.
**```--all```**                | Completely removes Trafodion, including the **```trafodion```** user ID and its /root directory. Run this option from a user ID with **```sudo```** access 

**Examples**

    cd /root/trafodion_downloads/installer
    # Uninstall Trafodion but leave user IDs and root directory in place
    ./trafodion_uninstaller --instance /home/trafodion/trafodion
    # Completely remove Trafodion
    ./trafodion_uninstaller --all

# Remove Hadoop Distribution
Use these instructions of you need to change the Hadoop distribution on your environment.

**Assumption:** You've already [removed Trafodion](#Remove_Trafodion), if previously installed.

## Remove Cloudera
**```cloudera_uninstall```** must be run by a user with **```root```** privileges. It accepts the following command options.

Option                          | Description 
--------------------------------|------------------------------------------------------------------------
**```--help```**                | Print help text and exit.
**```--nodes "<node-list>```**" | **```<node-list```** represents a space-separated list from which Cloudera should be uninstalled. Quotes are required.

**Example**

    cd /root/trafodion_downloads/installer/tools
    ./cloudera_uninstall --nodes "redhat-1 redhat-2" 

## Remove Hortonworks
**```hortonworks_uninstall```** must be run by a user with **```root```** privileges. It accepts the following command options.

Option                          | Description 
--------------------------------|------------------------------------------------------------------------
**```--help```**                | Print help text and exit.
**```--nodes "<node-list>```**" | **```<node-list```** represents a space-separated list from which Hortonworks should be uninstalled. Quotes are required.

**Example**

    cd /root/trafodion_downloads/installer/tools
    ./hortonworks_uninstall --nodes "redhat-1 redhat-2" 

<table><tr><td>
<p><strong>NOTE</strong></p>
<p>During removal, the Ambari Server database must be reset. Type <strong><code>yes</code></strong> when prompted.</p>
<p><strong>Example</strong></p>
<pre>Resetting ambari-server
**** WARNING **** You are about to reset and clear the Ambari Server database. This will remove all cluster host and configuration 
information from the database. You will be required to re-configure the Ambari server and re-run the cluster wizard.
Are you SURE you want to perform the reset [yes/no] (no)? yes
Confirm server reset [yes/no](no)? yes</pre>
</td></tr></table>

## Remove MapR
<table><tr><td>
<p><strong>NOTE</strong></p>
<p>Trafodion does not currently support MapR. However, the <strong><code>mapr_unistall</code></strong> allows you to remove an existing MapR installation.</p>
<p>Follow these instructions to remove MapR and install one of the supported Hadoop distributions before installing Trafodion.</p>
</td></tr></table>

**```mapr_uninstall```** must be run by a user with **```root```** privileges. It accepts the following command options.

Option                          | Description 
--------------------------------|------------------------------------------------------------------------
**```--help```**                | Print help text and exit.
**```--nodes "<node-list>```**" | **```<node-list```** represents a space-separated list from which MapR should be uninstalled. Quotes are required.


Do the following:

<table>
  <tr>
    <th width="15%">Step</th>
    <th width="30%">Description</th>
    <th width="55%">Example</th>
  </tr>
  <tr>
     <td><strong>Download Trafodion</strong></td>
     <td>If you haven't done so already, <a href="download.html">download</a> Trafodion.</td>
     <td></td>
  </tr>
  <tr>
     <td><strong>Create <code>trafodion_downloads</code> Directory</strong></td>
     <td>Place the downloaded <strong><code>trafodion-1.1.&lt;n&gt;.tar.gz</code></strong> and <strong><code>installer-1.1.&lt;n&gt;_v&lt;num&gt;.tar.gz</code></strong> tar files into the <strong><code>trafodion_downloads</code></strong> directory.</td>
     <td><pre>
mkdir $HOME/trafodion_downloads
mv &lt;your-download-path&gt;/trafodion-1.1.&lt;n&gt;.tar.gz $HOME/trafodion_downloads
mv &lt;your-download-path&gt;/installer-1.1.&lt;n&gt;_v&lt;num&gt;.tar.gz $HOME/trafodion_downloads
cd $HOME/trafodion_downloads</pre>
     </td>
  </tr>
  <tr>
     <td><strong>Unpack <code>tar</code> Files</strong></td>
     <td>Enables access to the Trafodion installation tools.</td>
     <td><pre>
tar -xzf trafodion-1.1.&lt;n&gt;.tar.gz
tar -xzf installer-1.1.&lt;n&gt;_v&lt;num&gt;.tar.gz</pre>
     </td>
  </tr>
  <tr>
     <td><strong>Run <code>mapr_uninstall</code></strong></td>
     <td>Remove the MapR distribution software.</td>
     <td><pre>
cd /root/trafodion_downloads/installer/tools
./mapr_uninstall --nodes "redhat-1 redhat-2"
./mapr_uninstall</pre>
     </td>
  </tr>
</table>


# Switch Hadoop Distribution
If needed, you can switch the Hadoop distribution that Trafodion is running on without reinstalling Trafodion.

Do the following.

## Step 1: Shut Down Trafodion

    su trafodion
    cd $MY_SQROOT/sql/scripts
    sqstop
    
## Step 2: Remove Hadoop Distribution
Remove the Hadoop Distribution per the instructions above.

Hadoop Distribution | Instructions
--------------------|--------------------------------------
**Cloudera**        | [Remove Cloudera](#Remove_Cloudera)
**Hortonworks**     | [Remove Hortonworks](#Remove_Hortonworks)
**MapR**            | [Remove MapR](#Remove_MapR)

## Step 3: Reinstall Hadoop Distribution
Refer to [Install Hadoop Distribution](install-preparation.html#Install_Hadoop_Distribution).

## Step 4: Run Trafodion Installer
Refer to the [Install](install.html).





