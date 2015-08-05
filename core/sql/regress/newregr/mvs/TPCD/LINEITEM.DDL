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
Create table lineitem  (
   l_orderkey          int                not null not droppable, 
   l_partkey           int                not null not droppable, 
   l_suppkey           int                not null not droppable, 
   l_linenumber        int                not null not droppable, 
   l_quantity          numeric(12,2)      not null not droppable, 
   l_extendedprice     numeric(12,2)      not null not droppable, 
   l_discount          numeric(12,2)      not null not droppable, 
   l_tax               numeric(12,2)      not null not droppable, 
   l_returnflag        char(1)            not null not droppable, 
   l_linestatus        char(1)            not null not droppable, 
   l_shipdate          date               not null not droppable, 
   l_commitdate        date               not null not droppable, 
   l_receiptdate       date               not null not droppable, 
   l_shipinstruct      char(25)           not null not droppable, 
   l_shipmode          char(10)           not null not droppable, 
   l_comment           varchar(44)        not null not droppable, 
primary key (l_orderkey,l_linenumber)  not droppable,
foreign key (l_partkey,l_suppkey) references partsupp(ps_partkey,ps_suppkey),
foreign key (l_orderkey) references orders(o_orderkey))
store by primary key
location $data1 
--partition ( 
--   add first key (         300001) location $data2)
attribute buffered, audit;
