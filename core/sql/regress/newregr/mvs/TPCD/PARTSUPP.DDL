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
Create table partsupp  (
   ps_partkey          int                not null not droppable, 
   ps_suppkey          int                not null not droppable, 
   ps_availqty         int                not null not droppable, 
   ps_supplycost       numeric(12,2)      not null not droppable, 
   ps_comment          varchar(199)       not null not droppable, 
primary key (ps_partkey,ps_suppkey)  not droppable,
foreign key (ps_partkey) references part(p_partkey),
foreign key (ps_suppkey) references supplier(s_suppkey))
store by primary key 
location $data1 
--partition ( 
--   add first key (          10001) location $data2)
attribute buffered, audit;
