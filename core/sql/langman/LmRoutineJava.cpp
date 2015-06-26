/* -*-C++-*-
**********************************************************************
*
* File:         LmRoutineJava.cpp
* Description:  
*
* Created:      08/22/2003
* Language:     C++
*
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2015 Hewlett-Packard Development Company, L.P.
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


#include "lmjni.h"
#include "ComRtUtils.h"
#include "LmRoutineJava.h"
#include "LmJavaExceptionReporter.h"
#include "LmAssert.h"
#include "LmUtility.h"
#include "ComObjectName.h"
#include "ComSqlId.h"

//////////////////////////////////////////////////////////////////////
//
// Class LmRoutineJava
// If we get errors in creating this object, fill diagsArea and return
// error. Caller is responsbile to cleanup by calling destructor.
//
//////////////////////////////////////////////////////////////////////
LmRoutineJava::LmRoutineJava(
  const char            *sqlName,
  const char            *externalName,
  const char            *librarySqlName,
  ComUInt32              numSqlParam,
  LmParameter           *returnValue,
  ComUInt32              maxResultSets,
  char                  *routineSig,
  ComRoutineParamStyle   paramStyle,
  ComRoutineTransactionAttributes transactionAttrs,
  ComRoutineSQLAccess    sqlAccessMode,
  ComRoutineExternalSecurity externalSecurity,
  Int32                 routineOwnerId,
  const char            *parentQid,
  ComUInt32              inputRowLen,
  ComUInt32              outputRowLen,
  const char            *currentUserName,
  const char            *sessionUserName,
  LmParameter           *parameters,
  LmLanguageManagerJava *lm,
  LmHandle               routine,
  LmContainer           *container,
  ComDiagsArea          *da)
  : LmRoutine(container, routine, sqlName, externalName, librarySqlName,
              numSqlParam, maxResultSets,
              COM_LANGUAGE_JAVA, paramStyle, transactionAttrs, sqlAccessMode,
              externalSecurity, 
	      routineOwnerId,
              parentQid, inputRowLen, outputRowLen, 
              currentUserName, sessionUserName, 
	      parameters, lm),
    javaParams_(NULL),
    retType_(LmJavaType::JT_NONE),
    defaultCatSch_(FALSE)
{
  JNIEnv *jni = (JNIEnv*)getLM()->jniEnv_;

  setUdrForJavaMain((str_cmp_ne(externalName, "main") == 0) ? TRUE : FALSE);

  // this is the internal SPJ used in CREATE PROCEDURE
  // mark it for examining Java exceptions to determine SQL diagnostics later

  if ((str_cmp_ne(container->getName(), "org.trafodion.sql.udr.LmUtility") == 0) &&
      (str_cmp_ne(externalName, "validateMethod") == 0))
    setIsInternalSPJ(TRUE);
  else
    setIsInternalSPJ(FALSE);

  if (paramStyle == COM_STYLE_JAVA_OBJ)
    return;
 
  // Get method return type.
  retType_ = LmJavaType(returnValue).getType();

  //
  // Now setup the parameters to the method.
  //
  // UDR-MAIN gets special treatment
  //
  // Allocate only one jvalue because main() takes only
  // one parameter of type String[].
  //
  // Then create java.lang.String array of size numSqlParam_
  // which equals to numSqlParam
  //
  if (isUdrForJavaMain())
  {
    numParamsInSig_ = 1;  // Number of parameters in Java main method

    javaParams_ = new (collHeap()) jvalue[1];
    memset((char*)javaParams_, 0, sizeof(jvalue));

    ((jvalue *)javaParams_)->l = jni->NewObjectArray((jsize)numSqlParam_,
                                      (jclass)lm->stringClass_,
                                      NULL);
    if (((jvalue *)javaParams_)->l == NULL)
    {
      *da << DgSqlCode(-LME_JVM_OUT_OF_MEMORY);
      getLM()->exceptionReporter_->checkJVMException(da, 0);
    }

    return;
  }

  // For a Java main method we would have already returned and 
  // do not get this far. Logic in the rest of this method does 
  // not need to consider main methods as a special case.

  // Get the total number of parameters present in the method 
  // signature.
  LmJavaSignature lmSig(routineSig, collHeap());
  Int32 result = lmSig.getParamCount();
  if( result < (Int32)numSqlParam_ ) {
    *da << DgSqlCode(-LME_INTERNAL_ERROR)
        << DgString0(": LmJavaSignature::getParamCount() returned an invalid value.");
    return;
  }
  numParamsInSig_ = (ComUInt32)result;

  //
  // Setup Paramters for Non-main methods
  //
  // Allocate Java method parameter array for SQL
  // and result set params
  if (numParamsInSig_ > 0)
  {
    javaParams_ = new (collHeap()) jvalue[numParamsInSig_];
    memset((char*)javaParams_, 0, numParamsInSig_ * sizeof(jvalue));
  }

 
  // Allocate a 1-element array for each OUT/INOUT mode parameter.
  jvalue *jval = (jvalue*)javaParams_;

  Int32 i = 0;
  for (i = 0; i < (Int32)numSqlParam_; i++)
  {
    LmParameter &p = lmParams_[i];

    if (p.direction() == COM_INPUT_COLUMN)
      continue;

    jobjectArray ja = NULL;

    switch (LmJavaType(&p).getType())
    {
    case LmJavaType::JT_SHORT:
      jval[i].l = (jobject)jni->NewShortArray(1);
      break;

    case LmJavaType::JT_INT:
      jval[i].l = (jobject)jni->NewIntArray(1);
      break;

    case LmJavaType::JT_LONG:
      jval[i].l = (jobject)jni->NewLongArray(1);
      break;

    case LmJavaType::JT_FLOAT:
      jval[i].l = (jobject)jni->NewFloatArray(1);
      break;

    case LmJavaType::JT_DOUBLE:
      jval[i].l = (jobject)jni->NewDoubleArray(1);
      break;

    case LmJavaType::JT_LANG_STRING:
      ja = (jobjectArray) jni->NewObjectArray(1, (jclass)lm->stringClass_, NULL);
      break;

    case LmJavaType::JT_MATH_BIGDEC:
      ja = (jobjectArray) jni->NewObjectArray(1, (jclass)lm->bigdecClass_, NULL);
      break;

    case LmJavaType::JT_SQL_DATE:
      ja = (jobjectArray) jni->NewObjectArray(1, (jclass)lm->dateClass_, NULL);
      break;

    case LmJavaType::JT_SQL_TIME:
      ja = (jobjectArray) jni->NewObjectArray(1, (jclass)lm->timeClass_, NULL);
      break;

    case LmJavaType::JT_SQL_TIMESTAMP:
      ja = (jobjectArray) jni->NewObjectArray(1, (jclass)lm->stampClass_, NULL);
      break;

    case LmJavaType::JT_LANG_INTEGER:
      ja = (jobjectArray) jni->NewObjectArray(1, (jclass)lm->intClass_, NULL);
      break;

    case LmJavaType::JT_LANG_LONG:
      ja = (jobjectArray) jni->NewObjectArray(1, (jclass)lm->longClass_, NULL);
      break;

    case LmJavaType::JT_LANG_FLOAT:
      ja = (jobjectArray) jni->NewObjectArray(1, (jclass)lm->floatClass_, NULL);
      break;

    case LmJavaType::JT_LANG_DOUBLE:
      ja = (jobjectArray) jni->NewObjectArray(1, (jclass)lm->doubleClass_, NULL);
      break;

    default:
      char str[LMJ_ERR_SIZE_256];
      sprintf (str,
               ": Unknown parameter type at parameter position %d.",
               i+1);
      *da << DgSqlCode(-LME_INTERNAL_ERROR)
          << DgString0(str);
      return;
    }// switch()

    if (ja != NULL)
      jval[i].l = ja;

    if (jval[i].l == NULL)
    {
      // OutOfMemory is the only error that could happen here.
      *da << DgSqlCode(-LME_JVM_OUT_OF_MEMORY);

      getLM()->exceptionReporter_->checkJVMException(da, 0);
      return;
    }

  } // for()

  // Allocate a 1-element array for each result set parameter.
  for (i = (Int32)numSqlParam_; i < (Int32)numParamsInSig_; i++)
  {
    jobjectArray ja = NULL;

    ja = jni->NewObjectArray(1, (jclass)lm->resultSetClass_, NULL);
    if (ja != NULL)
      jval[i].l = ja;

    if (jval[i].l == NULL)
    {
      // OutOfMemory is the only error that could happen here.
      *da << DgSqlCode(-LME_JVM_OUT_OF_MEMORY);

      getLM()->exceptionReporter_->checkJVMException(da, 0);
      return;
    }
  }

  return;
}

LmRoutineJava::~LmRoutineJava()
{
  JNIEnv *jni = (JNIEnv*)getLM()->jniEnv_;
  jvalue *jval = (jvalue*)javaParams_;

  // Free LmResultSet objects
  // This will also close the Java result set objects
  // and the Java connections that they are part of.
  cleanupResultSets();

  // Free LmConnection objects for default connections
  // Closes any open default connections that do not have 
  // result sets associated with them
  closeDefConnWithNoRS();

  connectionList_.clear();
    
  // Release array refs for params, indicated
  // by non-null object.
  for (Int32 i = 0; i < (Int32)numParamsInSig_; i++)
  {
    if (jval[i].l != NULL)
      jni->DeleteLocalRef(jval[i].l);
  }

  // Free the Java parameter array
  if (javaParams_)
    NADELETEBASIC((jvalue *)javaParams_, collHeap());
}

// Generates a new authentication token for Definer Rights SPJ usage.
// The format of the token fields is as follows:
// o	\5, \6  - identifies that it is a DR SPJ token - 2 bytes
// o	Definer user ID - routine owner Id - 7 bytes - leading zero padded
// o	UDR server Process Node id - 3 bytes - leading zero padded
// o	UDR server Process Pid - 6 bytes - leading zero padded
// o	Invoker password/token passed from executor - 68 bytes
const int DR_SPJ_TOKEN_PREFIX_LEN = 2;
const int DR_SPJ_TOKEN_USER_ID_LEN = 7;
const int DR_SPJ_TOKEN_NODE_LEN = ComSqlId::CPU_LEN; // 3
const int DR_SPJ_TOKEN_PID_LEN = ComSqlId::PIN_LEN; // 6
const int DR_SPJ_TOKEN_INVOKER_TOKEN_LEN = 68;
const int DR_SPJ_TOKEN_SEPARATOR_LEN = 1;
const int DR_SPJ_TOKEN_LEN =
            DR_SPJ_TOKEN_PREFIX_LEN +
            DR_SPJ_TOKEN_SEPARATOR_LEN +
            DR_SPJ_TOKEN_USER_ID_LEN +
            DR_SPJ_TOKEN_SEPARATOR_LEN +
            DR_SPJ_TOKEN_NODE_LEN +
            DR_SPJ_TOKEN_SEPARATOR_LEN +
            DR_SPJ_TOKEN_PID_LEN +
            DR_SPJ_TOKEN_SEPARATOR_LEN +
            DR_SPJ_TOKEN_INVOKER_TOKEN_LEN;

LmResult LmRoutineJava::handleFinalCall(ComDiagsArea *diagsArea)
{
  // this method is not yet used for Java
  return LM_OK;
}

LmResult LmRoutineJava::generateDefAuthToken(char *defAuthToken, ComDiagsArea *da)
{
  // Get the NODE, PID information for the UDR server process
  const int MAX_PROGRAM_DIR_LEN = 1024;
  char myProgramDir[MAX_PROGRAM_DIR_LEN+1];
  short myProcessType;
  Int32 myNode;
  char myNodeName[MAX_SEGMENT_NAME_LEN+1];
  Lng32  myNodeNum;
  short myNodeNameLen = MAX_SEGMENT_NAME_LEN;
  char myProcessName[PROCESSNAME_STRING_LEN];
  Int64 myProcessStartTime;
  pid_t myPid;
  Lng32 retStatus = ComRtGetProgramInfo(myProgramDir, MAX_PROGRAM_DIR_LEN,
		                        myProcessType,
		                        myNode,
                                        myPid,
                                        myNodeNum,
                                        myNodeName, myNodeNameLen,
                                        myProcessStartTime,
                                        myProcessName);
  if (retStatus)
  {
     char errStr[LMJ_ERR_SIZE_256];
     sprintf (errStr, ": Error returned from ComRtGetProgramInfo. Error is:  %d.", retStatus);
     *da << DgSqlCode(-LME_INTERNAL_ERROR)
         << DgString0(errStr);
     return LM_ERR;
   }

  sprintf (defAuthToken,
		  "%s%s:%07d:%03u:%06u:%.68s",
		  "\005", "\006",
		  getRoutineOwnerId(),
		  myNode,
		  myPid,
		  getLM()->userPassword_
		  );
  return LM_OK;
}


LmResult LmRoutineJava::invokeRoutine(void *inputRow,
				      void *outputRow,
				      ComDiagsArea *da)
{
  // Delete the LmResultSet objects for re-invocations
  // This will also close the Java result set objects
  // and the Java connections that they are part of
  cleanupResultSets(da);

  // This will close any default connections that do not have 
  // result sets associated with them
  closeDefConnWithNoRS(da);

  // Cleanup the list maintained in LmUtlity.cpp file that will get
  // populated with default connection objects that may get created
  // during this routine's invocation.
  lmUtilityInitConnList((JNIEnv*)getLM()->jniEnv_, (jmethodID)getLM()->connCloseId_);

  // Set the default catalog & schema as system properties. These
  // values are then retrieved by LmSQLMXDriver class for setting
  // it on a getConnection() call
  if (getDefaultCatSchFlag())
  {
    // Determine the catalog & schema values that will be in effect for
    // this UDR. If the values are set globally(with catalog/schema
    // props in CQD) they are used else the catalog/schema of
    // the UDR are used. 
    const char *catalog =
      (getLM()->sysCatalog_) ? getLM()->sysCatalog_ : udrCatalog_;
    const char *schema =
      (getLM()->sysSchema_) ? getLM()->sysSchema_ : udrSchema_;

    if (getLM()->setSystemProperty("sqlmx.udr.catalog", catalog, da) == LM_ERR)
      return LM_ERR;
    if (getLM()->setSystemProperty("sqlmx.udr.schema", schema, da) == LM_ERR)
      return LM_ERR;
  }

  const char *parentQid;
  if ((parentQid = getParentQid()) == NULL)
    parentQid = "NONE";
  if (getLM()->setSystemProperty("sqlmx.udr.parentQid", parentQid, da) == LM_ERR)
      return LM_ERR;
  // Set SQL access mode in LmUtility. This will be checked while SPJ
  // is trying to make a JDBC connection.
  lmUtilitySetSqlAccessMode(getSqlAccessMode());
  
  // Set Transaction Attribute in LmUtility. This will be checked while SPJ
  // is trying to join a transaction.
  lmUtilitySetTransactionAttrs(getTransactionAttrs());

  LmResult result = LM_OK;

  // Set the Definer Authentication Token as system property for
  // Definer Rights SPJs. This value is then retrieved by LmSQLMXDriver class 
  // for setting the password property in getConnection() call.
  if ( externalSecurity_ == COM_ROUTINE_EXTERNAL_SECURITY_DEFINER )
  {
      char *defAuthToken = new (collHeap()) char[DR_SPJ_TOKEN_LEN + 1];
      result = generateDefAuthToken(defAuthToken, da);
      if (result != LM_ERR)
         result = getLM()->setSystemProperty("sqlmx.udr.defAuthToken", defAuthToken, da);
      NADELETEBASIC(defAuthToken, collHeap());
      if (result == LM_ERR)
      {
        char errStr[LMJ_ERR_SIZE_256];
        sprintf (errStr,
                 ": Error returned from setting System Property:  %s.",
       	         "sqlmx.udr.defAuthToken");
        *da << DgSqlCode(-LME_INTERNAL_ERROR)
            << DgString0(errStr);
        return LM_ERR;
      }
  }

  lm_->setRoutineIsActive(TRUE);

  switch (retType_)
  {
    case LmJavaType::JT_VOID:
      result = voidRoutine(outputRow, NULL);
      break;
    default:
      // Other return types are not supported. In the future if we
      // support routines with non-void return types we will need to
      // add case blocks to this switch statement.
      LM_ASSERT(0);
      break;
  }
  
  lm_->setRoutineIsActive(FALSE);

  // Reset the sqlmx.udr.defAuthToken system property for
  // Definer Rights SPJs. 
  if ( externalSecurity_ == COM_ROUTINE_EXTERNAL_SECURITY_DEFINER )
  {
      result = getLM()->clearSystemProperty("sqlmx.udr.defAuthToken", da);
      if (result == LM_ERR)
      {
        char errStr[LMJ_ERR_SIZE_256];
        sprintf (errStr,
                 ": Error returned from Resetting System Property:  %s.",
       	         "sqlmx.udr.defAuthToken");
        *da << DgSqlCode(-LME_INTERNAL_ERROR)
            << DgString0(errStr);
        return LM_ERR;
      }
  }

  // Get the list containing the default connection objects
  // from LmUtility.cpp and create a LmConnection object for 
  // each item in that list.
  NAList<jobject> &connList = lmUtilityGetConnList();
  while(connList.entries())
  {
    jobject conn = connList[0];
    LmConnection *lmConn = 
      new (collHeap()) LmConnection(getLM(), conn,
                                    LmConnection::DEFAULT_CONN, getTransactionAttrs());
    
    connectionList_.insert( lmConn );
    connList.removeAt(0);
  }

  // reset SQL Access mode to COM_UNKNOWN_ROUTINE_SQL_ACCESS
  lmUtilitySetSqlAccessMode(COM_UNKNOWN_ROUTINE_SQL_ACCESS);
  
  // reset Transaction Attributes to COM_UNKNOWN_ROUTINE_TRANSACTION_ATTRIBUTE
  lmUtilitySetTransactionAttrs(COM_UNKNOWN_ROUTINE_TRANSACTION_ATTRIBUTE);

  return result;
}

// Deletes the LmResultSet object at a given index.
void LmRoutineJava::cleanupLmResultSet(ComUInt32 index,
                                       ComDiagsArea *diagsArea)
{
  LM_ASSERT(index < resultSetList_.entries());

  ((LmResultSetJava *)resultSetList_[index])->close( diagsArea );
  resultSetList_.removeAt(index);
}

// Deletes the given LmResultSet object.
// Any error during the close will be reported in the
// diagsArea, if available. Asserts on an invalid pointer.
void LmRoutineJava::cleanupLmResultSet(LmResultSet *resultSet,
                                       ComDiagsArea *diagsArea)
{
  for (ComUInt32 index=0; index < resultSetList_.entries(); index++)
  {
    if (resultSetList_[index] == resultSet)
    {
      ((LmResultSetJava *)resultSetList_[index])->close(diagsArea);
      resultSetList_.removeAt(index);

      return;
    }
  }

  // passed in resultset param is not a valid LmResultSet pointer.
  LM_ASSERT(0);
}

// Deletes all the LmResultSet objects from the resultSetList_ 
// and empties the list.
void LmRoutineJava::cleanupResultSets(ComDiagsArea *diagsArea)
{
  // Free LmResultSetJava objects
  while( resultSetList_.entries() )
    cleanupLmResultSet( (ComUInt32) 0, diagsArea );

  resultSetList_.clear();
}

// Closes any default connections in connectionList_,
// which do not have any result sets associated with
// them.
void LmRoutineJava::closeDefConnWithNoRS(ComDiagsArea *diagsArea)
{
  ComUInt32 i = 0;

  while( i < connectionList_.entries() )
  {
    LmConnection *lmConn = connectionList_[i];
    if( lmConn->connType() == LmConnection::DEFAULT_CONN &&
        lmConn->readyToClose() )
    {
      lmConn->close( diagsArea );
      connectionList_.removeAt(i);
    }
    else
      i++;  // Increment only when LmConnection::close() is
            // not called.
  }
}

// This method checks if the passed in java.sql.ResultSet object
// reference (newRS) is already part of a LmResultSetJava
// object in the resultSetList_.
//
// The parameter newRS should not be NULL.
// Returns TRUE if a match is found FALSE otherwise
NABoolean LmRoutineJava::isDuplicateRS( LmHandle newRS )
{
  JNIEnv *jni = (JNIEnv*)getLM()->jniEnv_;
  LmResultSetJava *lmRS;
  jobject newRSRef = (jobject)newRS;
  ComUInt32 indx;

  LM_ASSERT(newRS != NULL);

  for (indx = 0; indx < resultSetList_.entries(); indx++)
  {
	  lmRS = (LmResultSetJava *)resultSetList_[indx];

    // IsSameObject() JNI call compares the object references
    // of it's parameters
	  if (jni->IsSameObject( (jobject)lmRS->getResultSet(), newRSRef ))
	    return TRUE;

  }  // End for loop

  return FALSE;
}

// This method creates a LmResultSetJava object for the 
// passed in java.sql.ResultSet object (newRS) and inserts 
// it into resultSetList_.
//
// Parameters:
// newRS    - A java.sql.ResultSet object
// paramPos - The position (1-based) of newRS
//            in the Java method signature.
// da       - ComDiagsArea object to report any errors. 
//            This object should not be NULL.
LmResult LmRoutineJava::populateResultSetInfo(LmHandle newRS, Int32 paramPos, 
                                              ComDiagsArea *da)
{
  LmResult result = LM_OK;
  jobject newRSRef = (jobject)newRS;

  LM_ASSERT(da != NULL);

  // SPJ method can return same java.sql.ResultSet object in 
  // multiple OUT params but we ignore such duplicates.
  // We also ignore a NULL java.sql.ResultSet object.
  if( newRSRef == NULL || isDuplicateRS( newRSRef ))
    return result;

  LmResultSetJava::LmResultSetInfoStatus rsInfoStatus;

  LmResultSetJava *newLmRS = 
			  new (collHeap()) LmResultSetJava( getLM(),
                                          (LmHandle)newRSRef,
                                          paramPos,
                                          getNameForDiags(),
                                          rsInfoStatus,
                                          connectionList_,
                                          da );

  // If the constructor returned a success status then insert 
  // the newLmRS object into resultSetList_ otherwise delete it.
  if( rsInfoStatus == LmResultSetJava::RS_INFO_OK ) {

    // Order the objects inserted in the resultSetList_ based on
    // the order in which the result set cursors were opened.
    ComUInt32 i;
    LmResultSetJava *lmRS;
    for( i = 0; i < resultSetList_.entries(); i++ )
    {
      lmRS = (LmResultSetJava *)resultSetList_[i];
      if( newLmRS->rsCounter_ < lmRS->rsCounter_ )
        break;
    }
    resultSetList_.insertAt( i, newLmRS );
  }
  else {
    delete newLmRS;
    newLmRS = NULL;

    // Ignore closed result sets
    if( rsInfoStatus == LmResultSetJava::RS_INFO_CLOSED )
      result = LM_OK;
    else
      result = LM_ERR;
  }

  return result;
}

// Deletes the LmResultSetJava objects in resultSetList_
// which are over and above the routine's decalred max result set value.
void LmRoutineJava::deleteLmResultSetsOverMax(ComDiagsArea *diagsArea)
{
  ComUInt32 toDelete = 0;
  
  if( resultSetList_.entries() > maxResultSets_ )
    toDelete = resultSetList_.entries() - maxResultSets_;

  while( toDelete-- )
    cleanupLmResultSet( maxResultSets_, diagsArea );

  return;
}

// check validity of the object after calling the constructor
ComBoolean LmRoutineJava::isValid()
{
  if (retType_ > LmJavaType::JT_NONE &&
      retType_ <= LmJavaType::JT_LAST)
    return TRUE;
  return FALSE;
}

//////////////////////////////////////////////////////////////////////
// voidRoutine: invoke a static Java method that returns void
//////////////////////////////////////////////////////////////////////
LmResult LmRoutineJava::voidRoutine(void *dataPtr, LmParameter *)
{
  JNIEnv *jni = (JNIEnv*)getLM()->jniEnv_;

  jni->CallStaticVoidMethodA((jclass)getContainerHandle(),
                             (jmethodID)routine_,
                             (jvalue*)javaParams_);
  return LM_OK;
}
