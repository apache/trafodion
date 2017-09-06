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
 *****************************************************************************
 *
 * File:         GetErrorMessage.cpp
 * Description:
 *
 * Created:      2/23/96
 * Modified:     $ $Date: 2007/10/09 19:40:40 $ (GMT)
 * Language:     C++
 *
 *
 *****************************************************************************
 */



#include "Platform.h"
#include <ctype.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "GetErrorMessage.h"
#include "ErrorMessage.h"
#include "NAWinNT.h"

#include "sqlmxmsg_msg.h"

#include "NLSConversion.h"
#include "ExSMCommon.h"

  #include <fcntl.h>
	#include <signal.h>	
  #include "MsgCat.h"

// ----------------------------------------------------
// Definitions common to both _WINDOWS and non-WINDOWS.
// ----------------------------------------------------

// UR2
static const NAWchar
  MsgFile_Not_Found[] = WIDE_("*** ERROR[16000] Error message file not found."),
  Msg_Not_Found[]     = WIDE_("No message found for SQLCODE $0~sqlcode."),
  Error_Pfx[]	      = WIDE_("*** ERROR[%d] "),
  Warning_Pfx[]       = WIDE_("*** WARNING[%d] "),
  Info_Pfx[]	      = WIDE_("*** INFO[%d] "),
  Effect_Pfx[]	      = WIDE_("\nEFFECT:\n"),
  Success_Msg[]       = WIDE_("*** INFO[0] Successful completion.\n"),
  Cause_Pfx[]	      = WIDE_("\nCAUSE:\n"),
  Recovery_Pfx[]      = WIDE_("\nRECOVERY:\n");

// UR2. Used by getErrorMessage, NSK version
static const char
  Msg_Not_Found_NSK[] = "No message found for SQLCODE $0~sqlcode.";

const Int32 SQLSTATE_LEN = 5;	// per Ansi
const Int32 HELPID_LEN   = 5;	// per SQL/MX
const Int32 EMS_SEVERITY_LEN = 5;     // per SQL/MX
const Int32 EMS_EVENT_TARGET_LEN   = 7;     // per SQL/MX
const Int32 EMS_EXPERIENCE_LEVEL_LEN   = 8;     // per SQL/MX

static NABoolean isInfoSQLSTATE(const NAWchar* state) 
{
  Int32 i = SQLSTATE_LEN;
  for (; i--; )
    if (*state++ != '0') break;
  if (i < 0) return TRUE;		// SQLSTATE was "00000"
  // ComASSERT(*--state != ' ');
  return FALSE;
}

// Following is the implementation of the SQLSTATE-related functions.
class SqlstateInfo
{
public:
  SqlstateInfo(Lng32 sqlcode, char * sqlstate, NABoolean fabricatedSqlstate)
       : sqlcode_(sqlcode), sqlstate_(NULL),
	 fabricatedSqlstate_(fabricatedSqlstate)
  {
    if (sqlstate)
      {
	sqlstate_ = new char[6];
	strcpy(sqlstate_, sqlstate);
      }
  };

  ~SqlstateInfo()
  {
    if (sqlstate_)
      delete sqlstate_;
  };

  Lng32 sqlcode() { return sqlcode_; };
  char * sqlstate() { return sqlstate_; };
  NABoolean fabricatedSqlstate() { return fabricatedSqlstate_;};

  // -------------------------------------------------------------------------
  // Since a SqlstateInfo is an index, it has an automatic conversion operator
  // to type CollIndex.
  // -------------------------------------------------------------------------
  operator CollIndex () const { return sqlcode_; };


private:
  Lng32 sqlcode_;
  char * sqlstate_;
  NABoolean fabricatedSqlstate_;
};

static NAList<SqlstateInfo*> listOfSqlstates_(NULL);
static pthread_mutex_t       listOfSqlstates_mutex = PTHREAD_MUTEX_INITIALIZER;

NABoolean GetSqlstateInfo(Lng32 sqlcode, char * sqlstate,
			  NABoolean &fabricatedSqlstate)
{
  //SqlstateInfo ssi(sqlcode, NULL, TRUE);
  //CollIndex i = listOfSqlstates_.index(&ssi);

  NABoolean found = FALSE;
  CollIndex i = 0;

  (void) pthread_mutex_lock( &listOfSqlstates_mutex );
  while ((i < listOfSqlstates_.entries()) &&
	 (NOT found))
    {
      if (listOfSqlstates_[i]->sqlcode() == sqlcode)
	found = TRUE;
      else
	i++;
    }

  if (found) //i != NULL_COLL_INDEX)
    {
      SqlstateInfo * s = listOfSqlstates_[i];
      str_cpy_all(sqlstate, s->sqlstate(), 6);
      fabricatedSqlstate = s->fabricatedSqlstate();
    }
  (void) pthread_mutex_unlock( &listOfSqlstates_mutex );
  return found;
}

void AddSqlstateInfo(Lng32 sqlcode, char * sqlstate,
		     NABoolean fabricatedSqlstate)
{
  SqlstateInfo * s = NULL;
  s = new SqlstateInfo(sqlcode, sqlstate, fabricatedSqlstate);

  (void) pthread_mutex_lock( &listOfSqlstates_mutex );
  listOfSqlstates_.insert(s);
  (void) pthread_mutex_unlock( &listOfSqlstates_mutex );
}

// See sqlci/Define.cpp for this
static short needToBindToMessageFile = TRUE;
void GetErrorMessageRebindMessageFile() { needToBindToMessageFile = TRUE; }

// Trivial global function, just hides our message format from caller:
// Get past the "...] " and return pointer to rest of the text.
char *GetPastHeaderOfErrorMessage(char *text)
{
  while (*text)
    if (*text++ == ']')
      break;
  while (*text && isspace((unsigned char)*text))   // For VS2003
    text++;
  return text;
}


// Some static data and function followed by the accessor global function
static const size_t MAX_MSGFN_LEN = 60;

//
// NOTE: The variable msgfn_Ptr_ is made THREAD_P because different
//       Compiler threads might use a different Error Message file name
//       but it is believed that two Compiler instances within the same
//       thread would share the same Error Message file name.
//
static THREAD_P const char  *msgfn_Ptr_ = NULL;
//
// NOTE: The msgfn_Buf_ array is being left as global 'static' because
//       it holds a filename matching the SQLMX_MESSAGEFILE environment
//       variable and that should be the same for all Compiler threads.
//
static       char   msgfn_Buf_[MAX_MSGFN_LEN];

static void saveErrorMessageFileName(const char *nam, NABoolean fromGetEnv)
{
  if (!fromGetEnv)
    msgfn_Ptr_ = nam;
  else {
    msgfn_Ptr_ = msgfn_Buf_;
    size_t len = strlen(nam) + 1;	// +1 to include the final '\0'
    strncpy(msgfn_Buf_, nam, MINOF(MAX_MSGFN_LEN, len));

    if (len > MAX_MSGFN_LEN) {
      // Truncate the name in the buffer.  Yes, sizeof includes the final '\0'.
      static const char ellipsis[] = " ...";
      strcpy(&msgfn_Buf_[MAX_MSGFN_LEN - sizeof(ellipsis)], ellipsis);
    }
  }
}

const char *GetErrorMessageFileName()	// global func
{
  if (!msgfn_Ptr_) {
    NAWchar *s;
    GetErrorMessage(1, s);		// force a saveErrorMessageFileName()
  }
  return msgfn_Ptr_;
}

// ----------------------------------------------------

NABoolean openMessageCatalog(nl_catd* msgCatalog)
{ 
    #define MAX_MESSAGE_PATH_LEN 1024
    char defaultMessageFile[MAX_MESSAGE_PATH_LEN];
    defaultMessageFile[0] = '\0';

    char *mySQROOT          = getenv("TRAF_HOME");
    const char *msgCatPath2 = "/export/bin";
    const char *mbType      = getenv("SQ_MBTYPE");
    const char *msgCatPath4 = "/mxcierrors.cat";

    if (mbType == NULL)  // happens in older builds
      mbType = "32";

    snprintf(defaultMessageFile, MAX_MESSAGE_PATH_LEN, "%s%s%s%s",
             mySQROOT, msgCatPath2, mbType, msgCatPath4);
    
  // Note that SQLMX_MESSAGEFILE is usually set via ms.env, so the above is not relevant  
  const char *cat = getenv("SQLMX_MESSAGEFILE");
  if (!cat)   cat = defaultMessageFile;
  saveErrorMessageFileName(cat, cat != defaultMessageFile);

//BEGIN Solution number 10-040729-8360 
  sigset_t mask_set;	/* used to set a signal masking set. */
 
  sigemptyset(&mask_set);
  sigaddset(&mask_set,SIGINT);
  sigaddset(&mask_set,SIGQUIT);

  sigprocmask(SIG_BLOCK, &mask_set , NULL);
  *msgCatalog = catopen(cat, 0);
  sigprocmask(SIG_UNBLOCK, &mask_set , NULL);
  //END Solution number 10-040729-8360 

  if (!msgCatalog)              // The simulated catopen is really fopen...
    return FALSE;
  else 
    return TRUE;
}

NABoolean getErrorMessageFromCatalog(NAErrorCode error_code_abs, 
                                     MsgTextType M_type,
                                     NAWchar* msgBuf, Lng32 msgBufLen
                                     , nl_catd* msgCatalog
                                     )
{
  Int32 set_num;

  set_num = ((error_code_abs/10) * 10);

  // map M_type to the linenum offset in error.cat file.
  // ERROR_TEXT, SQL_STATE, HELP_ID are all available at
  // offset 1 (first line).
  Lng32 offset = 0;
  switch (M_type)
    {
    case ERROR_TEXT:
    case SQL_STATE:
    case HELP_ID:
    case EMS_SEVERITY:
    case EMS_EVENT_TARGET:
    case EMS_EXPERIENCE_LEVEL:
      offset = 1;
      break;

    case CAUSE_TEXT:
      offset = 2;
      break;

    case EFFECT_TEXT:
      offset = 3;
      break;

    case RECOVERY_TEXT:
      offset = 4;
      break;
    }

  NAErrorCode error_code_ix = (4 * (error_code_abs - set_num)) + offset;



  //BEGIN Solution number 10-040729-8360 
  //The second call of catgets dumps if the previous call has been interrupted. 
  sigset_t mask_set;	/* used to set a signal masking set. */
 
  sigemptyset(&mask_set);
  sigaddset(&mask_set,SIGINT);
  sigaddset(&mask_set,SIGQUIT);

  sigprocmask(SIG_BLOCK, &mask_set , NULL);
  char *msg = catgets(*msgCatalog, set_num, error_code_ix, Msg_Not_Found_NSK);

  sigprocmask(SIG_UNBLOCK, &mask_set , NULL);
  //END Solution number 10-040729-8360 


 // The catgets routine returns a pointer to default message if no
 // message was found for a certain error number.
  NABoolean result = (msg == Msg_Not_Found_NSK) ? FALSE : TRUE;

  // convert to Unicode
  Lng32 wMsgLen = LocaleStringToUnicode(CharInfo::ISO88591, 
#pragma nowarn(1506)   // warning elimination 
                        msg, strlen(msg), msgBuf, msgBufLen);
#pragma warn(1506)  // warning elimination 


  if ( wMsgLen == 0 )
     result = FALSE;

  return result ;
}

short GetErrorMessage (Lng32 error_code,
		       NAWchar *& return_text,
		       MsgTextType M_type,
		       NAWchar* alternate_return_text,
		       Int32 recurse_level, NABoolean prefixNeeded)
{

  short msgNotFound = TRUE;			// assume error return

  NAErrorCode error_code_abs = error_code;
  if (error_code_abs < 0)
    error_code_abs = -error_code_abs;		// absolute value

  //
  // This Gotten_MsgFile_Not_Found variable is made THREAD_P because 
  // different Compiler threads could be for different users.  We
  // should keep track on a per-user basis whether or not we have
  // previously put out a 'Msg File not found' message.
  //
  static THREAD_P short Gotten_MsgFile_Not_Found = FALSE;

  //
  // Different Compiler threads may need different msg_buf arrays, but
  // there should be no chance of invoking another Compiler instance
  // within the same thread while this buffer's contents are important.
  //
  static THREAD_P NAWchar msg_buf[ErrorMessage::MSG_BUF_SIZE];

  NAWchar *s = msg_buf;
  if (alternate_return_text) s = alternate_return_text;

  switch (M_type)
    {
    case ERROR_TEXT:
      {
          if ( prefixNeeded == TRUE ) {
		if (error_code < 0)
		  NAWsprintf(s, Error_Pfx, error_code_abs);
		else if (error_code > 0)
		  NAWsprintf(s, Warning_Pfx, error_code_abs);
		else
		  NAWsprintf(s, Info_Pfx, error_code_abs);
          } else
	    *s = 0;
          break;
      }
    case CAUSE_TEXT:
      {
		NAWsprintf(s, Cause_Pfx);
		break;
      }
    case EFFECT_TEXT:
      {
		NAWsprintf(s, Effect_Pfx);
		break;
      }
    case RECOVERY_TEXT:
      {
		NAWsprintf(s, Recovery_Pfx);
		break;
      }
    case SQL_STATE:
    case HELP_ID:
    case EMS_SEVERITY:
    case EMS_EVENT_TARGET:
    case EMS_EXPERIENCE_LEVEL:
      {
		// Don't need a header for SQLSTATE or HELP_ID..
		// However, ensure that msg_buf is empty
		*s = 0;
		break;
      }
    };

  NAWchar *s_orig = s;
  s += NAWstrlen(s);

  static NABoolean initialized = FALSE;
	
//
// msgCatalog is left as a global 'static' because, once it is initialized,
// it is never changed and all threads should be able to share it.
// This assumes that all threads will use the same msg catalog.
// We use a mutex to prevent more than one thread from trying to initialize
// it concurrently and the variable 'initialized' to prevent subsequent
// initializations after the first one.
//
  static nl_catd msgCatalog;
	
  if (!initialized) // Don't go for the mutex in initialization already done
  {
     static pthread_mutex_t openMsgCatMutex = PTHREAD_MUTEX_INITIALIZER;

     int rc = pthread_mutex_lock(&openMsgCatMutex);
     exsm_assert_rc( rc, "pthread_mutex_lock" );

     // Just in case the initialization happened in another thread while
     // we were waiting for the mutex, we recheck 'initialized'.
     //
     if ( ! initialized )
     {
        if ( openMessageCatalog(&msgCatalog) == FALSE )
        {
           // The message catalog could not be opened.
           // (Displayed only once per sqlci session.)
           if (!Gotten_MsgFile_Not_Found)
              NAWsprintf(s, WIDE_("\n%s"),  MsgFile_Not_Found);
           else
              NAWsprintf(s, WIDE_("\n"));
           Gotten_MsgFile_Not_Found = TRUE;
        } // openMessageCatalog == FALSE 
        else {
           initialized = TRUE;
        }
     }  // initialized

     rc = pthread_mutex_unlock(&openMsgCatMutex);
     exsm_assert_rc( rc, "pthread_mutex_unlock" );

  } // initialized

  NAWchar msg[ErrorMessage::MSG_BUF_SIZE];

  if  ( getErrorMessageFromCatalog(error_code_abs, M_type,  
                                  msg, ErrorMessage::MSG_BUF_SIZE
                                  , &msgCatalog
                                 ) == FALSE )
  {
    // No message found, just return the default message...
    // only if we are looking for the error message.
    if (M_type == ERROR_TEXT)
      {
	if (recurse_level == 0)
	  {
	    if (error_code == 0)
	      {
  		s = msg_buf;
		NAWsprintf(s, Success_Msg);
	      }
	    else
	      {
		NAWsprintf(s++, WIDE_("\n"));
		GetErrorMessage(-SQLERRORS_MSG_NOT_FOUND, s, ERROR_TEXT, s, 1);
	      }
	  }
	else
	  NAWsprintf(s, WIDE_("%s"), Msg_Not_Found);
      }
      else
	NAWsprintf(s, WIDE_("\n"));
    }
  else	// Message found
    {
      NAWchar *msgPtr = msg;

      if (M_type == SQL_STATE)
	{
	      // Return text starting at the SQLSTATE
	      NAWstrncpy(s, msgPtr, SQLSTATE_LEN);
	      s[SQLSTATE_LEN] = 0;	// Null terminate the buffer.
	}
      else if (M_type == HELP_ID)
	{
	      // Get past the SQLSTATE field
	      // and return text starting at the HELPID
	      msgPtr += SQLSTATE_LEN + 1;	// +1 for delimiting space
	      NAWstrncpy(s, msgPtr, HELPID_LEN);
	      s[HELPID_LEN] = 0;
	}
      else if (M_type == EMS_EXPERIENCE_LEVEL)
        {
	      // Get past the SQLSTATE and HELP_ID fields
	      // and return text starting at the EMS_EXPERIENCE_LEVEL
	      msgPtr += SQLSTATE_LEN + 1 + HELPID_LEN + 1;	// +1 for delimiting space
	      NAWstrncpy(s, msgPtr, EMS_EXPERIENCE_LEVEL_LEN);
	      s[EMS_EXPERIENCE_LEVEL_LEN] = 0;
        }
      else if (M_type == EMS_SEVERITY)
        {
	      // Get past the SQLSTATE, HELP_ID and EMS_EXPERIENCE_LEVEL fields
	      // and return text starting at the EMS_SEVERITY 
	      msgPtr += SQLSTATE_LEN + 1 + HELPID_LEN + 1 + EMS_EXPERIENCE_LEVEL_LEN + 1;	// +1 for delimiting space
	      NAWstrncpy(s, msgPtr, EMS_SEVERITY_LEN);
	      s[EMS_SEVERITY_LEN] = 0;
        }
      else if (M_type == EMS_EVENT_TARGET)
        {
	      // Get past the SQLSTATE, HELP_ID, EMS_EXPERIENCE_LEVEL and EMS_SEVERITY fields
	      // and return text starting at the EMS_EVENT_TARGET
	      msgPtr += SQLSTATE_LEN + 1 + HELPID_LEN + 1 + EMS_EXPERIENCE_LEVEL_LEN + 1
                                     + EMS_SEVERITY_LEN + 1;	// +1 for delimiting space
	      NAWstrncpy(s, msgPtr, EMS_EVENT_TARGET_LEN);
	      s[EMS_EVENT_TARGET_LEN] = 0;
        }
      else
	{
	      if (s != s_orig && isInfoSQLSTATE(msg))
	        {
		  s = s_orig;
		  NAWsprintf(s, Info_Pfx, error_code_abs);
  		  s += NAWstrlen(s);
		}

	      // Get past the SQLSTATE, HELP_ID, EMS_EXPERIENCE_LEVEL, EMS_SEVERITY, and 
	      // EMS_EVENT_TARGET fields and return only the message text.
	      msgPtr += SQLSTATE_LEN + 1 + HELPID_LEN + 1 + EMS_SEVERITY_LEN + 1 
                                     + EMS_EVENT_TARGET_LEN + 1 + EMS_EXPERIENCE_LEVEL_LEN + 1;
	      NAWsprintf(s, WIDE_("%s"), msgPtr);
	}

      msgNotFound = FALSE;			// success, message found
    }

  return_text = msg_buf;

  if (alternate_return_text) return_text = alternate_return_text;

  ErrorMessageOverflowCheckW(return_text, ErrorMessage::MSG_BUF_SIZE);

  return msgNotFound;

} // GetErrorMessage


// These two platform-dependent clones of GetErrorMessage could be
// enormously simplified by using the appropriate #defines from NAWinNT.h ...


//
// The variable kludgeMessageFileText is a pointer to a 'const' string.
// The pointer can change, but only once.  It is a global 'static'
// because it is believed that all threads can share the same string
// once it is set.  Also, it is used only in Debug-Mode builds.
//
static const char *kludgeMessageFileText = NULL;

static short kludgeReadStraightFromMessageFile
	       (Lng32 num, NAWchar *msgBuf, Lng32 bufSize)
{
#ifdef NDEBUG
  return FALSE;
#else
  // A kludge for when the message DLL is completely gone:
  // use 100K of system heap to read in as much of SqlciErrors.txt
  // as we can, and do string lookup on that to find messages.
  //
  static const char emptyText = '\0';
  if (!kludgeMessageFileText) {				// first time in
    kludgeMessageFileText = &emptyText;
    const char *env = getenv("SQLMX_MESSAGEFILE");	// /bin/SqlciErrors.txt
    saveErrorMessageFileName(env, !!env);
    if (!env) return FALSE;
    Int32 fd = open(env, O_RDONLY);
    if (fd < 0) return FALSE;


    struct stat stbuf;

    if (fstat(fd, &stbuf) != 0)
       return FALSE;

    char *buffer = new char[stbuf.st_size + 2]; // For prefix sentinel and trailing null
    if (!buffer) return FALSE;
    buffer[0] = '\n';					// prefix sentinel

    size_t i = 1;
    size_t num_left = stbuf.st_size;    
    size_t nread;

    while ((nread = read(fd, &buffer[i], (num_left < 8192 ? num_left : 8192))) != 0)
    {
      i += nread;
      num_left -= nread;
    }
    buffer[i] = '\0';
    kludgeMessageFileText = buffer;
    close(fd);
  }
  if (!*kludgeMessageFileText) return FALSE;
  char numAscii[20];
  sprintf(numAscii, "\n%d ", num);
  const char *msg = strstr(kludgeMessageFileText, numAscii);
  if (!msg) { *msgBuf = -1; return FALSE; }		// nonzero: msgfile fnd
  msg += strlen(numAscii);
  UInt32 i = 0;
#pragma warning (disable : 4018)   //warning elimination
  for (; i < bufSize; i++) { // cvt char* to WCHAR*
#pragma warning (default : 4018)   //warning elimination
    char c = msg[i];
    if (c == '\n' || c == '\r') break;
    msgBuf[i] = c;
  }
  msgBuf[i] = '\0';
  return TRUE;						// message found
#endif
} // kludgeReadStraightFromMessageFile

void GetPreprocessorInstallPath(char *thePath, char *CorCOBOL)
{
} // GetPreProcessorInstallPath


NABoolean openMessageCatalog()
{
  return 0;
}

short GetErrorMessageRC(Lng32 num, NAWchar* msgBuf, Lng32 bufSize)
{
    return kludgeReadStraightFromMessageFile(num, msgBuf, bufSize);
 // return TRUE;
} // GetErrorMessageRC



void ErrorMessageOverflowCheckW (NAWchar *buf, size_t max)
{
  size_t len = NAWstrlen(buf);
  if (len > max-1)			// max-1, to allow for terminal '\0'
    {
      cerr << endl << "ERROR: msg overflow " << len << " " << max-1 << endl;

      char* buf8bit = new char[len+1];
#pragma nowarn(1506)   // warning elimination 
      Lng32 l = UnicodeStringToLocale(CharInfo::ISO88591, buf, len, 
#pragma warn(1506)  // warning elimination 
#pragma nowarn(1506)   // warning elimination 
                                     buf8bit, len+1);
#pragma warn(1506)  // warning elimination 
#pragma warning (disable : 4018)   //warning elimination
      if ( l != len )
         ABORT("Unicode To Locale Translation");  
#pragma warning (default : 4018)   //warning elimination

      printf("%s\n", buf8bit);
      ABORT("ErrorMessageOverflowCheck");  // memory overrun/corruption, unsafe to continue
    }
}
