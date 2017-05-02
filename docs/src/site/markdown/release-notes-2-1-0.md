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

# 2.1.0-incubating Release Notes

This is the third release of the Apache Trafodion (incubating) project.

Build instructions are available in the [Trafodion Contributor Guide](https://cwiki.apache.org/confluence/display/TRAFODION/Create+Build+Environment).

## New Features

<span>
  <table>
    <tr>
      <th>Feature</th>
      <th>Jira ID</th>
    </tr>
    <tr>
      <td>Hashing functions added to Trafodion</td>
      <td>[TRAFODION-2229](https://issues.apache.org/jira/browse/TRAFODION-2229)</td>
    </tr>
    <tr>
      <td>Support for Hbase1.1 and CDH5.5-5.7</td>
      <td>[TRAFODION-2016](https://issues.apache.org/jira/browse/TRAFODION-2016)</td>
    </tr>
    <tr>
      <td>Better java exception handling in the java layer of Trafodion</td>
      <td>[TRAFODION-1988](https://issues.apache.org/jira/browse/TRAFODION-1988)</td>
    </tr>
    <tr>
      <td>TopN Sort operator</td>
      <td>[TRAFODION-2259](https://issues.apache.org/jira/browse/TRAFODION-2259)</td>
    </tr>
    <tr>
      <td>Runtime statistics enhancements</td>
      <td>[TRAFODION-2420](https://issues.apache.org/jira/browse/TRAFODION-2420)</td>
    </tr>
    <tr>
      <td>RegExp operator support</td>
      <td>[TRAFODION-2353](https://issues.apache.org/jira/browse/TRAFODION-2353)</td>
    </tr>
    <tr>
      <td>Support for LAG and LEAD OLAP functions</td>
      <td>[TRAFODION-2214](https://issues.apache.org/jira/browse/TRAFODION-2214)</td>
    </tr>
    <tr>
      <td>Bulk load enhancements to log errors</td>
      <td>[TRAFODION-2351](https://issues.apache.org/jira/browse/TRAFODION-2351)</td>
    </tr>
    <tr>
      <td>ODB tool support for Windows</td>
      <td>[TRAFODION-1931](https://issues.apache.org/jira/browse/TRAFODION-1931)</td>
    </tr>
    <tr>
      <td>Support for GROUP BY ROLLUP feature</td>
      <td>[TRAFODION-2246](https://issues.apache.org/jira/browse/TRAFODION-2246)</td>
    </tr>
    <tr>
      <td>Support ORDER BY clause in GROUP_CONCAT</td>
      <td>[TRAFODION-2270](https://issues.apache.org/jira/browse/TRAFODION-2270)</td>
    </tr>
    <tr>
      <td>Support for boolean datatype</td>
      <td>[TRAFODION-2099](https://issues.apache.org/jira/browse/TRAFODION-2099)</td>
    </tr>
    <tr>
      <td>Added encryption functions for Trafodion</td>
      <td>[TRAFODION-2228](https://issues.apache.org/jira/browse/TRAFODION-2228)</td>
    </tr>
    <tr>
      <td>Support of SQL extension 'EXCEPT'</td>
      <td>[TRAFODION-2117](https://issues.apache.org/jira/browse/TRAFODION-2117)</td>
    </tr>
    <tr>
      <td>Support for tinyint,largeint,unsinged datatype in SQL</td>
      <td>[TRAFODION-2086](https://issues.apache.org/jira/browse/TRAFODION-2086)</td>
    </tr>
    <tr>
      <td>SQL syntax to support INTERSECT</td>
      <td>[TRAFODION-2047](https://issues.apache.org/jira/browse/TRAFODION-2047)</td>
    </tr>
    <tr>
      <td>Enable support for various non-ansi sql syntax/functionality</td>
      <td>[TRAFODION-2180](https://issues.apache.org/jira/browse/TRAFODION-2180)</td>
    </tr>
    <tr>
      <td>Use of CQD to set scratch directory locations</td>
      <td>[TRAFODION2146](https://issues.apache.org/jira/browse/TRAFODION2146)</td>
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
      <td>Improve query compilation time</td>
      <td>[TRAFODION-2137](https://issues.apache.org/jira/browse/TRAFODION-2137)</td>
    </tr>
    <tr>
      <td>Improve upsert performance for table with indexes</td>
      <td>[TRAFODION-1546](https://issues.apache.org/jira/browse/TRAFODION-1546)</td>
    </tr>
    <tr>
      <td>Improve concurrency during DDL operations</td>
      <td>[TRAFODION-2037](https://issues.apache.org/jira/browse/TRAFODION-2037)</td>
    </tr>
  </table>
</span>

## Install and Configuration Changes

* Ambari Integration ([TRAFODION-2291](https://issues.apache.org/jira/browse/TRAFODION-2291))<br/>
Integration with Ambari cluster manager ambari interface/integration. The Ambari integration provides support for Hortonworks Hadoop distributions, while the command-line Trafodion Installer supports Cloudera and Hortonworks Hadoop distributions, and for select vanilla Hadoop installations.
* Python Installer ([TRAFODION-1839](https://issues.apache.org/jira/browse/TRAFODION-1839))<br/>
Trafodion Installer Evolution. The command-line installer has been replaced for the 2.1.0 release. Written in python, it replaces the legacy bash-script installer. The bash command-line installer is deprecated as of 2.1.0, but is still provided, just in case you experience any problems with the new installer. If so, please report those problems to the project team, since the legacy installer will soon be obsolete.
* Trafodion Configuration File ([TRAFODION-2306](https://issues.apache.org/jira/browse/TRAFODION-2306))<br/>
Introducing a configuration file traf-site.xml specific to Trafodion similar to hbase configuration file hbase-site.xml.  This configuration file extends the properties inherited from the standard hbase-site.xml.
* The environment variable MY_SQROOT has been renamed to TRAF_HOME.

## Fixes

This release contains fixes for 300+ JIRAs. Here is a [list of resolved JIRAs in Release 2.1](https://issues.apache.org/jira/issues/?jql=project%20%3D%20%22Apache%20Trafodion%22%20and%20fixVersion%20%3D%202.1-incubating%20order%20by%20updated%20desc).

## Documentation Updates

Several updates have been made to the SQL Reference Manual to reflect the new functions added in 2.1. Some functions have not been documented yet but have JIRAs outstading and will be completed by the next release. Provisioning Guide has been updated to reflect instructions for using hte new Ambari Installer or the python installation script. 

## Supported Platforms

<span>
  <table>
    <tr>
      <td>**Operating Systems**</td>
      <td>RedHat / CentOS 6.5 -- 6.8</td>
    </tr>
    <tr>
      <td>**Hadoop Distributions**</td>
      <td>Cloudera distributions CDH 5.4 -- 5.6<br/>
          Hortonworks distributions HDP 2.3 -- 2.4<br/>
          Apache Hadoop with Apache HBase 1.0 -- 1.1</td>
     </tr>
    <tr>
      <td>**Java Version**</td>
      <td>JDK 1.7.0_67 or newer</td>
    </tr>
  </table>
</span>
