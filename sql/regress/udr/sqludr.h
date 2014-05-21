/**********************************************************************
 *
 * File:         sqludr.h
 * Description:  Interface between the SQL engine and routine bodies
 * Language:     C
 *
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
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
 *
 *********************************************************************/
#ifndef SQLUDR_H
#define SQLUDR_H

/* Export declaration for DLL functions */
#ifdef _WIN32
#define SQLUDR_LIBFUNC __declspec(dllexport)
#else
#define SQLUDR_LIBFUNC
#endif

/* C type definitions */
typedef char                  SQLUDR_CHAR;

typedef char                  SQLUDR_INT8;
typedef unsigned char         SQLUDR_UINT8;

typedef short                 SQLUDR_INT16;
typedef unsigned short        SQLUDR_UINT16;

typedef long                  SQLUDR_INT32;
typedef unsigned long         SQLUDR_UINT32;

typedef long long             SQLUDR_INT64;
typedef unsigned long long    SQLUDR_UINT64;

typedef float                 SQLUDR_REAL;
typedef double                SQLUDR_DOUBLE;

/* UDR return codes */
#define SQLUDR_SUCCESS                ( 0)
#define SQLUDR_SUCCESS_WITH_WARNING   ( 1)
#define SQLUDR_ERROR                  (-1)

/* NULL indicators */
#define SQLUDR_NULL                   (-1)
#define SQLUDR_NOTNULL                ( 0)

/* To get the NULL indicator for an input value */
#define SQLUDR_GETNULLIND(null_indicator)  \
  ((((char*)(null_indicator))[1] && 0x0080) ? SQLUDR_NULL : SQLUDR_NOTNULL)

/* To set the NULL indicator for a return value */
#define SQLUDR_SETNULLIND(null_indicator) \
  (*((short *)(null_indicator)) = -1)

/* To clear the NULL indicator for a return value */
#define SQLUDR_CLEARNULLIND(null_indicator) \
  (*((short *)(null_indicator)) = 0)

/* UDR call type */
#define SQLUDR_CALLTYPE_INITIAL      ( 1)
#define SQLUDR_CALLTYPE_NORMAL       ( 2)
#define SQLUDR_CALLTYPE_FINAL        ( 3)

/* Buffer sizes for user-defined errors. These sizes include a null
   terminator. */
#define SQLUDR_SQLSTATE_SIZE        (  6)
#define SQLUDR_MSGTEXT_SIZE         (256)

/* UDR parameter styles */
enum SQLUDR_PARAMSTYLE
{
  SQLUDR_PARAMSTYLE_SQL = 1,
  SQLUDR_PARAMSTYLE_SQLROW = 2
};

/* UDR row formats */
enum SQLUDR_ROWFORMAT
{
  SQLUDR_ROWFORMAT_EXPLODED = 1
};

/* Collations for character values */
enum SQLUDR_COLLATION
{
  SQLUDR_COLLATION_UNKNOWN   = 0,
  SQLUDR_COLLATION_DEFAULT   = 1,
  SQLUDR_COLLATION_CZECH     = 2,
  SQLUDR_COLLATION_CZECH_CI  = 3,
  SQLUDR_COLLATION_SJIS      = 4
};

/* SQLUDR_BUFFER structure */
typedef struct
{
  /* Number of data bytes */
  SQLUDR_UINT32 length;
  SQLUDR_INT32 reserved1;

  /* A pointer to the data */
  SQLUDR_CHAR *data;
  SQLUDR_INT32 reserved2;

} SQLUDR_BUFFER;

/* SQLUDR_STATEAREA structure */
#define SQLUDR_STATEAREA_CURRENT_VERSION   (   1)
#define SQLUDR_STATEAREA_BUFFER_SIZE       (4096)
typedef struct
{
  /* Version of this structure */
  SQLUDR_UINT16 version;
  SQLUDR_UINT16 reserved1;
  SQLUDR_UINT32 reserved2;

  /* A memory region that persists for the lifetime   */
  /* of the host process.                             */
  SQLUDR_BUFFER host_data;

  /* A memory region that persists for the duration   */
  /* of a single statement execution.                 */
  SQLUDR_BUFFER stmt_data;

  SQLUDR_INT64 reserved3;
  SQLUDR_INT64 reserved4;

} SQLUDR_STATEAREA;

/* SQLUDR_VC_STRUCT structure for VARCHAR values */
typedef struct
{
  /* Number of bytes */
  SQLUDR_UINT32 length;
  SQLUDR_INT32 reserved1;

  /* A pointer to the data */
  char *data;
  SQLUDR_INT32 reserved2;

} SQLUDR_VC_STRUCT ;

/* SQLUDR_PARAM structure */
#define SQLUDR_PARAM_CURRENT_VERSION  1
typedef struct
{
  /* Version of this structure */
  SQLUDR_UINT16 version;

  /* Parameter type. Will be a value from the SQLTYPE_CODE   */
  /* enumeration in sqlcli.h.                                */
  SQLUDR_INT16 datatype;

  union
  {
    /* Numeric scale. Valid when the SQL type is NUMERIC or  */
    /* DECIMAL.                                              */
    SQLUDR_INT16 scale;

    /* Character set. Valid when the SQL type is any         */
    /* CHARACTER type. Will be a value from the              */
    /* SQLCHARSET_CODE enumeration in sqlcli.h.              */
    SQLUDR_INT16 character_set;

    /* Date/time code. Valid when the SQL type is            */
    /* DATE, TIME, or TIMESTAMP. Will be a value from the    */
    /* SQLDATETIME_CODE enumeration in sqlcli.h.             */
    SQLUDR_INT16 datetime_code;

    /* Interval type code. Valid when the SQL type is        */
    /* INTERVAL. Will be a value from the SQLINTERVAL_CODE   */
    /* enumeration in sqlcli.h. Note that this field is for  */
    /* future use. INTERVAL parameters are not yet           */
    /* supported.                                            */
    SQLUDR_INT16 interval_code;
  } u1;

  union
  {
    /* Numeric precision when the SQL type is NUMERIC or     */
    /* DECIMAL. Number of digits of fractional precision     */
    /* when the SQL type is TIME or TIMESTAMP.               */
    SQLUDR_INT16 precision;

    /* Collation. Valid when the SQL type is any CHARACTER   */
    /* type. Will be a value from the SQLUDR_COLLATION       */
    /* enumeration in sqludr.h.                              */
    SQLUDR_INT16 collation;
  } u2;

  /* Null-terminated parameter name */
  SQLUDR_CHAR *name;
  SQLUDR_INT32 reserved1;

  /* Maximum number of bytes in a valid data value */
  SQLUDR_UINT32 data_len;

  /* Offset within the data region. Valid only for parameter */
  /* style SQLROW.                                           */
  SQLUDR_UINT32 data_offset;

  /* Offset of the two-byte null indicator. Valid only for   */
  /* parameter style SQLROW.                                 */
  SQLUDR_UINT32 ind_offset;

  /* Offset of the four-byte VARCHAR length indicator.       */
  /* Valid only for parameter style SQLROW when datatype     */
  /* is SQLTYPECODE_VARCHAR_WITH_LENGTH.                     */
  SQLUDR_UINT32 vc_ind_offset;
  SQLUDR_UINT8  vc_ind_len;

  SQLUDR_INT8  reserved2;
  SQLUDR_INT16 reserved3;
  SQLUDR_INT32 reserved4;
  SQLUDR_INT64 reserved5;

} SQLUDR_PARAM;

#define SQLUDR_ASCENDING 1
#define SQLUDR_DESCENDING -1
#define SQLUDR_ZERO 0

typedef struct
{
  SQLUDR_UINT32 num_cols;
  SQLUDR_UINT32 *cols_list;  /* array of column indexes */
  SQLUDR_INT8   *direction;  /* defaults to zero, which indicates N/A */
}
SQLUDR_COLS_LIST;

#define SQLUDR_UNIQUE_CONSTRAINT 1
typedef struct
{
  SQLUDR_INT8 constraint_type;
  SQLUDR_COLS_LIST constraint_cols;
} SQLUDR_CONSTRAINT;


/* SQLUDR_TABLE_PARAM structure */
typedef struct
{
  SQLUDR_CHAR *table_name;
  SQLUDR_UINT32 num_params ;
  SQLUDR_PARAM *params;
  SQLUDR_COLS_LIST part_params;
  SQLUDR_COLS_LIST order_params;
  SQLUDR_UINT32 row_length;

  /* the fields below will be zero or NULL at runtime */
  SQLUDR_COLS_LIST pred_push_down_params;
  SQLUDR_UINT32 num_rows;
  SQLUDR_UINT32 *param_uecs;   /* number of entries in this list must */
                               /*  match num_params */

  /* If a uec is not known for a param the value 0 is used */
  SQLUDR_UINT32 num_constraints;
  SQLUDR_CONSTRAINT *constraints;

  SQLUDR_DOUBLE cost_per_row;
} SQLUDR_TABLE_PARAM;

/* UDRINFO structure */
#define SQLUDR_UDRINFO_CURRENT_VERSION    (1)
typedef struct
{
  /* Version of this structure */
  SQLUDR_UINT16 version;

  /* Version of SQL. For example R2.4 = 2400 */
  SQLUDR_UINT16 sql_version;

  /* Parameter passing style. Will be a value from the       */
  /* SQLUDR_PARAMSTYLE enumeration.                          */
  SQLUDR_UINT16 param_style;

  /* Parameter passing style version */
  SQLUDR_UINT16 param_style_version;

  /* Row format. Only valid for parameter style SQLROW.      */
  /* Will be a value from the SQLUDR_ROWFORMAT enumeration.  */
  SQLUDR_UINT16 row_format;

  SQLUDR_UINT16 reserved1;
  SQLUDR_UINT16 reserved2;
  SQLUDR_UINT16 reserved3;

  /* Null-terminated routine name */
  SQLUDR_CHAR *routine_name;
  SQLUDR_INT32 reserved4;

  /* Reserved fields. Can be used for catalog and schema     */
  /* names in the future                                     */
  SQLUDR_INT64 reserved5;
  SQLUDR_INT64 reserved6;

  /* UUDR action. NULL if this is not a UUDR. */
  SQLUDR_CHAR *uudr_action;
  SQLUDR_INT32 reserved7;

  /* Pass-through inputs */
  SQLUDR_UINT32 num_pass_through;
  SQLUDR_INT32 reserved8;
  SQLUDR_BUFFER *pass_through;
  SQLUDR_INT32 reserved9;

  /* Null-terminated current user name as returned by CURRENT_USER function*/
  SQLUDR_CHAR *current_user_name;
  SQLUDR_INT32 reserved10;

  /* Null-terminated login user name as returned by SESSION_USER function*/
  SQLUDR_CHAR *session_user_name;
  SQLUDR_INT32 reserved11;

  /* Null-terminated current role name as returned by CURRENT_ROLE function*/
  SQLUDR_CHAR *current_role_name;
  SQLUDR_INT32 reserved12;

  /* Null-terminated query identifier */
  SQLUDR_CHAR *query_id;
  SQLUDR_INT32 reserved13;

  /* Row length and row format. Only valid for parameter   */
  /* style SQLROW.                                         */
  SQLUDR_UINT32 in_row_length;
  SQLUDR_UINT32 out_row_length;

  /* Descriptive information about each input and output value */
  SQLUDR_UINT32 num_inputs;
  SQLUDR_UINT32 num_return_values;
  SQLUDR_PARAM *inputs;
  SQLUDR_INT32 reserved14;
  SQLUDR_PARAM *return_values;
  SQLUDR_INT32 reserved15;

  SQLUDR_INT64 reserved16;
  SQLUDR_INT64 reserved17;
  SQLUDR_INT64 reserved18;
  SQLUDR_INT64 reserved19;
  SQLUDR_INT64 reserved20;
  SQLUDR_INT64 reserved21;
  SQLUDR_INT64 reserved22;
  SQLUDR_INT64 reserved23;

  /* Instance of tmudfserv */
  SQLUDR_UINT32  instance_total;
  SQLUDR_UINT32  instance_current;

  /* Table related params info */
  SQLUDR_UINT32 num_table_inputs ;
  SQLUDR_TABLE_PARAM *table_inputs;

} SQLUDR_UDRINFO;

/* TMUDFINFO structure */
#define SQLUDR_TMUDFINFO_CURRENT_VERSION    (1)
typedef struct
{
  /* Version of this structure */
  SQLUDR_UINT16 version;

  /* Version of SQL. For example R2.4 = 2400 */
  SQLUDR_UINT16 sql_version;

  /* Null-terminated routine name */
  SQLUDR_CHAR *routine_name;

  /* Pass-through inputs */
  SQLUDR_UINT32 num_pass_through;
  SQLUDR_BUFFER *pass_through;

  /* Descriptive information about each input and output value */
  SQLUDR_UINT32 num_inputs;
  SQLUDR_PARAM *inputs;

  SQLUDR_UINT32 num_table_inputs ;
  SQLUDR_TABLE_PARAM *table_inputs;

  SQLUDR_TABLE_PARAM output_info;
} SQLUDR_TMUDFINFO;

/* SQLUDR_TRAIL_ARGS macro (to simplify function signatures) */
#define SQLUDR_TRAIL_ARGS \
  SQLUDR_CHAR        sqlstate[SQLUDR_SQLSTATE_SIZE], \
  SQLUDR_CHAR        msgtext[SQLUDR_MSGTEXT_SIZE], \
  SQLUDR_INT32       calltype, \
  SQLUDR_STATEAREA  *statearea, \
  SQLUDR_UDRINFO    *udrinfo


/* SQLTMUDF Queue State  */
typedef enum SQLUDR_Q_STATE
{
  SQLUDR_Q_MORE = 1,
  SQLUDR_Q_EOD,
  SQLUDR_Q_CANCEL
}SQLUDR_Q_STATE;


typedef void (*SQLUDR_GetNextRow)(char            *rowData,       /*OUT*/
                                  int             tableIndex,     /*IN*/
                                  SQLUDR_Q_STATE  *queue_state    /*OUT*/
                                 );

typedef void (*SQLUDR_EmitRow)  (char            *rowData,        /*IN*/
                                 int             tableIndex,      /*IN*/
                                 SQLUDR_Q_STATE  *queue_state     /*IN/OUT*/
                                 );

#define SQLUDR_TMUDF_TRAIL_ARGS \
  SQLUDR_CHAR           *rowDataSpace1,\
  SQLUDR_CHAR           *rowDataSpace2,\
  SQLUDR_GetNextRow     getRow,\
  SQLUDR_EmitRow        emitRow,\
  SQLUDR_TRAIL_ARGS

#endif
