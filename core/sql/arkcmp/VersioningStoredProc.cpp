/**********************************************************************
 *
 * File:         VersioningStoredProc.cpp
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

#include "Platform.h"

#include "VersioningStoredProc.h"
#include "CmpCommon.h"
#include "CmpErrors.h"

#include "CatSQLObjectCache.h"
#include "CatROBaseTable.h"
#include "CatROIndex.h"
#include "CatROView.h"
#include "CatROMV.h"
#include "CatRORoutine.h"
#include "CatError.h"
#include "ComVersionPrivate.h"
#include "CatCatalog.h"
#include "CatCatalogList.h"

#include "ComRtUtils.h"

//----------------------------------------------------------------------
//
// Methods for the Version_Info built-in table-valued function
//
//----------------------------------------------------------------------

static const char VERSION_INFO[] = "VERSION_INFO";

//----------------------------------------------------------------------
//
// To be called before the processing of the VERSION_INFO built-in function call, 
// validates the actual number of input parameters, and provides a description
// of their expected format.
//
SP_STATUS VersionInfoStoredProcedure::sp_InputFormat (SP_FIELDDESC_STRUCT *inputFieldFormat,
				                      Lng32 numFields,
				                      SP_COMPILE_HANDLE spCompileObj,
				                      SP_HANDLE spObj,
                                                      SP_ERROR_STRUCT *error)
{
  if ( numFields != 2 )
  {
    // The VERSION_INFO built-in function accepts two input columns
    error->error = arkcmpErrorISPWrongInputNum;
    strcpy(error->optionalString[0], VERSION_INFO);
    error->optionalInteger[0] = 2;
    return SP_FAIL;
  }
  
  // Describe input columns
  strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "rType  varchar(32) character set iso88591");   // Input type
  strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "rValue varchar(1544 bytes) character set utf8");  // Max length cat.sch.obj, external format
  // 1544 = (128*4+2)*3+2
  return SP_SUCCESS;
}

  
//----------------------------------------------------------------------
//
// To be called before the processing of the VERSION_INFO built-in function call. 
// Provides the number of output columns
//
SP_STATUS VersionInfoStoredProcedure::sp_NumOutputFields (Lng32 *numFields,
							  SP_COMPILE_HANDLE spCompileObj,
							  SP_HANDLE spObj,
							  SP_ERROR_STRUCT *error)
{
  // 5 output columns, including the two input cols
  *numFields = 5;
  return SP_SUCCESS;
}


//----------------------------------------------------------------------
//
// To be called before the processing of the VERSION_INFO built-in function call. 
// Provides the format of the output columns.
// 
SP_STATUS VersionInfoStoredProcedure::sp_OutputFormat(SP_FIELDDESC_STRUCT *format,
						      SP_KEYDESC_STRUCT keyFields[],
						      Lng32 *numKeyFields,
						      SP_HANDLE spCompileObj,
						      SP_HANDLE spObj,
						      SP_ERROR_STRUCT *error)
{
  // Define column names and data types
  strcpy(&((format++)->COLUMN_DEF[0]), "eType     char(32)     character set iso88591 not null");
  strcpy(&((format++)->COLUMN_DEF[0]), "eValue    varchar(1544 bytes) character set utf8 not null");
  // 1544 = (128*4+2)*3+2
  strcpy(&((format++)->COLUMN_DEF[0]), "version   int          not null"); 
  strcpy(&((format++)->COLUMN_DEF[0]), "node_name char(8)      character set iso88591 not null");
  strcpy(&((format++)->COLUMN_DEF[0]), "mxv       int          not null");

  return SP_SUCCESS;
}

//----------------------------------------------------------------------
//
// The actual processing of the VERSION_INFO built-in function call. Will be called
// once, with the SP_PROC_OPEN action; this corresponds to a cursor open.
// Subsequently, multiple calls are issued with the SP_PROC_FETCH action, until
// the method no longer returns SP_MOREDATA.
// Then it will be called once with the SP_PROC_CLOSE action, to "close" the
// cursor.
// 
SP_STATUS 
  VersionInfoStoredProcedure::sp_Process (SP_PROCESS_ACTION action,
	 				  SP_ROW_DATA inputData,
					  SP_EXTRACT_FUNCPTR eFunc,
					  SP_ROW_DATA outputData,
					  SP_FORMAT_FUNCPTR fFunc,
					  SP_KEY_VALUE keys,
					  SP_KEYVALUE_FUNCPTR kFunc,
					  SP_PROCESS_HANDLE *spProcHandle,
					  SP_HANDLE spObj,
					  SP_ERROR_STRUCT *error)
{

  VersionInfoSPContext *context = NULL;

  switch (action) 
  {
  case SP_PROC_OPEN:
    {
      // Initial call, validate input values and obtain result set from
      // catalog manager.
      char inputType [32+1];
      char inputValue [1544+2]; // 1544 = (128*4+2)*3+2 = ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES

      // obtain and validate type parameter. It will have the format
      //   <length><value><spaces>
      // The length part contains the total length, including the trailing spaces.
      if (!getVarcharInputParameter ( 0, eFunc, inputData, sizeof(inputType), inputType, error))
        return SP_FAIL;

      // obtain and validate value parameter. It will have the format
      //   <length><value><spaces>
      // The length part contains the total length, including the trailing spaces.
      if (!getVarcharInputParameter ( 1, eFunc, inputData, sizeof(inputValue), inputValue, error))
        return SP_FAIL;

      // Allocate context and validate input type
      context = new (STMTHEAP) VersionInfoSPContext ;
      *spProcHandle = context;

      try
      {
        context->setInputType  ((varchar *)inputType);
        context->setInputValue ((varchar *)inputValue);

        // Validate catman globals and obtain a pointer to catman cache
        CatProcess.validateCatGlobals();
        CatSQLObjectCache & cache = CatProcess.getSQLObjectCache();

        // Validate the input type
        VersionInfoSPInputType iType = context->validateInputType();

        // By definition, the local node is always returned by VERSION_INFO.
        // except for input type SYSTEM_SCHEMA
        if (iType != iSystemSchema && iType != iUnknown)
        {
          ComNodeName localNode;
          localNode.setLocal();
          context->getNodeSet().insertNode(localNode);
        } 

        // Do the job ...
        const CatROObject * object = NULL;
        switch (iType)
        {
        case iTable:
        case iTableAll:
          {
            // A base table, or a base table + all indexes and trigger temp table (if any)
            object = context->handleInputValue ( COM_TABLE_NAME
                                               , COM_BASE_TABLE_OBJECT
                                               , TRUE               // Resolve synonym
                                               , error);
            if (object == NULL)
              return SP_FAIL;

            const CatROBaseTable * baseTable = object->castToCatROBaseTable();

            // Produce output information
            context->getNodeSet().merge (baseTable->getObjectNodeSet());

            if (iType == iTableAll)
              // get information for child objects too
              baseTable->getAllChildrenNodes (context->getNodeSet());

          }
          break;

        case iIndex:
        case iIndexTable:
          {
            // An index, or an index + its base table
            object = context->handleInputValue ( COM_INDEX_NAME
                                               , COM_INDEX_OBJECT
                                               , FALSE              // Don't resolve synonym
                                               , error);
            if (object == NULL)
              return SP_FAIL;

            const CatROIndex * index = object->castToCatROIndex();

            // Produce output information
            context->getNodeSet().merge (index->getObjectNodeSet());

            if (iType == iIndexTable)
              // get information for parent table too
              index->getParentNodes (context->getNodeSet());

          }
          break;

        case iSchema:
          {
            ComSchemaName schemaName (context->getInputValue());

            // Actual input can be two-part schema name, or three-part object name. First, try schema name.
            if (schemaName.isValid())
            {
              // Not a 3-part object name. Perform full validation.
              if (!validateInputValue (schemaName, context, error))
                return SP_FAIL;
            }
            else
            {
              // Could be 3-part object name, try that.
              ComObjectName tableName (context->getInputValue(), COM_TABLE_NAME, FALSE, STMTHEAP);
              if (!validateInputValue (tableName, context, error))
                return SP_FAIL;

              // OK, a valid 3-part object name. Use the schema part of that
              schemaName.setCatalogNamePart (tableName.getCatalogNamePart());
              schemaName.setSchemaNamePart (tableName.getSchemaNamePart());
            }

            // Set the input value in the context to normalised external format
            context->setInputValue (schemaName.getExternalName());

            // Produce output information
            context->setOutputVersion 
              (cache.getSchemaNodeSet (schemaName , context->getNodeSet()));

          }
          break;

        case iSystemSchema:
          {
            ComNodeName nodeName (context->getInputValue());
            // An empty node name means the local node
            if (*context->getInputValue() == 0 )
              nodeName.setLocal();

            if (!validateInputValue (nodeName, context, error))
              return SP_FAIL;

            // Set the input value in the context to normalised external format
            context->setInputValue (nodeName);

            // Produce output information
            context->getNodeSet().insertNode (nodeName);    
            context->setOutputVersion (cache.getSystemSchemaVersion(nodeName));

          }
          break;

        case iView:
          {
            // A view - regular or materialised
            object = context->handleInputValue ( COM_TABLE_NAME
                                               , COM_VIEW_OBJECT
                                               , TRUE               // Resolve synonym
                                               , error);
            if (object == NULL)
              return SP_FAIL;

            if (object->getObjectType() == COM_VIEW_OBJECT)
            {
              // a regular view, produce output information
              const CatROView * view = object->castToCatROView();
              context->getNodeSet().merge (view->getObjectNodeSet());
            }
            else
            {
              // a materialised view, produce output information
              const CatROMV * view = object->castToCatROMV();
              context->getNodeSet().merge (view->getObjectNodeSet());
            }

          }
          break;

        case iProcedure:
          {
            // A stored procedure
            object = context->handleInputValue ( COM_TABLE_NAME
                                               , COM_USER_DEFINED_ROUTINE_OBJECT
                                               , FALSE              // Don't resolve synonym
                                               , error);
            if (object == NULL)
              return SP_FAIL;

            const CatRORoutine * routine = object->castToCatRORoutine();

            // Produce output information
            context->getNodeSet().merge (routine->getObjectNodeSet());

          }
          break;

        case iConstraint:
          {
            // A constraint
            object = context->handleInputValue ( COM_CONSTRAINT_NAME
                                               , COM_UNKNOWN_OBJECT // don't care about object type
                                               , FALSE              // Don't resolve synonym
                                               , error);
            if (object == NULL)
              return SP_FAIL;
          }
          break;

        case iTrigger:
          {
            // A trigger
            object = context->handleInputValue ( COM_TRIGGER_NAME
                                               , COM_TRIGGER_OBJECT
                                               , FALSE              // Don't resolve synonym
                                               , error);
            if (object == NULL)
              return SP_FAIL;
          }
          break;

        case iSynonym:
          {
            // A synonym
            object = context->handleInputValue ( COM_TABLE_NAME
                                               , COM_SYNONYM_OBJECT
                                               , FALSE              // Don't resolve synonym
                                               , error);
            if (object == NULL)
              return SP_FAIL;
          }
          break;

        case iModule:
        case iUnknown:
        default:
          // Not a known input type / not implemented yet
          error->error = arkcmpErrorISPWrongInputType;
          strcpy(error->optionalString[0], context->getInputType());
          strcpy(error->optionalString[1], VERSION_INFO);
          NADELETE (context, VersionInfoSPContext, STMTHEAP);
          return SP_FAIL;

        }
      }

      catch (...)
      {
        // Some exception ... presumably accompanied
        // by catman errors
        NADELETE (context, VersionInfoSPContext, STMTHEAP);
        error->error = arkcmpErrorISPMergeCatDiags;
        return SP_FAIL;
      }

      // Iterate over node set, issue warnings for unavailable nodes.
      ComVersion_DerivedNodeSet & nodeSet = context->getNodeSet();
      CollIndex rowIndex;
      for (rowIndex=0;rowIndex<nodeSet.entries();rowIndex++)
      {
        ComVersion_NodeInfo & element = nodeSet[rowIndex];
        if (element.getNodeAccessError() != FEOK)
        {
          // There was an error getting the version information for the node. 
          // Use a fake value for MXV.
          element.setMXV(COM_VERSION (999999));
          CatWarning ( CatErrorCode(DISTRIBUTION_NODE_IS_UNAVAILABLE)
                     , DgString0 (element.getNodeName())
                     , DgNskCode (element.getNodeAccessError())
                     );
        }
      }

      if (CatProcess.getDiagnosticsArea()->mainSQLCODE() > 0)
      {
        // Got catman warnings, tell the calling machinery to merge them in.
        error->error = arkcmpErrorISPMergeCatDiags;
        return SP_SUCCESS_WARNING;
      }

    }
    break;




  case SP_PROC_FETCH:
    {
      // Called at least once, to return the information that was obtained above
      // and stored in the context.

      // Get context values
      context = (VersionInfoSPContext *)(*spProcHandle);
      CollIndex rowIndex = context->getRowIndex();
      const ComVersion_DerivedNodeSet & nodeSet = context->getNodeSet();
      Lng32 outputVersion;

      context->setRowIndex (rowIndex + 1);  // Move to next row, for next call

      // if more rows left to process ...
      if (rowIndex < nodeSet.entries())
      {
        // Provide output values from current element
        const ComVersion_NodeInfo & element = nodeSet[rowIndex];
        // First two columns are copies of the corresponding input columns
        fFunc(0,  outputData, strlen(context->getInputType()),  (void *)context->getInputType(),  TRUE);
        fFunc(1,  outputData, strlen(context->getInputValue()), (void *)context->getInputValue(), TRUE);
        // Third column is the version of whatever thing is accessed
        outputVersion = context->getOutputVersion();
        fFunc(2,  outputData, sizeof(Lng32),                     (void *)&outputVersion,           FALSE);
        // Fourth column is the node name from the element
        fFunc(3,  outputData, strlen(element.getNodeName()),    (void *)&element.getNodeName(),   TRUE);
        // Fifth column is the MXV of the node from the element.
        outputVersion = element.getMXV();
        fFunc (4, outputData, sizeof(Lng32), (void *)&outputVersion, FALSE);
        return SP_MOREDATA;
      }
    }
    break;



  case SP_PROC_CLOSE:
    {
      // Called once, after all rows have been processed. 
      // Deallocate our context.
      context = (VersionInfoSPContext *)(*spProcHandle);
      NADELETE (context, VersionInfoSPContext, STMTHEAP);
      break;
    }
  }

  return SP_SUCCESS;  
}

//----------------------------------------------------------------------
// Register the Version_Info static methods with the built-in function
// machinery.
void VersionInfoStoredProcedure::Initialize(SP_REGISTER_FUNCPTR regFunc)
{
  regFunc( (char *)VERSION_INFO,
           sp_Compile,
           sp_InputFormat,
	   0,
           sp_NumOutputFields,
           sp_OutputFormat,
 	   sp_Process,
	   0,
	   CMPISPVERSION);
}




//----------------------------------------------------------------------
//
// Methods for the Relatedness built-in table-valued function
//
//----------------------------------------------------------------------

static const char RELATEDNESS[] = "RELATEDNESS";

//----------------------------------------------------------------------
//
// To be called before the processing of the built-in function call, validates
// the actual number of input parameters, and provides a description
// of their expected format.
//
SP_STATUS RelatednessStoredProcedure::sp_InputFormat (SP_FIELDDESC_STRUCT *inputFieldFormat,
				                      Lng32 numFields,
				                      SP_COMPILE_HANDLE spCompileObj,
				                      SP_HANDLE spObj,
                                                      SP_ERROR_STRUCT *error)
{
  if ( numFields != 2 )
  {
    // The RELATEDNESS built-in function accepts two input columns
    error->error = arkcmpErrorISPWrongInputNum;
    strcpy(error->optionalString[0], RELATEDNESS);
    error->optionalInteger[0] = 2;
    return SP_FAIL;
  }
  
  // Describe input columns
  strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "rType  varchar(32) character set iso88591");   // Input type
  strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "rValue varchar(1029 bytes) character set utf8");  // Max length cat.sch, external format, in UTF8
  return SP_SUCCESS;
}

  
//----------------------------------------------------------------------
//
// To be called before the processing of the built-in function call. 
// Provides the number of output columns
//
SP_STATUS RelatednessStoredProcedure::sp_NumOutputFields (Lng32 *numFields,
							  SP_COMPILE_HANDLE spCompileObj,
							  SP_HANDLE spObj,
							  SP_ERROR_STRUCT *error)
{
  // 3 output columns, including the two input cols
  *numFields = 3;
  return SP_SUCCESS;
}


//----------------------------------------------------------------------
//
// To be called before the processing of the built-in function call. 
// Provides the format of the output columns.
// 
SP_STATUS RelatednessStoredProcedure::sp_OutputFormat(SP_FIELDDESC_STRUCT *format,
						      SP_KEYDESC_STRUCT keyFields[],
						      Lng32 *numKeyFields,
						      SP_HANDLE spCompileObj,
						      SP_HANDLE spObj,
						      SP_ERROR_STRUCT *error)
{
  // Define column names and data types
  strcpy(&((format++)->COLUMN_DEF[0]), "eType     char(32)     character set iso88591 not null"); 
  strcpy(&((format++)->COLUMN_DEF[0]), "eValue    varchar(1029 bytes) character set utf8 not null");
  strcpy(&((format++)->COLUMN_DEF[0]), "eName     varchar(1029 bytes) character set utf8 not null");

  return SP_SUCCESS;
}

//----------------------------------------------------------------------
//
// The actual processing of the built-in function call. Will be called
// once, with the SP_PROC_OPEN action; this corresponds to a cursor open.
// Subsequently, multiple calls are issued with the SP_PROC_FETCH action, until
// the method no longer returns SP_MOREDATA.
// Then it will be called once with the SP_PROC_CLOSE action, to "close" the
// cursor.
// 
SP_STATUS 
  RelatednessStoredProcedure::sp_Process (SP_PROCESS_ACTION action,
	 				  SP_ROW_DATA inputData,
					  SP_EXTRACT_FUNCPTR eFunc,
					  SP_ROW_DATA outputData,
					  SP_FORMAT_FUNCPTR fFunc,
					  SP_KEY_VALUE keys,
					  SP_KEYVALUE_FUNCPTR kFunc,
					  SP_PROCESS_HANDLE *spProcHandle,
					  SP_HANDLE spObj,
					  SP_ERROR_STRUCT *error)
{

  RelatednessSPContext *context = NULL;

  switch (action) 
  {
  case SP_PROC_OPEN:
    {
      // Initial call, validate input values and obtain result set from
      // catalog manager.
      char inputType  [32+2];
      char inputValue [1029+2/*ComMAX_2_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+2*/];

      // obtain and validate type parameter. It will have the format
      //   <length><value><spaces>
      // The length part contains the total length, including the trailing spaces.
      if (!getVarcharInputParameter ( 0, eFunc, inputData, sizeof(inputType), inputType, error))
        return SP_FAIL;

      // obtain and validate value parameter. It will have the format
      //   <length><value><spaces>
      // The length part contains the total length, including the trailing spaces.
      if (!getVarcharInputParameter ( 1, eFunc, inputData, sizeof(inputValue), inputValue, error))
        return SP_FAIL;

      // Allocate context and validate input type
      context = new (STMTHEAP) RelatednessSPContext;
      *spProcHandle = context;

      try
      {
        context->setInputType  ((varchar *)inputType);
        context->setInputValue ((varchar *)inputValue);

        // Validate catman globals and obtain a pointer to catman cache
        CatProcess.validateCatGlobals();
        CatSQLObjectCache & cache = CatProcess.getSQLObjectCache();

        // Validate the input type
        RelatednessSPInputType rType = context->validateInputType();

        // Do the job ...

        switch (rType)
        {
        case rCatalog:
          {
            // A catalog
            ComAnsiNamePart catalogName ( context->getInputValue()
                                        , ComAnsiNamePart::EXTERNAL_FORMAT
                                        , STMTHEAP);
            if (!validateInputValue (catalogName, context, error))
              return SP_FAIL;

            context->setInputValue (catalogName.getExternalName());

            // Produce output information
            cache.getRelatedCatalogSet (catalogName, context->getRelatedItemSet(), CAT_LOCK_READONLY);

          }
          break;

        case rSchema:
          {
            // A schema
            ComSchemaName schemaName (context->getInputValue());
            if (!validateInputValue (schemaName, context, error))
              return SP_FAIL;

            // Set the input value in the context to normalised external format
            context->setInputValue (schemaName.getExternalName());

            // Produce output information
            cache.getRelatedSchemaSet (schemaName, context->getRelatedItemSet(), CAT_LOCK_READONLY);

          }
          break;

        case rNode:
          {
            // A node
            ComNodeName nodeName (context->getInputValue());
            // An empty node name means the local node
            if (*context->getInputValue() == 0 )
              nodeName.setLocal();

            if (!validateInputValue (nodeName, context, error))
              return SP_FAIL;

            // Set the input value in the context to normalised external format
            context->setInputValue (nodeName);

            // Produce output information
            cache.getRelatedNodesSet (nodeName, context->getRelatedItemSet());

          }
          break;

        case rUnknown:
        default:
          // Not a known input type / not implemented yet
          error->error = arkcmpErrorISPWrongInputType;
          strcpy(error->optionalString[0], context->getInputType());
          strcpy(error->optionalString[1], RELATEDNESS);
          NADELETE (context, RelatednessSPContext, STMTHEAP);
          return SP_FAIL;

        }
      }

      catch (...)
      {
        // Some exception ... presumably accompanied
        // by catman errors
        NADELETE (context, RelatednessSPContext, STMTHEAP);
        error->error = arkcmpErrorISPMergeCatDiags;
        return SP_FAIL;
      }

      if (CatProcess.getDiagnosticsArea()->mainSQLCODE() > 0)
      {
        // Got catman warnings, tell the calling machinery to merge them in.
        error->error = arkcmpErrorISPMergeCatDiags;
        return SP_SUCCESS_WARNING;
      }

    }
    break;




  case SP_PROC_FETCH:
    {
      // Called at least once, to return the information that was obtained above
      // and stored in the context.

      // Get context values
      context = (RelatednessSPContext *)(*spProcHandle);
      CollIndex rowIndex = context->getRowIndex();
      const CatRelatedItemSet & relatedItemSet = context->getRelatedItemSet ();

      context->setRowIndex (rowIndex + 1);  // Move to next row, for next call

      // if more rows left to process ...
      if (rowIndex < relatedItemSet.entries())
      {
        // Provide output values from current element
        const CatRelatedItemEntry & element = relatedItemSet[rowIndex];
        // First two columns are copies of the corresponding input columns. Third column is the name of whatever related thing is accessed.
        fFunc(0,  outputData, (Lng32) strlen(context->getInputType()), (void *)context->getInputType(), TRUE);
        fFunc(1,  outputData, (Lng32) strlen(context->getInputValue()), (void *)context->getInputValue(), TRUE);
        fFunc(2,  outputData, (Lng32) element.getExternalItemName().length(), (void *)(element.getExternalItemName().data()), TRUE);
        return SP_MOREDATA;
      }
    }
    break;



  case SP_PROC_CLOSE:
    {
      // Called once, after all rows have been processed. 
      // Deallocate our context.
      context = (RelatednessSPContext *)(*spProcHandle);
      NADELETE (context, RelatednessSPContext, STMTHEAP);
      break;
    }
  }

  return SP_SUCCESS;  
}
//----------------------------------------------------------------------
// Register the Relatedness static methods with the built-in function
// machinery.
void RelatednessStoredProcedure::Initialize(SP_REGISTER_FUNCPTR regFunc)
{
  regFunc( (char *)RELATEDNESS,
           sp_Compile,
           sp_InputFormat,
	   0,
           sp_NumOutputFields,
           sp_OutputFormat,
 	   sp_Process,
	   0,
	   CMPISPVERSION);
}




//----------------------------------------------------------------------
//
// Methods for the Feature_Version_Info built-in table-valued function
//
//----------------------------------------------------------------------

static const char FEATURE_VERSION_INFO[] = "FEATURE_VERSION_INFO";

//----------------------------------------------------------------------
//
// To be called before the processing of the FEATURE_VERSION_INFO built-in function call, 
// validates the actual number of input parameters, and provides a description
// of their expected format.
//
SP_STATUS FeatureVersionInfoStoredProcedure::sp_InputFormat 
                                                     (SP_FIELDDESC_STRUCT *inputFieldFormat,
				                      Lng32 numFields,
				                      SP_COMPILE_HANDLE spCompileObj,
				                      SP_HANDLE spObj,
                                                      SP_ERROR_STRUCT *error)
{
  if ( numFields != 3 )
  {
    // The FEATURE_VERSION_INFO built-in function accepts three input columns
    error->error = arkcmpErrorISPWrongInputNum;
    strcpy(error->optionalString[0], FEATURE_VERSION_INFO);
    error->optionalInteger[0] = 3;
    return SP_FAIL;
  }
  
  // Describe input columns
  strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "rType    varchar(32)     character set iso88591 not null");      
  strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "rValue   varchar(1030 bytes) character set utf8 not null");  
  strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "rVersion int             not null");  
return SP_SUCCESS;
}

  
//----------------------------------------------------------------------
//
// To be called before the processing of the FEATURE_VERSION_INFO built-in function call. 
// Provides the number of output columns
//
SP_STATUS FeatureVersionInfoStoredProcedure::sp_NumOutputFields 
                                                         (Lng32 *numFields,
							  SP_COMPILE_HANDLE spCompileObj,
							  SP_HANDLE spObj,
							  SP_ERROR_STRUCT *error)
{
  // 6 output columns, including the three input cols
  *numFields = 6;
  return SP_SUCCESS;
}


//----------------------------------------------------------------------
//
// To be called before the processing of the FEATURE_VERSION_INFO built-in function call. 
// Provides the format of the output columns.
// 
SP_STATUS FeatureVersionInfoStoredProcedure::sp_OutputFormat
                                                     (SP_FIELDDESC_STRUCT *format,
						      SP_KEYDESC_STRUCT keyFields[],
						      Lng32 *numKeyFields,
						      SP_HANDLE spCompileObj,
						      SP_HANDLE spObj,
						      SP_ERROR_STRUCT *error)
{
  // Define column names and data types
  strcpy(&((format++)->COLUMN_DEF[0]), "eType           char(32)      character set iso88591 not null"); 
  strcpy(&((format++)->COLUMN_DEF[0]), "eValue          varchar(1030 bytes)  character set utf8 not null");
  strcpy(&((format++)->COLUMN_DEF[0]), "eVersion        int           not null");
  strcpy(&((format++)->COLUMN_DEF[0]), "object_name     varchar(1544 bytes) character set utf8     not null");
  // 1544 = (128*4+2)*3+2
  strcpy(&((format++)->COLUMN_DEF[0]), "object_type     char(2)       character set iso88591 not null");
  strcpy(&((format++)->COLUMN_DEF[0]), "feature_version int           not null");

  return SP_SUCCESS;
}

//----------------------------------------------------------------------
//
// The actual processing of the FEATURE_VERSION_INFO built-in function call. Will be 
// called once, with the SP_PROC_OPEN action; this corresponds to a cursor open.
// Subsequently, multiple calls are issued with the SP_PROC_FETCH action, until
// the method no longer returns SP_MOREDATA.
// Then it will be called once with the SP_PROC_CLOSE action, to "close" the
// cursor.
// 
SP_STATUS 
  FeatureVersionInfoStoredProcedure::sp_Process (SP_PROCESS_ACTION action,
	 				         SP_ROW_DATA inputData,
					         SP_EXTRACT_FUNCPTR eFunc,
					         SP_ROW_DATA outputData,
					         SP_FORMAT_FUNCPTR fFunc,
					         SP_KEY_VALUE keys,
					         SP_KEYVALUE_FUNCPTR kFunc,
					         SP_PROCESS_HANDLE *spProcHandle,
					         SP_HANDLE spObj,
					         SP_ERROR_STRUCT *error)
{

  FeatureVersionInfoSPContext *context = NULL;

  switch (action) 
  {
  case SP_PROC_OPEN:
    {
      // Initial call, validate input values and obtain result set from
      // catalog manager.
      char       inputType [32+1];
      char inputValue [1030+2/*ComMAX_2_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+3*/];
      Int32        inputVersion = -1;
      NABoolean  doCascade = FALSE;


      // obtain and validate type parameter. It will have the format
      //   <length><value><spaces>
      // The length part contains the total length, including the trailing spaces.
      if (!getVarcharInputParameter ( 0, eFunc, inputData, sizeof(inputType), inputType, error))
        return SP_FAIL;

      // obtain and validate value parameter. It will have the format
      //   <length><value><spaces>
      // The length part contains the total length, including the trailing spaces.
      if (!getVarcharInputParameter ( 1, eFunc, inputData, sizeof(inputValue), inputValue, error))
        return SP_FAIL;

      // obtain and validate version parameter. 
      inputVersion = getIntInputParameter ( 2, eFunc, inputData, sizeof(Int32), inputVersion, error);
      if (inputVersion == -1)
	  return SP_FAIL;

      // Allocate context and validate input type
      context = new (STMTHEAP) FeatureVersionInfoSPContext ;
      *spProcHandle = context;

      try
      {
        context->setInputType    ((varchar *)inputType);
        context->setInputValue   ((varchar *)inputValue);
        context->setInputVersion (inputVersion);
	
        // Validate the input type
        FeatureVersionInfoSPInputType fType = context->validateInputType();

	// Validate the input version
        if (!validateInputVersion (inputVersion, context, error))
	   return SP_FAIL;

        // Do the job ...
        switch (fType)
        {   
	 case fCatalog: 
	 case fCatalogCascade: 
	  {
	    if (fType == fCatalogCascade)
               doCascade = TRUE; // Cascade option was specified, set boolean 
	    
	    // A catalog
            ComAnsiNamePart catalogName ( context->getInputValue()
                                        , ComAnsiNamePart::EXTERNAL_FORMAT
                                        , STMTHEAP);

            if (!validateInputValue (catalogName, context, error))
               return SP_FAIL;

            // Set the input value in the context to normalised external format
            context->setInputValue (catalogName.getExternalName()); 

	    // Produce output information
	    context->getFeatureVersionInfoSet().buildFeatureVersionInfoSet
                                                            ( catalogName
                                                            , doCascade
                                                            , inputVersion );

          }
         break;

         
	// Schema and Schema Cascade will be implemented in a later release.
	//case fSchema:          
        //case fSchemaCascade:
	//  {
	//    if (fType == fSchemaCascade)
        //       doCascade = TRUE; // Cascade option was specified, set boolean 
        //
        //    // A schema
        //    ComSchemaName schemaName (context->getInputValue());
        //
        //    if (!validateInputValue (schemaName, context, error))
        //     return SP_FAIL;
        //
        //    // Set the input value in the context to normalised external format
        //    context->setInputValue (schemaName.getExternalName());
        //
        //    // Produce output information
	//    context->getFeatureVersionInfoSet().buildFeatureVersionInfoSet
	//                                        ( schemaName.getExternalName()
	//					  , doCascade  
	//			                  , inputVersion );
        //
        //  }
        //  break; */


        case fUnknown:
        default:
          // Not a known input type / not implemented yet
          error->error = arkcmpErrorISPWrongInputType;
          strcpy(error->optionalString[0], context->getInputType());
          strcpy(error->optionalString[1], FEATURE_VERSION_INFO);
          NADELETE (context, FeatureVersionInfoSPContext, STMTHEAP);
          return SP_FAIL;

        }
      } 

      catch (...)
      {
        // Some exception ... presumably accompanied
        // by catman errors
        NADELETE (context, FeatureVersionInfoSPContext, STMTHEAP);
        error->error = arkcmpErrorISPMergeCatDiags;
        return SP_FAIL;
      }

      if (CatProcess.getDiagnosticsArea()->mainSQLCODE() > 0)
      {
        // Got catman warnings, tell the calling machinery to merge them in.
        error->error = arkcmpErrorISPMergeCatDiags;
        return SP_SUCCESS_WARNING;
      }

    }
    break; 


  case SP_PROC_FETCH:
    {
      // Called at least once, to return the information that was obtained above
      // and stored in the context.

      // Get context values
      context = (FeatureVersionInfoSPContext *)(*spProcHandle);
      CollIndex rowIndex = context->getRowIndex();
      const CatFeatureVersionInfoSet & featureVersionInfoSet = context->getFeatureVersionInfoSet();
      Lng32 outputVersion;

      context->setRowIndex (rowIndex + 1);  // Move to next row, for next call

      // if more rows left to process ...
      if (rowIndex < featureVersionInfoSet.entries())
      {
        // Provide output values from current element
	const CatFeatureVersionInfo & element = featureVersionInfoSet.operator[](rowIndex);
	// First three columns are copies of the corresponding input columns
        fFunc(0,  outputData, strlen(context->getInputType()),    (void *)context->getInputType(),    TRUE);
        fFunc(1,  outputData, strlen(context->getInputValue()),   (void *)context->getInputValue(),   TRUE);
	outputVersion = context->getInputVersion();
        fFunc(2,  outputData, sizeof(Lng32),                       (void *)&outputVersion,            FALSE);
	// Forth column is the name of the database object with an OFV higher than eVersion
	fFunc(3,  outputData, (Lng32) element.getExternalItemName().length(),
	                                                          (void *)(element.getExternalItemName().data()), TRUE);
        // Fifth column is the two character object type for the affected database object. 
        fFunc(4,  outputData, strlen(element.getObjectType()),    (void *)element.getObjectType(),   TRUE);
        // Sixth column is the actual OFV of that database object.
	outputVersion = element.getObjectFeatureVersion();
        fFunc (5, outputData, sizeof(Lng32),                       (void *)&outputVersion,             FALSE);
        return SP_MOREDATA;
      }
    }
    break; 


  case SP_PROC_CLOSE:
    {
      // Called once, after all rows have been processed. 
      // Deallocate our context.
      context = (FeatureVersionInfoSPContext *)(*spProcHandle);
      NADELETE (context, FeatureVersionInfoSPContext, STMTHEAP);
      break;
    }
  } 

  return SP_SUCCESS;  
}

//----------------------------------------------------------------------
// Register the Feature_Version_Info static methods with the built-in function
// machinery.
void FeatureVersionInfoStoredProcedure::Initialize(SP_REGISTER_FUNCPTR regFunc)
{
  regFunc( (char *)FEATURE_VERSION_INFO,
           sp_Compile,
           sp_InputFormat,
	   0,
           sp_NumOutputFields,
           sp_OutputFormat,
 	   sp_Process,
	   0,
	   CMPISPVERSION);
}

//----------------------------------------------------------------------
//
// Context management - Base class methods
//
//----------------------------------------------------------------------

// Set type from varchar
void VersioningSPContextBase::setInputType  (const varchar * inputType)
{
  memcpy (eType_, inputType->val, inputType->len);
  eType_[inputType->len] = 0;
}

// Set value from varchar
void VersioningSPContextBase::setInputValue (const varchar * inputValue)
{
  // Allocate buffer of the actual size
  if (eValue_ != NULL)
    NADELETEBASIC (eValue_, STMTHEAP);

  eValue_ = new (STMTHEAP) char [inputValue->len+1];
  memcpy (eValue_, inputValue->val, inputValue->len);
  eValue_[inputValue->len] = 0;
}

// Set value from char string
void VersioningSPContextBase::setInputValue (const char * inputValue)
{
  // Allocate buffer of the actual size
  if (eValue_ != NULL)
    NADELETEBASIC (eValue_, STMTHEAP);

  size_t len = strlen (inputValue);
  eValue_ = new (STMTHEAP) char [len+1];
  memcpy (eValue_, inputValue, len+1);
}


//----------------------------------------------------------------------
//
// Context management - Version info methods
//
//----------------------------------------------------------------------

VersionInfoSPInputType VersionInfoSPContext::validateInputType (void)
{
  VersionInfoSPInputType inputType = iUnknown;
  ComRt_Upshift (eType_);

  inputType = ComVersionInfoSPLiteralToInputType (eType_);
  return inputType;
}

// Generic input handling
const CatROObject * 
VersionInfoSPContext::handleInputValue ( const ComAnsiNameSpace nameSpace
                                       , const ComObjectType expectedObjectType
                                       , const NABoolean resolveSynonym
                                       , SP_ERROR_STRUCT *error)
{
  ComObjectName objName (getInputValue(), nameSpace, FALSE, STMTHEAP);
  if (!validateInputValue (objName, this, error))
    return NULL;

  CatSQLObjectCache & cache = CatProcess.getSQLObjectCache();

  // Open the object. Will throw an exception if the object
  // doesn't exist.
  const CatROObject * object;
  if (resolveSynonym)
    object = cache.openSQLReferenceObject (objName);
  else
    object = cache.openSQLObject (objName);

  // Check that the object really is what we want it to be
  if (expectedObjectType != COM_UNKNOWN_OBJECT)    // don't care ...
  {
    ComObjectType objectObjectType = object->getObjectType();
    if (objectObjectType == COM_MV_OBJECT)
      objectObjectType = COM_VIEW_OBJECT;
    if ( expectedObjectType != objectObjectType)
      // will throw an exception ...
      CatErrObjDoesNotExist (objName);
  }

  // Set the input value in the context to normalised external format
  setInputValue (objName.getExternalName());

  // Produce output version
  setOutputVersion (object->getObjectFeatureVersion());

  return object;
}

// Deallocation method
void VersionInfoSPContext::deleteMe (void)
{
  NADELETE (this, VersionInfoSPContext, STMTHEAP);
}



//----------------------------------------------------------------------
//
// Context management - Relatedness methods
//
//----------------------------------------------------------------------

RelatednessSPInputType RelatednessSPContext::validateInputType (void)
{
  RelatednessSPInputType inputType = rUnknown;
  ComRt_Upshift (eType_);

  inputType = ComRelatednessSPLiteralToInputType (eType_);

  return inputType;
}

// Deallocation method
void RelatednessSPContext::deleteMe (void)
{
  NADELETE (this, RelatednessSPContext, STMTHEAP);
}



//----------------------------------------------------------------------
//
// Context management - Feature_Version_info methods
//
//----------------------------------------------------------------------

FeatureVersionInfoSPInputType FeatureVersionInfoSPContext::validateInputType (void)
{
  FeatureVersionInfoSPInputType inputType = fUnknown;
  ComRt_Upshift (eType_);

  inputType = ComFeatureVersionInfoSPLiteralToInputType (eType_);

  return inputType;
}

// Deallocation method
void FeatureVersionInfoSPContext::deleteMe (void)
{
  NADELETE (this, FeatureVersionInfoSPContext, STMTHEAP);
}


// --------------------------------------------------------------------------------------------------------
// Standalone validation functions
NABoolean getVarcharInputParameter ( Lng32 fieldNo 
                                   , SP_EXTRACT_FUNCPTR eFunc
                                   , SP_ROW_DATA inputData
                                   , size_t maxSize
                                   , char * receivingField
                                   , SP_ERROR_STRUCT *error )
{
  if (eFunc ( fieldNo, inputData, (Lng32)maxSize, receivingField, FALSE) == SP_ERROR_EXTRACT_DATA)
  {
    error->error = arkcmpErrorISPFieldDef;
    return FALSE;
  }

    // Strip leading and trailing blanks and adjust actual length
  varchar * varInput = (varchar *)receivingField;

  // trailing blanks
  char * ptr = &varInput->val[(varInput->len-1)];
  while (*ptr == ' ')
  {
    *(ptr--) = 0;
    varInput->len--;
  }

  // leading blanks
  size_t numBlanks = 0;
  while (varInput->val[numBlanks] == ' ')
    numBlanks++;

  if (numBlanks)
  {
    // strip leading blanks
    for (size_t i = 0;i < (varInput->len-numBlanks);i++)
      varInput->val[i] = varInput->val[i+numBlanks];
    while (numBlanks--)
      varInput->val[--(varInput->len)] = 0;
  }

  return TRUE;
}

Int32 getIntInputParameter ( Lng32 fieldNo 
                         , SP_EXTRACT_FUNCPTR eFunc
                         , SP_ROW_DATA inputData
                         , size_t maxSize
                         , Int32 receivingField
                         , SP_ERROR_STRUCT *error )
{
  if (eFunc ( fieldNo, inputData, (Lng32)maxSize, &receivingField, FALSE) == SP_ERROR_EXTRACT_DATA)
  {
    error->error = arkcmpErrorISPFieldDef;
    return -1;
  }

  return receivingField;
}

void reportInputError ( VersioningSPContextBase * context
                      , SP_ERROR_STRUCT *error )
{
  // Report error -19017
  error->error = arkcmpErrorISPWrongInputValue;
  strcpy(error->optionalString[0], context->getInputValue());
  strcpy(error->optionalString[1], context->getInputType());
  // Deallocate the context
  context->deleteMe();
}

void reportInputVersionError ( VersioningSPContextBase * context
                             , SP_ERROR_STRUCT *error 
                             , const Int32 inVersion )
{
  // Report error -19022
  error->error = arkcmpErrorISPWrongFeatureVersion;
  error->optionalInteger[0] = inVersion;
  // Deallocate the context
  context->deleteMe();
}


NABoolean validateInputValue ( const ComObjectName & object
                             , VersioningSPContextBase * context
                             , SP_ERROR_STRUCT *error )
{
  // Validate that the input object name is a valid 3-part ANSI name
  // in external format
  if ( object.isValid()                        &&
       object.getCatalogNamePart().isValid()   &&
       object.getSchemaNamePart().isValid()
     )
    return TRUE;

  // Something flaky ...
  reportInputError (context, error);
  return FALSE;

}

NABoolean validateInputValue ( const ComSchemaName & schema
                             , VersioningSPContextBase * context
                             , SP_ERROR_STRUCT *error)
{
  // Validate that the input schema name is a valid 2-part schema name
  // in external format
  if ( schema.isValid()                        &&
       schema.getCatalogNamePart().isValid()
     )
    return TRUE;

  // Something flaky ...
  reportInputError (context, error);
  return FALSE;

}

NABoolean validateInputValue ( const ComAnsiNamePart & catalog
                             , VersioningSPContextBase * context
                             , SP_ERROR_STRUCT *error)
{
  // Validate that the input catalog name is a valid ANSI identifier
  // in external format
  if ( catalog.isValid() )
    return TRUE;

  // Something flaky ...
  reportInputError (context, error);
  return FALSE;

}

NABoolean validateInputValue ( const ComNodeName   & node
                             , VersioningSPContextBase * context
                             , SP_ERROR_STRUCT *error)
{
  // Validate that the input node name is a valid Expand node name
  if (node.isValid())
    return TRUE;

  // Something flaky ...
  reportInputError (context, error);
  return FALSE;
}

  
 NABoolean validateInputVersion ( Int32 inVersion 
                                , VersioningSPContextBase * context
                                , SP_ERROR_STRUCT *error)			        
 {  
  // Validate that the input feature version is a valid COM_VERSION
  short oldestSupportedMXV;
  short oldestSupportedSchemaVersion;
  short oldestSupportedSystemSchemaVersion;
  short oldestSupportedPlanVersion;

  if (ComVersion_GetMXVersionVector( (short) inVersion
                                   , oldestSupportedMXV
                                   , oldestSupportedSchemaVersion
                                   , oldestSupportedSystemSchemaVersion
                                   , oldestSupportedPlanVersion ))
     return TRUE;

  // Feature version doesn't exist
  reportInputVersionError (context, error, inVersion);
  return FALSE;
}



