-- @@@ START COPYRIGHT @@@
--
-- (C) Copyright 2002-2015 Hewlett-Packard Development Company, L.P.
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

create schema  NONSTOP_SQLMX_NSK.SCH;
create catalog cat;
create schema  cat.sch;

delete from NONSTOP_SQLMX_NSK.SYSTEM_DEFAULTS_SCHEMA.SYSTEM_DEFAULTS
  where ATTRIBUTE in ('CATALOG', 'SCHEMA');
insert into NONSTOP_SQLMX_NSK.SYSTEM_DEFAULTS_SCHEMA.SYSTEM_DEFAULTS
  (ATTRIBUTE, ATTR_VALUE, ATTR_COMMENT)
  values
  ('SCHEMA ', 'Cat.Sch', 'inserted by init_sql_dev');
