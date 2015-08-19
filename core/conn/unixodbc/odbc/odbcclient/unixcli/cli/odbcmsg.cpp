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
********************************************************************/
/*************************************************************************
*
**************************************************************************/
#include "odbcmsg.h"

#include "stdio.h"
#ifndef unixcli
#include "immudef.h"
#include "fs/feerrors.h"
#endif

#ifdef unixcli
#include "unixmsg.h"
#endif

#ifdef VERSION3
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#endif

#define MSG_NOT_FOUND "Message for ID[%d] not found"
#define MSG_INCOMPLETE "Message for ID[%d] is incomplete"
#define GENERAL_SQLSTATE	"HY000"

#define DEFAULT_MSGFILE   "$SYSTEM.SYSTEM.MXOMSG"

DWORD OdbcMsg::GetMsgId(DWORD ErrCode)
{
	return ErrCode  & 0x0FFFF;
}

OdbcMsg::OdbcMsg()
{
	strcpy(product_number,"T7971G06");
	default_language  = '0';
#ifndef unixcli
	bMsgFileOpened = immu_initialize();
#else
	bMsgFileOpened = 1;
#endif
}

OdbcMsg::~OdbcMsg()
{
	if (bMsgFileOpened)
		immu_close(err_msg_fnum);
}


BOOL OdbcMsg::GetOdbcMessage (DWORD dwLangId, DWORD ErrCode, ODBCMXMSG_Def *MsgStruc, ...)
{
	UINT	actual_len;
	DWORD	error_code;
	LPTSTR	lpMsgBuf;
	va_list	Arguments;

	va_start( Arguments, MsgStruc);
	lpMsgBuf = NULL;

	// clean up message structure
	CleanUpMsgStructure(MsgStruc);
	if (bMsgFileOpened)
	{
		actual_len = FormatODBCMessage( ErrCode, lpMsgBuf, &Arguments	);

		if (actual_len == 0)
		{
			ReportError (MsgStruc, ErrCode, GetLastError());
		}
		else
		{
			FillMsgStructure(ErrCode, (LPSTR)lpMsgBuf, MsgStruc, actual_len);
		}
		if (lpMsgBuf)
			free ( lpMsgBuf );
	}
	else
	{
		ReportError (MsgStruc, ErrCode, 0) ;
	}
	return TRUE;
}

void OdbcMsg::FillMsgStructure (UINT ErrCode, 
								LPSTR lpMsgBuf, 
								ODBCMXMSG_Def *MsgStruc, 
								int actual_len)
{


	char	tempStr[MAX_ERROR_TEXT_LEN+1];
	char	*szTmp;
	int	i = 0;
	int 	found = 0;

	// verify that the message is long enough, otherwise no clean message to return
	if (actual_len >= 12)
	{
			
		// sqlState is the third token in the string
		while ((found != 2) && (i < actual_len))
		{
			if (lpMsgBuf[i] == ' ')
				found++;
			i++;
		}

		if ((found == 2) && (i < actual_len))  
			strncpy(MsgStruc->lpsSQLState, &lpMsgBuf[i], min(5, sizeof(MsgStruc->lpsSQLState)-1));
		else
			strncpy(MsgStruc->lpsSQLState, GENERAL_SQLSTATE, 5);
		
		while ((found != 4) && (i < actual_len))
		{
			if (lpMsgBuf[i] == ' ')
				found++;
			i++;
		}
		
		// do we have the full text available?	
		if ((found == 4) && (i < actual_len))  
		{
			// then we may have the help ID plus something for message text
			szTmp = &lpMsgBuf[i-1];
		}
		else
		{
			// we don't have enough text to return
			sprintf(tempStr, MSG_INCOMPLETE, GetMsgId(ErrCode));
			szTmp = tempStr;
		}
	}
	else
	{
		// return general SQLCode and error message
		strcpy(MsgStruc->lpsSQLState, GENERAL_SQLSTATE);
		sprintf(tempStr, MSG_INCOMPLETE, GetMsgId(ErrCode));
		szTmp = tempStr;
	}

#ifndef COLLAPSED_CFGLIB
	MsgStruc->lpsMsgText = szTmp;
#else
	strncpy(MsgStruc->lpsMsgText,szTmp,USER_ERROR_MAX);
#endif
	
}

void OdbcMsg::CleanUpMsgStructure (ODBCMXMSG_Def * MsgStruc)
{
	memset (MsgStruc->lpsSQLState, '\0', sizeof (MsgStruc->lpsSQLState));
}

void OdbcMsg::ReportError (ODBCMXMSG_Def * MsgStruc, UINT MessageId, UINT error_code)
{
	char	tempStr[512];
	LPVOID	lpMsgBuf;

	strcpy(MsgStruc->lpsSQLState, GENERAL_SQLSTATE);
	sprintf(tempStr, MSG_NOT_FOUND, GetMsgId(MessageId));

#ifndef COLLAPSED_CFGLIB
	MsgStruc->lpsMsgText = tempStr;
#else
	strncpy(MsgStruc->lpsMsgText,tempStr,USER_ERROR_MAX);
#endif
	return ;
}

int OdbcMsg::FormatODBCMessage( DWORD ErrCode, LPTSTR& lpMsgBuf, va_list* Arguments)
{
	char* param[5];
	char* token;

	param[0] = va_arg(*Arguments, char*);
	param[1] = va_arg(*Arguments, char*);
	param[2] = va_arg(*Arguments, char*);
	param[3] = va_arg(*Arguments, char*);
	param[4] = va_arg(*Arguments, char*);

	char szTmp[MAX_ERROR_TEXT_LEN+1];
	szTmp[0]=0;
	short len;
#ifndef unixcli
	if ((len=read_immu_file(
			err_msg_fnum,
			product_number,
			ErrCode,
			szTmp,
			param[0],
			param[1],
			param[2],
			param[3],
			param[4]))!=FALSE)
	{
		if ((lpMsgBuf = (LPTSTR)malloc(len+1)) != NULL)
		{
			szTmp[len]=0;
			strcpy(lpMsgBuf,szTmp);
		}
	}
#else
	len = read_immu_file(ErrCode, szTmp);
	if (len != 0)
	{
                if ((lpMsgBuf = (LPTSTR)malloc(len+1)) != NULL)
                {
                        szTmp[len]=0;
                        strcpy(lpMsgBuf,szTmp);
                }
        }
#endif

	return strlen(szTmp);
}

#ifndef unixcli
short OdbcMsg::read_immu_file
( short            filenum,           /* IN:  Key of IMMU mesg. file       */
  char             *productnum,       /* IN:  Key of IMMU mesg. file       */
  short            err_num,           /* IN:  Key of IMMU mesg. file       */
  char             *error_text_p,     /* IN:  Pointer to the error text    */
  void             *param1,           /* IN:  Ptr to parameter 1           */
  void             *param2,           /* IN:  Ptr to parameter 2           */
  void             *param3,           /* IN:  Ptr to parameter 3           */
  void             *param4,           /* IN:  Ptr to parameter 4           */
  void             *param5)           /* IN:  Ptr to parameter 5           */
{
  IMMUKEY_DEF key;             /* Key of IMMU mesg. file            */
  short       error;           /* error code for IMMU proc, 0 = O.K */
  short       line_length;     /* Line length in bytes              */
  short       total_length;    /* Total length in bytes returned    */
  short       more_lines;      /* More to come indicator            */

  /*--------------------------------------------------------------*/
  /* Set up key ( prodnum, rectype, errnum, linenum, help_topic ) */
  /* for read statement.                                          */
  /*--------------------------------------------------------------*/

   key.IMMUkey_key_field1 = err_num;
   strncpy ((char *) key.IMMUkey_product_number, productnum, PRODUCT_NO_SIZE);
   strncpy ((char *) key.IMMUkey_record_type, "ER", 2);  /* Always = ERROR   */
   key.IMMUkey_key_field2 = 1;  /* This picks up the first line of the mesg  */
   memset(key.IMMUkey_key_field3, ' ', 31);  /* Fill with spaces, ascii 32   */
   total_length = 0;

  /*--------------------------------------------------------------*/
  /* Call IMMU_READ_FORMAT looping to fetch all of the error      */
  /* message. The message will be read into error_item.           */
  /*--------------------------------------------------------------*/

   for (;;)
      {
      error = immu_read_format( filenum,          /* Error msg filenum     */
                                &key,             /* Error message key     */
                                error_text_p,     /* Ptr to 'err_item'     */
                                MAX_ERROR_TEXT_LEN,
                                &line_length,     /* Actual length returned*/
                                special_symbol,   /* Special symbol = ~    */
                                &more_lines,      /* If = -1 there's more..*/
                                (char*)param1,
								(char*)param2,
								(char*)param3,
								(char*)param4,
								(char*)param5);

      if (error) /* Didn't retrieve the mesg. */
         return FALSE;
      else
         {
         error_text_p += line_length;
         total_length += line_length;
         if (!more_lines)
            return total_length;
         else
            {
            strcpy( error_text_p, " " );
            error_text_p += 1;
            total_length += 1;
            }
         }
      } /* end loop */

   return FALSE;

} /* read_immu_file */

#else

short OdbcMsg::read_immu_file (short err_num, char *error_text_p)
{
	short test;
	sprintf(error_text_p,"%s %d %s", msgarray[err_num-1].sever,
		 msgarray[err_num-1].id, msgarray[err_num-1].text);
	test = strlen(error_text_p);

	return test;
}

#endif

short OdbcMsg::get_msgfile_name (void)
{
   short error = 0;
   short len;
   short processhandle[10];
   char  object[EXT_FILENAME_LEN+1];
   char  subvol[FULL_SUBVOL_LEN+1];

   memset(msgfile_name, '\0', EXT_FILENAME_LEN+1);
#ifndef unixcli

   PROCESSHANDLE_GETMINE_(processhandle);
   error = PROCESS_GETINFO_(processhandle
                           , /* proc_fname */
                           , /* maxlen     */
                           , /* len        */
                           , /* priority   */
                           , /* mon-processhandle */
                           , /* hometerm   */
                           , /* max-len    */
                           , /* len        */
                           , /* processtime */
                           , /* creator_access_id */
                           , /* process-access-id */
                           , /* gmon-process-handle */
                           , /* job-id */
                           , object
                           , EXT_FILENAME_LEN
                           , &len);

   if (error != FEOK)
      return(error);

   object[len] = '\0';
   error = FILENAME_DECOMPOSE_(object,
                               (short)strlen(object),
                               subvol,
                               FULL_SUBVOL_LEN,
                               &len,
                               1,
                               0x02);

   if (error != FEOK)
      return(error);

   subvol[len]='\0';
   strncpy(msgfile_name, subvol, len);
   strcpy(&msgfile_name[len], ".MXOMSG");
	return(FEOK);
#else
// need to add mxomsg file to a common location, perhaps /etc/hpodbc/mxomsg
	strcpy(msgfile_name, "/etc/hpodbc/mxomsg");
   
 	return(0);
#endif

}

short OdbcMsg::immu_initialize(void)
{
	short error;

	error = get_msgfile_name();
#ifndef unixcli
	if (error == FEOK)
	{
		error = immu_open(msgfile_name,
                     (short)strlen(msgfile_name),
                     &err_msg_fnum,
                     product_number,
                     &special_symbol,
                     &default_language);
		if (error)
		{
			if (strcmp(msgfile_name, DEFAULT_MSGFILE) != 0)
				error = immu_open(DEFAULT_MSGFILE,
                            (short)strlen(DEFAULT_MSGFILE),
                            &err_msg_fnum,
                            product_number,
                            &special_symbol,
                            &default_language);
		}
	}

	if (error)
		return FALSE;
#else
	if (error == 0) //FEOK
		msgfile = fopen(msgfile_name, "r");
	
	if (!msgfile)
		return FALSE;

#endif
	return TRUE;
}

void OdbcMsg::immu_close(short err_msg_fnum)
{
#ifndef unixcli
	::immu_close(err_msg_fnum);
#else
//	fclose(msgfile); 
#endif
}








