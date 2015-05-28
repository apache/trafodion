/**********************************************************************
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
#ifndef LMLANGMANAGER_H
#define LMLANGMANAGER_H
/* -*-C++-*-
******************************************************************************
*
* File:         LmLangManager.h
* Description:  Language Manager (LM) base and support class definitions. The
*               LM and its various support classes are discussed in the 
*               Language Manager Internal Design Specification.
* Created:      07/01/1999
* Language:     C++
*
******************************************************************************
*/

#include "ComSmallDefs.h"
#include "LmError.h"
#include "NAType.h"
#include "ComDiags.h"
#include "LmCommon.h"
#include "sqludr.h"

#include "Platform.h"

//////////////////////////////////////////////////////////////////////
//
// Contents and Forwards.
//
//////////////////////////////////////////////////////////////////////
class LmLanguageManager;
class LmParameter;
class LmRoutine;
class LmContainer;
class LmTableInfo;

//////////////////////////////////////////////////////////////////////
//
// The LmLanguageManager is an internal support facility for external 
// UDRs in that it encapsulates external language related processing for 
// UDRs. Other components (Cat, Gen, Exe, etc.) call upon the services 
// of the LM in contexts where external language processing is required 
// or desirable. The LM services receive and return values in common 
// internal SQL/MX data format. External formats (e.g., Java/JNI types)
// are never exposed above the LM. LM services return LM_OK upon success 
// unless otherwise indicated.
//
// The LmLangugeManager is an abstract class. Languge specific versions 
// of the LM (e.g., LM for Java, LM for C), implement the virtual service 
// methods of this class. The LM may use the facilities of the Container
// Manager (CM) to gain access to external containers (Java class files,
// C DLLs, etc.) in a language-independent, abstract fashion.
//
//////////////////////////////////////////////////////////////////////
class SQLLM_LIB_FUNC LmLanguageManager : public NABasicObject
{
public:

  LmLanguageManager(NABoolean commandLineMode)
    : commandLineMode_(commandLineMode)
  {
  }

  virtual ComRoutineLanguage getLanguage() const = 0;

  //////////////////////////////////////////////////////////////////////
  // validateRoutine service: Called by the Catman during CREATE 
  // PROCEDURE/FUNCTION to verify the existence and specification of 
  // the underlying external routine.
  //////////////////////////////////////////////////////////////////////
  virtual LmResult validateRoutine(
    ComUInt32     numParam,
    ComFSDataType paramType[],
    ComUInt32     paramSubType[],
    ComColumnDirection direction[],
    const char    *routineName,
    const char    *containerName,
    const char    *externalPath,
    char          *sigBuf,
    ComUInt32     sigLen,
    ComFSDataType resultType = COM_UNKNOWN_FSDT,
    ComUInt32     resultSubType = 0,
    ComUInt32     numResultSets = 0,
    const char    *metaContainerName = NULL,
    const char    *optionalSig = NULL,
    ComDiagsArea *diagsArea = NULL) = 0;

  //////////////////////////////////////////////////////////////////////
  // convertIn service: Called by the Binder or Generator to determine
  // if an in-bound argument type, src, requires converion before LM 
  // processing. Returns:
  //   LM_OK              if src does not require conversion
  //   LM_CONV_REQUIRED   if src requries conversion to dst
  //   LM_CONV_ERROR      if src is not supported
  // 
  // dst is allocted from mem as required.
  //////////////////////////////////////////////////////////////////////
  virtual LmResult convertIn(
    NAType   *src,
    NAType   **dst,
    NAMemory *mem);

  //////////////////////////////////////////////////////////////////////
  // convertOut service: Called by the Binder or Generator to determine
  // if an out-bound argument or return type, src, requires converion 
  // before LM processing. Its semantics are similiar to convertIn.
  //////////////////////////////////////////////////////////////////////
  virtual LmResult convertOut(
    NAType   *src,
    NAType   **dst,
    NAMemory *mem);

  //////////////////////////////////////////////////////////////////////
  // getRoutine service: Called by the SQL runtime (e.g., Exe), likely
  // during "fixup", to allocate a handle to an external routine. The 
  // returned handle is then used to invoke the external routine (see 
  // invokeRoutine).
  //////////////////////////////////////////////////////////////////////
  virtual LmResult getRoutine(
    ComUInt32   numParam,
    LmParameter parameters[],
    ComUInt32   numTableInfo,
    LmTableInfo tableInfo[],
    LmParameter *returnValue,
    ComRoutineParamStyle paramStyle,
    ComRoutineTransactionAttributes transactionAttrs,
    ComRoutineSQLAccess sqlAccessMode,
    tmudr::UDRInvocationInfo *invocationInfo,
    tmudr::UDRPlanInfo       *planInfo,
    const char  *parentQid,
    ComUInt32   inputRowLen,
    ComUInt32   outputRowLen,
    const char  *sqlName,
    const char  *externalName,
    const char  *routineSig,
    const char  *containerName,
    const char  *externalPath,
    const char  *librarySqlName,
    const char  *currentUserName,
    const char  *sessionUserName,
    ComRoutineExternalSecurity externalSecurity,
    Int32 routineOwnerId,
    LmRoutine   **handle,
    LmHandle    getNextRowPtr,
    LmHandle    emitRowPtr,
    ComUInt32   maxResultSets = 0,
    ComDiagsArea *diagsArea = NULL) = 0;

  //////////////////////////////////////////////////////////////////////
  // putRoutine service: Called by the SQL runtime to de-allocate a 
  // handle to an external routine.
  //////////////////////////////////////////////////////////////////////
  virtual LmResult putRoutine(
    LmRoutine *handle,
    ComDiagsArea *diagsArea = NULL) = 0;

  //////////////////////////////////////////////////////////////////////
  // invokeRoutine service: Called by the SQL runtime to invoke the
  // external routine represented by handle. The LmParameter array
  // provided at getRoutine time is expected to remain intact and
  // valid for all subsequent invokeRoutine calls.
  //////////////////////////////////////////////////////////////////////
  virtual LmResult invokeRoutine(
    LmRoutine   *handle,
    void        *inputRow,
    void        *outputRow,
    ComDiagsArea *diagsArea = NULL) = 0;

  //////////////////////////////////////////////////////////////////////
  // The following public methods are not "LM service methods", but 
  // rather LM provided support methods for the CM.

  //////////////////////////////////////////////////////////////////////
  // createLoader: Called by the CM to create a language specific loader
  // to manage the specified path. A Java class loader is an example of
  // one of these loaders; A C LM would not require this functionality.
  //////////////////////////////////////////////////////////////////////
  virtual LmHandle createLoader(const char *externalPath, ComDiagsArea *diagsArea) = 0;

  //////////////////////////////////////////////////////////////////////
  // deleteLoader: Called by the CM to destory a language specific loader.
  //////////////////////////////////////////////////////////////////////
  virtual void deleteLoader(LmHandle) = 0;

  //////////////////////////////////////////////////////////////////////
  // loadContainer: Called by the CM to load a language specific container
  // (e.g., Java class file or C DLL) as specified by container name and 
  // path, using the specified external loader if language appropriate. 
  //////////////////////////////////////////////////////////////////////
  virtual LmHandle loadContainer(
    const char *containerName,
    const char *externalPath,
    LmHandle   extLoader,
    ComUInt32  *containerSize,
    ComDiagsArea *diagsArea) = 0;

  //////////////////////////////////////////////////////////////////////
  // unloadContainer: Called when container access is no longer required.
  //////////////////////////////////////////////////////////////////////
  virtual void unloadContainer(LmHandle) = 0;

  //////////////////////////////////////////////////////////////////////
  // containerExtension: Returns the language specific container 
  // extension (e.g., class for Java).
  //////////////////////////////////////////////////////////////////////
  virtual const char* containerExtension() = 0;

  //////////////////////////////////////////////////////////////////////
  // skipURLProtocol: Returns the index in externalPath beyond the file 
  // protocol of a file URL. File URLs without the protocol are allowed.
  //////////////////////////////////////////////////////////////////////
  Int32 skipURLProtocol(const char *externalPath);
  
  //////////////////////////////////////////////////////////////////////
  // Functions allowing LM callers to inspect properties of the 
  // external runtime system. For Java these will be the standard
  // Java system properties. For other external languages the 
  // meaning of "system property" is not yet defined but could be
  // mapped to environment variables or something along those lines.
  //////////////////////////////////////////////////////////////////////
  virtual LmResult getSystemProperty(
    const char *key,
    char *value,
    ComUInt32 bufferLen,
    ComBoolean &propertyIsSet,
    ComDiagsArea *diagsArea = NULL) = 0;

  virtual LmResult setSystemProperty(
    const char *key,
    const char *value,
    ComDiagsArea *diagsArea = NULL) = 0;

  NABoolean isRoutineActive() const { return isRoutineActive_; }
  void setRoutineIsActive(NABoolean b) { isRoutineActive_ = b; }
 
  NABoolean getCommandLineMode() const { return commandLineMode_; }

protected:

  NABoolean commandLineMode_;

  // To indicate if the Language Manager is currently invoking a routine
  NABoolean isRoutineActive_;

private:
  // Do not implement a default constructor
  LmLanguageManager();

}; // class LmLanguageManager

#endif
