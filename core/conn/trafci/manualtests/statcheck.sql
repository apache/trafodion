--
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
--

create schema trafci_manual;
set schema trafci_manual;

-- with statistics off

create table stat_check ( a int );
insert into stat_check values (1);

prepare s1 from select * from stat_check;
execute s1;
select * from stat_check;
get statistics;

-- with statistics on
set statistics on;

prepare s2 from select * from stat_check;
execute s2;
select * from stat_check;
get statistics;

set statistics off;
drop table stat_check;
drop schema trafci_manual;
