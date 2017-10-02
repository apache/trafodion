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
 * File:         CatSQLShare.cpp
 * Description:  See CatSQLShare.h for details.
 *               
 *               
 * Created:      7/2/99
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#define unsigned_char unsigned char
#define int_16        short

#include "CatSQLShare.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include "PortProcessCalls.h"
#include <pthread.h>

#include "cextdecs/cextdecs.h"
#include "seabed/ms.h"
#include "seabed/fs.h"
#include "seabed/fserr.h"


// Generate a unique value. This is really the meat and bone of UIDs,
// packaged in a context where it can be used by all SQL components, including utilities.
//
// Unique values are composed as follows:
// ---------------------------------------------------------------------------------
// WINDOWS - just uses the rightmost 51 bits of the Julian timestamp
// Linux:                                                                               !
// !          |rightmost 14 bits of node number | 
// |          |             XOR                 |
// |          |right most 14 bits of Thread ID  ! rghtmost 49 bits of Julian timestamp  !
// ! sign bit |       (14 bits)                 !   (expect rollover every 17 years)    !
// ---------------------------------------------------------------------------------
//
// The "Julian timestamp" is constructed from the combination of the system coldload
// time and the time since coldload, to avoid potential duplicates if the system clock 
// is set back while the system is running.

// On LINUX, use 49 bits, for other platforms, use 51 bits
#define MASK 0x1ffffffffffffLL

SQLShareInt64 CATSQLSHARE_LIB_FUNC
generateUniqueValue (void)
{
  static THREAD_P SQLShareInt64 lastTime = -1;
  static          SQLShareInt64 systemColdLoadTime = -1;
  static THREAD_P SQLShareInt64 highOrderBits = -1;
  static          SQLShareInt64 lastGeneratedUID = -1;
  pthread_mutex_t               lastGeneratedUID_mutex = PTHREAD_MUTEX_INITIALIZER;

  SQLShareInt64 timeSinceColdLoad = 0;

  // Generate the highOrderBits for the UID.  They will not change for`
  // the life of the thread, so we only need to calculate them once
  // per thread

  if (highOrderBits == -1)
  {
    Int32  rtnCode = -1;
    char monProcessName[MS_MON_MAX_PROCESS_NAME+1];

    Int32  monNodeId = -1;
    Int32  monProcessId = -1;
    Int32  monProcessType = -1;
    Int32  monZoneId = -1;
    Int32  lnxOsPid = -1;
    ThreadId lnxOsTid = -1;
    
    rtnCode = msg_mon_get_my_info(&monNodeId,              // int * mon node-id
                                  &monProcessId,           // int * mon process-id
                                  monProcessName,          // char * mon process-name
                                  MS_MON_MAX_PROCESS_NAME, // int mon process-name-max-len
                                  &monProcessType,         // int * mon process-type
                                  &monZoneId,              // int * mon zone-id
                                  &lnxOsPid,               // int * linux os process-id
                                  &lnxOsTid);              // long * linux os thread-id

    // should we return an error instead?
    if (rtnCode != XZFIL_ERR_OK)
    {
      monNodeId = 255;
      lnxOsTid = 1; 
    }

    highOrderBits = (monNodeId & 0x3fff) ^ (lnxOsTid & 0x3fff);

    // Shift over to make room for the Juliantimestamp
    highOrderBits = highOrderBits << 49;

    systemColdLoadTime = JULIANTIMESTAMP (1,OMIT,OMIT,OMIT);
    lastTime = JULIANTIMESTAMP (3,OMIT,OMIT,OMIT);

  }

  while (timeSinceColdLoad <= lastTime)
  {
    // cater for the situation where we run faster than the clock :-)
    timeSinceColdLoad = JULIANTIMESTAMP(3,OMIT,OMIT,OMIT);
  }

  // calculate the rightmost bits of JulianTimeStamp
  lastTime = timeSinceColdLoad;
  SQLShareInt64 tempValue = timeSinceColdLoad + systemColdLoadTime;
  tempValue = tempValue & MASK;

  // If highOrderBits + tempValue equals lastGeneratedUID then we generated the
  // same value as previous, add one to make it unique.
  // if hightOrderBits + tempValue is less than lastGeneratedUID, then we may have
  // generated the same value more than once, adjust.

  // Lock the mutex on lastGeneratedUID -- to ensure uniqueness.
  (void) pthread_mutex_lock(&lastGeneratedUID_mutex);

  if (lastGeneratedUID <= (highOrderBits + tempValue))
    lastGeneratedUID = highOrderBits + tempValue + 1;
  else
    lastGeneratedUID = highOrderBits + tempValue;

  SQLShareInt64 tempLastGeneratedUID = lastGeneratedUID;

  // Unlock the mutex
  (void) pthread_mutex_unlock(&lastGeneratedUID_mutex);

  return tempLastGeneratedUID;
}

//similar to generateUniqueValue
//except that it is faster since it does not use 
//time since coldload logic. Used for syskey generation in Trafodion.
#define MASK1 0x1ffffffffffffLL
Int64
generateUniqueValueFast ()
{
  static THREAD_P Int64 lastTime = -1;
  static THREAD_P Int64 highOrderBits = -1;
  Int64 uniqueValue = -1;
  Int64 currentTime = -1;
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 1000;  //1 microsecond
            
  // Generate the highOrderBits for the UID.  They will not change for`
  // the life of the thread, so we only need to calculate them once

  if (highOrderBits == -1)
  {
    
    Int32  rtnCode = -1;
    char monProcessName[MS_MON_MAX_PROCESS_NAME+1];

    Int32  monNodeId = -1;
    Int32  monProcessId = -1;
    Int32  monProcessType = -1;
    Int32  monZoneId = -1;
    Int32  lnxOsPid = -1;
    ThreadId lnxOsTid = -1;
    
    rtnCode = msg_mon_get_my_info(&monNodeId,              // int * mon node-id
                                  &monProcessId,           // int * mon process-id
                                  monProcessName,          // char * mon process-name
                                  MS_MON_MAX_PROCESS_NAME, // int mon process-name-max-len
                                  &monProcessType,         // int * mon process-type
                                  &monZoneId,              // int * mon zone-id
                                  &lnxOsPid,               // int * linux os process-id
                                  &lnxOsTid);              // long * linux os thread-id

    // should we return an error instead?
    if (rtnCode != XZFIL_ERR_OK)
    {
      monNodeId = 255;
      lnxOsTid = 1;
    }

    highOrderBits = (monNodeId & 0x3fff) ^ (lnxOsTid & 0x3fff);

    // Shift over to make room for the Juliantimestamp
    highOrderBits = highOrderBits << 49;

    currentTime = JULIANTIMESTAMP ();
  }
  else 
  {
    currentTime = JULIANTIMESTAMP ();
    while (currentTime <= lastTime)
    {
      //now sleep
      nanosleep(&ts, NULL);
      // cater for the situation where we run faster than the clock :-)
      currentTime = JULIANTIMESTAMP ();
    }
  }

  lastTime = currentTime ;
  // calculate the rightmost bits of JulianTimeStamp
  currentTime = currentTime & MASK1;
  uniqueValue = highOrderBits + currentTime ;

  return uniqueValue; 
}

//------------------------------------------------------------------------
// Generate a name, of specified format, based upon a generated unique value 
// (see above). Currently, we can generate 4 formats: 
//  - Funny filenames of the form axxxxx00
//  - Funny SMD subvol names of the form ZSDnxxxx
//  - Funny user subvol names of the form ZSDaxxxx
//  - Implicit ANSI name suffixes of the form 
//    <Truncated ANSI identifier>_nnnnnnnn_nnnn
// where x is an alphanum char, a is an alpha char and n is a digit,
// from the set of permitted characters below 
//
// generateFunnyName will return the desired name part, including decorations
//------------------------------------------------------------------------
//

// The layouts of the various forms of generated names
//
const char * NameFormatTemplates [] = 
{ "ZSDnxxxx",
  "ZSDaxxxx",
  "axxxxx00",
  "_nnnnnnnnn_nnnn"
};

//
// Permitted characters: No vowels, no zero digit
//
const char alphaNumCharacters [] = "BCDFGHJKLMNPQRSTVWXZ123456789";
const char * numericCharacters = &alphaNumCharacters[20];

// ----------------------------------------------------------------
// void generateFunnyName (const FunnyNameFormat nameFormat, char * generatedName);
//
// This function produces a new, hopefully unique "funny-name".  
// The Guardian name of a SQL partition has the form:
//      \system.$volume.ZSDxxxxx.axxxxx00
// where:
//    each "x" is a alphanumeric
//    the "a" is an alphabetic (numeric not allowed there by Guardian)
//    the last two chars of the file name are always zero ('0')
//
//
// The algorithm for generating the name has some features which
// are important to the way the name will be used.
// 1) First, a filename always is 8 chars long and ends
//    with "00".  This is needed on NT where the file system
//    & DP2 store multiple streams in the file (e.g., data
//    & resource fork) and use these last two chars internally
//    to identify which stream is being addressed
// 
// 2) Two unique values picked sequentially in the same process are likely 
//    to differ only by one. If we generated two funnynames from 
//    these by a standard right-to-left conversion into 
//    pseudo-base-36, the funny names would likely be alphabetically 
//    next to each other. This would cause an inefficiency for DP2
//    as the secondary labels of the two files would be intermixed.
//    So, when we convert the unique value into the funny name, we extract
//    the pseudo-base-36 right-to-left, but insert them left-to-right,
//    in order to keep the internal labels next to
//    their external counterparts.
// 
// ----------------------------------------------------------------


void CATSQLSHARE_LIB_FUNC
generateFunnyName (const FunnyNameFormat nameFormat, char * generatedName)
{

  const char * nameFormatTemplate = NameFormatTemplates[nameFormat];
  char * outputPointer = generatedName;

  SQLShareInt64 uniqueValue = generateUniqueValue ();

  const char * nextTemplateChar = nameFormatTemplate;

  while (*nextTemplateChar)
  {
    Int32 divisor;
    const char * base;

    switch (*nextTemplateChar)
    {
    case 'x': // Generate an alphanumeric character
      base = alphaNumCharacters;
      divisor = 29;
      break;

    case 'a': // Generate an alpha character
      base = alphaNumCharacters;
      divisor = 20;
      break;

    case 'n': // Generate a numeric character
      base = numericCharacters;
      divisor = 9;
      break;

    default:  // A decoration, simply move the character
      base = NULL;
      divisor = 0;
      break;
    }

    if (divisor)
    {
      *outputPointer = base [uniqueValue % divisor];
      uniqueValue /= divisor;
    }
    else
    {
      *outputPointer = *nextTemplateChar;
    }

    outputPointer++;
    nextTemplateChar++;
  }

  *outputPointer = 0;

}



//  CatDeriveRandomName
//
//  Generate a name from input simple object name by truncating to 20 chars
//  and appending 9-byte timestamp.
//  Caller must allocate and pass a 30 char array for output generatedName.
//  This array will be overwritten with a null-terminated string.
//  Both inputName and generatedName are treated as external formatted
//  names, but delimitted names are stripped of their delimitting quotes
//
//  The algorithm in this routine only works well when the inputName
//  contains characters in a character set like ISO88591; i.e., single-
//  byte characters.  The algorithm might run into problem when the
//  inputName contains multibyte characters; e.g., UTF-8 characters.

void CATSQLSHARE_LIB_FUNC 
  CatDeriveRandomName (const char* inputName, char* generatedName)
{
   generatedName[0] = '\0';
   UInt32 len = strlen(inputName);

   // Copy up to 20 chars of incoming name.
   if ( inputName[0] == '"' )
     // Identifier is delimited.  Strip the leading quote.
   {
     len = len - 1;
     if ( len > 20 )
       len = 20;
     strncpy( generatedName, &inputName[1], len);
   }
   else
   {
     if ( len > 20 )
       len = 20;
     strncpy ( generatedName, inputName, len );
   }
   generatedName[len] = '\0';

   // Strip trailing quotes from the (possibly truncated) name.
   for ( ; generatedName[len - 1] == '"' && len > 0; len-- );

   // Append the funny name suffix for uniqueness
   generateFunnyName (UID_GENERATED_ANSI_NAME, &generatedName[len]);

}


Int32 CATSQLSHARE_LIB_FUNC
SqlShareLnxGetMyProcessIdString (char   *processIdStrOutBuf,        // out
                                 size_t  processIdStrOutBufMaxLen,  // in
                                 size_t *pProcessIdStrLen)          // out
//
// Generates the process id string for my running process.
// The format of this process id string is:
// $mon-p-name:mon-node-id:mon-p-id:mon-p-type:mon-zone-id:linux-os-p-id:linux-os-thread-id
// where mon- stands for monitor- and -p- stands for -process-
// The generated process id string is stored in the DDL_LOCKS.PROCESS_ID
// metadata column by several utilities for later use by the RECOVER
// utility.
//
// Returns XZFIL_ERR_OK if successful.
// Returns XZFIL_ERR_BOUNDSERR if processIdStrOutBuf bufer is too small;
//                             *pProcessIdStrLen is set to the actual
//                             length of the generated string and
//                             processIdStrOutBuf is set to an empty string.
// Return the error code returned by the msg_mon_get_my_info() call and sets
//                             *pProcessIdStrLen to zero to tell the caller
//                             that this function is unable to compute the
//                             process id string
{
  Int32  rtnCode = -1;
  Int32  monNodeId = -1;
  Int32  monProcessId = -1;
  char monProcessName[MS_MON_MAX_PROCESS_NAME+1];
  Int32  monProcessType = -1;
  Int32  monZoneId = -1;
  Int32  lnxOsPid = -1;
  ThreadId lnxOsTid = -1;
  char buf[2016]; // plenty of room to avoide overflow condition
  char *p = NULL;

  // monProcessName[0] = '\0';
  rtnCode = msg_mon_get_my_info(&monNodeId,              // int * mon node-id
                                &monProcessId,           // int * mon process-id
                                monProcessName,          // char * mon process-name
                                MS_MON_MAX_PROCESS_NAME, // int mon process-name-max-len
                                &monProcessType,         // int * mon process-type
                                &monZoneId,              // int * mon zone-id
                                &lnxOsPid,               // int * linux os process-id
                                &lnxOsTid);              // long * linux os thread-id
  if (rtnCode != XZFIL_ERR_OK)
  {
    // set the actual length to zero to tell the caller
    // that we are unable to compute the process id string
    *pProcessIdStrLen = 0;
    processIdStrOutBuf[0] = '\0';
    return rtnCode;
  }

  // monProcessName[MS_MON_MAX_PROCESS_NAME] = '\0';
  memcpy(buf, monProcessName, strlen(monProcessName));
  p = &buf[strlen(monProcessName)];
  *p = '\0';

  sprintf(p,
          ":%-u:%-u:%-u:%-u:%-u:%-ld",
          monNodeId,
          monProcessId,
          monProcessType,
          monZoneId,
          lnxOsPid,
          lnxOsTid);

  if (strlen(buf) > processIdStrOutBufMaxLen)
  {
    // return the actual length just in case the caller
    // wants to allocate more space for the output buffer
    // and then tries again.
    *pProcessIdStrLen = strlen(buf);
    processIdStrOutBuf[0] = '\0';
    return XZFIL_ERR_BOUNDSERR;
  }

  *pProcessIdStrLen = strlen(buf);
  memcpy(processIdStrOutBuf, buf, *pProcessIdStrLen);
  if (*pProcessIdStrLen < processIdStrOutBufMaxLen)
    processIdStrOutBuf[*pProcessIdStrLen] = '\0';
  return XZFIL_ERR_OK;

}

