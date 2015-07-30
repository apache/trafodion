-- @@@ START COPYRIGHT @@@
--
-- Licensed to the Apache Software Foundation (ASF) under one
-- or more contributor license agreements.  See the NOTICE file
-- distributed with this work for additional information
-- regarding copyright ownership.  The ASF licenses this file
-- to you under the Apache License, Version 2.0 (the
-- "License"); you may not use this file except in compliance
-- with the License.  You may obtain a copy of the License at
--
--   http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing,
-- software distributed under the License is distributed on an
-- "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
-- KIND, either express or implied.  See the License for the
-- specific language governing permissions and limitations
-- under the License.
--
-- @@@ END COPYRIGHT @@@
-- table to store import execution times
drop table imptimes;
create table imptimes(run_time timestamp(3) not null, duration dec(6,2),
                      rows_imported int);

-- table to keep update statistics execution times
drop table updsttimes;
create table updsttimes (Operation char(20) not null,
                         Run_Time timestamp(3),
                         Duration time(3));

-- table to store query execution times
drop table querytimes;
create table querytimes (Query_Name char(20),
                         Run_Time timestamp(3),
                         Duration time(3));

-- baseline table
drop table baseperf;
create table baseperf(Operation char(30) not null unique, duration dec(8,3));

-- restore baseline table data
sh $mxlibdir/import exeperf.sch.baseperf -i perfbaseline.dat -FD '|' -F 1;

-- test result table
drop table resultperf;
create table resultperf(
    Operation char(30) not null unique,
    res char(5),
    expected dec(8,3),
    actual dec(8,3),
    pct dec(8,3)
);
