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

process hive ddl 'create schema if not exists sch_t009';
process hive ddl 'drop table sch_t009.t009t1';
process hive ddl 'create external table sch_t009.t009t1 (a int, b int, c int, d int) row format delimited fields terminated by ''|'' location ''/user/trafodion/hive/exttables/t009t1'' ';

-- process hive statement with insert runs into an error on HDP platform.
-- Use regrhive until that issue is fixed.
--process hive statement 'insert into table sch_t009.t009t1 select c_customer_sk, c_birth_day, c_birth_month, c_birth_year from customer limit 10 ';
sh echo "insert into table sch_t009.t009t1 select c_customer_sk, c_birth_day, c_birth_month, c_birth_year from customer limit 10;" > TEST009_junk;
sh regrhive.ksh -f TEST009_junk;

--select * from hive.sch_t009.t009t1;



