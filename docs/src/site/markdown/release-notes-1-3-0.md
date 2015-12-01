<!--
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
-->

# Release 1.3.0
This is the first release of the Apache Trafodion (incubating) project. In addition to including a number of new features and improvements across the project, the focus of this release is to comply with Apache release guidelines.

Build instructions are available [here](https://cwiki.apache.org/confluence/pages/viewpage.action?pageId=61316378).

## Supported Platforms
The following platforms are supported in this release.

<span>
  <table>
    <tr>
      <td>**Operating Systems**</td>
      <td>CentOS 6.5 -- 6.7</td>
    </tr>
    <tr>
      <td>**Hadoop Distributions**</td>
      <td>Cloudera distributions CDH 5.3.x<br />Hortonworks distribution HDP 2.2</td>
     </tr>
    <tr>
      <td>**Java Version**</td>
      <td>JDK 1.7.0_67 or newer</td>
    </tr>
    <tr>
      <td>**HBase Version**</td>
      <td>HBase 0.98.x</td>
    </tr>
  </table>
</span>

## New Features

This release contains the following new features.

<span>
  <table>
    <tr>
      <th>Category</th>
      <th>Feature</th>
      <th>Defect ID</th>
    </tr>
    <tr>
      <td>SQL</td>
      <td>Support for IDENTITY columns.</td>
      <td>(TRAFODION-62)</td>
    </tr>
    <tr>
      <td>SQL</td>
      <td>Support HBase region splitting and re-balancing with active transactions.</td>
      <td>(TRAFODION-34)</td>
    </tr>
    <tr>
      <td>Installer</td>
      <td>Support for Batch feature in Installer.</td>
      <td>(TRAFODION-1543)</td>
    </tr>
    <tr>
      <td>Installer</td>
      <td>Support for Cloudera parcels feature in Installer.</td>
      <td>(TRAFODION-1452)</td>
    </tr>
    <tr>
      <td>SQL</td>
      <td>Support for Multi-Temperate data – Part 1.</td>
      <td>(TRAFODION-49)</td>
    </tr>
    <tr>
      <td>UDF</td>
      <td>Compile time interface for Table Mapping UDFs (TMUDF).</td>
      <td>(TRAFODION-51)</td>
    </tr>
    <tr>
      <td>SQL</td>
      <td>Support for multiple column families in a Trafodion table.</td>
      <td>(TRAFODION-1419)</td>
    </tr>
    <tr>
      <td>SQL</td>
      <td>Support for MODIFY COLUMN attribute for ALTER TABLE.</td>
      <td>(TRAFODION-18)</td>
    </tr>
  </table>
</span>

## Improvements
This release contains the following improvements.

<span>
  <table>
    <tr>
      <th>Category</th>
      <th>Feature</th>
      <th>Defect ID</th>
    </tr>
    <tr>
      <td>SQL</td>
      <td>Enable HBase serialization feature.</td>
      <td>TRAFODION-1462</td>
    </tr>
    <tr>
      <td>SQL</td>
      <td>Enable EXPLAIN_IN_RMS feature by default.</td>
      <td>TRAFODION-1460</td>
    </tr>
    <tr>
      <td>Performance</td>
      <td>Reduce the path length for IUD operations in Trafodion.</td>
      <td>TRAFODION-1444</td>
    </tr>
    <tr>
      <td>SQL</td>
      <td>Enable Compressed Internal Format (CIF) for Trafodion scan operator.</td>
      <td>TRAFODION-1424</td>
    </tr>
    <tr>
      <td>Build</td>
      <td>Maven components to use locally built JDBC jar file.</td>
      <td>TRAFODION-1437</td>
    </tr>
    <tr>
      <td>Build</td>
      <td>Numerous build improvements.</td>
      <td></td>
    </tr>
  </table>
</span>

## Fixes

This release contains fixes for 114 bugs.

## Known Issues

### HBase Lease Timeout Patch

HBase uses a lease mechanism to protect against memory leaks in Region Servers caused by potential client instabilities that would open scanners, but die before having the opportunity to close cleanly and release resources. This mechanism relies on a server side timer, configured by the 'hbase.client.scanner.timeout.period' parameter in 'hbase-site.xml'. If a client fails to call 'next()' within the timeout period, the server will assume the client died, and will force close the server side scanner and release resources. However, in Trafodion, there are legitimate use cases where client is busy doing heavy processing, and needs more time than specified in the default scanner timeout value.  Increasing the 'hbase.client.scanner.timeout.period' value has the side effect of weakening the safety mechanism previously described. 

The HBase community agrees that the correct behavior of this safety feature should be to have the client *reset* the scanner and resume where it left off instead of giving up and throwing an exception. The change will be implemented in a future release of HBase. In the meantime, this release includes a mechanism to invoke the correct behavior via a custom setting. You can enable the behavior by adding this parameter in 'hbase-site.xml'.

    <property> 
      <name>hbase.trafodion.patchclientscanner.enabled</name>
      <value>true</value> 
      <description>
        Enable a Trafodion feature to allow a client to reset the HBase scanner and resume where it left off instead of throwing an exception upon expiry of the HBase hbase.client.scanner.timeout.period timer.
      </description>
    </property>
    
The default value of the parameter is **false**.
