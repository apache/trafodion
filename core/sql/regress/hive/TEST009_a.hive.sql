-- @@@ START COPYRIGHT @@@
--  Licensed under the Apache License, Version 2.0 (the "License");
--  you may not use this file except in compliance with the License.
--  You may obtain a copy of the License at
--
--      http://www.apache.org/licenses/LICENSE-2.0
--
--  Unless required by applicable law or agreed to in writing, software
--  distributed under the License is distributed on an "AS IS" BASIS,
--  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--  See the License for the specific language governing permissions and
--  limitations under the License.
--
-- @@@ END COPYRIGHT @@@

-- ============================================================================
-- All tables are created as EXTERNAL in a location already setup by 
-- install_local_hadoop. The EXTERNAL keyword lets you create a table and 
-- provide a LOCATION so that Hive does not use the default location 
-- specified by the configuration property hive.metastore.warehouse.dir.
-- ============================================================================

-- Our version of HIVE does not support special characters.  This test should 
-- be changed to use delimited names once we upgrade HIVE.

create schema if not exists hive.sch_t009;
set schema hive.sch_t009;
drop table t009t1;
create external table t009t1
(
    a int,
    b int, 
    c int
)
row format delimited fields terminated by '|'
location '/user/trafodion/hive/exttables/t009t1';

-- Our version of HIVE does not support insert ... VALUES clause, so use the
-- load command from an existing table.
insert into table t009t1
select c_customer_sk, c_birth_day, c_birth_month
from hive.customer
limit 10;

select * from t009t1;

drop table t009t2;
create external table t009t2
(
    a int,
    b int,
    c int
)
row format delimited fields terminated by '|'
location '/user/trafodion/hive/exttables/t009t2';

insert into table t009t2
select c_customer_sk, c_birth_day, c_birth_month
from hive.customer
limit 10;

select * from t009t2;


