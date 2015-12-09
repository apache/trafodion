#include "blobtest.h"

Int32 extractLengthOfBlobColumn(CliGlobals *cliglob, Int64 &lengthOfLob, 
				char *lobColumnName, char *tableName)
{
  Int32 retcode = 0;
  ExeCliInterface cliInterface((cliglob->currContext())->exHeap(), (Int32)SQLCHARSETCODE_UTF8, cliglob->currContext(),NULL);
  char * query = new char[4096];
  char *lobHandle = new char[1024];
  str_cpy_all(lobHandle," ",1024);
  Int32 lobHandleLen = 0;
  str_sprintf(query,"select %s from %s",lobColumnName,tableName);
  
  retcode = cliInterface.executeImmediate(query,lobHandle,&lobHandleLen,FALSE);

  if (retcode)
    return retcode;
  lobHandle[lobHandleLen]='\0';

  //Use lob handle to retrieve the lob length.
  char lobLengthResult[200];
  str_cpy_all(lobLengthResult," ",200);
  Int32 lobLengthResultLen = 0;
  str_sprintf(query,"extract loblength (lob '%s')",lobHandle);
  retcode = cliInterface.executeImmediate(query,lobLengthResult,&lobLengthResultLen,FALSE);

  // Extract lob data into a buffer.
  char lobBuf[200];
  str_cpy_all(lobBuf," ",200);
  char lobBufStatus[200];
  str_cpy_all(lobBufStatus," ",200);
  Int32 lobBufStatusLen = 0;

  str_sprintf(query,"extract lobtobuffer(lob '%s', LOCATION (Int64)lobBuf, SIZE 4 ", lobHandle);
  retcode = cliInterface.executeImmediate(query,lobBufStatus,&lobBufStatusLen,FALSE);

  return retcode;
}
