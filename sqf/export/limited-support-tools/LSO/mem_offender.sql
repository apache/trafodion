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
