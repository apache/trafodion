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

#define _SPUTIL_SP_DLL_
#include "CmpISPStd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <iostream>


SP_STATUS SP_FILEINFO_Compile (SP_COMPILE_ACTION /* action */,
			      SP_COMPILE_HANDLE* /* compHandle */,
			      SP_HANDLE /* spHandle */,
			      SP_ERROR_STRUCT* /* error */ )
{
  return SP_SUCCESS;
}

SP_STATUS SP_FILEINFO_InputFormat ( SP_FIELDDESC_STRUCT* format ,
				   Lng32 numFields,
				   SP_COMPILE_HANDLE,
				   SP_HANDLE /* spHandle */,
				   SP_ERROR_STRUCT* error )
{
  if ( numFields != 4 )
    {
      error->error = -19020;
      strcpy(error->optionalString[0], "SP2");
      error->optionalInteger[0] = 4;
      return SP_FAIL;
    }
  strcpy(&(format++->COLUMN_DEF[0]), "filename char(9) character set iso88591");
  strcpy(&(format++->COLUMN_DEF[0]), "counter INT");
  strcpy(&(format++->COLUMN_DEF[0]), "descpt varchar(64) character set iso88591");
  strcpy(&(format++->COLUMN_DEF[0]), "dt1 timestamp");

  return SP_SUCCESS;
}


SP_STATUS SP_FILEINFO_NumOutputs ( Lng32* num,
				  SP_COMPILE_HANDLE,
				  SP_HANDLE /* spHandle */,
				  SP_ERROR_STRUCT* /* error */)
{
  *num = 3;
  return SP_SUCCESS;
}

SP_STATUS SP_FILEINFO_OutputFormat (SP_FIELDDESC_STRUCT* format ,
				   SP_KEYDESC_STRUCT*  keyFields ,
				   Lng32*  numKeyFields ,
				   SP_COMPILE_HANDLE,
				   SP_HANDLE /* spHandle */,
				   SP_ERROR_STRUCT* /* error */  )
{
  strcpy(&((format++)->COLUMN_DEF[0]), "DATA varchar(128) character set iso88591");
  strcpy(&((format++)->COLUMN_DEF[0]), "descpt char(64) character set iso88591");
  strcpy(&((format++)->COLUMN_DEF[0]), "dt timestamp");


  return SP_SUCCESS;
}

SP_STATUS SP_FILEINFO_Process(SP_PROCESS_ACTION action,
			     SP_ROW_DATA  inputData ,
			     SP_EXTRACT_FUNCPTR  eFunc ,
			     SP_ROW_DATA outputData ,
			     SP_FORMAT_FUNCPTR fFunc,
			     SP_KEY_VALUE,
			     SP_KEYVALUE_FUNCPTR,
			     SP_PROCESS_HANDLE*  spProcHandle,
			     SP_HANDLE /* spHandle */,
			     SP_ERROR_STRUCT*  error
			     )
{
  struct SP_FILEINFO_STRUCT
  {
    FILE* fd;
    char desc[256];
    char dt1[64];
  };

  SP_FILEINFO_STRUCT* fileInfo = 0;
  if (action == SP_PROC_OPEN)
  {
    SP_STATUS spStatus = SP_SUCCESS;
    *spProcHandle = 0;
    FILE* fd=0;
    char fileName[256];
    Int32 counter;
    char desc[66];
    char tempStr[64];
    char dt1[64];
    if ( eFunc(0, inputData, 10, tempStr, 0) != SP_NO_ERROR )
      return SP_FAIL;
    else
      // int as input parameter
      if ( eFunc(1, inputData, 4, &counter, 0) != SP_NO_ERROR )
        return SP_FAIL;
      else
        if (eFunc(2, inputData, 66, desc, 0) != SP_NO_ERROR )
          return SP_FAIL;
        else
          if (eFunc(3, inputData, 64, dt1, 0) != SP_NO_ERROR )
            return SP_FAIL;
          else
          {

            // char as input parameter
            tempStr[9] = '\0';
            sprintf(fileName, "%s%d", tempStr, counter);
            if ( !(fd = fopen(fileName,"r") )  )
            {
              error->error = -19021;
              strcpy(error->optionalString[0], "FILEINFO");
              sprintf(error->optionalString[1], "%s does not exist", fileName);
              spStatus = SP_FAIL;
            }
            fileInfo = new SP_FILEINFO_STRUCT;
            fileInfo->fd = fd;

            // varchar as input parameter
            memset(fileInfo->desc, ' ', 255);
            short len;
            memcpy(&len, &desc[0], sizeof(len));
            memcpy(fileInfo->desc, &desc[2], len);
            fileInfo->desc[len] = 0;

            // datetime as input parameter
            memcpy(fileInfo->dt1, dt1, 64);
          }

    *spProcHandle =(SP_PROCESS_HANDLE) fileInfo;
    return spStatus;
  }
  if (action == SP_PROC_FETCH)
  {
    SP_FILEINFO_STRUCT* fileInfo = (SP_FILEINFO_STRUCT*) (*spProcHandle);
    FILE* fd = fileInfo->fd;

    // varchar as output
    char tempstr[128];
    char* p = &tempstr[2];
    if ( fd && fgets(p, 126, fd) )
    {
      char* nlptr = strchr(p, '\n');
      if (nlptr) *nlptr = '\0';
      short len = strlen(p);
      memcpy(&tempstr[0], &len, sizeof(len));
      fFunc(0, outputData, strlen(p) + 2, tempstr, 0);

      // error when fetching output
      if (strncmp(&tempstr[2], "errors", 6) == 0)
      {
        error->error = -19021;
        strcpy(error->optionalString[0], "FILEINFO");
        strcpy(error->optionalString[1], "found an error token");
        (error+1)->error = -19021;
        strcpy((error+1)->optionalString[0], "FILEINFO");
        strcpy((error+1)->optionalString[1], "found an error token");
        (error+2)->error = -19021;
        strcpy((error+2)->optionalString[0], "FILEINFO");
        strcpy((error+2)->optionalString[1], "found an error token");
        return SP_FAIL;
      }
      else if (strncmp(&tempstr[2], "error", 5) == 0)
      {
        error->error = -19021;
        strcpy(error->optionalString[0], "FILEINFO");
        strcpy(error->optionalString[1], "found an error token");
        return SP_FAIL;
      }
      // char as output
      fFunc(1, outputData, strlen(fileInfo->desc), fileInfo->desc, 0);

      // datetime as output
      fFunc(2, outputData, 64, fileInfo->dt1, 0);

      return SP_MOREDATA;
    }
    else
      return SP_SUCCESS;
  }
  if (action == SP_PROC_CLOSE)
  {
    SP_FILEINFO_STRUCT* fileInfo = (SP_FILEINFO_STRUCT*)(*spProcHandle);
    FILE* fd = fileInfo->fd;
    if (fd)
    {
      if ( fclose(fd)  )
      {
        error->error = -19021;
        strcpy(error->optionalString[0], "FILEINFO");
	strcpy(error->optionalString[1], "Can not close file");
	return SP_FAIL;
      }
    }
    else
    {
      error->error = -19021;
      strcpy(error->optionalString[0], "FILEINFO");
      strcpy(error->optionalString[1], "no file opened ");
      return SP_FAIL;
    }
    return SP_SUCCESS;
  }

  return SP_SUCCESS;
}

SP_STATUS SP_CASTING_Compile (SP_COMPILE_ACTION /* action */,
			      SP_COMPILE_HANDLE* /* compHandle */,
			      SP_HANDLE /* spHandle */,
			      SP_ERROR_STRUCT* /* error */ )
{
  return SP_SUCCESS;
}

SP_STATUS SP_CASTING_InputFormat ( SP_FIELDDESC_STRUCT* format ,
				   Lng32 numFields,
				   SP_COMPILE_HANDLE,
				   SP_HANDLE /* spHandle */,
				   SP_ERROR_STRUCT* error )
{
  if ( numFields != 4 )
    {
      error->error = -19020;
      strcpy(error->optionalString[0], "CASTING");
      error->optionalInteger[0] = 4;
      return SP_FAIL;
    }
  strcpy(&((format++)->COLUMN_DEF[0]), "I1 smallint");
  strcpy(&((format++)->COLUMN_DEF[0]), "d1file char(16) character set iso88591 ");
  strcpy(&((format++)->COLUMN_DEF[0]), "description varchar(20) character set iso88591 ");
  strcpy(&((format++)->COLUMN_DEF[0]), "dt timestamp" );

  return SP_SUCCESS;
}


SP_STATUS SP_CASTING_NumOutputs ( Lng32* num,
				  SP_COMPILE_HANDLE,
				  SP_HANDLE /* spHandle */,
				  SP_ERROR_STRUCT* /* error */)
{
  *num = 4;
  return SP_SUCCESS;
}

SP_STATUS SP_CASTING_OutputFormat (SP_FIELDDESC_STRUCT* format ,
				   SP_KEYDESC_STRUCT*  keyFields ,
				   Lng32*  numKeyFields ,
				   SP_COMPILE_HANDLE,
				   SP_HANDLE /* spHandle */,
				   SP_ERROR_STRUCT* /* error */  )
{
  strcpy(&((format++)->COLUMN_DEF[0]), "DATA int");
  strcpy(&((format++)->COLUMN_DEF[0]), "datetime1 timestamp");
  strcpy(&((format++)->COLUMN_DEF[0]), "description varchar(256) character set iso88591");
  strcpy(&((format++)->COLUMN_DEF[0]), "tag char(64) character set iso88591");

  return SP_SUCCESS;
}

inline void varchar2String(char* varchar, short size, char*& string, short& len)
{
  // This inline will convert the sql varchar format string into a
  // C/C++ style string ( null terminated if enough length )
  memcpy(&len, varchar, sizeof(short));
  string = &varchar[sizeof(short)];
  if ((UInt32)len + sizeof(short) < (UInt32)size )
    string[len] = 0;
}

inline short string2Varchar(char* string, char* varchar, short size)
{
  if ((UInt32)strlen(string) + sizeof(short) > (UInt32)size )
    return 0;
  short len = strlen(string);
  memcpy(varchar, &len, sizeof(short));
  memcpy(varchar+sizeof(short), string, len);
  return 1;
}

SP_STATUS SP_CASTING_Process(SP_PROCESS_ACTION action,
			     SP_ROW_DATA  inputData ,
			     SP_EXTRACT_FUNCPTR  eFunc ,
			     SP_ROW_DATA outputData ,
			     SP_FORMAT_FUNCPTR fFunc,
			     SP_KEY_VALUE,
			     SP_KEYVALUE_FUNCPTR,
			     SP_PROCESS_HANDLE*  spProcHandle,
			     SP_HANDLE /* spHandle */,
			     SP_ERROR_STRUCT*  error
			     )
{
  struct CounterStruct
  {
    FILE* fd;
    Int32 max;
    Int32 current;
    char desc[256];
    char tag[40];
  };

  if (action == SP_PROC_OPEN)
  {
    SP_STATUS spStatus = SP_SUCCESS;
    char counter[16];
    memset(counter, ' ', 15);
    counter[15] = 0;
    short len;
    if ( eFunc(0, inputData, 16, counter, 1) == SP_NO_ERROR )
    {
      char* cp;
      varchar2String(counter, 16, cp, len);
      Int32 iCounter = atoi(cp);
      CounterStruct* counterStruct = new CounterStruct();
      counterStruct->max = iCounter;
      counterStruct->current = 0;
      char fileName[64];
      if (eFunc(1, inputData, 64, fileName, 1) == SP_NO_ERROR)
      {
        char* fp;
        varchar2String(fileName, 64, fp, len);
        if ( !(counterStruct->fd = fopen(fp,"r") )  )
        {
          error->error = -19021;
          strcpy(error->optionalString[0], "CASTING");
          sprintf(error->optionalString[1], "%s does not exist", fileName);
          delete counterStruct;
          return SP_FAIL;
        }
        char tempStr[256];
        if ( eFunc(2, inputData, 256, tempStr, 1) != SP_NO_ERROR)
        {
          delete counterStruct;
          return SP_FAIL;
		}
        char* tp;
        varchar2String(tempStr, 256, tp, len);
        strcpy(counterStruct->desc, tp);
        if ( eFunc(3, inputData, 40, tempStr, 1) != SP_NO_ERROR)
        {
          error->error = -19021;
          strcpy(error->optionalString[0], "CASTING");
          strcpy(error->optionalString[1], "can't get the 4th parameter");
		  delete counterStruct;
          return SP_FAIL;
        }
        varchar2String(tempStr, 40, tp, len);
        strcpy(counterStruct->tag, tp);
        *spProcHandle =(SP_PROCESS_HANDLE) counterStruct;
      }
      else
	  {
	    delete counterStruct;
              return SP_FAIL;

	  }
	 delete counterStruct;
      return spStatus;
    }
    else
      return SP_FAIL;
  }
  if (action == SP_PROC_FETCH)
  {
    char string[256] = "";
    CounterStruct* counter = (CounterStruct *)(*spProcHandle);
    if ( counter )
    {
      if ( counter->current >= counter->max ||
        (counter->fd && !fgets(string, 256, counter->fd) ) )
        return SP_SUCCESS;
      else
      {
        char* lnPtr = strchr(string, '\n');
        if (lnPtr) *lnPtr='\0';
        char tempStr[1024];
        char id[64];
        sprintf(id, "%d", counter->current++);
        string2Varchar(id, tempStr, 1024);
        fFunc(0, outputData, 1024, tempStr, 1);
        string2Varchar(string, tempStr, 1024);
        fFunc(1, outputData, 1024, tempStr, 1);
        string2Varchar(counter->desc, tempStr, 1024);
        fFunc(2, outputData, 1024, tempStr, 1);
        string2Varchar(counter->tag, tempStr, 1024);
        fFunc(3, outputData, 1024, tempStr, 1);
        return SP_MOREDATA;
      }
    }
    else
    {
      error->error = -19021;
      strcpy(error->optionalString[0], "CASTING");
      strcpy(error->optionalString[1], "Not a valid CounterStruct*");
      return SP_FAIL;
    }
  }
  if (action == SP_PROC_CLOSE)
  {
    CounterStruct* counter = (CounterStruct *) (*spProcHandle);
    if ( counter->fd )
      fclose(counter->fd);
    delete counter;
    return SP_SUCCESS;
  }

  return SP_SUCCESS;
}

SP_STATUS SP_NOINPUT_Compile (SP_COMPILE_ACTION /* action */,
			      SP_COMPILE_HANDLE* /* compHandle */,
			      SP_HANDLE /* spHandle */,
			      SP_ERROR_STRUCT* /* error */ )
{
  return SP_SUCCESS;
}

SP_STATUS SP_NOINPUT_InputFormat ( SP_FIELDDESC_STRUCT* format ,
				   Lng32 numFields,
				   SP_COMPILE_HANDLE,
				   SP_HANDLE /* spHandle */,
				   SP_ERROR_STRUCT* error )
{
  if ( numFields != 0 )
    {
      error->error = -19020;
      strcpy(error->optionalString[0], "CASTING");
      error->optionalInteger[0] = 0;
      return SP_FAIL;
    }

  return SP_SUCCESS;
}


SP_STATUS SP_NOINPUT_NumOutputs ( Lng32* num,
				  SP_COMPILE_HANDLE,
				  SP_HANDLE /* spHandle */,
				  SP_ERROR_STRUCT* /* error */)
{
  *num = 0;
  return SP_SUCCESS;
}

SP_STATUS SP_NOINPUT_OutputFormat (SP_FIELDDESC_STRUCT* format ,
				   SP_KEYDESC_STRUCT*  /*keyFields */ ,
				   Lng32*  /*numKeyFields */ ,
				   SP_COMPILE_HANDLE,
				   SP_HANDLE /* spHandle */,
				   SP_ERROR_STRUCT* /* error */  )
{
  return SP_SUCCESS;
}

SP_STATUS SP_NOINPUT_Process(SP_PROCESS_ACTION action,
			     SP_ROW_DATA  inputData ,
			     SP_EXTRACT_FUNCPTR  eFunc ,
			     SP_ROW_DATA outputData ,
			     SP_FORMAT_FUNCPTR fFunc,
			     SP_KEY_VALUE,
			     SP_KEYVALUE_FUNCPTR,
			     SP_PROCESS_HANDLE*  spProcHandle,
			     SP_HANDLE /* spHandle */,
			     SP_ERROR_STRUCT*  error
			     )
{
  struct InfoStruct
  {
    char varchar[256];
  };

  if (action == SP_PROC_OPEN)
  {
    SP_STATUS spStatus = SP_SUCCESS;
    InfoStruct* is = new InfoStruct;
    char data[6];
    strcpy (data, "1000");
    string2Varchar(data, is->varchar, 256);
    *spProcHandle =(SP_PROCESS_HANDLE) is;
    cout << "In SP_NOINPUT_PROCESS, open" << endl << flush;
    return spStatus;
  }
  if (action == SP_PROC_FETCH)
  {
    return SP_SUCCESS;
  }
  if (action == SP_PROC_CLOSE)
  {
    cout << "In SP_NOINPUT_PROCESS, close" << endl << flush;
    return SP_SUCCESS;
  }

  return SP_SUCCESS;
}

SP_STATUS SP_PARSING_Compile (SP_COMPILE_ACTION action,
			      SP_COMPILE_HANDLE*  cmpHandle ,
			      SP_HANDLE /* spHandle */,
			      SP_ERROR_STRUCT* /* error */ )
{
  if ( action == SP_COMP_INIT )
  {
    *cmpHandle = (SP_COMPILE_HANDLE) new Int32(0);
    cout << "in SP_PARSING_Compile SP_COMP_INIT" << endl;
  }
  if ( action == SP_COMP_EXIT )
  {
    delete ((Int32*)(*cmpHandle));
    cout << "in SP_PARSING_COMPILE SP_COMP_EXIT" << endl;
  }
  return SP_SUCCESS;
}

SP_STATUS SP_PARSING_InputFormat ( SP_FIELDDESC_STRUCT* format ,
				   Lng32 numFields,
				   SP_COMPILE_HANDLE,
				   SP_HANDLE /* spHandle */,
				   SP_ERROR_STRUCT* error )
{
  if ( numFields != 2 )
    {
      error->error = -19020;
      strcpy(error->optionalString[0], "PARSING");
      error->optionalInteger[0] = 2;
      return SP_FAIL;
    }

  strcpy(&((format++)->COLUMN_DEF[0]), "string varchar(40) character set iso88591");
  strcpy(&((format++)->COLUMN_DEF[0]), "i1 int");
  return SP_SUCCESS;
}

SP_STATUS SP_PARSING_PARSER (
     char* param, /* input, null terminated */
     SP_COMPILE_HANDLE cmpHandle, /* input */
     SP_HANDLE spHandle, /* input */
     SP_ERROR_STRUCT* error /* output */
     )
{
  if (strcmp(param, "1")==0)
    *((Int32*)cmpHandle) = 1;
  else if (strcmp(param, "2") == 0 )
    *((Int32*)cmpHandle) = 2;
  else
  {
    error->error = -19021;
    strcpy(error->optionalString[0], "SP_PARSING_PARSER ");
    sprintf(error->optionalString[1], " invalid parameter : %s ", param);
    error->error = -19021;
    return SP_FAIL;
  }
  return SP_SUCCESS;
}


SP_STATUS SP_PARSING_NumOutputs ( Lng32* num,
				  SP_COMPILE_HANDLE cmpHandle,
				  SP_HANDLE /* spHandle */,
				  SP_ERROR_STRUCT* /* error */)
{
  Int32 param = *( (Int32*)cmpHandle );
  if ( param == 1 || param == 2)
    *num = param;
  else
    return SP_FAIL;
  return SP_SUCCESS;
}

SP_STATUS SP_PARSING_OutputFormat (SP_FIELDDESC_STRUCT* format ,
				   SP_KEYDESC_STRUCT*  /*keyFields */ ,
				   Lng32*  /*numKeyFields */ ,
				   SP_COMPILE_HANDLE cmpHandle,
				   SP_HANDLE /* spHandle */,
				   SP_ERROR_STRUCT* /* error */  )
{

  Int32 param = *( ( Int32*)cmpHandle );
  if ( param == 1 )
  {
    strcpy ( &((format++)->COLUMN_DEF[0]), "i1 int" );
    return SP_SUCCESS;
  }
  if ( param == 2 )
  {
    strcpy ( &((format++)->COLUMN_DEF[0]), "i1 int");
    strcpy ( &((format++)->COLUMN_DEF[0]), "c2 char(5) character set iso88591");
    return SP_SUCCESS;
  }

  return SP_FAIL;
}

SP_STATUS SP_PARSING_Process(SP_PROCESS_ACTION action,
			     SP_ROW_DATA  inputData ,
			     SP_EXTRACT_FUNCPTR  eFunc ,
			     SP_ROW_DATA outputData ,
			     SP_FORMAT_FUNCPTR fFunc,
			     SP_KEY_VALUE,
			     SP_KEYVALUE_FUNCPTR,
			     SP_PROCESS_HANDLE*  spProcHandle,
			     SP_HANDLE /* spHandle */,
			     SP_ERROR_STRUCT*  error
			     )
{
  struct InfoStruct
  {
    Int32 param; // 1 or 2
    Int32 max;
    Int32 counter;
    char string[5];
  };

  if (action == SP_PROC_OPEN)
  {
    SP_STATUS spStatus = SP_SUCCESS;
    char p[64];
    if (eFunc(0, inputData, 42, p, 0) == SP_NO_ERROR)
    {
      char* ptr;
      short len;
      varchar2String(p, 64, ptr, len);
      Int32 param = atoi(ptr);
      if ( param != 1 && param != 2 )
        return SP_FAIL;
      InfoStruct* is = new InfoStruct;
      is->param = param;
      is->counter = 0;
      eFunc(1, inputData, sizeof(Int32), &(is->max), 0);
      memcpy(is->string, "12345", 5);
      *spProcHandle = is;
      return spStatus;
    }
  }
  if (action == SP_PROC_FETCH)
  {
    InfoStruct* is = (InfoStruct *)(*spProcHandle);
    if ( is )
    {
      if ( is->counter > is->max )
        return SP_SUCCESS;
      fFunc(0, outputData, sizeof(Int32), &(is->counter), 0);
      is->counter++;

      if ( is->param == 2 )
      {
        fFunc(1, outputData, 5, &is->string[0], 0);
      }
      return SP_MOREDATA;
    }
    return SP_FAIL;
  }
  if (action == SP_PROC_CLOSE)
  {
    return SP_SUCCESS;
  }

  return SP_SUCCESS;
}

SP_STATUS SP_DELAY_Compile (SP_COMPILE_ACTION action,
			    SP_COMPILE_HANDLE*  cmpHandle ,
			    SP_HANDLE /* spHandle */,
			    SP_ERROR_STRUCT* /* error */ )
{
  return SP_SUCCESS;
}

SP_STATUS SP_DELAY_InputFormat ( SP_FIELDDESC_STRUCT* format ,
				   Lng32 numFields,
				   SP_COMPILE_HANDLE,
				   SP_HANDLE /* spHandle */,
				   SP_ERROR_STRUCT* error )
{
  if ( numFields != 2 )
    {
      error->error = -19020;
      strcpy(error->optionalString[0], "DELAY");
      error->optionalInteger[0] = 2;
      return SP_FAIL;
    }

  strcpy(&((format++)->COLUMN_DEF[0]), "i1 int");
  strcpy(&((format++)->COLUMN_DEF[0]), "i2 int");
  return SP_SUCCESS;
}

SP_STATUS SP_DELAY_NumOutputs ( Lng32* num,
				SP_COMPILE_HANDLE cmpHandle,
				SP_HANDLE /* spHandle */,
				SP_ERROR_STRUCT* /* error */)
{
  *num = 1;
  return SP_SUCCESS;
}

SP_STATUS SP_DELAY_OutputFormat (SP_FIELDDESC_STRUCT* format ,
				 SP_KEYDESC_STRUCT*  /*keyFields */ ,
				 Lng32*  /*numKeyFields */ ,
				 SP_COMPILE_HANDLE cmpHandle,
				 SP_HANDLE /* spHandle */,
				 SP_ERROR_STRUCT* /* error */  )
{
  strcpy(&((format++)->COLUMN_DEF[0]), "c1 int");

  return SP_SUCCESS;
}

SP_STATUS SP_DELAY_Process(SP_PROCESS_ACTION action,
			   SP_ROW_DATA  inputData ,
			   SP_EXTRACT_FUNCPTR  eFunc ,
			   SP_ROW_DATA outputData ,
			   SP_FORMAT_FUNCPTR fFunc,
			   SP_KEY_VALUE,
			   SP_KEYVALUE_FUNCPTR,
			   SP_PROCESS_HANDLE*  spProcHandle,
			   SP_HANDLE /* spHandle */,
			   SP_ERROR_STRUCT*  error
			   )
{
  struct InfoStruct
  {
    Int32 delay;
    Int32 max;
    Int32 counter;
  };

  if (action == SP_PROC_OPEN)
  {
    SP_STATUS spStatus = SP_SUCCESS;
    Int32 tempi;
    if (eFunc(0, inputData, sizeof(Int32), &tempi, 0) == SP_NO_ERROR)
    {
      Int32 tempi2;
      if (eFunc(1, inputData, sizeof(Int32), &tempi2, 0) == SP_NO_ERROR )
      {
        InfoStruct* is = new InfoStruct;
        is->delay = tempi;
        is->max = tempi2;
        is->counter = 0;
        *spProcHandle = is;
        return spStatus;
      }
    }
    return SP_FAIL;
  }
  if (action == SP_PROC_FETCH)
  {
    InfoStruct* is = (InfoStruct *)(*spProcHandle);
    if ( is )
    {
      if ( is->counter > is->max )
        return SP_SUCCESS;
      //Sleep(is->delay);
      for ( Int32 i=0; i < is->delay; i++ ) i = (i * 10) / 10;
      fFunc(0, outputData, sizeof(Int32), &(is->counter), 0);
      is->counter++;

      return SP_MOREDATA;
    }
    return SP_FAIL;
  }
  if (action == SP_PROC_CLOSE)
  {
    delete (InfoStruct *)(*spProcHandle);
    return SP_SUCCESS;
  }

  return SP_SUCCESS;
}

// ERROR is a stored procedure to test the CMPASSERT feature,
// It is to test that once exception happens in compilation, the
// SP_ERROR_Compile routine will be called for SP_COMP_EXIT.
// If exception happens in execution time, the
// SP_ERROR_Process routine will be called to close the ISP.
// To test this feature, you need
// 1. the following ISP implementation.
// 2. testing code in arkcmp/CmpStoredProc.cpp, search for SP_ERROR_
// 3. testing code in arkcmp/CmpISPUtils.cpp, seach for SP_ERROR_
//    The above 2 places are commented out for now, need to uncomment
//    them to test this feature.
// 4. from sqlci, run
//    select * from table(tdmsip('ERROR', 'TestCMPASSERT'));
//    should trigger the CMPASSERT in compilation time.
// 5. from sqlci, run
//    select * from table(tdmisp('ERROR', 'TestCMPASSERTEXE'));
//    should trigger the CMPASSERT in execution time.
SP_STATUS SP_ERROR_Compile (SP_COMPILE_ACTION action,
			    SP_COMPILE_HANDLE*  cmpHandle ,
			    SP_HANDLE /* spHandle */,
			    SP_ERROR_STRUCT* error )
{
  static Int32 parserCalled = 0;
  if ( action == SP_COMP_EXIT )
  {
    if ( parserCalled )
      return SP_SUCCESS;
    error->error = -19021;
    strcpy(error->optionalString[0], "SP_ERROR_Compile ");
    sprintf(error->optionalString[1], " SP_COMP_EXIT called ");
    return SP_FAIL;
  }
  else
  {
    parserCalled = 0;
    *cmpHandle = &parserCalled;
    return SP_SUCCESS;
  }
}

SP_STATUS SP_ERROR_InputFormat ( SP_FIELDDESC_STRUCT* format ,
				 Lng32 numFields,
				 SP_COMPILE_HANDLE,
				 SP_HANDLE /* spHandle */,
				 SP_ERROR_STRUCT* error )
{
  if ( numFields != 1 )
    {
      error->error = -19020;
      strcpy(error->optionalString[0], "ERROR");
      error->optionalInteger[0] = 1;
      return SP_FAIL;
    }

  strcpy(&((format++)->COLUMN_DEF[0]), "i1 char(16) character set iso88591");
  return SP_SUCCESS;
}

SP_STATUS SP_ERROR_PARSER (
     char* param, /* input, null terminated */
     SP_COMPILE_HANDLE cmpHandle, /* input */
     SP_HANDLE spHandle, /* input */
     SP_ERROR_STRUCT* error /* output */
     )
{
  Int32* parserFlag;
  parserFlag = ((Int32*)cmpHandle);
  *parserFlag = 1;
  return SP_SUCCESS;
}

SP_STATUS SP_ERROR_NumOutputs ( Lng32* num,
				SP_COMPILE_HANDLE cmpHandle,
				SP_HANDLE /* spHandle */,
				SP_ERROR_STRUCT* /* error */)
{
  *num = 1;
  return SP_SUCCESS;
}

SP_STATUS SP_ERROR_OutputFormat (SP_FIELDDESC_STRUCT* format ,
				 SP_KEYDESC_STRUCT*  /*keyFields */ ,
				 Lng32*  /*numKeyFields */ ,
				 SP_COMPILE_HANDLE cmpHandle,
				 SP_HANDLE /* spHandle */,
				 SP_ERROR_STRUCT* /* error */  )
{
  strcpy(&((format++)->COLUMN_DEF[0]), "i1 char(16) character set iso88591");
  return SP_SUCCESS;
}

SP_STATUS SP_ERROR_Process(SP_PROCESS_ACTION action,
			   SP_ROW_DATA  inputData ,
			   SP_EXTRACT_FUNCPTR  eFunc ,
			   SP_ROW_DATA outputData ,
			   SP_FORMAT_FUNCPTR fFunc,
			   SP_KEY_VALUE,
			   SP_KEYVALUE_FUNCPTR,
			   SP_PROCESS_HANDLE*  spProcHandle,
			   SP_HANDLE /* spHandle */,
			   SP_ERROR_STRUCT*  error
			   )
{

  if (action == SP_PROC_OPEN)
  {
    static Int32 withData = 0;
    *spProcHandle = &withData;
    return SP_SUCCESS;
  }
  if (action == SP_PROC_FETCH)
  {
    char temp[16];
    if (eFunc(0, inputData, 16, temp, 0) == SP_NO_ERROR)
    {
      *( (Int32*)(*spProcHandle) ) = 1;
      fFunc(0, outputData, 16, temp, 0);
      return SP_SUCCESS;
    }
    return SP_FAIL;
  }
  if (action == SP_PROC_CLOSE)
  {
    if ( * ((Int32*)(*spProcHandle)) != 1 )
    {
      error->error = -19021;
      strcpy(error->optionalString[0], "SP_ERROR_PROCESS ");
      sprintf(error->optionalString[1], " SP_PROC_CLOSE called ");
      return SP_FAIL;
    }
    //delete (InfoStruct *)(*spProcHandle);
    return SP_SUCCESS;
  }

  return SP_SUCCESS;
}

// COL16 is a internal stored procedure that will return with 16 columns
// of data. This is used to do performance testing. QA is using a test case
// with 13 cols of int and 3 cols of char(52) to test performance.
// input : counter int;  -- number of rows returned.
//         filename varchar(64) -- input file name.
// output : col1 - col13 int; -- sequence number starting from 1
//          col14 - col16 char(52); -- if filename exists, read from filename,
//                                  -- otherwise, some constant value.
SP_STATUS SP_COL16_Compile (SP_COMPILE_ACTION action,
			    SP_COMPILE_HANDLE*  cmpHandle ,
			    SP_HANDLE /* spHandle */,
			    SP_ERROR_STRUCT* /* error */ )
{
  return SP_SUCCESS;
}

SP_STATUS SP_COL16_InputFormat ( SP_FIELDDESC_STRUCT* format ,
				 Lng32 numFields,
				 SP_COMPILE_HANDLE,
				 SP_HANDLE /* spHandle */,
				 SP_ERROR_STRUCT* error )
{
  if ( numFields != 2 )
    {
      error->error = -19020;
      strcpy(error->optionalString[0], "COL16");
      error->optionalInteger[0] = 2;
      return SP_FAIL;
    }

  strcpy(&((format++)->COLUMN_DEF[0]), "maxCounter int");
  strcpy(&((format++)->COLUMN_DEF[0]), "fname varchar(64) character set iso88591");
  return SP_SUCCESS;
}

SP_STATUS SP_COL16_NumOutputs ( Lng32* num,
				SP_COMPILE_HANDLE cmpHandle,
				SP_HANDLE /* spHandle */,
				SP_ERROR_STRUCT* /* error */)
{
  *num = 16;
  return SP_SUCCESS;
}

SP_STATUS SP_COL16_OutputFormat (SP_FIELDDESC_STRUCT* format ,
				 SP_KEYDESC_STRUCT*  /*keyFields */ ,
				 Lng32*  /*numKeyFields */ ,
				 SP_COMPILE_HANDLE cmpHandle,
				 SP_HANDLE /* spHandle */,
				 SP_ERROR_STRUCT* /* error */  )
{
  strcpy(&((format++)->COLUMN_DEF[0]), "c1 int");
  strcpy(&((format++)->COLUMN_DEF[0]), "c2 int");
  strcpy(&((format++)->COLUMN_DEF[0]), "c3 int");
  strcpy(&((format++)->COLUMN_DEF[0]), "c4 int");
  strcpy(&((format++)->COLUMN_DEF[0]), "c5 int");
  strcpy(&((format++)->COLUMN_DEF[0]), "c6 int");
  strcpy(&((format++)->COLUMN_DEF[0]), "c7 int");
  strcpy(&((format++)->COLUMN_DEF[0]), "c8 int");
  strcpy(&((format++)->COLUMN_DEF[0]), "c9 int");
  strcpy(&((format++)->COLUMN_DEF[0]), "c10 int");
  strcpy(&((format++)->COLUMN_DEF[0]), "c11 int");
  strcpy(&((format++)->COLUMN_DEF[0]), "c12 int");
  strcpy(&((format++)->COLUMN_DEF[0]), "c13 int");
  strcpy(&((format++)->COLUMN_DEF[0]), "c14 char(52) character set iso88591");
  strcpy(&((format++)->COLUMN_DEF[0]), "c15 char(52) character set iso88591");
  strcpy(&((format++)->COLUMN_DEF[0]), "c16 char(52) character set iso88591");

  return SP_SUCCESS;
}

SP_STATUS SP_COL16_Process(SP_PROCESS_ACTION action,
			   SP_ROW_DATA  inputData ,
			   SP_EXTRACT_FUNCPTR  eFunc ,
			   SP_ROW_DATA outputData ,
			   SP_FORMAT_FUNCPTR fFunc,
			   SP_KEY_VALUE,
			   SP_KEYVALUE_FUNCPTR,
			   SP_PROCESS_HANDLE*  spProcHandle,
			   SP_HANDLE /* spHandle */,
			   SP_ERROR_STRUCT*  error
			   )
{
  struct InfoStruct
  {
    Int32 max;
    Int32 counter;
    FILE* fd;
  };

  if (action == SP_PROC_OPEN)
  {
    SP_STATUS spStatus = SP_SUCCESS;
    Int32 tempi;
    if (eFunc(0, inputData, sizeof(Int32), &tempi, 0) == SP_NO_ERROR)
    {
      char p[65];
      if (eFunc(1, inputData, 64, p, 0) == SP_NO_ERROR)
      {
        char* ptr;
        short len;
        varchar2String(p, 64, ptr, len);
        InfoStruct* is = new InfoStruct;
        is->max = tempi;
        is->counter = 0;
        is->fd = fopen ( ptr, "r");
        *spProcHandle = is;
        return spStatus;
      }
    }
    return SP_FAIL;
  }
  if (action == SP_PROC_FETCH)
  {
    InfoStruct* is = (InfoStruct *)(*spProcHandle);
    if ( is )
    {
      if ( is->counter > is->max )
        return SP_SUCCESS;
      for ( Int32 i=0; i < 13; i++ )
        fFunc(i, outputData, sizeof(Int32), &(is->counter), 0);
      char temp[64];
      const char tempConst[] =
        "Constant String                                            ";
      char* pch = &temp[0];
      if ( is->fd )
      {
        memset (temp, ' ', 63);
        fgets(temp, 64, is->fd);
        char* lnPtr = strchr(temp, '\n');
        if (lnPtr) *lnPtr = '\0';
      }
      else
        pch = (char*)(&tempConst[0]);
      fFunc(13, outputData, 52, pch, 0);
      fFunc(14, outputData, 52, pch, 0);
      fFunc(15, outputData, 52, pch, 0);

      is->counter++;

      return SP_MOREDATA;
    }
    return SP_FAIL;
  }
  if (action == SP_PROC_CLOSE)
  {
    delete (InfoStruct *)(*spProcHandle);
    return SP_SUCCESS;
  }

  return SP_SUCCESS;
}

SP_STATUS SP_ROW998_Compile (SP_COMPILE_ACTION action,
			     SP_COMPILE_HANDLE*  cmpHandle ,
			     SP_HANDLE /* spHandle */,
			     SP_ERROR_STRUCT* /* error */ )
{
  return SP_SUCCESS;
}

SP_STATUS SP_ROW998_InputFormat ( SP_FIELDDESC_STRUCT* format ,
				  Lng32 numFields,
				  SP_COMPILE_HANDLE,
				  SP_HANDLE /* spHandle */,
				  SP_ERROR_STRUCT* error )
{
  if ( numFields != 1 )
    {
      error->error = -19020;
      strcpy(error->optionalString[0], "ROW998");
      error->optionalInteger[0] = 1;
      return SP_FAIL;
    }

  strcpy(&((format++)->COLUMN_DEF[0]), "maxCounter int");
  return SP_SUCCESS;
}

SP_STATUS SP_ROW998_NumOutputs ( Lng32* num,
				 SP_COMPILE_HANDLE cmpHandle,
				 SP_HANDLE /* spHandle */,
				 SP_ERROR_STRUCT* /* error */)
{
  *num = 1;
  return SP_SUCCESS;
}

SP_STATUS SP_ROW998_OutputFormat (SP_FIELDDESC_STRUCT* format ,
				  SP_KEYDESC_STRUCT*  /*keyFields */ ,
				  Lng32*  /*numKeyFields */ ,
				  SP_COMPILE_HANDLE cmpHandle,
				  SP_HANDLE /* spHandle */,
				  SP_ERROR_STRUCT* /* error */  )
{
  strcpy(&((format++)->COLUMN_DEF[0]), "c1 char(998) character set iso88591");

  return SP_SUCCESS;
}

SP_STATUS SP_ROW998_Process(SP_PROCESS_ACTION action,
			    SP_ROW_DATA  inputData ,
			    SP_EXTRACT_FUNCPTR  eFunc ,
			    SP_ROW_DATA outputData ,
			    SP_FORMAT_FUNCPTR fFunc,
			    SP_KEY_VALUE,
			    SP_KEYVALUE_FUNCPTR,
			    SP_PROCESS_HANDLE*  spProcHandle,
			    SP_HANDLE /* spHandle */,
			    SP_ERROR_STRUCT*  error
			   )
{
  struct InfoStruct
  {
    Int32 max;
    Int32 counter;
  };

  if (action == SP_PROC_OPEN)
  {
    SP_STATUS spStatus = SP_SUCCESS;
    Int32 tempi;
    if (eFunc(0, inputData, sizeof(Int32), &tempi, 0) == SP_NO_ERROR)
    {
      InfoStruct* is = new InfoStruct;
      is->max = tempi;
      is->counter = 0;
      *spProcHandle = is;
      return spStatus;
    }
    return SP_FAIL;
  }
  if (action == SP_PROC_FETCH)
  {
    InfoStruct* is = (InfoStruct *)(*spProcHandle);
    if ( is )
    {
      if ( is->counter > is->max )
        return SP_SUCCESS;
      char temp[998];
      const char tempConst[] =
        "Constant String                                            ";
      sprintf(temp, "%s %d", tempConst, is->counter);
      fFunc(0, outputData, 998, temp, 0);

      is->counter++;

      return SP_MOREDATA;
    }
    return SP_FAIL;
  }
  if (action == SP_PROC_CLOSE)
  {
    delete (InfoStruct *)(*spProcHandle);
    return SP_SUCCESS;
  }

  return SP_SUCCESS;
}

Int32 SQLISP_INIT(SP_REGISTER_FUNCPTR rFunc, SP_DLL_HANDLE* h)
{
  if (!rFunc)
  {
    return 0;
  }

  char funcName[SP_STRING_MAX_LENGTH];
  strcpy(funcName, "FILEINFO");
  rFunc(funcName,
	  SP_FILEINFO_Compile,
	  SP_FILEINFO_InputFormat,
	  0,
	  SP_FILEINFO_NumOutputs,
	  SP_FILEINFO_OutputFormat,
	  SP_FILEINFO_Process,
	  0,
	  CMPISPVERSION);
  strcpy(funcName, "CASTING");
  rFunc(funcName,
	  SP_CASTING_Compile,
	  SP_CASTING_InputFormat,
	  0,
	  SP_CASTING_NumOutputs,
	  SP_CASTING_OutputFormat,
	  SP_CASTING_Process,
	  0,
	  CMPISPVERSION);
  strcpy(funcName, "NOINPUT");
  rFunc(funcName,
    SP_NOINPUT_Compile,
    SP_NOINPUT_InputFormat,
    0,
    SP_NOINPUT_NumOutputs,
    SP_NOINPUT_OutputFormat,
    SP_NOINPUT_Process,
    0,
    CMPISPVERSION);
  strcpy(funcName, "PARSING");
  rFunc(funcName,
    SP_PARSING_Compile,
    SP_PARSING_InputFormat,
    SP_PARSING_PARSER,
    SP_PARSING_NumOutputs,
    SP_PARSING_OutputFormat,
    SP_PARSING_Process,
    0,
    CMPISPVERSION);
  strcpy(funcName, "DELAY");
  rFunc(funcName,
    SP_DELAY_Compile,
    SP_DELAY_InputFormat,
    0,
    SP_DELAY_NumOutputs,
    SP_DELAY_OutputFormat,
    SP_DELAY_Process,
    0,
    CMPISPVERSION);
  strcpy(funcName, "ERROR");
  rFunc(funcName,
    SP_ERROR_Compile,
    SP_ERROR_InputFormat,
    SP_ERROR_PARSER,
    SP_ERROR_NumOutputs,
    SP_ERROR_OutputFormat,
    SP_ERROR_Process,
    0,
    CMPISPVERSION);
  strcpy(funcName, "COL16");
  rFunc(funcName,
    SP_COL16_Compile,
    SP_COL16_InputFormat,
    0,
    SP_COL16_NumOutputs,
    SP_COL16_OutputFormat,
    SP_COL16_Process,
    0,
    CMPISPVERSION);
  strcpy(funcName, "ROW998");
  rFunc(funcName,
    SP_ROW998_Compile,
    SP_ROW998_InputFormat,
    0,
    SP_ROW998_NumOutputs,
    SP_ROW998_OutputFormat,
    SP_ROW998_Process,
    0,
    CMPISPVERSION);

  return 1;
}

Int32 SQLISP_EXIT(SP_DLL_HANDLE)
{
  //cout << "in SQLISP_EXIT" << endl;
  return 1;
}
