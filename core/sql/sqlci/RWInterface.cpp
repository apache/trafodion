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
 * File:         RWInterface.cpp
 * Description:  File which has all the function declarations for RWInterface.h
 *               file.
 * Created:      6/11/2003
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "RWInterface.h"
#include "exp_clause_derived.h"


Lng32 MXCI_RW_getMaxColumns (void *SqlciEnv, Lng32 &max, ErrorValue* &e)
{ 
	return 0;
}

Lng32 MXCI_RW_getColInfo (void *SqlciEnv, Lng32 column, 
                           AttributeDetails* &entry, ErrorValue* &e)
{
	return 0;
}

Lng32 MXCI_RW_getColAddr (void *SqlciEnv, Lng32 column, char *input_row, 
                          Lng32 len, char* &ptr, ErrorValue* &e)
{ 
	return 0;
}

Lng32 MXCI_RW_convDoIt (void *SqlciEnv, char* srcPtr, AttributeDetails* srcEntry, char *tgtPtr, AttributeDetails* tgtEntry, ErrorValue* &e)
{
    return 0;
}

Lng32 MXCI_RW_allocateHeap (void* SqlciEnv, Lng32 len, char* &ptr)
{
  void *p = SqlciEnv->rwHeap()->allocateMemory(len, TRUE);
  if (p)
  {
    ptr = (char *)p;
    return SUCCESS;
  }
  else
    return ERROR;
}

Lng32 MXCI_RW_deallocateHeap (void* SqlciEnv, Lng32 len, char* ptr)
{
  if (len != 0)
  {
    SqlciEnv->rwHeap()->deallocateMemory((void*)ptr);
    return SUCCESS;
  }
  else
    return ERROR;
}

Lng32 MXCI_RW_getOutputDevice (void* SqlciEnv, OutputDevice &device,
                                ErrorValue* &e)
{
   if (SqlciEnv->get_logfile()->IsOpen())
     device = LOG_FILE;
   else
     device = SCREEN;
}

	
}

Lng32 MXCI_RW_convertToDateFormat (void *SqlciEnv, char* srcPtr, AttributeDetails* srcEntry, char *tgtPtr, AttributeDetails* tgtEntry,  DateTimeFormat format, 
ErrorValue* &e)
{
	return 0;
}

Lng32 MXCI_RW_unicodeConvertToUpper(void *SqlciEnv, char *input_char, Lng32 num_of_input_bytes, char *output_char, Lng32& num_of_output_chars, ErrorValue* &e)
{
	return 0;
}


Lng32 MXCI_RW_unicodeConvertToLower(void *SqlciEnv, char *input_wchar, Lng32 num_of_input_bytes, char *output_char, Lng32& num_of_output_chars, ErrorValue* &e)
{
	return 0;
}


