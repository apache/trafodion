/************************************************************************
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
**************************************************************************/
//
// MODULE: SrvrCommon.cpp
//
// PURPOSE: Implements the common functions used by Srvr
//

#include <platform_ndcs.h>
#include <sql.h>
#include <sqlext.h>
#include "SrvrCommon.h"
#include "SrvrKds.h"
#include "SqlInterface.h"
#include "CommonDiags.h"
#include "CSrvrConnect.h"
#include "Debug.h"

#define SQL_PSEUDO_FILE "$ZQFO"
#ifndef SEABASE_MD_SCHEMA
#define SEABASE_MD_SCHEMA     "\"_MD_\""
#define SEABASE_MD_CATALOG     "TRAFODION"
#define SEABASE_COLUMNS         "COLUMNS"
#define SEABASE_DEFAULTS        "DEFAULTS"
#define SEABASE_INDEXES          "INDEXES"
#define SEABASE_KEYS                "KEYS"
#define SEABASE_OBJECTS          "OBJECTS"
#define SEABASE_OBJECTUID      "OBJECTUID"
#define SEABASE_TABLES            "TABLES"
#define SEABASE_VIEWS              "VIEWS"
#define SEABASE_VIEWS_USAGE  "VIEWS_USAGE"
#define SEABASE_VERSIONS        "VERSIONS"
#endif
#define SQL_API_JDBC                    9999
#define SQL_API_SQLTABLES_JDBC          SQL_API_SQLTABLES + SQL_API_JDBC


#define CRTASIZE    1024
// Global Variables

SRVR_GLOBAL_Def     *srvrGlobal = NULL;
long                *TestPointArray = NULL;
__thread SQLMODULE_ID       nullModule;
__thread SQLDESC_ITEM       gDescItems[NO_OF_DESC_ITEMS];
__thread char               CatalogNm[MAX_ANSI_NAME_LEN+1];
__thread char               SchemaNm[MAX_ANSI_NAME_LEN+1];
__thread char               TableNm[MAX_ANSI_NAME_LEN+1];
__thread char               ColumnName[MAX_ANSI_NAME_LEN+1];
__thread char               ColumnHeading[MAX_ANSI_NAME_LEN+1];

// Linux port - ToDo
// Added dummy functions for transaction support (originally in pThreadsSync.cpp - we are not including that)
// Changing the method name (capitalize first letter) since SQL already has functions with same name results in a multiple definitions error
short beginTransaction(long *transTag)
{
    return 0;
}

short resumeTransaction(long transTag)
{
    return 0;
}

short endTransaction(void)
{
    return 0;
}

short abortTransaction(void)
{
    return 0;
}

// +++ T2_REPO
void getCurrentTime(char* timestamp)
{
    short currentTime[20];
    TIME(currentTime);
    snprintf(timestamp,25,"%d/%d/%d %d:%d:%d.%d",currentTime[0],currentTime[1],currentTime[2],currentTime[3],currentTime[4],currentTime[5],currentTime[6]);
}
void translateToUTF8(Int32 inCharset, char* inStr, Int32 inStrLen, char* outStr, Int32 outStrMax)
{
    Int32 err=0;
    Int32 charset = SQLCHARSETCODE_UNKNOWN;
    Int32 length = _min (inStrLen, outStrMax);
    Int32 outStrLen = 0;
    void *firstUntranslatedChar = 0;
    unsigned int translatedCharCnt = 0;
    if (inStr == NULL || length <= 0)
    {
        if (outStr != NULL) outStr[0] = '\0';
        return;
    }
    switch (inCharset)
    {
    case SQLCHARSETCODE_ISO88591:
        charset = SQLCONVCHARSETCODE_ISO88591;
        break;
    case SQLCHARSETCODE_KSC5601:
    case SQLCHARSETCODE_MB_KSC5601:
        charset = SQLCONVCHARSETCODE_KSC;
        break;
    case SQLCHARSETCODE_SJIS:
        charset = SQLCONVCHARSETCODE_SJIS;
        break;
    case SQLCHARSETCODE_UCS2:
        charset = SQLCONVCHARSETCODE_UTF16;
        break;
    case SQLCHARSETCODE_EUCJP:
        charset = SQLCONVCHARSETCODE_EUCJP;
        break;
    case SQLCHARSETCODE_BIG5:
        charset = SQLCONVCHARSETCODE_BIG5;
        break;
    case SQLCHARSETCODE_GB18030:
        charset = SQLCONVCHARSETCODE_GB18030;
        break;
    case SQLCHARSETCODE_UTF8:
        charset = SQLCONVCHARSETCODE_UTF8;
        break;
    case SQLCHARSETCODE_GB2312:
        charset = SQLCONVCHARSETCODE_BIG5;
        break;
    default:
        charset = SQLCONVCHARSETCODE_UNKNOWN;
        break;
    };
    if (charset != SQLCONVCHARSETCODE_UNKNOWN)
    {
        err = SQL_EXEC_LocaleToUTF8 (charset,
                                     inStr,
                                     inStrLen,
                                     outStr,
                                     outStrMax,
                                     &firstUntranslatedChar,
                                     &outStrLen,
                                     TRUE,
                                     (Int32*)&translatedCharCnt); //for 64-bit the Executor is going to change it to a int
        if (err != 0)
        {
            if (err == -2 && outStrMax <= outStrLen) // CNV_ERR_BUFFER_OVERRUN
            {
                outStr[outStrMax-1]= '\0';
            }
            else
            {
                memcpy(outStr, inStr, length-1);
                outStr[length-1] = '\0';
            }
        }
    }
    else
    {
        memcpy(outStr, inStr, length-1);
        outStr[length-1] = '\0';
    }
    return;
}

char* _i64toa( long long n, char *buff, int base )
{
   char t[100], *c=t, *f=buff;
   long d;
    int bit;

   if (base == 10) {
     if (n < 0) {
        *(f++) = '-';
        n = -n;
     }

     while (n != 0) {
        d = n % base;
        if (d < 0) d = -d;
        *(c++) = d + '0';
        n = n / base;
     }
   }

   else {
     short bitlen = 64;

     if (base == 2) bit = 1;
     else if (base == 8) bit = 3;
     else if (base == 16) bit = 4;
     { base = 16; bit =4;} // printf("Base value unknown!\n");

     while (bitlen != 0) {
        d = (n  & (base-1));
        *(c++) = d < 10 ? d + '0' : d - 10 + 'A';
        n =  n >> bit;
        bitlen -= bit;
     }

   }

   c--;

   while (c >= t) *(f++) = *(c--);

   if (buff == f)
       *(f++) = '0';
   *f = '\0';
   return buff;
}
//

int initSqlCore(int argc, char *argv[])
{
    FUNCTION_ENTRY("initSqlCore",(NULL));
    short   error;
    int     retcode = 0;

    gDescItems[0].item_id = SQLDESC_TYPE;
    gDescItems[1].item_id = SQLDESC_OCTET_LENGTH;
    gDescItems[2].item_id = SQLDESC_PRECISION;
    gDescItems[3].item_id = SQLDESC_SCALE;
    gDescItems[4].item_id = SQLDESC_NULLABLE;
    gDescItems[5].item_id = SQLDESC_PARAMETER_MODE;
    gDescItems[6].item_id = SQLDESC_INT_LEAD_PREC;
    gDescItems[7].item_id = SQLDESC_DATETIME_CODE;
    gDescItems[8].item_id = SQLDESC_CHAR_SET;
    gDescItems[9].item_id = SQLDESC_TYPE_FS;
    gDescItems[10].item_id = SQLDESC_CATALOG_NAME;
    gDescItems[10].string_val = CatalogNm;
    gDescItems[10].num_val_or_len = MAX_ANSI_NAME_LEN+1;
    gDescItems[11].item_id = SQLDESC_SCHEMA_NAME;
    gDescItems[11].string_val = SchemaNm;
    gDescItems[11].num_val_or_len = MAX_ANSI_NAME_LEN+1;
    gDescItems[12].item_id = SQLDESC_TABLE_NAME;
    gDescItems[12].string_val = TableNm;
    gDescItems[12].num_val_or_len = MAX_ANSI_NAME_LEN+1;
    gDescItems[13].item_id = SQLDESC_NAME;
    gDescItems[13].string_val = ColumnName;
    gDescItems[13].num_val_or_len = MAX_ANSI_NAME_LEN+1;
    gDescItems[14].item_id = SQLDESC_HEADING;
    gDescItems[14].string_val = ColumnHeading;
    gDescItems[14].num_val_or_len = MAX_ANSI_NAME_LEN+1;
    gDescItems[15].item_id = SQLDESC_VC_IND_LENGTH;

/*
    // Seaquest related - Linux port
    // Initialize seabed
    int sbResult;
    char buffer[FILENAME_MAX] = {0};
    bzero(buffer, sizeof(buffer));

    sbResult = file_init_attach(&argc, &argv, true, buffer);
    if(sbResult != XZFIL_ERR_OK){
        printf( "initSqlCore::file_init_attach()....FAILED. sbResult: %d\n\n", sbResult);
        abort();
    }
    printf( "initSqlCore::file_init_attach()....COMPLETED. sbResult: %d\n\n", sbResult);

    sbResult = file_mon_process_startup(true);
    if(sbResult != XZFIL_ERR_OK){
        printf( "initSqlCore::file_mon_process_startup()....FAILED. sbResult: %d\n\n", sbResult);
        abort();
    }
    printf( "initSqlCore::file_mon_process_startup()....COMPLETED. sbResult: %d\n\n", sbResult);

    msg_mon_enable_mon_messages(true);
    printf( "initSqlCore::msg_mon_enable_mon_messages()....COMPLETED. sbResult: %d\n\n", sbResult);
    // End Seaquest related
*/
    // Make sure you change NO_OF_DESC_ITEMS if you add any more items
    FUNCTION_RETURN_NUMERIC(retcode,(NULL));
}



SRVR_STMT_HDL *getSrvrStmt(long dialogueId,
                           long stmtId,
                           long *sqlcode)
{
    FUNCTION_ENTRY("getSrvrStmt",("dialogueId=0x%08x, stmtId=0x%08x, sqlcode=0x%08x",
        dialogueId,
        stmtId,
        sqlcode));
    SQLRETURN rc;
    //Soln. No.: 10-100202-7923
    SRVR_STMT_HDL *pSrvrStmt=NULL;

    SRVR_CONNECT_HDL *pConnect=NULL;
    //End of Soln. No.: 10-100202-7923

    if (dialogueId == 0)
    {
        *sqlcode = DIALOGUE_ID_NULL_ERROR;
        FUNCTION_RETURN_PTR(NULL,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
    }
    if (stmtId == 0)
    {
        *sqlcode = STMT_ID_NULL_ERROR;
        FUNCTION_RETURN_PTR(NULL,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
    }

    //Soln. No.: 10-100202-7923

    /*pSrvrStmt = (SRVR_STMT_HDL *)stmtId;*/
    pConnect = (SRVR_CONNECT_HDL *)dialogueId;
    pSrvrStmt = pConnect->getSrvrStmt(dialogueId, stmtId, sqlcode);
    if(pSrvrStmt != NULL)
    //End Soln.: 10-100202-7923
    {
        if (pSrvrStmt->dialogueId != dialogueId)
        {
            *sqlcode = STMT_ID_MISMATCH_ERROR;
            FUNCTION_RETURN_PTR(NULL,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
        }
    }
    rc = pConnect->switchContext(sqlcode);
    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        break;
    default:
        FUNCTION_RETURN_PTR(NULL,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
    }
    //Soln. No.: 10-100202-7923
    if(pSrvrStmt != NULL)
    //End of Soln. No.: 10-100202-7923
    {
        pConnect->setCurrentStmt(pSrvrStmt);
    }
    FUNCTION_RETURN_PTR(pSrvrStmt,(NULL));
}

SRVR_STMT_HDL *getInternalSrvrStmt(long dialogueId,
		const char* stmtLabel,
		long* sqlcode)
{
	FUNCTION_ENTRY("getInternalSrvrStmt",("dialogueId=0x%08x, stmtLabel=0x%08x, sqlcode=0x%08x",
				dialogueId,
				stmtLabel,
				sqlcode));

	SQLRETURN rc;
	SRVR_STMT_HDL *pSrvrStmt=NULL;

	SRVR_CONNECT_HDL *pConnect=NULL;

	if (dialogueId == 0)
	{
		*sqlcode = DIALOGUE_ID_NULL_ERROR;
		FUNCTION_RETURN_PTR(NULL,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
	}
	if (stmtLabel == NULL)
	{
		*sqlcode = STMT_ID_NULL_ERROR;
		FUNCTION_RETURN_PTR(NULL,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
	}

	pConnect = (SRVR_CONNECT_HDL *)dialogueId;
	pSrvrStmt = pConnect->getInternalSrvrStmt(dialogueId, stmtLabel, sqlcode);
	if(pSrvrStmt != NULL)
	{
		if (pSrvrStmt->dialogueId != dialogueId)
		{
			*sqlcode = STMT_ID_MISMATCH_ERROR;
			FUNCTION_RETURN_PTR(NULL,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
		}
	}
    else
    {
        FUNCTION_RETURN_PTR(NULL,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
    }

	rc = pConnect->switchContext(sqlcode);
	switch (rc)
	{
		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO:
			break;
		default:
			FUNCTION_RETURN_PTR(NULL,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
	}

	if(pSrvrStmt != NULL)
	{
		pConnect->setCurrentStmt(pSrvrStmt);
	}
	FUNCTION_RETURN_PTR(pSrvrStmt,(NULL));
}

SRVR_STMT_HDL *createSrvrStmt(long dialogueId,
                              const char *stmtLabel,
                              long  *sqlcode,
                              const char *moduleName,
                              long moduleVersion,
                              long long moduleTimestamp,
                              short sqlStmtType,
                              BOOL  useDefaultDesc,
                              BOOL    internalStmt,
                              long stmtId,
							  short sqlQueryType,
							  Int32  resultSetIndex,
							  SQLSTMT_ID* callStmtId)
{
    FUNCTION_ENTRY("createSrvrStmt",(""));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  dialogueId=%ld, stmtLabel=%s",
        dialogueId,
        DebugString(stmtLabel)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  sqlcode=0x%08x",
        sqlcode));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  moduleName=%s",
        DebugString(moduleName)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  moduleVersion=%ld, moduleTimestamp=%s",
        moduleVersion,
        DebugTimestampStr(moduleTimestamp)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  sqlStmtType=%s, useDefaultDesc=%d",
        CliDebugSqlStatementType(sqlStmtType),
        useDefaultDesc));

    SQLRETURN rc;
    SRVR_CONNECT_HDL *pConnect;
    SRVR_STMT_HDL *pSrvrStmt;

    if (dialogueId == 0)
    {
        *sqlcode = DIALOGUE_ID_NULL_ERROR;
        FUNCTION_RETURN_PTR(NULL,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
    }
    pConnect = (SRVR_CONNECT_HDL *)dialogueId;
    rc = pConnect->switchContext(sqlcode);
    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        break;
    default:
        FUNCTION_RETURN_PTR(NULL,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
    }
    pSrvrStmt = pConnect->createSrvrStmt(stmtLabel, sqlcode, moduleName, moduleVersion, moduleTimestamp,
        sqlStmtType, useDefaultDesc,internalStmt,stmtId, sqlQueryType, resultSetIndex, callStmtId);
    FUNCTION_RETURN_PTR(pSrvrStmt,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
}

SRVR_STMT_HDL *createSpjrsSrvrStmt(SRVR_STMT_HDL *callpSrvrStmt,
                                   long dialogueId,
                                   const char *stmtLabel,
                                   long *sqlcode,
                                   const char *moduleName,
                                   long moduleVersion,
                                   long long moduleTimestamp,
                                   short    sqlStmtType,
                                   long RSindex,
                                   const char *RSstmtLabel,
                                   BOOL useDefaultDesc)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_STMT,"createSpjrsSrvrStmt",
        ("dialogueId=0x%08x, stmtLabel=%s, sqlcode=0x%08x, moduleName=%s, moduleVersion=%ld, moduleTimestamp=%s,  sqlStmtType=%s, useDefaultDesc=%d",
        dialogueId,
        DebugString(stmtLabel),
        sqlcode,
        DebugString(moduleName),
        moduleVersion,
        DebugTimestampStr(moduleTimestamp),
        CliDebugSqlStatementType(sqlStmtType),
        useDefaultDesc));
    SQLRETURN rc;
    SRVR_CONNECT_HDL *pConnect;
    SRVR_STMT_HDL *pSrvrStmt;

    if (dialogueId == 0)
    {
        *sqlcode = DIALOGUE_ID_NULL_ERROR;
        FUNCTION_RETURN_PTR(NULL,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
    }
    pConnect = (SRVR_CONNECT_HDL *)dialogueId;
    rc = pConnect->switchContext(sqlcode);
    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        break;
    default:
        FUNCTION_RETURN_PTR(NULL,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
    }
    pSrvrStmt = pConnect->createSpjrsSrvrStmt(callpSrvrStmt, stmtLabel, sqlcode, moduleName,
        moduleVersion, moduleTimestamp,
        sqlStmtType, RSindex, RSstmtLabel, useDefaultDesc);
    FUNCTION_RETURN_PTR(pSrvrStmt,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
}


void removeSrvrStmt(long dialogueId, long stmtId)
{
    FUNCTION_ENTRY("removeSrvrStmt",("dialogueId=0x%08x, stmtId=0x%08x",
        dialogueId,
        stmtId));
    SQLRETURN   rc;
    long        sqlcode;

    SRVR_CONNECT_HDL *pConnect;

    if (dialogueId == 0) FUNCTION_RETURN_VOID((NULL));
    pConnect = (SRVR_CONNECT_HDL *)dialogueId;
    rc = pConnect->switchContext(&sqlcode);
    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        break;
    default:
        FUNCTION_RETURN_VOID((NULL));
    }
    pConnect->removeSrvrStmt((SRVR_STMT_HDL *)stmtId);
    FUNCTION_RETURN_VOID((NULL));
}
// Assuming outName is of sufficient size for efficiency
void convertWildcard(unsigned long metadataId, BOOL isPV, const char *inName, char *outName)
{
    FUNCTION_ENTRY("ConvertWildcard",("metadataId=%ld, isPV=%d, inName=%s, outName=0x%08x",
        metadataId,
        isPV,
        DebugString(inName),
        outName));
    char *in = (char *)inName;
    char *out = (char *)outName;
    BOOL    quoted = FALSE;
    BOOL    leadingBlank = TRUE;
    char    *trailingBlankPtr = NULL;


    if (metadataId) // Treat the argument as Identifier Argument
    {
        if (*in == '"')
        {
            in++;
            quoted = TRUE;
            while (*in != '\0' && *in == ' ') // Remove the leading blanks
                in++;
        }
        while (*in != '\0')
        {
            if (*in == ' ')
            {
                if (! leadingBlank)
                {
                    if (! trailingBlankPtr)
                        trailingBlankPtr = out; // Store the pointer to begining of the blank
                }
                *out++ = *in++;
            }
            else
            {
                leadingBlank = FALSE;
                if (quoted)
                {
                    if (*in != '"')
                    {
                        trailingBlankPtr = NULL;
                        *out++ = *in++;
                    }
                    else
                    {
                        in++;           // Assuming this may be last character
                        break;
                    }
                }
                else
                {
                    trailingBlankPtr = NULL;
                    switch (*in)
                    {
                    case '\\':      // Escape Character
                        *out++ = *in++;
                        break;
                    case '_':
                    case '%':
                        //*out++ = '\\';
                        break;
                    default:
                        break;
                    }
                    if (*in != '\0')
                        *out++ = toupper(*in++);
                }
            }
        }
        if (trailingBlankPtr)
            *trailingBlankPtr = '\0';
        else
            *out = '\0';
    }
    else
    {
        if (isPV)
            strcpy(outName, inName);
        else
        {
            if (*in == '"')
            {
                in++;
                quoted = TRUE;
            }
            while (*in != '\0')
            {
                if (quoted)
                {
                    if (*in != '"')
                        *out++ = *in++;
                    else
                    {
                        in++;       // Assuming this may be last character
                        break;
                    }
                }
                else
                {
                    switch (*in)
                    {
                    case '\\':      // Escape Character
                        *out++ = *in++;
                        break;
                    case '_':
                    case '%':
                        //*out++ = '\\';
                        break;
                    default:
                        break;
                    }
                    if (*in != '\0')
                        *out++ = *in++;
                }
            }
            *out = '\0';
        }
    }

    FUNCTION_RETURN_VOID(("inName=%s, outName=%s",
        DebugString(inName),
        DebugString(outName)));
}

void convertWildcardNoEsc(unsigned long metadataId, BOOL isPV, const char *inName, char *outName)
{
    FUNCTION_ENTRY("convertWildcardNoEsc",("metadataId=%lu, isPV=%d, inName=%s, outName=0x%08x",
        metadataId,
        isPV,
        DebugString(inName),
        outName));
    char *in = (char *)inName;
    char *out = (char *)outName;
    BOOL    quoted = FALSE;
    BOOL    leadingBlank = TRUE;
    char    *trailingBlankPtr = NULL;


    if (metadataId) // Treat the argument as Identifier Argument
    {
        if (*in == '"')
        {
            in++;
            quoted = TRUE;
            while (*in != '\0' && *in == ' ') // Remove the leading blanks
                in++;
        }
        while (*in != '\0')
        {
            if (*in == ' ')
            {
                if (! leadingBlank)
                {
                    if (! trailingBlankPtr)
                        trailingBlankPtr = out; // Store the pointer to begining of the blank
                }
                *out++ = *in++;
            }
            else
            {
                leadingBlank = FALSE;
                if (quoted)
                {
                    if (*in != '"')
                    {
                        trailingBlankPtr = NULL;
                        *out++ = *in++;
                    }
                    else
                    {
                        in++;           // Assuming this may be last character
                        break;
                    }
                }
                else
                {
                    trailingBlankPtr = NULL;
                    *out++ = toupper(*in++);
                }
            }
        }
        if (trailingBlankPtr)
            *trailingBlankPtr = '\0';
        else
            *out = '\0';
    }
    else
    {
        if (isPV)
            strcpy(outName, inName);
        else
        {
            if (*in == '"')
            {
                in++;
                quoted = TRUE;
            }
            while (*in != '\0')
            {
                if (quoted)
                {
                    if (*in != '"')
                        *out++ = *in++;
                    else
                    {
                        in++;       // Assuming this may be last character
                        break;
                    }
                }
                else
                {
                    if (*in != '\\') // Skip the Escape character since application is escapes with \.
                        *out++ = *in++;
                    else
                        in++;
                }
            }
            *out = '\0';
        }
    }
    FUNCTION_RETURN_VOID(("inName=%s, outName=%s",
        DebugString(inName),
        DebugString(outName)));
}

/* This function is used to suppress wildcard escape sequence since we don't support wild card characters
in CatalogNames except in SQLTables (for certain conditions).
This function returns an error when the wildcard character % is in the input string. The wildcard character
'_' is ignored and treated like an ordinary character
*/

// Assuming outName is of sufficient size for efficiency
BOOL checkIfWildCard(const char *inName, char *outName)
{
    FUNCTION_ENTRY("checkIfWildCard",("inName=%s, outName=0x%08x",
        DebugString(inName),
        outName));
    char *in = (char *)inName;
    char *out = (char *)outName;
    BOOL    rc = TRUE;

    while (*in != '\0')
    {
        switch(*in)
        {
        case '%':
            rc = FALSE;
        case '\\':
            if ((*(in+1) == '_') || (*(in+1) == '%')) // Dont copy '\'
                in++;
            break;
        default:
            break;
        }
        *out++ = *in++;

    }
    *out = '\0';
    FUNCTION_RETURN_NUMERIC(rc,("inName=%s, outName=%s",
        DebugString(inName),
        DebugString(outName)));
}

//#ifdef NSK_PLATFORM       // Linux port - Todo Used only in SrvrSmd.cpp
BOOL writeServerException( short retCode
                          , ExceptionStruct *exception_
                          , ExceptionStruct *prepareException
                          , ExceptionStruct *executeException
                          , ExceptionStruct *fetchException)
{
    FUNCTION_ENTRY("writeServerException",("retCode=%s, exception_=0x%08x, prepareException=0x%08x, executeException=0x%08x, fetchException=0x%08x",
        CliDebugSqlError(retCode),
        exception_,
        prepareException,
        executeException,
        fetchException));
    switch (retCode) {
    case STMT_LABEL_NOT_FOUND:
        exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_SMD_STMT_LABEL_NOT_FOUND;
        FUNCTION_RETURN_NUMERIC(FALSE,("FALSE"));
    case DATA_TYPE_ERROR:
        exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNSUPPORTED_SMD_DATA_TYPE;
        FUNCTION_RETURN_NUMERIC(FALSE,("FALSE"));
    case DATA_ERROR:
        exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_DATA_ERROR;
        FUNCTION_RETURN_NUMERIC(FALSE,("FALSE"));
    case PROGRAM_ERROR:
        exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_PREPARE_FAILED;
        FUNCTION_RETURN_NUMERIC(FALSE,("FALSE"));
    case PREPARE_EXCEPTION:
        switch (prepareException->exception_nr) {
    case CEE_SUCCESS:
        break;
    case odbc_SQLSvc_Prepare_SQLError_exn_:
        exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_SQLError_exn_;
        exception_->u.SQLError = prepareException->u.SQLError;
        FUNCTION_RETURN_NUMERIC(FALSE,("FALSE"));
    case odbc_SQLSvc_Prepare_ParamError_exn_:
        exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = prepareException->u.ParamError.ParamDesc;
        FUNCTION_RETURN_NUMERIC(FALSE,("FALSE"));
    default:
        exception_->exception_nr = prepareException->exception_nr;
        exception_->exception_detail = prepareException->exception_detail;
        FUNCTION_RETURN_NUMERIC(FALSE,("FALSE"));
        }
        break;
    case EXECUTE_EXCEPTION:
        switch (executeException->exception_nr) {
    case CEE_SUCCESS:
        break;
    case odbc_SQLSvc_ExecuteN_SQLNoDataFound_exn_:
        break;
    case odbc_SQLSvc_ExecuteN_SQLError_exn_:
        exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_SQLError_exn_;
        exception_->u.SQLError = executeException->u.SQLError;
        FUNCTION_RETURN_NUMERIC(FALSE,("FALSE"));
    case odbc_SQLSvc_ExecuteN_ParamError_exn_:
        exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = executeException->u.ParamError.ParamDesc;
        FUNCTION_RETURN_NUMERIC(FALSE,("FALSE"));
    case odbc_SQLSvc_ExecuteN_SQLInvalidHandle_exn_:
        exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_SQLInvalidHandle_exn_;
        exception_->u.SQLInvalidHandle.sqlcode = executeException->u.SQLInvalidHandle.sqlcode;
        FUNCTION_RETURN_NUMERIC(FALSE,("FALSE"));
    default:
        exception_->exception_nr = executeException->exception_nr;
        exception_->exception_detail = executeException->exception_detail;
        FUNCTION_RETURN_NUMERIC(FALSE,("FALSE"));
        }
        break;
    case FETCH_EXCEPTION:
        switch (fetchException->exception_nr) {
    case CEE_SUCCESS:
        break;
    case odbc_SQLSvc_FetchN_SQLNoDataFound_exn_:
        break;
    case odbc_SQLSvc_FetchN_SQLError_exn_:
        exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_SQLError_exn_;
        exception_->u.SQLError = fetchException->u.SQLError;
        FUNCTION_RETURN_NUMERIC(FALSE,("FALSE"));
    case odbc_SQLSvc_FetchN_ParamError_exn_:
        exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = fetchException->u.ParamError.ParamDesc;
        FUNCTION_RETURN_NUMERIC(FALSE,("FALSE"));
    default:
        exception_->exception_nr = fetchException->exception_nr;
        exception_->exception_detail = fetchException->exception_detail;
        FUNCTION_RETURN_NUMERIC(FALSE,("FALSE"));
        }
        break;
    default:
        FUNCTION_RETURN_NUMERIC(FALSE,("FALSE"));
        break;
    }
    FUNCTION_RETURN_NUMERIC(TRUE,("TRUE"));
}
//#endif

#ifdef _FASTPATH
short setParamValue(long dataType, BYTE *dataPtr, BYTE *indPtr, long allocLength, const char *dataValue)
{
    FUNCTION_ENTRY("setParamValue",("dataType=%s, dataPtr=0x%08x, indPtr=0x%08x, allocLength=%ld, dataValue=0x%08x",
        CliDebugSqlTypeCode(dataType),
        dataPtr,
        indPtr,
        allocLength,
        dataValue));
    short length;
    int iTmp;
    __int64 int64Tmp;
    short sTmp;
    double dTmp;

    if (dataValue != NULL)
    {
        if (indPtr != NULL)
            *(short *)indPtr = 0;
        switch (dataType)
        {
        case SQLTYPECODE_CHAR:
            length = strlen(dataValue);
            if (length > (allocLength-1))
                FUNCTION_RETURN_NUMERIC(DATA_ERROR,("DATA_ERROR"));
            memcpy(dataPtr, dataValue, length);
            memset(dataPtr, ' ', allocLength-length);
            dataPtr[allocLength-1] = '\0';
            break;
        case SQLTYPECODE_VARCHAR:
            length = strlen(dataValue);
            if (length > (allocLength-1))
                FUNCTION_RETURN_NUMERIC(DATA_ERROR,("DATA_ERROR"));
            memcpy(dataPtr, dataValue, length);
            dataPtr[length] = '\0';
            break;
        case SQLTYPECODE_VARCHAR_WITH_LENGTH:
        case SQLTYPECODE_VARCHAR_LONG:
        case SQLTYPECODE_DATETIME:
        case SQLTYPECODE_INTERVAL:
            length = strlen(dataValue);
            if (length > allocLength-(1+2)) // 2 Bytes for Length
                FUNCTION_RETURN_NUMERIC(DATA_ERROR,("DATA_ERROR"));
            memcpy(dataPtr, (void *)&length, sizeof(length));
            memcpy(dataPtr+sizeof(length), dataValue, length);
            dataPtr[length+2] = '\0';
            break;
        case SQLTYPECODE_SMALLINT: //5
            sTmp = (short) atoi(dataValue);
            *((SWORD *) dataPtr)= (SWORD)sTmp;
            break;
        case SQLTYPECODE_INTEGER: // 4
            iTmp = atoi(dataValue);
            *((SDWORD *) dataPtr)= (SDWORD)iTmp;
            break;
        case SQLTYPECODE_LARGEINT: // -402
            sscanf(dataValue, "%Ld", dataPtr);
            break;
        default:
            FUNCTION_RETURN_NUMERIC(DATA_TYPE_ERROR,("DATA_TYPE_ERROR"));
        }

    }
    else
    {
        if (indPtr != NULL)
            *(short *)indPtr = -1;
        else
            FUNCTION_RETURN_NUMERIC(DATA_ERROR,("DATA_ERROR"));
    }
    FUNCTION_RETURN_NUMERIC(0,(NULL));
}
#endif
short do_ExecSMD(
                 /* In  */ void *objtag_
                 , /* In    */ const CEE_handle_def *call_id_
                 , /* Out   */ ExceptionStruct *executeException
                 , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
                 , /* In    */ long dialogueId
                 , /* In    */ const char *tableTypeList
                 , /* In    */ unsigned long metadataId
                 , /* In    */ short APIType
                 , /* In    */ const char *stmtLabel
                 , /* In    */ short sqlStmtType
                 , /* In    */const char *catalogNm
                 , /* In    */ const char *schemaNm
                 , /* In    */ const char *tableNm
                 , /* In    */ const char *columnNm
                 , /* In    */ const char *tableParam[]
, /* In */ const char *inputParam[]
, /* Out   */ SQLItemDescList_def *outputDesc
, /* Out   */ long *stmtId
)
{
    FUNCTION_ENTRY("do_ExecSMD",("objtag_=%ld, call_id_=0x%08x, executeException=0x%08x, sqlWarning=0x%08x, dialogueId=%ld, stmtLabel=%s, sqlStmtType=%s, tableParam=0x%08x, inputParam=0x%08x, outputDesc=0x%08x, stmtId=0x%08x",
        objtag_,
        call_id_,
        executeException,
        sqlWarning,
        dialogueId,
        APIType,
        DebugString(stmtLabel),
        CliDebugSqlStatementType(sqlStmtType),
        tableParam,
        inputParam,
        outputDesc,
        stmtId));

    SRVR_STMT_HDL       *pSrvrStmt;
    long                rowsAffected;
    SQLItemDesc_def     *SQLItemDesc;
    unsigned long       curParamNo;
    int                 allocLength;
    long                retcode;
    SQLRETURN           rc;
    short               indValue;
    BOOL                tableParamDone;
    unsigned long       index;
    unsigned long totalSize=20000;
    char lc_tableTypeList[MAX_ANSI_NAME_LEN+1];
    char *token;
    char* saveptr;
    long                sqlcode;
    short               holdability;
    long                queryTimeout;
    odbc_SQLSvc_SQLError sqlError;
    char *odbcAppVersion = "3";
    CLEAR_ERROR(sqlError);

    char catalogNmNoEsc[MAX_ANSI_NAME_LEN+1];
    char schemaNmNoEsc[MAX_ANSI_NAME_LEN+1];
    char tableNmNoEsc[MAX_ANSI_NAME_LEN+1];
    char columnNmNoEsc[MAX_ANSI_NAME_LEN+1];

    char expCatalogNm[MAX_ANSI_NAME_LEN+1];
    char expSchemaNm[MAX_ANSI_NAME_LEN+1];
    char expTableNm[MAX_ANSI_NAME_LEN+1];
    char expColumnNm[MAX_ANSI_NAME_LEN+1];

    char tableName1[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3];
    char tableName2[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3];
    char tableName3[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3];

    // Setup module filenames for MX metadata
    //Module version is changed from 0 to "SQLCLI_ODBC_MODULE_VERSION" R3.0
    pSrvrStmt = createSrvrStmt(dialogueId,
    stmtLabel,
    &sqlcode,
    NULL,
    0,
    0,
    sqlStmtType,
    false,true);

    *stmtId = (long)pSrvrStmt;
    SQLValue_def *sqlString = NULL;
    // allocate space for the sqlString
    MEMORY_ALLOC(sqlString, SQLValue_def)
    MEMORY_ALLOC_ARRAY(sqlString->dataValue._buffer,unsigned char, totalSize)
    pSrvrStmt->sqlStmtType = TYPE_SELECT;

    switch(APIType)
            {
        case SQL_API_SQLTABLES :
        case SQL_API_SQLTABLES_JDBC :
               if ((strcmp(catalogNm,"%") == 0) && (strcmp(schemaNm,"") == 0) && (strcmp(tableNm,"") == 0))
               {
                    strcpy(catalogNmNoEsc, SEABASE_MD_CATALOG);
                    inputParam[0] = catalogNmNoEsc;
                    inputParam[1] = inputParam[0];
                    inputParam[2] = NULL;

                        if (APIType == SQL_API_SQLTABLES)
                          {
                            snprintf((char *)sqlString->dataValue._buffer, totalSize,
"select distinct(cast('%s' as varchar(128))) TABLE_CAT, "
"cast(NULL as varchar(128) ) TABLE_SCHEM, "
"cast(NULL as varchar(128) ) TABLE_NAME, "
"cast(NULL as varchar(128) ) TABLE_TYPE, "
"cast(NULL as varchar(128)) REMARKS "
"from TRAFODION.\"_MD_\".objects "
"where CATALOG_NAME = '%s' "
"FOR READ UNCOMMITTED ACCESS ORDER BY 4,1,2,3;",
                            inputParam[0], inputParam[1]);
                          }
                          else
                          {
                            snprintf((char *)sqlString->dataValue._buffer, totalSize,
"select distinct(cast('%s' as varchar(128) )) TABLE_CAT "
"from TRAFODION.\"_MD_\".objects "
"where CATALOG_NAME = '%s' "
"FOR READ UNCOMMITTED ACCESS ORDER BY 1;",
                            inputParam[0], inputParam[1]);
                          }
                   }
                   else
                   if ((strcmp(catalogNm,"") == 0) && (strcmp(schemaNm,"%") == 0) && (strcmp(tableNm,"") == 0))
                         {
                              convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
                              strcpy(catalogNmNoEsc, SEABASE_MD_CATALOG);
                              inputParam[0] = catalogNmNoEsc;
                              inputParam[1] = inputParam[0];
                              inputParam[2] = (char*) schemaNm;
                              inputParam[3] = expSchemaNm;
                              inputParam[4] = NULL;

                                      if (APIType == SQL_API_SQLTABLES)
                                      {
                                          snprintf((char *)sqlString->dataValue._buffer, totalSize,
         "select distinct cast(NULL as varchar(128) ) TABLE_CAT, "
           "cast(trim(SCHEMA_NAME) as varchar(128) ) TABLE_SCHEM, "
           "cast(NULL as varchar(128) ) TABLE_NAME, "
           "cast(NULL as varchar(128) ) TABLE_TYPE, "
           "cast(NULL as varchar(128)) REMARKS "
         "from TRAFODION.\"_MD_\".objects "
         "where "
           "(CATALOG_NAME = '%s' or "
           " CATALOG_NAME LIKE '%s' ESCAPE '\\') "
           "and (SCHEMA_NAME = '%s' or "
           "SCHEMA_NAME LIKE '%s' ESCAPE '\\') "
         "FOR READ UNCOMMITTED ACCESS ORDER BY 4,1,2,3;",
                                         inputParam[0], inputParam[1], inputParam[2],
                                         inputParam[3]);
                                      }
                                      else
                                      {
                                          snprintf((char *)sqlString->dataValue._buffer, totalSize,
         "select distinct "
           "cast(trim(SCHEMA_NAME) as varchar(128) ) TABLE_SCHEM, "
           "cast(trim(CATALOG_NAME) as varchar(128) ) TABLE_CATALOG "
         "from TRAFODION.\"_MD_\".objects "
         "where "
           "(CATALOG_NAME = '%s' or "
           " CATALOG_NAME LIKE '%s' ESCAPE '\\') "
           "and (SCHEMA_NAME = '%s' or "
           "SCHEMA_NAME LIKE '%s' ESCAPE '\\') "
         "FOR READ UNCOMMITTED ACCESS ORDER BY 2;",
                                         inputParam[0], inputParam[1], inputParam[2],
                                         inputParam[3]);
                                      }
                               }
                                else
                                if ((strcmp(catalogNm,"") == 0) && (strcmp(schemaNm,"")
        == 0) && (strcmp(tableNm,"") == 0) && (strcmp(tableTypeList,"%") == 0))
                                {
                                        strcpy(catalogNmNoEsc, "%");
                                        strcpy(schemaNmNoEsc, "%");
                                        strcpy(tableNmNoEsc, "%");
                                        tableParam[0] = NULL;
                                        inputParam[0] = NULL;
                                        snprintf((char *)sqlString->dataValue._buffer, totalSize,
         "select cast(NULL as varchar(128) ) TABLE_CAT,"
            "cast(NULL as varchar(128) ) TABLE_SCHEM, "
            "cast(NULL as varchar(128) ) TABLE_NAME,"
            "trim(TABLE_TYPE) TABLE_TYPE,"
            "cast(NULL as varchar(128)) REMARKS "
         " from (VALUES "
                 "('TABLE'),"
                 "('SYSTEM TABLE'),"
                 "('VIEW'))"
             " tp (\"TABLE_TYPE\")"
         " FOR READ UNCOMMITTED ACCESS ORDER BY 4,1,2,3;");
                                }
                                else
                                {
                                        if (tableNm[0] == '\0')
                                                strcpy((char *)tableNmNoEsc,"%");
                                        /*if (! checkIfWildCard(catalogNm, expCatalogNm))
                                        {
                                                exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
                                                exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
                                                goto MapException;
                                        }*/
                                        if (strcmp(catalogNm,"") == 0) // If catalog empty default to system catalog
                                                strcpy(tableName1,SEABASE_MD_CATALOG);
                                        else
                                        {
                                                strncpy(tableName1,catalogNm, sizeof(tableName1));
                                                tableName1[sizeof(tableName1)-1] = 0;
                                        }
                                        tableParam[0] = tableName1;
                                        tableParam[1] = NULL;
                                        convertWildcardNoEsc(metadataId, TRUE, schemaNm,schemaNmNoEsc);
                                        convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
                                        if (tableNm[0] == '\0')
                                        {
                                        convertWildcardNoEsc(metadataId, TRUE, tableNmNoEsc, tableNmNoEsc);
                                        convertWildcard(metadataId, TRUE, tableNmNoEsc, expTableNm);
                                        }
                                        else
                                        {
                                        convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
                                        convertWildcard(metadataId, TRUE, tableNm, expTableNm);
                                        }

                                        inputParam[0] = schemaNmNoEsc;
                                        inputParam[1] = expSchemaNm;
                                        inputParam[2] = tableNmNoEsc;
                                        inputParam[3] = expTableNm;
                                        if (tableTypeList == NULL || strlen(tableTypeList) == 0 || strcmp(tableTypeList,"%") == 0)
                                        {
                                                inputParam[4]  = "UT"; // User Table
                                                inputParam[5]  = "BT";
                                                inputParam[6]  = "VI";
                                                inputParam[7]  = "SM"; // System MetaData
                                                inputParam[8]  = "BT";
                                                inputParam[9] = NULL;
                                        }
                                        else
                                        {
                                                inputParam[4]  = "";
                                                inputParam[5]  = "";
                                                inputParam[6]  = "";
                                                inputParam[7]  = "";
                                                inputParam[8]  = "";
                                                inputParam[9] = NULL;

                                                strncpy(lc_tableTypeList, tableTypeList, sizeof(lc_tableTypeList));
                                                lc_tableTypeList[sizeof(lc_tableTypeList)-1] = 0;
                                                token = strtok_r(lc_tableTypeList, " ,'", &saveptr);
                                                while (token != NULL)
                                                {
                                                        if (strcmp(token, "SYSTEM") == 0
        )
                                                        {
                                                                token = strtok_r(NULL, ",'", &saveptr);
                                                                if (token != NULL && strcmp(token, "TABLE") == 0)
                                                                {
                                                                        inputParam[7] = "SM";
                                                                        inputParam[8] = "BT";
                                                                }
                                                                else
                                                                        continue;
                                                        }
                                                        else
                                                        if (strcmp(token, "TABLE") == 0)                                                {
                                                                inputParam[4]  = "UT";
                                                                inputParam[5]  = "BT";
                                                        }
                                                        else
                                                        if (strcmp(token, "VIEW") == 0)
                                                        {
                                                                inputParam[4]  = "UT";
                                                                inputParam[6]  = "VI";
                                                        }

                                                        token = strtok_r(NULL, " ,'", &saveptr);
                                                }
                                        }

                                        if (APIType == SQL_API_SQLTABLES)
                                        {
                                            snprintf((char *)sqlString->dataValue._buffer, totalSize,
         "select cast('%s' as varchar(128) ) TABLE_CAT,"
           "cast(trim(SCHEMA_NAME) as varchar(128) ) TABLE_SCHEM,"
           "cast(trim(OBJECT_NAME) as varchar(128) ) TABLE_NAME,"
           "trim(case OBJECT_TYPE "
               "when 'BT' then 'TABLE' "
               "when 'VI' then 'VIEW' "
              "end) TABLE_TYPE,"
           "cast(NULL as varchar(128)) REMARKS "
         " from TRAFODION.\"_MD_\".OBJECTS "
         " where (SCHEMA_NAME = '%s' or "
         " trim(SCHEMA_NAME) LIKE '%s' ESCAPE '\\')"
         " and (OBJECT_NAME = '%s' or"
         " trim(OBJECT_NAME) LIKE '%s' ESCAPE '\\')"
         "  and ((SCHEMA_NAME <> '_MD_' and '%s'='UT' and OBJECT_TYPE in ('%s', '%s'))"
         "  or   (SCHEMA_NAME = '_MD_' and '%s'='SM' and OBJECT_TYPE in ('%s')))"
         " FOR READ UNCOMMITTED ACCESS ORDER BY 4, 1, 2, 3 ;",
                                           tableParam[0], inputParam[0], inputParam[1],
                                           inputParam[2], inputParam[3], inputParam[4],
                                           inputParam[5], inputParam[6], inputParam[7],
                                           inputParam[8]);

                                        }
                                        else
                                        {
                                            snprintf((char *)sqlString->dataValue._buffer, totalSize,
         "select cast('%s' as varchar(128) ) TABLE_CAT,"
          "cast(trim(SCHEMA_NAME) as varchar(128) ) TABLE_SCHEM,"
          "cast(trim(OBJECT_NAME) as varchar(128) ) TABLE_NAME,"
          "trim(case OBJECT_TYPE "
               "when 'BT' then 'TABLE' "
               "when 'VI' then 'VIEW' "
               "end) TABLE_TYPE,"
          "cast(NULL as varchar(128)) REMARKS, "
          "cast(NULL as varchar(128)) TYPE_CAT,"
          "cast(NULL as varchar(128)) TYPE_SCHEM, "
          "cast(NULL as varchar(128)) TYPE_NAME,"
          "cast(NULL as varchar(128)) SELF_REFERENCING_COL_NAME, "
          "cast(NULL as varchar(128)) REF_GENERATION"
         " from TRAFODION.\"_MD_\".OBJECTS "
         " where (SCHEMA_NAME = '%s' or "
         " trim(SCHEMA_NAME) LIKE '%s' ESCAPE '\\')"
         " and (OBJECT_NAME = '%s' or"
         " trim(OBJECT_NAME) LIKE '%s' ESCAPE '\\')"
         "  and ((SCHEMA_NAME <> '_MD_' and '%s'='UT' and OBJECT_TYPE in ('%s', '%s'))"
         "  or   (SCHEMA_NAME = '_MD_' and '%s'='SM' and OBJECT_TYPE in ('%s')))"
         " FOR READ UNCOMMITTED ACCESS ORDER BY 4, 1, 2, 3 ;",
                                           tableParam[0], inputParam[0], inputParam[1],
                                           inputParam[2], inputParam[3], inputParam[4],
                                           inputParam[5], inputParam[6], inputParam[7],
                                           inputParam[8]);

                                        }

                               }
                               break;
        case SQL_API_SQLGETTYPEINFO :
          snprintf((char *)sqlString->dataValue._buffer, totalSize,
          "select distinct TYPE_NAME TYPE_NAME,"
                "DATA_TYPE DATA_TYPE,PREC COLUMN_SIZE,"
                "LITERAL_PREFIX LITERAL_PREFIX,"
                "LITERAL_SUFFIX LITERAL_SUFFIX,"
                "CREATE_PARAMS CREATE_PARAMS,"
                "IS_NULLABLE NULLABLE,"
                "CASE_SENSITIVE CASE_SENSITIVE,"
                "SEARCHABLE SEARCHABLE,"
                "UNSIGNED_ATTRIBUTE UNSIGNED_ATTRIBUTE,"
                "FIXED_PREC_SCALE FIXED_PREC_SCALE,"
                "AUTO_UNIQUE_VALUE AUTO_UNIQUE_VALUE,"
                "LOCAL_TYPE_NAME LOCAL_TYPE_NAME,"
                "MINIMUM_SCALE MINIMUM_SCALE,"
                "MAXIMUM_SCALE MAXIMUM_SCALE,"
                "SQL_DATA_TYPE SQL_DATA_TYPE,"
                "SQL_DATETIME_SUB SQL_DATETIME_SUB,"
                "NUM_PREC_RADIX NUM_PREC_RADIX,"
                "INTERVAL_PRECISION INTERVAL_PRECISION "
         " from "
         " (VALUES "
                "(cast('BIGINT' as varchar(128)),cast(-5 as smallint), cast(19 as integer), cast (NULL as varchar(128)), cast (NULL as varchar(128)),"
                "cast (NULL as varchar(128)), cast(1 as smallint), cast(0 as smallint), cast(2 as smallint) , cast(0 as smallint), cast(0 as smallint),"
                "cast(0 as smallint), cast('LARGEINT' as varchar(128)), cast(NULL as smallint), cast(NULL as smallint), cast('LARGEINT' as varchar(128)),"
                "cast(10 as smallint), cast(19 as integer), cast(20 as integer), cast(-402 as smallint), cast(NULL as smallint), cast(NULL as smallint),"
                 "cast(0 as smallint), cast(0 as smallint), cast(3 as smallint), cast(0 as smallint)),"
                "('BIGINT SIGNED', -5, 19, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'LARGEINT', NULL, NULL, 'SIGNED LARGEINT', 10, 19, 20, -402, NULL, NULL, 0, 0, 3, 0),"
                "('CHAR', 1, 32000, '''', '''', 'max length', 1, 1, 3, NULL, 0, NULL, 'CHARACTER', NULL, NULL, 'CHARACTER', NULL, -1, -1, 1, NULL, NULL, 0, 0, 3, 0),"
                "('DATE', 91, 10, '{d ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'DATE', NULL, NULL, 'DATE', NULL, 10, 6, 9, 1, NULL, 1, 3, 3, 0),"
                "('DECIMAL', 3, 18, NULL, NULL, 'precision,scale', 1, 0, 2, 0, 0, 0, 'DECIMAL', 0, 18, 'DECIMAL', 10, -2, -3, 3, NULL, NULL, 0, 0, 3, 0),"
                "('DECIMAL SIGNED', 3, 18, NULL, NULL, 'precision,scale', 1, 0, 2, 0, 0, 0, 'DECIMAL', 0, 18, 'SIGNED DECIMAL', 10, -2, -3, 3, NULL, NULL, 0, 0, 3, 0),"
                "('DECIMAL UNSIGNED', 3, 18, NULL, NULL, 'precision,scale', 1, 0, 2, 1, 0, 0, 'DECIMAL', 0, 18, 'UNSIGNED DECIMAL', 10, -2, -3, -301, NULL, NULL, 0, 0, 3, 0),"
                "('DOUBLE PRECISION', 8, 15, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'DOUBLE', NULL, NULL, 'DOUBLE PRECISION', 2, 54, -1, 8, NULL, NULL, 0, 0, 3, 0),"
                "('DOUBLE PRECISION', 8, 15, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'DOUBLE', NULL, NULL, 'DOUBLE', 2, 54, -1, 8, NULL, NULL, 0, 0, 3, 0),"
                "('FLOAT', 6, 15, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'FLOAT', NULL, NULL, 'FLOAT', 2, -2, -1, 6, NULL, NULL, 0, 0, 3, 0),"
                "('INTEGER', 4, 10, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'INTEGER', NULL, NULL, 'INTEGER', 10, 10, -1, 4, NULL, NULL, 0, 0, 3, 0),"
                "('INTEGER SIGNED', 4, 10, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'INTEGER', NULL, NULL, 'SIGNED INTEGER', 10, 10, -1, 4, NULL, NULL, 0, 0, 3, 0),"
                "('INTEGER UNSIGNED', 4, 10, NULL, NULL, NULL, 1, 0, 2, 1, 0, 0, 'INTEGER', NULL, NULL, 'UNSIGNED INTEGER', 10, 10, -1, -401, NULL, NULL, 0, 0, 3, 0),"
                "('INTERVAL', 113, 0, '{INTERVAL ''', ''' MINUTE TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 3, 34, 100, 13, 2, 5, 6, 3, 0),"
                "('INTERVAL', 105, 0, '{INTERVAL ''', ''' MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 0, 34, 100, 5, 2, 5, 5, 3, 0),"
                "('INTERVAL', 101, 0, '{INTERVAL ''', ''' YEAR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 0, 34, 100, 1, 2, 1, 1, 3, 0),"
                "('INTERVAL', 106, 0, '{INTERVAL ''', ''' SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 0, 34, 100, 6, 2, 6, 6, 3, 0),"
                "('INTERVAL', 104, 0, '{INTERVAL ''', ''' HOUR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 0, 34, 100, 4, 2, 4, 4, 3, 0),"
                "('INTERVAL', 107, 0, '{INTERVAL ''', ''' YEAR TO MONTH}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 3, 34, 100, 7, 2, 1, 2, 3, 0),"
                "('INTERVAL', 108, 0, '{INTERVAL ''', ''' DAY TO HOUR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 3, 34, 100, 8, 2, 3, 4, 3, 0),"
                "('INTERVAL', 102, 0, '{INTERVAL ''', ''' MONTH}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 0, 34, 100, 2, 2, 2, 2, 3, 0),"
                "('INTERVAL', 111, 0, '{INTERVAL ''', ''' HOUR TO MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 3, 34, 100, 11, 2, 4, 5, 3, 0),"
                "('INTERVAL', 112, 0, '{INTERVAL ''', ''' HOUR TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 6, 34, 100, 12, 2, 4, 6, 3, 0),"
                "('INTERVAL', 110, 0, '{INTERVAL ''', ''' DAY TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 9, 34, 100, 10, 2, 3, 6, 3, 0),"
                "('INTERVAL', 109, 0, '{INTERVAL ''', ''' DAY TO MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 6, 34, 100, 9, 2, 3, 5, 3, 0),"
                "('INTERVAL', 103, 0, '{INTERVAL ''', ''' DAY}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 0, 34, 100, 3, 2, 3, 3, 3, 0),"
                "('NUMERIC', 2, 128, NULL, NULL, 'precision,scale', 1, 0, 2, 0, 0, 0, 'NUMERIC', 0, 128, 'NUMERIC', 10, -2, -3, 2, NULL, NULL, 0, 0, 3, 0),"
                "('NUMERIC SIGNED', 2, 128, NULL, NULL, 'precision,scale', 1, 0, 2, 0, 0, 0, 'NUMERIC', 0, 128, 'SIGNED NUMERIC', 10, -2, -3, 2, NULL, NULL, 0, 0, 3, 0),"
                "('NUMERIC UNSIGNED', 2, 128, NULL, NULL, 'precision,scale', 1, 0, 2, 1, 0, 0, 'NUMERIC', 0, 128, 'UNSIGNED NUMERIC', 10, -2, -3, 2, NULL, NULL, 0, 0, 3, 0),"
                "('REAL', 7, 7, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'REAL', NULL, NULL, 'REAL', 2, 22, -1, 7, NULL, NULL, 0, 0, 3, 0),"
                "('SMALLINT', 5, 5, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'SMALLINT', NULL, NULL, 'SMALLINT', 10, 5, -1, 5, NULL, NULL, 0, 0, 3, 0),"
                "('SMALLINT SIGNED', 5, 5, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'SMALLINT', NULL, NULL, 'SIGNED SMALLINT', 10, 5, -1, 5, NULL, NULL, 0, 0, 3, 0),"
                "('SMALLINT UNSIGNED', 5, 5, NULL, NULL, NULL, 1, 0, 2, 1, 0, 0, 'SMALLINT', NULL, NULL, 'UNSIGNED SMALLINT', 10, 5, -1, -502, NULL, NULL, 0, 0, 3, 0),"
                "('TIME', 92, 8, '{t ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'TIME', NULL, NULL, 'TIME', NULL, 8, 6, 9, 2, NULL, 4, 6, 3, 0),"
                "('TIMESTAMP', 93, 26, '{ts ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'TIMESTAMP', 0, 6, 'TIMESTAMP', NULL, 19, 16, 9, 3, NULL, 1, 6, 3, 0),"
                "('VARCHAR', 12, 32000, '''', '''', 'max length', 1, 1, 3, NULL, 0, NULL, 'VARCHAR', NULL, NULL, 'VARCHAR', NULL, -1, -1, 12, NULL, NULL, 0, 0, 3, 0)"
                 " ) "
          " dt(\"TYPE_NAME\", \"DATA_TYPE\", \"PREC\", \"LITERAL_PREFIX\", \"LITERAL_SUFFIX\", \"CREATE_PARAMS\", \"IS_NULLABLE\", \"CASE_SENSITIVE\", \"SEARCHABLE\","
          "\"UNSIGNED_ATTRIBUTE\", \"FIXED_PREC_SCALE\", \"AUTO_UNIQUE_VALUE\", \"LOCAL_TYPE_NAME\", \"MINIMUM_SCALE\", \"MAXIMUM_SCALE\", \"SQL_TYPE_NAME\","
          "\"NUM_PREC_RADIX\", \"USEPRECISION\", \"USELENGTH\", \"SQL_DATA_TYPE\", \"SQL_DATETIME_SUB\", \"INTERVAL_PRECISION\", \"DATETIMESTARTFIELD\","
          "\"DATETIMEENDFIELD\", \"APPLICATION_VERSION\", \"TRANSLATION_ID\")"
        " ORDER BY 2,1 FOR READ UNCOMMITTED ACCESS;");
            break;
        case SQL_API_SQLCOLUMNS :
            if (tableNm[0] != '$' && tableNm[0] != '\\')
                        {
                            char schcriteria[CRTASIZE];
                            char tblcriteria[CRTASIZE];
                            char colcriteria[CRTASIZE];
                            if (strcmp(catalogNm,"") == 0)
                                strcpy(tableName1,SEABASE_MD_CATALOG);
                            else
                                strcpy(tableName1, catalogNm);
                            tableParam[0] = tableName1;
                            convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
                            convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
                            convertWildcardNoEsc(metadataId, TRUE, columnNm, columnNmNoEsc);
                            inputParam[0] = odbcAppVersion;
                            inputParam[1] = NULL;
                    if(schemaNm[0]=='"'){
                        snprintf(schcriteria,CRTASIZE,"and ob.SCHEMA_NAME = '%s' ",schemaNmNoEsc);
                    }
                    else{
                        snprintf(schcriteria,CRTASIZE,"and trim(ob.SCHEMA_NAME) LIKE '%s' ESCAPE '\\' ",schemaNmNoEsc);
                    }
                    if(tableNm[0]=='"'){
                        snprintf(tblcriteria,CRTASIZE,"and ob.OBJECT_NAME = '%s' ",tableNmNoEsc);
                    }
                    else{
                        snprintf(tblcriteria,CRTASIZE,"and trim(ob.OBJECT_NAME) LIKE '%s' ESCAPE '\\' ",tableNmNoEsc);
                    }
                    if(columnNm[0]=='"'){
                        snprintf(colcriteria,CRTASIZE,"and co.COLUMN_NAME = '%s' ",columnNmNoEsc);
                    }
                    else{
                        snprintf(colcriteria,CRTASIZE,"and trim(co.COLUMN_NAME) LIKE '%s' ESCAPE '\\' ",columnNmNoEsc);
                    }

                    snprintf((char *)sqlString->dataValue._buffer, totalSize,
        "select " "cast('%s' as varchar(128) ) TABLE_CAT, "
        "cast(trim(ob.SCHEMA_NAME) as varchar(128) ) TABLE_SCHEM, "
        "cast(trim(ob.OBJECT_NAME) as varchar(128) ) TABLE_NAME, "
        "cast(trim(co.COLUMN_NAME) as varchar(128) ) COLUMN_NAME, "
        "cast((case when co.FS_DATA_TYPE = 0 and co.character_set = 'UCS2' then -8 "
        "when co.FS_DATA_TYPE = 64 and co.character_set = 'UCS2' then -9 else dt.DATA_TYPE end) as smallint) DATA_TYPE, "
        "trim(dt.TYPE_NAME) TYPE_NAME, "
        "cast((case when co.FS_DATA_TYPE = 0 and co.character_set = 'UCS2' then co.COLUMN_SIZE/2 "
        "when co.FS_DATA_TYPE = 64 and co.character_set = 'UCS2' then co.COLUMN_SIZE/2 "
        "when dt.USEPRECISION = -1 then co.COLUMN_SIZE when dt.USEPRECISION = -2 then co.COLUMN_PRECISION "
        "when co.FS_DATA_TYPE = 192 then dt.USEPRECISION + 1 "
        "when co.FS_DATA_TYPE >= 195 and co.FS_DATA_TYPE <= 207 then dt.USEPRECISION + 1 "
        "else dt.USEPRECISION end) as integer) COLUMN_SIZE, "
        "cast((case when dt.USELENGTH = -1 then co.COLUMN_SIZE when dt.USELENGTH = -2 then co.COLUMN_PRECISION "
        "when dt.USELENGTH = -3 then co.COLUMN_PRECISION + 2  "
        "else dt.USELENGTH end) as integer) BUFFER_LENGTH, "
        "cast(co.COLUMN_SCALE as smallint) DECIMAL_DIGITS, "
        "cast(dt.NUM_PREC_RADIX as smallint) NUM_PREC_RADIX, "
        "cast(co.NULLABLE as smallint) NULLABLE, "
        "cast('' as varchar(128)) REMARKS, "
        "trim(co.DEFAULT_VALUE) COLUMN_DEF, "
        "cast((case when co.FS_DATA_TYPE = 0 and co.character_set = 'UCS2' then -8 "
        "when co.FS_DATA_TYPE = 64 and co.character_set = 'UCS2' then -9 else dt.SQL_DATA_TYPE end) as smallint) SQL_DATA_TYPE, "
        "cast(dt.SQL_DATETIME_SUB as smallint) SQL_DATETIME_SUB, cast((case dt.DATA_TYPE when 1 then co.COLUMN_SIZE "
        "when -1 then co.COLUMN_SIZE when 12 then co.COLUMN_SIZE else NULL end) as integer) CHAR_OCTET_LENGTH, "
        "cast((case when (trim(co1.COLUMN_CLASS) <> 'S') then co.column_number+1 else "
        "co.column_number end) as integer) ORDINAL_POSITION, "
        "cast((case when co.NULLABLE = 0 then 'NO' else 'YES' end) as varchar(3)) IS_NULLABLE  "
        "from  "
        "TRAFODION.\"_MD_\".objects ob, "
        "TRAFODION.\"_MD_\".columns co, "
        "TRAFODION.\"_MD_\".columns co1, "
        "(VALUES ("
        "cast('BIGINT' as varchar(128)),cast(-5 as smallint), cast(19 as integer), cast (NULL as varchar(128)), cast (NULL as varchar(128)), "
        "cast (NULL as varchar(128)), cast(1 as smallint), cast(0 as smallint), cast(2 as smallint) , cast(0 as smallint), cast(0 as smallint), "
        "cast(0 as smallint), cast('LARGEINT' as varchar(128)), cast(NULL as smallint), cast(NULL as smallint), cast('SIGNED LARGEINT' as varchar(128)), cast(134 as integer), "
        "cast(10 as smallint), cast(19 as integer), cast(20 as integer), cast(-402 as smallint), cast(NULL as smallint), cast(NULL as smallint), "
        "cast(0 as smallint), cast(0 as smallint), cast(3 as smallint), cast(0 as smallint)), "
        "('CHAR', 1, 32000, '''', '''', 'max length', 1, 1, 3, NULL, 0, NULL, 'CHARACTER', NULL, NULL, 'CHARACTER', 0, NULL, -1, -1, 1, NULL, NULL, 0, 0, 3, 0), "
        "('DATE', 91, 10, '{d ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'DATE', NULL, NULL, 'DATE', 192, NULL, 10, 6, 9, 1, NULL, 1, 3, 3, 0), "
        "('DECIMAL', 3, 18, NULL, NULL, 'precision,scale', 1, 0, 2, 0, 0, 0, 'DECIMAL', 0, 18, 'SIGNED DECIMAL', 152, 10, -2, -3, 3, NULL, NULL, 0, 0, 3, 0), "
        "('DECIMAL UNSIGNED', 3, 18, NULL, NULL, 'precision,scale', 1, 0, 2, 1, 0, 0, 'DECIMAL', 0, 18, 'UNSIGNED DECIMAL', 150, 10, -2, -3, -301, NULL, NULL, 0, 0, 3, 0), "
        "('DOUBLE PRECISION', 8, 15, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'DOUBLE', NULL, NULL, 'DOUBLE', 143, 2, 54, -1, 8, NULL, NULL, 0, 0, 3, 0), "
        "('FLOAT', 6, 15, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'FLOAT', NULL, NULL, 'FLOAT', 142, 2, -2, -1, 6, NULL, NULL, 0, 0, 3, 0), "
        "('INTEGER', 4, 10, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'INTEGER', NULL, NULL, 'SIGNED INTEGER', 132, 10, 10, -1, 4, NULL, NULL, 0, 0, 3, 0), "
        "('INTEGER UNSIGNED', 4, 10, NULL, NULL, NULL, 1, 0, 2, 1, 0, 0, 'INTEGER', NULL, NULL, 'UNSIGNED INTEGER', 133, 10, 10, -1, -401, NULL, NULL, 0, 0, 3, 0), "
        "('INTERVAL', 113, 0, '{INTERVAL ''', ''' MINUTE TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 207, NULL, 3, 34, 100, 13, 2, 5, 6, 3, 0), "
        "('INTERVAL', 105, 0, '{INTERVAL ''', ''' MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 199, NULL, 0, 34, 100, 5, 2, 5, 5, 3, 0), "
        "('INTERVAL', 101, 0, '{INTERVAL ''', ''' YEAR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 195, NULL, 0, 34, 100, 1, 2, 1, 1, 3, 0), "
        "('INTERVAL', 106, 0, '{INTERVAL ''', ''' SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 200, NULL, 0, 34, 100, 6, 2, 6, 6, 3, 0), "
          "('INTERVAL', 104, 0, '{INTERVAL ''', ''' HOUR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 198, NULL, 0, 34, 100, 4, 2, 4, 4, 3, 0), "
          "('INTERVAL', 107, 0, '{INTERVAL ''', ''' YEAR TO MONTH}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 201, NULL, 3, 34, 100, 7, 2, 1, 2, 3, 0), "
          "('INTERVAL', 108, 0, '{INTERVAL ''', ''' DAY TO HOUR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 202, NULL, 3, 34, 100, 8, 2, 3, 4, 3, 0), "
          "('INTERVAL', 102, 0, '{INTERVAL ''', ''' MONTH}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 196, NULL, 0, 34, 100, 2, 2, 2, 2, 3, 0), "
          "('INTERVAL', 111, 0, '{INTERVAL ''', ''' HOUR TO MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 205, NULL, 3, 34, 100, 11, 2, 4, 5, 3, 0), "
          "('INTERVAL', 112, 0, '{INTERVAL ''', ''' HOUR TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 206, NULL, 6, 34, 100, 12, 2, 4, 6, 3, 0), "
          "('INTERVAL', 110, 0, '{INTERVAL ''', ''' DAY TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 204, NULL, 9, 34, 100, 10, 2, 3, 6, 3, 0), "
          "('INTERVAL', 109, 0, '{INTERVAL ''', ''' DAY TO MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 203, NULL, 6, 34, 100, 9, 2, 3, 5, 3, 0), "
          "('INTERVAL', 103, 0, '{INTERVAL ''', ''' DAY}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 197, NULL, 0, 34, 100, 3, 2, 3, 3, 3, 0), "
          "('NUMERIC', 2, 128, NULL, NULL, 'precision,scale', 1, 0, 2, 0, 0, 0, 'NUMERIC', 0, 128, 'SIGNED NUMERIC', 156, 10, -2, -3, 2, NULL, NULL, 0, 0, 3, 0), "
          "('NUMERIC UNSIGNED', 2, 128, NULL, NULL, 'precision,scale', 1, 0, 2, 1, 0, 0, 'NUMERIC', 0, 128, 'UNSIGNED NUMERIC', 155, 10, -2, -3, 2, NULL, NULL, 0, 0, 3, 0), "
          "('REAL', 7, 7, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'REAL', NULL, NULL, 'REAL', 142, 2, 22, -1, 7, NULL, NULL, 0, 0, 3, 0), "
          "('SMALLINT', 5, 5, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'SMALLINT', NULL, NULL, 'SIGNED SMALLINT', 130, 10, 5, -1, 5, NULL, NULL, 0, 0, 3, 0), "
          "('SMALLINT UNSIGNED', 5, 5, NULL, NULL, NULL, 1, 0, 2, 1, 0, 0, 'SMALLINT', NULL, NULL, 'UNSIGNED SMALLINT', 131, 10, 5, -1, -502, NULL, NULL, 0, 0, 3, 0), "
          "('TIME', 92, 8, '{t ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'TIME', NULL, NULL, 'TIME', 192, NULL, 8, 6, 9, 2, NULL, 4, 6, 3, 0), "
          "('TIMESTAMP', 93, 26, '{ts ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'TIMESTAMP', 0, 6, 'TIMESTAMP', 192, NULL, 19, 16, 9, 3, NULL, 1, 6, 3, 0), "
          "('VARCHAR', 12, 32000, '''', '''', 'max length', 1, 1, 3, NULL, 0, NULL, 'VARCHAR', NULL, NULL, 'VARCHAR', 64, NULL, -1, -1, 12, NULL, NULL, 0, 0, 3, 0) "
          " ) "
          "dt(\"TYPE_NAME\", \"DATA_TYPE\", \"PREC\", \"LITERAL_PREFIX\", \"LITERAL_SUFFIX\", \"CREATE_PARAMS\", \"IS_NULLABLE\", \"CASE_SENSITIVE\", \"SEARCHABLE\", "
             "\"UNSIGNED_ATTRIBUTE\", \"FIXED_PREC_SCALE\", \"AUTO_UNIQUE_VALUE\", \"LOCAL_TYPE_NAME\", \"MINIMUM_SCALE\", \"MAXIMUM_SCALE\", \"SQL_TYPE_NAME\", \"FS_DATA_TYPE\", "
             "\"NUM_PREC_RADIX\", \"USEPRECISION\", \"USELENGTH\", \"SQL_DATA_TYPE\", \"SQL_DATETIME_SUB\", \"INTERVAL_PRECISION\", \"DATETIMESTARTFIELD\", "
             "\"DATETIMEENDFIELD\", \"APPLICATION_VERSION\", \"TRANSLATION_ID\") "
        "where  ob.OBJECT_UID = co.OBJECT_UID "
          "and dt.FS_DATA_TYPE = co.FS_DATA_TYPE "
          "and co.OBJECT_UID = co1.OBJECT_UID and co1.COLUMN_NUMBER = 0 "
          "and (dt.DATETIMESTARTFIELD = co.DATETIME_START_FIELD) "
          "and (dt.DATETIMEENDFIELD = co.DATETIME_END_FIELD) "
          "%s%s%s"
          "and (ob.OBJECT_TYPE in ('BT' , 'VI') ) "
          "and (trim(co.COLUMN_CLASS) not in ('S', 'M')) "
          "and dt.APPLICATION_VERSION = %s "
        "FOR READ UNCOMMITTED ACCESS order by 1, 2, 3, co.COLUMN_NUMBER ; ",
                                tableParam[0], schcriteria, tblcriteria, colcriteria , inputParam[0]);
                    }
                    break;
                case SQL_API_SQLPRIMARYKEYS:
                    if ((!checkIfWildCard(catalogNm, catalogNmNoEsc) || !checkIfWildCard(schemaNm, schemaNmNoEsc) || !checkIfWildCard(tableNm, tableNmNoEsc)) && !metadataId)
                    {
                        executeException->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
                        executeException->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
                        FUNCTION_RETURN_NUMERIC(EXECUTE_EXCEPTION,("EXECUTE_EXCEPTION"));
                    }

                    if (strcmp(catalogNm,"") == 0)
                        strcpy(tableName1,SEABASE_MD_CATALOG);
                    else
                        strcpy(tableName1, catalogNm);
                    tableParam[0] = tableName1;
                    convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
                    convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
                    convertWildcard(metadataId, TRUE, tableNm, expTableNm);
                    convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
                    inputParam[0] = schemaNmNoEsc;
                    inputParam[1] = expSchemaNm;
                    inputParam[2] = tableNmNoEsc;
                    inputParam[3] = expTableNm;
                    inputParam[4] = NULL;


                    snprintf((char *)sqlString->dataValue._buffer, totalSize,
                                "select "
                                "cast('%s' as varchar(128) ) TABLE_CAT,"
                                "cast(trim(ob.SCHEMA_NAME) as varchar(128) ) TABLE_SCHEM,"
                                "cast(trim(ob.OBJECT_NAME) as varchar(128) ) TABLE_NAME,"
                                "trim(ky.COLUMN_NAME) COLUMN_NAME,"
                                "cast((ky.keyseq_number) as smallint) KEY_SEQ,"
                                "trim(ob.OBJECT_NAME) PK_NAME "
                                " from TRAFODION.\"_MD_\".OBJECTS ob, "
                                "TRAFODION.\"_MD_\".KEYS ky "
                                " where (ob.SCHEMA_NAME = '%s' or "
                                " trim(ob.SCHEMA_NAME) LIKE '%s' ESCAPE '\\') "
                                " and (ob.OBJECT_NAME = '%s' or "
                                " trim(ob.OBJECT_NAME) LIKE '%s' ESCAPE '\\') "
                                " and ob.OBJECT_UID = ky.OBJECT_UID and ky.COLUMN_NAME <> '_SALT_' "
                                " FOR READ UNCOMMITTED ACCESS order by 1, 2, 3, 5 ;",
                        tableParam[0], inputParam[0], inputParam[1],
                        inputParam[2], inputParam[3]);
                    break;
                case SQL_API_SQLPROCEDURES:
                    if ((!checkIfWildCard(catalogNm, catalogNmNoEsc) || !checkIfWildCard(schemaNm, schemaNmNoEsc) || !checkIfWildCard(
tableNm, tableNmNoEsc)) && !metadataId)
                    {
                        executeException->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
                        executeException->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
                        FUNCTION_RETURN_NUMERIC(EXECUTE_EXCEPTION,("EXECUTE_EXCEPTION"));
                    }

                    if (strcmp(catalogNm,"") == 0)
                        strcpy(tableName1,SEABASE_MD_CATALOG);
                    else
                        strcpy(tableName1, catalogNm);
                    tableParam[0] = tableName1;
                    convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
                    convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
                    convertWildcard(metadataId, TRUE, tableNm, expTableNm);
                    convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
                    inputParam[0] = schemaNmNoEsc;
                    inputParam[1] = expSchemaNm;
                    inputParam[2] = tableNmNoEsc;
                    inputParam[3] = expTableNm;
                    inputParam[4] = NULL;

                    snprintf((char *)sqlString->dataValue._buffer, totalSize,
                        "select obj.CATALOG_NAME PROCEDURE_CAT, obj.SCHEMA_NAME PROCEDURE_SCHEM, "
			"obj.OBJECT_NAME PROCEDURE_NAME, cast(NULL as varchar(10)) R1,cast(NULL as varchar(10)) R2,"
                        "cast(NULL as varchar(10)) R3, cast(NULL as varchar(10)) REMARKS,"
			"cast(case when routines.UDR_TYPE = 'P' then 1"
			" when routines.UDR_TYPE = 'F' or routines.UDR_TYPE = 'T'"
			" then 2 else 0 end as smallint) PROCEDURE_TYPE, "
                        "obj.OBJECT_NAME SPECIFIC_NAME "
                        " from TRAFODION.\"_MD_\".OBJECTS obj "
			" left join TRAFODION.\"_MD_\".ROUTINES routines on obj.OBJECT_UID = routines.UDR_UID"
                        " where "
                        " (obj.SCHEMA_NAME = '%s' or trim(obj.SCHEMA_NAME) LIKE '%s' ESCAPE '\\')"
			" and (obj.OBJECT_NAME = '%s' or trim(obj.OBJECT_NAME) LIKE '%s' ESCAPE '\\')"
                        " and obj.OBJECT_TYPE='UR' "
                        " order by obj.OBJECT_NAME"
                        " FOR READ UNCOMMITTED ACCESS;",
                        inputParam[0], inputParam[1], inputParam[2], inputParam[3]);
                    break;
                case SQL_API_SQLPROCEDURECOLUMNS:
                    if ((!checkIfWildCard(catalogNm, catalogNmNoEsc) || !checkIfWildCard(schemaNm, schemaNmNoEsc) || !checkIfWildCard(
tableNm, tableNmNoEsc)) && !metadataId)
                    {
                        executeException->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
                        executeException->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
                        FUNCTION_RETURN_NUMERIC(EXECUTE_EXCEPTION,("EXECUTE_EXCEPTION"));
                    }

                    if (strcmp(catalogNm,"") == 0)
                        strcpy(tableName1,SEABASE_MD_CATALOG);
                    else
                        strcpy(tableName1, catalogNm);
                    tableParam[0] = tableName1;
                    convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
                    convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
                    convertWildcard(metadataId, TRUE, tableNm, expTableNm);
		            convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
                    convertWildcard(metadataId, TRUE, columnNm, expColumnNm);
                    convertWildcardNoEsc(metadataId, TRUE, columnNm, columnNmNoEsc);
                    inputParam[0] = schemaNmNoEsc;
                    inputParam[1] = expSchemaNm;
                    inputParam[2] = tableNmNoEsc;
                    inputParam[3] = expTableNm;
                    inputParam[4] = columnNmNoEsc;
                    inputParam[5] = expColumnNm;
                    inputParam[6] = NULL;

                    snprintf((char *)sqlString->dataValue._buffer, totalSize,
                            "select obj.CATALOG_NAME PROCEDURE_CAT, obj.SCHEMA_NAME PROCEDURE_SCHEM,"
                            "obj.OBJECT_NAME PROCEDURE_NAME, cols.COLUMN_NAME COLUMN_NAME, "
                            "cast((case when cols.DIRECTION='I' then 1 when cols.DIRECTION='N' then 2 when cols.DIRECTION='O' then 3 else 0 end) as smallint) COLUMN_TYPE, "
                            "cols.FS_DATA_TYPE DATA_TYPE, cols.SQL_DATA_TYPE TYPE_NAME, "
                            "cols.COLUMN_PRECISION \"PRECISION\", cols.COLUMN_SIZE LENGTH, cols.COLUMN_SCALE SCALE, "
                            "cast(1 as smallint) RADIX, cols.NULLABLE NULLABLE, cast(NULL as varchar(10)) REMARKS, "
                            "cols.DEFAULT_VALUE COLUMN_DEF, cols.FS_DATA_TYPE SQL_DATA_TYPE, cast(0 as smallint) SQL_DATETIME_SUB, "
                            "cols.COLUMN_SIZE CHAR_OCTET_LENGTH, cols.COLUMN_NUMBER ORDINAL_POSITION, "
                            "cols.NULLABLE IS_NULLABLE, cols.COLUMN_NAME SPECIFIC_NAME"
                            " from TRAFODION.\"_MD_\".OBJECTS obj "
                            " left join TRAFODION.\"_MD_\".COLUMNS cols on obj.OBJECT_UID=cols.OBJECT_UID "
                            " where "
                            " (obj.SCHEMA_NAME = '%s' or trim(obj.SCHEMA_NAME) LIKE '%s' ESCAPE '\\')"
                            " and (obj.OBJECT_NAME = '%s' or trim(obj.OBJECT_NAME) LIKE '%s' ESCAPE '\\')"
                            " and (cols.COLUMN_NAME = '%s' or trim(cols.COLUMN_NAME) LIKE '%s' ESCAPE '\\')"
                            " order by cols.COLUMN_NUMBER"
                            " FOR READ UNCOMMITTED ACCESS;"
                        ,inputParam[0], inputParam[1], inputParam[2], inputParam[3], inputParam[4], inputParam[5]);
                    break;
            }
    if (pSrvrStmt == NULL)
    {
        executeException->exception_nr = odbc_SQLSvc_Prepare_SQLError_exn_;
        kdsCreateSQLErrorException(&sqlError, 1);
        kdsCopySQLErrorException(&sqlError, SQLSVC_EXCEPTION_PREPARE_FAILED, sqlcode,
            "HY000");
        executeException->u.SQLError.errorList._length = sqlError.errorList._length;
        executeException->u.SQLError.errorList._buffer = sqlError.errorList._buffer;
        FUNCTION_RETURN_NUMERIC(EXECUTE_EXCEPTION,("EXECUTE_EXCEPTION"));
    }
    //make pSrvrStmt->Prepare() happy
    sqlString->dataValue._length=strlen((const char*)sqlString->dataValue._buffer);
    rc = pSrvrStmt->Prepare(sqlString, sqlStmtType, holdability, queryTimeout);
    if (rc == SQL_ERROR)
    {
        executeException->exception_nr = odbc_SQLSvc_ExecuteN_SQLError_exn_;
        executeException->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
        executeException->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
        FUNCTION_RETURN_NUMERIC(EXECUTE_EXCEPTION,("EXECUTE_EXCEPTION"));

    }

    //free RAM for sqlString
    MEMORY_DELETE_ARRAY(sqlString->dataValue._buffer)
    MEMORY_DELETE(sqlString)

    pSrvrStmt->InternalStmtClose(SQL_CLOSE);
    outputDesc->_length = pSrvrStmt->outputDescList._length;
    outputDesc->_buffer = pSrvrStmt->outputDescList._buffer;

    SRVR_DESC_HDL   *IPD;
    IPD = pSrvrStmt->IPD;
    BYTE    *dataPtr;
    BYTE    *indPtr;
    long    dataType;

    for (curParamNo = 0, index = 0,  tableParamDone = FALSE;
        curParamNo < pSrvrStmt->inputDescList._length ; curParamNo++, index++)
    {
        dataPtr = IPD[curParamNo].varPtr;
        indPtr = IPD[curParamNo].indPtr;
        dataType = IPD[curParamNo].dataType;
        SQLItemDesc = (SQLItemDesc_def *)pSrvrStmt->inputDescList._buffer + curParamNo;
        getMemoryAllocInfo(SQLItemDesc->dataType, SQLItemDesc->SQLCharset, SQLItemDesc->maxLen, SQLItemDesc->vc_ind_length, 0,
            NULL, &allocLength, NULL);
        if (! tableParamDone)
        {
            if (tableParam[index] == NULL)
            {
                tableParamDone = TRUE;
                index = 0;
            }
            else
            {
                retcode = setParamValue(dataType, dataPtr, indPtr, allocLength, tableParam[index]);
                DEBUG_OUT(DEBUG_LEVEL_METADATA,("tableParam[%d] = %s ",index,tableParam[index]));
            }
        }
        if (tableParamDone)
        {
            retcode = setParamValue(dataType, dataPtr, indPtr, allocLength, inputParam[index]);
            DEBUG_OUT(DEBUG_LEVEL_METADATA,("inputParam[%d] = %s ",index,inputParam[index]));
        }

        if (retcode != 0)
            FUNCTION_RETURN_NUMERIC((short) retcode,(NULL));
    }

    executeException->exception_nr = 0;

    // sqlStmtType has value of types like TYPE_SELECT, TYPE_DELETE etc.
    odbc_SQLSvc_ExecuteN_sme_(objtag_, call_id_, executeException, dialogueId, *stmtId,
        (char *) stmtLabel,
        sqlStmtType, 1, &pSrvrStmt->inputValueList, SQL_ASYNC_ENABLE_OFF, 0,
        &pSrvrStmt->outputValueList, sqlWarning);
    if (executeException->exception_nr != CEE_SUCCESS){
        FUNCTION_RETURN_NUMERIC(EXECUTE_EXCEPTION,("EXECUTE_EXCEPTION"));

    }

    FUNCTION_RETURN_NUMERIC(0,(NULL));
}

// Compute the memory allocation requirements for the descriptor data type
void getMemoryAllocInfo(long data_type, long char_set, long data_length, long vc_ind_length, long curr_mem_offset,
                        long *mem_align_offset, int *alloc_size, long *var_layout)
{
    FUNCTION_ENTRY("getMemoryAllocInfo",
        ("data_type=%s, char_set=%s, data_length=%ld, vc_ind_length=%ld, curr_mem_offset=%ld, mem_align_offset=0x%08x, alloc_size=0x%08x, var_layout=0x%08x",
        CliDebugSqlTypeCode(data_type),
        getCharsetEncoding(char_set),
        data_length,
	vc_ind_length,
        curr_mem_offset,
        mem_align_offset,
        alloc_size,
        var_layout));
    long varPad = 0;            // Bytes to pad allocation for actual data type memory requirements
    long varNulls = 0;          // Number of extra bytes that will be appended to data type (e.g. NULL for strings)
    long memAlignOffset = 0;    // Boundry offset from current memory location to set the data pointer
    long allocBoundry = 0;      // Boundry to round the size of the memory allocation to end on proper boundry

    switch (data_type)
    {
    case SQLTYPECODE_CHAR:
    case SQLTYPECODE_VARCHAR:
    case SQLTYPECODE_CLOB:
    case SQLTYPECODE_BLOB:
        if( nullRequired(char_set) )
            varNulls = 1;
        break;
    case SQLTYPECODE_VARCHAR_WITH_LENGTH:
    case SQLTYPECODE_VARCHAR_LONG:
        if (vc_ind_length == 4)
        {
            memAlignOffset = (((curr_mem_offset + 4 - 1) >> 2) << 2) - curr_mem_offset;
            varPad = 2;
            varNulls = 1;
            allocBoundry = 4;
        }
        else
        {
            memAlignOffset = (((curr_mem_offset + 2 - 1) >> 1) << 1) - curr_mem_offset;
            varPad = 2;
            varNulls = 1;
            allocBoundry = 2;
        }
        break;
    case SQLTYPECODE_SMALLINT:
    case SQLTYPECODE_SMALLINT_UNSIGNED:
        memAlignOffset = (((curr_mem_offset + 2 - 1) >> 1) << 1) - curr_mem_offset;
        break;
    case SQLTYPECODE_INTEGER:
    case SQLTYPECODE_INTEGER_UNSIGNED:
        memAlignOffset = (((curr_mem_offset + 4 - 1) >> 2) << 2) - curr_mem_offset;
        break;
    case SQLTYPECODE_LARGEINT:
    case SQLTYPECODE_REAL:
    case SQLTYPECODE_DOUBLE:
        memAlignOffset = (((curr_mem_offset + 8 - 1) >> 3) << 3) - curr_mem_offset;
        break;
    case SQLTYPECODE_DECIMAL_UNSIGNED:
    case SQLTYPECODE_DECIMAL:
    case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
    case SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
        break;
    case SQLTYPECODE_INTERVAL:      // Treating as CHAR
    case SQLTYPECODE_DATETIME:
        memAlignOffset = (((curr_mem_offset + 2 - 1) >> 1) << 1) - curr_mem_offset;
        varPad = 2;
        varNulls = 1;
        allocBoundry = 2;
        break;
    default:
        memAlignOffset = (((curr_mem_offset + 8 - 1) >> 3) << 3) - curr_mem_offset;
        break;
    }
    long varLayout = data_length + varNulls;
    long allocSize = varLayout + varPad;
    if (allocBoundry) allocSize += allocSize % allocBoundry;
    if (mem_align_offset) *mem_align_offset = memAlignOffset;
    if (alloc_size) *alloc_size = allocSize;
    if (var_layout) *var_layout = varLayout;
    FUNCTION_RETURN_VOID(("memAlignOffset=%ld, allocSize=%ld, varLayout=%ld",
        memAlignOffset,
        allocSize,
        varLayout));
}

void appendOutputValueList(SQLValueList_def *targ, SQLValueList_def *src, bool free_src)
{
    FUNCTION_ENTRY("appendOutputValueList",("targ=0x%08x, src=0x%08x",
        targ,
        src));

    if (src->_length)
    {
        if (free_src && (targ->_length==0))
        {
            // The target list is empty and the source is
            //   going to be deleted, so just move the buffers.
            targ->_buffer = src->_buffer;
            targ->_length = src->_length;
            src->_buffer = NULL;
            src->_length = 0;
        }
        else
        {
            unsigned long totalSize = targ->_length + src->_length;
            SQLValue_def *newBuffer = NULL;

            // allocate space for the combined buffers
            MEMORY_ALLOC_ARRAY(newBuffer, SQLValue_def, totalSize);

            // copy the previous target buffer to the new buffer
            if (targ->_length)
            {
                memcpy(newBuffer,
                    targ->_buffer,
                    (sizeof (SQLValue_def)) * targ->_length);
            }

            // append the source buffer to the target buffer in the new buffer
            memcpy(newBuffer + targ->_length,
                src->_buffer,
                (sizeof (SQLValue_def)) * src->_length);

            // Copy the dataValue buffers for each output value
            for (int i=0; i < src->_length; i++)
            {
                if (free_src)
                {
                    // Move the source buffer since it is going to be freed anyway
                    newBuffer[targ->_length + i].dataValue._buffer = src->_buffer[i].dataValue._buffer;
                    src->_buffer[i].dataValue._buffer = NULL;
                }
                else
                {
                    // Allocate a new dataValue buffer and copy from source
                    newBuffer[targ->_length + i].dataValue._buffer = NULL;
                    MEMORY_ALLOC_ARRAY(newBuffer[targ->_length + i].dataValue._buffer,
                        unsigned char,
                        newBuffer[targ->_length + i].dataValue._length);
                    memcpy(newBuffer[targ->_length + i].dataValue._buffer,
                        src->_buffer[i].dataValue._buffer,
                        src->_buffer[i].dataValue._length);
                }
            }
            // Cleanup the target buffer and set the target output value list
            //   to use the new appended buffer
            MEMORY_DELETE_ARRAY(targ->_buffer);
            targ->_buffer = newBuffer;
            targ->_length = totalSize;
            // Free the source output value list if needed
            if (free_src) freeOutputValueList(src);
        }
    }
    FUNCTION_RETURN_VOID((NULL));
}

void freeOutputValueList(SQLValueList_def *ovl)
{
    FUNCTION_ENTRY("freeOutputValueList",("olv=0x%08x",
        ovl));

    // Free up the dataValue buffers
    for (int i=0; i < ovl->_length; i++) MEMORY_DELETE(ovl->_buffer[i].dataValue._buffer);

    // Free up the output value list buffer
    MEMORY_DELETE(ovl->_buffer);
    ovl->_length = 0;

    FUNCTION_RETURN_VOID((NULL));
}

short executeAndFetchSMDQuery(
    /* In   */ void * objtag_
  , /* In   */ const CEE_handle_def *call_id_
  , /* In   */ long dialogueId
  , /* In   */ short APIType
  , /* In   */ const char *stmtLabel
  , /* In   */ short sqlStmtType
  , /* In   */ const char *tableParam[]
  , /* In   */ const char *inputParam[]
  , /* In   */const char *catalogNm
  , /* In   */ const char *schemaNm
  , /* In   */ const char *tableNm
  , /* In   */ const char *columnNm
  , /* In   */ const char *tableTypeList
  , /* In   */ unsigned long metadataId
  , /* Out   */ SQLItemDescList_def *outputDesc
  , /* Out   */ ExceptionStruct *executeException
  , /* Out   */ ExceptionStruct *fetchException
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
  , /* Out   */ long *rowsAffected
  , /* Out   */ SQLValueList_def *outputValueList
  , /* Out   */ long *stmtId
  )
{
    FUNCTION_ENTRY("executeAndFetchSMDQuery",("..."));
    short retCode;

    retCode = do_ExecSMD(objtag_, call_id_, executeException, sqlWarning, dialogueId,
            tableTypeList, metadataId, APIType, stmtLabel, sqlStmtType, catalogNm, schemaNm,
            tableNm, columnNm, tableParam, inputParam, outputDesc, stmtId);
    if(retCode != CEE_SUCCESS)
        FUNCTION_RETURN_NUMERIC(retCode,("do_ExecSMD() Failed"));

    long rowsRead;
    SQLValueList_def fetchOutputValueList;
    memset(&fetchOutputValueList, 0, sizeof (fetchOutputValueList));
    *rowsAffected = 0;
    do
    {
        odbc_SQLSvc_FetchN_sme_(objtag_, call_id_, fetchException, dialogueId, *stmtId, SQL_MAX_COLUMNS_IN_SELECT,
                                SQL_ASYNC_ENABLE_OFF, 0, &rowsRead, &fetchOutputValueList, sqlWarning);

        if (fetchException->exception_nr != CEE_SUCCESS){
            FUNCTION_RETURN_NUMERIC(FETCH_EXCEPTION,("FETCH_EXCEPTION - odbc_SQLSvc_FetchN_sme_() Failed"));
        }
        *rowsAffected += rowsRead;
        if (rowsRead==SQL_MAX_COLUMNS_IN_SELECT) appendOutputValueList(outputValueList,&fetchOutputValueList,true);
    } while (rowsRead==SQL_MAX_COLUMNS_IN_SELECT);
    appendOutputValueList(outputValueList,&fetchOutputValueList,true);

    FUNCTION_RETURN_NUMERIC(CEE_SUCCESS,("CEE_SUCCESS"));
}

BOOL nullRequired(long charSet)
{
    FUNCTION_ENTRY("nullRequired", ("charSet = %s", getCharsetEncoding(charSet)));

    switch(charSet)
    {
    case SQLCHARSETCODE_KANJI:
    case SQLCHARSETCODE_KSC5601:
    case SQLCHARSETCODE_UCS2:
        FUNCTION_RETURN_NUMERIC(FALSE,("%s",DebugBoolStr(FALSE)));
        break;
    case SQLCHARSETCODE_ISO88591:
        FUNCTION_RETURN_NUMERIC(TRUE,("%s",DebugBoolStr(TRUE)));
        break;
    }
    FUNCTION_RETURN_NUMERIC(FALSE,("%s",DebugBoolStr(FALSE)));
}

#ifdef _DEBUG
// Debug Function to print out the contents of the outputValueList buffer.
// Usage Examples are as follows:
//      print_outputValueList(outputValueList, pSrvrStmt->columnCount, "outputValueList");
//      print_outputValueList(&pSrvrStmt->outputValueList, pSrvrStmt->columnCount, "pSrvrStmt->outputValueList");
void print_outputValueList(SQLValueList_def *oVL, long colCount, const char * fcn_name) {

    printf("%s 0x%08x\n", fcn_name, oVL);

    for (int row=0; row < oVL->_length/colCount; row++) {
        printf(" %s row=%d\n",fcn_name,row);
        for (int col=0; col < colCount && col < 4; col++) {
            int index=row*colCount+col;
            if (oVL->_buffer->dataType == SQLTYPECODE_VARCHAR) {
                char * text = new char[oVL->_buffer[index].dataValue._length+1];

                memcpy(text, oVL->_buffer[index].dataValue._buffer, oVL->_buffer[index].dataValue._length);

                text[oVL->_buffer->dataValue._length] = 0;

                printf("  SQLTYPECODE_VARCHAR oVL[%d]=%s\n",
                    col, text);

                //Soln. No.: 10-111229-1174 fix memory leak
                delete[] text;

            }
            if (oVL->_buffer->dataType == SQLTYPECODE_VARCHAR_WITH_LENGTH) {

                short * len = (short *)oVL->_buffer[index].dataValue._buffer;
                char * text = new char[*len+1];

                memcpy(text, (char *)oVL->_buffer[index].dataValue._buffer+2, *len);

                text[*len] = 0;

                printf("  SQLTYPECODE_VARCHAR_WITH_LENGTH oVL[%d]=%s\n",
                    col, text);

                //Soln. No.: 10-111229-1174 fix memory leak
                delete[] text;
            }
        }
    }
    fflush(stdout);
}
#endif

// DO NOT call this function using pSrvrStmt->sqlWarningOrErrorLength and pSrvrStmt->sqlWarningOrError,
// Since the WarningOrError is static and pSrvrStmt->sqlWarningOrError will deallocate this memory. 
extern "C" void GETMXCSWARNINGORERROR(
          /* In    */ Int32 sqlcode
        , /* In    */ char *sqlState
        , /* In    */ char *msg_buf
        , /* Out   */ Int32 *MXCSWarningOrErrorLength
        , /* Out   */ BYTE *&MXCSWarningOrError)
{
    Int32 total_conds = 1;
    Int32 buf_len;
    Int32 curr_cond = 1;
    Int32 msg_buf_len = strlen(msg_buf)+1;
    Int32 time_and_msg_buf_len = 0;
    Int32 msg_total_len = 0;
    Int32 rowId = 0; // use this for rowset recovery.
    char tsqlState[6];
    BYTE WarningOrError[1024];
    char  strNow[TIMEBUFSIZE + 1];
    char* time_and_msg_buf = NULL;

    memset(tsqlState,0,sizeof(tsqlState));
    memcpy(tsqlState,sqlState,sizeof(tsqlState)-1);

    bzero(WarningOrError,sizeof(WarningOrError));

    *MXCSWarningOrErrorLength = 0;
    MXCSWarningOrError = WarningOrError; // Size of internally generated message should be enough

    *(Int32 *)(WarningOrError+msg_total_len) = total_conds;
    msg_total_len += sizeof(total_conds);
    *(Int32 *)(WarningOrError+msg_total_len) = rowId;
    msg_total_len += sizeof(rowId);
    *(Int32 *)(WarningOrError+msg_total_len) = sqlcode;
    msg_total_len += sizeof(sqlcode);
    time_and_msg_buf_len   = msg_buf_len + TIMEBUFSIZE;
    *(Int32 *)(WarningOrError+msg_total_len) = time_and_msg_buf_len;
    msg_total_len += sizeof(time_and_msg_buf_len);
    //Get the timetsamp
        time_and_msg_buf = new char[time_and_msg_buf_len];
    strncpy(time_and_msg_buf, msg_buf, msg_buf_len);
    time_t  now = time(NULL);
    bzero(strNow, sizeof(strNow));
    strftime(strNow, sizeof(strNow), " [%Y-%m-%d %H:%M:%S]", localtime(&now));
    strcat(time_and_msg_buf, strNow);
    memcpy(WarningOrError+msg_total_len, time_and_msg_buf, time_and_msg_buf_len);
    msg_total_len += time_and_msg_buf_len;
    delete time_and_msg_buf;
    memcpy(WarningOrError+msg_total_len, tsqlState, sizeof(tsqlState));
    msg_total_len += sizeof(tsqlState);

    memcpy(MXCSWarningOrError, WarningOrError, sizeof(WarningOrError));
    *MXCSWarningOrErrorLength = msg_total_len;
    return;
}

char* strcpyUTF8(char *dest, const char *src, size_t destSize, size_t copySize)
{
    char c;
    size_t len;

    if (copySize == 0)
        len = strlen(src);
    else
        len = copySize;

    if (len >= destSize)
        len = destSize-1; // truncation

    while (len > 0)
    {
        c = src[len-1];
        if (c < 0x80 || c > 0xbf)
            break;
        len--; // in second, third, or fourth byte of a multi-byte sequence
    }
    strncpy((char*)dest, (const char*)src, len);
    dest[len] = 0;

    return dest;
}

int getAllocLength(int DataType, int Length)
{
    int AllocLength;

    switch (DataType) {
        case SQLTYPECODE_CHAR:
        //case SQLTYPECODE_CHAR_UP: // sqlcli doesn't support anymore
        case SQLTYPECODE_VARCHAR:
            AllocLength = Length+1; 
            break;
        case SQLTYPECODE_VARCHAR_WITH_LENGTH:
        //case SQLTYPECODE_VARCHAR_UP: // sqlcli doesn't support anymore
        case SQLTYPECODE_VARCHAR_LONG:
            AllocLength = Length+3;
            break;
        default:
            AllocLength = Length;
            break;
    }
    return AllocLength;
}

