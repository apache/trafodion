#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "BaseTypes.h"
#include "NAAssert.h"
#include "stdlib.h"
#include "stdio.h"
#include "sqlcli.h"
#include "ComDiags.h"
#include "ex_stdh.h"
#include "memorymonitor.h"
#include "ex_exe_stmt_globals.h"
#include "ex_esp_frag_dir.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_split_bottom.h"
#include "ex_send_bottom.h"
#include "NAExit.h"
#include "ExSqlComp.h"
#include "Globals.h"
#include "Int64.h"
#include "SqlStats.h"
#include "ComUser.h"
#include "ExpError.h"
#include "ComSqlId.h"
#include "ex_globals.h"
#include "ex_tcb.h"
#include "ExExeUtil.h"
#include "Globals.h"
#include "Context.h"
Int32 extractLobHandle(CliGlobals *cliglob, char *& lobHandle, 
		       char *lobColumnName, char *tableName);

Int32 extractLengthOfLobColumn(CliGlobals *cliglob, char * lobHandle, Int64 &lengthOfLob,char *lobColumnName, char *tableName);

Int32 extractLobToBuffer(CliGlobals *cliglob, char * lobHandle, Int64 &lengthOfLob, 
			 char *lobColumnName, char *tableName);
Int32 extractLobToFileInChunks(CliGlobals *cliglob, char * lobHandle, char *filename, Int64 &lengthOfLob, 
			 char *lobColumnName, char *tableName);
Int32 insertBufferToLob(CliGlobals *cliglob,char *tbaleName);
Int32 updateBufferToLob(CliGlobals *cliglob, char *tableName, char *columnName);
Int32 updateAppendBufferToLob(CliGlobals *cliGlob, char *tableName, char *columnName);
