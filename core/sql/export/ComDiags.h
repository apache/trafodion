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
****************************************************************************
*
* File:         ComDiags.h (previously under /common)
* Description:  Declaration for class ComDiags and ComCondition.
*               Those classes represent an ANSI SQL92 "diagnostics area".
*
* Created:      5/6/98
* Language:     C++
*
*
****************************************************************************
*/

#ifndef __COM_DIAGS_H
#define __COM_DIAGS_H

////////////////
// Header Files
////////////////

  #ifdef max
    #undef max
  #endif
  #ifdef min
    #undef min
  #endif
#include "BaseTypes.h"
#include "Collections.h"
#include "IpcMessageObj.h"
#include "NAVersionedObject.h"
#include "charinfo.h"
#include "NAWinNT.h"

#include "DgBaseType.h"
#include "SqlCliDllDefines.h"

class ComCondition;
class ComDiagsArea;

// -----------------------------------------------------------------------
inline SQLCHARSET_CODE ComGetErrMsgInterfaceCharSet()
{
  return SQLCHARSETCODE_UTF8;
}

// -----------------------------------------------------------------------
// This is a constant integer with a value that is supposed to
// look "uninitialized" if users ever see it.  We use this value
// to initialize a few of the values of a ComCondition object.

const Int32 ComDiags_UnInitialized_Int = -99999;

// The SIGNAL statement has an entry in the SqlciErrors.txt file, with the
// following SQLCODE and SQLSTATE values. These are used in the code to
// return the SQLSTATE velaue supplied by the user, instead of the one
// generated from SQLCODE. See:
// 1. ComSQLSTATE() in sqlmsg\ComDiagsMsg.cpp
// 2. ComDiagsArea::getSignalSQLSTATE() in common\ComDiags.cpp

const Int32  ComDiags_SignalSQLCODE    = 3193;
const char ComDiags_SignalSQLSTATE[] = "SIGNL";

// SQLCODE corresponding to Triggered Action Exception

const Int32 ComDiags_TrigActionExceptionSQLCODE = 11028;
 
// We provide a typedef, for what ANSI refers to
// as ``an exact numeric with scale of 0.'' This is from the ANSI spec
// clause 18 describing the GET DIAGNOSTICS statement, and is *not*
// a direct specification of this implementation.  Nevertheless, we
// decide upon a typedef for the purposes of this file at least
// for the sake of consistency; there is more than one place where
// the typedef will get used.

typedef    Lng32   ComDiagBigInt;

// NOTE:
// The following group of global functions can be found in
// sqlmsg/ComDiagsMsg.cpp.

// We need to be able to map a SQLCODE value to a SQLSTATE value.
// To do this, we'll declare a function taking a long int and
// a pointer to an output buffer of at least 6 characters.
// This function will store the NULL-terminated SQLSTATE value for
// the given SQLCODE value in the output buffer.
//
// We do not provide specific functions mapping a SQLCODE value to a
// SQLSTATE class or subclass value since the caller may obtain the
// class and subclass simply by examining substrings of the returned
// SQLSTATE value.

NABoolean ComSQLSTATE(Lng32 theSQLCODE, char *theSQLSTATE);

void ComEMSSeverity(Lng32 theSQLCODE, char *theEMSSeverity);

void ComEMSEventTarget(Lng32 theSQLCODE, char *theEMSEventTarget, NABoolean forceDialout=FALSE);

void ComEMSExperienceLevel(Lng32 theSQLCODE, char *theEMSExperienceLevel);

// Closely tied to the SQLSTATE value itself is the class origin
// and subclass origin values.  See ANSI clause 18.1 for what these
// values stand for.  We define two more functions that are closely
// associated with the mapping from SQLCODE to SQLSTATE and the
// functions provide the origin information:

const char * ComClassOrigin(Lng32 theSQLCODE);
const char * ComSubClassOrigin(Lng32 theSQLCODE);

void emitError( Lng32, char *, Lng32, ... );


// -----------------------------------------------------------------------
// Class ComCondition
// -----------------------------------------------------------------------

class ComCondition : public IpcMessageObj {
public:
  // In order to for ComDiagsArea::negateCondition to do its job, it will have
  // to see into the ComCondition class.

  friend    class ComDiagsArea;

  // We allow for five char* and five long ``extra'' parameters
  // to be associated with the ComCondition class.  We define an
  // enumeration constant to represent this magic number of extra parameters.

  enum           {NumOptionalParms=5};

  // We keep a bitmap with a bit position for every data member in the
  // ComCondition class. The bitmap is used to compress data when
  // a ComCondition object gets packed into a message buffer. The bit
  // map can also help in using a "union" for multiple ComCondition fields
  // that are mutually exclusive. For example, if we never set tableName_
  // and (a future field) routineName_ at the same time, we could use a
  // single pointer for it, but use two different bitmap entries.
  // Note that the enum only has entries for data members that get sent
  // in messages (messageText_, for example, does not get sent).
  // List the items in the order in which they are packed, and note
  // in which software the item was added

  enum {
    USED_FLAGS                          = 0x00000001,  // fields for Rel. 1.5
    USED_SQLCODE                        = 0x00000002,
    USED_SERVER_NAME                    = 0x00000004,
    USED_CONNECTION_NAME                = 0x00000008,
    USED_CONSTRAINT_CATALOG             = 0x00000010,
    USED_CONSTRAINT_SCHEMA              = 0x00000020,
    USED_CONSTRAINT_NAME                = 0x00000040,
    USED_CATALOG_NAME                   = 0x00000080,
    USED_SCHEMA_NAME                    = 0x00000100,
    USED_TABLE_NAME                     = 0x00000200,
    USED_COLUMN_NAME                    = 0x00000400,
    USED_SQLID                          = 0x00000800,
    USED_ROW_NUMBER                     = 0x00001000,
    USED_NSK_CODE                       = 0x00002000,
    USED_NUM_STRING_PARAMS              = 0x00004000,
    USED_NUM_INT_PARAMS                 = 0x00008000, // end of Rel 1.5 fields
    // can be used to mask out fields that the current
    // version of the code doesn't understand (used
    // to add ComCondition fields without incrementing
    // the version number of the object)
    USED_CUSTOM_SQLSTATE		= 0x00010000,
    USED_TRIGGER_CATALOG                = 0x00020000,
    USED_TRIGGER_SCHEMA                 = 0x00040000,
    USED_TRIGGER_NAME                   = 0x00080000,
    // USED_ISO_MAPPING_CHARSET            = 0x00100000,
    USED_FUTURE_FIELDS                  = 0xFFFE0000
  };

  // flags in the flagsTBS_ field (these flags get sent when a ComCondition
  // is packed into a message)
  enum {
    // this flag indicates that at some point we lost certain fields
    // which we didn't understand when we received them from another process
    FLAGS_TBS_SUPPRESSED_UPREV_FIELDS   = 0x00000001
  };

  enum {
    INVALID_ROWNUMBER = -1,
    NONFATAL_ERROR = -2
  };

  enum {
    NO_LIMIT_ON_ERROR_CONDITIONS = -1
  };

  enum {
    NO_MORE = 0,
    MORE_ERRORS = 1,
    MORE_WARNINGS = 2
  };

  ComCondition();

  // If you want to dynamically allocate a ComCondition, you can't
  // use "new."    Instead, you must use "allocate" and later "deAllocate"
  // to free this object.  This is so that we can allow the option of
  // this class' user supplying a CollHeap to manage storage.

  static  ComCondition   *allocate      (CollHeap* = NULL);

          void            deAllocate    ();

  // The destructor is not declared virtual in order
  // to ease the implementation of IPC functionality for the ComCondition
  // class.  Note that any programmer deriving a class from ComCondition
  // therefore needs to be careful about not using their derived class
  // in a way that their destructor does not get called.
  //
  // It is the job of the destructor to free all storage held by
  // this ComCondition object and to free that storage using
  // the heap which the caller specified at construction time.
  //
  // The destructor **does not free** the heap referenced by collHeapPtr_.

  ~ComCondition             ();

  // Here is an assignment operator.

  ComCondition   &operator=     (const ComCondition&);

  // The clear() member function frees all storage used by this class,
  // and, leaves the collHeapPtr_ __intact__ (it's still there).  All
  // other members are "reset" to what they would be just after
  // construction.

  void clear                ();

  // We must guarantee of the ComCondition class:
  // This will allow that some users of ComCondition will be in
  // privileged SRL (shared runtime libraries).
  //
  // So we have the following members which provide access to the
  // message text of a ComCondition object:

  //
  // The following three methods are only exported through tdm_sqlcli.dll
  // but not tdm_sqlexport.dll due to indirect dependencies on C-run time.
  //
  const NAWchar     *      getMessageText              (NABoolean prefixAdded = TRUE);

  const NAWchar     * const getMessageText              (NABoolean prefixAdded,
                                                         CharInfo::CharSet isoMapCS);

  ComDiagBigInt             getMessageLength            ();

  ComDiagBigInt             getMessageOctetLength       ();

  NABoolean                 isLocked                    () const;

  // We need to provide set and met methods for the data elements
  // of a ComCondition class object.  It is straightforward to
  // declare a set of methods for getting each of the attributes
  // of a ComCondition object.
  //
  // In the case of the string data, for the GET DIAGNOSTICS statement
  // ANSI requires that the returned type be CHARACTER_VARYING(L) where
  // L is not less then 128.  For our purposes, a const char* will do.

  ComDiagBigInt              getConditionNumber    () const;

  //
  // The following three methods are only exported through tdm_sqlcli.dll
  // but not tdm_sqlexport.dll due to indirect dependencies on C-run time.
  //

  void                       getSQLSTATE           (char *theSQLSTATE) const;

  const char         * getClassOrigin              () const;

  const char         * getSubClassOrigin           () const;

  const char         * getServerName               () const;

  const char         * getConnectionName           () const;

  const char         * getConstraintCatalog        () const;

  const char         * getConstraintSchema         () const;

  const char         * getConstraintName           () const;

  const char         * getTriggerCatalog           () const;

  const char         * getTriggerSchema            () const;

  const char         * getTriggerName              () const;

  const char         * getCatalogName              () const;

  const char         * getSchemaName               () const;

  const char         * getTableName                () const;

  const char         * getColumnName               () const;


  const char         * getSqlID                  () const;

  Lng32                       getRowNumber          () const;

  Lng32                       getNskCode            () const;


  Lng32                       getSQLCODE            () const;

  Lng32                  getEMSEventVisits () const;

  void                   incrEMSEventVisits ();



  // Each of these set char* functions ** must be implemented to copy
  // the buffers given as arguments.**  This is so that all storage
  // owned by a ComCondition object is kept on a heap which the
  // ComCondition object is told about when it is constructed.
  //
  // Remember also that there will be an assertion failure if any of these
  // set members are called on an object which is in a locked state.
  //
  // You cannot directly set a SQLSTATE nor a class origin,
  // or a subclass origin.
  // This is because in our design that information is all derived from
  // the SQLCODE.  You *are* allowed to set the SQLCODE.


  void                      setConditionNumber    (ComDiagBigInt);

  void                      setSQLCODE            (Lng32);

  void                      setServerName         (const char *const);

  void                      setConnectionName     (const char *const);

  void                      setConstraintCatalog  (const char *const);

  void                      setConstraintSchema   (const char *const);

  void                      setConstraintName     (const char *const);

  void                      setTriggerCatalog  (const char *const);

  void                      setTriggerSchema   (const char *const);

  void                      setTriggerName     (const char *const);

  void                      setCatalogName        (const char *const);

  void                      setSchemaName         (const char *const);

  void                      setTableName          (const char *const);


  void                      setCustomSQLState         (const char *const);

  const char *              getCustomSQLState         ()
  {
    return customSQLState_;
  }

  void                      setColumnName         (const char *const);

  void                      setSqlID            (const char *const);

  void                      setRowNumber          (  Lng32);

  // Next, we declare a set of get and set functions which
  // each take an indexing argument to refer to one of these optional
  // parameters.  It is an assertion failure to call one of these
  // get or set functions with the index out of range.  The range
  // is 0...NumOptionalParms-1.



  void                      setNskCode            (  Lng32);
  


  NABoolean                 hasOptionalString        (Lng32)  const;

  // Next, we declare a set of get and set functions which           
  // each take an indexing argument to refer to one of these optional        
  // parameters.  It is an assertion failure to call one of these            
  // get or set functions with the index out of range.  The range    
  // is 0...NumOptionalParms-1.                                         

  const char              * getOptionalString        (Lng32)  const;


  const NAWchar           * getOptionalWString       (Lng32)  const;


  Lng32                            getOptionalInteger (Lng32)  const;


  void               setOptionalString (Lng32, const char* const,
		         CharInfo::CharSet = CharInfo::ISO88591
			               );

  void               setOptionalWString (
			Lng32, const NAWchar* const
					);


  CharInfo::CharSet  getOptionalStringCharSet(Lng32) const;



  void                            setOptionalInteger (Lng32,
						      Lng32);

  // There are three methods each that must be overridden
  // in order to provide for packing and unpacking of this
  // class in order to support IPC.


  IpcMessageObjSize    packedLength        ();

  IpcMessageObjSize    packedLength32      ();

  IpcMessageObjSize    packObjIntoMessage  (IpcMessageBufferPtr buffer);

  IpcMessageObjSize    packObjIntoMessage32 (IpcMessageBufferPtr buffer);

  IpcMessageObjSize    packObjIntoMessage  (IpcMessageBufferPtr buffer,
                                         NABoolean swapBytes);

  IpcMessageObjSize    packObjIntoMessage32 (IpcMessageBufferPtr buffer,
                                         NABoolean swapBytes);


  void                 unpackObj           (IpcMessageObjType objType,
					    IpcMessageObjVersion objVersion,
					    NABoolean sameEndianness,
					    IpcMessageObjSize objSize,
					    IpcConstMessageBufferPtr buffer);

  void                 unpackObj32         (IpcMessageObjType objType,
					    IpcMessageObjVersion objVersion,
					    NABoolean sameEndianness,
					    IpcMessageObjSize objSize,
					    IpcConstMessageBufferPtr buffer);

  NABoolean            checkObj       (IpcMessageObjType objType,
                                       IpcMessageObjVersion objVersion,
                                       NABoolean sameEndianness,
                                       IpcMessageObjSize objSize,
                                       IpcConstMessageBufferPtr buffer) const;

protected:
  // In support of allocation, creation, and destruction:


                              ComCondition     (CollHeap *);

inline  static  void         *operator new     (size_t,CollHeap* = NULL);

inline  static  void          operator delete  (void*);

                void          destroyMe        ();

  // For the heap management there is a private data member which is
  // a CollHeap*, established by the constructor.  All storage owned
  // by a ComCondition object is allocated on the heap specified
  // by its CollHeap* member.

  CollHeap           *collHeapPtr_;

private:
  Lng32                       conditionNumber_;
  Lng32                       usageMap_;
  Lng32                       theSQLCODE_;
  char                      *serverName_;
  char                      *connectionName_;
  char                      *constraintCatalog_;
  char                      *constraintSchema_;
  char                      *constraintName_;
  char                      *triggerCatalog_;
  char                      *triggerSchema_;
  char                      *triggerName_;
  char                      *catalogName_;
  char                      *schemaName_;
  char                      *tableName_;
  char			    *customSQLState_;
  char                      *columnName_;
  char                      *sqlID_;
  Lng32                       rowNumber_;
  Lng32                       nskCode_;
  Lng32                       numStringParamsUsed_;
  Lng32                       numIntParamsUsed_;
  NABoolean                  flagsTBS_; // flags (to be sent in messages)
  NABoolean                  isLocked_;
  NAWchar		    *messageText_;
  Lng32                       messageLen_;

  // The data representing these members is a pair of private arrays.
  // The constructors, generally, NULL the values of the
  // char * arrays, and even zero the values of the long arrays.

  void			 *optionalString_[NumOptionalParms];
  enum CharInfo::CharSet optionalStringCharSet_[NumOptionalParms];
  Lng32                   optionalInteger_[NumOptionalParms];

  Lng32                     emsEventVisits_;

  // enum CharInfo::CharSet   iso88591MappingCharSet_;

  // reserve space for adding new members or extending the size
  // of existing members
  char                       fillers_[64];
  // char                       fillers_[60];


  // This function is very helpful in writing each of the char* "set"
  // member functions.  The first argument should be one of the char*
  // data members.  The second argument should be one of the char*
  // arguments to a set-char* function.  This function takes care of
  // copying the given buffer's contents to the member buffer, using
  // the proper heap to do so.
  //
  // Apparently we do need to overload this for WINNT.
void assignStringMember(char    *& memberBuff, const char    *const src);

// UR2
  void assignStringMember(NAWchar *& memberBuff, const NAWchar *const src);

  // can't touch these:

  Int32             operator==    (const ComCondition&);

                  ComCondition  (const ComCondition&);
};

///////////////////////////////////////////////////////////////
// These are the inline functions of the ComCondition class. //
///////////////////////////////////////////////////////////////


inline
void *ComCondition::operator new(size_t theSize, CollHeap* heapPtr)
{
   if (heapPtr != NULL)
     return (ComCondition*) heapPtr->allocateMemory(theSize);
   else
     return (ComCondition*) ::new char[theSize];  // note: vector
}

inline
void ComCondition::operator delete(void *ptr)
{
   if ( ((ComCondition*) ptr)->collHeapPtr_ == NULL )
     // Can not delete a (void *) pointer, first cast to a char *.
     ::delete [] (char *)ptr;
}



inline
ComCondition *ComCondition::allocate(CollHeap* heapPtr)
{
   if (heapPtr == NULL)
      return  new ComCondition();
   else
      return  new(heapPtr) ComCondition(heapPtr);
}


inline
void ComCondition::deAllocate()
{
   if (collHeapPtr_ == NULL)
      delete this;
   else {
     // save CollHeapPtr_. destroyMe() sets it to NULL
     // This is just a quick solution to get going. Better solution:
     // get rid of allocate(), deAllocate() and derive ComCondition from
     // NABasicObject
     CollHeap * p = collHeapPtr_;
     destroyMe();
     p->deallocateMemory(this);
   };
}


inline
ComDiagBigInt ComCondition::getConditionNumber() const
{
   return conditionNumber_;
}

inline
const char* ComCondition::getServerName() const
{
   return serverName_;
}

inline
const char* ComCondition::getConnectionName() const
{
   return connectionName_;
}

inline
const char* ComCondition::getConstraintCatalog() const
{
   return constraintCatalog_;
}

inline
const char* ComCondition::getConstraintSchema() const
{
   return constraintSchema_;
}

inline
const char* ComCondition::getConstraintName() const
{
   return constraintName_;
}

inline
const char* ComCondition::getTriggerCatalog() const
{
   return triggerCatalog_;
}

inline
const char         * ComCondition::getTriggerSchema() const
{
   return triggerSchema_;
}

inline
const char* ComCondition::getTriggerName() const
{
   return triggerName_;
}

inline
const char* ComCondition::getCatalogName() const
{
   return catalogName_;
}

inline
const char* ComCondition::getSchemaName() const
{
   return schemaName_;
}

inline
const char* ComCondition::getTableName() const
{
   return tableName_;
}

inline
const char* ComCondition::getColumnName() const
{
   return columnName_;
}

inline
const char* ComCondition::getSqlID() const
{
   return sqlID_;
}

inline
Lng32 ComCondition::getRowNumber() const
{
   return rowNumber_;
}

inline
Lng32 ComCondition::getNskCode() const
{
   return nskCode_;
}

inline
Lng32 ComCondition::getSQLCODE() const
{
   return theSQLCODE_;
}

inline
Lng32 ComCondition::getEMSEventVisits() const
{
   return emsEventVisits_;
}

inline 
void ComCondition::incrEMSEventVisits()
{
   emsEventVisits_++;
}

inline
NABoolean ComCondition::isLocked () const
{
  return isLocked_;
}

// -----------------------------------------------------------------------
// Class ComDiagsArea
// -----------------------------------------------------------------------

class ComDiagsArea : public IpcMessageObj {
public:
  // For the ``SQL function'' setting and getting operations, we declare
  // an enumeration which represents a SQL function.  This is entirely
  // based on table 22 of the ANSI standard (subclause 18.1).
  //
  // WARNING: If you modify this list of enumerations be careful that
  //          you also update the array, functionNames[] in ComDiags.C.

  enum    FunctionEnum   {
    NULL_FUNCTION,  // means "no function"
    ALLOCATE_CURSOR,
    ALLOCATE_DESCRIPTOR,
    ALTER_DOMAIN,
    ALTER_TABLE,
    CREATE_ASSERTION,
    CREATE_CHARACTER_SET,
    CLOSE_CURSOR,
    CREATE_COLLATION,
    COMMIT_WORK,
    CONNECT,
    DEALLOCATE_DESCRIPTOR,
    DEALLOCATE_PREPARE,
    DELETE_CURSOR,
    DELETE_WHERE,
    DESCRIBE,
    SELECT,
    DISCONNECT,
    CREATE_DOMAIN,
    DROP_ASSERTION,
    DROP_CHARACTER_SET,
    DROP_COLLATION,
    DROP_DOMAIN,
    DROP_SCHEMA,
    DROP_TABLE,
    DROP_TRANSLATION,
    DROP_VIEW,
    DYNAMIC_CLOSE,
    DYNAMIC_DELETE_CURSOR,
    DYNAMIC_FETCH,
    DYNAMIC_OPEN,
    DYNAMIC_UPDATE_CURSOR,
    EXECUTE_IMMEDIATE,
    EXECUTE,
    FETCH,
    GET_DESCRIPTOR,
    GET_DIAGNOSTICS,
    GRANT,
    INSERT,
    OPEN,
    PREPARE,
    REVOKE,
    ROLLBACK_WORK,
    CREATE_SCHEMA,
    SET_CATALOG,
    SET_CONNECTION,
    SET_CONSTRAINT,
    SET_DESCRIPTOR,
    SET_TIME_ZONE,
    SET_NAMES,
    SET_SCHEMA,
    SET_TRANSACTION,
    SET_SESSION_AUTHORIZATION,
    CREATE_TABLE,
    CREATE_TRANSLATION,
    UPDATE_CURSOR,
    UPDATE_WHERE,
    CREATE_VIEW,
    MAX_FUNCTION_ENUM
  };

   enum {
    INVALID_MARK_VALUE = -1
  };

  ComDiagsArea           ();

  // The destructor is not declared virtual since we
  // absolutely do not expect ComDiagsArea to be a base class.
  //
  // It is the job of the destructor to free all storage held by
  // this ComDiagsArea object and to free that storage using
  // the heap specified at the time this object was
  // constructed.
  //
  // The destructor **does not free** the heap referenced by collHeapPtr_.

  ~ComDiagsArea                ();

  // If you want to dynamically allocate a ComDiagsArea, you can't
  // use "new."    Instead, you must use "allocate" and later "deAllocate"
  // to free this object.  This is so that we can allow the option of
  // this class' user supplying a CollHeap to manage storage.

  static  ComDiagsArea   *allocate      (CollHeap*);
  static  ComDiagsArea   *allocate      ();
          void            deAllocate    ();

  // Copy a ComDiagsArea object.  The copy is constructed in the same
  // heap in which the object being copied resides.  A deep copy is
  // performed, meaning that not only is the top-level object copied,
  // but also all nested objects and attributes (except for the embedded
  // CollHeap).  The reference count of the copy is initialized to one.
  ComDiagsArea* copy();


  // These members provide set and get operations on the data
  // of a ComDiagsArea that is defined in ANSI table 21, in subclause
  // 18.1.  See also, ``Creating Errors Korrectly.''

  Lng32	              getNumber           () const;
  Lng32		      getNumber           (DgSqlCode::ErrorOrWarning) const;
  NABoolean           areMore             () const;
  NABoolean           canAcceptMoreErrors () const;
  Int64               getRowCount         () const;
  void                setRowCount         (Int64);
  void                addRowCount         (Int64);
  ComDiagBigInt       getAvgStreamWaitTime      () const;
  void                setAvgStreamWaitTime      (ComDiagBigInt);
  double              getCost             () const;
  void                setCost             (double);
  Lng32                getLengthLimit      () const;
  void                setLengthLimit      (Lng32);

// This method will set the sqlID attribute of every error condition
// and warning in the diags area that isn't already set.
  void                setAllSqlID         (char *);

// This method will set the RowNumber attribute of every error condition
// in the diags area when it called.
  void                setAllRowNumber      (Lng32, DgSqlCode::ErrorOrWarning errorOrWarn = DgSqlCode::ERROR_);

// This method will check the RowNumber attribute of every error condition
// and return the value of the smallest rownumber that is greater than or equal to indexValue.
// In other words it returns the value of the first rowset index that has raised an error
// that is greater than or equal to indexValue. 
// If none is found, INVALID_ROWNUMBER will be returned.
  Lng32                getNextRowNumber      (Lng32 indexValue) const;

// this method returns TRUE is rowsetRowCountArray_ is not NULL.
  NABoolean	      hasValidRowsetRowCountArray () const;

// this method returns the number of entries in the rowsetRowCountArray_.
// should not exceed run-time size of input rowset. Currently we do not 
// support rowset sizes that do not fit into a long datatype.
  Lng32	      numEntriesInRowsetRowCountArray () const;

// this method inserts "value" into the specified index of the rowsetRowCountArray_
// It also handles allocation of the array if this is the first element being inserted
// into the array.
  void	      insertIntoRowsetRowCountArray (Lng32 index, Int64 value, 
					    Lng32 arraySize, CollHeap* heapPtr) ;

// this method returns the value in the specified index of rowsetRowCountArray_.
// returns -1 if the index specified is unused.
  Int64	      getValueFromRowsetRowCountArray (Lng32 index) const;


  // The set and get functions for setting and getting
  // the ``SQL function'' of a ComDiagsArea use the FunctionEnum
  // type.  You can also get a char* which is the name of the
  // function.

  void                setFunction         (FunctionEnum);
  const char *        getFunctionName     () const;
  FunctionEnum        getFunction         () const;

  // To help make it easier to get ComConditions and their data into
  // a diags area, we define these friend operator<< functions which
  // allow "streamslike" syntax: diags << DgSqlCode(13030); for example.

  // This function creates a new ComCondition and inserts it into
  // the proper internal list (warning or error) depending on the
  // given code (0 causes an assertion failure).
  //
  // It is an error to invoke this function while there is a "new"
  // ComCondition in this ComDiagsArea (see makeNewCondition, etc., below)

  friend ComDiagsArea& operator<<(ComDiagsArea&, const DgBase&);

  // makeNewCondition -- creates a new ComCondition object
  // using the heap which the ComDiagsArea knows about (the caller
  // doesn't necessarily know about this heap).  A pointer to the
  // ComCondition object is returned.  The user may put data
  // into the ComCondition as they wish.  It's bad if they delete it.
  //
  // It is an assertion failure to say makeNewCondition when
  // you have not ``accepted'' or ``discarded'' the current new
  // ComCondition object.

  ComCondition     *      makeNewCondition           ();

  // mainSQLCODE() returns 0, if there are no ComConditions in
  // this ComDiagsArea, and it returns the SQLCODE of the highest
  // priority ComCondition otherwise.

  Lng32                    mainSQLCODE                () const;

  // Returnes the SQLSTATE value of the last SIGNAL statement.
  // Assumes the SIGNAL condition is the highest priority error.


  const char 			 *getSignalSQLSTATE			 () const;
  
  // Removes all ComConditions from this object.  One is on their own
  // if they mark,clear,acceptNewCondition,rewind.

  void                    clear                      ();
  void                    clearConditionsOnly        ();
  void			  clearErrorConditionsOnly   ();
  // the next 3 methods are called to set a warning, an EOD indication(100),
  // or an error. Useful while debugging to find out when/where an 
  // error/warning/EOD is being set.
  void insertNewWarning();

  void insertNewEODWarning();

  void insertNewError();

  // acceptNewCondition -- inserts the new ComCondition object into the
  // ComDiagsArea.
  // No argument need be passed in telling the ComDiagsArea which
  // ComCondition object to insert because it keeps track of this
  // object starting with the makeNewCondition call.
  // This method  makes the
  // ComDiagsArea ready to create a new ComCondition object.
  //
  // An assertion failure will occur if the the SQLCODE of the
  // new condition is zero.
  //
  // An assertion failure will occur if this function is called at a
  // time when there is current new condition.

  void                    acceptNewCondition         ();


  // discardNewCondition --  is provided
  // just in case the user of this class has created
  // a new ComCondition, maybe put some data into it, but then
  // decided they don't want to insert it into the  ComDiagsArea object.
  // This method destroys that ComCondition object and makes the
  // ComDiagsArea ready to create a new ComCondition object.

  void                    discardNewCondition        ();

  // In order to access an element of the sequence of ComCondition
  // objects, we provide operator[].  A valid index
  // is in the range 1...getNumber().  Passing an invalid
  // index will result in an assertion failure.

  ComCondition       &operator[]    (Lng32)  const;


  // getErrorEntry and getWarningEntry - same function as the above []
  // operator, but this method only accesses ComCondition objects in the
  // specified list (error or warning).
  // index ranges from 1..getNumber(DgSqlCode::WARNING_ or ERROR_)
  ComCondition* getWarningEntry (Lng32);
  ComCondition* getErrorEntry   (Lng32);

  ComCondition* findCondition   (Lng32 sqlCode, Lng32 *entryNumber=NULL); // return NULL if not found


 
  // It is only possible to negate a ComCondition object when it
  // is an element of a ComDiagsArea.
  //
  // There results of negating a ComCondition are:
  //
  // * The ComCondition is the opposite kind of condition that it was
  // before (error to warning, or the other way around).
  //
  // * Internally, the ComDiagsArea swaps which list (errors_ or
  // warnings_ the ComCondition resides in.
  //
  // * The position, and therefore condition number, of the ComCondition
  // object within the sequence of ComConditions within the
  // ComDiagsArea changes (unless it is the sole ComCondition
  // within that  ComDiagsArea).
  //
  // Also see global function NegateAllErrors(), elsewhere in this file.

  void          negateCondition  (CollIndex);

 void negateAllErrors  ()
 {
   while (getNumber(DgSqlCode::ERROR_))
     negateCondition(0);
 }
 
void negateAllWarnings  ()
 {
   while (getNumber(DgSqlCode::WARNING_))
     negateCondition(0);
 }
 
  // This function merges another ComDiagsArea object into this
  // one. An assertion failure occurs if there exists a new-condition
  // (being built by the user) in this ComDiagsArea upon a "mergeAfter."

  void           mergeAfter      (const ComDiagsArea&);

  // These functions allow you to:
  // 1. Save, or ``mark'' the state of this ComDiagsArea.
  // 2. Keep inserting more ComCondition items into this ComDiagsArea.
  // 3. At a later time, retore the sequence of ComCondition objects
  //    in this ComDiagsArea to what it was when the ``mark'' operation
  //    was performed.
  //
  // The other data members, such as the basic info, of this ComDiagsArea
  // remains unaffected by a rewind operation.

  Lng32               mark        () const;
  void               rewind      (Lng32 markValue, NABoolean decId=FALSE);

  // The rewindAndMerge() method works very much like rewinding.
  // The ComDiagsArea* you pass in refers to an object that receives
  // the ComConditions that are being removed from this object as
  // a result of the rewind.  If the given object is the same as *this,
  // then a simple rewind takes place.

  void               rewindAndMergeIfDifferent  (Lng32,ComDiagsArea*);

  // The removeFinalCondition100() method is a special purpose method,
  // used by the SQL CLI, to work around a problem: the EOF condition,
  // SQLCODE 100, is added to the context diags area always (?) but 
  // should not be returned to CLI client after a SELECT INTO or an I/U/D.

  void removeFinalCondition100();

 // The removeLastErrorCondition() method is a special purpose method,
  // used by non-atomic statements, when the NOT_ATOMIC_FAILURE_LIMIT is set to a finite value.
  // If the number of errors raised exceeds this limit then the last condition is removed
  // to make space for error -30031.

  void removeLastErrorCondition();
  void removeLastNonFatalCondition();
  Lng32 markDupNFConditions();
  // the deleteWarning and deleteError methods delete a warning/error from
  // the warnings_/errors_ list.
  // entryNumber is the index to the warnings_/errors_ list
  // entryNumber should range from 1 to getNumber(DgSqlCode::WARNING_ or
  // ERROR_)
  void deleteWarning(Lng32 entryNumber);
  void deleteError(Lng32 entryNumber);

// similar to deleteError except that the Error entry is not
// destroyed (i.e. deallocated) and a pointer to it returned.
// The error entry is removes from the errors_ list.
ComCondition * removeError(Lng32 entryNumber);

  // returns TRUE, if any ComCondition in the diagsArea contains
  // error SQLCode.
  // returns FALSE, otherwise.
  NABoolean contains(Lng32 SQLCode) const;

  // check if a particular error/warning occurred for a particular file.
  // returns TRUE, if diagsArea contains the fileName for error SQLCode.
  // returns FALSE, otherwise.
  NABoolean containsForFile(Lng32 SQLCode, const char * fileName);

  // returns TRUE, if any ComCondition in the diagsArea contains
  // warning SQLCode. Returns FALSE, otherwise.
  NABoolean containsWarning(Lng32 SQLCode) const;

// Check if warnings_ contains SQLCODE within the range [begin, warnings_.entries()).
// Note beg is 0-based.

  NABoolean containsWarning(CollIndex begin, Lng32 SQLCode) const;

  // returns TRUE, if any ComCondition in the diagsArea contains
  // error SQLCode. Returns FALSE, otherwise.

  NABoolean containsError(Lng32 SQLCode) const;

//returnIndex returns the index number of a given SQLCODE in this diagsarea
//If the given SQLCODE is not found in the diagsarea then NULL_COLL_INDEX is returned.
   CollIndex returnIndex(Lng32 SQLCODE) const;


  // Decrement reference count.  Object is deallocated, in the heap in
  // which it resides, when reference count drops to zero.
  IpcMessageRefCount decrRefCount();

  // There are three methods each that must be overridden
  // in order to provide for packing and unpacking of this
  // class in order to support IPC.
  //
  // An assertion failure occurs upon packing or unpacking a ComDiagsArea
  // whose newCondition_ member is not NULL.

  IpcMessageObjSize    packedLength        ();
  IpcMessageObjSize    packedLength32      ();
  IpcMessageObjSize    packObjIntoMessage  (IpcMessageBufferPtr buffer);
  IpcMessageObjSize    packObjIntoMessage (IpcMessageBufferPtr buffer,
                                           NABoolean swapBytes);
  IpcMessageObjSize    packObjIntoMessage32 (IpcMessageBufferPtr buffer);
  IpcMessageObjSize    packObjIntoMessage32 (IpcMessageBufferPtr buffer,
                                             NABoolean swapBytes);
  void                 unpackObj           (IpcMessageObjType objType,
					    IpcMessageObjVersion objVersion,
					    NABoolean sameEndianness,
					    IpcMessageObjSize objSize,
					    IpcConstMessageBufferPtr buffer);
  void                 unpackObj32         (IpcMessageObjType objType,
					    IpcMessageObjVersion objVersion,
					    NABoolean sameEndianness,
					    IpcMessageObjSize objSize,
					    IpcConstMessageBufferPtr buffer);
  NABoolean            checkObj       (IpcMessageObjType objType,
                                       IpcMessageObjVersion objVersion,
                                       NABoolean sameEndianness,
                                       IpcMessageObjSize objSize,
                                       IpcConstMessageBufferPtr buffer) const;

  NABoolean getRollbackTransaction() const;

  void setRollbackTransaction(short value);

  NABoolean getNoRollbackTransaction() const;

  void setNoRollbackTransaction(NABoolean value);

  NABoolean getNonFatalErrorSeen() const;

  void setNonFatalErrorSeen(NABoolean value);

  NABoolean containsRowCountFromEID() const;

  void setContainsRowCountFromEID(NABoolean value);

  NABoolean getNonFatalErrorIndexToBeSet() const;

  void setNonFatalErrorIndexToBeSet(NABoolean value);

private:

  // We want to be able to declare a linked list of pointers to
  // ``a ComCondition with an long id attribute.''  We need
  // this long attribute so that inside a ComDiagsArea we
  // can track the chronological arrival of ComConditions.  This
  // makes it possible to define and implement a number of operations
  // including merging, insertion, marking, and rewinding.
  //
  // So the first step is to declare a class private to ComDiagsArea
  // that provides this new abstraction:

  class DiagsCondition : public ComCondition {
  public:
    Lng32             getDiagsId           () const;
    void             setDiagsId           (Lng32);

    // We want a DiagsCondition object to behave pretty much
    // like a ComCondition object, so the constructor(s), destructor,
    // comparison, and assignment operators will all be declared following the
    // pattern of the ComCondition class:

     DiagsCondition             ();
    ~DiagsCondition             ();

    static  DiagsCondition *allocate      (CollHeap* = NULL);
            void            deAllocate    ();

    // Copy a DiagsCondition object.  The copy is constructed in the same
    // heap in which the object being copied resides.  A deep copy is
    // performed, meaning that not only is the top-level object copied,
    // but all attributes as well (except for the CollHeap).
    DiagsCondition* copy();

     // There are three methods each that must be overridden
     // in order to provide for packing and unpacking of this
     // class in order to support IPC.

     IpcMessageObjSize    packedLength      ();
     IpcMessageObjSize    packedLength32    ();
     IpcMessageObjSize    packObjIntoMessage(IpcMessageBufferPtr buffer);
     IpcMessageObjSize    packObjIntoMessage(IpcMessageBufferPtr buffer,
                                          NABoolean swapBytes);
     IpcMessageObjSize    packObjIntoMessage32(IpcMessageBufferPtr buffer);
     IpcMessageObjSize    packObjIntoMessage32(IpcMessageBufferPtr buffer,
                                          NABoolean swapBytes);
     void                 unpackObj         (IpcMessageObjType objType,
  				            IpcMessageObjVersion objVersion,
  					    NABoolean sameEndianness,
					    IpcMessageObjSize objSize,
     				            IpcConstMessageBufferPtr buffer);
     void                 unpackObj32       (IpcMessageObjType objType,
  				            IpcMessageObjVersion objVersion,
  					    NABoolean sameEndianness,
					    IpcMessageObjSize objSize,
     				            IpcConstMessageBufferPtr buffer);
     NABoolean            checkObj     (IpcMessageObjType objType,
                                        IpcMessageObjVersion objVersion,
                                        NABoolean sameEndianness,
                                        IpcMessageObjSize objSize,
                                        IpcConstMessageBufferPtr buffer) const;

    DiagsCondition &operator=       (const DiagsCondition&);

  private:
    Lng32         diagsId_;

  // In support of allocation, creation, and destruction:

                              DiagsCondition   (CollHeap *);
   inline  static  void      *operator new     (size_t,CollHeap* = NULL);

   // delete inherited from ComCondition
                void          destroyMe        ();

// Added to allow NCHAR error messages.
class DgWString0 : public DgBase
{
public:
  DgWString0  (const NAWchar* const x) : theWCharStr_(x) {};
  const NAWchar*   getWCharStr () const {return theWCharStr_; };
  DGTYPE           getTypeName () const { return DGWSTRING0; };
private:
  const NAWchar* const theWCharStr_;
};


class DgWString1 : public DgBase
{
public:
  DgWString1  (const NAWchar* const x) : theWCharStr_(x) {};
  const NAWchar*    getWCharStr () const {return theWCharStr_; };
  DGTYPE            getTypeName () const { return DGWSTRING1; };
private:
  const NAWchar* const theWCharStr_;
};


class DgWString2 : public DgBase
{
public:
  DgWString2  (const NAWchar* const x) : theWCharStr_(x) {};
  const NAWchar*  getWCharStr () const {return theWCharStr_; };
  DGTYPE          getTypeName () const  { return DGWSTRING2; };
private:
  const NAWchar* const theWCharStr_;
};


class DgWString3 : public DgBase
{
public:
  DgWString3  (const NAWchar* const x) : theWCharStr_(x) {};
  const NAWchar*  getWCharStr () const {return theWCharStr_; };
  DGTYPE          getTypeName () const { return DGWSTRING3; };
private:
  const NAWchar* const theWCharStr_;
};


class DgWString4 : public DgBase
{
public:
  DgWString4  (const NAWchar* const x) : theWCharStr_(x) {};
  const NAWchar*   getWCharStr  () const {return theWCharStr_; };
  DGTYPE           getTypeName() const { return DGWSTRING4; };
private:
  const NAWchar* const theWCharStr_;
};


    // can't touch these:
    Int32             operator==      (const DiagsCondition&);

    DiagsCondition  (const DiagsCondition&);
  };

public:

  // In support of allocation, creation, and destruction:

                              ComDiagsArea     (CollHeap *);
private:

inline  static  void         *operator new     (size_t,CollHeap*);
inline  static  void         *operator new     (size_t);

inline  static  void          operator delete  (void*);


                void          destroyMe        ();

  // This function is called when accepting, merging, and setLengthLimit
  // to make sure that the number of ComCondition
  // objects currently in this ComDiagsArea is less than or equal
  // to the value in lengthLimit_.  It may be that one or more ComCondition
  // objects need to be removed (deleted, freed, etc.) from this object.
  // In that case, those ComConditions with the lowest priority go first
  // (warnings before errors, reverse chronologically).

   void                       enforceLengthLimit ();

  // can't touch these:

  ComDiagsArea   &operator=     (const ComDiagsArea&);

  Int32             operator==    (const ComDiagsArea&);

  ComDiagsArea  (const ComDiagsArea&);


  // For the heap management there is a private data member which is
  // a CollHeap*, established by the constructor.  All storage owned
  // by a ComCondition object is allocated on the heap specified
  // by its CollHeap* member.

  CollHeap           *collHeapPtr_;


  // Now we can say LIST(DiagsCondition*) to give a type to
  // two linked lists: one for the error conditions, and the other
  // for the warning conditions.  These are ordered sequences,
  // and the ComDiagsArea will not only keep its conditions
  // carefully partitioned into these two lists, but also keep
  // the ordering of each list in terms of chronological arrival.
  // This meets the ANSI requirements nicely --- see ``Creating Errors
  // Correctly.''

  LIST(DiagsCondition*)         errors_, warnings_;

  // In order to keep track of the ``new condition'' there is a
  // private member, a pointer: when NULL there is no current new
  // ComCondition, and when non-NULL this pointer refers
  // to a DiagsCond object which represents the current new condition.

  DiagsCondition                *newCondition_;

  // TRUE if and only if there were too many elements in this
  // diags area upon some event including:
  //    * setting the lengthLimit lower than the current length.
  //    * accepting a condition while this object is currently full.
  //    * merging with another ComDiagsArea and exceeding the length limit.
  // ComCondition into this diags area when it was not in a
  // state to accept any further entries.
  // can also be used to detect if the number of error conditions
  // exceeds the lengthLimit

  Int32                       areMore_;

  // This data members indicates the count of the maximum number of
  // ComCondition objects that this ComDiagsArea may hold.  Zero is
  // the lowest value it may hold.

  Int32                       lengthLimit_;

  // Set from client code, this ``property'' of this class tells
  // how many rows are associated with its data.

  Int64                       rowCount_;


  // The associated SQL "function."

  Int32                       theSQLFunction_;

  // maxDiagsId_ helps keep track of the chronological arrival
  // of ComCondition objects into this ComDiagsArea object.

  Int32                maxDiagsId_;    

  // avgStreamWaitTime_ is a SQL/MX extension used for stream selects,
  // to help the client application balance load.  Its units are centiseconds.

  ComDiagBigInt        avgStreamWaitTime_;

  // Cost is a SQL/MP extension info item and tells
  // the cost associated with this query. Set in diags area
  // after the query is prepared.
  double                      cost_;

  // Various flags.
  enum DiagsAreaFlags
  {
    // this flag, if set, indicates that the transaction has to be
    // aborted before returning error to application. This flag is
    // set if an error occured while doing an operation that would
    // leave the database in an unstable state (like, at DDL time).
    // Used when rollbackOnError is not set in exe root tdb.
    // Executor, on seeing this flag, rollbacks that transaction.
    ROLLBACK_TRANSACTION = 0x0001,

    // this flag, if set, indicates that the transaction does not
    // have to be aborted in case of an error. 
    // Used when the rollbackOnError is not set in executor root tdb.
    NO_ROLLBACK_TRANSACTION = 0x0002,

    // used to raise warning 30022 and set output of mainSQLCODE() to 30022
    // used only for non-atomic inserts when some nonfatal errors have been seen,
    // but some rows have also been inserted.
    NONFATAL_ERROR_SEEN = 0x0004,

    // indicates that diagsarea contains rowcount from eid
    // which should be used instead of counting matchNo's.
    // Currently used for vsbb inserts when some duplicate rows
    // are ignored.
    CONTAINS_ROWCOUNT_FROM_EID = 0x0008,
    // if TRUE - indicates a non fatal error whose row index needs to  be set
    // else indicates a nonfatal error row whose  index doesn't need to be set
    NONFATAL_ERROR_ROWINDEX_TOBE_SET = 0x0010
  };

  UInt32                      flags_;

  // contains the number of rows_affected by each element of a rowset
  // search-condition for rowset updates and deletes.
  ARRAY(Int64)                *rowsetRowCountArray_;


  // reserve space for adding new members or extending the size
  // of existing members
  char                       fillers_[60];

};	// ComDiagsArea

// Change all errors to warnings.
//
// ##This is a global inline func only because I am too lazy to make it a
// ##public ComDiagsArea::negateAllErrors() method, with the following
// ##(or equivalent) in the .cpp file, and recompile and rebuild everything.
inline
void          NegateAllErrors  (ComDiagsArea *a)
{
  a->negateAllErrors();
}

///////////////////////////////////////////////////////////////
// These are the inline functions of the ComDiagsArea class. //
///////////////////////////////////////////////////////////////

inline
void *ComDiagsArea::operator new(size_t theSize)
{
    return (ComDiagsArea*) ::new char[theSize];  // note: vector
}

inline
void *ComDiagsArea::operator new(size_t theSize, CollHeap* heapPtr)
{
  if (heapPtr != NULL)
    return (ComDiagsArea*) heapPtr->allocateMemory(theSize);
  else
    return (ComDiagsArea*) ::new char[theSize];  // note: vector

}

inline
void ComDiagsArea::operator delete(void *ptr)
{
  if ( ((ComDiagsArea*) ptr)->collHeapPtr_ == NULL)
     // Can not delete a (void *) pointer, first cast to a char *.
     ::delete [] (char *)ptr;
}



inline
ComDiagsArea  *ComDiagsArea::allocate(CollHeap* heapPtr)
{
   return  new(heapPtr) ComDiagsArea(heapPtr);
}

inline
ComDiagsArea  *ComDiagsArea::allocate()
{
   return  new ComDiagsArea();
}


inline
void ComDiagsArea::deAllocate()
{
   if (collHeapPtr_ == NULL)
      delete this;
   else {
     // save collHeapPtr, because destroyMe() sets it to NULL
     // Better solution: derive ComDiagsArea from NABasicObject and get
     // rid of allocate() / deAllocate()
     CollHeap * p = collHeapPtr_;
     destroyMe();
     p->deallocateMemory(this);
   };
}


inline
void *ComDiagsArea::DiagsCondition::operator new(size_t theSize, CollHeap* heapPtr)
{
  if (heapPtr != NULL)
    return (ComDiagsArea*) heapPtr->allocateMemory(theSize);
  else
    return (ComDiagsArea*) ::new char[theSize];  // note: vector
}


inline
ComDiagsArea::DiagsCondition*
   ComDiagsArea::DiagsCondition::allocate(CollHeap* heapPtr)
{
   if (heapPtr == NULL)
      return  new DiagsCondition();
   else
      return  new(heapPtr) DiagsCondition(heapPtr);
}


inline
void ComDiagsArea::DiagsCondition::deAllocate()
{
   if (collHeapPtr_ == NULL)
      delete this;
   else {
     // detroyMe() calls the destructor which resets collHeapPtr
     // save the pointer
     CollHeap * p = collHeapPtr_;
     destroyMe();
     p->deallocateMemory(this);
   };
}

inline
void ComDiagsArea::setLengthLimit(Lng32 newLimit)
{
   lengthLimit_ = newLimit;
   enforceLengthLimit();
}

inline
Lng32 ComDiagsArea::getLengthLimit () const
{
   return lengthLimit_;
}

// We create an operator<< for outputting a ComDiagsArea
// and giving a summary of its contents.
//
// This is commented out in this header since it references ostream.
// If you want to use this operator (for debugging purposes), then
// just copy this commented out stuff into the source file where you
// want to code calls to this operator.
//
// ostream &operator<<(ostream &dest, const ComDiagsArea& da)


// inline methods to manipulate flags_.
inline
  NABoolean ComDiagsArea::getRollbackTransaction() const
  {
    return ((flags_ & ROLLBACK_TRANSACTION) != 0);
  }

inline
  void ComDiagsArea::setRollbackTransaction(short value)
  {
    if (value)
      flags_ |= ROLLBACK_TRANSACTION;    // set the bit
    else
      flags_ &= ~ROLLBACK_TRANSACTION;   // reset the bit
  }
// inline methods to manipulate flags_.
inline
  NABoolean ComDiagsArea::getNoRollbackTransaction() const
  {
    return ((flags_ & NO_ROLLBACK_TRANSACTION) != 0);
  }

inline
  void ComDiagsArea::setNoRollbackTransaction(NABoolean value)
  {
    if (value)
      flags_ |= NO_ROLLBACK_TRANSACTION;    // set the bit
    else
      flags_ &= ~NO_ROLLBACK_TRANSACTION;   // reset the bit
  }
inline
  NABoolean ComDiagsArea::getNonFatalErrorSeen() const
  {
    return ((flags_ & NONFATAL_ERROR_SEEN) != 0);
  }

inline
  void ComDiagsArea::setNonFatalErrorSeen(NABoolean value)
  {
    if (value)
      flags_ |= NONFATAL_ERROR_SEEN;    // set the bit
    else
      flags_ &= ~NONFATAL_ERROR_SEEN;   // reset the bit
  }

inline
  NABoolean ComDiagsArea::containsRowCountFromEID() const
  {
    return ((flags_ & CONTAINS_ROWCOUNT_FROM_EID) != 0);
  }

inline
  void ComDiagsArea::setContainsRowCountFromEID(NABoolean value)
  {
    if (value)
      flags_ |= CONTAINS_ROWCOUNT_FROM_EID;  // set the bit
    else
      flags_ &= ~CONTAINS_ROWCOUNT_FROM_EID;   // reset the bit
  }
inline
  NABoolean ComDiagsArea::getNonFatalErrorIndexToBeSet() const
  {
    return ((flags_ & NONFATAL_ERROR_ROWINDEX_TOBE_SET) != 0);
  }

inline
  void ComDiagsArea::setNonFatalErrorIndexToBeSet(NABoolean value)
  {
    if (value)
      flags_ |= NONFATAL_ERROR_ROWINDEX_TOBE_SET;    // set the bit
    else
      flags_ &= ~NONFATAL_ERROR_ROWINDEX_TOBE_SET;   // reset the bit
  }

// -----------------------------------------------------------------------
// Class ComDiagsTranslator: An abstract base class for classes that need 
//                           to translate individual conditions in a diags
//                           area
// -----------------------------------------------------------------------

class ComDiagsTranslator
{
public:

  void translateDiagsArea ( ComDiagsArea &diags, 
                            const NABoolean twoPass = FALSE);

  virtual const NABoolean translateCondition 
                          ( ComDiagsArea &diags,
                            const ComCondition &cond) = 0;

  // don't implement - we want derived classes that use analyzeCondition
  // to provide their own method
  virtual void analyzeCondition (const ComCondition &cond);

protected:
  
  ComDiagsTranslator (void) : firstError_ (FALSE) {};

  NABoolean firstError_;

private:
  // Virtual methods, to be called before the first condition is processed.
  // Derived classes that want to know about this can redefine.
  virtual void beforeAnalyze (void);
  virtual void beforeTranslate (void);

  // Virtual method, to be called after the last condition is processed.
  // Derived classes that want to know about this can redefine.
  // Note that there is no "afterAnalyze" since that would be identical to
  // "beforeTranslate".
  virtual void afterTranslate (void);
};


#endif
