-- @@@ START COPYRIGHT @@@
--
-- (C) Copyright 2003-2015 Hewlett-Packard Development Company, L.P.
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

-- create environment for embedded SQL-C testcases chapter

SET SCHEMA cat1.schm;

drop trigger trig1;
drop trigger trig2;
drop trigger trig3;
drop trigger trig4;
drop trigger trig11;
drop trigger trig12;

DELETE FROM tab1A; -- empty table
DELETE FROM tab1B; -- empty table

DELETE FROM cat3.schm.tab3A; -- empty table
