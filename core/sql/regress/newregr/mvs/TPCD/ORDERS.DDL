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
Create table orders  (
   o_orderkey          int                not null not droppable, 
   o_custkey           int                not null not droppable, 
   o_orderstatus       char(1)            not null not droppable, 
   o_totalprice        numeric(12,2)      not null not droppable, 
   o_orderdate         date               not null not droppable, 
   o_orderpriority     char(15)           not null not droppable, 
   o_clerk             char(15)           not null not droppable, 
   o_shippriority      int                not null not droppable, 
   o_comment           varchar(79)        not null not droppable, 
primary key (o_orderkey)  not droppable,
foreign key (o_custkey) references customer(c_custkey)) 
store by primary key 
location $data1
--partition ( 
--   add first key (         300001) location $data2)
attribute buffered, audit;
