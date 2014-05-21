/************************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**************************************************************************/
//
// MODULE: SrvrCommon.cpp
//
// PURPOSE: Implements the common functions used by Srvr
//

#include <platform_ndcs.h>
#ifdef NSK_PLATFORM
#include <sqlWin.h>
#else
#include <sql.h>
#endif
#include <sqlext.h>
#include "SrvrCommon.h"
#include "SrvrKds.h"
#include "SqlInterface.h"
#include "CommonDiags.h"
//#include "tdm_odbcSrvrMsg.h"
#ifdef NSK_PLATFORM
#include "SrvrFunctions.h"			// Linux port - Todo
#include "NskUtil.h"
#include "cextdecs.h"
#include "pThreadsSync.h"
#include <tslxExt.h>
#include "feerrors.h"
#endif
#include "CSrvrConnect.h"
#include "Debug.h"

#define SQL_PSEUDO_FILE "$ZQFO"
// Global Variables

SRVR_SESSION_HDL	*pSrvrSession = NULL;
SRVR_GLOBAL_Def		*srvrGlobal = NULL;
//ODBCMXEventMsg		*srvrEventLogger = NULL;
long				*TestPointArray = NULL;
SQLMODULE_ID		nullModule;
SQLDESC_ITEM		gDescItems[NO_OF_DESC_ITEMS];
char				CatalogNm[MAX_ANSI_NAME_LEN+1];
char				SchemaNm[MAX_ANSI_NAME_LEN+1];
char				TableNm[MAX_ANSI_NAME_LEN+1];
char				ColumnName[MAX_ANSI_NAME_LEN+1];
char				ColumnHeading[MAX_ANSI_NAME_LEN+1];

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

int initSqlCore(int argc, char *argv[])
{
	FUNCTION_ENTRY("initSqlCore",(NULL));
	short	error;
	int		retcode = 0;

	// Assign the module Information
	nullModule.version = SQLCLI_ODBC_MODULE_VERSION;
	nullModule.module_name = NULL;
	nullModule.module_name_len = 0;
	nullModule.charset = "ISO88591";
	nullModule.creation_timestamp = 0;

#ifndef DISABLE_NOWAIT
	if (srvrGlobal->nowaitOn)
	{
		error = FILE_OPEN_(SQL_PSEUDO_FILE,			// Filename
			strlen(SQL_PSEUDO_FILE),	// The length of file name Guardian does not understand C strings
			&srvrGlobal->nowaitFilenum, // Return the file number (file descriptor)
			,						   // Access - soecifies the desired access mode (default read-write)
			,						   // Exclusion - specifies the desired mode (default shared)
			1);						 // Nowait-depth - number of outstanding I/O requests
		retcode = error;
		if (error != FEOK)
		{
			// TODO - Write to EMS log in future
			srvrGlobal->nowaitOn = FALSE;
		}
	}
	if (srvrGlobal->nowaitOn)
	{
		retcode = registerPseudoFileIO(srvrGlobal->nowaitFilenum);
		if (retcode != TSLXE_SUCCESS)
		{
			// TODO - Write to EMS log in future
			srvrGlobal->nowaitOn = FALSE;
		}

	}
#endif

	gDescItems[0].item_id = SQLDESC_TYPE;
	gDescItems[1].item_id = SQLDESC_LENGTH;
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

/*
	// Seaquest related - Linux port
	// Initialize seabed
	int	sbResult;
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
						   long	*sqlcode)
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

SRVR_STMT_HDL *createSrvrStmt(long dialogueId,
							  const char *stmtLabel,
							  long	*sqlcode,
							  const char *moduleName,
							  long moduleVersion,
							  long long moduleTimestamp,
							  short	sqlStmtType,
							  BOOL	useDefaultDesc,
							  BOOL    internalStmt,
							  long stmtId)
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
		sqlStmtType, useDefaultDesc,internalStmt,stmtId);
	FUNCTION_RETURN_PTR(pSrvrStmt,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
}


SRVR_STMT_HDL *createSrvrStmtForMFC(
									long dialogueId,
									const char *stmtLabel,
									long	*sqlcode,
									const char *moduleName,
									long moduleVersion,
									long long moduleTimestamp,
									short	sqlStmtType,
									BOOL	useDefaultDesc)
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

	pSrvrStmt = pConnect->createSrvrStmtForMFC(stmtLabel, sqlcode, moduleName, moduleVersion, moduleTimestamp,
		sqlStmtType, useDefaultDesc);
	FUNCTION_RETURN_PTR(pSrvrStmt,("sqlcode=%s",CliDebugSqlError(*sqlcode)));
}


SRVR_STMT_HDL *createSpjrsSrvrStmt(SRVR_STMT_HDL *callpSrvrStmt,
								   long dialogueId,
								   const char *stmtLabel,
								   long	*sqlcode,
								   const char *moduleName,
								   long moduleVersion,
								   long long moduleTimestamp,
								   short	sqlStmtType,
								   long	RSindex,
								   const char *RSstmtLabel,
								   BOOL	useDefaultDesc)
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
	SQLRETURN	rc;
	long		sqlcode;

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
	BOOL	quoted = FALSE;
	BOOL 	leadingBlank = TRUE;
	char	*trailingBlankPtr = NULL;


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
						trailingBlankPtr = out;	// Store the pointer to begining of the blank
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
						in++;			// Assuming this may be last character
						break;
					}
				}
				else
				{
					trailingBlankPtr = NULL;
					switch (*in)
					{
					case '\\':		// Escape Character
						*out++ = *in++;
						break;
					case '_':
					case '%':
						*out++ = '\\';
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
						in++;		// Assuming this may be last character
						break;
					}
				}
				else
				{
					switch (*in)
					{
					case '\\':		// Escape Character
						*out++ = *in++;
						break;
					case '_':
					case '%':
						*out++ = '\\';
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
	BOOL	quoted = FALSE;
	BOOL 	leadingBlank = TRUE;
	char	*trailingBlankPtr = NULL;


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
						trailingBlankPtr = out;	// Store the pointer to begining of the blank
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
						in++;			// Assuming this may be last character
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
						in++;		// Assuming this may be last character
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
	BOOL	rc = TRUE;

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

//#ifdef NSK_PLATFORM		// Linux port - Todo Used only in SrvrSmd.cpp
BOOL writeServerException( short retCode
						  ,	ExceptionStruct *exception_
						  ,	ExceptionStruct *prepareException
						  ,	ExceptionStruct *executeException
						  ,	ExceptionStruct *fetchException)
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
short setParamValue(long dataType, BYTE *dataPtr, BYTE *indPtr, long allocLength, char *dataValue)
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
				 /* In	*/ void *objtag_
				 , /* In	*/ const CEE_handle_def *call_id_
				 , /* Out   */ ExceptionStruct *executeException
				 , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
				 , /* In	*/ long dialogueId
				 , /* In	*/ const char *stmtLabel
				 , /* In	*/ short sqlStmtType
				 , /* In	*/ char *tableParam[]
, /* In	*/ char *inputParam[]
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
		DebugString(stmtLabel),
		CliDebugSqlStatementType(sqlStmtType),
		tableParam,
		inputParam,
		outputDesc,
		stmtId));

	SRVR_STMT_HDL		*pSrvrStmt;
	long				rowsAffected;
	SQLItemDesc_def		*SQLItemDesc;
//	SMD_QUERY_TABLE		*smdQueryTable;		// Linux port - Commenting for now
	unsigned long		curParamNo;
	//long				allocLength; 64 change
	int				allocLength;
	long				retcode;
	SQLRETURN			rc;
	short				indValue;
	BOOL				tableParamDone;
	unsigned long		index;
	long				sqlcode;

	odbc_SQLSvc_SQLError ModuleError;
	CLEAR_ERROR(ModuleError);

	// Setup module filenames for MX metadata
	//Module version is changed from 0 to "SQLCLI_ODBC_MODULE_VERSION" R3.0
	if (strncmp(stmtLabel, "SQL_GETTYPEINFO",15) == 0)
		pSrvrStmt = createSrvrStmt(dialogueId,
		stmtLabel,
		&sqlcode,
		"NONSTOP_SQLMX_NSK.MXCS_SCHEMA.CATANSIMXGTI",
		SQLCLI_ODBC_MODULE_VERSION,
		1234567890,
		sqlStmtType,
		false,true);
	else if (strncmp(stmtLabel, "SQL_JAVA_",9) == 0)
		pSrvrStmt = createSrvrStmt(dialogueId,
		stmtLabel,
		&sqlcode,
		"NONSTOP_SQLMX_NSK.MXCS_SCHEMA.CATANSIMXJAVA",
		SQLCLI_ODBC_MODULE_VERSION,
		1234567890,
		sqlStmtType,
		false,true);
	else
		pSrvrStmt = createSrvrStmt(dialogueId,
		stmtLabel,
		&sqlcode,
		"NONSTOP_SQLMX_NSK.MXCS_SCHEMA.CATANSIMX",
		SQLCLI_ODBC_MODULE_VERSION,
		1234567890,
		sqlStmtType,
		false,true);

	*stmtId = (long)pSrvrStmt;

	if (pSrvrStmt == NULL)
	{
		executeException->exception_nr = odbc_SQLSvc_PrepareFromModule_SQLError_exn_;
		kdsCreateSQLErrorException(&ModuleError, 1);
		kdsCopySQLErrorException(&ModuleError, SQLSVC_EXCEPTION_READING_FROM_MODULE_FAILED, sqlcode,
			"HY000");
		executeException->u.SQLError.errorList._length = ModuleError.errorList._length;
		executeException->u.SQLError.errorList._buffer = ModuleError.errorList._buffer;
		FUNCTION_RETURN_NUMERIC(EXECUTE_EXCEPTION,("EXECUTE_EXCEPTION"));
	}

	rc = pSrvrStmt->PrepareFromModule(INTERNAL_STMT);
	if (rc == SQL_ERROR)
	{
		executeException->exception_nr = odbc_SQLSvc_ExecuteN_SQLError_exn_;
		executeException->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
		executeException->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
		FUNCTION_RETURN_NUMERIC(EXECUTE_EXCEPTION,("EXECUTE_EXCEPTION"));
	}

	pSrvrStmt->InternalStmtClose(SQL_CLOSE);
	outputDesc->_length = pSrvrStmt->outputDescList._length;
	outputDesc->_buffer = pSrvrStmt->outputDescList._buffer;

	SRVR_DESC_HDL	*IPD;
	IPD = pSrvrStmt->IPD;
	BYTE	*dataPtr;
	BYTE	*indPtr;
	long	dataType;

	for (curParamNo = 0, index = 0,  tableParamDone = FALSE;
		curParamNo < pSrvrStmt->inputDescList._length ; curParamNo++, index++)
	{
		dataPtr = IPD[curParamNo].varPtr;
		indPtr = IPD[curParamNo].indPtr;
		dataType = IPD[curParamNo].dataType;
		SQLItemDesc = (SQLItemDesc_def *)pSrvrStmt->inputDescList._buffer + curParamNo;
		getMemoryAllocInfo(SQLItemDesc->dataType, SQLItemDesc->SQLCharset, SQLItemDesc->maxLen, 0,
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

	if (executeException->exception_nr != CEE_SUCCESS)
		FUNCTION_RETURN_NUMERIC(EXECUTE_EXCEPTION,("EXECUTE_EXCEPTION"));

	FUNCTION_RETURN_NUMERIC(0,(NULL));
}

// Compute the memory allocation requirements for the descriptor data type
void getMemoryAllocInfo(long data_type, long char_set, long data_length, long curr_mem_offset,
						long *mem_align_offset, int *alloc_size, long *var_layout)
{
	FUNCTION_ENTRY("getMemoryAllocInfo",
		("data_type=%s, char_set=%s, data_length=%ld, curr_mem_offset=%ld, mem_align_offset=0x%08x, alloc_size=0x%08x, var_layout=0x%08x",
		CliDebugSqlTypeCode(data_type),
		getCharsetEncoding(char_set),
		data_length,
		curr_mem_offset,
		mem_align_offset,
		alloc_size,
		var_layout));
	long varPad = 0;			// Bytes to pad allocation for actual data type memory requirements
	long varNulls = 0;			// Number of extra bytes that will be appended to data type (e.g. NULL for strings)
	long memAlignOffset = 0;	// Boundry offset from current memory location to set the data pointer
	long allocBoundry = 0;		// Boundry to round the size of the memory allocation to end on proper boundry

	switch (data_type)
	{
	case SQLTYPECODE_CHAR:
	case SQLTYPECODE_VARCHAR:
		if( nullRequired(char_set) )
			varNulls = 1;
		break;
	case SQLTYPECODE_VARCHAR_WITH_LENGTH:
	case SQLTYPECODE_VARCHAR_LONG:
		memAlignOffset = (((curr_mem_offset + 2 - 1) >> 1) << 1) - curr_mem_offset;
		varPad = 2;
		varNulls = 1;
		allocBoundry = 2;
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
	case SQLTYPECODE_INTERVAL:		// Treating as CHAR
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
			MEMORY_DELETE(targ->_buffer);
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


short do_ExecFetchAppend(
						 /* In    */ void *objtag_
						 , /* In    */ const CEE_handle_def *call_id_
						 , /* Out   */ ExceptionStruct *executeException
						 , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
						 , /* In    */ long dialogueId
						 , /* In    */ const char *stmtLabel
						 , /* In    */ short sqlStmtType
						 , /* In    */ char *tableParam[]
, /* In    */ char *inputParam[]
, /* Out   */ SQLItemDescList_def *outputDesc
, /* Out   */ long *rowsAffected
, /* Out   */ SQLValueList_def *outputValueList
, /* Out   */ long *stmtId
)
{
	FUNCTION_ENTRY("do_ExecFetchAppend",("objtag_=%ld, call_id_=0x%08x, executeException=0x%08x, sqlWarning=0x%08x, dialogueId=%ld, stmtLabel=%s, sqlStmtType=%s, tableParam=0x%08x, inputParam=0x%08x, outputDesc=0x%08x, stmtId=0x%08x",
		objtag_,
		call_id_,
		executeException,
		sqlWarning,
		dialogueId,
		DebugString(stmtLabel),
		CliDebugSqlStatementType(sqlStmtType),
		tableParam,
		inputParam,
		outputDesc,
		stmtId));

	SRVR_STMT_HDL		*pSrvrStmt;
	SQLItemDesc_def		*SQLItemDesc;
//	SMD_QUERY_TABLE		*smdQueryTable;		// Linux port - Commenting for now
	unsigned long		curParamNo;
	//long				allocLength;
	int				allocLength;
	long				retcode;
	SQLRETURN			rc;
	short				indValue;
	BOOL				tableParamDone;
	unsigned long		index;
	long				sqlcode;
	odbc_SQLSvc_SQLError ModuleError;
	CLEAR_ERROR(ModuleError);

	SQLValueList_def	tempOutputValueList;	// temp buffer for combined data
	CLEAR_LIST(tempOutputValueList);

	long				totalLength=0;

	// Setup module filenames for MX metadata
	if (strncmp(stmtLabel, "SQL_GETTYPEINFO",15) == 0) {
		pSrvrStmt = createSrvrStmt(dialogueId,
			stmtLabel,
			&sqlcode,
			"NONSTOP_SQLMX_NSK.MXCS_SCHEMA.CATANSIMXGTI",
			SQLCLI_ODBC_MODULE_VERSION,
			1234567890,
			sqlStmtType,
			false,true);
	}
	else if (strncmp(stmtLabel, "SQL_JAVA_",9) == 0) {
		pSrvrStmt = createSrvrStmt(dialogueId,
			stmtLabel,
			&sqlcode,
			"NONSTOP_SQLMX_NSK.MXCS_SCHEMA.CATANSIMXJAVA",
			SQLCLI_ODBC_MODULE_VERSION,
			1234567890,
			sqlStmtType,
			false,true);
	}
	else {
		pSrvrStmt = createSrvrStmt(dialogueId,
			stmtLabel,
			&sqlcode,
			"NONSTOP_SQLMX_NSK.MXCS_SCHEMA.CATANSIMX",
			SQLCLI_ODBC_MODULE_VERSION,
			1234567890,
			sqlStmtType,
			false,true);
	}

	if (pSrvrStmt == NULL){
		executeException->exception_nr = odbc_SQLSvc_PrepareFromModule_SQLError_exn_;
		kdsCreateSQLErrorException(&ModuleError, 1);
		kdsCopySQLErrorException(&ModuleError, SQLSVC_EXCEPTION_READING_FROM_MODULE_FAILED, sqlcode,
			"HY000");
		executeException->u.SQLError.errorList._length = ModuleError.errorList._length;
		executeException->u.SQLError.errorList._buffer = ModuleError.errorList._buffer;
		FUNCTION_RETURN_NUMERIC(EXECUTE_EXCEPTION,("EXECUTE_EXCEPTION"));
	}

	rc = pSrvrStmt->PrepareFromModule(INTERNAL_STMT);
	*stmtId = (long)pSrvrStmt;
	if (rc == SQL_ERROR)
	{
		executeException->exception_nr = odbc_SQLSvc_ExecuteN_SQLError_exn_;
		executeException->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
		executeException->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
		FUNCTION_RETURN_NUMERIC(EXECUTE_EXCEPTION,("EXECUTE_EXCEPTION"));
	}

#ifndef _FASTPATH
	if ((rc = AllocAssignValueBuffer(&pSrvrStmt->inputDescList,
		&pSrvrStmt->inputValueList, pSrvrStmt->inputDescVarBufferLen, 1,
		pSrvrStmt->inputValueVarBuffer)) != SQL_SUCCESS)
	{
		executeException->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
		executeException->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_BUFFER_ALLOC_FAILED;
		FUNCTION_RETURN_NUMERIC(EXECUTE_EXCEPTION,("EXECUTE_EXCEPTION"));
	}
#endif
	pSrvrStmt->InternalStmtClose(SQL_CLOSE);
	outputDesc->_length = pSrvrStmt->outputDescList._length;
	outputDesc->_buffer = pSrvrStmt->outputDescList._buffer;

#ifdef _FASTPATH

	SRVR_DESC_HDL	*IPD;
	IPD = pSrvrStmt->IPD;
	BYTE	*dataPtr;
	BYTE	*indPtr;
	long	dataType;

	// Populate the prepared module statement with the tableParam and inputParam lists
	for (curParamNo = 0, index = 0,  tableParamDone = FALSE;
		curParamNo < pSrvrStmt->inputDescList._length ; curParamNo++, index++)
	{
		dataPtr = IPD[curParamNo].varPtr;
		indPtr = IPD[curParamNo].indPtr;
		dataType = IPD[curParamNo].dataType;
		SQLItemDesc = (SQLItemDesc_def *)pSrvrStmt->inputDescList._buffer + curParamNo;
		getMemoryAllocInfo(SQLItemDesc->dataType, SQLItemDesc->SQLCharset, SQLItemDesc->maxLen, 0,
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
#else
	for (curParamNo = 0, index = 0, tableParamDone = FALSE, pSrvrStmt->inputValueList._length = 0;
		curParamNo < pSrvrStmt->inputDescList._length ; curParamNo++, index++)
	{
		SQLItemDesc = (SQLItemDesc_def *)pSrvrStmt->inputDescList._buffer + curParamNo;
		getMemoryAllocInfo(SQLItemDesc->dataType, SQLItemDesc->SQLCharset, SQLItemDesc->maxLen, 0,
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
				retcode = kdsCopyToSMDSQLValueSeq(&pSrvrStmt->inputValueList,
					SQLItemDesc->dataType, 0, tableParam[index], allocLength, SQLItemDesc->ODBCCharset);
			}
		}
		if (tableParamDone)
		{
			if  (inputParam[index] == NULL)
				indValue = -1;
			else
				indValue = 0;
			retcode = kdsCopyToSMDSQLValueSeq(&pSrvrStmt->inputValueList,
				SQLItemDesc->dataType, indValue, inputParam[index], allocLength, SQLItemDesc->ODBCCharset);
		}
		if (retcode != 0)
			FUNCTION_RETURN_NUMERIC((short) retcode,(NULL));
	}
#endif
	executeException->exception_nr = 0;

	// sqlStmtType has value of types like TYPE_SELECT, TYPE_DELETE etc.
	odbc_SQLSvc_ExecuteN_sme_(objtag_, call_id_, executeException, dialogueId, *stmtId,
		(char *)stmtLabel,
		sqlStmtType, 1, &pSrvrStmt->inputValueList, SQL_ASYNC_ENABLE_OFF, 0,
		&pSrvrStmt->outputValueList, sqlWarning);

	if (executeException->exception_nr != CEE_SUCCESS) {
		FUNCTION_RETURN_NUMERIC(EXECUTE_EXCEPTION,("EXECUTE_EXCEPTION"));
	}

	if ((pSrvrStmt = getSrvrStmt(dialogueId, *stmtId, &sqlcode)) == NULL)
	{
		executeException->exception_nr = odbc_SQLSvc_FetchN_SQLInvalidHandle_exn_;
		executeException->u.SQLInvalidHandle.sqlcode = sqlcode;
		FUNCTION_RETURN_NUMERIC(-1, ("getSrvrStmt() Failed"));
	}

	do
	{
		rc = pSrvrStmt->Fetch(SQL_MAX_COLUMNS_IN_SELECT, SQL_ASYNC_ENABLE_OFF, 0);

		switch (rc)
		{
		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO:
			appendOutputValueList(outputValueList,&pSrvrStmt->outputValueList,false);

			if (pSrvrStmt->outputValueList._length) *rowsAffected += pSrvrStmt->rowsAffected;
			else *rowsAffected = pSrvrStmt->rowsAffected;

			executeException->exception_nr = 0;

			// Save off any warnings
			if (pSrvrStmt->sqlWarning._length > 0)
			{
				sqlWarning->_length = pSrvrStmt->sqlWarning._length;
				sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
			}
			break;
		case SQL_STILL_EXECUTING:
			executeException->exception_nr = odbc_SQLSvc_FetchN_SQLStillExecuting_exn_;
			break;
		case SQL_INVALID_HANDLE:
			executeException->exception_nr = odbc_SQLSvc_FetchN_SQLInvalidHandle_exn_;
			break;
		case SQL_NO_DATA_FOUND:
			executeException->exception_nr = odbc_SQLSvc_FetchN_SQLNoDataFound_exn_;
			break;
		case SQL_ERROR:
			ERROR_DESC_def *error_desc_def;
			error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
			if (pSrvrStmt->sqlError.errorList._length != 0 &&
				(error_desc_def->sqlcode == -8007 || error_desc_def->sqlcode == -8007))
			{
				executeException->exception_nr = odbc_SQLSvc_FetchN_SQLQueryCancelled_exn_;
				executeException->u.SQLQueryCancelled.sqlcode = error_desc_def->sqlcode;
			}
			else
			{
				executeException->exception_nr = odbc_SQLSvc_FetchN_SQLError_exn_;
				executeException->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
				executeException->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
			}
			break;
		case PROGRAM_ERROR:
			executeException->exception_nr = odbc_SQLSvc_FetchN_ParamError_exn_;
			executeException->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_FETCH_FAILED;
		default:
			break;
		}

		// Loop until we have no more rows
	} while (((rc==SQL_SUCCESS) || (rc==SQL_SUCCESS_WITH_INFO)) &&
		(pSrvrStmt->rowsAffected==SQL_MAX_COLUMNS_IN_SELECT));

	FUNCTION_RETURN_NUMERIC(0,(NULL));
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
//		print_outputValueList(outputValueList, pSrvrStmt->columnCount, "outputValueList");
//		print_outputValueList(&pSrvrStmt->outputValueList, pSrvrStmt->columnCount, "pSrvrStmt->outputValueList");
void print_outputValueList(SQLValueList_def	*oVL, long colCount, const char * fcn_name) {

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
