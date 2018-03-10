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

# 2.2.0-Release Notes

This is the first release of the Apache Trafodion project.

Build instructions are available in the [Trafodion Contributor Guide](https://cwiki.apache.org/confluence/display/TRAFODION/Create+Build+Environment).

##  New Feature

<span>
  <table>
    <tr>
      <th>Feature</th>
      <th>Jira ID</th>
    </tr>
    <tr>
      <td>Trafodion Elasticity enhancements</td>
      <td>[TRAFODION-2001](https://issues.apache.org/jira/browse/TRAFODION-2001)</td>
    </tr>
    <tr>
      <td>LOB Support in JDBC</td>
      <td>[TRAFODION-2287](https://issues.apache.org/jira/browse/TRAFODION-2287)</td>
    </tr>
    <tr>
      <td>Improve UPDATE STATISTICS performance for varchar columns</td>
      <td>[TRAFODION-2376](https://issues.apache.org/jira/browse/TRAFODION-2376)</td>
    </tr>
    <tr>
      <td>RMS enhancements</td>
      <td>[TRAFODION-2420](https://issues.apache.org/jira/browse/TRAFODION-2420)</td>
    </tr>
    <tr>
      <td>jdbcT4 profile configuration for publish to maven central</td>
      <td>[TRAFODION-2513](https://issues.apache.org/jira/browse/TRAFODION-2513)</td>
    </tr>
    <tr>
      <td>Improve the log4j and log4cxx infrastructure in Trafodion</td>
      <td>[TRAFODION-2596](https://issues.apache.org/jira/browse/TRAFODION-2596)</td>
    </tr>
    <tr>
      <td>Port Esgyn DTM changes to Trafodion</td>
      <td>[TRAFODION-2623](https://issues.apache.org/jira/browse/TRAFODION-2623)</td>
    </tr>
  </table>
</span>

##  Improvement

<span>
  <table>
    <tr>
      <th>Feature</th>
      <th>Jira ID</th>
    </tr>
    <tr>
      <td>hdfs directories owned by trafodion id  should be under /user/trafodion</td>
      <td>[TRAFODION-2098](https://issues.apache.org/jira/browse/TRAFODION-2098)</td>
    </tr>
    <tr>
      <td>populateSortCols was flagged as major perf offender during profiling</td>
      <td>[TRAFODION-2422](https://issues.apache.org/jira/browse/TRAFODION-2422)</td>
    </tr>
    <tr>
      <td>Enhance stringtolob builtin function to take varchar/char columns as parameter</td>
      <td>[TRAFODION-2516](https://issues.apache.org/jira/browse/TRAFODION-2516)</td>
    </tr>
    <tr>
      <td>Allow scalar UDFs with delimited identifiers</td>
      <td>[TRAFODION-2517](https://issues.apache.org/jira/browse/TRAFODION-2517)</td>
    </tr>
    <tr>
      <td>Improve handling of index hints</td>
      <td>[TRAFODION-2569](https://issues.apache.org/jira/browse/TRAFODION-2569)</td>
    </tr>
    <tr>
      <td>Remove deprecated CQD HIVE_MAX_STRING_LENGTH</td>
      <td>[TRAFODION-2583](https://issues.apache.org/jira/browse/TRAFODION-2583)</td>
    </tr>
    <tr>
      <td>Remove obsolete utility commands, turn off obsolete privileges</td>
      <td>[TRAFODION-2603](https://issues.apache.org/jira/browse/TRAFODION-2603)</td>
    </tr>
    <tr>
      <td>set rowcount option in UPDATE STATISTICS does not suppress rowcount logic</td>
      <td>[TRAFODION-2618](https://issues.apache.org/jira/browse/TRAFODION-2618)</td>
    </tr>
    <tr>
      <td>Simplify installation setting of HBase config parameters</td>
      <td>[TRAFODION-2663](https://issues.apache.org/jira/browse/TRAFODION-2663)</td>
    </tr>
    <tr>
      <td>Ensure RMS can be disabled properly</td>
      <td>[TRAFODION-2698](https://issues.apache.org/jira/browse/TRAFODION-2698)</td>
    </tr>
    <tr>
      <td>Add or edition to error info 4222</td>
      <td>[TRAFODION-2805](https://issues.apache.org/jira/browse/TRAFODION-2805)</td>
    </tr>
    <tr>
      <td>A redundant statement</td>
      <td>[TRAFODION-2984](https://issues.apache.org/jira/browse/TRAFODION-2984)</td>
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
      <td>Implement relational operator for common subexpressions and transformation</td>
      <td>[TRAFODION-2317](https://issues.apache.org/jira/browse/TRAFODION-2317)</td>
    </tr>
    <tr>
      <td>Add optimizer/opt_error.h to analyzeMessageGuide.py</td>
      <td>[TRAFODION-2474](https://issues.apache.org/jira/browse/TRAFODION-2474)</td>
    </tr>
    <tr>
      <td>function support: Reverse()</td>
      <td>[TRAFODION-2485](https://issues.apache.org/jira/browse/TRAFODION-2485)</td>
    </tr>
    <tr>
      <td>Update wiki for scalar UDFs to explain trusted flavor</td>
      <td>[TRAFODION-2558](https://issues.apache.org/jira/browse/TRAFODION-2558)</td>
    </tr>
    <tr>
      <td>Support Index hints in a DML statement</td>
      <td>[TRAFODION-2573](https://issues.apache.org/jira/browse/TRAFODION-2573)</td>
    </tr>
    <tr>
      <td>SQL engine work for Elasticity</td>
      <td>[TRAFODION-2628](https://issues.apache.org/jira/browse/TRAFODION-2628)</td>
    </tr>
    <tr>
      <td>Add check to NATable cache for snapshot info</td>
      <td>[TRAFODION-2723](https://issues.apache.org/jira/browse/TRAFODION-2723)</td>
    </tr>
  </table>
</span>
        
##  Bug Fixes

<span>
  <table>
    <tr>
      <th>Feature</th>
      <th>Jira ID</th>
    </tr>
    <tr>
      <td>LP Bug: 1442483 - SQL queries hang when Region Server goes down</td>
      <td>[TRAFODION-1151](https://issues.apache.org/jira/browse/TRAFODION-1151)</td>
    </tr>
    <tr>
      <td>LP Bug: 1443482 - Accessing hive table with ucs2 encoded field returns 0 rows</td>
      <td>[TRAFODION-1165](https://issues.apache.org/jira/browse/TRAFODION-1165)</td>
    </tr>
    <tr>
      <td>after uninstall, and reinstall again, dcscheck report dcs master not up</td>
      <td>[TRAFODION-1989](https://issues.apache.org/jira/browse/TRAFODION-1989)</td>
    </tr>
    <tr>
      <td>Need better errors when DIVISION BY expression is incorrect</td>
      <td>[TRAFODION-2240](https://issues.apache.org/jira/browse/TRAFODION-2240)</td>
    </tr>
    <tr>
      <td>WITH clause with CTE used in subquery gives error 3288</td>
      <td>[TRAFODION-2248](https://issues.apache.org/jira/browse/TRAFODION-2248)</td>
    </tr>
    <tr>
      <td>TLOG repeatedly reporting exceptions in deleteEntriesOlderThanASN</td>
      <td>[TRAFODION-2253](https://issues.apache.org/jira/browse/TRAFODION-2253)</td>
    </tr>
    <tr>
      <td>need add privilege checking for explain statement</td>
      <td>[TRAFODION-2294](https://issues.apache.org/jira/browse/TRAFODION-2294)</td>
    </tr>
    <tr>
      <td>Sequence operators are not parallel starting with the WITH clause support</td>
      <td>[TRAFODION-2324](https://issues.apache.org/jira/browse/TRAFODION-2324)</td>
    </tr>
    <tr>
      <td>bash installer: always copy bashrc template file to trafodion&#39;s $HOME</td>
      <td>[TRAFODION-2428](https://issues.apache.org/jira/browse/TRAFODION-2428)</td>
    </tr>
    <tr>
      <td>Invalid characters in UCS2 to UTF8 translation are not handled correctly</td>
      <td>[TRAFODION-2477](https://issues.apache.org/jira/browse/TRAFODION-2477)</td>
    </tr>
    <tr>
      <td>HDFS connection issue during LOB creation</td>
      <td>[TRAFODION-2495](https://issues.apache.org/jira/browse/TRAFODION-2495)</td>
    </tr>
    <tr>
      <td>TMUDF sometimes does not pass errors from its input table up to the caller</td>
      <td>[TRAFODION-2499](https://issues.apache.org/jira/browse/TRAFODION-2499)</td>
    </tr>
    <tr>
      <td>Obscure cores seen in Trafodion while running jenkins tests with RH7</td>
      <td>[TRAFODION-2514](https://issues.apache.org/jira/browse/TRAFODION-2514)</td>
    </tr>
    <tr>
      <td>process abend when updating primary key with TRAF_RELOAD_NATABLE_CACHE ON</td>
      <td>[TRAFODION-2527](https://issues.apache.org/jira/browse/TRAFODION-2527)</td>
    </tr>
    <tr>
      <td>Update stats on cell or row access to a Trafodion table raises error 9252</td>
      <td>[TRAFODION-2529](https://issues.apache.org/jira/browse/TRAFODION-2529)</td>
    </tr>
    <tr>
      <td> UPDATE STATISTICS is sensitive to tdm_arkcmp autocommit setting</td>
      <td>[TRAFODION-2530](https://issues.apache.org/jira/browse/TRAFODION-2530)</td>
    </tr>
    <tr>
      <td> Create index succeeds but resulting index is empty, if dop greater than 32</td>
      <td>[TRAFODION-2535](https://issues.apache.org/jira/browse/TRAFODION-2535)</td>
    </tr>
    <tr>
      <td>Salted indexes do not result in parallel index scan plans</td>
      <td>[TRAFODION-2537](https://issues.apache.org/jira/browse/TRAFODION-2537)</td>
    </tr>
    <tr>
      <td>Skew buster plan not chosen when join predicate involves SUBSTRs</td>
      <td>[TRAFODION-2552](https://issues.apache.org/jira/browse/TRAFODION-2552)</td>
    </tr>
    <tr>
      <td>RIGHT function gives incorrect answer on UTF-8 varchars sometimes</td>
      <td>[TRAFODION-2559](https://issues.apache.org/jira/browse/TRAFODION-2559)</td>
    </tr>
    <tr>
      <td>Index plan not chosen for UPDATE when WHERE clause and set clause are on the same index column</td>
      <td>[TRAFODION-2574](https://issues.apache.org/jira/browse/TRAFODION-2574)</td>
    </tr>
    <tr>
      <td>UPDATE STATS sometimes fails on very long varchars</td>
      <td>[TRAFODION-2575](https://issues.apache.org/jira/browse/TRAFODION-2575)</td>
    </tr>
    <tr>
      <td>Incremental UPDATE STATS fails on long varchar values</td>
      <td>[TRAFODION-2576](https://issues.apache.org/jira/browse/TRAFODION-2576)</td>
    </tr>
    <tr>
      <td>installers should allow multiple ldap hosts and ldap UID lines</td>
      <td>[TRAFODION-2579](https://issues.apache.org/jira/browse/TRAFODION-2579)</td>
    </tr>
    <tr>
      <td>UPSERT USING LOAD running slower than UPSERT with transactions disabled</td>
      <td>[TRAFODION-2586](https://issues.apache.org/jira/browse/TRAFODION-2586)</td>
    </tr>
    <tr>
      <td>Give better diagnostics when HBase is not available while Trafodion starts</td>
      <td>[TRAFODION-2592](https://issues.apache.org/jira/browse/TRAFODION-2592)</td>
    </tr>
    <tr>
      <td>Insert Select to/from Trafodion tables containing LOB columns</td>
      <td>[TRAFODION-2598](https://issues.apache.org/jira/browse/TRAFODION-2598)</td>
    </tr>
    <tr>
      <td>sort operator merge phase memory pool improvement</td>
      <td>[TRAFODION-2604](https://issues.apache.org/jira/browse/TRAFODION-2604)</td>
    </tr>
    <tr>
      <td>Rework fix to JIRA Trafodion 2294</td>
      <td>[TRAFODION-2605](https://issues.apache.org/jira/browse/TRAFODION-2605)</td>
    </tr>
    <tr>
      <td>Input parameters and current functions in input tables of TMUDFs</td>
      <td>[TRAFODION-2611](https://issues.apache.org/jira/browse/TRAFODION-2611)</td>
    </tr>
    <tr>
      <td>Internal assert in CLEANUP command in some unusual cases</td>
      <td>[TRAFODION-2612](https://issues.apache.org/jira/browse/TRAFODION-2612)</td>
    </tr>
    <tr>
      <td>HBaseTxClient throws TableNotFoundException for TRAFODION._DTM_.TDDL</td>
      <td>[TRAFODION-2614](https://issues.apache.org/jira/browse/TRAFODION-2614)</td>
    </tr>
    <tr>
      <td>TMUDF returns wrong results with small numeric inputs</td>
      <td>[TRAFODION-2615](https://issues.apache.org/jira/browse/TRAFODION-2615)</td>
    </tr>
    <tr>
      <td> Nested join regression after fix for TRAFODION-2569</td>
      <td>[TRAFODION-2616](https://issues.apache.org/jira/browse/TRAFODION-2616)</td>
    </tr>
    <tr>
      <td>Memory leak in emitRow() in table-mapping UDFs</td>
      <td>[TRAFODION-2625](https://issues.apache.org/jira/browse/TRAFODION-2625)</td>
    </tr>
    <tr>
      <td>Disk IO counter is not populated for hdfs/hive IOs</td>
      <td>[TRAFODION-2631](https://issues.apache.org/jira/browse/TRAFODION-2631)</td>
    </tr>
    <tr>
      <td>FLOOR and CEIL return a float data type instead of the argument data type</td>
      <td>[TRAFODION-2634](https://issues.apache.org/jira/browse/TRAFODION-2634)</td>
    </tr>
    <tr>
      <td>Core on select count( *) using hbase cell access on a salted Trafodion table</td>
      <td>[TRAFODION-2635](https://issues.apache.org/jira/browse/TRAFODION-2635)</td>
    </tr>
    <tr>
      <td>Modest memory leak in metadata context and with CQS</td>
      <td>[TRAFODION-2636](https://issues.apache.org/jira/browse/TRAFODION-2636)</td>
    </tr>
    <tr>
      <td>Library management in the absence of password-less ssh</td>
      <td>[TRAFODION-2637](https://issues.apache.org/jira/browse/TRAFODION-2637)</td>
    </tr>
    <tr>
      <td>Ambari integration - dcs install with HA enabled</td>
      <td>[TRAFODION-2642](https://issues.apache.org/jira/browse/TRAFODION-2642)</td>
    </tr>
    <tr>
      <td>Obsolete the bash installer</td>
      <td>[TRAFODION-2644](https://issues.apache.org/jira/browse/TRAFODION-2644)</td>
    </tr>
    <tr>
      <td>mxosrvr connection state doesn&#39;t change to AVAILABLE after the timeout if no client connect to it</td>
      <td>[TRAFODION-2646](https://issues.apache.org/jira/browse/TRAFODION-2646)</td>
    </tr>
    <tr>
      <td>sqgen no longer provides &quot;overflow&quot; directive for scratch disks</td>
      <td>[TRAFODION-2647](https://issues.apache.org/jira/browse/TRAFODION-2647)</td>
    </tr>
    <tr>
      <td>New added persist configuration section missing program run time options</td>
      <td>[TRAFODION-2648](https://issues.apache.org/jira/browse/TRAFODION-2648)</td>
    </tr>
    <tr>
      <td>Method used in rmscheck script for obtaining status is incompatible with elasticity</td>
      <td>[TRAFODION-2649](https://issues.apache.org/jira/browse/TRAFODION-2649)</td>
    </tr>
    <tr>
      <td>Sort operator loops at times</td>
      <td>[TRAFODION-2653](https://issues.apache.org/jira/browse/TRAFODION-2653)</td>
    </tr>
    <tr>
      <td>Change the location of trafodion-site.xml from $TRAF_HOME/etc to config</td>
      <td>[TRAFODION-2654](https://issues.apache.org/jira/browse/TRAFODION-2654)</td>
    </tr>
    <tr>
      <td>MDAM plans on prefixes sometimes not chosen when they should be</td>
      <td>[TRAFODION-2655](https://issues.apache.org/jira/browse/TRAFODION-2655)</td>
    </tr>
    <tr>
      <td>Incremental UPDATE STATS fails on very large sample tables</td>
      <td>[TRAFODION-2662](https://issues.apache.org/jira/browse/TRAFODION-2662)</td>
    </tr>
    <tr>
      <td>privileges regression tests privs1/TEST040-44 output non-printable characters</td>
      <td>[TRAFODION-2678](https://issues.apache.org/jira/browse/TRAFODION-2678)</td>
    </tr>
    <tr>
      <td>Repeated execution of prepared SQL select statement causes memory leak</td>
      <td>[TRAFODION-2681](https://issues.apache.org/jira/browse/TRAFODION-2681)</td>
    </tr>
    <tr>
      <td>JVM startup options like heap are not passed correctly</td>
      <td>[TRAFODION-2682](https://issues.apache.org/jira/browse/TRAFODION-2682)</td>
    </tr>
    <tr>
      <td> the value of Numeric Struct need not to be changed to BidEndian</td>
      <td>[TRAFODION-2687](https://issues.apache.org/jira/browse/TRAFODION-2687)</td>
    </tr>
    <tr>
      <td>Log files are not created by monitor child processes</td>
      <td>[TRAFODION-2689](https://issues.apache.org/jira/browse/TRAFODION-2689)</td>
    </tr>
    <tr>
      <td>Monitor fails to start when node names are not of the right form</td>
      <td>[TRAFODION-2692](https://issues.apache.org/jira/browse/TRAFODION-2692)</td>
    </tr>
    <tr>
      <td>control query cancel qid fails with error 8031 sometimes</td>
      <td>[TRAFODION-2696](https://issues.apache.org/jira/browse/TRAFODION-2696)</td>
    </tr>
    <tr>
      <td>[ODBC]The maxlength for LargeInt was fixed to 8</td>
      <td>[TRAFODION-2701](https://issues.apache.org/jira/browse/TRAFODION-2701)</td>
    </tr>
    <tr>
      <td>[ODBC] The SQL type is set to CHARACTER(n) CHARACTER set USC2, SQLGetData to read data multiple times returns the wrong length value</td>
      <td>[TRAFODION-2702](https://issues.apache.org/jira/browse/TRAFODION-2702)</td>
    </tr>
    <tr>
      <td>Using multi-threads app with linux-odbc to connect trafodion will make dcs down</td>
      <td>[TRAFODION-2709](https://issues.apache.org/jira/browse/TRAFODION-2709)</td>
    </tr>
    <tr>
      <td>JDBC LOB tests show symptoms of a leaked statement handle</td>
      <td>[TRAFODION-2724](https://issues.apache.org/jira/browse/TRAFODION-2724)</td>
    </tr>
    <tr>
      <td>SQL types are real, FLOAT, and DOUBLE. Some values are inserted, a stack overflow occurs when SQLGetData is executed.</td>
      <td>[TRAFODION-2725](https://issues.apache.org/jira/browse/TRAFODION-2725)</td>
    </tr>
    <tr>
      <td>Using function strtod is not enough to convert C_CHAR to DOUBLE</td>
      <td>[TRAFODION-2750](https://issues.apache.org/jira/browse/TRAFODION-2750)</td>
    </tr>
    <tr>
      <td>JDBC executeQuery() throws exception on the with ... select stmt</td>
      <td>[TRAFODION-2757](https://issues.apache.org/jira/browse/TRAFODION-2757)</td>
    </tr>
    <tr>
      <td>LOAD and UNLOAD statements with LOB columns cause runtime errors</td>
      <td>[TRAFODION-2764](https://issues.apache.org/jira/browse/TRAFODION-2764)</td>
    </tr>
    <tr>
      <td>Select count( * ) from a renamed table should return error 4082 instead of error 8448</td>
      <td>[TRAFODION-2767](https://issues.apache.org/jira/browse/TRAFODION-2767)</td>
    </tr>
    <tr>
      <td> When convert NULL  to  SQL_VARCHAR , the length was not be sent</td>
      <td>[TRAFODION-2811](https://issues.apache.org/jira/browse/TRAFODION-2811)</td>
    </tr>
    <tr>
      <td>For Server 2008, function pow() in driver ODBC throws STATUS_ILLEGAL_INSTRUCTION</td>
      <td>[TRAFODION-2818](https://issues.apache.org/jira/browse/TRAFODION-2818)</td>
    </tr>
    <tr>
      <td>When using failed connection handle to alloc statement handle, crash happens</td>
      <td>[TRAFODION-2890](https://issues.apache.org/jira/browse/TRAFODION-2890)</td>
    </tr>
    <tr>
      <td>datalen is wrong while converting varchar in table to local datetime struct</td>
      <td>[TRAFODION-2902](https://issues.apache.org/jira/browse/TRAFODION-2902)</td>
    </tr>
    <tr>
      <td>Catalog Api gives wrong values about NON_UNIQUE column</td>
      <td>[TRAFODION-2911](https://issues.apache.org/jira/browse/TRAFODION-2911)</td>
    </tr>
    <tr>
      <td>Python installer does fails when Kerberos is enabled</td>
      <td>[TRAFODION-2935](https://issues.apache.org/jira/browse/TRAFODION-2935)</td>
    </tr>
    <tr>
      <td>initialize trafodion failed at CentOS 6.9</td>
      <td>[TRAFODION-2941](https://issues.apache.org/jira/browse/TRAFODION-2941)</td>
    </tr>
    <tr>
      <td>license year should be updated</td>
      <td>[TRAFODION-2942](https://issues.apache.org/jira/browse/TRAFODION-2942)</td>
    </tr>
  </table>
</span>


## Task

<span>
  <table>
    <tr>
      <th>Feature</th>
      <th>Jira ID</th>
    </tr>
    <tr>
      <td> JDBCT4 group id needs to be changed to org.apache.trafodion in JAR</td>
      <td>[TRAFODION-2544](https://issues.apache.org/jira/browse/TRAFODION-2544)</td>
    </tr>
    <tr>
      <td>JDBC T2 build changes needed for Trafodion release</td>
      <td>[TRAFODION-2554](https://issues.apache.org/jira/browse/TRAFODION-2554)</td>
    </tr>
  </table>
</span>

## Documentation

<span>
  <table>
    <tr>
      <th>Feature</th>
      <th>Jira ID</th>
    </tr>
    <tr>
      <td>update sql reference manual about with clause syntax</td>
      <td>[TRAFODION-2405](https://issues.apache.org/jira/browse/TRAFODION-2405)</td>
    </tr>
    <tr>
      <td>Add SCRATCH_DISKS CQD</td>
      <td>[TRAFODION-2521](https://issues.apache.org/jira/browse/TRAFODION-2521)</td>
    </tr>
    <tr>
      <td>Add with Clause</td>
      <td>[TRAFODION-2522](https://issues.apache.org/jira/browse/TRAFODION-2522)</td>
    </tr>
    <tr>
      <td>Add tinyint data type for sql reference manual</td>
      <td>[TRAFODION-2548](https://issues.apache.org/jira/browse/TRAFODION-2548)</td>
    </tr>
    <tr>
      <td>update sql reference manual about new hive data type</td>
      <td>[TRAFODION-2549](https://issues.apache.org/jira/browse/TRAFODION-2549)</td>
    </tr>
    <tr>
      <td>update sql reference manual about metadata clean up command</td>
      <td>[TRAFODION-2550](https://issues.apache.org/jira/browse/TRAFODION-2550)</td>
    </tr>
    <tr>
      <td>Remove Automating Update Statistics for SQL Reference Manual</td>
      <td>[TRAFODION-2657](https://issues.apache.org/jira/browse/TRAFODION-2657)</td>
    </tr>
    <tr>
      <td>Update Character String Data Types</td>
      <td>[TRAFODION-2665](https://issues.apache.org/jira/browse/TRAFODION-2665)</td>
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
