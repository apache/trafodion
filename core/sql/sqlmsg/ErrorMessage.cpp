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

#include "NAWinNT.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ErrorMessage.h"
#include "GetErrorMessage.h"

#include "ComASSERT.h"
#include "SqlciError.h"
#include "NLSConversion.h"

const size_t DEST_BUF_SIZE = 2 * ErrorMessage::MSG_BUF_SIZE;

void ErrorMessage::insertParams(NAError * errcb)
{
  if (errcb->getErrParamCount() > 0)
    {
      // Note that we allocate twice the size for tmp, in order to forestall
      // array overrun problems (i.e. memory corruption)
      NAWchar      tmp[MSG_BUF_SIZE * 2];
      NAWchar      paramName[MSG_BUF_SIZE];

      NAWchar	paramVal[MSG_BUF_SIZE];		// regular char, not TCHAR
      Int32	paramLen;
      Lng32	paramPos;
      Int32	tmpLen = 0;
      Int32	truncation = FALSE;

      Int32	msgBufOrigLen = NAWstrlen(msgBuf_);

      for (Int32 i = 0; i < msgBufOrigLen; i++)
	{
	  if (msgBuf_[i] == ERRORPARAM_BEGINMARK)
	    {
	      // Get the formal parameter name, excluding the leading '$' mark
	      NAWchar *p = paramName;
	      while (++i < msgBufOrigLen &&
	             (isalnum(msgBuf_[i]) || msgBuf_[i] == ERRORPARAM_TYPESEP))
	        *p++ = msgBuf_[i];
	      *p = NAWchar('\0');
	      i--;				// let's not lose a character!
	      paramPos = FixupMessageParam(paramName, POSITIONAL_PARAM);
	      if (paramPos >= 0)
	        {
		  paramVal[0] = NAWchar('\0');	// default is empty param
		  NAErrorParam * param = errcb->getNAErrorParam(paramPos);
		  if (param)
		    switch (param->getNAErrorParamType())
		      {
		      case NAErrorParam::NAERROR_PARAM_TYPE_INTEGER:
			NAWsprintf(paramVal,WIDE_("%d"),param->getIntegerNAErrorParam());
			break;
		      case NAErrorParam::NAERROR_PARAM_TYPE_CHAR_STRING:
			NAWsprintf(paramVal,WIDE_("%s"),param->getStringNAErrorParam());
			break;
		      }	      
		  paramLen = NAWstrlen(paramVal);

		  NAWstrncpy(&tmp[tmpLen], paramVal, paramLen);
	        }
	      else	// invalid formal param (e.g. "$ab" "$9~" "$~9" "$9x")
		{
		  tmp[tmpLen++] = ERRORPARAM_BEGINMARK;

		  paramLen = NAWstrlen(paramName);
		  NAWstrncpy(&tmp[tmpLen], paramName, paramLen);
	        }
	      tmpLen += paramLen;
	    }
	  else
	    {
	      tmp[tmpLen++] = msgBuf_[i];
	    }
	    
	  // If necessary, truncate the message and exit loop early.
	  // The -1 is for the terminating '\0' below the loop.
	  if (tmpLen > MSG_BUF_SIZE - 1)
	    {
	      tmpLen = MSG_BUF_SIZE - 1;
	      truncation = TRUE;
	      break;
	    }
	} // for

      // Indicate truncation by overwriting last three characters with '...'
      if (truncation)
	tmp[tmpLen-3] = tmp[tmpLen-2] = tmp[tmpLen-1] = NAWchar('.');

      NAWstrncpy(msgBuf_, tmp, tmpLen);
      msgBuf_[tmpLen] = NAWchar('\0');

    }

} // ErrorMessage::insertParams()

void ErrorMessage::printErrorMessage(NAError * errcb)
{
  NAWchar* tmp = msgBuf_;

  // This is always a positive number (but make sure of it!)
  NAErrorCode erc_abs = errcb->getErrCode();
  if (erc_abs < 0) erc_abs = -erc_abs;

  // A warning is positive, an error negative -- 
  // GetErrorMessage generates the proper text for each.
  NAErrorCode erc_signed = (errcb->getErrType() == NAError::NAERROR_WARNING) ?
  			   erc_abs : -erc_abs;

  NABoolean msgNotFound = GetErrorMessage(erc_signed, tmp);

  NABoolean forceParamSubst = msgNotFound && errcb->getErrParamCount() > 0;

/* 
  // if tmp was assigned to a different (e.g. a static) string, we need to copy
  // its contents into this msgBuf_ so that insertParams overwrites our copy
  // and not the original.
*/
    NAWstrcpy(msgBuf_, tmp);

  if (forceParamSubst)
    {
      // msgBuf_ will contain a suitable msg-not-found message, so now we just
      // append substitution parameters to at least make debugging easier.
      //
      // This mirrors what ComCondition::getMessageText does.
      NAWstrcat(msgBuf_, WIDE_(" $0 $1 $2 $3 $4 $5 $6 $7 $8 $9"));

      //dbg: NAWstrcat(msgBuf_, WIDE_(" $ $$ $ab $9 $~ $~~ $~0 $0~ $~a $a~ $0x $0x~int0 $int0~x # $0~int0 $int0~0 $0 $00 $0$0"));
      //dbg: NAWstrcat(msgBuf_, WIDE_(" $Int0~0$int0~1 $0~Int0$1~int0 #"));
      //dbg: NAWstrcat(msgBuf_, WIDE_(" $Int0~0$int0~0 $0~Int0$0~int0 #"));
      //dbg: NAWstrcat(msgBuf_, WIDE_(" $Int0~0$0~int0 $0~Int0$int0~0 #"));
    }

  ErrorMessageOverflowCheckW(msgBuf_, MSG_BUF_SIZE);

  insertParams(errcb);

  if (forceParamSubst)
    {
      // remove trailing blanks and unsubstituted substitution marks
      Int32 tmpLen = NAWstrlen(msgBuf_);
      while (--tmpLen >= 0 && 
      	     (msgBuf_[tmpLen] == NAWchar(' ')  || 
	      msgBuf_[tmpLen] == NAWchar('\t') ||
	      msgBuf_[tmpLen] == ERRORPARAM_BEGINMARK))
        ;
      msgBuf_[++tmpLen] = NAWchar('\0');
    }

  char msgBuf8bit[2*MSG_BUF_SIZE]; 
  UnicodeStringToLocale(CharInfo::ISO88591, msgBuf_, MSG_BUF_SIZE, msgBuf8bit, 2*MSG_BUF_SIZE);

  printf("%s\n", msgBuf8bit);

  fflush(stdout);

} // ErrorMessage::printErrorMessage()

// paramName must be passed in WITHOUT leading '$' (ERRORPARAM_BEGINMARK).
// paramName returns stripped of any internal '~' (ERRORPARAM_TYPESEP) and the
//   chars either preceding it or following it, if paramName is at all valid.
//   E.g., "0~string0" returns as "0" if positional but "string0" if named;
//   "int0~1" returns as "1" if pos but "int0" if named;
//   "2" returns as "2" either way;
//   "b" returns as "b" either way (if pos, function result is -1, invalid);
//   "intx~y" returns as "y" if pos (and function result is -1, invalid) 
//   but "intx~y" if named (with a successful result, and the ComDiagsMsg.C
//   caller will reject it as not matching a string table lookup and then
//   display the entire bogus name).
// Function returns -1 for invalid paramName,
//   0 for valid NAMED_PARAM,
//   n>=0 for valid POSITIONAL_PARAM (n = the position, 0th, 1st, 2nd, ...)
//
// Why do we need to do this?  Well, we have two kinds of messages --
// ComDiagsMsg.C ComCondition ones with named params, and
// ErrorMessage.C ErrorMessage ones with positional.
// The positional params need to have position numbers so that messages can
// be translated (I18N of text often requires reordering params).
// Tagging each param with both name and position info means that the same
// msg code can be used from anywhere (either ComDiags or E'Msg),
// i.e. we can share messages and not have a confusing welter of nearly
// identical ones.
//
Lng32 FixupMessageParam(NAWchar *paramName, MsgParamType paramType)
{
  ComASSERT(paramName);
  if (!*paramName) return -1;			// invalid (empty) paramName

  NAWchar *p;
  NAWchar* sep = NAWstrchr(paramName, ERRORPARAM_TYPESEP);

  NABoolean begend = sep ? (sep == paramName || sep[1] == NAWchar('\0')) : FALSE;

  switch (paramType)
    {
    case NAMED_PARAM:
      if (begend) return 0;			// "~x" and "9~" `legal' names
      if (sep)
	if (isdigit(*paramName))
	  NAWstrcpy(paramName, ++sep);		// "9~x" -> "x"
	else if (isdigit(sep[1]))
	  *sep = NAWchar('\0');			// "x~9" -> "x"
        //else {}				// "x~y" `legal'
      return 0;					// (Dubious legal names will be
      						// flagged by our caller.)

    case POSITIONAL_PARAM:
      if (begend) return -1;			// "~x" and "9~" invalid nums
      if (!isdigit(*paramName))
        if (!sep)
	  return -1;				// "x" invalid num
	else
	  NAWstrcpy(paramName, ++sep);		// "x~9" -> "9"
      else
        if (sep)
	  *sep = NAWchar('\0');			// "9~x" -> "9"
        //else {}				// "9" valid
      for (p=paramName; *p; p++)
        if (!isdigit(*p))
	  return -1;				// "9x" invalid num
      Lng32 pos;
      NAWsscanf(paramName, WIDE_("%d"), &pos);
      return pos;

    default:
      return -1;				// invalid (unknown paramType)
    }
} // FixupMessageParam


void FixCarriageReturn(char *str)
{
  if (! str)
    return;

  // remove any trailing \r(carriage return) or \n(line feed) from error text.
  size_t len = strlen(str);
  if (len == 0)
    return;

  if (len > 0 && str[len-1] == '\n')
    len--;
  if (len > 0 && str[len-1] == '\r')
    len--;

  if (len == 0)
    return;

  size_t j = 0;
  for (size_t i = 0; i < len; i++)
    {
      if (str[i] != '\r')
	{
	  str[j] = str[i];
	  j++;
	}
    }
  str[j] = '\0';
}

void FixCarriageReturn(NAWchar *str)
{
  // remove any trailing \r(carriage return) or \n(line feed) from error text.
  size_t len = NAWstrlen(str);
  if (len > 0 && str[len-1] == '\n')
    len--;
  if (len > 0 && str[len-1] == '\r')
    len--;

  size_t j = 0;
  for (size_t i = 0; i < len; i++)
    {
      if (str[i] != '\r')
	{
	  str[j] = str[i];
	  j++;
	}
    }
  str[j] = '\0';
}

// Changes to this function should be emulated in HandleCLIError (SqlCmd.C)
void NADumpDiags(ostream& outStream, ComDiagsArea* diags,
		 NABoolean newline,
		 Int32 commentIf,
		 FILE* fp,
		 short verbose,
                 CharInfo::CharSet terminal_cs)
{
  if (!diags) return;

  Int32 numDiags = diags->getNumber();
  
  if (!numDiags) return;

  Int32 numWarns = diags->getNumber(DgSqlCode::WARNING_);

  if ( commentIf != NO_COMMENT )
    outStream << endl;		// blank line at beginning

  if (fp) fprintf(fp, "\n");

  NABoolean sqlcodePrefixAdded = (commentIf != NO_COMMENT);
  for (Int32 i = 0; i++ < numDiags; )
  {
    NABoolean cmt = sqlcodePrefixAdded && commentIf && 
                    (commentIf < 0 || (*diags)[i].getSQLCODE() >= 0);

    if ( verbose || (!verbose) && (*diags)[i].getSQLCODE() <= 0  )
    {
     const NAWchar *msg = (*diags)[i].getMessageText(sqlcodePrefixAdded);	// NAWchar, not TCHAR
     NAWriteConsole(msg, outStream, newline, cmt, terminal_cs);

    if (fp)			// if a logfile is open, mirror messages to it
    {
      char mbstr[DEST_BUF_SIZE + 16]; 
      UnicodeStringToLocale(terminal_cs, msg, NAWstrlen(msg),  
                            mbstr, DEST_BUF_SIZE);

      FixCarriageReturn(mbstr);
      fprintf(fp, "%s\n", mbstr);

      if (newline) fprintf(fp, "\n"); 
    }
    }
  }

  outStream << flush;
  if (fp) fflush(fp);
}

void NAWriteConsole(const char *str, ostream& outStream,
		    NABoolean newline,
		    NABoolean comment)
{
  if (!str) return;
  FixCarriageReturn((char *)str);
  if (comment) outStream << "--- ";
  outStream << str << endl;
  if (newline) outStream << endl;
}

void NAWriteConsole(const NAWchar *str, ostream& outStream,
		    NABoolean newline,
		    NABoolean comment,
                    CharInfo::CharSet terminal_cs)
{
  if (!str) return;
  char mbstr[DEST_BUF_SIZE + 16];

  UnicodeStringToLocale(terminal_cs, str, NAWstrlen(str), mbstr, DEST_BUF_SIZE+16);
  NAWriteConsole(mbstr, outStream, newline, comment);
}

