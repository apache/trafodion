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
 * File:         CatApiRequest.h
 * Description:  This file defines classes CatApiParam and CatApiRequest.
 *
 * Created:      5/1/97
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */


#ifndef _CATAPIREQUEST_H_
#define _CATAPIREQUEST_H_


#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "Platform.h"
#ifdef NA_STD_NAMESPACE
   using namespace std;
#endif
   #ifdef CLI_DLL
      #define CATAPI_LIB_CLASS __declspec( dllexport )
   #else
      #ifdef SHARE_DLL
         #define CATAPI_LIB_CLASS __declspec( dllexport )
      #else
         #ifdef CAT_DEBUG
            #define CATAPI_LIB_CLASS __declspec( dllexport )
         #else
            #define CATAPI_LIB_CLASS __declspec( dllimport )
         #endif
      #endif
   #endif

// contents:
class CatApiParam;
class CatApiRequest;
class CatApiRequestList;
// ============================================================================
// CatAPiRequests can be run under SQLCI in the following format:
//
// CREATE TANDEM_CAT_REQUEST&1 request_type number_of_params <param1> <param2> ...;
// ============================================================================
// Definitions
// ============================================================================
enum CatApiRequestType {  UNKNOWN_REQUEST_TYPE      = 0,
                          DDL_CREATE_LOCK           = 1,
                          DDL_DROP_LOCK             = 2,
                          DDL_ALTER_LOCK            = 3,
                          ADD_PARTN                 = 4,
                          DROP_PARTN                = 5,
                          CHANGE_PARTN_STATUS       = 6,
                          UPDATE_STATS              = 7,
                          CHANGE_PARTN_FIRST_KEY    = 8,
                          UPDATE_PARTN_STATUS       = 9,
                          ENCODE_KEY                = 10,
                          UPDATE_INDEX_STATUS       = 11,
                          INVALIDATE_OPENS          = 12,
                          TOGGLE_OBJ_CORRUPT        = 13,
                          POPULATE_INDEX            = 14,
                          UPDATE_INDEX_MAP          = 15,
                          //++ MV
                          MV_SET_TABLE_EPOCH        = 16,
                          MV_SET_MVSTATUS           = 17,
                          MV_SET_REFRESHED_AT       = 18,
                          MV_SET_MV_TABLE_AUDIT     = 19,
                          //-- MV
                          SET_INDEX_LEVELS          = 20,

                          ALTER_PRIMARY_CONSTRAINT  = 21,
                          CHANGE_DDL_STATUS         = 22,
                          SET_INDEX_LEVELS2         = 23,
                          REQUEST_24_AVAILABLE      = 24, // for future use
                          SET_FILE_LABEL_FLAGS      = 25,
                          SET_ROW_HIDING_PREDICATE  = 26,
                          UPDATE_REDEF_TIME         = 27,
                          SET_REDEF_TIME            = 28,
                          ALLOCATE_EXTENTS          = 29,
                          UPDATE_MAX_EXTENTS        = 30,
                          REQUESTER_PROCESS_INFO    = 31,
                          UPDATE_PARTITION_METADATA = 32,
                          TOGGLE_OBJ_AUDIT          = 33,
#ifdef _DEBUG
                          SECRET_REGRESS_REQUEST    = 34, // Do stuff with no checks at all
#endif
                          LAZY_UPDATE_RCB           = 35,
			  UPDATE_ANSI_NAME          = 36,
                          ALTER_UIDS                = 37,
                          COMPARE_RCB               = 38,
			  RESET_OBJECT_PRIVILEGES   = 39,
			  TRANSFORM_SWITCHOVER      = 40,
                          UPDATE_VALIDATE           = 41,
			  VERIFY_OBJECT_PRIVILEGES  = 42,
                          MV_QR_PUBLISH             = 43, // MV query rewrite
                          RECREATE_SG_PARTITION     = 44,
 			  RECREATE_SS_LABEL	    = 45, 
                          DROP_SS_LABEL   	    = 46,
                          GIVE_OBJECT               = 47,
                          MOVE_VIEW_LABEL           = 48,
                          METADATA_VIEWS_FIXUP	    = 49, // Metadata views fixup
                          MV_QR_CREATE_DESC         = 50, 
                          MANAGE_SYSTEM_ROLE        = 51,
                          COMPONENT_PRIV_CHECK      = 52,

                          GET_OBJECT_PRIVILEGES     = 54,
			  RECREATE_AUDIT_CONFIG	    = 55,
                          DOWNGRADE_METADATA        = 56,
                          UPGRADE_METADATA   	    = 57,
 			  VALIDATE_VIEW		    = 58,

                          // Schema-level operations - in own interval, to minimise risk of merge conflicts
                          UPGRADE_DOWNGRADE_INIT    = 100,
                          RECOVER_INIT              = 101,
                          CREATE_DEF_SCH            = 102,
                          DROP_DEF_SCH              = 103,
                          UPDATE_TOM                = 104,
                          READ_AND_COPY_SMD         = 105,
                          CUTOVER                   = 106,
                          CUTOVER_UMD               = 107,
                          GET_OBJECTS_TOM           = 108,
                          PROCESS_OBJECTS           = 109,
                          PROCESS_SYSTEM_SCHEMA_TABLES = 110,   // V2400: Cutover for system schema tables
                          FINALISE_USER_SCHEMA      = 111,
                          REMOVE_FAKE_DEF_SCH       = 112,
                          UPGRADE_ONE_CATALOG       = 113,
                          UGDG_UNUSED_1             = 114,      // V2400: no longer in use, can be re-used
                          METADATA_FIXUP_SYSSCH     = 115,      // VL: Fixup local system schema tables
                          METADATA_FIXUP_DEFSCH     = 116,      // VL: Fixup definition schema tables, and system catalog tables (local and remote)
                          METADATA_FIXUP_UMD        = 117,      // VL: Fixup UMD tables 

                          // Last schema-level request number
                          METADATA_LAST_REQUEST     = 199,
                          // End of schema-level operations


                          LAST_API_REQUEST };

enum CatApiUnits { API_BYTES, API_KBYTES, API_MBYTES, API_GBYTES };

enum CatApiObjectType { API_TABLE, 
                        API_INDEX,
                        API_MV, 
                        API_IUD_LOG, 
                        API_RANGE_LOG,
                        API_GHOST_TABLE,
                        API_GHOST_INDEX,
                        API_GHOST_IUD_LOG,
                        API_SG,
                        API_SL
                      };

enum CatApiPartitionStatus { API_AVAILABLE,
                             API_UNAVAILABLE_OFFLINE,
                             API_UNAVAILABLE_AUDITED,
                             API_UNAVAILABLE_NON_AUDITED };
enum CatApiIndexStatus { API_INDEX_AVAILABLE,
                         API_INDEX_AVAILABLE_AND_SYSTEM_CREATED,
                         API_INDEX_NOT_AVAILABLE };

enum CatApiExplicitOrImplicitIndex { API_EXPLICIT_INDEX, API_IMPLICIT_INDEX };

enum CatApiOperation { API_UNKNOWN_OPERATION,
                       API_BACKUP,
                       API_DROPLABEL,
                       API_DUP,
                       API_EXPORT,
                       API_IMPORT,
                       API_MODIFY_TABLE,
                       API_MODIFY_INDEX,
                       API_POPULATE_INDEX,
                       API_PURGEDATA,
                       API_RECOVER,
                       API_RESTORE,
                       API_UPDATE_STATISTICS,
                       API_REFRESH,
                       API_UPDATE_PARTITION_METADATA,
		       API_ALL_METADATA_UPGRADE,
		       API_ALL_METADATA_DOWNGRADE,
                       API_TRANSFORM,
                       API_VALIDATE,
		       API_REPLICATE
                    };

enum CatApiSetEpochOperationType{ API_SET_EPOCH,
                                  API_INCREMENT_EPOCH,
                                  API_SET_EPOCH_IGNORE_LOCK,
                                  API_INCREMENT_EPOCH_IGNORE_LOCK

                                };

enum CatApiMVStatusType { API_MV_INITIALIZED,
                          API_MV_UNAVAILABLE,
                          API_MV_UNINITIALIZED};

// used by query rewrite PUBLISH table
enum CatApiMVRefreshType { API_MV_PUBLISH_REFRESH_INCREMENTAL,
                           API_MV_PUBLISH_REFRESH_RECOMPUTE,
                           API_MV_PUBLISH_REPUBLISH };

// used by query rewrite create descriptor API request
enum CatApiMVRepublishFlag { API_MV_DO_REPUBLISH,
                             API_MV_NO_REPUBLISH };

enum CatApiMVTableAuditType { API_MV_TABLE_AUDIT,
                              API_MV_TABLE_NOAUDIT };

enum CatApiDdlStatus { API_NO_DDL_IN_PROGRESS,
                       API_ROW_HIDING,
                       API_KEY_RANGE_CHECKING };

enum CatApiFileLabelFlagId { API_FL_UNKNOWN_FLAG              = 0,
                             API_FL_AUDIT_FLAG                = 1,
                             API_FL_BROKEN_FLAG               = 2,
                             API_FL_INCOMPLETE_PART_BOUND_CHG = 3,
                             API_FL_UNRECLAIMED_SPACE         = 4,
			     API_FL_AUDITCOMPRESS_FLAG        = 5};

enum CatApiTransformDependent { API_TR_DROP_DEPENDENT,
				API_TR_CASCADE_DEPENDENT,
				API_TR_RECREATE_DEPENDENT,
				API_TR_KEEP_DEPENDENT };

enum CatApiPrivType { API_UNKNOWN_PRIV,
                      API_SELECT_PRIV,
                      API_DELETE_PRIV,
                      API_INSERT_PRIV,
                      API_ALTER_PRIV,
                      API_EXECUTE_PRIV };

#define CATAPI         "CREATE TANDEM_CAT_REQUEST&"
#define APIVERSION     "2"
#define APILIST        "LIST"
#define MAXPARAMSIZE    3000
#define MAXNUMREQUESTS  1024
#define APIHEADER       CATAPI APIVERSION " " APILIST " "
#define APIHEADERLEN    strlen(CATAPI)+strlen(APIVERSION)+strlen(APILIST)+2

#define VALID_OBJECT(x) (x == API_TABLE || x == API_INDEX)
#define VALID_OBJECT_WITH_MV_IUDLOG(x) (x == API_TABLE || x == API_INDEX || \
                                  x == API_MV || x== API_IUD_LOG || x== API_SL)
#define VALID_DDL_STATUS(x) (x==API_NO_DDL_IN_PROGRESS || \
                             x==API_ROW_HIDING || \
                             x==API_KEY_RANGE_CHECKING)
#define VALID_OBJECT_RESET_PRIVILEGES(x) (x == API_TABLE || x == API_MV )
#define VALID_PRIV(x) (x == API_SELECT_PRIV || x == API_DELETE_PRIV || \
                       x == API_INSERT_PRIV || x == API_ALTER_PRIV || \
                       x == API_EXECUTE_PRIV)
                                         
// ============================================================================
// Class definition:  CatApiParam
// ============================================================================
class CATAPI_LIB_CLASS CatApiParam
{
   friend ostream &operator<< (ostream &s, const CatApiParam &p);
   friend class CatApiRequest;

  public:
    // -------------------------------------------------------------------
    // Accessors:
    // -------------------------------------------------------------------
    inline const CatApiParam * getNextParam        (void) const;
    inline const Lng32          getParamLength      (void) const;
    inline const Lng32          getParamLengthForMsg(void) const;
    inline char              * getParamValue       (void) const;

    // -------------------------------------------------------------------
    // Constructors/Destructor:
    // -------------------------------------------------------------------
    CatApiParam ( char * param );
    CatApiParam ( char * param, Lng32 lengthForMsg );
    virtual ~CatApiParam (void);

  protected:

  private:

    // -------------------------------------------------------------------
    // Constructors/Destructor:
    // -------------------------------------------------------------------
    CatApiParam ();     // do not use

    // -------------------------------------------------------------------
    // Data Members:
    // -------------------------------------------------------------------
    CatApiParam  *  nextParam_;
    char         *  param_;
    // Contains the length of the param, not as it is when the CatApiParam is
    // constructed, but what what it should be when the text of the 
    // CatApiRequest is constructed.  This is useful for a request that is
    // embedded in a "select * from table" query because during a prepare,
    // a double single-quote ('') becomes a single single-quote (').
    Lng32            paramLengthForMsg_;

    // -------------------------------------------------------------------
    // Mutators:
    // -------------------------------------------------------------------
    inline void updateNextParam( CatApiParam * nextParam );

};

// ----------------------------------------------------------------------
// inline methods for CatApiParams:
// ----------------------------------------------------------------------

// getNextParam:
inline
const CatApiParam *
CatApiParam::getNextParam(void) const
{
  return nextParam_;
}

// getParamValue:
inline
char *
CatApiParam::getParamValue( void ) const
{
  return param_;
}

// getParamLength: (the null character is not included)
inline
const Lng32
CatApiParam::getParamLength( void ) const
{
#pragma nowarn(1506)   // warning elimination 
  return strlen( param_ );
#pragma warn(1506)  // warning elimination 
}

// getParamLengthForMsg: (the null character is not included)
// Returns the length of the param that should be stored in the text of the
// CatApiRequest.  Most of the time this will be the same as the actual strlen.
// See description of paramLengthForMsg_ for more details.
inline
const Lng32
CatApiParam::getParamLengthForMsg( void ) const
{
  return (const Lng32)paramLengthForMsg_;
}

// updateNextParam:
inline
void CatApiParam::updateNextParam( CatApiParam * nextParam )
{
  nextParam_ = nextParam;
}

// end class CatApiParam

// ============================================================================
// Class definition:  CatApiRequest
// ============================================================================
 class CATAPI_LIB_CLASS CatApiRequest
 {

  public:

  // ---------------------------------------------------------------------
  // Friends:
  // ---------------------------------------------------------------------
  friend ostream &operator << (ostream &s, CatApiRequest &obj);

    // -------------------------------------------------------------------
    // Accessors:
    // -------------------------------------------------------------------
    inline const CatApiRequestType getCatApiRequestType (void) const;
    inline const Lng32              getNumParams         (void) const;
    inline const CatApiParam *     getFirstParam        (void) const;
    inline const CatApiParam *     getParam             (Lng32 index) const;

    const Lng32 getTextLength (void) const;
    short getText ( char * text
                  , const short includeSemicolon = 1
                  ) const;
    const Lng32 getSelectTextLength (void) const;
    short getSelectText ( char * text
                        , const short includeSemicolon = 1
                        ) const;
 
    const CatApiParam * operator[] (Lng32 index) const; 

    // -------------------------------------------------------------------
    // Mutators:
    // -------------------------------------------------------------------
    void addParam (Lng32 value);
    void addConstParam (const Lng32 value);
    void addInt64Param (Int64 value);
    void addConstInt64Param (const Int64 value);

    void addParam (char * value);
    void addConstParam (const char * value);
    void addParam (char * value, Lng32 length);
    void addConstParam (const char * value, const Lng32 length);
    // special method to handle strings with single quotes that will be prepared in a query
    void addConstParam (const Lng32 lengthForMsg, const char * value);

    short convertTextToCatApiRequest(const char *text);
    short convertTextToCatApiRequest(const char *text, size_t &startPos);

    // -------------------------------------------------------------------
    // Constructors/Destructor:
    // -------------------------------------------------------------------
    CatApiRequest (void);
    CatApiRequest (CatApiRequestType request);

    virtual ~CatApiRequest (void);

  protected:

  private:
    // -------------------------------------------------------------------
    // Helper methods:
    // -------------------------------------------------------------------
    inline void incrNumParams     (void);
    inline void decrNumParams     (void);

    char *getParamEnd(size_t startPos, const char *text);
    size_t getParams (const char *text);
    size_t getPreviousVersionParams (const char *text);
    short versionSupported (void);

    // -------------------------------------------------------------------
    // Mutators:
    // -------------------------------------------------------------------
    inline void setCatApiRequestType ( CatApiRequestType requestType );
           void appendParam ( CatApiParam *newParam );

    // -------------------------------------------------------------------
    // Data Members:
    // -------------------------------------------------------------------
    CatApiRequestType  requestType_;
    char              *versionInfo_;
    Lng32               numParams_;
    CatApiParam       *firstParam_;
};


// -----------------------------------------------------------------------
// inline functions -- Accessors:
// -----------------------------------------------------------------------

// getCatApiRequestType:
inline
const CatApiRequestType
CatApiRequest::getCatApiRequestType( void ) const
{
  return requestType_;
}

// getNumParams
inline
const Lng32
CatApiRequest::getNumParams( void ) const
{
  return numParams_;
}

// getFirstParam
inline
const CatApiParam *
CatApiRequest::getFirstParam( void ) const
{
  return firstParam_;
}

// getParam:
inline
const CatApiParam *
CatApiRequest::getParam( Lng32 index ) const
{
  return operator[](index);
}

// setCatApiRequestType:
inline
void
CatApiRequest::setCatApiRequestType( CatApiRequestType requestType )
{
  requestType_ = requestType;
}

// incrNumParams:
inline
void
CatApiRequest::incrNumParams( void )
{
  numParams_++;
}

// decrNumParams:
inline
void
CatApiRequest::decrNumParams( void )
{
  numParams_--;
}

// ============================================================================
// Class definition:  CatApiRequestList
// ============================================================================
 class CATAPI_LIB_CLASS CatApiRequestList
 {

  public:

  // ---------------------------------------------------------------------
  // Friends:
  // ---------------------------------------------------------------------
  friend ostream &operator << (ostream &s, CatApiRequestList &obj);

  // -------------------------------------------------------------------
  // Accessors:
  // -------------------------------------------------------------------
  inline const Lng32             getNumRequests  (void) const;
  inline const char *           getVersionInfo  (void) const;

  const CatApiRequest *         operator[]      (Lng32 index) const;
  const Lng32                    getTextLength   (void) const;
  short getText(char *text);

  // -------------------------------------------------------------------
  // Mutators:
  // -------------------------------------------------------------------
  short addRequest              (CatApiRequest *request);
  short convertTextToApiRequest (const char *text);

  // -------------------------------------------------------------------
  // Constructors/Destructor:
  // -------------------------------------------------------------------
  CatApiRequestList (void);
  virtual ~CatApiRequestList (void);

  protected:

  private:

  // --------------------------------------------------------------------
  // Data Members:
  // --------------------------------------------------------------------
  char *          versionInfo_;
  Lng32            numRequests_;
  CatApiRequest * requestList_[MAXNUMREQUESTS];

 };

// -----------------------------------------------------------------------
// inline functions:
// -----------------------------------------------------------------------
inline
const Lng32
CatApiRequestList::getNumRequests  (void) const
{
  return numRequests_;
}

inline
const char *
CatApiRequestList::getVersionInfo (void) const
{
  return versionInfo_;
}


#endif // _CatApiRequest_H_
