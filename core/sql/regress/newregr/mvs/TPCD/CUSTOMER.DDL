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
Create table customer  (
   c_custkey           int                not null not droppable, 
   c_name              varchar(25)        not null not droppable, 
   c_address           varchar(40)        not null not droppable, 
   c_nationkey         int                not null not droppable, 
   c_phone             char(15)           not null not droppable, 
   c_acctbal           numeric(12,2)      not null not droppable, 
   c_mktsegment        char(10)           not null not droppable, 
   c_comment           varchar(117)       not null not droppable, 
primary key (c_custkey)  not droppable,
foreign key (c_nationkey) references nation(n_nationkey))
store by primary key 
location $data1 
--partition ( 
--   add first key (           7501) location $data2)
attribute buffered, audit;
