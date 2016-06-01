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
/**************************************************************************
**************************************************************************/
//
#ifndef DMFUNCTIONS_H
#define DMFUNCTIONS_H

/*** READ MODULE ****/

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define INVALID_CHARS	"[]{}(),;?*=!@\\"

extern BOOL WritePrivateProfileString (LPCSTR lpszSection, LPCSTR lpszEntry,
    LPCSTR lpszString, LPCSTR lpszFilename);

extern DWORD dwGlobalTraceVariable;
extern char szGlobalTraceFileName[];
extern DWORD gTraceFlags;
extern RETCODE SQL_API TraceCloseLogFile();
extern RETCODE SQL_API TraceOpenLogFile( LPWSTR szFileName, LPWSTR lpwszOutputMsg, DWORD cbOutputMsg);

//====================================================
/* configuration file entry */
//====================================================
typedef struct TCFGENTRY
  {
	char *section;
    char *id;
    char *value;
    char *comment;
    unsigned short flags;
  }
TCFGENTRY, *PCFGENTRY;

/* values for flags */
#define CFE_MUST_FREE_SECTION	0x8000
#define CFE_MUST_FREE_ID	0x4000
#define CFE_MUST_FREE_VALUE	0x2000
#define CFE_MUST_FREE_COMMENT	0x1000

/* configuration file */
typedef struct TCFGDATA
  {
    char *fileName;		/* Current file name */

    int dirty;			/* Did we make modifications? */

    char *image;		/* In-memory copy of the file */
    size_t size;		/* Size of this copy (excl. \0) */
    time_t mtime;		/* Modification time */

    unsigned int numEntries;
    unsigned int maxEntries;
    PCFGENTRY entries;

    /* Compatibility */
    unsigned int cursor;
    char *section;
    char *id;
    char *value;
    char *comment;
    unsigned short flags;

  }
TCONFIG, *PCONFIG;

#define CFG_VALID		0x8000
#define CFG_EOF			0x4000

#define CFG_ERROR		0x0000
#define CFG_SECTION		0x0001
#define CFG_DEFINE		0x0002
#define CFG_CONTINUE		0x0003

#define CFG_TYPEMASK		0x000F
#define CFG_TYPE(X)		((X) & CFG_TYPEMASK)
#define fun_cfg_valid(X)	((X) != NULL && ((X)->flags & CFG_VALID))
#define fun_cfg_eof(X)	((X)->flags & CFG_EOF)
#define fun_cfg_section(X)	(CFG_TYPE((X)->flags) == CFG_SECTION)
#define fun_cfg_define(X)	(CFG_TYPE((X)->flags) == CFG_DEFINE)
#define fun_cfg_cont(X)	(CFG_TYPE((X)->flags) == CFG_CONTINUE)

namespace ODBC 
{

	SQLRETURN fun_NumResultCols (
		SQLHSTMT hstmt,
		SQLSMALLINT *pccol);

	SQLRETURN fun_ExtendedFetch (
		SQLHSTMT hstmt,
		SQLUSMALLINT fFetchType,
		SQLLEN irow, 
#ifndef unixcli
		SQLULEN FAR * pcrow, 
		SQLUSMALLINT FAR * rgfRowStatus);
#else
        SQLULEN * pcrow,
        SQLUSMALLINT * rgfRowStatus);
#endif
	SQLRETURN fun_SetPos (
		SQLHSTMT StatementHandle,
		SQLUSMALLINT RowNumber,
		SQLUSMALLINT Operation,
		SQLUSMALLINT LockType);

	SQLRETURN fun_SetConnectOption (
		SQLHDBC hdbc,
		SQLUSMALLINT fOption,
		SQLUINTEGER vParam,
		bool isWideCall);

	SQLRETURN fun_GetConnectOption (
		SQLHDBC hdbc,
		SQLUSMALLINT fOption,
		SQLPOINTER pvParam,
		bool isWideCall);

	SQLRETURN fun_transact (
		HDBC hdbc,
		UWORD fType);

	SQLRETURN fun_sqlerror (
		SQLHENV henv,
		SQLHDBC hdbc,
		SQLHSTMT hstmt,
        SQLCHAR * szSqlstate,
        SQLINTEGER * pfNativeError,
        SQLCHAR * szErrorMsg,
        SQLSMALLINT cbErrorMsgMax,
        SQLSMALLINT * pcbErrorMsg,
        int bDelete,
        bool isWideCall);

	void fun_do_cursoropen (CStmt *pstmt);

	void fun_do_cursoropen_exec_direct (CStmt *pstmt);

	SQLRETURN fun_cata_state_ok ( CStmt * pstmt, int fidx);

	SQLRETURN fun_cata_state_tr ( CStmt * pstmt, int fidx, SQLRETURN result);

	char * fun_getkeyvalbydsn (char *dsn,int dsnlen,char *keywd,char *value,int size);

	char * fun_getinifile (char *buf, int size, int bIsInst, int doCreate);
}

extern BOOL ValidDSN (LPCSTR lpszDSN);

static PCFGENTRY fun_cfg_poolalloc (PCONFIG p, u_int count);

static int fun_cfg_parse (PCONFIG pconfig);

static int fun_cfg_init (PCONFIG * ppconf, const char *filename, int doCreate);
static int fun_cfg_done (PCONFIG pconfig);
static int fun_cfg_freeimage (PCONFIG pconfig);
static int fun_cfg_refresh (PCONFIG pconfig);
static int fun_cfg_storeentry (PCONFIG pconfig, char *section, char *id, char *value, char *comment, int dynamic);
static int fun_cfg_rewind (PCONFIG pconfig);
static int fun_cfg_nextentry (PCONFIG pconfig);
static int fun_cfg_find (PCONFIG pconfig, char *section, char *id);
static int fun_cfg_next_section (PCONFIG pconfig);

static int fun_cfg_write (PCONFIG pconfig, char *section, char *id, char *value);
static int fun_cfg_commit (PCONFIG pconfig);
static int fun_cfg_getstring (PCONFIG pconfig, char *section, char *id, char **valptr);
static int fun_cfg_getlong (PCONFIG pconfig, char *section, char *id, long *valptr);
static int fun_cfg_getshort (PCONFIG pconfig, char *section, char *id, short *valptr);
static int fun_cfg_search_init (PCONFIG * ppconf, const char *filename,int doCreate);
static int fun_list_entries (PCONFIG pCfg, LPCSTR lpszSection,LPSTR lpszRetBuffer, int cbRetBuffer);
static int fun_list_sections (PCONFIG pCfg, LPSTR lpszRetBuffer, int cbRetBuffer);
static BOOL do_create_dsns (PCONFIG pCfg, PCONFIG pInfCfg, LPSTR szDriver, LPSTR szDSNS, LPSTR szDiz);
static BOOL install_from_ini (PCONFIG pCfg, PCONFIG pOdbcCfg, LPSTR szInfFile, LPSTR szDriver, BOOL drivers);
static int install_from_string (PCONFIG pCfg, PCONFIG pOdbcCfg, LPSTR lpszDriver, BOOL drivers);

static char *fun_remove_quotes(const char *szString);

int GetPrivateProfileString (
		LPCSTR lpszSection, 
		LPCSTR lpszEntry,
		LPCSTR lpszDefault, 
		LPSTR lpszRetBuffer, 
		int cbRetBuffer,
		LPCSTR lpszFilename);

BOOL WritePrivateProfileString (
		LPCSTR lpszSection, 
		LPCSTR lpszEntry,
		LPCSTR lpszString, 
		LPCSTR lpszFilename);

BOOL RemoveDSNFromIni (LPCSTR lpszDSN);

BOOL RemoveDefaultDataSource (void);

BOOL WriteDSNToIni (LPCSTR lpszDSN, LPCSTR lpszDriver);

#endif
