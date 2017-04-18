/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
****************************************************************************
*
* File:         clitest.cpp
* Description:  Test driver useing exe util cli interface
*
*
*
*
****************************************************************************
*/
#include "Platform.h"


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
#include "blobtest.h"

//DEFINE_DOVERS(clitestdriver)
// 
int main(int argc, const char * argv[])
{

  if ((argc < 2) || (argc > 2))
    {
      cout << "Error -  provide an option: " << endl;
      cout << "Usage : clitestdriver <option number>" << endl;
      cout << "TEST                                OPTION        "<<endl;
      cout << "-------------------------------------------"<<endl;
      cout << "Blob test extract to buffer         1      "<< endl;
      cout << "Blob test extract to file in chunks 2      "<< endl;
      cout << "Blob test to insert to lob column   3     "<< endl;
      cout << "Blob test to update lob column      4     "<< endl;
      cout << "Blob test to append to lob column   5      "<< endl;
      cout << "Blob test to update to lob lobhandle   6   "<< endl;
      cout << "Blob test to append to lobhandle   7      "<< endl;
      cout << "Blob test to truncate to lobhandle   8      "<< endl;
      cout << "Blob test append  lobhandle from a buffer in chunks from file 9 " << endl;
      return 0;
    }
  Int32 retcode = 0;
  SQLCTX_HANDLE defContext = 0;
  Lng32 retCode = SQL_EXEC_CreateContext(&defContext, NULL, 0);
 
  int option = atoi(argv[1]);

  if(retcode != 0 )
    cout << "Error creating a CLI context error " << endl;
   
  
  CliGlobals * cliGlob = GetCliGlobals();
  char tablename[50] = {'\0'};
  char columnname[50] = {'\0'};
  char filename[50] = {'\0'};
  switch (option) 
    {
    case 1:
      {
	cout <<"***********"  <<endl;
	cout << "   Blob test extract to user buffer " << endl;
	cout <<"***********"  <<endl;
	cout << "Extract from a lob column in a lob table" << endl << endl;
	cout << "Input lob table name :" << endl;
	cin.getline(tablename,40);
	cout << "Table name : " << tablename << endl;
	cout << "Input lob column name to extract from :" << endl;
	cin.getline(columnname,40); 
	cout << "Column Name : " << columnname << endl;

      //extract lob handle
      
      char *lobHandle = new char[1024];
      str_cpy_all(lobHandle," ",1024);
      cout << "Extracting  lob handle for column " << columnname << "..." << endl;
      retcode = extractLobHandle(cliGlob, lobHandle, (char *)columnname,(char *)tablename);
      if (retcode)
	{
	  cout << "extractLobHandle returned " << retcode <<endl;
	
	  delete lobHandle;
	  return retcode;
	}
      cout << "LOB handle for "<< columnname << ": " << lobHandle << endl;
      //extract length of lob column from a table with 1 lob column.

      cout << "Extracting LOB data length for the above handle..." << endl;
      Int64 lengthOfLob= 0;
      retCode = extractLengthOfLobColumn(cliGlob, lobHandle, lengthOfLob,columnname,(char *)tablename);
      if (retcode)
	{
	  cout << "extractLengthOfLobColumn returned " << retcode <<endl;
	 
	  delete lobHandle;
	  return retcode;
	}
      cout << "LOB data length :" << lengthOfLob << endl;
      //extract lob data to buffer
      cout << "Extracting lob data into user buffer in a loop ..." << endl;
      retCode = extractLobToBuffer(cliGlob,lobHandle, lengthOfLob,columnname,tablename);
      if (retcode)
	{
	  cout << "extractLobToBuffer returned " << retcode <<endl;

	  delete lobHandle;
	  return retcode;
	}
      delete lobHandle;
      return retcode;
      }
     
      break;
    
     case 2:
      {
	cout <<"*************************************"  <<endl;
	cout << "Blob test extract to file in chunks " << endl;
	cout <<"************************************"  <<endl;
	cout << "Extract from a lob column in a lob table" << endl << endl;
	cout << "Input lob table name :" << endl;
	cin.getline(tablename,40);
	cout << "Table name : " << tablename << endl;
	cout << "Input lob column name to extract from :" << endl;
	cin.getline(columnname,40); 
	cout << "Column Name : " << columnname << endl;
	cout <<"Input a filename to extract to : " << endl;
	cin.getline(filename,40); 
	cout << "Output File Name : " << filename << endl;

      //extract lob handle
      
      char *lobHandle = new char[1024];
      str_cpy_all(lobHandle," ",1024);
      cout << "Extracting  lob handle for column " << columnname << "..." << endl;
      retcode = extractLobHandle(cliGlob, lobHandle, (char *)columnname,tablename);
      if (retcode)
	{
	  cout << "extractLobHandle returned " << retcode <<endl;
	
	  delete lobHandle;
	  return retcode;
	}
      cout << "LOB handle for "<< columnname << ": " << lobHandle << endl;
      //extract length of lob column from a table with 1 lob column.

      cout << "Extracting LOB data length for the above handle..." << endl;
      Int64 lengthOfLob= 0;
      retCode = extractLengthOfLobColumn(cliGlob, lobHandle, lengthOfLob,columnname,tablename);
      if (retcode)
	{
	  cout << "extractLengthOfLobColumn returned " << retcode <<endl;
	 
	  delete lobHandle;
	  return retcode;
	}
      cout << "LOB data length :" << lengthOfLob << endl;
      //extract lob data to buffer
      cout << "Extracting lob data into file in chunks of 1000 ..." << endl;
      retCode = extractLobToFileInChunks(cliGlob,lobHandle, filename, lengthOfLob,columnname,tablename);
      if (retcode)
	{
	  cout << "extractLobToFileInChunks returned " << retcode <<endl;

	  delete lobHandle;
	  return retcode;
	}
      delete lobHandle;
      return retcode;
      }
     
      break;
    
    case 3:
      {
	
	cout <<"*************************************"  <<endl;
	cout << "Blob test insert lobdata from a buffer " << endl;
	cout << "Input lob table name (1st int column, 2nd blob column:" << endl;
	cin.getline(tablename,40);
	cout << "Table name : " << tablename << endl;
	retcode = insertBufferToLob(cliGlob,tablename);
	return retcode;
      }
      break;
    case 4:
      {
	
	cout <<"*************************************"  <<endl;
	cout << "Blob test update lobdata from a buffer " << endl;
	cout << "Input lob table name :" << endl;
	cin.getline(tablename,40);
	cout << "Table name (1st int column ,2nd blob column): " << tablename << endl;
	cout << "Input lob column name to update :" << endl;
	cin.getline(columnname,40); 
	cout << "Column Name : " << columnname << endl;
	retcode = updateBufferToLob(cliGlob,tablename,columnname);
	return retcode;
      }
      break;
    case 5:
      {
	
	cout <<"*************************************"  <<endl;
	cout << "Blob test update append lobdata from a buffer " << endl;
	cout << "Input lob table name :" << endl;
	cin.getline(tablename,40);
	cout << "Table name (1st int column , 2nd blob column: " << tablename << endl;
	cout << "Input lob column name to update :" << endl;
	cin.getline(columnname,40); 
	cout << "Column Name : " << columnname << endl;
	retcode = updateAppendBufferToLob(cliGlob,tablename,columnname);
	return retcode;
      }
      break;
    case 6:
      {
	
	cout <<"*************************************"  <<endl;
	cout << "Blob test update  lobhandle from a buffer " << endl;
	cout << "Input lob table name :" << endl;
	cin.getline(tablename,40);
	cout << "Table name : " << tablename << endl;
	cout << "Input lob column name to get handle from :" << endl;
	cin.getline(columnname,40); 
	cout << "Column Name : " << columnname << endl;

        //extract lob handle
      
        char *lobHandle = new char[1024];
        str_cpy_all(lobHandle," ",1024);
        cout << "Extracting  lob handle for column " << columnname << "..." << endl;
        retcode = extractLobHandle(cliGlob, lobHandle, (char *)columnname,tablename);
        if (retcode)
          {
            cout << "extractLobHandle returned " << retcode <<endl;
	
            delete lobHandle;
	  return retcode;
          }
        cout << "LOB handle for "<< columnname << ": " << lobHandle << endl;
	retcode = updateBufferToLobHandle(cliGlob,lobHandle);
	return retcode;
      }
      break;
case 7:
      {
	
	cout <<"*************************************"  <<endl;
	cout << "Blob test append  lobhandle from a buffer " << endl;
	cout << "Input lob table name :" << endl;
	cin.getline(tablename,40);
	cout << "Table name : " << tablename << endl;
	cout << "Input lob column name to get handle from :" << endl;
	cin.getline(columnname,40); 
	cout << "Column Name : " << columnname << endl;

        //extract lob handle
      
        char *lobHandle = new char[1024];
        str_cpy_all(lobHandle," ",1024);
        cout << "Extracting  lob handle for column " << columnname << "..." << endl;
        retcode = extractLobHandle(cliGlob, lobHandle, (char *)columnname,tablename);
        if (retcode)
          {
            cout << "extractLobHandle returned " << retcode <<endl;
	
            delete lobHandle;
	  return retcode;
          }
        cout << "LOB handle for "<< columnname << ": " << lobHandle << endl;
        char *source= new char[300];
        strcpy(source,"zzzzzzzzzzzzzzzzzzzz");
	retcode = updateAppendBufferToLobHandle(cliGlob,lobHandle,20,(Int64)source);
	return retcode;
      }
      break;
case 8:
      {
	
	cout <<"*************************************"  <<endl;
	cout << "Blob test empty  lobhandle from a buffer " << endl;
	cout << "Input lob table name :" << endl;
	cin.getline(tablename,40);
	cout << "Table name : " << tablename << endl;
	cout << "Input lob column name to get handle from :" << endl;
	cin.getline(columnname,40); 
	cout << "Column Name : " << columnname << endl;

        //extract lob handle
      
        char *lobHandle = new char[1024];
        str_cpy_all(lobHandle," ",1024);
        cout << "Extracting  lob handle for column " << columnname << "..." << endl;
        retcode = extractLobHandle(cliGlob, lobHandle, (char *)columnname,tablename);
        if (retcode)
          {
            cout << "extractLobHandle returned " << retcode <<endl;
	
            delete lobHandle;
	  return retcode;
          }
        cout << "LOB handle for "<< columnname << ": " << lobHandle << endl;
	retcode = updateTruncateLobHandle(cliGlob,lobHandle);
	return retcode;
      }
      break;

case 9:
      {
	
	cout <<"*************************************"  <<endl;
	cout << "Blob test append  lobhandle from a buffer from file " << endl;
	cout << "Input lob table name :" << endl;
	cin.getline(tablename,40);
	cout << "Table name : " << tablename << endl;
	cout << "Input lob column name to get handle from :" << endl;
	cin.getline(columnname,40); 
	cout << "Column Name : " << columnname << endl;
        cout << "Input source file name :" << endl;
	cin.getline(filename,512);
	cout << "Source name : " << filename << endl;
        //extract lob handle
      
        char *lobHandle = new char[1024];
        str_cpy_all(lobHandle," ",1024);
        cout << "Extracting  lob handle for column " << columnname << "..." << endl;
        retcode = extractLobHandle(cliGlob, lobHandle, (char *)columnname,tablename);
        if (retcode)
          {
            cout << "extractLobHandle returned " << retcode <<endl;
	
            delete lobHandle;
	  return retcode;
          }
        cout << "LOB handle for "<< columnname << ": " << lobHandle << endl;


        //read from input source file in chunks and append
        int fdSourceFile = open(filename,O_RDONLY);
        struct stat statbuf;
        if (stat(filename, &statbuf) != 0) {
          return -1;
       }

        Int64 sourceEOF = statbuf.st_size;
        char *fileInputData= new char[32*1024*1024]; //32 KB chunks
        //char *fileInputData = new char[10000];
        Int64 lengthRead=0;
        Int64 offset=0;
        Int64 chunkLen = 32*1024*1024;
        //Int64 chunkLen = 10000;

        while ((offset < sourceEOF) )
          {
            if (lengthRead=pread(fdSourceFile, fileInputData, chunkLen, offset) == -1) {
              close(fdSourceFile);
              return -1;
            }
            chunkLen = min(chunkLen,sourceEOF-offset);
            retcode = updateAppendBufferToLobHandle(cliGlob,lobHandle,chunkLen,(Int64)fileInputData);
            offset +=chunkLen;
          }
	return retcode;
      }
      break;
    }//end switch
  
  return 0;
  
}


