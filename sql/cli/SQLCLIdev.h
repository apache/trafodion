/**********************************************************************
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
**********************************************************************/
#ifndef SQLCLIDEV_HDR
#define SQLCLIDEV_HDR

/* -*-C++-*-
******************************************************************************
*
* File:         SQLCLIdev.h
* Description:  Declarations for the internal NonStop SQL CLI.  This file
*               replaces the development includes of SQLCLI.h
*               
* Created:      2/3/98
* Language:     C and C++
*
*
*
*
******************************************************************************
*/

#include "sqlcli.h"
#include "Platform.h"  // 64-BIT
/*#include "ExpLOBexternal.h"*/

class ComDiagsArea;
class CliStatement;
class ComTdb;
class ExStatisticsArea;

// For internal use only -- do not document!
SQLCLI_LIB_FUNC Lng32 SQL_EXEC_AttachCodeToStatement_Internal(
                /*IN*/ SQLSTMT_ID * statement_id,
                /*IN*/ ComDiagsArea & comDiagsArea,
                /*IN*/ char *generated_code,
                /*IN*/ ULng32 generated_code_length);

SQLCLI_LIB_FUNC Lng32 SQL_EXEC_DropModule (
     /*IN*/ SQLMODULE_ID * module_name);

// For internal use only -- do not document!
SQLCLI_LIB_FUNC Lng32 SQL_EXEC_IsTransactionStarted_Internal();

// For internal use only -- do not document!
SQLCLI_LIB_FUNC void SQL_EXEC_SetParserFlagsForExSqlComp_Internal(
                /*IN*/ ULng32 flagbits);

// For internal use only -- do not document!
SQLCLI_LIB_FUNC Lng32 SQL_EXEC_SetParserFlagsForExSqlComp_Internal2(
                /*IN*/ ULng32 flagbits);

// For internal use only -- do not document!
SQLCLI_LIB_FUNC void SQL_EXEC_ResetParserFlagsForExSqlComp_Internal(
                /*IN*/ ULng32 flagbits);

// For internal use only -- do not document!
SQLCLI_LIB_FUNC Lng32 SQL_EXEC_ResetParserFlagsForExSqlComp_Internal2(
                /*IN*/ ULng32 flagbits);

Lng32 SQL_EXEC_AssignParserFlagsForExSqlComp_Internal(
						      /*IN*/ ULng32 flagbits);

Lng32 SQL_EXEC_GetParserFlagsForExSqlComp_Internal(
						      /*IN*/ ULng32 &flagbits);

Lng32 SQL_EXEC_DeleteHbaseJNI();

// For internal use only -- do not document!
SQLCLI_LIB_FUNC short SQL_EXEC_GetDefaultVolume_Internal(
                /*OUT*/ char  outBuf[],
                /*IN */ const short outBufMaxLen,  // NULL-terminated
                /*OUT*/ short &defaultVolLen);

// For internal use only -- do not document!
SQLCLI_LIB_FUNC Lng32
SQL_EXEC_GetListOfAuditedVolumes_Internal(
                /*INOUT*/ char **volNames,
                /*INOUT*/ Lng32 *numOfVols);

// For internal use only -- do not document!
SQLCLI_LIB_FUNC const char *const *const
                SQL_EXEC_GetListOfVolumes_Internal();


SQLCLI_LIB_FUNC Lng32
SQL_EXEC_GetNumOfQualifyingVolumes_Internal(
                       /*IN */  const char *nodeName,
                       /*INOUT*/ Lng32 *numOfVols);

SQLCLI_LIB_FUNC Lng32
SQL_EXEC_GetListOfQualifyingVolumes_Internal(/*IN*/ const char *nodeName,
                                             /*IN*/ Lng32 numOfVols,
                                             /*OUT*/   char **volNames,
                                             /*OUT*/   Lng32 *cpuNums,
                                             /*OUT*/   Lng32 *capacities,
                                             /*OUT*/   Lng32 *freespaces,
                                             /*OUT*/   Lng32 *largestFragments);

// For internal use only -- do not document!
//
// returns TRUE if the specified Guardian volume is up and
// would be useful for creating MX objects; returns FALSE otherwise.
//
// FILE_GETINFOLISTBYNAME_ is used to obtain information about the 
// parameter volume, which can be local or remote. Any error from
// FILE_GETINFOLISTBYNAME_ will cause FALSE to be returned.
//
// If FILE_GETINFOLISTBYNAME_ completes successfully, the following
// checks are performed
// - the volume must be a volume (doh!) The device type is checked,
//   and must have the value 3
// - the volume must be a physical volume. The device subtype is
//   checked, it cannot be 36 (SMF volume) or 52 (OSF)
// - the volume must be a TMF datavol.
//
// When FALSE is returned, the fileSystemError parameter will 
// have one of the following values
//
// fileSystemError        Means ...
// ---------------        ------------------------------------------
//  2 (FEINVALOP)         The volume is not a physical volume
// 80 (FEAUDITINVALOP)    The volume is not audited
// other FS error         Examples are 14 (device does not exist) and
//                        66 (device is down)
//
SQLCLI_LIB_FUNC Lng32 SQL_EXEC_IsVolumeUseful_Internal(
                /* IN*/ const char *const volName,  // NULL-terminated
                /*OUT*/ short &fileSystemError);

// For internal use only -- do not document!
//
// returns pointer pointing to the Tandem System Volume name
// (NULL-terminated) cached in the Executor dynamic global
// memory area.  Returns NULL if cannot get the name (after
// logging an error message in the Windows NT Even log).
//
SQLCLI_LIB_FUNC Lng32 SQL_EXEC_GetSystemVolume_Internal(
     /*INOUT*/ char * SMDlocation);

// For internal use only -- do not document!
//
// returns pointer pointing to the Tandem System Volume name
// (NULL-terminated) from any accessible NSK node.  Returns 
// local node's System Volume name if nodeName param is passed
// in as null. On NT, if the nodeName param is any of null, "NSK"
// or "\NSK", this function returns local node's System Volume name.
// Return value is 0, -EXE_NAME_MAPPING_BAD_ANCHOR (-8303), or
// -EXE_NAME_MAPPING_FS_ERROR.  In the latter case, the feError param
// is used to return the original FS error.
//
SQLCLI_LIB_FUNC Lng32 SQL_EXEC_GetRemoteSystemVolume_Internal(
     /*INOUT*/ char *SMDlocation,
     /*IN*/    const char *nodeName,
     /*INOUT*/ Lng32 *fsError);

// For internal use only -- do not document
// A new CLI call to allow catman to specify the the CLI which version of compiler
// to use to prepare a query. The caller can specify either a node name or a version but
// not both. If both are specified an error will be returned.


SQLCLI_LIB_FUNC Lng32  SQL_EXEC_SetCompilerVersion_Internal(  short mxcmpVersionToUse, char *nodeName);


SQLCLI_LIB_FUNC Lng32  SQL_EXEC_GetCompilerVersion_Internal(  short &mxcmpVersionToUse, char *nodeName);



SQLCLI_LIB_FUNC Lng32 SQL_EXEC_GetTotalTcbSpace(char*tdb, char*otherInfo);

// For internal use only -- do not document!
// This method returns the type of stats that were collected.
// If statement_id is not passed in, this method returns info for
// the last statement that was executed.
// See comexe/Comtdb.cpp, CollectStatsType enum for the numeric values
// corresponding to various stats that could be collected.
// enum SQLCLIDevCollectStatsType declared here has the same values as 
// CollectStatsType enum in ComTdb. Should we move it to a common place
// so everyone can access it? TBD. Maybe.
// This method is currenly called by mxci only.
enum SQLCLIDevCollectStatsType
{
  SQLCLIDEV_SAME_STATS    = SQLCLI_SAME_STATS,
  SQLCLIDEV_NO_STATS      = SQLCLI_NO_STATS,
  SQLCLIDEV_MEASURE_STATS = SQLCLI_MEASURE_STATS,     // collect statistics for measure counters
  SQLCLIDEV_ACCUMULATED_STATS = SQLCLI_ACCUMULATED_STATS, // collect accumulated stats. Same as measure.
  SQLCLIDEV_PERTABLE_STATS   = SQLCLI_PERTABLE_STATS,  // collect same stats that were collected in sql/mp on a per table basis
  SQLCLIDEV_ALL_STATS     = SQLCLI_ALL_STATS,      // collect all stats about all exe operators
  SQLCLIDEV_OPERATOR_STATS = SQLCLI_OPERATOR_STATS     // collect all stats but merge at 
                                   // operator(tdb) granularity. 
                                   // Used to return data at user operator
                                   // level.

};

enum SQLATTRHOLDABLE_INTERNAL_TYPE
{
  SQLCLIDEV_NONHOLDABLE = SQL_NONHOLDABLE,
  SQLCLIDEV_HOLDABLE    = SQL_HOLDABLE,
  SQLCLIDEV_ANSI_HOLDABLE = 2,
  SQLCLIDEV_PUBSUB_HOLDABLE = 3
};

SQLCLI_LIB_FUNC Lng32 SQL_EXEC_GetCollectStatsType_Internal(
		/*OUT*/ ULng32 * collectStatsType,
		/*IN*/ SQLSTMT_ID * statement_id);

// For internal use only -- do not document!
// Sets the input environ (list of envvars) in cli globals
// so they could be used by executor.
// if propagate is set to 1, then propagate environment to mxcmp now.
// Otherwise, set them in internal cli globals so they could be propagated
// the next time mxcmp is started.
SQLCLI_LIB_FUNC Lng32 SQL_EXEC_SetEnviron_Internal(/*IN*/Lng32 propagate);

#ifndef NO_SQLCLIDEV_INCLUDES
#include "sql_charset_strings.h"
#endif

// SQLDESC_CHAR_SET_CAT, SQLDESC_CHAR_SET_SCH, SQLDESC_CHAR_SET_NAM
// SQLDESC_COLLATION, SQLDESC_COLL_CAT, SQLDESC_COLL_SCH and SQLDESC_COLL_NAM
// can only be set by SQL/MX engine.
const signed char
    SQLDESC_TYPE_ORDER          =     0,
    SQLDESC_DATETIME_CODE_ORDER =     1, // ANSI DATETIME_INTERVAL_CODE
    SQLDESC_LENGTH_ORDER        =     6,
    SQLDESC_OCTET_LENGTH_ORDER  =    -1,
    SQLDESC_PRECISION_ORDER     =     3,
    SQLCESC_UNUSED_ITEM1_ORDER  =    -1,
    SQLDESC_SCALE_ORDER         =     4,
    SQLDESC_INT_LEAD_PREC_ORDER =     2, // ANSI DATETIME_INTERVAL_PRECISION
    SQLDESC_NULLABLE_ORDER      =    -1,
    SQLDESC_CHAR_SET_ORDER      =     5, 
    SQLDESC_CHAR_SET_CAT_ORDER  =    -1,
    SQLDESC_CHAR_SET_SCH_ORDER  =    -1,
    SQLDESC_CHAR_SET_NAM_ORDER  =     5, 
    SQLDESC_COLLATION_ORDER     =    -1,
    SQLDESC_COLL_CAT_ORDER      =    -1,
    SQLDESC_COLL_SCH_ORDER      =    -1,
    SQLDESC_COLL_NAM_ORDER      =    -1,
    SQLDESC_NAME_ORDER          =    -1,
    SQLDESC_UNNAMED_ORDER       =    -1,
    SQLDESC_HEADING_ORDER       =    -1,
    SQLDESC_IND_TYPE_ORDER      =     7,
    SQLDESC_VAR_PTR_ORDER       =     11,
    SQLDESC_IND_PTR_ORDER       =     10,
    SQLDESC_RET_LEN_ORDER       =    -1,
    SQLDESC_RET_OCTET_LEN_ORDER =    -1,
    SQLDESC_VAR_DATA_ORDER      =     11,
    SQLDESC_IND_DATA_ORDER      =     10,
    SQLDESC_TYPE_ANSI_ORDER     =     0,
    SQLDESC_IND_LENGTH_ORDER    =    -1,
    SQLDESC_ROWSET_VAR_LAYOUT_SIZE_ORDER   = 9,
    SQLDESC_ROWSET_IND_LAYOUT_SIZE_ORDER   = 8,
    SQLDESC_ROWSET_SIZE_ORDER              = -1,
    SQLDESC_ROWSET_HANDLE_ORDER            = -1,
    SQLDESC_ROWSET_NUM_PROCESSED_ORDER     = -1,
    SQLDESC_ROWSET_ADD_NUM_PROCESSED_ORDER = -1,
    SQLDESC_ROWSET_STATUS_PTR_ORDER        = -1,
    SQLDESC_ITEM_ORDER_COUNT      = SQLDESC_VAR_DATA_ORDER + 1;

const signed char SQLDESC_ITEM_MAX = SQLDESC_ROWSET_STATUS_PTR;

const signed char SQLDESC_ITEM_ORDER[SQLDESC_ITEM_MAX] = {
    SQLDESC_TYPE_ORDER,
    SQLDESC_DATETIME_CODE_ORDER,
    SQLDESC_LENGTH_ORDER,
    SQLDESC_OCTET_LENGTH_ORDER,
    SQLDESC_PRECISION_ORDER,
    SQLCESC_UNUSED_ITEM1_ORDER,
    SQLDESC_SCALE_ORDER,
    SQLDESC_INT_LEAD_PREC_ORDER,
    SQLDESC_NULLABLE_ORDER,
    SQLDESC_CHAR_SET_ORDER,
    SQLDESC_CHAR_SET_CAT_ORDER,
    SQLDESC_CHAR_SET_SCH_ORDER,
    SQLDESC_CHAR_SET_NAM_ORDER,
    SQLDESC_COLLATION_ORDER,
    SQLDESC_COLL_CAT_ORDER,
    SQLDESC_COLL_SCH_ORDER,
    SQLDESC_COLL_NAM_ORDER,
    SQLDESC_NAME_ORDER,
    SQLDESC_UNNAMED_ORDER,
    SQLDESC_HEADING_ORDER,
    SQLDESC_IND_TYPE_ORDER,
    SQLDESC_VAR_PTR_ORDER,
    SQLDESC_IND_PTR_ORDER,
    SQLDESC_RET_LEN_ORDER,
    SQLDESC_RET_OCTET_LEN_ORDER,
    SQLDESC_VAR_DATA_ORDER,
    SQLDESC_IND_DATA_ORDER,
    SQLDESC_TYPE_ANSI_ORDER,
    SQLDESC_IND_LENGTH_ORDER,
    SQLDESC_ROWSET_VAR_LAYOUT_SIZE_ORDER,
    SQLDESC_ROWSET_IND_LAYOUT_SIZE_ORDER,
    SQLDESC_ROWSET_SIZE_ORDER,
    SQLDESC_ROWSET_HANDLE_ORDER,
    SQLDESC_ROWSET_NUM_PROCESSED_ORDER,
    SQLDESC_ROWSET_ADD_NUM_PROCESSED_ORDER,
    SQLDESC_ROWSET_STATUS_PTR_ORDER
};

enum UDRErrorFlag {
  /* The bit offset in the bitmap vector */
  SQLUDR_SQL_VIOL   = 0x01, /* SQL access mode violation */
  SQLUDR_XACT_VIOL  = 0x02, /* attempt to issue transaction statements */
  SQLUDR_UNUSED_1   = 0x04, /* not used */
  SQLUDR_UNUSED_2   = 0x08, /* not used */
  SQLUDR_XACT_ABORT = 0x10  /* transaction was aborted */
};

SQLCLI_LIB_FUNC
Lng32 SQL_EXEC_GetUdrErrorFlags_Internal(/*OUT*/ Lng32 *udrErrorFlags);
/* returns a bitmap vector of flags defined in enum UDRErrorFlag */

SQLCLI_LIB_FUNC
Lng32 SQL_EXEC_ResetUdrErrorFlags_Internal();

SQLCLI_LIB_FUNC
Lng32 SQL_EXEC_SetUdrAttributes_Internal(/*IN*/ Lng32 sqlAccessMode,
                                        /*IN*/ Lng32 /* for future use */);

SQLCLI_LIB_FUNC
Lng32 SQL_EXEC_SetUdrRuntimeOptions_Internal(/*IN*/ const char *options,
                                            /*IN*/ ULng32 optionsLen,
                                            /*IN*/ const char *delimiters,
                                            /*IN*/ ULng32 delimsLen);

// For internal use only -- do not document!
// This method sets flag in CliGlobal to enable break handling.
SQLCLI_LIB_FUNC
Lng32 SQL_EXEC_BreakEnabled_Internal(/*IN*/ UInt32 enabled );

// For internal use only -- do not document!
// This method checks a flag in CliGlobal to see if a break signal was
// received while executing a stored proc. It also resets this
// flag. This flag is used by mxci to display the appropriate break error
// message for operations that require the RECOVER command to be run.
SQLCLI_LIB_FUNC
Lng32 SQL_EXEC_SPBreakReceived_Internal(/*OUT*/ UInt32 *breakRecvd);

// For internal use only -- do not document!
// This method merges the CLI diags area into the caller's diags area
SQLCLI_LIB_FUNC
Lng32 SQL_EXEC_MergeDiagnostics_Internal (/*INOUT*/ ComDiagsArea & newDiags);

// For internal use only -- do not document!
// This method returns the CLI diags area in packed format
SQLCLI_LIB_FUNC
Lng32 SQL_EXEC_GetPackedDiagnostics_Internal(
      /*OUT*/            char * message_buffer_ptr,
      /*IN*/    ULng32   message_obj_size,
      /*OUT*/   ULng32 * message_obj_size_needed,
      /*OUT*/            Lng32 * message_obj_type,
      /*OUT*/            Lng32 * message_obj_version);

enum ROWSET_TYPE {
  ROWSET_NOT_SPECIFIED = 0,
  ROWSET_COLUMNWISE    = 1,
  ROWSET_ROWWISE       = 2,
  ROWSET_ROWWISE_V1    = ROWSET_ROWWISE,
  ROWSET_ROWWISE_V2    = 3
};

enum SQLCLIDevVersionType
{
  SQLCLIDEV_MODULE_VERSION              = 1,
  SQLCLIDEV_STATIC_STMT_PLAN_VERSION    = 2,
  SQLCLIDEV_DYN_STMT_PLAN_VERSION       = 3,
  SQLCLIDEV_SYSTEM_VERSION              = 4,
  SQLCLIDEV_SYSTEM_MODULE_VPROC_VERSION = 5,
  SQLCLIDEV_MODULE_VPROC_VERSION        = 6
};

SQLCLI_LIB_FUNC
Lng32 SQL_EXEC_GetVersion_Internal
(/*IN*/  Lng32 versionType,
 /*OUT*/ Lng32 * versionValue,
 /*IN OPTIONAL*/ const char * nodeName,
 /*IN OPTIONAL*/ const SQLMODULE_ID * module_name,
 /*IN OPTIONAL*/ const SQLSTMT_ID * statement_id);

/******************************************************************
 *   Procedures to support TMFARLB2 features for SQL/MX tables    *
 ******************************************************************/

#ifdef __cplusplus
/* use C linkage */
extern "C" {
#endif

#define MXARLibCurrentVersion 1200

/* note that the following structures will work with any alignment
   directive, they are designed not to have any implicit fillers */

#define MXARLibAuditParamsFillerSize 88

typedef struct MXARLibAuditParamsStruct
{
  /*****************************************************************

  version_           interface version, this field should be set to
                     MXARLibCurrentVersion. If the version of the
                     caller does not match the version of the SQL/MX
                     procedures, an error may get returned (or the
                     procedures may handle the older version
                     correctly in some cases). The versioning scheme
                     here is a "lazy" scheme that avoids changing the
                     version number until the length or non-filler
                     fields of the structure change.

  guardianName_      Pointer to a Guardian name with fully qualified
                     Volume, subvolume, and file name. The system
                     name is optional and must be the local system
                     if provided.

  guardianNameLen_   Length of the Guardian name that is passed in
                     (bytes).

  modifiedFieldMap_  Pointer to an optional ModifiedFieldMap structure
                     (declared here as a char pointer to avoid
                     dependencies on other include files), set
                     to NULL (0) if this parameter is not present.

  modifiedFieldMapLen_ Length of the optional modified field map
                     structure (in bytes) or 0.

  encodedKey_        The encoded key of the record or a NULL (0)
                     pointer. This parameter is optional and is
                     not used in R2 EAP. It may at some point be
                     used to return key columns that are otherwise
                     not contained in the audit record.

  encodedKeyLen_     Length of the encoded key (in bytes) or 0 or -1.
                     If set to -1, the auditImage_ buffer is
                     assumed to contain the encoded key and the
                     after and/or before audit image(s) in a
                     contiguous block, starting at the address
                     encodedKey_. The entire block has to be
                     contained in the address range auditImage_
                     to auditImage_ + auditImageLen_.

  auditImage_        Pointer to a before or after image or a NULL
                     (0) pointer. The image must be for the table
                     specified in guardianName, and this must be
                     an SQL/MX table that exists and has the same
                     column layout as at the time when the audit
                     record was created. An error or invalid data
                     may be returned otherwise.

  auditImageLen_     Length of the audit image that is passed in
                     (bytes).

  sqlmxAuditVersion_ Version of the SQL/MX audit image format. This
                     version is passed in by the caller to make
                     sure that the generated audit can be
                     intepreted by this version of the software.

  crvsn_             CRVSN from the audit record, or zero. If non-zero
                     then the value is compared against the CRVSN of
                     the table and an error is returned if the two do
                     not match. If zero then the CRVSN check is
                     bypassed.

  filler_            For future additions. The caller must
                     initialize the filler space with zeroes.

  ******************************************************************
  */

  Lng32       version_;
  const char *guardianName_;
  Lng32       guardianNameLen_;
  const char *modifiedFieldMap_;
  Lng32       modifiedFieldMapLen_;
  const char *encodedKey_;
  Lng32       encodedKeyLen_;
  const char *auditImage_;
  Lng32       auditImageLen_;
  Lng32       sqlmxAuditVersion_;

  _int64     crvsn_;

  char       filler_[MXARLibAuditParamsFillerSize];

} MXARLibAuditParams;


typedef struct MXARLibColumnInfoStruct
{
  /*****************************************************************

  columnNum_        Column number (same as in SQL/MX metadata tables).
                    User column numbers start with 1, the SYSKEY
                    column is column #0.

  columnAnsiType_   ANSI type code for the data type of the column.
                    See table 18 in the ANSI SQL92 standard.

  columnFsType_     FS data type of the column. See file
                    sqlcli.h for the _SQLDT_* defines that describe
                    the codes returned here.

  keyField_         Set to -1 if this column is not part of the
                    clustering key, set to the key column position
                    otherwise (first column is column #0).

  dataOffset_       Offset in bytes of the data portion of this
                    column. Note that the data portion does not
                    include the null indicator or the varchar
                    length.

  dataLength_       Length in bytes of the data portion of this
                    column. The data portion will occupy <dataLength_>
                    bytes, starting at offset <dataOffset_>.

  nullIndOffset_    Offset of the NULL indicator of this column
                    in bytes. This value is undefined if the
                    column is not nullable.

  nullIndLength_    Length in bytes of the NULL indicator of this
                    column. The length is 0 if the column is not
                    nullable. Otherwise, the NULL indicator will
                    occupy <nullIndLength> bytes, starting at
                    offset <nullIndOffset>. Note that the NULL
                    indicator may or may not be adjacent to the
                    data portion of the column. This value could
                    be 0, 2, 4, or 8.

  VarLenOffset_     Similar to the NULL indicator, this
                    describes the offset of the length indicator
                    for varchars (and maybe other variable-length
                    columns in the future). This value is undefined
                    if the column does not have a variable length
                    indicator.

  varLenLength_     Size (length) in bytes of the variable length
                    indicator. This value could be 0, 2, 4, or 8.
                    A value of 0 indicates that the column does not
                    have a variable length indicator.

  datetimeIntCode_  Detailed information on datetime or interval
                    ANSI datatypes. See Tables 19 and 20 in the
                    ANSI SQL92 standard or see the SQLDTCODE_ and
                    SQLINTCODE_ constants defined in file sqlcli.h.

  datetimeFsCode_   A more detailed description if <columnFsType> has
                    the value 192 (_SQLDT_DATETIME). Other interfaces
                    such as GET DESCRIPTOR sometimes return this
                    information as the "precision" of a datetime
                    value. Here in the MXARLIB interface this value is
                    NOT returned in the precision field.  See the
                    SQLDTCODE_* constants defined in file sqlcli.h for
                    the values returned.  This value is undefined if
                    <columnFsType> is not _SQLDT_DATETIME.

                    Currently this value is always equal to
                    datetimeIntCode_. If we ever implement time zones
                    we may have to return different codes, since
                    SQL/MP uses some of the ANSI values for time zones
                    for its own datatypes.

  leadingPrecision_ This value is only set for interval data types
                    and indicates the precision of the leading part
                    of the interval (years, months, etc.).

  precision_        The precision (number of significant decimal
                    digits) for numeric values.

  scale_            The scale (decimal digits after the decimal point)
                    for numeric values. For interval, time and
                    timestamp columns, this is the "fraction
                    precision", the precision of fractional
                    seconds (a value between 0 for whole
                    seconds and 6 for microsecond resolution).

  characterSet_     Character set for a character column. See
                    SQLCHARSETCODE_ literals in file sqlcli.h.

  futureCollation_  a literal identifying the collation of a
                    character column, should collations be
                    supported in Release 2.

  filler_           for future column attributes, initialized to 0.

  To ensure upward-compatibility, please do not make assumptions on
  how information gets returned. For example, the following
  assumptions should NOT be made:
  - Assuming that the offsets in the columns are in ascending order
  - Assuming that the null indicator offset, varlen offset and
    data offset are within a contiguous range without intervening
    fillers or data from other columns
  - Assuming that the data in the image buffer is "dense",
    meaning that there are no unused spaces in it
  - Assuming that the first field in the record starts at a fixed
    offset
  - Assuming that the values of nullIndLength_ and varLenLength_
    are fixed at 2 and 4, respectively, if non-zero
  - Assuming that fields in the image buffer that are not described
    by an MXARLibColumnInfo structure are set to any given value

  ******************************************************************
  */

  Lng32       columnNum_;
  Lng32       columnAnsiType_;
  Lng32       columnFsType_;
  Lng32       keyField_;
  Lng32       dataOffset_;
  Lng32       dataLength_;
  Lng32       nullIndOffset_;
  Lng32       nullIndLength_;
  Lng32       varLenOffset_;
  Lng32       varLenLength_;
  Lng32       datetimeIntCode_;
  Lng32       datetimeFsCode_;
  Lng32       leadingPrecision_;
  Lng32       precision_;
  Lng32       scale_;
  Lng32       characterSet_;
  Lng32       futureCollation_;
  char       filler_[20];
} MXARLibColumnInfo;





/*******************************************************************
 Container for MXARLibColumnInfo structs

  ColumnCount_      Number of MXARLibColumnInfo structs present

  EncodedKeyLength_ Length of the encoded key for this table
                    (needed more for internal reasons than for
                    customers), probably not returned by TMFARLB2

********************************************************************
*/
typedef struct MXARLibImageInfoStruct
{
  Lng32              columnCount_;
  Lng32              encodedKeyLength_;
  char              filler_[120];
  MXARLibColumnInfo columnInfoArray_[1];
} MXARLibImageInfo;





/*******************************************************************
 Get before image data for a given audit record

  return code       out: FEOK or FE error number if the call is
                         not successful. NOTE: This procedure does not
                         return SQLCODE values nor does it set the
                         SQL diagnostics area.
                         Other possible error codes:
                           FENOTFOUND: underlying SQL/MX object
                                       not found
                           more errors to be added.

  arlibParams       in:  Guardian name, audit record, and
                         global parameters

  requestBitmap     in:  Requested columns bitmap. One bit is set
                          to 1 for each requested column. Bits
                         that don't correspond to any column
                         are ignored, so that the caller can
                         just pass a longer array initialized
                         with all 1 bits if desired.

  replyBitmap       out: Bitmap for returned columns, one bit
                         is set for each returned column in the
                         image.

  bitmapLength      in:  Length of request and reply bitmaps in
                         bytes.

  imageBuffer       out: Buffer with "presentation format" of
                         the before or after image     

  imageBufferLength in:  Allocated length of imageBuffer

  endImageDataOffset out: Length of the area in the image buffer
                          that contains valid data (in bytes).

  replyHint         out: Highest column number for which data was
                         returned, if this procedure completes
                         successfully. Otherwise, if FEBUFTOOSMALL
                         is returned as an error, then the
                         reply hint contains the needed size
                         of imageBuffer.

********************************************************************
*/
SQLCLI_LIB_FUNC short SQL_MXARLIB_FETCHBEFOREDATA(
    MXARLibAuditParams  *arlibParams,
    const Lng32          *requestBitmap,
    Lng32                *replyBitmap,
    Lng32                bitmapLength,
    Lng32                *imageBuffer, // $$$$ Int64??
    Lng32                imageBufferLength,
    Lng32                *endImageDataOffset,
    Lng32                *replyHint);


/*******************************************************************
 Get after image for a given audit record
 Same parameters as SQL_MXARLIB_FETCHBEFOREDATA
*/
SQLCLI_LIB_FUNC short SQL_MXARLIB_FETCHAFTERDATA(
    MXARLibAuditParams  *arlibParams,
    const Lng32          *requestBitmap,
    Lng32                *replyBitmap,
    Lng32                bitmapLength,
    Lng32                *imageBuffer, // $$$$ Int64
    Lng32                imageBufferLength,
    Lng32                *endImageDataOffset,
    Lng32                *replyHint);



/*******************************************************************
 Get information on the column layout of an SQL/MX table

  return code       out: FEOK or FE error number if the call is
                         not successful. NOTE: This procedure does not
                         return SQLCODE values nor does it set the
                         SQL diagnostics area.
                         Other possible error codes:
                           FENOTFOUND: underlying SQL/MX object
                                       not found
                           more errors to be added.

  guardianName       in:  Physical Guardian name of a
                         partition of the table for which info
                         is to be retrieved

  infoBuffer        out: Buffer with record information

  infoBufferLength  in:  allocated length of infoBuffer
                         in bytes

  imageBufferLengthNeeded  out: needed length for the
                         imageBuffer used in a subsequent
                         call to SQL_MXARLIB_FETCHxxxDATA
                         (in bytes).

  replyHint         out: This parameter indicates the needed length
                         (in bytes) for infoBuffer. If the needed 
                         length for infoBuffer
                         is greater than infoBufferLength
                         then error FEBUFTOOSMALL is returned
                         and this parameter indicates the smallest
                         acceptable value for infoBufferLength.
 
********************************************************************
*/
SQLCLI_LIB_FUNC short SQL_MXARLIB_GETCOLUMNINFO(
    const char       *guardianName,
    Lng32             guardianNameLen,
    MXARLibImageInfo *infoBuffer,
    Lng32             infoBufferLength,
    Lng32             *imageBufferLengthNeeded,
    Lng32             *replyHint);


/*******************************************************************
 Invalidate an entry in the cache of table information that the
 CLI maintains.

  return code       out: FEOK or FE error number if the call is
                         not successful.
  guardianName      in:  physical Guardian name of a partition which
                         should be invalidated in the cache. All
                         other partitions of the same file may be
                         invalidated as well.
  guardianNameLen   in:  length of Guardian name passed in
********************************************************************
*/
SQLCLI_LIB_FUNC short SQL_MXARLIB_INVALTABLEINFOCACHE(
    const char       *guardianName,
    Lng32             guardianNameLen);


/*******************************************************************
 Set the size of the cache of table information that the CLI
 maintains. This procedure should be called before any other
 CLI calls described in this section are made, but doing so is
 optional. If this method is not called before any of the calls
 that use the cache, then some default values for the cache size
 and the expiration time are assumed.

  return code       out: FEOK or FE error number if the call is
                         not successful.
  cacheSize         in:  Size of an internal cache (in megabytes)
                         that is used to store information about
                         columns, partitions, and ANSI names of
                         SQL/MX tables used in the CLI procedures.

  cacheInvTimeout   in:  Time (in centiseconds) for which we will
                         consider a cache entry valid. The timeout
                         may be 0. If a cache entry has not been
                         verified for longer than the timeout, the
                         procedures will verify it again, using a
                         comparison of the redefinition timestamp.
                         In case the redefinition timestamp does
                         not match, the cache entry will be
                         invalidated.
********************************************************************
*/
SQLCLI_LIB_FUNC short SQL_MXARLIB_SETTABLEINFOCACHE(
    Lng32             cacheSize,
    Lng32             cacheInvTimeout);


/*******************************************************************
 Return the ANSI name that corresponds to the
 Guardian name passed in

  return code       out: FEOK or FE error number if the call is
                         not successful. NOTE: This procedure does not
                         return SQLCODE values nor does it set the
                         SQL diagnostics area.
                         Other possible error codes:
                           FENOTFOUND: underlying SQL/MX object
                                       not found
                           more errors to be added.

  guardianName      in:  Physcical (non-SMF) Guardian name
                         to be resolved. The name is in external
                         format, partially or fully qualified.

  guardianNameLen   in:  Length of Guardian name passed in in bytes.

  ansiName          out: Buffer for the returned ANSI name in external
                         form. ANSI names in their external form are no
                         longer than776 bytes (3 times 258 bytes for
                         catalog, schema, and name, plus two dots).

  ansiNameMaxLength in:  Max. length of buffer for ANSI name.

  returnedAnsiNameLength out: Length of returned ANSI name. If the length
                         is greater than ansiNameLength, then an error
                         is also returned, the name is truncated, and
                         this parameter indicates the needed length.

  nameSpace         out: A pointer to a two-character buffer (to be
                         allocated by the caller) in which the name
                         space of the ANSI name will be returned.
                         Current name spaces are:
                         "TA"  Base table
                         "IX"  Index
                         "IL"  Log table for Materialized Views
                         "RL"  Range log table for Materialized Views
                         "TT"  Trigger temp table
                         "  "  Unknown name space (e. g. Resource fork)

  partitionName     An identifier of the partition of the table,
                    index, etc. (if applicable). A partition name
                    can be used for TMF recovery operations on
                    particular table partitions if the user wants
                    to specify ANSI names for these partitions.
                    The partition name should never be longer than
                    128 bytes. This parameter is optional, no
                    partition information will be returned if it
                    is set to a NULL pointer (0).

  partitionNameMaxLength Length of the buffer allocated for the
                    partition name.

  returnedPartitionNameLength actual length of the returned
                    partition identifier. Optional parameter, can
                    be set to a NULL pointer (0).

********************************************************************
*/
SQLCLI_LIB_FUNC short SQL_MXARLIB_GETANSINAME(
    const char     *guardianName,           /* in:  Guardian name    */
    Lng32           guardianNameLen,         /* in:  lenght of name   */
    char           *ansiName,               /* out: ANSI name        */
    Lng32           ansiNameMaxLength,       /* in:  len of buffer    */
    Lng32           *returnedAnsiNameLength, /* out: len of name      */
    char           *nameSpace,              /* out: table/index/etc. */
    char           *partitionName,          /* out: partition ident. */
    Lng32           partitionNameMaxLength,  /* in:  length of buffer */
    Lng32           *returnedPartitionNameLength); /* out: p name len */

Lng32 SQL_EXEC_GetAuthID(
   const char * authName,
   Lng32  & authID);

Lng32 SQL_EXEC_GetAuthName_Internal(
   Lng32   auth_id,
   char   *string_value,
   Lng32   max_string_len,
   Lng32  &len_of_item);

SQLCLI_LIB_FUNC Lng32 SQL_EXEC_GetDatabaseUserName_Internal (
    /*IN*/            Lng32   user_id,
    /*OUT*/           char   *string_value,
    /*IN*/            Lng32   max_string_len,
    /*OUT OPTIONAL*/  Lng32  *len_of_item);

SQLCLI_LIB_FUNC Lng32 SQL_EXEC_GetDatabaseUserID_Internal (
    /*IN*/            char   *string_value,
    /*OUT*/           Lng32  *numeric_value);

SQLCLI_LIB_FUNC Lng32 SQL_EXEC_SetSessionAttr_Internal (
    /*IN (SESSIONATTR_TYPE)*/  Lng32 attrName,
    /*IN OPTIONAL*/            Lng32 numeric_value,
    /*IN OPTIONAL*/            char  *string_value);

SQLCLI_LIB_FUNC Lng32 SQL_EXEC_SetErrorCodeInRTS(
                /*IN*/ SQLSTMT_ID * statement_id,
	        /*IN*/ Lng32     sqlErrorCode);  

/*
Statistics info collected for Replicate Operator
ComTdb::ex_REPLICATE in the replicator processes
*/

#define REPLICATOR_STATS_EYE_CATCHER "REOS"

typedef struct SQL_REPLICATOR_OPERATOR_STATS
{
  char    eye_catcher[4];
  _int64  operCpuTime;       
  char    source_filename[52];
  char    target_filename[52];
  Int32   blocklen;
  _int64  total_compress_time;
  _int64  total_compressed_bytes;
  _int64  total_uncompress_time;
  _int64  total_uncompressed_bytes;
  _int64  rows_read;
  _int64  total_blocks;
  _int64  blocks_replicated;
  Int32   percent_done;
  _int64  blocks_read;
} REPLICATOR_OPERATOR_STATS;


typedef struct SQLCLI_OBJ_ID SQLQUERY_ID;
/*
Registers the query in RMS shared segment for any process that runs in non-priv mode.
This function should be called from the same process that is intending the register
the simulated and/or actual query fragment. This function registers the query 
fragement and creates a root operator entry and a operator stats entry based 
on  the tdb type.

Tdb Type                    Struct type  
ComTdb::ex_REPLICATE        SQL_REPLICATOR_OPERATOR_STATS 
*/
SQLCLI_LIB_FUNC Lng32 SQL_EXEC_RegisterQuery(SQLQUERY_ID *queryId,
                                            Lng32 fragId,
                                            Lng32 tdbId,
                                            Lng32 explainTdbId,
                                            short collectStatsType,
                                            Lng32 instNum,
                                            Lng32 tdbType,
                                            char *tdbName,
                                            Lng32 tdbNameLen
                                            );


/*
Deregisters the query in RMS shared segment
*/
SQLCLI_LIB_FUNC Lng32 SQL_EXEC_DeregisterQuery(SQLQUERY_ID *queryId,
					       Lng32 fragId);

enum SECliQueryType
  {
    SE_CLI_CREATE_CONTEXT,
    SE_CLI_DROP_CONTEXT,
    SE_CLI_SWITCH_CONTEXT,
    SE_CLI_CURRENT_CONTEXT,

    // clear global diags
    SE_CLI_CLEAR_DIAGS,

    // executeImmediate
    SE_CLI_EXEC_IMMED,

    // executeImmediatePrepare
    SE_CLI_EXEC_IMMED_PREP,

    // executeImmediate clearExecFetchClose
    SE_CLI_EXEC_IMMED_CEFC,
    
    // clearExecFetchClose
    SE_CLI_CEFC,
    
    // prologue to fetch rows (prepare, set up descriptors...)
    SE_CLI_FETCH_ROWS_PROLOGUE,

    // open cursor
    SE_CLI_EXEC,

    // fetch a row
    SE_CLI_FETCH,

    // close cursor
    SE_CLI_CLOSE,

    SE_CLI_STATUS_XN,
    SE_CLI_BEGIN_XN,
    SE_CLI_COMMIT_XN,
    SE_CLI_ROLLBACK_XN,
    
    SE_CLI_GET_DATA_OFFSETS,
    SE_CLI_GET_PTR_AND_LEN,

    SE_CLI_GET_IO_LEN,

    // get attributes of the statement.
    SE_CLI_GET_STMT_ATTR,

    // deallocate the statement
    SE_CLI_DEALLOC,

    // queue of TrafSE specific info maintained in context
    SE_CLI_TRAFQ_INSERT,
    SE_CLI_TRAFQ_GET
  };

SQLCLI_LIB_FUNC Lng32 SQL_EXEC_SEcliInterface
(
 SECliQueryType qType,
 
 void* *cliInterface,  /* IN: if passed in and not null, use it.
                                                  OUT: if returned, save it and pass it back in */

 const char * inStrParam1 = NULL,
 const char * inStrParam2 = NULL,
 int inIntParam1 = -1,
 int inIntParam2 = -1,

 char* *outStrParam1 = NULL,
 char* *outStrParam2 = NULL,
 Lng32 *outIntParam1 = NULL

 );

// This method returns the pointer to the CLI ExStatistics area.
// The returned pointer is a read only pointer, its contents cannot be
// modified by the caller.
Lng32 SQL_EXEC_GetStatisticsArea_Internal 
(
 /* IN */    short statsReqType,
 /* IN */    char *statsReqStr,
 /* IN */    Lng32 statsReqStrLen,
 /* IN */    short activeQueryNum,
 /* IN */    short statsMergeType,
 /*INOUT*/ const ExStatisticsArea* &exStatsArea
 );

Int32 SQL_EXEC_SWITCH_TO_COMPILER_TYPE
(
 /*IN*/ Int32 cmpCntxtType
 );

Int32 SQL_EXEC_SWITCH_TO_COMPILER
(
 /*IN*/ void * cmpCntxt
 );

Int32 SQL_EXEC_SWITCH_BACK_COMPILER
(
 );

SQLCLI_LIB_FUNC Lng32 SQL_EXEC_SeqGenCliInterface
(
 void* *cliInterface,  /* IN: if passed in and not null, use it.
			        OUT: if returned, save it and pass it back in */

 void * seqGenAttrs
 );

#ifdef __cplusplus
/* end of C linkage */
}
#endif

#endif /* SQLCLIDEV_HDR */
