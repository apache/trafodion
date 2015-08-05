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
#ifndef TRACECON_H
#define TRACECON_H

/*
 * The AIX xlc compiler gives an error for redefinitions like these instead of a warning
 * For now we'll just undef it - but longer term these things should not be there at all
 */
#ifdef TEXT
#undef TEXT
#endif

#define TEXT(x)			x

#define	dCSEG(x)		static const x

//

#define PRM_16BIT              		0x00000001L
#define PRM_32BIT              		0x00000002L
#define PRM_32MSK              		0x00000004L
#define PRM_32HDL              		0x00000008L
#define PRM_STR              	 	0x00000010L
#define PRM_CONN_PREFIX				0x00000020L
#define PRM_STMT_PREFIX				0x00000040L
#define PRM_HWND					0x00000080L
#define PRM_UWORD_PTR				0x00000100L
#define PRM_SDWORD_PTR				0x00000200L
#define PRM_UDWORD_PTR				0x00000400L
#define PRM_PTR						0x00000800L
#define PRM_HENV					0x00001000L
#define PRM_HDBC					0x00002000L
#define PRM_HSTMT					0x00004000L
#define PRM_HDESC					0x00008000L
#define	PRM_PTR_BIN					0x00010000L
#define PRM_PTR_32BIT				0x00020000L
#define INTERVALVAL				0x00040000L

#define	NULLPTR						9999
#define	VALIDPTR					9997
#define	SAMEAS_STRLEN				9996

#define	NumItems(x)	(sizeof(x) / sizeof(x[0]))


//*------------------------------------------------------------------------
//| DFTARRAY:
//|	This structure describes a set of default constants.  Note that an
//|	fOpt may have another array associated with it.  For example,
//|	the SQL_FETCH_DIRECTION fOpt can be any one of the SQL_FD_xxx
//|	flags.  For this case, lpLink points to the linked array of options.
//*------------------------------------------------------------------------
typedef struct tagDFTARRAY {
	SDWORD_P	fOpt;					// The option ID, note that format is based on following
	SWORD	fCType;					// The SQL_C_xxx type of fOpt 
	LPCTSTR	szOpt;					// The option name
	UDWORD_P	uVersion;				// Major/minor version of this item
	UDWORD_P	uValInfo;				// User-defined (eg: could give type for GetInfo)
	UINT	cLinks;					// Number of items in the linked array
	struct tagDFTARRAY * lpLink;		// Array of linked defaults	
	} DFTARRAY;
typedef DFTARRAY * lpDFTARRAY;

#endif

