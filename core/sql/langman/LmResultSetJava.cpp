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
* File:         LmResultSetJava.cpp
* Description:  Container for Java specific result set data.
*
* Created:      09/07/2005
* Language:     C++
*
******************************************************************************
*/
#include "Platform.h"
  #include "lmjni.h"
#include "LmResultSetJava.h"
#include "jni.h"
#include "LmJavaExceptionReporter.h"
#include "LmDebug.h"
#include "LmExtFunc.h"
#include "LmJavaType.h"
// #include "ExpError.h"

// Constructor
// Makes a JNI call to LmUtility::getRSInfo()
// to get result set information for the passed in 
// java.sql.ResultSet object (parameter rsRef) and
// initializes the data members accordingly.
LmResultSetJava::LmResultSetJava(LmLanguageManagerJava *lm,
                                 LmHandle rsRef,
                                 Int32 paramPos,
                                 const char *routineName,
                                 LmResultSetInfoStatus &status,
                                 NAList<LmConnection*> &lmConnList,
                                 ComDiagsArea *da)

 : LmResultSet(), lmj_(lm), jdbcRSRef_(NULL), proxySyntax_(NULL),
   firstBufferedRow_(0),
   lastBufferedRow_(0), currentRowPosition_(0),
   cursorType_(RS_TYPE_UNKNOWN), rsCounter_(0),
   iArray_(NULL), lArray_(NULL), oArray_(NULL),
   errCode_(NULL), errDetail_(NULL),
   lmConn_(NULL), lmConnList_(lmConnList),CLIStmtClosed_(0)
{
  JNIEnv *jni = (JNIEnv*)lmj_->jniEnv_;
 
  status = RS_INFO_OK;

  // Find if this Result Set is using  T2 or T4 connection.
  Int32 connType = jni->CallStaticIntMethod((jclass) lmj_->utilityClass_,
		        (jmethodID) lmj_->utilityGetConnTypeId_,
			(jobject) rsRef);

  connectionType_ = (LmJDBCConnectionType) connType;

  // Create a global reference of Result Set object
  jdbcRSRef_ = (jobject)jni->NewGlobalRef((jobject)rsRef);

  if (connectionType_ == JDBC_TYPE4_CONNECTION)
    initType4ResultSet(paramPos, routineName, status, lmConnList, da);
  else if (connectionType_ == JDBC_TYPE2_CONNECTION)
    initType2ResultSet(paramPos, routineName, status, lmConnList, da);
  else
  {
    *da << DgSqlCode(-LME_RS_INFO_ERROR)
        << DgInt0((Lng32) paramPos)
        << DgString0("An unknown Connection type found for ResultSet"); 

    status  = RS_INFO_ERROR;
    return;
  }

}

void LmResultSetJava::initType4ResultSet(Int32 paramPos,
		                         const char *routineName,
					 LmResultSetInfoStatus &status,
					 NAList<LmConnection*> &lmConnList,
					 ComDiagsArea *da)
{ 
  JNIEnv *jni = (JNIEnv*)lmj_->jniEnv_;

  jlongArray lArray = jni->NewLongArray(1);
  jbooleanArray bArray = jni->NewBooleanArray(1);
  jbooleanArray bArray2 = jni->NewBooleanArray(1);
  jintArray iArray = jni->NewIntArray(1);
  jobjectArray oArray1 =
    jni->NewObjectArray(1, (jclass)lmj_->stringClass_, NULL);
  jobjectArray oArray2 =
    jni->NewObjectArray(1, (jclass)lmj_->connClass_, NULL);

  // Call getT4RSInfo() of LmUtility java class
  jni->CallStaticVoidMethod((jclass) lmj_->utilityClass_,
		            (jmethodID) lmj_->utilityGetT4RSInfoId_,
		            (jobject) jdbcRSRef_,
			    lArray,
			    bArray,
			    iArray,
			    bArray2,
			    oArray1,
			    oArray2);

  // Check if any exceptions are thrown by the above JNI call and report it
  if (jni->ExceptionOccurred())
  {
    *da << DgSqlCode(-LME_RS_INFO_ERROR)
        << DgInt0((Lng32) paramPos)
        << DgString0("An unexpected Java exception was encountered when invoking \
                      org.trafodion.sql.udr.LmUtility.getT4RSInfo().");

    lmj_->exceptionReporter_->checkJVMException(da, 0);

    status = RS_INFO_ERROR;
    return;
  }

  // Now process the values returned by getT4RSInfo()

  // Check RS status
  jboolean closeStatus[1];
  jni->GetBooleanArrayRegion(bArray, 0, 1, closeStatus);
  LM_ASSERT(jni->ExceptionOccurred() == NULL);
  if (closeStatus[0])
  {
    status = RS_INFO_CLOSED;
    return;
  }

  // Check for any LOB columns
  jboolean lobData[1];
  jni->GetBooleanArrayRegion(bArray2, 0, 1, lobData);
  LM_ASSERT(jni->ExceptionOccurred() == NULL);
  if (lobData[0])
  {
    *da << DgSqlCode(-LME_LOB_COL_IN_RS_ERROR)
        << DgString0(routineName);

    status = RS_INFO_LOBCOL;
    return;
  }

  // Get RS Counter
  jlong rsCounter[1];
  jni->GetLongArrayRegion(lArray, 0, 1, rsCounter);
  LM_ASSERT(jni->ExceptionOccurred() == NULL);
  rsCounter_ = rsCounter[0];

  // Get RS Type
  jint rsType[1];
  jni->GetIntArrayRegion(iArray, 0, 1, rsType);
  LM_ASSERT(jni->ExceptionOccurred() == NULL);
  cursorType_ = (LmResultSetType) rsType[0];

  // Get proxy syntax
  jstring pSyntax = (jstring) jni->GetObjectArrayElement(oArray1, 0);
  const char *str = jni->GetStringUTFChars(pSyntax, NULL);
  ComUInt32 strSize = str_len(str);
  proxySyntax_ =  new (collHeap()) char[strSize + 1];
  str_cpy(proxySyntax_, str, (Int32)strSize);
  proxySyntax_[strSize] = '\0';
  jni->ReleaseStringUTFChars(pSyntax, str);
  jni->DeleteLocalRef(pSyntax);


  // Get connection reference
  jobject connObj = jni->GetObjectArrayElement(oArray2, 0);
  jobject jdbcConnRef = jni->NewGlobalRef(connObj);
  jni->DeleteLocalRef(connObj);

  // Set RS cursor to to point before first row for Scrollable RS.
  if (isScrollable())
  {
    jni->CallVoidMethod((jobject) jdbcRSRef_,
                        (jmethodID) lmj_->rsBeforeFirstId_);
    currentRowPosition_ = 0;
  }

  // Check if a LmConnection object has already been created for
  // 'jdbcConnRef' if it is then increment the reference count 
  // for that LmConnection object. If not then create a new 
  // LmConnection object add it to the connectionList_.
  ComUInt32 i;
  for( i = 0; i < lmConnList.entries(); i++ )
  {
    lmConn_ = lmConnList[ i ];
    if( jni->IsSameObject( lmConn_->getConnRef(), (jobject)jdbcConnRef ) )
    {
      lmConn_->incrRefCnt();
      jni->DeleteGlobalRef( jdbcConnRef );
      break;
    }
  }

  if( i == lmConnList.entries() ) {
  	//This is not a default connection. For non-default connections we do not perform any transaction 
  	//management so we can set the LmConnection transaction attribute to NO TRANSACTION REQUIRED.
    lmConn_ = new (collHeap()) LmConnection( lmj_,
                                            (jobject)jdbcConnRef,
                                            LmConnection::NON_DEFAULT_CONN, COM_NO_TRANSACTION_REQUIRED);                                             																															
    lmConn_->incrRefCnt();
    lmConnList.insert( lmConn_ );
  }
}

// Exclude the following functions for coverage as Type 2 JDBC is not used any more
void
LmResultSetJava::initType2ResultSet(Int32 paramPos,
                                    const char *routineName,
                                    LmResultSetInfoStatus &status,
                                    NAList<LmConnection*> &lmConnList,
                                    ComDiagsArea *da)
{
  JNIEnv *jni = (JNIEnv*)lmj_->jniEnv_;

  // We will now call the getRSInfo() method in the LmUtility Java class 
  // to get the information for the result set object (jdbcRSRef_).
  // Refer to the LmUtility.java file for details on the parameters 
  // expected by the getRSInfo() method.

  // *** Note: *** 
  // Any change to the parameters in LmUtility::getRSInfo() java method 
  // will impact the code below.

  // The magic number 9 below is to match one of the output int
  // array parameter returned by a call to LmUtility.getRSInfo().
  const Int32 arrSize = 9;

  // Allocate the parameters for the JNI call.
  iArray_ = jni->NewIntArray(arrSize);
  lArray_ = jni->NewLongArray(1);
  oArray_ = jni->NewObjectArray(1, (jclass)lmj_->connClass_, NULL);
  errCode_ = jni->NewIntArray(1);
  errDetail_ = jni->NewObjectArray(1, (jclass)lmj_->stringClass_, NULL);

  if( jni->ExceptionOccurred() || iArray_ == NULL || 
      lArray_ == NULL          || oArray_ == NULL ||
      errCode_ == NULL         || errDetail_ == NULL ) {

    // JVM Out of memory
    *da << DgSqlCode(-LME_JVM_OUT_OF_MEMORY);

    lmj_->exceptionReporter_->checkJVMException(da, 0);
    status = RS_INFO_ERROR;
    return;
  }

  jni->CallStaticVoidMethod((jclass) lmj_->utilityClass_,
                            (jmethodID) lmj_->utilityGetRSInfoId_,
                            (jobject) jdbcRSRef_,
                            iArray_,
                            lArray_,
                            oArray_,
                            errCode_,
                            errDetail_);

  // Check if any exceptions are thrown by the above JNI call and report it
  if (jni->ExceptionOccurred())
  {
    *da << DgSqlCode(-LME_RS_INFO_ERROR)
        << DgInt0((Lng32) paramPos)
        << DgString0("An unexpected Java exception was encountered when invoking \
                      org.trafodion.sql.udr.LmUtility.getRSInfo().");

    lmj_->exceptionReporter_->checkJVMException(da, 0);

    status = RS_INFO_ERROR;
    return;
  }

  // Check the errCode_ parameter to see if any errors have been reported
  // by LmUtility::getRSInfo().
  jint error[1];

  jni->GetIntArrayRegion(errCode_, 0, 1, error);
  LM_ASSERT(jni->ExceptionOccurred() == NULL);

  if( error[0] != 0 ) { // A non-zero value indicates an error
    // Get the value in errDetail_
    jstring jstr = (jstring) jni->GetObjectArrayElement(errDetail_, 0);
    const char *errStr = jni->GetStringUTFChars(jstr, NULL);
    LM_ASSERT(jni->ExceptionOccurred() == NULL);

    lmj_->exceptionReporter_->insertDiags(da,
                                         -error[0], // LME_RS_INFO_ERROR
                                          errStr);

    jni->ReleaseStringUTFChars(jstr, errStr);
    jni->DeleteLocalRef(jstr);
    status = RS_INFO_ERROR;
    return;
  }

  jint ia[arrSize];
  jlong la[1];

  jni->GetIntArrayRegion(iArray_, 0, arrSize, ia);
  LM_ASSERT(jni->ExceptionOccurred() == NULL);

  jni->GetLongArrayRegion(lArray_, 0, 1, la);
  LM_ASSERT(jni->ExceptionOccurred() == NULL);

  // If the result set is already closed then no further
  // processing is required and return back with
  // the appropriate status
  NABoolean rsClosed = (ia[7] == 0 ? FALSE : TRUE);
  if( rsClosed )
  {
    status = RS_INFO_CLOSED;
    return;
  }

  NABoolean lobData = (ia[0] == 0 ? FALSE : TRUE);
  if( lobData ) {
    *da << DgSqlCode(-LME_LOB_COL_IN_RS_ERROR)
	<< DgString0(routineName);

    status = RS_INFO_LOBCOL;
    return;
  }

  setCtxHandle( (SQLCTX_HANDLE)ia[1] );
  setStmtID( (SQLSTMT_ID *)ia[2] );

  firstBufferedRow_		= ia[3];
  lastBufferedRow_		= ia[4];
  currentRowPosition_	= ia[5];
  cursorType_         = (LmResultSetType) ia[6]; 
  rsCounter_          = la[0];
  CLIStmtClosed_    = (ia[8] == 0 ? FALSE : TRUE);

  jobject connObj = jni->GetObjectArrayElement(oArray_, 0);

  // Create a global references of Connection object
  jobject jdbcConnRef = jni->NewGlobalRef( connObj );

  jni->DeleteLocalRef(connObj);

  // Set RS cursor to to point before first row for Scrollable RS.
  if (isScrollable())
  {
    jni->CallVoidMethod((jobject) jdbcRSRef_,
                        (jmethodID) lmj_->rsBeforeFirstId_);
    currentRowPosition_ = 0;
  }

  // Check if a LmConnection object has already been created for
  // 'jdbcConnRef' if it is then increment the reference count 
  // for that LmConnection object. If not then create a new 
  // LmConnection object add it to the connectionList_.
  ComUInt32 i;
  for( i = 0; i < lmConnList.entries(); i++ )
  {
    lmConn_ = lmConnList[ i ];
    if( jni->IsSameObject( lmConn_->getConnRef(), (jobject)jdbcConnRef ) )
    {
      lmConn_->incrRefCnt();
      jni->DeleteGlobalRef( jdbcConnRef );
      break;
    }
  }

  if( i == lmConnList.entries() ) {
  	//This is not a default connection. For non-default connections we do not perform any transaction 
  	//management so we can set the LmConnection transaction attribute to NO TRANSACTION REQUIRED.
    lmConn_ = new (collHeap()) LmConnection( lmj_,
                                            (jobject)jdbcConnRef,
                                            LmConnection::NON_DEFAULT_CONN, COM_NO_TRANSACTION_REQUIRED); 
    lmConn_->incrRefCnt();
    lmConnList.insert( lmConn_ );
  }

}

// Destructor:
LmResultSetJava::~LmResultSetJava()
{
  JNIEnv *jni = (JNIEnv*)lmj_->jniEnv_;

  if( iArray_ )
    jni->DeleteLocalRef( iArray_ );

  if( lArray_ )
    jni->DeleteLocalRef( lArray_ );

  if( oArray_ )
    jni->DeleteLocalRef( oArray_ );

  if( errCode_ )
    jni->DeleteLocalRef( errCode_ );

  if( errDetail_ )
    jni->DeleteLocalRef( errDetail_ );

  if( jdbcRSRef_ != NULL )
    jni->DeleteGlobalRef((jobject)jdbcRSRef_);

  if (proxySyntax_)
    NADELETEBASIC(proxySyntax_, collHeap());
}

// Makes a JNI call to invoke the close() method on the 
// jdbcRSRef_ object reference (a java.sql.ResultSet object)
// to close the result set.
// 
// Decrements the reference count in the associated LmConnection 
// object.
//
// NOTE: This method should Not return without decrementing it's
// LmConnection object's reference count. Otherwise the LmConnection
// object will not get deleted and it's associated Java connection
// will Not get closed.
void LmResultSetJava::close( ComDiagsArea *da )
{
  JNIEnv *jni = (JNIEnv*)lmj_->jniEnv_;


  if( jdbcRSRef_ != NULL )
  {
    LM_DEBUG0( "LmResultSetJava::close() - closing result set" );

    jni->CallVoidMethod((jobject)jdbcRSRef_,
                        (jmethodID)lmj_->rsCloseId_);

    LM_DEBUG1( "ResultSet.close() returned %s", 
               jni->ExceptionOccurred() ? "an Exception" : "no Exception" );


    // Populate ComDiagsArea if there are any Java exceptions
    if( da )
      lmj_->exceptionReporter_->checkJVMException( da, 0 );
    else
      jni->ExceptionClear();  // If ComDiagsArea is not availabe then we clear
                              // any pending exception
  }

  // The below assertion check is only for an internal sanity check.
  //
  // We will need to decrement the reference count in the LmConnection
  // object but before that we first check if the LmConnection associated 
  // with this result set is existing otherwise we will assert.
  //
  NABoolean found = FALSE;
  ComUInt32 i = 0;

  for( i = 0; i < lmConnList_.entries(); i++ )
  {
    if( lmConn_ == lmConnList_[i] ) {
      found = TRUE;
      break;
    }
  }

  LM_ASSERT(found == TRUE);

  // Decrement the reference count on the associated
  // LmConnection object and remove the object from 
  // lmConnList_ if the connection gets closed.
  if( lmConn_->decrRefCnt() )
    lmConnList_.removeAt( i );


  // Finally delete this object
  delete this;
}

// This function handles the diagnostics
void
LmResultSetJava::insertIntoDiagnostic(ComDiagsArea &da, ComUInt32 col_num)
{
   da << DgSqlCode(-LME_JVM_RESULTSET_ROW_COLUMN_EXCEPTION)
      << DgInt0 ((Lng32) col_num); 
   lmj_->exceptionReporter_->checkJVMException(&da, 0);
}

// This function gets the value of given column of RS object
// as jlong value.
LmResult
LmResultSetJava::getValueAsJlong(jobject javaRS,
                                 ComUInt32 columnIndex,
                                 ComDiagsArea &da,
                                 NABoolean &wasNull,
                                 jlong &returnvalue)
{
  JNIEnv *jni = (JNIEnv*)lmj_->jniEnv_;

  // Get value as BigDecimal object
  jobject bigdecObj = jni->CallObjectMethod(javaRS,
                        (jmethodID)lmj_->rsGetBigDecimalId_,
                        columnIndex);

  if (jni->ExceptionOccurred())
  {
    insertIntoDiagnostic(da, columnIndex);
    return LM_ERR;
  }

  // Check if the value read was null
  wasNull = jni->CallBooleanMethod(javaRS, (jmethodID)lmj_->rsWasNullId_);
  if (jni->ExceptionOccurred())
  {
    insertIntoDiagnostic(da, columnIndex);
    return LM_ERR;
  }

  if (wasNull)
  {
    // Value is NULL, return from here
    return LM_OK;
  }

  // Get BigInteger from bigdecObj
  jobject bigintObj;
  bigintObj = jni->CallObjectMethod(bigdecObj,
                                    (jmethodID)lmj_->bigdecUnscaleId_);
  jni->DeleteLocalRef(bigdecObj);
  if (jni->ExceptionOccurred())
  {
    insertIntoDiagnostic(da, columnIndex);
    return LM_ERR;
  }

  // Get value as jlong from BigInteger
  returnvalue = jni->CallLongMethod(bigintObj,
                                    (jmethodID)lmj_->bigintLongValueId_);
  jni->DeleteLocalRef(bigintObj);

  if (jni->ExceptionOccurred())
  {
    insertIntoDiagnostic(da, columnIndex);
    return LM_ERR;
  }

  return LM_OK;
}

// This function processes one row at a time and returns 1, if it successes.
// If this code is changed in future to process more than one row and returns  
// more than 1, then fetchRowsFromJDBC() function in UdrResultSet.cpp file 
// is also required to change.  
Lng32
LmResultSetJava::fetchSpecialRows(void *dataPtr,
		                  LmParameter *colDesc,
                                  ComUInt32 numCols, 
                                  ComDiagsArea &da,  // will have errors 
                                  ComDiagsArea *rda) // will have warnings
{
  if (! moreSpecialRows())
    return 0;

  JNIEnv *jni = (JNIEnv*)lmj_->jniEnv_;
  jobject javaRS = (jobject) getResultSet();

  // The row at currentRowPosition_ is already accessed.
  // We need to move to next row and advance currentRowPosition_.
  NABoolean ret = (jni->CallBooleanMethod(javaRS, (jmethodID) lmj_->rsNextId_));
  if (jni->ExceptionOccurred())
  {  
     da << DgSqlCode(-LME_JVM_RESULTSET_NEXT_EXCEPTION);
     lmj_->exceptionReporter_->checkJVMException(&da, 0);
     return -1;
  } 

  if (ret)
  { // got next row
    currentRowPosition_++;
    Int32 has_warning = 0;
    
    for (ComUInt32 index = 0; index < numCols; index++)
    {
      Int32 conv_ret = 0;
      NABoolean wasNull = FALSE;
      LmParameter *col = &colDesc[index];
      char *thisColDataPtr = (char*)dataPtr + col->outDataOffset();

      Lng32 retcode = 0;
      LmResult lmResult = LM_OK;

      switch(LmJavaType(col).getType())
      {
        case LmJavaType::JT_TINY:
        {
          jlong jlval;
          lmResult = getValueAsJlong(javaRS, index + 1, da, wasNull, jlval);

          if (lmResult == LM_ERR)
          {
            retcode = -1;  // Diags are already populated
            break;
          }

          if (!wasNull)
	  {
            // Now cast jlong to appropriate value
            if (col->fsType() == COM_SIGNED_BIN8_FSDT)
            {
	      char sval = (char) jlval;
              memcpy(thisColDataPtr, (char *)&sval, col->outSize());
            }
            else
            {
              unsigned char sval = (unsigned char) jlval;
              memcpy(thisColDataPtr, (char *)&sval, col->outSize());
            }
	  }
          
        } // JT_TINY
        break;
        
        case LmJavaType::JT_SHORT:
        {
          jlong jlval;
          lmResult = getValueAsJlong(javaRS, index + 1, da, wasNull, jlval);

          if (lmResult == LM_ERR)
          {
            retcode = -1;  // Diags are already populated
            break;
          }

          if (!wasNull)
	  {
            // Now cast jlong to appropriate value
            if (col->fsType() == COM_SIGNED_BIN16_FSDT)
            {
	      short sval = (short) jlval;
              memcpy(thisColDataPtr, (char *)&sval, col->outSize());
            }
            else
            {
              unsigned short sval = (unsigned short) jlval;
              memcpy(thisColDataPtr, (char *)&sval, col->outSize());
            }
	  }
          
        } // JT_SHORT
        break;
        
        case LmJavaType::JT_INT:
        {
          jlong jlval;
          lmResult = getValueAsJlong(javaRS, index + 1, da, wasNull, jlval);
          
          if (lmResult == LM_ERR)
          {
            retcode = -1;  // Diags are already populated
            break;
          }
          
          if (!wasNull)
          {
            // Now cast jlong to appropriate value
            if (col->fsType() == COM_SIGNED_BIN32_FSDT)
            {
              Int32 ival = (Int32)jlval;
              memcpy(thisColDataPtr, (char *)&ival, col->outSize());
            }
            else
            {
              UInt32 ival = (UInt32)jlval;
              memcpy(thisColDataPtr, (char *)&ival, col->outSize());
            }
          }
          
        } // JT_INT
        break;
        
        case LmJavaType::JT_LONG:
        {
          jlong jlval;
          lmResult = getValueAsJlong(javaRS, index + 1, da, wasNull, jlval);
          
          if (lmResult == LM_ERR)
          {
            retcode = -1;  // Diags are already populated
            break;
          }

          if (!wasNull)
          {
            // Note: don't cast val to long. (long lval = (long) val;)
            // The reason is jlong is 64 bits long. On NSK system long long 
            // is 64 bits long and long is 32 bits long. 
	    // SQL LARGEINT is 64 bit long.
            memcpy(thisColDataPtr, (char *)&jlval, col->outSize());
          }
          
        } // JT_LONG
        break;
        
        case LmJavaType::JT_FLOAT:
        {
          jfloat val = jni->CallFloatMethod(javaRS,
                                            (jmethodID)lmj_->rsGetFloatId_,
                                            index + 1);
          if (jni->ExceptionOccurred())
          {
            insertIntoDiagnostic(da, index + 1);
            retcode = -1;
            break;
          }

          // Check if the value read was null
          wasNull = jni->CallBooleanMethod(javaRS,
                                           (jmethodID)lmj_->rsWasNullId_);
          if (jni->ExceptionOccurred())
          {
            insertIntoDiagnostic(da, index + 1);
            retcode = -1;
          }
          else if (!wasNull)
          {
            memcpy(thisColDataPtr, (char *)&val, col->outSize()); 
          }
          
        } // JT_FLOAT
        break;

        case LmJavaType::JT_DOUBLE:
        {
          jdouble val = jni->CallDoubleMethod(javaRS,
                                              (jmethodID)lmj_->rsGetDoubleId_,
                                              index + 1);
          if (jni->ExceptionOccurred())
          {
            insertIntoDiagnostic(da, index + 1);
            retcode = -1;
            break;
          }

          // Check if the value read was null
          wasNull = jni->CallBooleanMethod(javaRS,
                                           (jmethodID)lmj_->rsWasNullId_);
          if (jni->ExceptionOccurred())
          {
            insertIntoDiagnostic(da, index + 1);
            retcode = -1;
          }
          else if (!wasNull)
          {
            memcpy(thisColDataPtr, (char *)&val, col->outSize());
          }
          
        } // JT_DOUBLE
        break;
        
        case LmJavaType::JT_LANG_STRING:
        {
          // The SQL CHAR strings and SQL INTERVAL types are set as 
          // java string types since JDBC doesn't have type INTERVAL.
          // JDBC handles SQL INTERVAL types as Types.OTHER.
          jobject strObj =
            jni->CallObjectMethod(javaRS,
                                  (jmethodID)lmj_->rsGetObjectId_,
                                  index + 1);
          
          if (jni->ExceptionOccurred())
          {
            insertIntoDiagnostic(da, index + 1);
            retcode = -1;
            break;
          }

          // Check if the value read was null
          wasNull = jni->CallBooleanMethod(javaRS,
                                           (jmethodID)lmj_->rsWasNullId_);
          if (jni->ExceptionOccurred())
          {
            insertIntoDiagnostic(da, index + 1);
            retcode = -1;
          }
          else if (!wasNull)
          {
            if (col->isCharacter())
              conv_ret = lmj_->convertFromString(col, dataPtr, strObj, &da);
            else  // all interval types
              conv_ret = lmj_->convertFromInterval(col, dataPtr, strObj, &da);
            
            if (conv_ret == LM_PARAM_OVERFLOW)
              *rda << DgSqlCode(LME_DATA_OVERFLOW_WARN)
                   << DgInt0((Lng32) index + 1);
            
            if (conv_ret == LM_ERR)
	      retcode = -1;
          }

          jni->DeleteLocalRef(strObj);
                      
        } // JT_LANG_STRING
        break;

        case LmJavaType::JT_MATH_BIGDEC:
        {
          // NOTE: this is a long CASE block. If no Java exception
          // occurs inside the following call then we own an object
          // reference on the BigDecimal object that must be released
          // at the end of the CASE block. Try to avoid early return
          // or break statements or if any are necessary, use caution
          // and be sure to release the reference.

          jobject bigdecObj = jni->CallObjectMethod(javaRS,
                          (jmethodID)lmj_->rsGetBigDecimalId_,
                          index + 1);
          if (jni->ExceptionOccurred())
          {
            insertIntoDiagnostic(da, index + 1);
            retcode = -1;
            break;
          }

          // Check if the value read was null
          wasNull = jni->CallBooleanMethod(javaRS,
                                           (jmethodID)lmj_->rsWasNullId_);
          if (jni->ExceptionOccurred())
          {
            insertIntoDiagnostic(da, index + 1);
            retcode = -1;
          }
          else if (!wasNull)
          {
            // Several SQL types map to the Java BigDecimal type. Cases
            // to consider:
            // * NUMERIC precision > 18 (BIGNUM)
            // * NUMERIC precision <= 18
            // * DECIMAL
            
            if (col->isNumeric())
            {
	      if (col->isBigNum())
	      {
	        // NUMERIC precision > 18

                // Get the value in string format then call convDoIt
	        // to convert it to binary
		conv_ret = lmj_->convertFromBigdec(col, dataPtr, bigdecObj,
                                                   FALSE, // isDecimal
                                                   TRUE,  // copyBinary
                                                   &da);
                
                if (conv_ret == LM_PARAM_OVERFLOW)
                {
                   *rda << DgSqlCode(LME_DATA_OVERFLOW_WARN)
                        << DgInt0((Lng32) index + 1);
                }
                else if (conv_ret == LM_CONV_ERROR)
                {
                  da << DgSqlCode(-LME_CONVERT_ERROR)
                     << DgInt0 ((Lng32) (index + 1));
                  retcode = -1;
                }
                else if (conv_ret == LM_ERR)
                {
                  retcode = -1;
                }
                
	      } // NUMERIC precision > 18

              else
              {
                // NUMERIC precision <= 18
                jobject bigintObj;
                bigintObj =
                  jni->CallObjectMethod(bigdecObj, 
                                        (jmethodID)lmj_->bigdecUnscaleId_);
                
                if (jni->ExceptionOccurred())
                {
                  insertIntoDiagnostic(da, index + 1);
                  retcode = -1;
                }
                else
                {
                  jlong jlval;
                  jlval =
                    jni->CallLongMethod(bigintObj, 
                                        (jmethodID)lmj_->bigintLongValueId_);
                  jni->DeleteLocalRef(bigintObj);
                  
                  if (jni->ExceptionOccurred())
                  {
                    insertIntoDiagnostic(da, index + 1);
                    retcode = -1;
                  }
                  else
                  {
                    if ((col->fsType() == COM_SIGNED_BIN8_FSDT) ||
                        (col->fsType() == COM_UNSIGNED_BIN8_FSDT))
                    {
                      char sval = (char)jlval;
                      memcpy(thisColDataPtr, (char *)&sval, col->outSize());
                    }
                    else
                    if ((col->fsType() == COM_SIGNED_BIN16_FSDT) ||
                        (col->fsType() == COM_UNSIGNED_BIN16_FSDT))
                    {
                      short sval = (short)jlval;
                      memcpy(thisColDataPtr, (char *)&sval, col->outSize());
                    }
                    else
                    if ((col->fsType() == COM_SIGNED_BIN32_FSDT) ||
                          (col->fsType() == COM_UNSIGNED_BIN32_FSDT))
                    {
                      Int32 ival = (Int32)jlval;
                      memcpy(thisColDataPtr, (char *)&ival, col->outSize());
                    }
                    else
                    {
                      // 64 bit value
                      memcpy(thisColDataPtr, (char *)&jlval, col->outSize());
                    }
                  }

                } // if (exception) else ...
                
              } // NUMERIC precision <= 18
            } // NUMERIC
            
            else
            {
              // DECIMAL
              conv_ret = lmj_->convertFromBigdec(col, dataPtr, bigdecObj, 
                                                 col->isDecimal(),
                                                 TRUE /*copyBinary*/, &da); 
              
              if (conv_ret == LM_PARAM_OVERFLOW)
              {
                *rda << DgSqlCode(LME_DATA_OVERFLOW_WARN)
                     << DgInt0((Lng32) index + 1);
              }
              else if (conv_ret == LM_CONV_ERROR)
              {
                // We could have used EXE_CONVERT_STRING_ERROR. To use this
                // error we need to include ExpError.h file. If we include it,
                // then we had compilation issue even after changing 
                // the tdm_sqllangman.mak file.
                da << DgSqlCode(-LME_CONVERT_ERROR)
                   << DgInt0 ((Lng32) (index + 1));
                retcode = -1;
              }
              else if (conv_ret == LM_ERR)
              {
                retcode = -1;
              }
              
            } // DECIMAL
          } // if (!wasNull)
          
          jni->DeleteLocalRef(bigdecObj);
          
        } // JT_MATH_BIGDEC
        break;
        
        case LmJavaType::JT_SQL_DATE:
        {
          jobject dateObj = jni->CallObjectMethod(javaRS,
                                  (jmethodID)lmj_->rsGetObjectId_,
                                  index + 1);
          if (jni->ExceptionOccurred())
          {
            insertIntoDiagnostic(da, index + 1);
            retcode = -1;
            break;
          }
          
          // Check if the value read was null
          wasNull = jni->CallBooleanMethod(javaRS,
                                           (jmethodID)lmj_->rsWasNullId_);
          if (jni->ExceptionOccurred())
          {
            insertIntoDiagnostic(da, index + 1);
            retcode = -1;
          }
          else if (!wasNull)
          {
            conv_ret = lmj_->convertFromDate(col, dataPtr, dateObj, &da);
            
            if (conv_ret == LM_PARAM_OVERFLOW)
            {
              *rda << DgSqlCode(LME_DATA_OVERFLOW_WARN)
                   << DgInt0((Lng32) index + 1);
            }
            else if (conv_ret == LM_ERR)
            {
              retcode = -1;
            }
          }
          
          jni->DeleteLocalRef(dateObj);
          
        } // JT_SQL_DATE
        break;
        
        case LmJavaType::JT_SQL_TIME:
        {
          jobject timeObj =
            jni->CallObjectMethod(javaRS,
                                  (jmethodID)lmj_->rsGetTimeId_,
                                  index + 1);
          if (jni->ExceptionOccurred())
          {
            insertIntoDiagnostic(da, index + 1);
            retcode = -1;
            break;
          }
          
          // Check if the value read was null
          wasNull = jni->CallBooleanMethod(javaRS,
                                           (jmethodID)lmj_->rsWasNullId_);
          if (jni->ExceptionOccurred())
          {
            insertIntoDiagnostic(da, index + 1);
            retcode = -1;
          }
          else if (!wasNull)
          {
            conv_ret = lmj_->convertFromTime(col, dataPtr, timeObj, &da);

            if (conv_ret == LM_PARAM_OVERFLOW)
            {
              *rda << DgSqlCode(LME_DATA_OVERFLOW_WARN)
                   << DgInt0((Lng32) index + 1);
            }
            else if (conv_ret == LM_ERR)
            {
              retcode = -1;
            }
          }
          
          jni->DeleteLocalRef(timeObj);
          
        } // JT_SQL_TIME
        break;
        
        case LmJavaType::JT_SQL_TIMESTAMP:
        {
          jobject timestampObj =
            jni->CallObjectMethod(javaRS,
                                  (jmethodID)lmj_->rsGetTimestampId_,
                                  index + 1);
          if (jni->ExceptionOccurred())
          {
            insertIntoDiagnostic(da, index + 1);
            retcode = -1;
            break;
          }
          
          // Check if the value read was null
          wasNull = jni->CallBooleanMethod(javaRS,
                                           (jmethodID)lmj_->rsWasNullId_);
          if (jni->ExceptionOccurred())
          {
            insertIntoDiagnostic(da, index + 1);
            retcode = -1;
          }
          else if (!wasNull)
          {
            conv_ret = lmj_->convertFromTimestamp(col, dataPtr,
                                                  timestampObj, &da);
            if (conv_ret == LM_PARAM_OVERFLOW)
            {
              *rda << DgSqlCode(LME_DATA_OVERFLOW_WARN)
                   << DgInt0((Lng32) index + 1);
            }
            else if (conv_ret == LM_ERR)
            {
              retcode = -1;
            }
          }
          
          jni->DeleteLocalRef(timestampObj);

        } // JT_SQL_TIMESTAMP
        break;

        default:
          LM_ASSERT(TRUE); 
          break;
          
      } // switch (getType())
      
      if (retcode < 0)
        return retcode;
      
      if ((conv_ret == LM_PARAM_OVERFLOW) && (has_warning == 0))
        has_warning = 1;
      
      // Fill NULL indicator with correct bytes by calling setNull.
      col->setNullOutput((char *)dataPtr, wasNull);

    }  // for each column

    if (has_warning)
    {
      rda->setAllRowNumber(1, DgSqlCode::WARNING_);
    }
   
  }
  else
  {
    // RS.next() returned FALSE. This means that there are no rows left
    // for reading.
    currentRowPosition_ = lastBufferedRow_;
    return 0;
  }

  return 1;
}
