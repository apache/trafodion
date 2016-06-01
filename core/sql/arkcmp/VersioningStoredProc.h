/**********************************************************************
 *
 * File:         VersioningStoredProc.h
 * Description:  Implementation of the VERSION_INFO built-in function
 *               and related context management
 *
 * Created:      February 2005
 * Language:     C++
 *
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
#ifndef VERSIONINGSP_H
#define VERSIONINGSP_H

#include "CmpISPStd.h"
#include "ComDistribution.h"
#include "ComVersionNodeInfo.h"
#include "ComObjectName.h"
#include "ComSchemaName.h"
#include "ComGuardianFileNameParts.h"
#include "ComPointerList.h"

// Don't use for memory allocation - variable length struct
struct varchar
{
  short len;
  char  val[1];   // actual varchar size depends upon value of length
};

// ---------------------------------------------------------------------------------------------------------
// VersionInfoStoredProcedure implements the functionality of the VERSION_INFO built-in function.
// The version information is obtained from catalog manager.
// --------------------------------------------------------------------------------------------------------
class VersionInfoStoredProcedure {

public:
  VersionInfoStoredProcedure();
  virtual ~VersionInfoStoredProcedure();

  // Initialize() is called at the time when the stored procedure is initially
  // being registered with ARKCMP.  The default implementation does  
  // nothing and the derived class can implement whatever initialization 
  // behavior it wants.
  static void Initialize(SP_REGISTER_FUNCPTR regFunc);

  // sp_Compile. For Embedded SQL environment, a stored procedure is  
  // compiled only  the first time it is invoked.	
  static SP_STATUS sp_Compile(SP_COMPILE_ACTION action,
			      SP_COMPILE_HANDLE *pCompileObj,
			      SP_HANDLE pObj,
			      SP_ERROR_STRUCT* error)
  {
    return SP_SUCCESS;
  }

  // sp_InputFormat is called with action=OPEN before any compile-time 
  // functions are called.  It is then again called after all compile-time
  // functions have been called, this time with action=CLOSE.
  static SP_STATUS sp_InputFormat(SP_FIELDDESC_STRUCT *inputFieldFormat,
				  Lng32 numFields,
				  SP_COMPILE_HANDLE spCompileObj,
				  SP_HANDLE spObj,
                                  SP_ERROR_STRUCT *error);

  // sp_NumOutputFields function is called at compile-time of the stored 
  // procedure to inquire about the number of output fields in a row.
  static SP_STATUS sp_NumOutputFields(Lng32 *numFields,
				      SP_COMPILE_HANDLE spCompileObj,
				      SP_HANDLE spObj,
				      SP_ERROR_STRUCT *error);

  // sp_OutputFormat is called at compile-time of the stored procedure to 
  // determine  the format (type info) of each field that will become part of the 
  // row being  output from the stored procedure.
  static SP_STATUS sp_OutputFormat(SP_FIELDDESC_STRUCT *outputFieldFormat,
			           SP_KEYDESC_STRUCT keyFields[],
				   Lng32 *numKeyFields,
				   SP_HANDLE spCompileObj,
				   SP_HANDLE spObj,
				   SP_ERROR_STRUCT *error);

  // sp_Process is called at run-time of the stored procedure. 
  static SP_STATUS sp_Process(SP_PROCESS_ACTION action,
     			      SP_ROW_DATA inputData,
	     		      SP_EXTRACT_FUNCPTR eFunc,
			      SP_ROW_DATA outputData,
			      SP_FORMAT_FUNCPTR fFunc,
			      SP_KEY_VALUE keys,
			      SP_KEYVALUE_FUNCPTR kFunc,
			      SP_PROCESS_HANDLE *spProcHandle,
			      SP_HANDLE spObj,
			      SP_ERROR_STRUCT *error);
};


// ---------------------------------------------------------------------------------------------------------
// RelatednessStoredProcedure implements the functionality of the RELATEDNESS built-in function.
// The version information is obtained from catalog manager.
// --------------------------------------------------------------------------------------------------------
class RelatednessStoredProcedure {

public:
  RelatednessStoredProcedure();
  virtual ~RelatednessStoredProcedure();

  // Initialize() is called at the time when the stored procedure is initially
  // being registered with ARKCMP.  The default implementation does  
  // nothing and the derived class can implement whatever initialization 
  // behavior it wants.
  static void Initialize(SP_REGISTER_FUNCPTR regFunc);

  // sp_Compile. For Embedded SQL environment, a stored procedure is  
  // compiled only  the first time it is invoked.	
  static SP_STATUS sp_Compile(SP_COMPILE_ACTION action,
			      SP_COMPILE_HANDLE *pCompileObj,
			      SP_HANDLE pObj,
			      SP_ERROR_STRUCT* error)
  {
    return SP_SUCCESS;
  }

  // sp_InputFormat is called with action=OPEN before any compile-time 
  // functions are called.  It is then again called after all compile-time
  // functions have been called, this time with action=CLOSE.
  static SP_STATUS sp_InputFormat(SP_FIELDDESC_STRUCT *inputFieldFormat,
				  Lng32 numFields,
				  SP_COMPILE_HANDLE spCompileObj,
				  SP_HANDLE spObj,
                                  SP_ERROR_STRUCT *error);

  // sp_NumOutputFields function is called at compile-time of the stored 
  // procedure to inquire about the number of output fields in a row.
  static SP_STATUS sp_NumOutputFields(Lng32 *numFields,
				      SP_COMPILE_HANDLE spCompileObj,
				      SP_HANDLE spObj,
				      SP_ERROR_STRUCT *error);

  // sp_OutputFormat is called at compile-time of the stored procedure to 
  // determine  the format (type info) of each field that will become part of the 
  // row being  output from the stored procedure.
  static SP_STATUS sp_OutputFormat(SP_FIELDDESC_STRUCT *outputFieldFormat,
			           SP_KEYDESC_STRUCT keyFields[],
				   Lng32 *numKeyFields,
				   SP_HANDLE spCompileObj,
				   SP_HANDLE spObj,
				   SP_ERROR_STRUCT *error);

  // sp_Process is called at run-time of the stored procedure. 
  static SP_STATUS sp_Process(SP_PROCESS_ACTION action,
     			      SP_ROW_DATA inputData,
	     		      SP_EXTRACT_FUNCPTR eFunc,
			      SP_ROW_DATA outputData,
			      SP_FORMAT_FUNCPTR fFunc,
			      SP_KEY_VALUE keys,
			      SP_KEYVALUE_FUNCPTR kFunc,
			      SP_PROCESS_HANDLE *spProcHandle,
			      SP_HANDLE spObj,
			      SP_ERROR_STRUCT *error);
};

// ---------------------------------------------------------------------------------------------------------
// FeatureVersionInfoStoredProcedure implements the functionality of the FEATURE_VERSION_INFO built-in 
// function. The version information is obtained from catalog manager.
// --------------------------------------------------------------------------------------------------------
class FeatureVersionInfoStoredProcedure {

public:
  FeatureVersionInfoStoredProcedure();

  virtual ~FeatureVersionInfoStoredProcedure();

  // Initialize() is called at the time when the stored procedure is initially
  // being registered with ARKCMP.  The default implementation does  
  // nothing and the derived class can implement whatever initialization 
  // behavior it wants.
  static void Initialize(SP_REGISTER_FUNCPTR regFunc);

  // sp_Compile. For Embedded SQL environment, a stored procedure is  
  // compiled only  the first time it is invoked.	
  static SP_STATUS sp_Compile(SP_COMPILE_ACTION action,
			      SP_COMPILE_HANDLE *pCompileObj,
			      SP_HANDLE pObj,
			      SP_ERROR_STRUCT* error)
  {
    return SP_SUCCESS;
  }

  // sp_InputFormat is called with action=OPEN before any compile-time 
  // functions are called.  It is then again called after all compile-time
  // functions have been called, this time with action=CLOSE.
  static SP_STATUS sp_InputFormat(SP_FIELDDESC_STRUCT *inputFieldFormat,
				  Lng32 numFields,
				  SP_COMPILE_HANDLE spCompileObj,
				  SP_HANDLE spObj,
                                  SP_ERROR_STRUCT *error);

  // sp_NumOutputFields function is called at compile-time of the stored 
  // procedure to inquire about the number of output fields in a row.
  static SP_STATUS sp_NumOutputFields(Lng32 *numFields,
				      SP_COMPILE_HANDLE spCompileObj,
				      SP_HANDLE spObj,
				      SP_ERROR_STRUCT *error);

  // sp_OutputFormat is called at compile-time of the stored procedure to 
  // determine  the format (type info) of each field that will become part of the 
  // row being  output from the stored procedure.
  static SP_STATUS sp_OutputFormat(SP_FIELDDESC_STRUCT *outputFieldFormat,
			           SP_KEYDESC_STRUCT keyFields[],
				   Lng32 *numKeyFields,
				   SP_HANDLE spCompileObj,
				   SP_HANDLE spObj,
				   SP_ERROR_STRUCT *error);

  // sp_Process is called at run-time of the stored procedure. 
  static SP_STATUS sp_Process(SP_PROCESS_ACTION action,
     			      SP_ROW_DATA inputData,
	     		      SP_EXTRACT_FUNCPTR eFunc,
			      SP_ROW_DATA outputData,
			      SP_FORMAT_FUNCPTR fFunc,
			      SP_KEY_VALUE keys,
			      SP_KEYVALUE_FUNCPTR kFunc,
			      SP_PROCESS_HANDLE *spProcHandle,
			      SP_HANDLE spObj,
			      SP_ERROR_STRUCT *error);
};

// --------------------------------------------------------------------------------------------------------
// Abstract base class for the context to be kept during execution
// of the VERSION_INFO, RELATEDNESS and FEATURE_VERSION_INFO built-in functions
class VersioningSPContextBase
{
public:
  VersioningSPContextBase (void)
    : rowIndex_  (0)
    , eValue_ (NULL)
  {
    // set the type to a null string
    eType_[0] = 0;
  };

  virtual ~VersioningSPContextBase (void) {if (eValue_ != NULL) NADELETEBASIC (eValue_, STMTHEAP);};

  // Accessors
  const char * getInputType (void) const
    { return eType_; };
  const char * getInputValue (void) const
    { return eValue_; };

  CollIndex getRowIndex (void) const
    { return rowIndex_; };
  
  // Mutators
  void setRowIndex (const CollIndex rowIndex)
    { rowIndex_ = rowIndex; };
  void setInputType  (const varchar * inputType);
  void setInputValue (const varchar * inputValue);
  void setInputValue (const char * inputValue);

  // Deallocation function
  virtual void deleteMe (void) = 0;

protected:

  // Data members
  char                  eType_[33];     // Input type
  char *                eValue_;        // Only allocate what's required
  CollIndex             rowIndex_;
};


// --------------------------------------------------------------------------------------------------------
// Class to hold context during the execution of the VERSION_INFO
// built-in function. Must be allocated from the STMTHEAP.
class VersionInfoSPContext : public VersioningSPContextBase
{
public:
  VersionInfoSPContext (void)
    : outputVersion_ (COM_VERS_UNKNOWN)
    , nodeSet_ (CatProcess.getGlobalNodeSet())
  {};
  virtual ~VersionInfoSPContext (void) {};

  // Accessors
  COM_VERSION getOutputVersion (void) const
    { return outputVersion_; };
  ComVersion_DerivedNodeSet & getNodeSet (void)
    { return nodeSet_; };

  // Mutators
  void setOutputVersion (const COM_VERSION version)
    { outputVersion_ = version; };

  // Input validation
  VersionInfoSPInputType validateInputType (void);

  // Generic input handling
  const CatROObject * handleInputValue ( const ComAnsiNameSpace nameSpace
                                       , const ComObjectType expectedObjectType
                                       , const NABoolean resolveSynonym
                                       , SP_ERROR_STRUCT *error);
                                    

  // Deallocation function
  virtual void deleteMe (void);

private:
  COM_VERSION               outputVersion_;
  ComVersion_DerivedNodeSet nodeSet_;

};

// --------------------------------------------------------------------------------------------------------
// Class to hold context during the execution of the RELATEDNESS
// built-in function. Must be allocated from the STMTHEAP.

class RelatednessSPContext : public VersioningSPContextBase
{
public:
  RelatednessSPContext (void)
  {};

  virtual ~RelatednessSPContext (void)
  {};

  // Accessors
  CatRelatedItemSet & getRelatedItemSet (void)
    { return relatedItemSet_; };

  // Input validation
  RelatednessSPInputType validateInputType (void);

  // Deallocation function
  void deleteMe (void);

private:
  CatRelatedItemSet relatedItemSet_;

};


// --------------------------------------------------------------------------------------------------------
// Class to hold context during the execution of the FEATURE_VERSION_INFO
// built-in function. Must be allocated from the STMTHEAP.
// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------

class FeatureVersionInfoSPContext : public VersioningSPContextBase
{
public:
  FeatureVersionInfoSPContext (void)
  {};
  
  virtual ~FeatureVersionInfoSPContext (void) 
  {};

  // Accessors
  Int32 getInputVersion (void)
    { return eVersion_; };

  
  CatFeatureVersionInfoSet & getFeatureVersionInfoSet (void)
    { return featureVersionInfoSet_; };

  
  void setInputVersion (Int32 inVersion)
  { eVersion_ = inVersion; };

  // Input validation
  FeatureVersionInfoSPInputType validateInputType (void);  
  
  // Deallocation function
  void deleteMe (void);

private:
  Int32                       eVersion_;
  CatFeatureVersionInfoSet  featureVersionInfoSet_;
};


// --------------------------------------------------------------------------------------------------------
// Standalone input validation functions. Will deallocate the context object - if any - in case of an error
NABoolean getVarcharInputParameter ( Lng32 fieldNo 
                                   , SP_EXTRACT_FUNCPTR eFunc
                                   , SP_ROW_DATA inputData
                                   , size_t maxSize
                                   , char * receivingField
                                   , SP_ERROR_STRUCT *error );

Int32 getIntInputParameter ( Lng32 fieldNo 
                         , SP_EXTRACT_FUNCPTR eFunc
                         , SP_ROW_DATA inputData
                         , size_t maxSize
                         , Int32 receivingField
                         , SP_ERROR_STRUCT *error );  

NABoolean validateInputValue ( const ComObjectName & object
                             , VersioningSPContextBase * context
                             , SP_ERROR_STRUCT *error );

NABoolean validateInputValue ( const ComSchemaName & schema
                             , VersioningSPContextBase * context
                             , SP_ERROR_STRUCT *error);

NABoolean validateInputValue ( const ComAnsiNamePart & catalog
                             , VersioningSPContextBase * context
                             , SP_ERROR_STRUCT *error);

NABoolean validateInputValue ( const ComNodeName   & node
                             , VersioningSPContextBase * context
                             , SP_ERROR_STRUCT *error);

NABoolean validateInputVersion ( Int32 inVersion 
                               , VersioningSPContextBase * context
                               , SP_ERROR_STRUCT *error);


#endif
