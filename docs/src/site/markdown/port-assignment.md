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
The following table lists the default ports used by the different Trafodion components plus the configuration file and configuration attribute associated with each port setting.

<table>
  <tr>
     <th>Default Port</th>
     <th>Configuration File</th>
     <th>Configuration Entry</th>
     <th>Required</th>
     <th>Range</th>
     <th>Protocol</th>
     <th>Comment</th>
  </tr>
  <tr>
    <td><strong>4200</strong></td>
    <td><code>rest-site.xml</code></td>
    <td><code>trafodion.rest.port</code></td>
    <td>Yes</td>
    <td>1</td>
    <td>REST</td>
    <td>Trafodion REST Server.</td>
  </tr>
  <tr>
    <td><strong>4201</strong></td>
    <td><code>rest-site.xml</code></td>
    <td><code>trafodion.rest.https.port</code></td>
    <td>Yes</td>
    <td>1</td>
    <td>HTTPS</td>
    <td>Trafodion REST Server (HTTPS).</td>
  </tr>
  <tr>
    <td><strong>23400</strong></td>
    <td><code>dcs-site.xml</code></td>
    <td><code>dcs.master.port</code></td>
    <td>Yes</td>
    <td>n</td>
    <td>binary</td>
    <td>Start of Trafodion DCS port range. (37800 for Trafodion 1.1)</td>
  </tr>
  <tr>
    <td><strong>24400</strong></td>
    <td><code>dcs-site.xml</code></td>
    <td><code>dcs.master.info.port</code></td>
    <td>Yes</td>
    <td>1</td>
    <td>HTTP</td>
    <td>DCS master web GUI. (40010 for Trafodion 1.1)</td>
  </tr>
  <tr>
    <td><strong>24410</strong></td>
    <td><code>dcs-site.xml</code></td>
    <td><code>dcs.server.info.port</code></td>
    <td>Yes</td>
    <td>n</td>
    <td>HTTP</td>
    <td>Start of range for DCS server web GUIs. (40020 for Trafodion 1.1)</td>
  </tr>
  <tr>
    <td><strong>50030</strong></td>
    <td><code>mapred-site.xml</code></td>
    <td><code>mapred.job.tracker.http.address</code></td>
    <td>No</td>
    <td>1</td>
    <td>HTTP</td>
    <td>MapReduce Job Tracker web GUI.</td>
  </tr>
  <tr>
    <td><strong>50070</strong></td>
    <td><code>hdfs-site.xml</code></td>
    <td><code>dfs.http.address</code></td>
    <td>No</td>
    <td>1</td>
    <td>HTTP</td>.
    <td>HDFS Name Node web GUI.</td>
  </tr>
  <tr>
    <td><strong>50075</strong></td>
    <td><code>hdfs-site.xml</code></td>
    <td><code>dfs.datanode.http.address</code></td>
    <td>No</td>
    <td>1</td>
    <td>HTTP</td>.
    <td>HDFS Data Node web GUI.</td>
  </tr>
  <tr>
    <td><strong>50090</strong></td>
    <td><code>hdfs-site.xml</code></td>
    <td><code>dfs.secondary.http.address</code></td>
    <td>No</td>
    <td>1</td>
    <td>HTTP</td>.
    <td>HDFS Secondary Name Node web GUI.</td>
  </tr>
  <tr>
    <td><strong>60010</strong></td>
    <td><code>hbase-site.xml</code></td>
    <td><code>hbase.master.info.port</code></td>
    <td>No</td>
    <td>1</td>
    <td>HTTP</td>.
    <td>HBase Master web GUI.</td>
  </tr>
  <tr>
    <td><strong>60030</strong></td>
    <td><code>hbase-site.xml</code></td>
    <td><code>hbase.regionserver.info.port</code></td>
    <td>No</td>
    <td>1</td>
    <td>HTTP</td>.
    <td>HBase Region Server web GUI.</td>
  </tr>
</table>

<div class="alert alert-dismissible alert-info">
  <button type="button" class="close" data-dismiss="alert">&close;</button>
  <p style="color:black">There are two port ranges used by Trafodion. <ul><li style="color:black">23400 is a range, to allow multiple mxosrvr processes on each node. Allow a range of a few ports, enough to cover all the servers per node that are listed in the "servers" file in the DCS configuration directory.</li> <li style="color:black">24410 is a range as well, enough to cover the DCS servers per node, usually 1 or 2.</li></ul></p>
<p style="color:black">If you use Trafodion 1.1, then the older port numbers are 37800 and 40010.</p>
<p style="color:black">On top of that you would need the ports required by your Hadoop distribution. Most of this information comes from a script to sand-box a development install, including the major ports used. The script's name is <strong><code>trafodion/core/sqf/sql/scripts/install_local_hadoop</code></strong>.</p>
  <p style="color:black">Although not all the ports will be used on every node of the cluster, you would need to open most of them for all the nodes in the cluster that have Trafodion, HBase or HDFS servers on them.</p>
</div>