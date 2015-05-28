-- Used to perform initialization of the SQL database during seabase regr run
--
-- @@@ START COPYRIGHT @@@
--
-- (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
--
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

#ifdef SEABASE_REGRESS

cqd mode_seabase 'ON';

delete from TRAFODION."_MD_".DEFAULTS
     where ATTRIBUTE  in('MODE_SEABASE', 'MODE_SEAHIVE', 'SCHEMA',
     'SEABASE_VOLATILE_TABLES');

insert into TRAFODION."_MD_".DEFAULTS
     values
     ('MODE_SEABASE ', 'ON', 'inserted during seabase regressions run'),
     ('SCHEMA ', 'TRAFODION.SCH ', 'inserted during seabase regressions run'),
     ('SEABASE_VOLATILE_TABLES ', 'ON', 'insert during seabase regressions run');

insert into TRAFODION."_MD_".DEFAULTS
     values
     ('MODE_SEAHIVE ', 'ON', 'inserted during seabase regressions run');

create shared schema trafodion.sch;

#endif

