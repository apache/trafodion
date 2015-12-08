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

This is the initial source-only release of the Apache Trafodion (incubating) project.

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
      <td>Hortonworks distribution HDP 2.0</td>
     </tr>
    <tr>
      <td>**Java Version**</td>
      <td>JDK 1.7.0_67 or newer</td>
    </tr>
    <tr>
      <td>**HBase Version**</td>
      <td>HBase 0.94.x</td>
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
      <td>**SQL**</td>
      <td>
        <ul>
          <li>Foreign key (referential integrity), check, and unique constraints in addition to primary key and not null constraints.</li>
          <li>Stored Procedures in Java (SPJs) (not yet fully tested).</li>
          <li>User-Defined Functions (UDFs) written in C (not yet fully tested).</li>
          <li>ACID properties apart from failure recovery.</li>
          <li>Transactional atomicity and consistency provided by the standard two-phased commit protocol with presumed abort semantics, supporting commit and rollback.</li>
          <li>Serializability based on a Multi-Version Concurrency Control (MVCC) algorithm in HBase-trx.</li>
          <li>Distributed/global transaction support across the cluster, including transaction propagation between SQL components.</li>
          <li>Support for multithreaded SQL clients.</li>
        </ul>
      </td>
      <td></td>
    </tr>
  </table>
</span>

# Fixes

## 0.8.0
Fixes to the Trafodion ODBC driver for Windows and the installer, which handles the installation of the Trafodion software. This release also includes performance fixes and cleanup to the Trafodion software code.

## 0.8.1
Release 0.8.1 provides these fixes to the Trafodion installer:

* An updated license agreement in the installer.
* Fix to the installation problem noted in [Launchpad Question #250671](https://answers.launchpad.net/trafodion/+question/250671).

## 0.8.2
Release 0.8.2 has a corrected license file in the installer.

## 0.8.3
Release 0.8.3 contains fixes to more than 35 critical and high defects, which were filed through [Launchpad](https://bugs.launchpad.net/trafodion/), and includes some performance enhancements based on DC and YCSB benchmarks. Native Expressions and Transaction Manager Recovery were disabled in order to avoid two critical issues. Those issues will be addressed in an upcoming release.

# Known Issues

## sqlci prints no banner or prompt when used in a su session

**Defect:** [1329001](https://bugs.launchpad.net/trafodion/+bug/1329001)

**Symptom:** <code>sqlci</code> hangs and is missing output after “<code>su</code>” command.

**Cause:** The <code>sqlci</code> program may appear hung (producing no output) when you use the “<code>su</code>” command to switch to another user. This is caused by a permission problem in Linux, making the standard output of <code>sqlci</code> disappear.

**Solution:** There are two possible workarounds:

1. Create a new session for the changed user; for example: with the <code>ssh</code> or <code>xterm</code> commands, or
2. Invoke the program like this: <code>sqlci | cat</code>

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
