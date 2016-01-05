#include "blobtest.h"

Int32 extractLengthOfLobColumn(CliGlobals *cliglob, char *lobHandle, 
			       Int64 &lengthOfLob, 
			       char *lobColumnName, char *tableName)
{
  Int32 retcode = 0;
  char * query = new char[4096];
  ExeCliInterface cliInterface((cliglob->currContext())->exHeap(), (Int32)SQLCHARSETCODE_UTF8, cliglob->currContext(),NULL);
  //Use lob handle to retrieve the lob length.
  char lobLengthResult[200];
  str_cpy_all(lobLengthResult," ",200);
  Int32 lobLengthResultLen = 0;
  str_sprintf(query,"extract loblength (lob '%s') LOCATION %Ld ",lobHandle, &lengthOfLob);
  retcode = cliInterface.executeImmediate(query,lobLengthResult,&lobLengthResultLen,FALSE);

  delete query;
  return retcode;
 
}

Int32 extractLobHandle(CliGlobals *cliglob, char *& lobHandle, 
		       char *lobColumnName, char *tableName)
{
  Int32 retcode = 0;
  ExeCliInterface cliInterface((cliglob->currContext())->exHeap(), (Int32)SQLCHARSETCODE_UTF8, cliglob->currContext(),NULL);
  char * query = new char[4096];
  Int32 lobHandleLen = 0;
  str_sprintf(query,"select %s from %s",lobColumnName,tableName);
  
  retcode = cliInterface.executeImmediate(query,lobHandle,&lobHandleLen,FALSE);

  if (retcode)
    return retcode;
  lobHandle[lobHandleLen]='\0';
  delete query;
  
  return retcode;
 
}

Int32 extractLobToBuffer(CliGlobals *cliglob, char * lobHandle, Int64 &lengthOfLob, 
				char *lobColumnName, char *tableName)
{
  Int32 retcode = 0;
  ExeCliInterface cliInterface((cliglob->currContext())->exHeap(), (Int32)SQLCHARSETCODE_UTF8, cliglob->currContext(),NULL);
  // Extract lob data into a buffer.
  char * query = new char [500];
  
  char *lobFinalBuf = new char[lengthOfLob];
  char statusBuf[200] = {'\0'};
  Int32 statusBufLen = 0;
  Int64 lobExtractLen = 1000;
  char *lobDataBuf = new char[lobExtractLen];
  
  str_sprintf(query,"extract lobtobuffer(lob '%s', LOCATION %Ld, SIZE %Ld) ", lobHandle, (Int64)lobDataBuf, &lobExtractLen);
 
 
  retcode = cliInterface.executeImmediatePrepare(query);
  short i = 0;
  while ((retcode != 100) && !(retcode<0))
    {    
      retcode = cliInterface.clearExecFetchClose(NULL,NULL,statusBuf, &statusBufLen);
      if (!retcode)
	{
	memcpy((char*)&(lobFinalBuf[i]),(char *)lobDataBuf,lobExtractLen);
	i += lobExtractLen;
	}
    }
  if (retcode ==100 || retcode ==0)
    {
      FILE * lobFileId = fopen("lob_output_file","w");
  
      int byteCount=fwrite(lobFinalBuf,sizeof(char),lengthOfLob, lobFileId);
      cout << "Writing " << byteCount << " bytes from user buffer to file lob_output_file in current directory" << endl;

      fclose(lobFileId);
    }
  delete  lobFinalBuf;
  delete query;
  delete lobDataBuf;
    

  return retcode;

}


Int32 extractLobToFileInChunks(CliGlobals *cliglob,  char * lobHandle, char *filename,Int64 &lengthOfLob, 
				char *lobColumnName, char *tableName)
{
  Int32 retcode = 0;
  ExeCliInterface cliInterface((cliglob->currContext())->exHeap(), (Int32)SQLCHARSETCODE_UTF8, cliglob->currContext(),NULL);
  // Extract lob data into a buffer.
  char * query = new char [500];
  
  
  char statusBuf[200] = {'\0'};
  Int32 statusBufLen = 0;
  Int64 lobExtractLen = 1000;
  char *lobDataBuf = new char[lobExtractLen];
  Int64 *inputOutputAddr = &lobExtractLen;

  str_sprintf(query,"extract lobtobuffer(lob '%s', LOCATION %Ld, SIZE %Ld) ", lobHandle, (Int64)lobDataBuf, inputOutputAddr);
 
 
  retcode = cliInterface.executeImmediatePrepare(query);
  short i = 0;
  FILE * lobFileId = fopen(filename,"a+");
  int byteCount = 0;
  while ((retcode != 100) && !(retcode<0))
    {    
      retcode = cliInterface.clearExecFetchClose(NULL,NULL,statusBuf, &statusBufLen);
      if (!retcode)
	{
	  byteCount=fwrite(lobDataBuf,sizeof(char),*inputOutputAddr, lobFileId);
	 cout << "Wrote " << byteCount << " bytes to file : " << filename << endl;
	}
    }
  

  fclose(lobFileId);

 
  delete query;
  delete lobDataBuf;
    

  return retcode;

}


Int32 insertBufferToLob(CliGlobals *cliglob, char *tableName)
{
  Int32 retcode = 0;
  ExeCliInterface cliInterface((cliglob->currContext())->exHeap(), (Int32)SQLCHARSETCODE_UTF8, cliglob->currContext(),NULL);
  // Extract lob data into a buffer.
  char * query = new char [500];
  
 
  char statusBuf[200] = {'\0'};
  Int32 statusBufLen = 0;
  Int64 lobInsertLen = 10;
  char *lobDataBuf = new char[lobInsertLen];
  memcpy(lobDataBuf, "xxxxxyyyyy",10);
  str_sprintf(query,"insert into %s values (1, buffertolob (LOCATION %Ld, SIZE %Ld))", tableName,(Int64)lobDataBuf, lobInsertLen);
 
 
  retcode = cliInterface.executeImmediate(query);
  if (retcode <0)
    return retcode;

  retcode = cliInterface.executeImmediate("commit work");
  delete query;
  delete lobDataBuf;
    

  return retcode;

}


Int32 updateBufferToLob(CliGlobals *cliglob, char *tableName, char *columnName)
{
  Int32 retcode = 0;
  ExeCliInterface cliInterface((cliglob->currContext())->exHeap(), (Int32)SQLCHARSETCODE_UTF8, cliglob->currContext(),NULL);
  // Extract lob data into a buffer.
  char * query = new char [500];
  
 
  char statusBuf[200] = {'\0'};
  Int32 statusBufLen = 0;
  Int64 lobUpdateLen = 10;
  char *lobDataBuf = new char[lobUpdateLen];
  memcpy(lobDataBuf, "zzzzzzzzzzzzzzzzzzzz",20);
  str_sprintf(query,"update %s set %s= buffertolob(LOCATION %Ld, SIZE %Ld)", tableName,columnName, (Int64)lobDataBuf, lobUpdateLen);
 
 
  retcode = cliInterface.executeImmediate(query);
  if (retcode <0)
    return retcode;

  retcode = cliInterface.executeImmediate("commit work");
  delete query;
  delete lobDataBuf;
    

  return retcode;

}



Int32 updateAppendBufferToLob(CliGlobals *cliglob, char *tableName, char *columnName)
{
  Int32 retcode = 0;
  ExeCliInterface cliInterface((cliglob->currContext())->exHeap(), (Int32)SQLCHARSETCODE_UTF8, cliglob->currContext(),NULL);
  // Extract lob data into a buffer.
  char * query = new char [500];
  
 
  char statusBuf[200] = {'\0'};
  Int32 statusBufLen = 0;
  Int64 lobUpdateLen = 10;
  char *lobDataBuf = new char[lobUpdateLen];
  memcpy(lobDataBuf, "aaaaabbbbbccccc",15);
  str_sprintf(query,"update %s set %s=buffertolob (LOCATION %Ld, SIZE %Ld,append)", tableName, columnName,(Int64)lobDataBuf, lobUpdateLen);
 
 
  retcode = cliInterface.executeImmediate(query);
  if (retcode <0)
    return retcode;

  retcode = cliInterface.executeImmediate("commit work");
  delete query;
  delete lobDataBuf;
    

  return retcode;

}
