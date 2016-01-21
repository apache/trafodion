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

This is the source-only release of the Apache Trafodion (incubating) project. This release provides support for HBase 0.98 and many new product features.

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
      <td>**Performance**</td>
      <td>
        <ul>
          <li>Move to HBase 0.98.</li>
          <li>Significant improvements in benchmarks over Trafodion 0.8.3:</li>
            <ul>
              <li>YCSB</li>
              <li>Debit/Credit</li>
              <li>Order Entry</li>
              <li>Atomics</li>
            </ul>
        </ul>
      </td>
      <td></td>
    </tr>
    <tr>
      <td>**Infrastructure and High Availability**</td>
      <td>
        <ul>
          <li>Support on newer Hadoop distributions:
            <ul>
              <li>Cloudera CDH 5.1</li>
              <li>Hortonworks HDP 2.1</li>              
            </ul>
          </li>
          <li>Better integration with HBase:
            <ul>
              <li>Use of coprocessors</li>
              <li>Use of HLOG (HBase Write Ahead Log)</li>
            </ul>
          </li>
          <li>Recovery from catastrophic HBase failures.</li>
        </ul>
      </td>
      <td></td>
    </tr>
    <tr>
      <td>**Database and Query Performance**</td>
      <td>
        <ul>
          <li>Improvement in scan performance:
            <ul>
              <li>Tuning Degree of Parallelism</li>
              <li>Tuning scan buffer size</li>
            </ul>
          </li>
          <li>Prefetch</li>
          <li>Enhancements in JNI interface</li>
          <li>Improved partitioning for salted tables</li>
          <li>Push down predicates to HBase layer</li>
          <li>Enhancement in data movement/message traffic by sending compressed (internal format) data from one operator to another</li>
          <li>Improved memory usage</li>
          <li>Improvements to query compile time through embedded compilers</li>
          <li>Improvements in performance of update statistics feature</li>
        </ul>
      </td>
      <td></td>
    </tr>
    <tr>
      <td>**Database Movement and Connectivity**</td>
      <td>
        <ul>
          <li>Bulk Loader</li>
          <li>Support for compression in ODBC drivers</li>
        </ul>
      </td>
      <td></td>
    </tr>
    <tr>
      <td>**Security**</td>
      <td>
        <ul>
          <li>Support for Grant/Revoke</li>
        </ul>
      </td>
      <td></td>
    </tr>
    <tr>
      <td>**Stability**</td>
      <td>
        <ul>
          <li>Fixes for 140+ defects.</li>
        </ul>
      </td>
      <td></td>
    </tr>
  </table>
</span>

# Fixes

This release contains fixes to 140 defects. Those defects were filed through [Launchpad](https://launchpad.net/trafodion/+milestone/r1.1).

# Known Issues

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
