<!--
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
-->

# 2.3.0-Release Notes

This is the second release of the Apache Trafodion project.

Build instructions are available in the [Trafodion Contributor Guide](https://cwiki.apache.org/confluence/display/TRAFODION/Create+Build+Environment).

## New Features

<span>
  <table>
    <tr>
      <th>Feature</th>
      <th>Jira ID</th>
    </tr>
    <tr>
      <td>support adding/retrieving comments to SQL objects</td>
      <td>[TRAFODION-1915](https://issues.apache.org/jira/browse/TRAFODION-1915)</td>
    </tr>
    <tr>
      <td>support functions in the column default definition</td>
      <td>[TRAFODION-2335](https://issues.apache.org/jira/browse/TRAFODION-2335)</td>
    </tr>
    <tr>
      <td>support to load json file</td>
      <td>[TRAFODION-2335](https://issues.apache.org/jira/browse/TRAFODION-2335)</td>
    </tr>
    <tr>
      <td>Add new Trafodion LOB Guide to documentation build and web site</td>
      <td>[TRAFODION-2771](https://issues.apache.org/jira/browse/TRAFODION-2771)</td>
    </tr>
    <tr>
      <td>Script to help resolve merge conflicts particularly in regression test results</td>
      <td>[TRAFODION-2835](https://issues.apache.org/jira/browse/TRAFODION-2835)</td>
    </tr>
    <tr>
      <td>Refactor Trafodion implementation of hdfs scan for text formatted hive tables</td>
      <td>[TRAFODION-2917](https://issues.apache.org/jira/browse/TRAFODION-2917)</td>
    </tr>
    <tr>
      <td>add create option storage policy</td>
      <td>[TRAFODION-3026](https://issues.apache.org/jira/browse/TRAFODION-3026)</td>
    </tr>
    <tr>
      <td>Trafodion to support compressed Hive Text formatted tables</td>
      <td>[TRAFODION-3065](https://issues.apache.org/jira/browse/TRAFODION-3065)</td>
    </tr>
    <tr>
      <td>cp command does not support writting to bad file</td>
      <td>[TRAFODION-3252](https://issues.apache.org/jira/browse/TRAFODION-3252)</td>
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
      <td>LP Bug: 1444549 - DCS floating IP should set internal interface</td>
      <td>[TRAFODION-1176](https://issues.apache.org/jira/browse/TRAFODION-1176)</td>
    </tr>
    <tr>
      <td>Implement T2 Driver&#39;s Rowsets ability to enhance the batch insert performance</td>
      <td>[TRAFODION-1628](https://issues.apache.org/jira/browse/TRAFODION-1628)</td>
    </tr>
    <tr>
      <td>&#39;initialized trafodion&#39; failed on CentOS 7.2</td>
      <td>[TRAFODION-1869](https://issues.apache.org/jira/browse/TRAFODION-1869)</td>
    </tr>
    <tr>
      <td>Have jdbc drivers available on Maven Repository</td>
      <td>[TRAFODION-1957](https://issues.apache.org/jira/browse/TRAFODION-1957)</td>
    </tr>
    <tr>
      <td>add encryption functions for Trafodion</td>
      <td>[TRAFODION-2228](https://issues.apache.org/jira/browse/TRAFODION-2228)</td>
    </tr>
    <tr>
      <td>Documentation update for REST and DCS</td>
      <td>[TRAFODION-2307](https://issues.apache.org/jira/browse/TRAFODION-2307)</td>
    </tr>
    <tr>
      <td>Memory leak in the compiler part of the code in Trafodion</td>
      <td>[TRAFODION-2727](https://issues.apache.org/jira/browse/TRAFODION-2727)</td>
    </tr>
    <tr>
      <td>Provide an improved memory quota assignment for big memory operators (BMO)</td>
      <td>[TRAFODION-2733](https://issues.apache.org/jira/browse/TRAFODION-2733)</td>
    </tr>
    <tr>
      <td>Native hbase table access via Trafodion improvements/issues</td>
      <td>[TRAFODION-2737](https://issues.apache.org/jira/browse/TRAFODION-2737)</td>
    </tr>
    <tr>
      <td>LOB support in JDBC Type 2</td>
      <td>[TRAFODION-2741](https://issues.apache.org/jira/browse/TRAFODION-2741)</td>
    </tr>
    <tr>
      <td>UPDATE STATS unnecessarily creates LOB columns in sample tables</td>
      <td>[TRAFODION-2781](https://issues.apache.org/jira/browse/TRAFODION-2781)</td>
    </tr>
    <tr>
      <td>Trafodion core code base needs to be thread safe</td>
      <td>[TRAFODION-2821](https://issues.apache.org/jira/browse/TRAFODION-2821)</td>
    </tr>
    <tr>
      <td>export will scan full table with splitby column</td>
      <td>[TRAFODION-2830](https://issues.apache.org/jira/browse/TRAFODION-2830)</td>
    </tr>
    <tr>
      <td>Enable Trafodion SQL processes to configure garbage collector in its embedded JVM</td>
      <td>[TRAFODION-2836](https://issues.apache.org/jira/browse/TRAFODION-2836)</td>
    </tr>
    <tr>
      <td>Remove incubating from trafodion web pages</td>
      <td>[TRAFODION-2857](https://issues.apache.org/jira/browse/TRAFODION-2857)</td>
    </tr>
    <tr>
      <td>Foundation infrastructure changes needed to support operating in Cloudera Manager environment</td>
      <td>[TRAFODION-2882](https://issues.apache.org/jira/browse/TRAFODION-2882)</td>
    </tr>
    <tr>
      <td>Preliminary Trafodion Foundation Scalability Enhancements</td>
      <td>[TRAFODION-2883](https://issues.apache.org/jira/browse/TRAFODION-2883)</td>
    </tr>
    <tr>
      <td>Trafodion Foundation Scalability Enhancements</td>
      <td>[TRAFODION-2884](https://issues.apache.org/jira/browse/TRAFODION-2884)</td>
    </tr>
    <tr>
      <td>Streamline setjmp/longjmp concepts in Trafodion</td>
      <td>[TRAFODION-2888](https://issues.apache.org/jira/browse/TRAFODION-2888)</td>
    </tr>
    <tr>
      <td>optionally add leading space when get a number column to display</td>
      <td>[TRAFODION-2904](https://issues.apache.org/jira/browse/TRAFODION-2904)</td>
    </tr>
    <tr>
      <td>Link to JIRA should use https:</td>
      <td>[TRAFODION-2926](https://issues.apache.org/jira/browse/TRAFODION-2926)</td>
    </tr>
    <tr>
      <td>Add news articles about Trafodion to Trafodion web site</td>
      <td>[TRAFODION-2928](https://issues.apache.org/jira/browse/TRAFODION-2928)</td>
    </tr>
    <tr>
      <td>Remove the 64K hard limit for process ids in RMS infrastructure</td>
      <td>[TRAFODION-2963](https://issues.apache.org/jira/browse/TRAFODION-2963)</td>
    </tr>
    <tr>
      <td>Secure *.apache.org links</td>
      <td>[TRAFODION-2994](https://issues.apache.org/jira/browse/TRAFODION-2994)</td>
    </tr>
    <tr>
      <td>Several scripts incorrectly use cut -b 1-5 to extract pid information</td>
      <td>[TRAFODION-3013](https://issues.apache.org/jira/browse/TRAFODION-3013)</td>
    </tr>
    <tr>
      <td>Add more enum files to analyzeMessageGuide.py</td>
      <td>[TRAFODION-3023](https://issues.apache.org/jira/browse/TRAFODION-3023)</td>
    </tr>
    <tr>
      <td>Query with nested subqueries chooses bad plan</td>
      <td>[TRAFODION-3031](https://issues.apache.org/jira/browse/TRAFODION-3031)</td>
    </tr>
    <tr>
      <td>removee &#39;mt_&#39; column family</td>
      <td>[TRAFODION-3059](https://issues.apache.org/jira/browse/TRAFODION-3059)</td>
    </tr>
    <tr>
      <td>Installer needs to support RH7 / Centos7</td>
      <td>[TRAFODION-3075](https://issues.apache.org/jira/browse/TRAFODION-3075)</td>
    </tr>
    <tr>
      <td>enhance the &#39;get table&#39; utility to return number of rows</td>
      <td>[TRAFODION-3101](https://issues.apache.org/jira/browse/TRAFODION-3101)</td>
    </tr>
    <tr>
      <td>Remove some obsolete cruft from the SQL parser and related modules</td>
      <td>[TRAFODION-3135](https://issues.apache.org/jira/browse/TRAFODION-3135)</td>
    </tr>
    <tr>
      <td>Remove obsolete code from UPDATE STATISTICS</td>
      <td>[TRAFODION-3138](https://issues.apache.org/jira/browse/TRAFODION-3138)</td>
    </tr>
    <tr>
      <td>Make &#39;hbase_options&#39; as default setting and use cqd to change the default setting</td>
      <td>[TRAFODION-3145](https://issues.apache.org/jira/browse/TRAFODION-3145)</td>
    </tr>
    <tr>
      <td>add sys_guid function</td>
      <td>[TRAFODION-3147](https://issues.apache.org/jira/browse/TRAFODION-3147)</td>
    </tr>
    <tr>
      <td>Docker files should show correct version</td>
      <td>[TRAFODION-3200](https://issues.apache.org/jira/browse/TRAFODION-3200)</td>
    </tr>
  </table>
</span>

## Sub-task

<span>
  <table>
    <tr>
      <th>Feature</th>
      <th>Jira ID</th>
    </tr>
    <tr>
      <td>Preliminary Trafodion Foundation Scalability Enhancements</td>
      <td>[TRAFODION-2883](https://issues.apache.org/jira/browse/TRAFODION-2883)</td>
    </tr>
    <tr>
      <td>Trafodion Foundation Scalability Enhancements</td>
      <td>[TRAFODION-2884](https://issues.apache.org/jira/browse/TRAFODION-2884)</td>
    </tr>
  </table>
</span>
        
## Fixes

<span>
  <table>
    <tr>
      <th>Feature</th>
      <th>Jira ID</th>
    </tr>
    <tr>
      <td>ODB doesn&#39;t terminate connections with DB when execution is interrupted</td>
      <td>[TRAFODION-1117](https://issues.apache.org/jira/browse/TRAFODION-1117)</td>
    </tr>
    <tr>
      <td>volatile table is not dropped after hpdci session ends</td>
      <td>[TRAFODION-138](https://issues.apache.org/jira/browse/TRAFODION-138)</td>
    </tr>
    <tr>
      <td>DCS - SQLProcedures and SQLProcedureColumns need to be supported</td>
      <td>[TRAFODION-206](https://issues.apache.org/jira/browse/TRAFODION-206)</td>
    </tr>
    <tr>
      <td>SPJ w result set failed with ERROR[11220], SQLCODE of -29261, SQLSTATE of HY000</td>
      <td>[TRAFODION-533](https://issues.apache.org/jira/browse/TRAFODION-533)</td>
    </tr>
    <tr>
      <td>select (insert) with prepared stmt fails with rowset</td>
      <td>[TRAFODION-598](https://issues.apache.org/jira/browse/TRAFODION-598)</td>
    </tr>
    <tr>
      <td>ODBC SQLRowCount function returns wrong row number</td>
      <td>[TRAFODION-735](https://issues.apache.org/jira/browse/TRAFODION-735)</td>
    </tr>
    <tr>
      <td>ODBC: Several values returned by SQLColumns are incorrect</td>
      <td>[TRAFODION-994](https://issues.apache.org/jira/browse/TRAFODION-994)</td>
    </tr>
    <tr>
      <td>UDF: Lack of checking for scalar UDF input/output values</td>
      <td>[TRAFODION-1145](https://issues.apache.org/jira/browse/TRAFODION-1145)</td>
    </tr>
    <tr>
      <td>Drop schema cascade returns error 1069</td>
      <td>[TRAFODION-1212](https://issues.apache.org/jira/browse/TRAFODION-1212)</td>
    </tr>
    <tr>
      <td>Hybrid Query Cache: query with equals predicate on INTERVAL datatype should not have a non-parameterized literal.</td>
      <td>[TRAFODION-1221](https://issues.apache.org/jira/browse/TRAFODION-1221)</td>
    </tr>
    <tr>
      <td>SPJ with resultset doesnt work thru T2 driver</td>
      <td>[TRAFODION-1258](https://issues.apache.org/jira/browse/TRAFODION-1258)</td>
    </tr>
    <tr>
      <td>Linux ODBC Driver is not able to create certificate file with long name length (over 30 bytes).</td>
      <td>[TRAFODION-1442](https://issues.apache.org/jira/browse/TRAFODION-1442)</td>
    </tr>
    <tr>
      <td>executor/TEST106 hangs at drop table at times</td>
      <td>[TRAFODION-1923](https://issues.apache.org/jira/browse/TRAFODION-1923)</td>
    </tr>
    <tr>
      <td>Instance will be down when the zookeeper on name node has been down</td>
      <td>[TRAFODION-2664](https://issues.apache.org/jira/browse/TRAFODION-2664)</td>
    </tr>
    <tr>
      <td>DCS server can&#39;t be restarted after switch to backup-master</td>
      <td>[TRAFODION-2877](https://issues.apache.org/jira/browse/TRAFODION-2877)</td>
    </tr>
    <tr>
      <td>DCS-ODBC-Getting &#39;Invalid server handle&#39; after bound hstmt is used for a while.</td>
      <td>[TRAFODION-157](https://issues.apache.org/jira/browse/TRAFODION-157)</td>
    </tr>
    <tr>
      <td>windows ODBC driver internal hp keyword cleanup</td>
      <td>[TRAFODION-427](https://issues.apache.org/jira/browse/TRAFODION-427)</td>
    </tr>
    <tr>
      <td>Status array returned by batch operations contains wrong return value for T2</td>
      <td>[TRAFODION-480](https://issues.apache.org/jira/browse/TRAFODION-480)</td>
    </tr>
    <tr>
      <td>[Trafci] An internal error occurred in the code generator in file ../generator/GenRelDCL.cpp at line 314</td>
      <td>[TRAFODION-510](https://issues.apache.org/jira/browse/TRAFODION-510)</td>
    </tr>
    <tr>
      <td>In full explain output, begin/end key for char/varchar key column should be min/max if there is no predica</td>
      <td>[TRAFODION-1053](https://issues.apache.org/jira/browse/TRAFODION-1053)</td>
    </tr>
    <tr>
      <td>Accessing hive table with ucs2 encoded field returns 0 rows.</td>
      <td>[TRAFODION-1165](https://issues.apache.org/jira/browse/TRAFODION-1165)</td>
    </tr>
    <tr>
      <td>Hybrid Query Cache: sqlci may err with JRE SIGSEGV.</td>
      <td>[TRAFODION-1173](https://issues.apache.org/jira/browse/TRAFODION-1173)</td>
    </tr>
    <tr>
      <td>DCS floating IP should set internal interface</td>
      <td>[TRAFODION-1176](https://issues.apache.org/jira/browse/TRAFODION-1176)</td>
    </tr>
    <tr>
      <td>sqgen may NOT copy all script files to all nodes</td>
      <td>[TRAFODION-1186](https://issues.apache.org/jira/browse/TRAFODION-1186)</td>
    </tr>
    <tr>
      <td>Self-referencing update updates the column to a wrong value</td>
      <td>[TRAFODION-1575](https://issues.apache.org/jira/browse/TRAFODION-1575)</td>
    </tr>
    <tr>
      <td>Generate constraint expression for update/merge commands</td>
      <td>[TRAFODION-1610](https://issues.apache.org/jira/browse/TRAFODION-1610)</td>
    </tr>
    <tr>
      <td>UPDATE STATISTICS allocates memory even when a SAMPLE table is used</td>
      <td>[TRAFODION-1715](https://issues.apache.org/jira/browse/TRAFODION-1715)</td>
    </tr>
    <tr>
      <td>Inserting NULL for all key columns in a table causes a failure</td>
      <td>[TRAFODION-1801](https://issues.apache.org/jira/browse/TRAFODION-1801)</td>
    </tr>
    <tr>
      <td>Range delete on tables with nullable key columns deletes fewer rows</td>
      <td>[TRAFODION-1803](https://issues.apache.org/jira/browse/TRAFODION-1803)</td>
    </tr>
    <tr>
      <td>support Trafodion running on CentOS 7</td>
      <td>[TRAFODION-1861](https://issues.apache.org/jira/browse/TRAFODION-1861)</td>
    </tr>
    <tr>
      <td>Have jdbc drivers available on Maven Repository</td>
      <td>[TRAFODION-1957](https://issues.apache.org/jira/browse/TRAFODION-1957)</td>
    </tr>
    <tr>
      <td>[offline backup&amp;restore] TrafexportSnapshot ran into NoClassDefFoundError on HDP2.4</td>
      <td>[TRAFODION-1966](https://issues.apache.org/jira/browse/TRAFODION-1966)</td>
    </tr>
    <tr>
      <td>Trafodion cannot adjust your working status in time when network broken.</td>
      <td>[TRAFODION-2077](https://issues.apache.org/jira/browse/TRAFODION-2077)</td>
    </tr>
    <tr>
      <td>JVM exception for TMUDF</td>
      <td>[TRAFODION-2182](https://issues.apache.org/jira/browse/TRAFODION-2182)</td>
    </tr>
    <tr>
      <td>After a region split the transactions to check against list is not fully populated</td>
      <td>[TRAFODION-2305](https://issues.apache.org/jira/browse/TRAFODION-2305)</td>
    </tr>
    <tr>
      <td>Documentation update for REST and DCS</td>
      <td>[TRAFODION-2307](https://issues.apache.org/jira/browse/TRAFODION-2307)</td>
    </tr>
    <tr>
      <td>JDBC T4 support read LOB</td>
      <td>[TRAFODION-2308](https://issues.apache.org/jira/browse/TRAFODION-2308)</td>
    </tr>
    <tr>
      <td>TransactionState.hasConflict returns true if it gets a null pointer exception</td>
      <td>[TRAFODION-2348](https://issues.apache.org/jira/browse/TRAFODION-2348)</td>
    </tr>
    <tr>
      <td>Update Messages Guide for some messages from 1100-1199</td>
      <td>[TRAFODION-2398](https://issues.apache.org/jira/browse/TRAFODION-2398)</td>
    </tr>
    <tr>
      <td>TRAFCI gui installer does not work</td>
      <td>[TRAFODION-2462](https://issues.apache.org/jira/browse/TRAFODION-2462)</td>
    </tr>
    <tr>
      <td>Alter table hbase options is not transaction enabled.</td>
      <td>[TRAFODION-2472](https://issues.apache.org/jira/browse/TRAFODION-2472)</td>
    </tr>
    <tr>
      <td>SQL POM file needs changes to be able to stage on Nexus repository</td>
      <td>[TRAFODION-2545](https://issues.apache.org/jira/browse/TRAFODION-2545)</td>
    </tr>
    <tr>
      <td>add more debug info for SQL Error 8402</td>
      <td>[TRAFODION-2590](https://issues.apache.org/jira/browse/TRAFODION-2590)</td>
    </tr>
    <tr>
      <td>ESP cores seen during daily builds after hive tests run</td>
      <td>[TRAFODION-2597](https://issues.apache.org/jira/browse/TRAFODION-2597)</td>
    </tr>
    <tr>
      <td>Error 9252 during update statistics of an encrypted Trafodion table</td>
      <td>[TRAFODION-2617](https://issues.apache.org/jira/browse/TRAFODION-2617)</td>
    </tr>
    <tr>
      <td>Docker</td>
      <td>[TRAFODION-2627](https://issues.apache.org/jira/browse/TRAFODION-2627)</td>
    </tr>
    <tr>
      <td>Merge returns internal error when the table has a constraint</td>
      <td>[TRAFODION-2630](https://issues.apache.org/jira/browse/TRAFODION-2630)</td>
    </tr>
    <tr>
      <td>MDAM costing overestimates I/O cost</td>
      <td>[TRAFODION-2645](https://issues.apache.org/jira/browse/TRAFODION-2645)</td>
    </tr>
    <tr>
      <td>The SQLite Trafodion Configuration database storage method can become stale.</td>
      <td>[TRAFODION-2650](https://issues.apache.org/jira/browse/TRAFODION-2650)</td>
    </tr>
    <tr>
      <td>The monitor to monitor process communication cannot handle a network reset</td>
      <td>[TRAFODION-2651](https://issues.apache.org/jira/browse/TRAFODION-2651)</td>
    </tr>
    <tr>
      <td>elastic - sqcheck cannot display configured TM/RMS count when instance is down</td>
      <td>[TRAFODION-2660](https://issues.apache.org/jira/browse/TRAFODION-2660)</td>
    </tr>
    <tr>
      <td>when I copy data from oracle table to trafodion table with max for limit rows， but the result rows in trafodion table is differ with max rows , and without any error.</td>
      <td>[TRAFODION-2670](https://issues.apache.org/jira/browse/TRAFODION-2670)</td>
    </tr>
    <tr>
      <td>Create index sees error 2006 internal assertion (keyColOffset == totalKeyLength)</td>
      <td>[TRAFODION-2674](https://issues.apache.org/jira/browse/TRAFODION-2674)</td>
    </tr>
    <tr>
      <td>odb load source with src=-file is failure</td>
      <td>[TRAFODION-2677](https://issues.apache.org/jira/browse/TRAFODION-2677)</td>
    </tr>
    <tr>
      <td>odb crash when extracting data to xml file</td>
      <td>[TRAFODION-2679](https://issues.apache.org/jira/browse/TRAFODION-2679)</td>
    </tr>
    <tr>
      <td>misleading error message for &#39;get schemas&#39; when _MD_ tables in hbase is not avialble</td>
      <td>[TRAFODION-2693](https://issues.apache.org/jira/browse/TRAFODION-2693)</td>
    </tr>
    <tr>
      <td>installers should secure bulkload hdfs dir for hbase upload</td>
      <td>[TRAFODION-2697](https://issues.apache.org/jira/browse/TRAFODION-2697)</td>
    </tr>
    <tr>
      <td>[ODBC]The maxlength for LargeInt was fixed to 8</td>
      <td>[TRAFODION-2701](https://issues.apache.org/jira/browse/TRAFODION-2701)</td>
    </tr>
    <tr>
      <td>[ODBC] The SQL type is set to CHAR (n) CHARACTER SET USC2, SQLGetData to read data multiple times returns the wrong length value</td>
      <td>[TRAFODION-2702](https://issues.apache.org/jira/browse/TRAFODION-2702)</td>
    </tr>
    <tr>
      <td>Using multi-threads app with linux-odbc to connect trafodion will make dcs down</td>
      <td>[TRAFODION-2709](https://issues.apache.org/jira/browse/TRAFODION-2709)</td>
    </tr>
    <tr>
      <td>odb didn&#39;t extract data from database to correct xml file</td>
      <td>[TRAFODION-2712](https://issues.apache.org/jira/browse/TRAFODION-2712)</td>
    </tr>
    <tr>
      <td>Query compilation gets stuck at listSnapshots() at times</td>
      <td>[TRAFODION-2716](https://issues.apache.org/jira/browse/TRAFODION-2716)</td>
    </tr>
    <tr>
      <td>Trafci insert with parameters can insert a string which length is larger than the column&#39;s length.</td>
      <td>[TRAFODION-2719](https://issues.apache.org/jira/browse/TRAFODION-2719)</td>
    </tr>
    <tr>
      <td>When setting max property , copy data from mysql to trafodion failed</td>
      <td>[TRAFODION-2721](https://issues.apache.org/jira/browse/TRAFODION-2721)</td>
    </tr>
    <tr>
      <td>Memory leak in the compiler part of the code in Trafodion</td>
      <td>[TRAFODION-2727](https://issues.apache.org/jira/browse/TRAFODION-2727)</td>
    </tr>
    <tr>
      <td>Parallel plan not chosen for native HBase table</td>
      <td>[TRAFODION-2729](https://issues.apache.org/jira/browse/TRAFODION-2729)</td>
    </tr>
    <tr>
      <td>UPDATE STATS fails sometimes when a column name is a SQL keyword</td>
      <td>[TRAFODION-2732](https://issues.apache.org/jira/browse/TRAFODION-2732)</td>
    </tr>
    <tr>
      <td>Provide an improved memory quota assignment for big memory operators (BMO)</td>
      <td>[TRAFODION-2733](https://issues.apache.org/jira/browse/TRAFODION-2733)</td>
    </tr>
    <tr>
      <td>Missing predicates on salt columns when using an index join</td>
      <td>[TRAFODION-2736](https://issues.apache.org/jira/browse/TRAFODION-2736)</td>
    </tr>
    <tr>
      <td>Native hbase table access via Trafodion improvements/issues</td>
      <td>[TRAFODION-2737](https://issues.apache.org/jira/browse/TRAFODION-2737)</td>
    </tr>
    <tr>
      <td>RMS semaphore handling need to log the error for easy resolution of the issue</td>
      <td>[TRAFODION-2739](https://issues.apache.org/jira/browse/TRAFODION-2739)</td>
    </tr>
    <tr>
      <td>Linux and Windows ODBC need to support LOB</td>
      <td>[TRAFODION-2742](https://issues.apache.org/jira/browse/TRAFODION-2742)</td>
    </tr>
    <tr>
      <td>Monitor exhibits memory corruption in large cluster configuration &gt 30 nodes</td>
      <td>[TRAFODION-2746](https://issues.apache.org/jira/browse/TRAFODION-2746)</td>
    </tr>
    <tr>
      <td>UPDATE STATS mc histogram failure when column has reserved word name</td>
      <td>[TRAFODION-2749](https://issues.apache.org/jira/browse/TRAFODION-2749)</td>
    </tr>
    <tr>
      <td>Use of rowset with large memory leads to various errors at runtime</td>
      <td>[TRAFODION-2759](https://issues.apache.org/jira/browse/TRAFODION-2759)</td>
    </tr>
    <tr>
      <td>hbase cache blocks is OFF for broad table with narrow index</td>
      <td>[TRAFODION-2760](https://issues.apache.org/jira/browse/TRAFODION-2760)</td>
    </tr>
    <tr>
      <td>Error when all TMUDF columns are eliminated</td>
      <td>[TRAFODION-2761](https://issues.apache.org/jira/browse/TRAFODION-2761)</td>
    </tr>
    <tr>
      <td>MDAM is not considered when sometimes it should be</td>
      <td>[TRAFODION-2765](https://issues.apache.org/jira/browse/TRAFODION-2765)</td>
    </tr>
    <tr>
      <td>HIVE: Inserted row into a hive table has gone missing</td>
      <td>[TRAFODION-2766](https://issues.apache.org/jira/browse/TRAFODION-2766)</td>
    </tr>
    <tr>
      <td>&quot;Table not found&quot; errors not being handled correctly by AQR for hive access</td>
      <td>[TRAFODION-2770](https://issues.apache.org/jira/browse/TRAFODION-2770)</td>
    </tr>
    <tr>
      <td>Add new Trafodion LOB Guide to documentation build and web site</td>
      <td>[TRAFODION-2771](https://issues.apache.org/jira/browse/TRAFODION-2771)</td>
    </tr>
    <tr>
      <td>Insert does not raise duplicate row error for hbase format table with defaulted first column</td>
      <td>[TRAFODION-2775](https://issues.apache.org/jira/browse/TRAFODION-2775)</td>
    </tr>
    <tr>
      <td>Mdam plans with more than one disjunct sometimes cause either a compiler core or have an incorrect predicate</td>
      <td>[TRAFODION-2776](https://issues.apache.org/jira/browse/TRAFODION-2776)</td>
    </tr>
    <tr>
      <td>Fix latent bug unmasked by JIRA TRAFODION-2765 fix</td>
      <td>[TRAFODION-2777](https://issues.apache.org/jira/browse/TRAFODION-2777)</td>
    </tr>
    <tr>
      <td>Unexpected assert when retrieving the current users roles</td>
      <td>[TRAFODION-2779](https://issues.apache.org/jira/browse/TRAFODION-2779)</td>
    </tr>
    <tr>
      <td>Mxosrvr dumps core when connection idle timer expires at times</td>
      <td>[TRAFODION-2780](https://issues.apache.org/jira/browse/TRAFODION-2780)</td>
    </tr>
    <tr>
      <td>mxosrvr dumps core generated when loading data from Oracle to Trafodion at times</td>
      <td>[TRAFODION-2785](https://issues.apache.org/jira/browse/TRAFODION-2785)</td>
    </tr>
    <tr>
      <td>Enabling Authentication fails to change setting</td>
      <td>[TRAFODION-2800](https://issues.apache.org/jira/browse/TRAFODION-2800)</td>
    </tr>
    <tr>
      <td>Salting + heuristics prevent MDAM plan on base table from being considered</td>
      <td>[TRAFODION-2813](https://issues.apache.org/jira/browse/TRAFODION-2813)</td>
    </tr>
    <tr>
      <td>Drop table fails with error 4247 when certain CQDs are set</td>
      <td>[TRAFODION-2819](https://issues.apache.org/jira/browse/TRAFODION-2819)</td>
    </tr>
    <tr>
      <td>Trafodion core code base needs to be thread safe</td>
      <td>[TRAFODION-2821](https://issues.apache.org/jira/browse/TRAFODION-2821)</td>
    </tr>
    <tr>
      <td>MERGE on a view defined using [first n] or [any n] does not work</td>
      <td>[TRAFODION-2822](https://issues.apache.org/jira/browse/TRAFODION-2822)</td>
    </tr>
    <tr>
      <td>odb export will scan full table with splitby column</td>
      <td>[TRAFODION-2830](https://issues.apache.org/jira/browse/TRAFODION-2830)</td>
    </tr>
    <tr>
      <td>Add missing error check in NATableDB::get</td>
      <td>[TRAFODION-2838](https://issues.apache.org/jira/browse/TRAFODION-2838)</td>
    </tr>
    <tr>
      <td>compGeneral/TEST023 leaves a bunch of stuff in /home/xxx/cbfs directory</td>
      <td>[TRAFODION-2839](https://issues.apache.org/jira/browse/TRAFODION-2839)</td>
    </tr>
    <tr>
      <td>ORDER BY clause on a view circumvents [first n] updatability check</td>
      <td>[TRAFODION-2840](https://issues.apache.org/jira/browse/TRAFODION-2840)</td>
    </tr>
    <tr>
      <td>Internal error (or core) on full outer join on an aggregate</td>
      <td>[TRAFODION-2843](https://issues.apache.org/jira/browse/TRAFODION-2843)</td>
    </tr>
    <tr>
      <td>Update Messages Guide for some messages in the range 1200-1299</td>
      <td>[TRAFODION-2852](https://issues.apache.org/jira/browse/TRAFODION-2852)</td>
    </tr>
    <tr>
      <td>Memory leak of ComDiagsArea in Context</td>
      <td>[TRAFODION-2853](https://issues.apache.org/jira/browse/TRAFODION-2853)</td>
    </tr>
    <tr>
      <td>Load encounter Operating system error 201</td>
      <td>[TRAFODION-2854](https://issues.apache.org/jira/browse/TRAFODION-2854)</td>
    </tr>
    <tr>
      <td>Remove incubating from trafodion web pages</td>
      <td>[TRAFODION-2857](https://issues.apache.org/jira/browse/TRAFODION-2857)</td>
    </tr>
    <tr>
      <td>Remove incubating reference(s) from code base</td>
      <td>[TRAFODION-2861](https://issues.apache.org/jira/browse/TRAFODION-2861)</td>
    </tr>
    <tr>
      <td>Update Messages Guide for some messages in the range 1300-1399</td>
      <td>[TRAFODION-2865](https://issues.apache.org/jira/browse/TRAFODION-2865)</td>
    </tr>
    <tr>
      <td>Trafodion DISCLAIMER should reflect the fact that Trafodion is TLP</td>
      <td>[TRAFODION-2869](https://issues.apache.org/jira/browse/TRAFODION-2869)</td>
    </tr>
    <tr>
      <td>DCS can not be normal visit.</td>
      <td>[TRAFODION-2870](https://issues.apache.org/jira/browse/TRAFODION-2870)</td>
    </tr>
    <tr>
      <td>Core dump due to reference to deallocated memory for EstLogProp object</td>
      <td>[TRAFODION-2879](https://issues.apache.org/jira/browse/TRAFODION-2879)</td>
    </tr>
    <tr>
      <td>Update Messages Guide for some messages in the range 1500-1599</td>
      <td>[TRAFODION-2880](https://issues.apache.org/jira/browse/TRAFODION-2880)</td>
    </tr>
    <tr>
      <td>Multiple node failures occur during HA testing</td>
      <td>[TRAFODION-2881](https://issues.apache.org/jira/browse/TRAFODION-2881)</td>
    </tr>
    <tr>
      <td>Foundation infrastructure changes needed to support operating in Cloudera Manager environment</td>
      <td>[TRAFODION-2882](https://issues.apache.org/jira/browse/TRAFODION-2882)</td>
    </tr>
    <tr>
      <td>dcsserver can&#39;t restart while master switching to backup-master</td>
      <td>[TRAFODION-2885](https://issues.apache.org/jira/browse/TRAFODION-2885)</td>
    </tr>
    <tr>
      <td>Streamline setjmp/longjmp concepts in Trafodion</td>
      <td>[TRAFODION-2888](https://issues.apache.org/jira/browse/TRAFODION-2888)</td>
    </tr>
    <tr>
      <td>Update Messages Guide for range 1700-1999 and some other cleanups</td>
      <td>[TRAFODION-2895](https://issues.apache.org/jira/browse/TRAFODION-2895)</td>
    </tr>
    <tr>
      <td>Internal error in stored procedures when a warning is generated in SQL</td>
      <td>[TRAFODION-2896](https://issues.apache.org/jira/browse/TRAFODION-2896)</td>
    </tr>
    <tr>
      <td>main branch break by mistake merge, fix it</td>
      <td>[TRAFODION-2898](https://issues.apache.org/jira/browse/TRAFODION-2898)</td>
    </tr>
    <tr>
      <td>Catalog API SQLColumns does not support ODBC2.x</td>
      <td>[TRAFODION-2899](https://issues.apache.org/jira/browse/TRAFODION-2899)</td>
    </tr>
    <tr>
      <td>optionally add leading space when get a number column to display</td>
      <td>[TRAFODION-2904](https://issues.apache.org/jira/browse/TRAFODION-2904)</td>
    </tr>
    <tr>
      <td>query id contains null if exec spj through trafci</td>
      <td>[TRAFODION-2905](https://issues.apache.org/jira/browse/TRAFODION-2905)</td>
    </tr>
    <tr>
      <td>create table with wrong char length hang and crash</td>
      <td>[TRAFODION-2908](https://issues.apache.org/jira/browse/TRAFODION-2908)</td>
    </tr>
    <tr>
      <td>Non-deterministic scalar UDFs not executed once per row</td>
      <td>[TRAFODION-2912](https://issues.apache.org/jira/browse/TRAFODION-2912)</td>
    </tr>
    <tr>
      <td>Tweak some MDAM-related heuristics</td>
      <td>[TRAFODION-2913](https://issues.apache.org/jira/browse/TRAFODION-2913)</td>
    </tr>
    <tr>
      <td>Refactor Trafodion implementation of hdfs scan for text formatted hive tables</td>
      <td>[TRAFODION-2917](https://issues.apache.org/jira/browse/TRAFODION-2917)</td>
    </tr>
    <tr>
      <td>various regression tests fail due to TRAFODION-2805 fix</td>
      <td>[TRAFODION-2918](https://issues.apache.org/jira/browse/TRAFODION-2918)</td>
    </tr>
    <tr>
      <td>calling int[] executeBatch() method execute create/drop/delete/insert/update/upsert statement always return -2</td>
      <td>[TRAFODION-2922](https://issues.apache.org/jira/browse/TRAFODION-2922)</td>
    </tr>
    <tr>
      <td>Keep log information for UPDATE STATISTICS in case of errors</td>
      <td>[TRAFODION-2927](https://issues.apache.org/jira/browse/TRAFODION-2927)</td>
    </tr>
    <tr>
      <td>Add news articles about Trafodion to Trafodion web site</td>
      <td>[TRAFODION-2928](https://issues.apache.org/jira/browse/TRAFODION-2928)</td>
    </tr>
    <tr>
      <td>ConnectionTimeout value for jdbc can&#39;t lager than 32768</td>
      <td>[TRAFODION-2933](https://issues.apache.org/jira/browse/TRAFODION-2933)</td>
    </tr>
    <tr>
      <td>In HA env, one node lose network, when recover, trafci can&#39;t use</td>
      <td>[TRAFODION-2940](https://issues.apache.org/jira/browse/TRAFODION-2940)</td>
    </tr>
    <tr>
      <td>Remove the 64K hard limit for process ids in RMS infrastructure</td>
      <td>[TRAFODION-2963](https://issues.apache.org/jira/browse/TRAFODION-2963)</td>
    </tr>
    <tr>
      <td>New MDAM costing code incorrectly assumes key column is always on the left</td>
      <td>[TRAFODION-2964](https://issues.apache.org/jira/browse/TRAFODION-2964)</td>
    </tr>
    <tr>
      <td>Hash partial groupby does not report a row count in operator statistics</td>
      <td>[TRAFODION-2965](https://issues.apache.org/jira/browse/TRAFODION-2965)</td>
    </tr>
    <tr>
      <td>New MDAM costing code does not always respect CQS forcing MDAM</td>
      <td>[TRAFODION-2966](https://issues.apache.org/jira/browse/TRAFODION-2966)</td>
    </tr>
    <tr>
      <td>UPDATE STATISTICS sometimes cores in CommonLogger::buildMsgBuffer</td>
      <td>[TRAFODION-2967](https://issues.apache.org/jira/browse/TRAFODION-2967)</td>
    </tr>
    <tr>
      <td>Subquery with [first 1] in select list results in Normalizer internal error</td>
      <td>[TRAFODION-2969](https://issues.apache.org/jira/browse/TRAFODION-2969)</td>
    </tr>
    <tr>
      <td>Some predefined UDFs should be regular UDFs so we can revoke rights</td>
      <td>[TRAFODION-2974](https://issues.apache.org/jira/browse/TRAFODION-2974)</td>
    </tr>
    <tr>
      <td>Correct the descriptions of how to launch ODBC in *Trafodion Client Installation Guide*</td>
      <td>[TRAFODION-2976](https://issues.apache.org/jira/browse/TRAFODION-2976)</td>
    </tr>
    <tr>
      <td>Trafodion allows creating objects with longer key lengths than HBase supports</td>
      <td>[TRAFODION-2977](https://issues.apache.org/jira/browse/TRAFODION-2977)</td>
    </tr>
    <tr>
      <td>Query fails with an internal error in the generator</td>
      <td>[TRAFODION-2983](https://issues.apache.org/jira/browse/TRAFODION-2983)</td>
    </tr>
    <tr>
      <td>Add documentation of systimestamp and sysdate in sql manual</td>
      <td>[TRAFODION-2987](https://issues.apache.org/jira/browse/TRAFODION-2987)</td>
    </tr>
    <tr>
      <td>CREATE TABLE LIKE fails with long numeric default value</td>
      <td>[TRAFODION-2990](https://issues.apache.org/jira/browse/TRAFODION-2990)</td>
    </tr>
    <tr>
      <td>https links in code</td>
      <td>[TRAFODION-2995](https://issues.apache.org/jira/browse/TRAFODION-2995)</td>
    </tr>
    <tr>
      <td>sleep execution code cannot build on CentOS 7</td>
      <td>[TRAFODION-2998](https://issues.apache.org/jira/browse/TRAFODION-2998)</td>
    </tr>
    <tr>
      <td>make UUID non reserved keyword</td>
      <td>[TRAFODION-2999](https://issues.apache.org/jira/browse/TRAFODION-2999)</td>
    </tr>
    <tr>
      <td>MERGE DELETE on table with unique index fails with error 4002</td>
      <td>[TRAFODION-3000](https://issues.apache.org/jira/browse/TRAFODION-3000)</td>
    </tr>
    <tr>
      <td>UPDATE STATS fails on certain INTERVAL columns</td>
      <td>[TRAFODION-3002](https://issues.apache.org/jira/browse/TRAFODION-3002)</td>
    </tr>
    <tr>
      <td>SQL dense buffers structure contain a size limit due to an incorrect cast</td>
      <td>[TRAFODION-3004](https://issues.apache.org/jira/browse/TRAFODION-3004)</td>
    </tr>
    <tr>
      <td>CREATE INDEX on certain long chars fails with Java exceptions</td>
      <td>[TRAFODION-3005](https://issues.apache.org/jira/browse/TRAFODION-3005)</td>
    </tr>
    <tr>
      <td>merge R2.2 changes into main branch</td>
      <td>[TRAFODION-3014](https://issues.apache.org/jira/browse/TRAFODION-3014)</td>
    </tr>
    <tr>
      <td>retrieve a value from numeric type get no result if using xx=&#39;value&#39;</td>
      <td>[TRAFODION-3015](https://issues.apache.org/jira/browse/TRAFODION-3015)</td>
    </tr>
    <tr>
      <td>windows odbc driver add support UTF8 output</td>
      <td>[TRAFODION-3016](https://issues.apache.org/jira/browse/TRAFODION-3016)</td>
    </tr>
    <tr>
      <td>Select of UPPER on upshifted column in join predicate cores</td>
      <td>[TRAFODION-3018](https://issues.apache.org/jira/browse/TRAFODION-3018)</td>
    </tr>
    <tr>
      <td>compGeneral/TESTTOK2 failed due to unneeded parser production.</td>
      <td>[TRAFODION-3020](https://issues.apache.org/jira/browse/TRAFODION-3020)</td>
    </tr>
    <tr>
      <td>set operator not cacheable</td>
      <td>[TRAFODION-3021](https://issues.apache.org/jira/browse/TRAFODION-3021)</td>
    </tr>
    <tr>
      <td>Add more enum files to analyzeMessageGuide.py</td>
      <td>[TRAFODION-3023](https://issues.apache.org/jira/browse/TRAFODION-3023)</td>
    </tr>
    <tr>
      <td>order by clause in subquery does not work</td>
      <td>[TRAFODION-3025](https://issues.apache.org/jira/browse/TRAFODION-3025)</td>
    </tr>
    <tr>
      <td>add create option storage policy</td>
      <td>[TRAFODION-3026](https://issues.apache.org/jira/browse/TRAFODION-3026)</td>
    </tr>
    <tr>
      <td>Control query shape (CQS) does not work for Hive insert and unload</td>
      <td>[TRAFODION-3028](https://issues.apache.org/jira/browse/TRAFODION-3028)</td>
    </tr>
    <tr>
      <td>Query with nested subqueries chooses bad plan</td>
      <td>[TRAFODION-3031](https://issues.apache.org/jira/browse/TRAFODION-3031)</td>
    </tr>
    <tr>
      <td>RAND() function is not always random</td>
      <td>[TRAFODION-3042](https://issues.apache.org/jira/browse/TRAFODION-3042)</td>
    </tr>
    <tr>
      <td>Inaccurate conditions of judgment cause low efficiency</td>
      <td>[TRAFODION-3049](https://issues.apache.org/jira/browse/TRAFODION-3049)</td>
    </tr>
    <tr>
      <td>Inefficient plan when joining on VARCHAR columns</td>
      <td>[TRAFODION-3050](https://issues.apache.org/jira/browse/TRAFODION-3050)</td>
    </tr>
    <tr>
      <td>Core in pCode evaluator for query on million-row non-partitioned table</td>
      <td>[TRAFODION-3052](https://issues.apache.org/jira/browse/TRAFODION-3052)</td>
    </tr>
    <tr>
      <td>seatrans pom files need to update</td>
      <td>[TRAFODION-3053](https://issues.apache.org/jira/browse/TRAFODION-3053)</td>
    </tr>
    <tr>
      <td>update pom.xml.cdh</td>
      <td>[TRAFODION-3054](https://issues.apache.org/jira/browse/TRAFODION-3054)</td>
    </tr>
    <tr>
      <td>Linux ODBC add Ansi function support</td>
      <td>[TRAFODION-3058](https://issues.apache.org/jira/browse/TRAFODION-3058)</td>
    </tr>
    <tr>
      <td>removee &quot;mt_&quot; column family</td>
      <td>[TRAFODION-3059](https://issues.apache.org/jira/browse/TRAFODION-3059)</td>
    </tr>
    <tr>
      <td>eliminate building errors on CentOS7</td>
      <td>[TRAFODION-3061](https://issues.apache.org/jira/browse/TRAFODION-3061)</td>
    </tr>
    <tr>
      <td>Deeply nested subqueries may have warning 2053 at compile time</td>
      <td>[TRAFODION-3066](https://issues.apache.org/jira/browse/TRAFODION-3066)</td>
    </tr>
    <tr>
      <td>DATEDIFF function gives strange results when executed on interval data types</td>
      <td>[TRAFODION-3071](https://issues.apache.org/jira/browse/TRAFODION-3071)</td>
    </tr>
    <tr>
      <td>Installer needs to support RH7 / Centos7</td>
      <td>[TRAFODION-3075](https://issues.apache.org/jira/browse/TRAFODION-3075)</td>
    </tr>
    <tr>
      <td>update pom.xml.hdp for protobuf compile</td>
      <td>[TRAFODION-3076](https://issues.apache.org/jira/browse/TRAFODION-3076)</td>
    </tr>
    <tr>
      <td>Trafodion compiles fail on CentOS7</td>
      <td>[TRAFODION-3083](https://issues.apache.org/jira/browse/TRAFODION-3083)</td>
    </tr>
    <tr>
      <td>Multi-value BETWEEN on interval key column gives wrong result</td>
      <td>[TRAFODION-3088](https://issues.apache.org/jira/browse/TRAFODION-3088)</td>
    </tr>
    <tr>
      <td>Incorrect rowcount reported after commit conflict error</td>
      <td>[TRAFODION-3092](https://issues.apache.org/jira/browse/TRAFODION-3092)</td>
    </tr>
    <tr>
      <td>At times the query involving sequence function fail and dumps core</td>
      <td>[TRAFODION-3097](https://issues.apache.org/jira/browse/TRAFODION-3097)</td>
    </tr>
    <tr>
      <td>enhance the &#39;get table&#39; utility to return number of rows</td>
      <td>[TRAFODION-3101](https://issues.apache.org/jira/browse/TRAFODION-3101)</td>
    </tr>
    <tr>
      <td>selct count(0) with long varchar hitting jvm OOM error</td>
      <td>[TRAFODION-3107](https://issues.apache.org/jira/browse/TRAFODION-3107)</td>
    </tr>
    <tr>
      <td>new checkin broken CentOS 7 code building</td>
      <td>[TRAFODION-3111](https://issues.apache.org/jira/browse/TRAFODION-3111)</td>
    </tr>
    <tr>
      <td>INTERVAL SECOND(m,n) in multi-column key results in 6003 warnings</td>
      <td>[TRAFODION-3128](https://issues.apache.org/jira/browse/TRAFODION-3128)</td>
    </tr>
    <tr>
      <td>Error message incorrect when describing non existing procedure</td>
      <td>[TRAFODION-1112](https://issues.apache.org/jira/browse/TRAFODION-1112)</td>
    </tr>
    <tr>
      <td>Change core file names in Sandbox</td>
      <td>[TRAFODION-1246](https://issues.apache.org/jira/browse/TRAFODION-1246)</td>
    </tr>
    <tr>
      <td>Revoke privilege return dependent grant ERROR[1037]</td>
      <td>[TRAFODION-1276](https://issues.apache.org/jira/browse/TRAFODION-1276)</td>
    </tr>
    <tr>
      <td>support adding/retrieving comments to SQL objects</td>
      <td>[TRAFODION-1915](https://issues.apache.org/jira/browse/TRAFODION-1915)</td>
    </tr>
    <tr>
      <td>sqps and sqcheck display wrong information after node reintegrated</td>
      <td>[TRAFODION-1958](https://issues.apache.org/jira/browse/TRAFODION-1958)</td>
    </tr>
    <tr>
      <td>support functions in the column default definition</td>
      <td>[TRAFODION-2335](https://issues.apache.org/jira/browse/TRAFODION-2335)</td>
    </tr>
  </table>
</span>


## Tasks

<span>
  <table>
    <tr>
      <th>Feature</th>
      <th>Jira ID</th>
    </tr>
    <tr>
      <td>SQL POM file  needs changes to be able to stage on Nexus repository</td>
      <td>[TRAFODION-2545](https://issues.apache.org/jira/browse/TRAFODION-2545)</td>
    </tr>
    <tr>
      <td>Remove usage of  TRAF_EXCLUDE_LIST</td>
      <td>[TRAFODION-2907](https://issues.apache.org/jira/browse/TRAFODION-2907)</td>
    </tr>
    <tr>
      <td>merge R2.2 changes into main branch</td>
      <td>[TRAFODION-3014](https://issues.apache.org/jira/browse/TRAFODION-3014)</td>
    </tr>
    <tr>
      <td>compGeneral/TESTTOK2  failed due to unneeded parser production.</td>
      <td>[TRAFODION-3020](https://issues.apache.org/jira/browse/TRAFODION-3020)</td>
    </tr>
    <tr>
      <td>set operator not cacheable</td>
      <td>[TRAFODION-3021](https://issues.apache.org/jira/browse/TRAFODION-3021)</td>
    </tr>
    <tr>
      <td>order by clause in subquery does not work</td>
      <td>[TRAFODION-3025](https://issues.apache.org/jira/browse/TRAFODION-3025)</td>
    </tr>
  </table>
</span>
## Documentation Updates

<span>
  <table>
    <tr>
      <th>Feature</th>
      <th>Jira ID</th>
    </tr>
    <tr>
      <td>Correct the descriptions of how to launch ODBC in *Trafodion Client Installation Guide</td>
      <td>[TRAFODION-2976](https://issues.apache.org/jira/browse/TRAFODION-2976)</td>
    </tr>
    <tr>
      <td>minor issues in adoc found during mvn site</td>
      <td>[TRAFODION-2986](https://issues.apache.org/jira/browse/TRAFODION-2986)</td>
    </tr>
    <tr>
      <td>Add documentation of systimestamp and sysdate in sql manual</td>
      <td>[TRAFODION-2987](https://issues.apache.org/jira/browse/TRAFODION-2987)</td>
    </tr>
  </table>
</span>

## Supported Platforms

<span>
  <table>
    <tr>
      <td>**Operating Systems**</td>
      <td>RedHat / CentOS 6.5 -- 6.8</td>
    </tr>
    <tr>
      <td>**Hadoop Distributions**</td>
      <td>Cloudera distributions CDH 5.4 -- 5.7<br/>
          Hortonworks distributions HDP 2.3 -- 2.4<br/>
          Apache Hadoop with Apache HBase 1.0 -- 1.1</td>
     </tr>
    <tr>
      <td>**Java Version**</td>
      <td>JDK 1.7, version 1.7.0_67 or newer</td>
    </tr>
    <tr>
      <td>**GCC Compiler**</td>
      <td>GCC 4.4</td>
    </tr>
  </table>
</span>
