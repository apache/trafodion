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
#define SQLUDR_LIBFUNC

#include <stdint.h>

/* C type definitions */
typedef char                  SQLUDR_CHAR;

typedef int8_t                SQLUDR_INT8;
typedef uint8_t               SQLUDR_UINT8;

typedef int16_t               SQLUDR_INT16;
typedef uint16_t              SQLUDR_UINT16;

typedef int32_t               SQLUDR_INT32;
typedef uint32_t              SQLUDR_UINT32;

typedef int64_t               SQLUDR_INT64;
typedef uint64_t              SQLUDR_UINT64;

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

/* in case we need to call another function with SQLUDR_TRAIL_ARGS */
#define PASS_ON_SQLUDR_TRAIL_ARGS \
  sqlstate, msgtext, calltype, statearea, udrinfo


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

/* in case we need to call another function with SQLUDR_TMUDF_TRAIL_ARGS */
#define PASS_ON_SQLUDR_TMUDF_TRAIL_ARGS \
  rowDataSpace1, rowDataSpace2, getRow, emitRow, PASS_ON_SQLUDR_TRAIL_ARGS


/*************************************************/
/* C++ compiler interaction interface for TMUDFs */
/*************************************************/

#ifdef __cplusplus

#include <string>
#include <vector>

// this is a class used internally by Trafodion to set up
// the environment for TMUDFs, it should not be used
// by TMUDF writers and is therefore not declared here
class TMUDFInternalSetup;

namespace tmudr
{

  typedef char * Bytes;


  // -------------------------------------------------------------------
  // UDRException
  //
  // Exception that can be thrown by code provided by the UDF writer
  // -------------------------------------------------------------------

  class UDRException
  {
  public:

    UDRException(unsigned int sqlState, const char *printf_format, ...);
    inline unsigned int getSQLState() const;
    inline const std::string &getText() const;
    // These member functions get and set the error information within a UDRException.  The SQLState 
    // value must be a five digit value in form: 38XXX, or the construction of the object will fail.

  private:

    unsigned int sqlState_;
    std::string  text_;
  };

  // class UDRWarning
  // {
  // public:
  // 
  //   UDRWarning(unsigned int sqlState, const std::string &text);
  //   unsigned int getSQLState(int i) const;
  //   const std::string &getText(int i) const;
    // These member functions get and set the warning information within a UDRException.
    // The SQLState // value must be a five digit value in form: 38XXX, or the construction of the object will fail.
  // };

  // -------------------------------------------------------------------
  // TypeInfo
  //
  // Describes an SQL data type and the corresponding C/C++ type,
  // used for scalar input parameters, columns of input rows and
  // columns of result rows.
  // -------------------------------------------------------------------

  class TypeInfo
  {
  public:

    enum CTYPE_CODE // Maps a column to its C++ type.
      {
        UNDEFINED_C_TYPE,
        INT16,           // int16_t
        INT32,           // int32_t
        INT64,           // int64_t
        UINT16,          // uint16_t
        UINT32,          // uint32_t
        UINT64,          // uint64_t
        FLOAT, 
        DOUBLE,
        CHAR_ARRAY,      // char *
        CHAR16_ARRAY,    // uint16_t *
        VARCHAR_ARRAY,   // char * with length stored in first 2 bytes
        VARCHAR16_ARRAY, // uint16_t * with length stored in first 2 bytes
        STRING,          // C++ STL string type.
        U16STRING,       // C++ STL u16string of 2 byte characters
        //BYTES,         // class UDRBytes for binary data
        //TIME           // class UDRTime for date, time, interval … values.
      };
  
    enum SQLTYPE_CODE // Maps a column to its SQL type.
      {
        UNDEFINED_SQL_TYPE,
        SMALLINT,             // 16 bit integer
        INT,                  // 32 bit integer
        LARGEINT,             // 64 bit integer
        NUMERIC,              // Numeric with decimal precision
        DECIMAL_LSE,          // Decimal, leading sign embedded
        SMALLINT_UNSIGNED,    // unsigned 16 bit integer
        INT_UNSIGNED,         // unsigned 32 bit integer
        NUMERIC_UNSIGNED,     // unsigned numeric
        DECIMAL_UNSIGNED,     // unsigned decimal
        REAL,                 // 4 byte floating point number
        DOUBLE_PRECISION,     // 8 byte floating point number
        CHAR,                 // fixed length character types.
        VARCHAR,              // varying length character types.
        DATE,
        TIME,
        TIMESTAMP,
        INTERVAL
      };
  
    enum SQLTYPE_CLASS_CODE // Classes of types defined in the SQL standard
      {
        CHARACTER_TYPE,       // char and varchar types
        NUMERIC_TYPE,         // exact and approximate numerics
        DATETIME_TYPE,        // date/time/timestamp
        INTERVAL_TYPE,        // day/month or hour/second intervals
        UNDEFINED_TYPE_CLASS
      };
  
    // character sets
    enum SQLCHARSET_CODE {
      UNDEFINED_CHARSET,
      CHARSET_ISO88591,       // ISO 8859-1, single byte western European characters
      CHARSET_UTF8,           // UTF-8, 1-4 byte Unicode encoding, length is in bytes
      CHARSET_UCS2            // UCS-2, 16 bit Unicode encoding, tolerates UTF-16
    };

    // collations
    enum SQLCOLLATION_CODE {
      UNDEFINED_COLLATION,
      SYSTEM_COLLATION
    };

    // same values as SQLINTERVAL_CODE in file trafodion/core/sql/cli/sqlcli.h
    enum SQLINTERVAL_CODE {
      UNDEFINED_INTERVAL_CODE =  0,
      INTERVAL_YEAR           =  1,
      INTERVAL_MONTH          =  2,
      INTERVAL_DAY            =  3,
      INTERVAL_HOUR           =  4,
      INTERVAL_MINUTE         =  5,
      INTERVAL_SECOND         =  6,
      INTERVAL_YEAR_MONTH     =  7,
      INTERVAL_DAY_HOUR       =  8,
      INTERVAL_DAY_MINUTE     =  9,
      INTERVAL_DAY_SECOND     = 10,
      INTERVAL_HOUR_MINUTE    = 11,
      INTERVAL_HOUR_SECOND    = 12,
      INTERVAL_MINUTE_SECOND  = 13
    };

    inline TypeInfo(const TypeInfo &type);
    // for use in UDFs, to make a TypeInfo from a C type
    TypeInfo(CTYPE_CODE ctype = UNDEFINED_C_TYPE,
             int length = 0,
             bool nullable = false);
    // for use in UDFs, to make a TypeInfo from an SQL type
    TypeInfo(SQLTYPE_CODE sqltype,
             int length = 0,
             bool nullable = false,
             int scale = 0,
             SQLCHARSET_CODE charset = CHARSET_UTF8,
             SQLINTERVAL_CODE intervalCode = UNDEFINED_INTERVAL_CODE,
             int precision = 0,
             SQLCOLLATION_CODE collation = SYSTEM_COLLATION);
    inline TypeInfo(CTYPE_CODE ctype,
                    SQLTYPE_CODE sqltype,
                    bool nullable,
                    int scale,
                    SQLCHARSET_CODE charset,
                    SQLINTERVAL_CODE intervalCode,
                    int precision,
                    SQLCOLLATION_CODE collation,
                    int length);

    inline CTYPE_CODE getCType() const;
    inline SQLTYPE_CODE getSQLType() const;
           SQLTYPE_CLASS_CODE getSQLTypeClass() const;
    inline bool getIsNullable() const;
    inline int getScale() const;
    inline SQLCHARSET_CODE getCharset() const;
    inline SQLINTERVAL_CODE getIntervalCode() const;
    inline int getPrecision() const;
    inline SQLCOLLATION_CODE getCollation() const;
    inline int getLength() const;

    // get data of a given type from a buffer
    long getLongValue(const Bytes data,
                      int dataLen) const;           // numeric value, for numeric types
    double getDoubleValue(const Bytes data,
                          int dataLen) const;       // numeric value, for bigger numeric types
    const char *getStringValue(const Bytes data,
                               int dataLen) const;  // string value for string types, in UTF8

  private:

    CTYPE_CODE        cType_;
    SQLTYPE_CODE      sqlType_;
    bool              nullable_;
    int               scale_;         // scale for exact numeric,
                                    // fraction precision for datetime/interval
    SQLCHARSET_CODE   charset_;       // for character types
    SQLINTERVAL_CODE  intervalCode_;  // for interval types
    int               precision_;     // decimal precision for exact numerics,
                                    // leading interval precision for intervals
    SQLCOLLATION_CODE collation_;     // for character types
    int               length_;        // for numeric (decimal precision) and character types

    // this class is used by the Trafodion compiler
    friend class ::TMUDFInternalSetup;
  };

  // -------------------------------------------------------------------
  // ParameterInfo
  //
  // Describes a scalar input parameter (formal or actual parameter)
  // -------------------------------------------------------------------

  class ParameterInfo
  {   
  public:

    enum VALUE_CODE // type of constant value supplied
      {
        NO_VALUE,
        STRING_VALUE,
        EXACT_NUMERIC_VALUE,
        APPROX_NUMERIC_VALUE
      };

    inline const std::string &getParameterName() const;
    inline long getLongValue() const;           // numeric value, for numeric types
    inline double getDoubleValue() const;       // numeric value, for bigger numeric types
    inline const char *getStringValue() const;  // string value for string types, in UTF8
    // later and for runtime, support arbitrary data types
    // inline const Bytes getData() const;      // binary data, for all types
    // inline const int getDataLen() const;     // length, in bytes, of string or binary data
    inline const TypeInfo &getType() const;     // The corresponding ‘C++’ type that values in this column map to.
    inline VALUE_CODE isAvailable() const;      // What type of value is available, if any

  private:

    inline ParameterInfo(const char *name,
                         const TypeInfo &type);

    std::string  name_;
    TypeInfo     type_;

    // for actual parameters that are provided
    // as constants at compile time
    VALUE_CODE   isAvailable_;
    std::string  stringValue_;
    long         exactNumericValue_;
    double       approxNumericValue_;

    // this class is used by the Trafodion compiler
    friend class ::TMUDFInternalSetup;
  };

  enum ORDER_TYPE // For outputs, the ordering of values from the first row out to the last.
    {
      NO_ORDER,    // Unordered values
      ASCENDING,   // Values ordered such that they are monotonically increasing for fixed columns 
                   // to the left in ordering list.  For example, if there are two columns, COL1 and COL2, 
                   // that are ASCENDING, then COL1 must be monotonically increasing, and COL2 
                   // must be monotonically increasing for every value of COL1.
      DESCENDING   // Values ordered such that they are monotonically decreasing for fixed columns 
                   // to the left in ordering list.  For example, if there are two columns, COL1 and COL2, 
                   // that are DESCENDING, then COL1 must be monotonically decreasing, and COL2 
                   // must be monotonically decreasing for every value of COL1.
    };

  // -------------------------------------------------------------------
  // ProvenanceInfo
  //
  // Describes where an output column is coming from.
  // -------------------------------------------------------------------

  class ProvenanceInfo
  {
  public:

    inline ProvenanceInfo();
    inline ProvenanceInfo(int inputTableNum,
                          int inputColumnNum);
    inline int getInputTableNum() const;
    inline int getInputColumnNum() const;
    inline bool isFromInputTable() const;

  private:
    int inputTableNum_;
    int inputColumnNum_;
  };

  // -------------------------------------------------------------------
  // ColumnInfo
  //
  // Describes a column in a table-valued input or in the output table
  // -------------------------------------------------------------------

  class ColumnInfo
  {
  public:

    enum COLUMN_USE // Whether the column is used.
      {
        USED,        // For an input, it’s needed by the UDF, for an output it’s needed by the SQL Engine
        NOT_USED,    // For an input, it’s not needed by the UDF,
                     // For an output it’s not needed by the SQL Engine but it is produced by the UDF.
        NOT_PRODUCED // For outputs only, a column that is not needed by the SQL Engine and will 
                     // not be produced by the UDF.
      };

    inline ColumnInfo();
    inline ColumnInfo(const char *name,
                      const TypeInfo &type,
                      COLUMN_USE usage = USED,
                      long uniqueEntries = -1);

    inline const std::string &getColName() const;
    inline const TypeInfo &getType() const;
    inline long getUniqueEntries() const;
    inline COLUMN_USE getUsage() const;
    inline const ProvenanceInfo &getProvenance() const;

    // for use during compilation
    inline void setColName(const char *name);
    inline void setType(TypeInfo &type);
    inline void setUniqueEntries(long uniqueEntries);
    inline void setUsage(COLUMN_USE usage);
    inline void setProvenance(const ProvenanceInfo &provenance);
    
  private:

    std::string name_;
    TypeInfo type_;
    COLUMN_USE usage_;
    long uniqueEntries_;
    ProvenanceInfo provenance_;
  };

  // -------------------------------------------------------------------
  // ConstraintInfo
  //
  // Describes an SQL constraint on an input or output table
  // -------------------------------------------------------------------

  class ConstraintInfo
  {
  public:

    enum CONSTRAINT_TYPE // the type of a constraint
      {
        UNIQUE         // A uniqueness constraint
      };

    ConstraintInfo(CONSTRAINT_TYPE constraintType);

    CONSTRAINT_TYPE getType() const;    
    void setType(CONSTRAINT_TYPE type);

  private:
    CONSTRAINT_TYPE constraintType_;
  };

  // -------------------------------------------------------------------
  // UniqueConstraintInfo
  // -------------------------------------------------------------------

  class UniqueConstraintInfo : public ConstraintInfo
  {
  public: 
    // todo: std::vector<ColumnInfo> &getColumns() const;    
    // todo: void setColumns(std::vector<ColumnInfo> &cols);
  };

  // -------------------------------------------------------------------
  // PredicateInfo
  //
  // Describes a predicate to be evaluated on a table
  // -------------------------------------------------------------------

  class PredicateInfo
  {
  public:

    enum PRED_OPERATOR // The relation of the predicate.
      { 
        EQUAL, 
        NOT_EQUAL,
        LESS, 
        LESS_EQUAL, 
        GREATER, 
        GREATER_EQUAL, 
        IN, 
        NOT_IN
      };

    // todo: PRED_OPERATOR getOperator() const;
    // todo: const ColumnInfo &getLeftColumn() const;
    // todo: bool isRightColumn() const;
    // todo: const ColumnInfo &getRightColumn() const;  
    // todo: const ParameterInfo &getRightValues() const;
    // todo: void absorbed(bool absorb);
  };

  // -------------------------------------------------------------------
  // PartitionInfo
  // -------------------------------------------------------------------

  class PartitionInfo : public std::vector<int>
  {
  public:

    enum PARTITION_TYPE // The type of partitioning
      { 
        ANY,          // for a set of input data, the UDF can handle any number of rows in any order.  That is, 
                      // the UDF output is only influenced by a single row of input; there is no state stored after 
                      // processing a row.  This allows the SQL engine to call the UDF in a manner best suited 
                      // to the data flow of the query.  The UDF may be required to handle all rows in one call 
                      // to processData() or in multiple parallel calls to processData(), where these processData() 
                      // calls cannot communicate.  No partitioning columns can be specified in PartitionInfo for
                      // this case (partitionCols_ must be an empty list), or an error will occur at Bind time 
                      // following the return from describeUDR().

        SINGLE,       // for a set of input data, the UDF requires that all rows be received in one call to 
                      // processData().  The rows are not ordered.  Columns cannot be specified with SINGLE 
                      // partitioning.   A partitioning scheme of SINGLE means the UDF cannot be parallelized
                      // during execution.

        REPLICATE,    // for a set of input data, the UDF requires that all rows be received in any call to 
                      // processData(), however, it allows parallel execution.  With this type of partitioning, the 
                      // UDRInfo allows for parallel execution, with each invocation receiving the full set of 
                      // input data.  The UDF should make use of getNumInstances() and getInstanceNum() 
                      // in processData() to determine which instance it is executing as. 

                      // If a UDF receives input from multiple sources, all but one input source must have 
                      // REPLICATE partitioning, since the workload can only be divided using partitioning on 
                      // one input source.  With multiple inputs, if a UDF does not specify REPLICATE 
                      // partitioning for all but one, the SQL compiler will issue an error and end the query.

        FULL,         // for a set of input data, the UDF requires that all rows for an invocation of 
                      // processData() have the same value in the columns specified by partitionCols_ and 
                      // that processData() receive all these rows in the same invocation.  In this case, 
                      // processData() will be called multiple times, likely in parallel, where these processData() 
                      // calls cannot communicate.  Each call will receive all the rows for which the partitioning 
                      // columns have the same values.  For example, columns A, B, C = 9, 20, 3 would go to one 
                      // call to processData() and 9, 15, 0 would go to another.   If no partitioning columns are 
                      // specified in partitionCols_, an error will occur at bind time following the call to 
                      // describeUDR().

        PARTIAL,      // for a set of input data, the UDF requires that all rows for an invocation of processData() 
                      // have the same value in the columns specified by partitionCols_, but does not require that 
                      // processData() receive all these rows in the same invocation. In this case, processData() 
                      // will be called multiple times, likely in parallel, where these processData() calls cannot 
                      // communicate.  Each call will receive a subset of rows for which the partitioning columns 
                      // have the same values.  For example, partitioning columns A, B, C = 9, 20, 3 would go to 
                      // one set of calls to processData() and A, B, C = 9, 15, 0 would go to a different set of calls 
                      // to processData().   If no partitioning columns are specified in partitionCols_, an error will 
                      // occur at bind time following the call to describeUDR().
      };

    PARTITION_TYPE getType() const;
    
    void setType(PARTITION_TYPE type);

  private:

    PARTITION_TYPE type_;
  };

  // -------------------------------------------------------------------
  // OrderInfo
  // -------------------------------------------------------------------

  class OrderInfo : public std::vector<ORDER_TYPE>
  {
  };

  // -------------------------------------------------------------------
  // TableInfo
  // -------------------------------------------------------------------

  class TableInfo
  {
  public: 

    inline TableInfo();
    ~TableInfo();

    // Functions for use by UDF writer, both at compile and at run time
    inline int getNumColumns() const;
    inline const ColumnInfo &getColumn(unsigned int i) const;
    const ColumnInfo &getColumn(const std::string &name) const;
    inline long getNumRows() const;
    inline const PartitionInfo &getQueryPartitioning()const;
    inline const OrderInfo &getQueryOrdering() const;
    inline bool isStream() const;
    inline int getNumConstraints() const;
    inline const ConstraintInfo &getConstraint(unsigned int i) const;

    // non-const methods, used during compile time only
    inline ColumnInfo &getColumn(unsigned int i);
    ColumnInfo &getColumn(const std::string &name);
    inline ConstraintInfo &getConstraint(unsigned int i);
    void addColumn(const ColumnInfo &column);

    // for convenient adding of columns of a common type
    void addIntegerColumn(const char *colName, bool isNullable = false);
    void addCharColumn(
         const char *colName,
         int length,
         bool isNullable = false,
         TypeInfo::SQLCHARSET_CODE charset = TypeInfo::CHARSET_UTF8,
         TypeInfo::SQLCOLLATION_CODE collation = TypeInfo::SYSTEM_COLLATION);
    void addVarCharColumn(
         const char *colName,
         int length,
         bool isNullable = false,
         TypeInfo::SQLCHARSET_CODE charset = TypeInfo::CHARSET_UTF8,
         TypeInfo::SQLCOLLATION_CODE collation = TypeInfo::SYSTEM_COLLATION);

    void addColumns(const std::vector<ColumnInfo *> &columns);
    void addColumnAt(const ColumnInfo &column, int position);
    void deleteColumn(unsigned int i);
    void deleteColumn(const std::string &name);
    inline void setNumRows(long rows);
    void addConstraint(ConstraintInfo &constraint);
    void setIsStream(bool stream);

  private:

    // this object owns all the ColumnInfo and ConstraintInfo objects
    // contained in its data members, and the destructor will delete them

    std::vector<ColumnInfo *>     columns_;
    long                          numRows_;
    PartitionInfo                 queryPartitioning_;
    OrderInfo                     queryOrdering_;
    std::vector<ConstraintInfo *> constraints_;
    
    // this class is used by the Trafodion compiler
    friend class ::TMUDFInternalSetup;
  };

  // -------------------------------------------------------------------
  // UDFWriterPrivateData
  //
  // This is a class a UDF writer can use to store any information
  // necessary for the compiler interaction functions that needs to
  // persist between calls of the individual methods. The UDF writer
  // can create derived classes, attach an object of such a class to
  // UDRInvocationInfo and UDRPlanInfo objects and use that as private
  // data. For a derived class, the destructor must deallocate any
  // resources associated with the class. Note: This private data is
  // only available during compile time, NOT at runtime.
  // -------------------------------------------------------------------

  class UDFWriterPrivateData
  {
  public:

    UDFWriterPrivateData();
    virtual ~UDFWriterPrivateData();

  };

  // -------------------------------------------------------------------
  // UDRInvocationInfo
  // -------------------------------------------------------------------

  class UDRInvocationInfo
  {
  public:

    enum FUNC_TYPE  // The type of this UDF.
      {
        ROWFUNC,              // A function that takes a single row of input per invocation and 
                              // produces 0 or more rows of outputs.  
        ROWFUNC_EQUALIZER,    // A function that takes a single row of input per invocation and 
                              // produces exactly one row of output.                                  
        ROWFUNC_FILTER,       // A function that takes a single row of input per invocation and 
                              // produces 0 or 1 rows of output. 
        SETFUNC,              // A function that takes 0 or more rows of input per invocation and 
                              // produces 0 or more rows of output. 
        SETFUNC_EQUALIZER,    // A function that takes 0 or more rows of input per invocation and 
                              // produces 1 row of output per row of input.
        SETFUNC_UNIFIER,      // A function that takes 0 or more rows of input per invocation and 
                              // produces 1 row of output.
        VECTORFUNC,           // A function that takes 0 or more ordered rows of input per invocation
                              // and produces 0 or more rows of output. 
        VECTORFUNC_EQUALIZER, // A function that takes 0 or more ordered rows of input per invocation 
                              // and produces 1 row of output per row of input.
        VECTORFUNC_UNIFIER    // A function that takes 0 or more ordered rows of input per invocation
                              // and produces 1 row of output. 
    };

    // there are no public constructors for this class

    // const Functions for use by UDF writer, both at compile and at run time
    inline const std::string &getUDRName() const;
    inline int getNumTableInputs() const;
    const TableInfo &getInputTableInfo(int childNum) const;
    inline const TableInfo &getOutputTableInfo() const;
    inline FUNC_TYPE getFuncType() const;
    inline int getNumFormalParameters() const;
    const ParameterInfo &getFormalParameterInfo(const std::string &name) const;
    inline const ParameterInfo &getFormalParameterInfo(int position) const;
    inline int getNumActualParameters() const;
    inline const ParameterInfo &getActualParameterInfo(int position) const;
    const ParameterInfo &getActualParameterInfo(const std::string &name) const;
    inline unsigned short getVersion() const;
    inline unsigned short getCurrentVersion() const { return 1; }
    inline int getNumPredicates() const;
    inline const PredicateInfo &getPredicate(unsigned int i) const;

    // Functions available at compile time only
    inline TableInfo &getOutputTableInfo();
    void setFuncType(FUNC_TYPE type);
    void addPassThruColumns(int inputTableNum,
                            int startInputColumnNum = 0,
                            int endInputColumnNum = -1);
    void addPredicate(const PredicateInfo &pred);
    inline UDFWriterPrivateData *getPrivateData();
    void setPrivateData(UDFWriterPrivateData *privateData);

    // Functions available at run-time only
    int getNumInstances() const;
    int getInstanceNum() const;

  private:

    UDRInvocationInfo();
    ~UDRInvocationInfo();

    static const int MAX_INPUT_TABLES = 2;

    std::string name_;
    int numTableInputs_;
    TableInfo inputTableInfo_[MAX_INPUT_TABLES];
    TableInfo outputTableInfo_;
    FUNC_TYPE funcType_;
    std::vector<ParameterInfo *> formalParameterInfo_;
    std::vector<ParameterInfo *> actualParameterInfo_;
    unsigned short version_;
    std::vector<PredicateInfo *> predicates_;
    UDFWriterPrivateData *udfWriterPrivateData_;

    // this class is used by the Trafodion compiler
    friend class ::TMUDFInternalSetup;
  };

  // -------------------------------------------------------------------
  // UDRPlanInfo
  // -------------------------------------------------------------------

  class UDRPlanInfo
  {
  public:
    // Functions for use by UDF writer, both at compile and at run time
    inline long getCostPerRow() const;
    inline int getDesiredDegreeOfParallelism() const;

    // Functions available at compile time only
    inline void setCostPerRow(long microseconds);

    // values that can be used in the setDesiredDegreeOfParallelism()
    // method below, in addition to positive numbers for the degree of
    // parallelism
    static const int ANY_DEGREE_OF_PARALLELISM     =  0;
    static const int DEFAULT_DEGREE_OF_PARALLELISM = -1;
    static const int MAX_DEGREE_OF_PARALLELISM     = -2;
    static const int ONE_INSTANCE_PER_NODE         = -3;
    inline void setDesiredDegreeOfParallelism(int dop);

    inline UDFWriterPrivateData *getPrivateData();
    void setPrivateData(UDFWriterPrivateData *privateData);

  private:

    UDRPlanInfo();
    ~UDRPlanInfo();

    long costPerRow_;
    int degreeOfParallelism_;
    UDFWriterPrivateData *udfWriterPrivateData_;

    // this class is used by the Trafodion compiler
    friend class ::TMUDFInternalSetup;
  };

  // ----------------------------------------------------------------------
  // class TMUDRInterface
  //
  // This class represents the default behavior of a TMUDF. TMUDF
  // writers can create a derived class and implement these methods
  // for their specific UDR
  //
  //
  // Using the compiler interface is completely optional!!
  // =====================================================
  //
  // - A TMUDF writer can decide to override none, some or all of
  //   these virtual methods.
  // - See file sqludr.cpp for the default implementation of these
  //   methods.
  // - When overriding methods, the TMUDF writer has the option to
  //   call the default method to do part of the work, and then to
  //   implement additional logic.
  // - Multiple TMUDFs could share the same subclass of TMUDRInterface.
  //   The UDR name is passed in UDRInvocationInfo, so the logic can
  //   depend on the name.
  // - A single query may invoke the same TMUDF more than once. A
  //   different UDRInvocationInfo object will be passed for each
  //   such invocation.
  // - The optimizer may try different execution plans for a TMUDF
  //   invocation, e.g. with different partitioning and ordering
  //   of input and/or output data. These alternative plans share
  //   the same UDRInvocationInfo object but they will use different
  //   UDRPlanInfo objects.
  // ----------------------------------------------------------------------

  class TMUDRInterface
  {
  public:

    TMUDRInterface();
    virtual ~TMUDRInterface();
    virtual void describeParamsAndColumns(UDRInvocationInfo &info);    // Binder
    virtual void describeDataflow(UDRInvocationInfo &info);            // Normalizer
    virtual void describeConstraints(UDRInvocationInfo &info);         // Normalizer
    virtual void describeStatistics(UDRInvocationInfo &info);          // Analyzer
    virtual void describeDesiredDegreeOfParallelism(UDRInvocationInfo &info,
                                                    UDRPlanInfo &plan);// Optimizer
    virtual void describePlanProperties(UDRInvocationInfo &info,
                                        UDRPlanInfo &plan);            // Optimizer
    virtual void completeDescription(UDRInvocationInfo &info,
                                     UDRPlanInfo &plan);               // PrecodeGen

    // Future runtime method to replace C interface.
    // When implemented, the completeDescription() method may not be required.
    // virtual void processData(UDRInvocationInfo &info,
    //                          UDRPlanInfo &plan,
    //                          UDRData &input,
    //                          UDRData &output);


  };

  // ----------------------------------------------------------------------
  // typedef CreateCompilerInterfaceObjectFunc
  //
  // Function pointer type for the factory method provided by the UDF writer
  // To use the compiler interface, the UDF writer must provide a function
  // with this signature and with a name that's the UDR name plus the
  // string "_CreateCompilerInterfaceObject".
  // Example, assuming the UDF is called "MYUDF"
  /*
      // define a class that is derived from TMUDRInterface
      class MyUDFInterface : public TMUDRInterface
      {
        // override any methods where the UDF author would
        // like to change the default behavior
        ...
      };

      // define a "factory" function to return an object of this class
      SQLUDR_LIBFUNC TMUDRInterface * MYUDF_CreateCompilerInterfaceObject(
           const UDRInvocationInfo *info)
      {
        return new MyUDFInterface;
      }
  */
  //
  // ----------------------------------------------------------------------

  typedef TMUDRInterface * (*CreateCompilerInterfaceObjectFunc) (
       const UDRInvocationInfo *info);

  ////////////////////////////////////////////////////////////////////////////
  // Implementation of inline methods
  ////////////////////////////////////////////////////////////////////////////

  // Inline methods for UDRException

  inline unsigned int UDRException::getSQLState() const   { return sqlState_; }
  inline const std::string &UDRException::getText() const     { return text_; }
  
  // Inline methods for TypeInfo

  TypeInfo::TypeInfo(const TypeInfo &type) :
       cType_(type.cType_),
       sqlType_(type.sqlType_),
       nullable_(type.nullable_),
       scale_(type.scale_),
       charset_(type.charset_),
       intervalCode_(type.intervalCode_),
       precision_(type.precision_),
       collation_(type.collation_),
       length_(type.length_)
  {}

  TypeInfo::TypeInfo(CTYPE_CODE cType,
                     SQLTYPE_CODE sqlType,
                     bool nullable,
                     int scale,
                     SQLCHARSET_CODE charset,
                     SQLINTERVAL_CODE intervalCode,
                     int precision,
                     SQLCOLLATION_CODE collation,
                     int length) :
       cType_(cType),
       sqlType_(sqlType),
       nullable_(nullable),
       scale_(scale),
       charset_(charset),
       intervalCode_(intervalCode),
       precision_(precision),
       collation_(collation),
       length_(length)
  {}
  

  TypeInfo::CTYPE_CODE TypeInfo::getCType() const            { return cType_; }
  TypeInfo::SQLTYPE_CODE TypeInfo::getSQLType() const      { return sqlType_; }
  bool TypeInfo::getIsNullable() const                    { return nullable_; }
  int TypeInfo::getScale() const                             { return scale_; }
  TypeInfo::SQLCHARSET_CODE TypeInfo::getCharset() const   { return charset_; }
  TypeInfo::SQLINTERVAL_CODE TypeInfo::getIntervalCode() const
                                                      { return intervalCode_; }
  int TypeInfo::getPrecision() const                     { return precision_; }
  TypeInfo::SQLCOLLATION_CODE TypeInfo::getCollation() const
                                                         { return collation_; }
  int TypeInfo::getLength() const                           { return length_; }

  // Inline methods for ParameterInfo

  const std::string &ParameterInfo::getParameterName() const  { return name_; }
  long ParameterInfo::getLongValue() const       { return exactNumericValue_; }
  double ParameterInfo::getDoubleValue() const  { return approxNumericValue_; }
  const char *ParameterInfo::getStringValue() const
                                                { return stringValue_.data(); }
  const TypeInfo &ParameterInfo::getType() const              { return type_; }
  ParameterInfo::VALUE_CODE ParameterInfo::isAvailable() const
                                                       { return isAvailable_; }

  ParameterInfo::ParameterInfo(const char *name,
                               const TypeInfo &type) :
       name_(name),
       type_(type),
       isAvailable_(NO_VALUE),
       exactNumericValue_(0),
       approxNumericValue_(0.0)
  {}

  // Inline methods for ProvenanceInfo

  ProvenanceInfo::ProvenanceInfo() : inputTableNum_(-1), inputColumnNum_(-1) {}
  ProvenanceInfo::ProvenanceInfo(int inputTableNum,
                                 int inputColumnNum) :
       inputTableNum_(inputTableNum),
       inputColumnNum_(inputColumnNum)                                       {}
  int ProvenanceInfo::getInputTableNum() const       { return inputTableNum_; }
  int ProvenanceInfo::getInputColumnNum() const     { return inputColumnNum_; }
  bool ProvenanceInfo::isFromInputTable() const
                        { return inputTableNum_ >= 0 && inputColumnNum_ >= 0; }

  // Inline methods for ColumnInfo

  ColumnInfo::ColumnInfo() :
       usage_(USED),
       uniqueEntries_(-1)
  {}

  ColumnInfo::ColumnInfo(const char *name,
                         const TypeInfo &type,
                         COLUMN_USE usage,
                         long uniqueEntries) :
       name_(name),
       type_(type),
       usage_(usage),
       uniqueEntries_(uniqueEntries)
  {}
  
  const std::string &ColumnInfo::getColName() const           { return name_; }
  const TypeInfo &ColumnInfo::getType() const                 { return type_; }
  long ColumnInfo::getUniqueEntries() const          { return uniqueEntries_; }
  ColumnInfo::COLUMN_USE ColumnInfo::getUsage() const        { return usage_; }
  const ProvenanceInfo &ColumnInfo::getProvenance() const
                                                        { return provenance_; }

  void ColumnInfo::setColName(const char *name)               { name_ = name; }
  void ColumnInfo::setType(TypeInfo &type)                    { type_ = type; }
  void ColumnInfo::setUniqueEntries(long uniqueEntries)
                                            { uniqueEntries_ = uniqueEntries; }
  void ColumnInfo::setUsage(COLUMN_USE usage)               { usage_ = usage; }
  void ColumnInfo::setProvenance(const ProvenanceInfo &provenance)
                                                  { provenance_ = provenance; }

  // Inline methods for ConstraintInfo

  // Inline methods for UniqueConstraintInfo

  // Inline methods for PredicateInfo

  // Inline methods for PartitionInfo

  // Inline methods for TableInfo

  TableInfo::TableInfo() : numRows_(-1) {}
  int TableInfo::getNumColumns() const              { return columns_.size(); }
  const ColumnInfo &TableInfo::getColumn(unsigned int i) const
                                                     { return *(columns_[i]); }
  long TableInfo::getNumRows() const                       { return numRows_; }
  const PartitionInfo &TableInfo::getQueryPartitioning() const
                                                 { return queryPartitioning_; }
  const OrderInfo &TableInfo::getQueryOrdering() const
                                                     { return queryOrdering_; }
  bool TableInfo::isStream() const                            { return false; }
  int TableInfo::getNumConstraints() const      { return constraints_.size(); }
  const ConstraintInfo &TableInfo::getConstraint(unsigned int i) const
                                                 { return *(constraints_[i]); }

  ColumnInfo &TableInfo::getColumn(unsigned int i)   { return *(columns_[i]); }
  ConstraintInfo &TableInfo::getConstraint(unsigned int i)
                                                 { return *(constraints_[i]); }
  void TableInfo::setNumRows(long rows)                    { numRows_ = rows; }

  // Inline methods for UDRInvocationInfo

  const std::string &UDRInvocationInfo::getUDRName() const    { return name_; }
  int UDRInvocationInfo::getNumTableInputs() const  { return numTableInputs_; }
  const TableInfo &UDRInvocationInfo::getOutputTableInfo() const
                                                   { return outputTableInfo_; }
  UDRInvocationInfo::FUNC_TYPE UDRInvocationInfo::getFuncType() const
                                                          { return funcType_; }
  int UDRInvocationInfo::getNumFormalParameters() const
                                        { return formalParameterInfo_.size(); }
  const ParameterInfo &UDRInvocationInfo::getFormalParameterInfo(int position) const
                                  { return *(formalParameterInfo_[position]); }
  int UDRInvocationInfo::getNumActualParameters() const
                                        { return actualParameterInfo_.size(); }
  const ParameterInfo &UDRInvocationInfo::getActualParameterInfo(int position) const
                                  { return *(actualParameterInfo_[position]); }
  unsigned short UDRInvocationInfo::getVersion() const     { return version_; }
  int UDRInvocationInfo::getNumPredicates() const
                                                 { return predicates_.size(); }
  const PredicateInfo &UDRInvocationInfo::getPredicate(unsigned int i) const
                                                  { return *(predicates_[i]); }
  TableInfo &UDRInvocationInfo::getOutputTableInfo()
                                                   { return outputTableInfo_; }
  UDFWriterPrivateData *UDRInvocationInfo::getPrivateData()
                                              { return udfWriterPrivateData_; }

  // Inline methods for UDRPlanInfo

  long UDRPlanInfo::getCostPerRow() const               { return costPerRow_; }
  int UDRPlanInfo::getDesiredDegreeOfParallelism() const
                                               { return degreeOfParallelism_; }
  void UDRPlanInfo::setCostPerRow(long microseconds)
                                                { costPerRow_ = microseconds; }
  void UDRPlanInfo::setDesiredDegreeOfParallelism(int dop)
                                                { degreeOfParallelism_ = dop; }

  UDFWriterPrivateData *UDRPlanInfo::getPrivateData()
                                              { return udfWriterPrivateData_; }

} // end of namespace tmudr

// end of C++ compiler interaction interface for TMUDFs
#endif

// SQLUDR_H
#endif
