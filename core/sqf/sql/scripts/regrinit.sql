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

-- initializes environment needed for sql dev regressions
upsert into TRAFODION."_MD_".DEFAULTS
     values
     ('SCHEMA ', 'TRAFODION.SCH ', 'inserted during seabase regressions run', 0);

create shared schema trafodion.sch;

alter user DB__ROOT set external name trafodion;

initialize authorization;

register user sql_user1 as sql_user1;
register user sql_user2 as sql_user2;
register user sql_user3 as sql_user3;
register user sql_user4 as sql_user4;
register user sql_user5 as sql_user5;
register user sql_user6 as sql_user6;
register user sql_user7 as sql_user7;
register user sql_user8 as sql_user8;
register user sql_user9 as sql_user9;
register user sql_user10 as sql_user10;

