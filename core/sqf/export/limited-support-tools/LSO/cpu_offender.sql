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

-- set param ?filter 'CPU_OFFENDER=-1' ;
select current_timestamp "CURRENT_TIMESTAMP"  -- (1) Now
       ,count(*) no_of_processes              -- (2) Number of processed in QID
       ,sum(cast(tokenstr('diffCpuTime:', variable_info)        -- (3) Sum of CPU TIME in QID
             as NUMERIC(18) )) DIFF_CPU_TIME
       ,cast(tokenstr('Qid:', variable_info)             -- (4) QID
         as varchar(175) CHARACTER SET UTF8) QUERY_ID 
from table (statistics(NULL,?filter))
group by 4
order by 3 descending;
