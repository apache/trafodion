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
//
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciRWSimulator.cpp
 * Description:  Methods to implement RW_MXCI commands until the real code
 *               from RW is available.
 *               
 *               
 * Created:      6/6/2003
 * Language:     C++
 * Status:       
 *
 *
 *
 *
 *****************************************************************************
 */

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "sqlcmd.h"
#include "SqlciRWCmd.h"
#include "RWInterface.h"
#include "Formatter.h"



class ReportWriterEnv
{
public:
  ReportWriterEnv();
  ~ReportWriterEnv();

  char* &headingRow() { return headingRow_; };
  char* &footingRow() { return footingRow_; };
  char* &underline() { return underline_; };
  char* &inputRow() { return inputRow_; };

  Lng32 &headingRowNum() { return headingRowNum_; }
  Lng32 &footingRowNum() { return footingRowNum_; }
private:
  char * headingRow_;
  char * underline_;
  char * inputRow_;

  Lng32 headingRowNum_;

  char * footingRow_;
  Lng32 footingRowNum_;
};

ReportWriterEnv::ReportWriterEnv()
     : headingRow_(NULL), underline_(NULL),
       headingRowNum_(-1),
       footingRow_(NULL), footingRowNum_(-1)
{
}

ReportWriterEnv::~ReportWriterEnv()
{
  if (headingRow_)
    delete headingRow_;

  if (footingRow_)
    delete footingRow_;

  if (underline_)
    delete underline_;
}

Lng32 RW_MXCI_Constructor (void* &RWEnv, void *SqlciEnv)
{
  ReportWriterEnv * rwEnv = new ReportWriterEnv();

  RWEnv = rwEnv;

  return 0;
}

Lng32 RW_MXCI_Destructor (void* &RWEnv, void *sqlciEnv)
{
  SqlciEnv * sqlci_env = (SqlciEnv *)sqlciEnv;
  delete (ReportWriterEnv *)RWEnv;

  return SUCCESS;
}

Lng32 RW_MXCI_sendQuery (void* &RWEnv, void *sqlciEnv, char *query, Lng32 len)
{
  ReportWriterEnv * rwEnv = (ReportWriterEnv *)RWEnv;
  SqlciEnv * sqlci_env = (SqlciEnv *)sqlciEnv;

  if (strncmp(query, "rw reset_list", 13) == 0)
    return RESET_LIST;
  else
    return SEND_QUERY;
}

Lng32 RW_MXCI_sendInputRow (void* &RWEnv, void* sqlciEnv, char *input_row, Lng32 len)
{
  ReportWriterEnv * rwEnv = (ReportWriterEnv *)RWEnv;
  SqlciEnv * sqlci_env = (SqlciEnv *)sqlciEnv;

  rwEnv->inputRow() = input_row;

  return GET_OUTPUT_ROW;
}

static Lng32 RWformatRow(void* &RWEnv,
			void *sqlciEnv, char* &output_row, Lng32 &len)
{
  ReportWriterEnv * rwEnv = (ReportWriterEnv *)RWEnv;

  Lng32 max;
  ErrorValue *e = NULL;

  char * buf;
  char * tgt;
  Lng32 error = MXCI_RW_allocateHeap (sqlciEnv, 200, buf);
  Lng32 error1 = MXCI_RW_allocateHeap (sqlciEnv, 600, tgt);

  if (error)
    return ERR;
 
  MXCI_RW_getMaxColumns (sqlciEnv, max, e);

  Lng32 curpos = 0;
  AttributeDetails * entry = NULL; 

  for (short column=1; column <= max; column++)
  {
      MXCI_RW_getColInfo(sqlciEnv, column, 
			 entry, e);
      
      char * ptr = NULL;
      MXCI_RW_getColAddr (sqlciEnv, column, rwEnv->inputRow(),
                          len, ptr, e);

      char dateFmtBuf[30];
      if (getenv("RW_DATEFORMAT") &&
	  entry->dataType_ == 192)
	{
	  char * d = getenv("RW_DATEFORMAT");
	  DateTimeFormat dt = EUROPEAN;
	  if (strcmp(d, "DEFAULT") == 0)
	    dt = DEFAULT;
	  else if (strcmp(d, "USA") == 0)
	    dt = USA;
	  else if (strcmp(d, "EUROPEAN") == 0)
	    dt = EUROPEAN;
	    
	  error = MXCI_RW_convertToDateFormat(sqlciEnv, ptr, entry,
					      dateFmtBuf, entry,
					      dt,
					      e);
	  ptr = dateFmtBuf;
	}

      Formatter::buffer_it((SqlciEnv *)sqlciEnv, ptr,
			   entry->dataType_,
			   entry->length_,
			   entry->precision_,
			   entry->scale_,
			   NULL,
			   entry->displayLen_,
			   entry->displayLen_,
			   entry->nullable_,
			   &buf[curpos], &curpos,
			   column < max,	//separatorNeeded
			   TRUE);               //checkShowNonPrinting


    }
   
  /*//Unicode test case # 1
  NAWchar Buf1[50];
  Buf1[0]=0x00F8; // from 11_lt2u.h
  Buf1[1]=0x0042; // regular ascii character
  long input_len1 = 4 ; // 2 * 2 = 4 ; 2 bytes per wide char
  NAWchar out_buf1[6];
  long out_len1 = 6;
  long retcode = MXCI_RW_unicodeConvertToUpper(sqlciEnv,(char*)Buf1,input_len1, (char*)out_buf1, out_len1, e);

  // Unicode test case # 2
  NAWchar *Buf2 = L"ABCD EFG";
  long input_len2 = wcslen(Buf2);
  NAWchar out_buf2[100];
  long out_len2 = 100;
  long retcode2 = MXCI_RW_unicodeConvertToUpper(sqlciEnv,(char*)Buf2,input_len2, (char*)out_buf2, out_len2, e);

  // Unicode test case # 3
  NAWchar Buf3[50];
  Buf3[0]=0x00D8; // from 11_ut2l.h
  Buf3[1]=0x0042; // regular ascii character
  long input_len3 = 4 ; // 2 * 2 = 4 ; 2 bytes per wide char
  NAWchar out_buf3[6];
  long out_len3 = 6;
  long retcode3 = MXCI_RW_unicodeConvertToLower(sqlciEnv,(char*)Buf3,input_len3, (char*)out_buf3, out_len3, e);

  // Unicode test case # 4
  NAWchar *Buf4 = L"ABCD EFG";
  long input_len4 = wcslen(Buf2);
  NAWchar out_buf4[100];
  long out_len4 = 100;
  long retcode4 = MXCI_RW_unicodeConvertToLower(sqlciEnv,(char*)Buf4,input_len4, (char*)out_buf4, out_len4, e);
  */

  /* // ConvDoIt test case
  AttributeDetails *srcEntry = new AttributeDetails;
  AttributeDetails *tgtEntry = new AttributeDetails;
  char srcPtr[20] = "FAN";
  char tgtPtr[20] ;
  ErrorValue *err = NULL;

  srcEntry->nullable_= 1;
  srcEntry->dataType_= 0;
  srcEntry->precision_=0;
  srcEntry->scale_=0;
  tgtEntry->nullable_=1;
  tgtEntry->dataType_=0;
  tgtEntry->precision_=0;
  tgtEntry->scale_=0;
  short formatting = 0;

  long retcode = MXCI_RW_convDoIt(sqlciEnv, srcPtr, srcEntry, tgtPtr, tgtEntry, formatting, err);

  */



  buf[curpos] = 0;

  output_row = buf; // memory leak here but who cares, this is a simulator.
  len = curpos;
  if (e)
    { 
      delete e;
      e = NULL;
    }

  return 0;
}

Lng32 RW_MXCI_getOutputRow (void* &RWEnv, void *sqlciEnv, char* &output_row, Lng32 &len)
{
  SqlciEnv * sqlci_env = (SqlciEnv *)sqlciEnv;
  ReportWriterEnv * rwEnv = (ReportWriterEnv *)RWEnv;

  if (rwEnv->headingRowNum() == 0)
    {
      output_row = rwEnv->headingRow();
      len = strlen(rwEnv->headingRow());
      rwEnv->headingRowNum()++;

      return GET_OUTPUT_ROW;
    }
  else if (rwEnv->headingRowNum() == 1)
    {
      output_row = rwEnv->underline();
      len = strlen(rwEnv->underline());
		
      rwEnv->headingRowNum() = -1;

      return SEND_INPUT_ROW;
    }
  else if (rwEnv->footingRowNum() == 0)
    {
      output_row = rwEnv->footingRow();
      len = strlen(rwEnv->footingRow());
		
      rwEnv->footingRowNum() = -1;

      return DONE;
    }

  /*
  PrepStmt * prepStmt = sqlci_env->sqlciRWEnv()->rwExe()->prepStmt();
  SqlCmd::displayRow(sqlci_env, prepStmt);
  output_row = prepStmt->outputBuf();
  len = prepStmt->outputBuflen();
  */
  RWformatRow(RWEnv, sqlciEnv, output_row, len);
  

  return SEND_INPUT_ROW;
}

Lng32 RW_MXCI_sendSelectStatus (void* &RWEnv, void *sqlciEnv, Lng32 num_rows, SelectStatus status)
{
  Lng32 retcode = 0;

  ReportWriterEnv * rwEnv = (ReportWriterEnv *)RWEnv;
  SqlciEnv * sqlci_env = (SqlciEnv *)sqlciEnv;
  ErrorValue * e = NULL;

  if (getenv("RW_SEND_SELECT_STATUS_ERR"))
    {
      return ERR;
    }

  if ((status == LIST_FIRST_STARTED) ||
      (status == LIST_ALL_STARTED))
    {
      // compute heading
      PrepStmt * prepStmt = sqlci_env->sqlciRWEnv()->rwExe()->prepStmt();

      Lng32 output_buflen = prepStmt->outputBuflen();
      rwEnv->headingRow() = new char[output_buflen + 1];
      rwEnv->underline() = new char[output_buflen + 1];
      
      retcode = 
	SqlCmd::getHeadingInfo(sqlci_env, prepStmt, 
			       rwEnv->headingRow(), rwEnv->underline()); 

      rwEnv->headingRowNum() = 0;

      return GET_OUTPUT_ROW;
    }


  if (status == LIST_ENDED)
    return SEND_QUERY;
  else if (status == SELECT_ENDED)
    return DONE;
  else if (status == SELECT_CANCELLED)
    {
      rwEnv->footingRow() = new char[78];
      strcpy(rwEnv->footingRow(), "Report Cancelled!");
      rwEnv->footingRowNum() = 0;
      return GET_OUTPUT_ROW;
    }
  else
    return SEND_INPUT_ROW;
}

Lng32 RW_MXCI_getErrorInfo (void* &RWEnv, void *sqlciEnv, ErrorValue *e)
{
  e->errorCode_ = 666;

  return SUCCESS;
}

Lng32 RW_MXCI_sendOutputDevice (void* &RWEnv, void *sqlciEnv, OutputDevice device)
{
  if (device == LOG_FILE || device == SCREEN)
    return SUCCESS;
  else 
    return ERR;
}

