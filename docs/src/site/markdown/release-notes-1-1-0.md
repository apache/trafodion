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

This is the source-only release of the Apache Trafodion (incubating) project. In addition to including a number of new features and improvements across the project, the focus of this release is to comply with Apache release guidelines.

Build instructions are available [here](build.html).

# Supported Platforms
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

# Enhancements

This release contains the following new features.

<span>
  <table>
    <tr>
      <th>Category</th>
      <th>Feature</th>
      <th>Defect ID</th>
    </tr>
    <tr>
      <td>**Marketability, Infrastructure, and Scalability**</td>
      <td>
        <ul>
          <li>Critical and high defect repairs.</li>
          <li>Infrastructure refresh, including support for HDP 2.2 and CDH 5.3</li>
        </ul>
      </td>
      <td></td>
    </tr>
    <tr>
      <td>**Performance**</td>
      <td>
        <ul>
          <li>Query plan quality improvements identified by PoC/benchmarks, such as costing changes to help generate MDAM plans when appropriate.</li>
          <li>Performance and hardening improvements to user-defined routines (UDRs) (that is, stored procedures and C scalar user-defined functions (UDFs)).</li>
          <li>[Hybrid Query Cache](https://blueprints.launchpad.net/trafodion/+spec/hybrid-query-cache), which improves transaction performance and efficiency by moving the compiler’s SQL similarity detection check to the parser phase. Query caching allows reuse of pre-existing optimized SQL execution plans thereby eliminating costly compile and optimization overhead.</li>
          <li>[Skew Buster](https://blueprints.launchpad.net/trafodion/+spec/skew-buster), a patented feature in Trafodion that can recognize situations where data is skewed in intermediate stages of a query and adjust the query plan and execution time redistribution of intermediate data to ensure that all data is evenly distributed over all processing nodes.</li> 
          <li>[Immediate Update Statistics](https://wiki.trafodion.org/wiki/index.php/Creating_Statistics_During_Bulk_Load) for the entire table based on the sample taken during fast data loading. (Technology Preview--Complete But Not Fully Tested)</li>
        </ul>
      </td>
      <td></td>
    </tr>
    <tr>
      <td>**High Availability (HA) and Distributed Transaction Management (DTM)**</td>
      <td>
        <ul>
          <li>Transaction management efficiency and performance enhancements.</li>
          <li>Improvement of overall cluster HA now that the Trafodion Transaction Manager (TM) process is a persistent process, which eliminates the triggering of a node failure if the TM fails.</li>
          <li>DTM local transaction support to minimize the overhead of transactions by eliminating interactions with the TM process when the scope is local to the client that began the transaction. (Technology Preview--Work in Progress)</li>
          <li>Ability to run DDL statements in transactions, thus providing database consistency protection for DDL operations. (Technology Preview--Work in Progress)</li>
          <li>Stateless/Stateful Concurrency Control (SSCC), which ensures that transactions prevent data contention anomalies that can corrupt the consistency of a database. It works by preventing user transactions that interfere with each other’s data. SSCC is an extension of the Snapshot Isolation (SI) algorithm. SI prevents the majority of anomalies associated with data corruption and provides superior isolation to Multi-Version Concurrency Control (MVCC), which is used by DTM today. (Technology Preview--Work in Progress)</li>
        </ul>
      </td>
      <td></td>
    </tr>
    <tr>
      <td>**Usability**</td>
      <td>
        <ul>
          <li>Production-ready version of the [Backup/Restore Utility](https://wiki.trafodion.org/wiki/index.php/Backup_and_Restore), which allows you to backup and restore a Trafodion database while it is offline.</li>
          <li>Parallel data loader/extractor tool called "[odb](docs/Trafodion_odb_User_Guide.pdf)" for [trickle data loading and extraction](https://wiki.trafodion.org/wiki/index.php/Data_Loading#Trickle_Loading_Data_Into_Trafodion_Tables) to and from Trafodion tables.</li>
          <li>[Metadata Cleanup Utility](https://wiki.trafodion.org/wiki/index.php/Metadata_Cleanup), which is used to cleanup metadata tables after a failed DDL statement (Technology Preview--Complete But Not Tested)</li>
          <li>[Table-Mapping User-Defined Functions (TMUDF)](https://wiki.trafodion.org/wiki/index.php/Tutorial:_The_object-oriented_UDF_interface) written in C++. (Technology Preview--Complete But Not Tested)</li>
        </ul>
      </td>
      <td></td>
    </tr>
    <tr>
      <td>**Manageability**</td>
      <td>
        <ul>
          <li>Support for HP Data Services Manager (HP DSM), a unified, browser-based tool for management of Hadoop, Vertica, and now Trafodion data services. **NOTE**: The version of HP DSM that integrates with Trafodion is not yet available.</li>
          <li>Stability and overhead optimizations to reduce the overhead of capturing and maintaining query performance information (in repository tables).</li>
          <li>Query cancel for DDL, update statistics, and additional child query operations. For details, see the CONTROL QUERY CANCEL statement in the [Trafodion SQL Reference Manual](docs/Trafodion_SQL_Reference_Manual.pdf).</li>        
        </ul>
      </td>
      <td></td>
    </tr>
    <tr>
      <td>**Security**</td>
      <td>
        <ul>
          <li>Security subsystem hardening improvements including performance and QA testing.</li>
          <li>Security enhancements for the Trafodion metadata, data loader, and Data Connectivity Services (DCS).</li>
          <li>Upgrade authorization.</li>
          <li>Ability to grant privileges on behalf of a role using the GRANTED BY clause. For details, see the GRANT statements in the [Trafodion SQL Reference Manual](docs/Trafodion_SQL_Reference_Manual.pdf).</li>
        </ul>
      </td>
      <td></td>
    </tr>
    <tr>
      <td>**Installer**</td>
      <td>
        <ul>
          <li>Prompts to configure and enable security.</li>
          <li>Support for the latest distributions, HDP 2.2 and CDH 5.3.</li>        
        </ul>
      </td>
      <td></td>
    </tr>
  </table>
</span>

# Fixes

This release contains fixes to around 96 defects, including 17 critical defects, 53 high defects, 20 medium defects, and two low defects. Those defects were filed through [Launchpad](https://launchpad.net/trafodion/+milestone/r1.1).

# Known Issues

## EXECUTE.BATCH update creates core-file

**Defect:** [1274962](https://bugs.launchpad.net/trafodion/+bug/1274962)

**Symptom:** EXECUTE.BATCH hangs for a long time doing updates, and the update creates a core file.

**Cause:** To be determined.

**Solution:** Batch updates and ODBC row arrays do not currently work.

## Random update statistics failures with HBase OutOfOrderScannerNextException

**Defect:** [1391271](https://bugs.launchpad.net/trafodion/+bug/1391271)

**Symptom:** While running update statistics commands, you see HBase <code>OutOfOrderScannerNextException</code> errors.

**Cause:** The default <code>hbase.rpc.timeout</code> and <code>hbase.client.scanner.timeout.period</code> values might be too low given the size of the tables. Sampling in update statistics is implemented using the HBase Random RowFilter. For very large tables with several billion rows, the sampling ratio required to get a sample of 1 million rows is very small. This can result in HBase client connection timeout errors since there may be no row returned by a RegionServer for an extended period of time.

**Solution:** Increase the <code>hbase.rpc.timeout</code> and <code>hbase.client.scanner.timeout.period</code> values. We have found that increasing those values to 600 seconds (10 minutes) might sometimes prevent many timeout-related errors. For more information, see the [HBase Configuration and Fine Tuning Recommendations](https://wiki.trafodion.org/wiki/index.php/Configuration#Recommendations).

If increasing the <code>hbase.rpc.timeout</code> and <code>hbase.client.scanner.timeout.period</code> values does not work, try increasing the chosen sampling size. Choose a sampling percentage higher than the default setting of 1 million rows for large tables. For example, suppose table T has one billion rows. The following <code>UPDATE STATISTICS</code> statement will sample a million rows, or approximately one-tenth of one percent of the total rows:

    update statistics for table T on every column sample;

To sample one percent of the rows, regardless of the table size, you must explicitly state the sampling rate as follows:

    update statistics for table T on every column sample random 1 percent;

## Following update statistics, stats do not take effect immediately

**Defect:** [1409937](https://bugs.launchpad.net/trafodion/+bug/1409937)

**Symptom:** Immediately following an update statistics operation, the generated query plan does not seem to reflect the existence of statistics. For example, in a session, you create, and populate a table and then run update statistics on the table, prepare a query, and exit. A serial plan is generated and the estimated cardinality is 100 for both tables. In a new session, you prepare the same query, and a parallel plan is generated where the estimated cardinality reflects the statistics.

**Cause:** This is a day-one issue.

**Solution:** Retry the query after two minutes. Set <code>CQD HIST_NO_STATS_REFRESH_INTERVAL</code> to <code>'0'</code>. Run an <code>UPDATE STATISTICS</code> statement. Perform DML operations in a different session.

