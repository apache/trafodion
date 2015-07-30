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
Create table $$table2$$  (
   o_orderkey          int                not null, 
   o_custkey           int                not null, 
   o_orderstatus       char(1)            not null, 
   o_totalprice        numeric(12,2)      not null, 
   o_orderdate         date               not null, 
   o_orderpriority     char(15)           not null, 
   o_clerk             char(15)           not null, 
   o_shippriority      int                not null, 
   o_comment           varchar(79)        not null, 
?ifMX
primary key (o_orderkey) )  location $DATA2
attribute buffered
?ifMX

?ifNSKRel1
primary key (o_orderkey))
buffered
?ifNSKRel1
;
