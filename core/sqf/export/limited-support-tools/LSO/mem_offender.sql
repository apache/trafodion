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

select current_timestamp "CURRENT_TIMESTAMP",
   cast(tokenstr('nodeId:', variable_info) as integer) node,
   cast(tokenstr('processId:', variable_info) as integer) pid,
cast(tokenstr('exeMemHighWMInMB:', variable_info) as integer) EXE_MEM_HIGH_WM_MB,
cast(tokenstr('exeMemAllocInMB:', variable_info) as integer) EXE_MEM_ALLOC_MB,
cast(tokenstr('ipcMemHighWMInMB:', variable_info) as integer) IPC_MEM_HIGH_WM_MB,
cast(tokenstr('ipcMemAllocInMB:', variable_info) as integer) IPC_MEM_ALLOC_MB
from table(statistics(null, ?filter))
order by 5
;
