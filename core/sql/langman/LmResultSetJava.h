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
******************************************************************************
*
* File:         LmResultSetJava.h
* Description:  Container for Java specific result set data.
*
* Created:      09/07/2005
* Language:     C++
*
******************************************************************************
*/

#ifndef LMRESULTSETJAVA_H
#define LMRESULTSETJAVA_H

#include "ComSmallDefs.h"
#include "LmCommon.h"
#include "LmResultSet.h"
#include "LmLangManagerJava.h"
#include "LmRoutineJava.h"
#include "jni.h"
#include "LmConnection.h"

//////////////////////////////////////////////////////////////////////
//
// LmResultSetJava
//
//////////////////////////////////////////////////////////////////////

class LmResultSetJava : public LmResultSet
{
friend class LmRoutineJava;

public:

  enum LmResultSetInfoStatus {
    RS_INFO_OK = 0,
    RS_INFO_CLOSED,
    RS_INFO_LOBCOL,
    RS_INFO_ERROR = -1
  };

  enum LmJDBCConnectionType {
    JDBC_UNKNOWN_CONNECTION = -1,
    JDBC_TYPE4_CONNECTION = 1,
    JDBC_TYPE2_CONNECTION = 2
  };

  // Accessor methods
  LmHandle getResultSet() const { return jdbcRSRef_; }

  char *getProxySyntax() { return proxySyntax_; }

  NABoolean usesT2Connection()
    { return connectionType_ == JDBC_TYPE2_CONNECTION; }

  NABoolean usesT4Connection()
    { return connectionType_ == JDBC_TYPE4_CONNECTION; }

  NABoolean moreSpecialRows()
    { return (usesT2Connection()) ?
               lastBufferedRow_ > currentRowPosition_ : TRUE; }

  NABoolean isCLIStmtClosed()
    { 
       return CLIStmtClosed_; 
    }
  Lng32 fetchSpecialRows(void *dataPtr,
		        LmParameter *colDesc,
                        ComUInt32 numCols,
		        ComDiagsArea &da,
                        ComDiagsArea *rda);

private:
  NABoolean isScrollable() const
  {
    if (cursorType_ == RS_TYPE_SCROLL_INSENSITIVE ||
        cursorType_ == RS_TYPE_SCROLL_SENSITIVE)
      return TRUE;
    else
      return FALSE;
  }

  // The constructor and destructor are defined as private 
  // since the object management of this class can only be 
  // done by the LmRoutineJava class.

  // Constrcutor:
  // Makes a JNI call to LmUtility::getRSInfo()
  // to get result set information for the passed in 
  // java.sql.ResultSet object (parameter rsRef) and
  // initializes the data members accordingly.
  //
  // Parameters description:
  // lm       : Pointer to LmLanguageManagerJava object
  // rsRef    : java.sql.ResultSet object reference
  // paramPos	: The position (1-based) of the Java result set object
  //            in the java method signature. Used only in error reporting.
  // routineName : The name of the routine. Used only in error reporting.
  // status (output) : The status indicating the outcome of calling the
  //                   LmUtility::getRSInfo() java method.
  // da (output)     : Diagnostics area to report errors
  //
  // The following 'status' can be returned:
  // RS_INFO_OK : Result set information was retrieved successfully
  // RS_INFO_CLOSED : The result set object (rsRef) is already closed
  // RS_INFO_ERROR  : There was a problem getting result set information
  LmResultSetJava(LmLanguageManagerJava *lm,
                  LmHandle rsRef,
                  Int32 paramPos,
                  const char *routineName,
                  LmResultSetInfoStatus &status,
                  NAList<LmConnection*> &lmConnList,
                  ComDiagsArea *da);

  // Destructor:
  // Deletes the global references in jdbcRSRef_.
  // Should be called only from within the close() method
  ~LmResultSetJava();

  void initType4ResultSet(Int32 paramPos,
                          const char *routineName,
                          LmResultSetInfoStatus &status,
                          NAList<LmConnection*> &lmConnList,
                          ComDiagsArea *da);

  void initType2ResultSet(Int32 paramPos,
                          const char *routineName,
                          LmResultSetInfoStatus &status,
                          NAList<LmConnection*> &lmConnList,
                          ComDiagsArea *da);

  // Calls the close() method on the Java result set object
  // Decrements the reference count in the associated LmConnection object
  // Calls the class destructor
  void close( ComDiagsArea *da = NULL );

  void insertIntoDiagnostic(ComDiagsArea &da, ComUInt32 col_num);

  LmResult getValueAsJlong(jobject bigdecObj,
                           ComUInt32 columnIndex,
                           ComDiagsArea &da,
			   NABoolean &wasNull,
                           jlong &returnvalue);

  LmLanguageManagerJava *lmj_;

  NAList<LmConnection*> &lmConnList_; // +++ NEED COMMENTS

  LmConnection *lmConn_;  // Manages the java.sql.Connection object
                          // of this result set

  LmJDBCConnectionType connectionType_;  // Type 2 or Type 4 connection

  LmHandle	jdbcRSRef_;   // The Java result set object of this
                              // result set. Contains a  global reference
                              // the object passed into the init() method.

  char *proxySyntax_;     // Proxy syntax (used when T4 conn is used)

  Int32 firstBufferedRow_;  // The row position of the first row that JDBC/MX
                          // has fetched from SQL/MX and that is still
			  // buffered in the JDBC/MX driver. The row
			  // numbers are 1-based.

  Int32 lastBufferedRow_;   // The row position of the last row that JDBC/MX
                          // has fetched from SQL/MX and that is still
			  // buffered in the JDBC/MX driver.

  Int32 currentRowPosition_; // The current row position of this
                           // java.sql.ResultSet instance.

  LmResultSetType cursorType_;  // Indicates whether this is a scrollable
                                // or forward-only cursor etc.

  Int64 rsCounter_;       // An unique value given to each result set
                          // object in JDBC/MX to indicate the order
			  // in which the result set's underlying SQL
			  // statement was executed.

  NABoolean CLIStmtClosed_;  // Indicates whether the CLI statement is
                             // closed or not

  // The following fields are allocated in the constructor and used as
  // parameters to the LmUtility::getRSInfo() method. Destructor
  // deallocates them.

  // ++++++++++++++++++++
  // TBD:
  // Can we declare the below fields as 'static' since they are only used
  // during LmUtility::getRSInfo() JNI call and are not required beyond the
  // constructor. We can just allocate them the first time this class
  // constructor is called and then just re-use them after that. They
  // will need to be deallocated when LM gets destructed.
  //
  // Will this static approach be a concern if LM is made to work in a
  // multi-threaded environment in the future.
  // ++++++++++++++++++++
  jintArray iArray_;
  jlongArray lArray_;
  jobjectArray oArray_;
  jintArray errCode_;
  jobjectArray errDetail_;

};


#endif
