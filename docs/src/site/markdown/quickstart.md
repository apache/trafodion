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
This page describes how to get going with Trafodion. 

**NOTE**: You need to install, configure, and start Trafodion before using the instructions on this page.  See [Install Instructions](download.html#install) on the [Download](download.html) page or the [Provisioning Guide](docs/provisioning_guide/index.html). 

# Basic Queries

Once you've started Trafodion, you're ready to do your first query. There are a couple of easy ways to create and enter Trafodion queries, but the fastest is to run **sqlci** or **trafci** from the Linux prompt on the server where you started Trafodion.

You can run **sqlci** or **trafci** directly from a Linux prompt.  At the sqlci or trafci prompt, you can type SQL statements such as:

    create schema test_schema;
    set schema test_schema;
    create table test_table (c1 int not null, c2 int not null, primary key (c1));
    insert into test_table values (1,1);
    insert into test_table values (2,2);
    select * from test_table where c2 = 2;

and so on.  Be sure to clean up when you're done.

    drop table test_table;
    drop schema test_schema cascade;

For a longer simple script that uses a few more Trafodion features, try the following, which should complete without errors:

    -------------------------------------------
    --
    --  Acid test script to make sure SQL has installed
    --
    ------------------------------------------

    create schema test_sandbox_schema;

    set schema test_sandbox_schema;

    create table t (c1 int not null, c2 int not null, primary key (c1));

    insert into t values (1,1);
    insert into t values (2,3);
    insert into t values (3,2);

    -- Now group two statements with a single transaction
    begin work;
    insert into t values (4,5);
    insert into t values (5,2);
    commit work;

    insert into t values (7,3);

    select * from t order by c2;

    create index tix on t (c2);

    create view tview as select c1, c2 from t where c2 > 3;

    select * from tview where c2 < 3;
    select * from tview where c2 > 2;

    update statistics for table t on every column;

    explain select * from t order by c2;
    select * from t order by c2;

    drop view tview;
    drop table t;
    drop schema test_sandbox_schema cascade;


This sort of script validates that Trafodion has been installed correctly, but does not generate significant load on the system.   To generate more load, refer to sample applications such as the OpenCart demo.  For information on how to set up and run the OpenCart demo with Trafodion, see [Trafodion Additional Documentation] (https://cwiki.apache.org/confluence/display/TRAFODION/Documentation/ "Additional Trafodion Documentation").

As you gain experience, you can refer to the [Trafodion SQL Reference Manual] (/docs/sql_reference/index.html) for additional SQL syntax.

# Connect with HBase

Trafodion interfaces and queries can reference both HBase data and Trafodion data in the same queries, including comparing, joining and analyzing the data.  For information on how to access native HBase tables, see the Trafodion SQL Reference Manual's [Using Trafodion SQL to Access HBase tables] (/docs/sql_reference/index.html#using_trafodion_sql_to_access_hbase_tables) section.

# Connect with Hive

Trafodion interfaces and queries can reference both HIVE data and Trafodion data in the same queries, including comparing, joining and analyzing the data.  For information on how to access tables in Hive, see the Trafodion SQL Reference Manual's [Using Trafodion SQL to Access Hive tables] (/docs/sql_reference/index.html#using_trafodion_sql_to_access_hive_tables) section.

# Import Test Data

Once you have entered a few basic SQL statements with Trafodion, you might consider adding data in bulk using one of the many load tools available.  See the [Trafodion odb User Guide] (/docs/odb/index.html) for usage examples for one such tool.

# Connect Database Tools

Trafodion provides connectivity services so that applications, tools, and other external clients can use Trafodion to access data.  See the [Trafodion Client Installation Guide] (http://trafodion.apache.org/docs/client_install/index.html) for information on how to connect external tools and clients, and see the [Trafodion Database Connectivity Guide] (/docs/dcs_reference/index.html) for information on Database Connectivity services.

# More Information

For more information, refer to the [Trafodion Documentation Page] (documentation.html) for starting points.  There is also a lot more information on the [Trafodion website] (http://trafodion.apache.org) and [Trafodion Wiki] (https://cwiki.apache.org/confluence/display/TRAFODION/Apache+Trafodion+Home) that you might refer to as you explore Trafodion.

If you have questions or suggestions or just want to share what you've learned about Trafodion, you can contact a community of Trafodion users via the [Trafodion User Group mailing list](http://mail-archives.apache.org/mod_mbox/incubator-trafodion-user/) or other [Project Mailing Lists] (http://trafodion.apache.org/mail-lists.html)

Have fun with Trafodion!
