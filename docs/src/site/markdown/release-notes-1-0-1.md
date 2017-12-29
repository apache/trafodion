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

This is the source-only release of the Apache Trafodion (incubating) project. This release provides product-level collection of query statistics and critical bug fixes above [1.0.0](release-notes-1-0-0.html).

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
      <td>Cloudera CDH distribution 5.1 and Hortonworks distribution HDP 2.1</td>
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
      <td>**High Availability**</td>
      <td>Production Ready:
        <ul>
          <li>Recovery from Trafodion infrastructure process failures. (Transaction Manager, Monitor)</li>
          <li>Recovery from HBase Region Server failures.</li>
          <li>Recovery from node failures.</li>
          <li>Recovery from catastrophic system failures.</li>
        </ul>
      </td>
      <td></td>
    </tr>
    <tr>
      <td>**Performance**</td>
      <td>Production Ready:
        <ul>
          <li>[Automated Update Statistics](https://blueprints.launchpad.net/trafodion/+spec/ustat-automation) (initial, basic support).</li>
          <li>Nested join enhancements.</li>
          <li>Index support enhancements.</li>
          <li>Scan performance.</li>
        </ul>
        Technology Preview:
        <ul>
          <li>[Multi-Temperature Data](https://cwiki.apache.org/confluence/display/TRAFODION/Cmp-divisioning) (Complete But Not Tested)</li>
        </ul>
        For more information about automated update statistics and index support enhancements, see the [Trafodion SQL Reference Manual](docs/Trafodion_SQL_Reference_Manual.pdf).
      </td>
      <td></td>
    </tr>
    <tr>
      <td>**Usability**</td>
      <td>Technology Preview:
        <ul>
          <li>[Native HBase Tables](https://blueprints.launchpad.net/trafodion/+spec/access-external-hbase-tables) — Select/insert/update/delete data. (Complete But Not Tested)</li>
          <li>[Sequence numbers](https://blueprints.launchpad.net/trafodion/+spec/sequence-numbers) and [IDENTITY column](https://blueprints.launchpad.net/trafodion/+spec/identity-column) (Complete But Not Tested)</li>
          <li>Oracle compatibility features (disabled by default) (Complete But Not Tested)</li>
          <li>[Large Objects (LOB)](https://blueprints.launchpad.net/trafodion/+spec/lob-support) support (Work in Progress)</li>
        </ul>
      </td>
      <td></td>
    </tr>
    <tr>
      <td>**Database Movement and Connectivity**</td>
      <td>Production Ready:
        <ul>
          <li>[Bulk Unload](https://blueprints.launchpad.net/trafodion/+spec/bulkunload)</li>
        </ul>
        Technology Preview:
        <ul>
          <li>[Backup/Restore](https://wiki.trafodion.org/wiki/index.php/Backup_and_Restore). (Complete But Not Tested)</li>
          <li>Thread-Safe JDBC Type 2 Driver. (Complete But Not Tested)</li>
        </ul>
        For more information about the new UNLOAD statement, see the [Trafodion SQL Reference Manual](docs/Trafodion_SQL_Reference_Manual.pdf).
      </td>
      <td></td>
    </tr>
    <tr>
      <td>**Manageability**</td>
      <td>Production Ready:
        <ul>
          <li>Visibility of SQL runtime statistics via the [Runtime Management System (RMS)](https://blueprints.launchpad.net/trafodion/+spec/enable-get-statistics-thru-rms).</li>
          <li>Ability to [Cancel Queries](https://blueprints.launchpad.net/trafodion/+spec/sql-query-cancel) (DML statements only, not DDL, update statistics, and additional child query operations).</li>
          <li>[Event Handling](https://wiki.trafodion.org/wiki/index.php/Trafodion_Manageability#Logging_Events) through UDFs and log4cpp.</li>
          <li>Collection of session and query statistics in [Repository Tables](https://wiki.trafodion.org/wiki/index.php/Trafodion_Manageability) (disabled by default)</li>
        </ul>
        For more information about gathering SQL runtime statistics and canceling an executing query, see the [Trafodion SQL Reference Manual](docs/Trafodion_SQL_Reference_Manual.pdf).
      </td>
      <td></td>
    </tr>
    <tr>
      <td>**Security**</td>
      <td>Production Ready:
        <ul>
          <li>[ANSI Schema Support](https://blueprints.launchpad.net/trafodion/+spec/security-ansi-schemas).</li>
          <li>[Privilege checking for SPJs, UDFs, libraries, and sequence generators](https://blueprints.launchpad.net/trafodion/+spec/security-privilege-updates).</li>
          <li>[Privilege checking for utilities](https://blueprints.launchpad.net/trafodion/+spec/security-privilege-updates).</li>
          <li>[Metadata Query Invalidation](https://blueprints.launchpad.net/trafodion/+spec/ddl-query-invalidation) (that is, detection and propagation of privilege changes to SQL compilers).</li>
        </ul>
        For more information about ANSI schema support and privilege and authorization updates in this release, see the [Trafodion SQL Reference Manual](docs/Trafodion_SQL_Reference_Manual.pdf).
      </td>
      <td></td>
    </tr>
  </table>
</span>

# Fixes

This release contains fixes to around 85 defects, including 25 critical defects, 54 high defects, and 10 medium and low defects. Those defects were filed through [Launchpad](https://launchpad.net/trafodion/+milestone/r1.1).

# Known Issues

Release 1.0.1 provides fixes to these bugs:

* **[1415165](https://bugs.launchpad.net/trafodion/+bug/1415165)**: MXOSRVR memory leak when stats are enabled
* **[1421727](https://bugs.launchpad.net/trafodion/+bug/1421727)**: Default/aggregated stats type not publishing query stats
* **[1416539](https://bugs.launchpad.net/trafodion/+bug/1416539)** Snapshot scan installer additions
* **[1413418](https://bugs.launchpad.net/trafodion/+bug/1413418)**: Need metadata upgrade option in installer

## Getting TM error 97 when tables split or get moved

**Defect:** [1274651](https://bugs.launchpad.net/trafodion/+bug/1274651)

**Symptom:** HBase Region Splits, Load Balancing, and Error 97.

**Cause:** As part of an HBase environment’s ongoing operations (and based on the policies configured for the HBase environment), an HBase region can either get split (into two daughter regions) or moved to a different region server. (Please see the blog: [http://hortonworks.com/blog/apache-hbase-region-splitting-and-merging/](http://hortonworks.com/blog/apache-hbase-region-splitting-and-merging/).) If that happens when a Trafodion transaction is active (and operates on rows within the region that is being split or load-balanced), then a subsequent transaction commit operation by the application might encounter an error 97. Please note that under such conditions the Trafodion Transaction Manager will abort the transaction and will preserve the integrity of the database.

**Solution:** To minimize disruptions when this happens, we suggest that you use one or more of the following approaches:

<ol>
  <li>Enhance your JDBC application logic to retry when an error 97 is returned for a commit operation.</li>
  <li>Update the HBase configuration to reduce the times when such disruptions happen. It involves updates to some properties that can be set in <code>hbase-site.xml</code> (or via the manageability interface of your Hadoop distribution).</li>
    <ul>
      <li>Set the maximum file size of an HBase Region to 100 GB. For example, set the value of the property <code>hbase.hregion.max.filesize</code> to <code>107374182400</code>.</li>
      <li><p>Set the HBase region split policy to '<code>ConstantSizeRegionSplitPolicy</code>'. For example, set the value of the property <code>hbase.regionserver.region.split.policy</code> to <code>org.apache.hadoop.hbase.regionserver.ConstantSizeRegionSplitPolicy</code>.</p> 
          <p><strong>NOTE</strong>: The split policy should already be set to '<code>onstantSizeRegionSplitPolicy</code>' by the Trafodion installer.</p>
          <p><strong>Summary</strong></p>
          <p>
             <span>
               <table>
                 <tr>
                   <th>Property</th>
                   <th>Value</th>
                 </tr>
                 <tr>
                   <td><code>hbase.hregion.max.filesize</code></td>
                   <td><code>107374182400</code></td>
                 </tr>
                 <tr>
                   <td><code>hbase.regionserver.region.split.policy</code></td>
                   <td><code>org.apache.hadoop.hbase.regionserver.ConstantSizeRegionSplitPolicy</code></td>
                 </tr>
               </table>
            </span>
            More information: [http://hortonworks.com/blog/apache-hbase-region-splitting-and-merging/](http://hortonworks.com/blog/apache-hbase-region-splitting-and-merging/)
          </p>
       </li>
    </ul>
    <li><p>Disable HBase Region Load Balancing. Use the HBase shell command balance_switch false to disable the movement of a region from one server to another.</p>
        <p><strong>Example</strong></p>
        <pre>
hbase shell
hbase(main):002:0> balance_switch false
true  -- Output will be the last setting of the balance_switch value
0 row(s) in 0.0080 seconds
        </pre> 
    </li>
    <li>
    Pre-split the table into multiple regions by using the <code>SALT USING n PARTITIONS</code> clause when creating the table. The number of partitions that you specify could be a function of the number of region servers present in the HBase cluster. Here is a simple example in which the table INVENTORY is pre-split into four regions when created:
    <pre>
CREATE TABLE INVENTORY
  (
    ITEM_ID       INT UNSIGNED NO DEFAULT NOT NULL
  , ITEM_TYPE     INT UNSIGNED NO DEFAULT NOT NULL
  , ITEM_COUNT    INT UNSIGNED NO DEFAULT NOT NULL
  , PRIMARY KEY (ITEM_ID ASC)
  )  SALT USING 4 PARTITIONS
  ;    </pre>
    </li>
</ol>

## EXECUTE.BATCH update creates core-file

**Defect:** [1274962](https://bugs.launchpad.net/trafodion/+bug/1274962)

**Symptom:** <code>EXECUTE.BATCH</code> hangs for a long time doing updates, and the update creates a core file.

**Cause:** To be determined.

**Solution:** Batch updates and ODBC row arrays do not currently work.

## Random update statistics failures with HBase OutOfOrderScannerNextException

**Defect:** [1391271](https://bugs.launchpad.net/trafodion/+bug/1391271)

**Symptom:** While running update statistics commands, you see HBase OutOfOrderScannerNextException errors.

**Cause:** The default <code>hbase.rpc.timeout</code> and <code>hbase.client.scanner.timeout.period</code> values might be too low given the size of the tables. Sampling in update statistics is implemented using the HBase Random RowFilter. For very large tables with several billion rows, the sampling ratio required to get a sample of 1 million rows is very small. This can result in HBase client connection timeout errors since there may be no row returned by a RegionServer for an extended period of time.

**Solution:** Increase the <code>hbase.rpc.timeout</code> and <code>hbase.client.scanner.timeout.period</code> values. We have found that increasing those values to 600 seconds (10 minutes) might sometimes prevent many timeout-related errors. For more information, see the [HBase Configuration and Fine Tuning Recommendations](https://wiki.trafodion.org/wiki/index.php/Configuration#Recommendations).

If increasing the <code>hbase.rpc.timeout</code> and <code>hbase.client.scanner.timeout.period</code> values does not work, try increasing the chosen sampling size. Choose a sampling percentage higher than the default setting of 1 million rows for large tables. For example, suppose table T has one billion rows. The following <code>UPDATE STATISTICS</code> statement will sample a million rows, or approximately one-tenth of one percent of the total rows:

    update statistics for table T on every column sample;

To sample one percent of the rows, regardless of the table size, you must explicitly state the sampling rate as follows:

    update statistics for table T on every column sample random 1 percent;

## Following update statistics, stats do not take effect immediately

**Defect:** [1409937](https://bugs.launchpad.net/trafodion/+bug/1409937)

**Symptom:**  Immediately following an update statistics operation, the generated query plan does not seem to reflect the existence of statistics. For example, in a session, you create, and populate a table and then run update statistics on the table, prepare a query, and exit. A serial plan is generated and the estimated cardinality is 100 for both tables. In a new session, you prepare the same query, and a parallel plan is generated where the estimated cardinality reflects the statistics.

**Cause:** This is a day-one issue.

**Solution:** Retry the query after two minutes. Set <code>CQD HIST_NO_STATS_REFRESH_INTERVAL</code> to '<code>0</code>'. Run an <code>UPDATE STATISTICS</code> statement. Perform DML operations in a different session.
