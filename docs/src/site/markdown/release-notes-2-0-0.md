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

# 2.0.0-incubating Release Notes

This is the second release of the Apache Trafodion (incubating) project. It builds on the initial R1.3 release, with many new features and improvements, and will offer a binary package for the first time.

Build instructions are available [here](build.html).

## New Features

<span>
  <table>
    <tr>
      <th>Feature</th>
      <th>Jira ID</th>
    </tr>
    <tr>
      <td>Support for Hadoop distributions CDH 5.4, HDP 2.3, as well as Apache HBase 1.0</td>
      <td>(TRAFODION-1706)</td>
    </tr>
    <tr>
      <td>Ability to online add / remove nodes from a Trafodion instance</td>
      <td>(TRAFODION-1885)</td>
    </tr>
    <tr>
      <td>Transaction protection for DDL operations </td>
      <td>(TRAFODION-1798)</td>
    </tr>
    <tr>
      <td>Native support for Large OBjects (BLOB and CLOB data types) (PREVIEW MODE)</td>
      <td></td>
    </tr>
    <tr>
      <td>Support to ALTER all attributes of a column</td>
      <td>(TRAFODION-1844)</td>
    </tr>
    <tr>
      <td>Support for GBK charset in SQL TRANSLATE function</td>
      <td>(TRAFODION-1720)</td>
    </tr>
  </table>
</span>

## Improvements

<span>
  <table>
    <tr>
      <th>Feature</th>
      <th>Jira ID</th>
    </tr>
    <tr>
      <td>Advanced predicate pushdown</td>
      <td>TRAFODION-1662</td>
    </tr>
    <tr>
      <td>Improved co-ordination between transactions and HBase region splits / rebalance</td>
      <td>TRAFODION-1648</td>
    </tr>
    <tr>
      <td>Optimize MDAM scans with small scanner</td>
      <td>TRAFODION-1900</td>
    </tr>
    <tr>
      <td>Integrate library management into Trafodion metadata</td>
      <td>TRAFODION-1879</td>
    </tr>
    <tr>
      <td>Improved support for UPDATE STATISTICS for large tables (up to 1B rows)</td>
      <td></td>
    </tr>
    <tr>
      <td>Numerous build improvements</td>
      <td></td>
    </tr>
  </table>
</span>

## Fixes

This release contains 250+ fixes.

## Migration Considerations

1. This release does not include convenience binaries for the client package due to a licensing issue. Convenience binaries will be included in a follow-on patch release (R2.0.1) that should be available shortly. During this time, existing documentation (Trafodion Client Installation Guide and Trafodion Provisioning Guide), which refer to binaries, will be briefly out of sync. 

2. New SQL procedures have been added to manage user libraries. If you have an existing Trafodion installation, execute the following as user trafodion.

        $ sqlci
        sql> initialize trafodion, create library management;
        sql> exit;

    This statement creates a new schema called \_LIBMGR\_ and creates the library management procedures in this schema. If security is enabled, a new system role called DM\_\_LIBMGRROLE is added. 


## Notes

### ESP Idle timeout [TRAFODION-1823]

A new CQD ESP_IDLE_TIMEOUT has been introduced in this release. This setting governs the life of an idle ESP (defined as an ESP that is not executing any work). On expiry of the timer, the ESP will kill itself.

    sql> CQD ESP_IDLE_TIMEOUT ‘<seconds>’;

The default timer value is 1800 seconds. 

### HBase Lease Timeout Patch [Same as Release 1.3]

HBase uses a lease mechanism to protect against memory leaks in Region Servers caused by potential client instabilities that would open scanners, but die before having the opportunity to close cleanly and release resources. This mechanism relies on a server side timer, configured by the 'hbase.client.scanner.timeout.period' parameter in 'hbase-site.xml'. If a client fails to call 'next()' within the timeout period, the server will assume the client died, and will force close the server side scanner and release resources. However, in Trafodion, there are legitimate use cases where client is busy doing heavy processing, and needs more time than specified in the default scanner timeout value.  Increasing the 'hbase.client.scanner.timeout.period' value has the side effect of weakening the safety mechanism previously described. 

The HBase community agrees that the correct behavior of this safety feature should be to have the client *reset* the scanner and resume where it left off instead of giving up and throwing an exception. The change will be implemented in a future release of HBase. In the meantime, this release includes a mechanism to invoke the correct behavior via a custom setting. You can enable the behavior by adding this parameter in 'hbase-site.xml'.

    <property> 
      <name>hbase.trafodion.patchclientscanner.enabled</name>
      <value>true</value> 
      <description>
        Enable a Trafodion feature to allow a client to reset the HBase scanner and resume where it left off instead of throwing an exception upon expiration of the HBase hbase.client.scanner.timeout.period timer.
      </description>
    </property>

The default value of the parameter is **false**.

## Supported Platforms

<span>
  <table>
    <tr>
      <td>**Operating Systems**</td>
      <td>RedHat / CentOS 6.5 -- 6.7</td>
    </tr>
    <tr>
      <td>**Hadoop Distributions**</td>
      <td>Cloudera distributions CDH 5.4<br />Hortonworks distribution HDP 2.3<br />Apache Hadoop with Apache HBase 1.0</td>
     </tr>
    <tr>
      <td>**Java Version**</td>
      <td>JDK 1.7.0_67 or newer</td>
    </tr>
  </table>
</span>

