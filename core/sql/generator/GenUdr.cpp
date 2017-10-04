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
* File:         GenUdr.cpp
* Description:  Generator code for UDR operators
* Created:      10/28/2000
* Language:     C++
*
******************************************************************************
*/

#include "RelMisc.h"
#include "RelRoutine.h"
#include "NARoutine.h"
#include "LmExpr.h"
#include "LmGenUtil.h"
#include "LmError.h"
#include "ComTdbUdr.h"
#include "Generator.h"
#include "GenExpGenerator.h"
#include "sql_buffer.h"
#include "NAUserId.h"
#include "ComUser.h"
#include "ExplainTuple.h"

#include "ExplainTupleMaster.h"
#include "ComQueue.h"
#include "UdfDllInteraction.h"

// Helper function to allocate a string in the plan
char *AllocStringInSpace(ComSpace &space, const char *s)
{
  char *result = space.allocateAndCopyToAlignedSpace(s, str_len(s));
  return result;
}

// Helper function to allocate binary data in the plan. The data
// will be preceded by a 4-byte length field
char *AllocDataInSpace(ComSpace &space, const char *data, UInt32 len)
{
  char *result = space.allocateAndCopyToAlignedSpace(data, len, 4);
  return result;
}

// A helper function to add optional data from a UDR TDB
// instance into the description string for EXPLAIN output.
static void addOptionalData(Queue *optData, NAString &description)
{
  if (optData == NULL)
    return;

  char buf[128];
  Int32 i = 0;
  const char *s = NULL;

  optData->position();
  while ((s = (const char *) optData->getNext()) != NULL)
  {
    // Each data element is prefixed by a 4-byte length field
    UInt32 len = 0;
    str_cpy_all((char *)&len, s, 4);
    if (len > 0)
    {
      sprintf(buf, "optional_data[%d]: ", i++);
      description += buf;

      // Create a buffer containing at most 200 bytes of data
      if (len > 200)
        len = 200;
      char truncatedBuf[201];
      str_cpy_all(truncatedBuf, s + 4, (Lng32) len);
      truncatedBuf[len] = 0;

      // Change NULL bytes and non-ASCII characters to a dot for
      // display purposes. Also change colons to dots because tools
      // that parse EXPLAIN output consider colon a token delimiter.
      for (UInt32 j = 0; j < len; j++)
      {
        if (truncatedBuf[j] == 0 ||
            !isascii(truncatedBuf[j]) || 
            truncatedBuf[j] == ':')
        {
          truncatedBuf[j] = '.';
        }
      }

      description += truncatedBuf;
      description += " ";

    } // if (len > 0)
  } // for each data element
}


ExplainTuple *IsolatedNonTableUDR::addSpecificExplainInfo(ExplainTupleMaster *explainTuple, 
                                             ComTdb *tdb, 
                                             Generator *generator)
{

  ULng32 i;
  char buf[128];
  const QualifiedName &qname = getEffectiveNARoutine()->getSqlName();
  NAString ansiName = qname.getQualifiedNameAsString();
 
  NAString description = "routine_name: ";
  description += ansiName;

  description += " parameter_modes: ";
  const ULng32 numParams = getEffectiveNARoutine()->getParamCount();

  if (numParams == 0)
  {
    description += "none";
  }
  else
  {
    for (i = 0; i < numParams; i++)
    {
      const NAColumnArray &formalParams = getEffectiveNARoutine()->getParams();
      const NAColumn &c = *(formalParams[i]);
      ComColumnDirection dir = ((NAColumn &)c).getColumnMode();

      switch (dir)
      {
        case COM_INPUT_COLUMN:
          description += COM_INPUT_COLUMN_LIT;
          break;
        case COM_OUTPUT_COLUMN:
          description += COM_OUTPUT_COLUMN_LIT;
          break;
        case COM_INOUT_COLUMN:
          description += COM_INOUT_COLUMN_LIT;
          break;
        case COM_UNKNOWN_DIRECTION:
        default:
        {
          NAString msg = "No parameter mode specified for routine ";
          msg += ansiName;
          GenAssert(FALSE, msg.data());
          break;
        }
      }

    } // for each SQL parameter
  } // if numParams > 0

  description += " sql_access_mode: ";
  switch (getEffectiveNARoutine()->getSqlAccess())
  {
    case COM_NO_SQL:
      description += "NO SQL";
      break;
    case COM_CONTAINS_SQL:
      description += "CONTAINS SQL";
      break;
    case COM_READS_SQL:
      description += "READS SQL DATA";
      break;
    case COM_MODIFIES_SQL:
      description += "MODIFIES SQL DATA";
      break;
    case COM_UNKNOWN_ROUTINE_SQL_ACCESS:
    default:
    {
      NAString msg = "No SQL access mode specified for routine ";
      msg += ansiName;
      GenAssert(FALSE, msg.data());
      break;
    }
  } // switch on SQL access mode

  description += " external_name: ";
  description += getEffectiveNARoutine()->getExternalName();

  description += " library: ";
  description += getEffectiveNARoutine()->getLibrarySqlName().getExternalName();

  description += " external_file: ";
  description += getEffectiveNARoutine()->getFile();

  ComUInt32 sigLen = getEffectiveNARoutine()->getSignature().length();
  description += " signature: ";
  description += (sigLen > 0 ? getEffectiveNARoutine()->getSignature() : "(none)");

  description += " language: ";
  switch (getEffectiveNARoutine()->getLanguage())
  {
    case COM_LANGUAGE_JAVA:
      description += "JAVA";
      break;
    case COM_LANGUAGE_C:
      description += "C";
      break;
    case COM_LANGUAGE_CPP:
      description += "C++";
      break;
    case COM_LANGUAGE_SQL:
      description += "SQL";
      break;
    case COM_UNKNOWN_ROUTINE_LANGUAGE:
    default:
    {
      NAString msg = "No language specified for routine ";
      msg += ansiName;
      GenAssert(FALSE, msg.data());
      break;
    }
  } // switch on language type

  description += " parameter_style: ";
  switch (getEffectiveNARoutine()->getParamStyle())
  {
    case COM_STYLE_JAVA_CALL:
      description += "JAVA";
      break;
    case COM_STYLE_JAVA_OBJ:
      description += "JAVA_OBJ";
      break;
    case COM_STYLE_SQL:
      description += "SQL";
      break;
    case COM_STYLE_SQLROW:
      description += "SQLROW";
      break;
    case COM_STYLE_SQLROW_TM:
      description += "SQLROW_TM";
      break;
    case COM_STYLE_CPP_OBJ:
      description += "C++";
      break;
    case COM_UNKNOWN_ROUTINE_PARAM_STYLE:
    default:
    {
      NAString msg = "No parameter style specified for routine ";
      msg += ansiName;
      GenAssert(FALSE, msg.data());
      break;
    }
  }

  description += " external_security: ";
  switch (getEffectiveNARoutine()->getExternalSecurity())
  {
    case COM_ROUTINE_EXTERNAL_SECURITY_INVOKER:
      description += "INVOKER";
      break;
    case COM_ROUTINE_EXTERNAL_SECURITY_DEFINER:
      description += "DEFINER";
      break;
    case COM_ROUTINE_EXTERNAL_SECURITY_IMPLEMENTATION_DEFINED:
    default:
    {
      NAString msg = "No external security specified for routine ";
      msg += ansiName;
      GenAssert(FALSE, msg.data());
      break;
    }
  }

  ComTdbUdr &udrTdb = *((ComTdbUdr *) tdb);
#if 0
  // On Neo, CQDs are not exposed so this block of code is commented out.
  description += " runtime_options: ";
  description += udrTdb.getRuntimeOptions();
  description += " runtime_option_delimiters: '";
  description += udrTdb.getRuntimeOptionDelimiters();
  description += "'";
#endif

  ComSInt32 maxRS = getEffectiveNARoutine()->getMaxResults();
  sprintf(buf, "%d", (Int32) maxRS);
  description += " max_result_sets: ";
  description += buf;

  description += " ";
  addOptionalData(udrTdb.getOptionalData(), description);

  explainTuple->setDescription(description);
  explainTuple->setTableName(ansiName);

  return explainTuple;
}


ExplainTuple *SPProxyFunc::addExplainInfo(ComTdb *tdb, 
                                          ExplainTuple *leftChild,
                                          ExplainTuple *rightChild,
                                          Generator *generator)
{
  ExplainTupleMaster *explainTuple = (ExplainTupleMaster *)
    RelExpr::addExplainInfo(tdb, leftChild, rightChild, generator);

  NAString description = "";

  ComTdbUdr &udrTdb = *((ComTdbUdr *) tdb);

  addOptionalData(udrTdb.getOptionalData(), description);

  explainTuple->setDescription(description);

  return explainTuple;
}

ExplainTuple *PhysicalTableMappingUDF::addSpecificExplainInfo(ExplainTupleMaster *explainTuple, 
                                             ComTdb *tdb, 
                                             Generator *generator)
{

  ULng32 i;
  NAString name = getUserTableName().getCorrNameAsString();
  const NAColumnArray & formalInputParams = getScalarInputParams();
  const NAColumnArray & outputColumns = getOutputParams();

  NAString description = "TMUDF_name: ";
  description += name;

  description += " input_parameters: ";
  for (CollIndex i=0; i<formalInputParams.entries(); i++)
    {
      if (i > 0)
        description += ", ";
      description += formalInputParams[i]->getColName();
    }

  description += " result_columns: ";
  for (CollIndex o=0; o<outputColumns.entries(); o++)
    {
      if (o > 0)
        description += ", ";
      description += outputColumns[o]->getColName();
    }
  
  description += " external_name: ";
  description += getNARoutine()->getExternalName();

  description += " library: ";
  description += getNARoutine()->getLibrarySqlName().getExternalName();

  description += " external_file: ";
  if (getNARoutine()->getLanguage() == COM_LANGUAGE_JAVA)
    description += getNARoutine()->getExternalPath();
  else
    description += getNARoutine()->getFile(); // CPP
  description += " ";

  explainTuple->setDescription(description);
  explainTuple->setTableName(name);

  return explainTuple;
}

static ULng32 min_sql_buffer_overhead()
{
  // When we compute buffer sizes for IPC messages, we sometimes want
  // to know how much SqlBuffer overhead is required to send one data
  // row plus a NO_DATA indicator. This function returns the number of
  // bookkeeping bytes required. The bookkeeping consists of SqlBuffer
  // class members, two control rows, and one data row.
  const ULng32 bufferPad = 
    sizeof(SqlBuffer)
    + (2 * (sizeof(ControlInfo) + sizeof(tupp_descriptor)))
    + sizeof(tupp_descriptor)   // for the data row
    + 100                       // just to be safe
    ;

  return bufferPad;
}


//--------------------------------------------------------------------------
// Our code generator for UDR operators is implemented by a static
// function. Several physical RelExpr classes currently exist for UDR
// interactions. They are CallSP for the CALL statement,
// PhysicalSPProxyFunc for result set proxy statements and IsolatedScalarUDF
// for general UDF support. Each of those
// RelExpr classes has a codeGen() method that calls this udr_codegen()
// function.
//--------------------------------------------------------------------------
static short udr_codegen(Generator *generator,
                             RelExpr &relExpr,
                             ComTdbUdr *&newTdb,
                             const RoutineDesc *rdesc,
                             const ValueIdList *inVids,
                             const ValueIdList *outVids,
                             const NAColumnArray *inColumns,
                             const NAColumnArray *outColumns,
                             const NAColumnArray *formalColumns,
                             Cardinality estimatedRowCount,
                             ULng32 downQueueMaxSize,
                             ULng32 upQueueMaxSize,
                             ULng32 outputBufferSize,
                             ULng32 requestBufferSize,
                             ULng32 replyBufferSize,
                             ULng32 numOutputBuffers,
                             const char *runtimeOptsFromCaller,
                             const char *runtimeOptDelimsFromCaller,
                             ComTdb ** childTdbs,
                             Queue *optionalData,
                             tmudr::UDRInvocationInfo *udrInvocationInfo,
                             tmudr::UDRPlanInfo *udrPlanInfo)
{
  CmpContext *cmpContext = generator->currentCmpContext();
  Space *space = generator->getSpace();
  ExpGenerator *exp_gen = generator->getExpGenerator();
  MapTable *map_table = generator->getMapTable();
  MapTable *last_map_table = generator->getLastMapTable();
  ex_expr *input_expr = NULL;
  ex_expr *output_expr = NULL;
  ex_expr *scan_expr = NULL;
  ex_expr *proj_expr = NULL;
  ex_expr ** childInput_exprs = NULL ;
  ULng32 i;
  ULng32 requestRowLen = 0;
  ULng32 replyRowLen = 0;
  ULng32 outputRowLen = 0;
  ULng32 childInputRowLen = 0;
  ULng32 combinedRequestChildOutputRowLen = 0;
  ExpTupleDesc *requestTupleDesc = NULL;     // input params in langman format
  ExpTupleDesc *replyTupleDesc = NULL;       // output row/params in langman format
  ExpTupleDesc *outputTupleDesc = NULL;      // output row/params in executor format
  ExpTupleDesc *childInputTupleDesc = NULL;  // child table result in langman format
  newTdb = NULL;
  const NARoutine *metadata = NULL;
  const NARoutine *effectiveMetadata = NULL;
  Int32 javaDebugPort = 0;
  Int32 javaDebugTimeout = 0;

  OperatorTypeEnum relExprType = relExpr.getOperatorType();
  NABoolean isResultSet = (relExprType == REL_SP_PROXY ? TRUE : FALSE);
  
  const ULng32 numInValues = (inVids ? inVids->entries() : 0);
  const ULng32 numOutValues = (outVids ? outVids->entries() : 0);
  
  ULng32 totalNumParams = (rdesc ? 
                               (rdesc->getInParamColumnList().entries() +
                                rdesc->getOutputColumnList().entries()) : 
                               (numInValues + numOutValues));
  if (relExpr.castToTableMappingUDF())
    totalNumParams = (numInValues + numOutValues);

  // If we have a PhysicalSPProxyFunc, using the udr_codegen() method, the
  // rdesc will be NULL since it does not have any metadata to go with it.

  if (rdesc)
  {
     metadata = rdesc->getNARoutine();

       // if this is a universal function this returns the NARoutine 
       // for the action otherwise it returns the metadata for the 
       // parent function.
     effectiveMetadata = rdesc->getEffectiveNARoutine();  
    
     // Make sure we actually have something to work with.
     CMPASSERT(metadata);
     CMPASSERT(effectiveMetadata);
  }
  
  // Sanity checks on input arguments
  if (numInValues > 0)
  {
    GenAssert(inVids, "Input ValueId list is required");
    GenAssert(inVids->entries() == numInValues,
              "Input ValueId list contains wrong number of elements");
    GenAssert(inColumns, "Input column array is required");
    GenAssert(inColumns->entries() >= numInValues,
              "Input column array has to few elements");
  }
  
  if (numOutValues > 0)
  {
    GenAssert(outVids, "Output ValueId list is required");
    GenAssert(outVids->entries() == numOutValues,
              "Output ValueId list contains wrong number of elements");
    GenAssert(outColumns, "Output column array is required");
    GenAssert(outColumns->entries() == numOutValues,
              "Output column array contains wrong number of elements");
  }
  
  if (formalColumns)
  {
    GenAssert(formalColumns->entries() >= numInValues,
              "Formal column array has too few elements");
    GenAssert(formalColumns->entries() >= numOutValues,
              "Formal column array has too few elements");
    if (effectiveMetadata)
    {
      GenAssert(formalColumns->entries() ==
                (ComUInt32) (effectiveMetadata->getParamCount()),
                "Formal column array contains wrong number of elements");
    } 
  }
  
  //----------------------------------------------------------------------
  //
  // Returned ATP layout:
  //
  //   |-------------------------------|
  //   |  Input data   |  Output data  |
  //   |  ( N tupps )  |  ( 1 tupp )   |
  //   |-------------------------------|
  //   <--- returned row to parent ---->
  //
  //   Input data:  The ATP input to this node by its parent
  //   Output data: The tupp where output values are placed
  //
  // About the work ATP:
  //
  //   If the operator has any input or output parameters then a work ATP
  //   will be used to copy the input/output values to/from message
  //   buffers.
  // 
  //   The CRI descriptor for the work ATP will have 2, 3, or 4 tupps:
  //     - Constants
  //     - Temps
  //     - OPTIONAL request data
  //     - OPTIONAL reply data
  //
  //   ex_expr::eval() takes two ATPs as input and numbers them 0 and 1. 
  //   When evaluating expressions involving the work ATP, the work ATP 
  //   will always be passed in as ATP number 1.
  // 
  //----------------------------------------------------------------------

  ULng32 numChildInputs = 0;
  ULng32 numChildInputCols = 0;
  TableMappingUDFChildInfo* childInfo = NULL;
  if (relExpr.castToTableMappingUDF())
  {
    numChildInputs = relExpr.getArity();
    if (numChildInputs == 1)
    {
      PhysicalTableMappingUDF * op = (PhysicalTableMappingUDF *) (&relExpr);

      childInfo = op->getChildInfo(0);
      numChildInputCols = childInfo->getOutputs().entries();
      childInput_exprs = new(space)ex_expr*[1];
    }
  }
  
  const Int32 workAtpNumber = 1;
  ex_cri_desc *given_desc = generator->getCriDesc(Generator::DOWN);
  ex_cri_desc *returned_desc = NULL;
  ex_cri_desc *work_cri_desc = NULL;
  
  // Set the returned_desc pointer
  if (numOutValues > 0)
  {
    returned_desc = new (space)
      ex_cri_desc((unsigned short) (given_desc->noTuples() + 1), space);
  }
  else
  {
    returned_desc = given_desc;
  }
  
  // Setup local variables related to the work ATP
  unsigned short numWorkTupps = 0;
  unsigned short requestTuppIndex = 0;
  unsigned short replyTuppIndex = 0;
  unsigned short childInputTuppIndex = 0;
  if ((numInValues + numOutValues) > 0)
  {
    numWorkTupps = 2;
    
    if (numInValues > 0)
    {
      numWorkTupps++;
      requestTuppIndex = 2;
    }
    
    if (numOutValues > 0)
    {
      numWorkTupps++;
      replyTuppIndex = (numInValues > 0 ? 3 : 2);
    }

    if ((numChildInputs == 1)&&(numChildInputCols > 0))
    {
      numWorkTupps++;
      childInputTuppIndex = numWorkTupps - 1 ;
    }

    work_cri_desc = new (space) ex_cri_desc(numWorkTupps, space);
  }
  
  //----------------------------------------------------------------------
  // Process each input value
  //
  // Generate new ValueIds for request values. Request values are the
  // results of copying input values into a message buffer. Type
  // conversions may be necessary if the input type is not supported
  // by the Language Manager
  //----------------------------------------------------------------------
  ComRoutineLanguage routineLanguage =
    (metadata ? metadata->getLanguage() : COM_UNKNOWN_ROUTINE_LANGUAGE);
  ComRoutineParamStyle paramStyle =
    (metadata ? metadata->getParamStyle() : COM_UNKNOWN_ROUTINE_PARAM_STYLE);

  if (numInValues > 0)
  {
    // requestVids represents input parameter values in the message buffer
    ValueIdList requestVids;
    
    for (i = 0; i < numInValues; i++)
    {
      ItemExpr &inputExpr = *((*inVids)[i].getItemExpr());
      const NAType &inputType = (*inVids)[i].getType();
      const NAColumn &c = *((*inColumns)[i]);
      const NAType &formalType = *(c.getType());
      ItemExpr *lmExpr = NULL;
      
      LmExprResult lmResult = CreateLmInputExpr (
        formalType,         // [IN] UDR param type
        inputExpr,          // [IN] Actual input value
        routineLanguage,    // [IN] Routine language
        paramStyle,         // [IN] Parameter style
        cmpContext,         // [IN] Compilation context
        lmExpr              // [OUT] Returned expression
        );
      GenAssert(lmResult == LmExprOK && lmExpr != NULL,
                "Error building expression tree for LM input value");
      
      lmExpr->bindNode(generator->getBindWA());
      requestVids.insert(lmExpr->getValueId());
      
    } // for (i = 0; i < numInValues; i++)
    
    // Generate an expression to move input data into the request
    // buffer. After this call returns, the MapTable has request 
    // values in the work ATP at index requestTuppIndex.
    exp_gen->generateContiguousMoveExpr (
      requestVids,                          // [IN] source ValueIds
      TRUE,                                 // [IN] add convert nodes?
      workAtpNumber,                        // [IN] target atp number (0 or 1)
      requestTuppIndex,                     // [IN] target tupp index
      ExpTupleDesc::SQLARK_EXPLODED_FORMAT, // [IN] target tuple data format
      requestRowLen,                        // [OUT] target tuple length
      &input_expr,                          // [OUT] move expression
      &requestTupleDesc,                    // [optional OUT] target tuple desc
      ExpTupleDesc::LONG_FORMAT             // [optional IN] target desc format
      );
    
    //
    // Add the tuple descriptor for request values to the work ATP
    //
    work_cri_desc->setTupleDescriptor(requestTuppIndex, requestTupleDesc);

    combinedRequestChildOutputRowLen = requestRowLen;
    
  } // if (numInValues > 0)

  if ((numChildInputs == 1)&&(numChildInputCols > 0))
  {
    // requestVids represents values in the message buffer
    ValueIdList childRequestVids;
    const ValueIdList& childVals = childInfo->getOutputs();

    for (i = 0; i < childVals.entries(); i++)
    {
      ItemExpr &inputExpr = *(childVals[i].getItemExpr());
      const NAType &inputType = childVals[i].getType();
      const NAColumn &c = *(childInfo->getInputTabCols()[i]);
      const NAType &formalType = childVals[i].getType();
      ItemExpr *lmExpr = NULL;
      
      LmExprResult lmResult = CreateLmInputExpr (
        formalType,         // [IN] UDR param type
        inputExpr,          // [IN] Actual input value
        routineLanguage,    // [IN] Routine language
        paramStyle,         // [IN] Parameter style
        cmpContext,         // [IN] Compilation context
        lmExpr              // [OUT] Returned expression
        );
      GenAssert(lmResult == LmExprOK && lmExpr != NULL,
                "Error building expression tree for LM child Input value");
      
      lmExpr->bindNode(generator->getBindWA());
      childRequestVids.insert(lmExpr->getValueId());
      
    } // for (i = 0; i < numInValues; i++)
    
    // Generate an expression to move input data into the request
    // buffer. After this call returns, the MapTable has request 
    // values in the work ATP at index requestTuppIndex.
    exp_gen->generateContiguousMoveExpr (
      childRequestVids,                     // [IN] source ValueIds
      TRUE,                                 // [IN] add convert nodes?
      workAtpNumber,                        // [IN] target atp number (0 or 1)
      childInputTuppIndex,                  // [IN] target tupp index
      ExpTupleDesc::SQLARK_EXPLODED_FORMAT, // [IN] target tuple data format
      childInputRowLen,                     // [OUT] target tuple length
      &childInput_exprs[0],                     // [OUT] move expression
      &childInputTupleDesc,                 // [optional OUT] target tuple desc
      ExpTupleDesc::LONG_FORMAT             // [optional IN] target desc format
      );
    
    //
    // Add the tuple descriptor for request values to the work ATP
    //
    work_cri_desc->setTupleDescriptor(childInputTuppIndex, childInputTupleDesc);

    combinedRequestChildOutputRowLen =
      MAXOF(combinedRequestChildOutputRowLen, childInputRowLen);
    
  } // if ((numChildInputs == 1)&&(numChildInputCols > 0))
  
  //----------------------------------------------------------------------
  // Process each output value
  //
  // Generate new ValueIds for reply values. Reply values are 
  // values that arrive in a message buffer. We will end up making a
  // copy of the reply values and putting those values in our output
  // row. The results of that copy are considered "output" values and
  // our binder already created ValueIds for the output values. When
  // copying reply values to the output row type conversions may be 
  // necessary if the formal parameter type is not supported by the 
  // Language Manager and/or UDR server.
  //----------------------------------------------------------------------
  if (numOutValues > 0)
  {
    // Create two new ValueId lists
    // - Reply values that came back from the server
    // - Output values that will be returned to the parent.
    //   This list contains cast nodes that will copy reply
    //   values and perform any LM-required type conversions.
    //   Note that ValueIds for our output row were already
    //   created during binding. The cast nodes we introduce
    //   here will have different ValueIds. We will later 
    //   account for this when we generate the move expression
    //   by making sure that the output ValueIds created during
    //   binding refer to the outputs of the move expression
    
    ValueIdList replyVids;
    ValueIdList tempOutputVids;
    
    for (i = 0; i < numOutValues; i++)
    {
      const NAColumn &c = *((*outColumns)[i]);
      const NAType &formalType = *(c.getType());
      ItemExpr *replyValue = NULL;
      ItemExpr *outputValue = NULL;
      
      LmExprResult lmResult = CreateLmOutputExpr(
        formalType,         // [IN] UDR param type
        routineLanguage,    // [IN] Routine language
        paramStyle,         // [IN] Parameter style
        cmpContext,         // [IN] Compilation context
        replyValue,         // [OUT] Returned expression for reply value
        outputValue,        // [OUT] Returned expression for output value
        isResultSet
        );
      GenAssert(lmResult == LmExprOK && replyValue != NULL
                && outputValue != NULL,
                "Error building expression tree for LM output value");
      
      replyValue->synthTypeAndValueId();
      replyValue->bindNode(generator->getBindWA());
      replyVids.insert(replyValue->getValueId());
      
      outputValue->bindNode(generator->getBindWA());
      tempOutputVids.insert(outputValue->getValueId());
      
    } // for (i = 0; i < numOutValues; i++)
    
    // Add reply columns to the MapTable. After this call the MapTable
    // has reply values in the work ATP at index replyTuppIndex.
    exp_gen->processValIdList(
      replyVids,                             // [IN] ValueIdList
      ExpTupleDesc::SQLARK_EXPLODED_FORMAT,  // [IN] tuple data format
      replyRowLen,                           // [OUT] tuple length 
      workAtpNumber,                         // [IN] atp number
      replyTuppIndex,                        // [IN] index into atp
      &replyTupleDesc,                       // [optional OUT] tuple desc
      ExpTupleDesc::LONG_FORMAT              // [optional IN] desc format
      );
    
    // Add the tuple descriptor for reply values to the work ATP
    work_cri_desc->setTupleDescriptor(replyTuppIndex, replyTupleDesc);
    
    // Generate the expression to move reply values out of the message
    // buffer into an output buffer. After this call returns, the 
    // MapTable has reply values in ATP 0 at the last index.
    exp_gen->generateContiguousMoveExpr(
      tempOutputVids,                       // [IN] source ValueIds
      FALSE,                                // [IN] add convert nodes?
      0,                                    // [IN] target atp number
      returned_desc->noTuples() - 1,        // [IN] target tupp index
      ExpTupleDesc::SQLARK_EXPLODED_FORMAT, // [IN] target tuple format
      outputRowLen,                         // [OUT] target tuple length
      &output_expr,                         // [OUT] move expression
      &outputTupleDesc,                     // [optional OUT] target tuple desc
      ExpTupleDesc::LONG_FORMAT             // [optional IN] target desc format
      );
    
  } // if (numOutValues > 0)
  
  // We can now remove all appended map tables
  generator->removeAll(last_map_table);
  
  // Put declared outputs in the MapTable. After this call the
  // MapTable has declared output values in ATP 0 at the last index.
  if (numOutValues > 0)
  {
    exp_gen->processValIdList (
      *outVids,                              // [IN] ValueIdList
      ExpTupleDesc::SQLARK_EXPLODED_FORMAT,  // [IN] tuple data format
      outputRowLen,                          // [OUT] tuple length 
      0,                                     // [IN] atp number
      returned_desc->noTuples() - 1,         // [IN] index into atp
      &outputTupleDesc,                      // [optional OUT] tuple desc
      ExpTupleDesc::LONG_FORMAT              // [optional IN] tuple desc format
      );
  }
  
  // Generate an expression object for the selection predicate if one
  // is required
  if (!(relExpr.selectionPred().isEmpty()))
  {
    ItemExpr *pred = relExpr.selectionPred().rebuildExprTree(ITM_AND,
                                                             TRUE,
                                                             TRUE);
    exp_gen->generateExpr(pred->getValueId(),
                          ex_expr::exp_SCAN_PRED,
                          &scan_expr);
  }
  
  //----------------------------------------------------------------------
  // *** DONE WITH MAP TABLES AND GENERATING EXPRESSIONS ***
  //----------------------------------------------------------------------
  
  // Now we can start preparing data that goes in the TDB.
  
  // Make sure all buffer sizes are large enough to accomodate two
  // control rows plus one data row.
  const ULng32 bufferPad = min_sql_buffer_overhead();
  // request buffer is used to sent parent down queue entries to
  // udrserver, and also to send child output rows to udrserver
  requestBufferSize = MAXOF(requestBufferSize, combinedRequestChildOutputRowLen + bufferPad);
  // reply buffer is used to send UDF result rows back to the executor
  replyBufferSize = MAXOF(replyBufferSize, replyRowLen + bufferPad);
  // output buffer is used to store result rows that are inserted
  // in the up queue to the parent
  outputBufferSize = MAXOF(outputBufferSize, outputRowLen + bufferPad);
  
  //
  // Now we start preparing the metadata the will go into the TDB.
  // Before we create the TDB we prepare metadata for the routine
  // itself, not for each individual column. After the TDB has been
  // created we initialize column metadata.
  //
  char *sqlName = NULL;
  char *routineEntryName = NULL;
  char *sig = NULL;
  char *container = NULL;
  char *path = NULL;
  char *librarySqlName = NULL;
  char *runtimeOptions = NULL;
  char *runtimeOptionDelimiters = NULL;
  ComRoutineType rtype = COM_UNKNOWN_ROUTINE_TYPE;
  ComRoutineSQLAccess sqlmode = COM_UNKNOWN_ROUTINE_SQL_ACCESS;
  ComRoutineParamStyle pstyle = COM_UNKNOWN_ROUTINE_PARAM_STYLE;
  ComRoutineExternalSecurity extSecurity = COM_ROUTINE_EXTERNAL_SECURITY_INVOKER;
  Int32 ownerId = NA_UserIdDefault;
  ComSInt32 maxrs = 0;
  ComSInt32 statesize = 0;
  UInt32 udrFlags = 0;
  NABoolean isUUDF = ((rdesc == NULL) ? FALSE : rdesc->isUUDFRoutine());
  
  if (relExprType == REL_SP_PROXY)
    udrFlags |= UDR_RESULT_SET;
  else if (relExpr.castToTableMappingUDF())
  {
    udrFlags |= UDR_TMUDF;
  }

  // This variable will inform the generator if our fragment needs a
  // transaction. In the debug build for test purposes an environment
  // variable allows UDRs to not require a transaction.
  //
  // Note: SP proxy plans execute in the transaction of their owning
  // CALL statement. The proxy fragment does not require a
  // transaction. If we are compiling a proxy plan, the metadata
  // pointer will be NULL and we set txAttrs to
  // COM_NO_TRANSACTION_REQUIRED.
  ComRoutineTransactionAttributes txAttrs = COM_NO_TRANSACTION_REQUIRED;

  if (metadata && effectiveMetadata )
  {
    // getSqlName will always return the name of the Function regardless 
    // which NARoutine we access, so we will always get the original 
    // Function name here and not the one of the Action.

    txAttrs = effectiveMetadata->getTxAttrs();

    const QualifiedName &qname = metadata->getSqlName();
    sqlName = AllocStringInSpace(*space, qname.getQualifiedNameAsAnsiString());

    // For now the parent function Metadata is used to to initiate the 
    // call. It is unclear yet how the Actions metadata should alter this
    // if different... For now we use all the parents info!! XXX
    if ( relExprType != REL_ISOLATED_SCALAR_UDF )
    {
       routineEntryName = AllocStringInSpace(*space, metadata->getMethodName());
    }
    else 
    {
       routineEntryName = AllocStringInSpace(*space, metadata->getDllEntryPoint());
    }

    // Class name for Java, unqualified DLL name for C/C++
    pstyle = metadata->getParamStyle();
    if (pstyle == COM_STYLE_JAVA_OBJ || pstyle == COM_STYLE_CPP_OBJ)
      container = AllocStringInSpace(*space, metadata->getContainerName());
    else
      container = AllocStringInSpace(*space, metadata->getFile());
    // Fully qualified jar file for Java, directory of DLL for C/C++
    path = AllocStringInSpace(*space, metadata->getExternalPath());
    sig = AllocStringInSpace(*space, metadata->getSignature());

    const ComObjectName &libName = metadata->getLibrarySqlName();
    librarySqlName = AllocStringInSpace(*space, libName.getExternalName());
    
    rtype = metadata->getRoutineType();
    sqlmode = metadata->getSqlAccess();

    extSecurity = metadata->getExternalSecurity();
    ownerId = metadata->getObjectOwner();

    maxrs = metadata->getMaxResults();
    statesize = metadata->getStateAreaSize();
    
    if (metadata->isDeterministic())
      udrFlags |= UDR_DETERMINISTIC;
    if (metadata->isIsolate())
      udrFlags |= UDR_ISOLATE;
    if (metadata->isCallOnNull())
      udrFlags |= UDR_CALL_ON_NULL;
    if (metadata->isExtraCall())
      udrFlags |= UDR_EXTRA_CALL;

    if (metadata->getLanguage() == COM_LANGUAGE_JAVA)
      {
        javaDebugPort = relExpr.getDefault(UDR_JVM_DEBUG_PORT);
        if (javaDebugPort != 0)
          {
#ifndef _DEBUG
            // in a release build, only DB__ROOT can debug the JVM
            if (!ComUser::isRootUserID())
              {
                // a user other than DB__ROOT is trying to debug
                // a UDR, don't allow it and issue a warning
                javaDebugPort = 0;
                *(CmpCommon::diags()) << DgSqlCode(1260);
              }
            else
#endif
              javaDebugTimeout = relExpr.getDefault(UDR_JVM_DEBUG_TIMEOUT);
          }
      }
  }  // if (metadata && effectiveMetadata) 

#ifdef _DEBUG
  const char *value = getenv("UDR_GEN_NO_TX_REQD");
  if (value && value[0] != 0 && value[0] != '0')
    txAttrs = COM_NO_TRANSACTION_REQUIRED;
#endif
  if (runtimeOptsFromCaller)
  {
    GenAssert(runtimeOptDelimsFromCaller,
              "Invalid runtime option delimiter string passed in");
    runtimeOptions = AllocStringInSpace(*space, runtimeOptsFromCaller);
    runtimeOptionDelimiters = AllocStringInSpace(*space,
                                                 runtimeOptDelimsFromCaller);
  }

  Int32 udrSerInvocationInfoLen = 0;
  char *udrSerInvocationInfo = NULL;
  Int32 udrSerPlanInfoLen = 0;
  char *udrSerPlanInfo = NULL;

  try
    {
      int tempLen = 0;
      char *tempBuffer = NULL;

      if (udrInvocationInfo)
        {
          ExpTupleDesc *inputTupleDescs[2];

          GenAssert(udrInvocationInfo->getNumTableInputs() <= 1,
                    "this method is not yet ready for multiple input tables");
          if (udrInvocationInfo->getNumTableInputs() == 1)
            inputTupleDescs[0] = childInputTupleDesc;

          // store the computed offsets and record lengths
          // in the UDRInvocationInfo, to be used by the UDR
          TMUDFInternalSetup::setOffsets(udrInvocationInfo,
                                         requestTupleDesc,
                                         replyTupleDesc,
                                         inputTupleDescs);
          
          // Serialize the UDRInvocationInfo object, so that we
          // can pass it to the UDR at runtime. This could be in
          // another process (e.g. ESP, tdm_udrserv, or even to
          // a Java program.
          udrSerInvocationInfoLen = tempLen = udrInvocationInfo->serializedLength();
          udrSerInvocationInfo = tempBuffer = space->allocateAlignedSpace(tempLen);
          udrInvocationInfo->serialize(tempBuffer, tempLen);
        } 
 
      if (udrPlanInfo)
        {
          // same for UDRPlanInfo
          udrSerPlanInfoLen = tempLen = udrPlanInfo->serializedLength();
          udrSerPlanInfo = tempBuffer = space->allocateAlignedSpace(tempLen);
          udrPlanInfo->serialize(tempBuffer, tempLen);
        } 
    }
  catch (tmudr::UDRException e)
    {
      TMUDFDllInteraction::processReturnStatus(
           e,
           (udrInvocationInfo ?
            udrInvocationInfo->getUDRName().data() :
            "unknown"));
    }
 
  // Create a TDB
  ComTdbUdr *tdb = new (space) ComTdbUdr (
    sqlName,
    routineEntryName,
    sig,
    container,
    path,
    librarySqlName,
    
    runtimeOptions,
    runtimeOptionDelimiters,
    
    udrFlags,
    numInValues,
    numOutValues,
    totalNumParams,
    maxrs,
    statesize,
    rtype,
    routineLanguage,
    pstyle,
    sqlmode,
    txAttrs,
    
    extSecurity,
    ownerId,

    estimatedRowCount,
    given_desc,
    returned_desc,
    work_cri_desc,
    downQueueMaxSize,
    upQueueMaxSize,
    
    (Lng32) numOutputBuffers,
    outputBufferSize,
    requestBufferSize,
    replyBufferSize,
    
    input_expr,
    output_expr,
    scan_expr,
    proj_expr,
    requestTuppIndex,
    replyTuppIndex,
    requestRowLen,
    replyRowLen,
    outputRowLen,

    numChildInputs,
    childInput_exprs,
    childTdbs,

    optionalData,

    udrSerInvocationInfoLen,
    udrSerInvocationInfo,
    udrSerPlanInfoLen,
    udrSerPlanInfo,

    javaDebugPort,
    javaDebugTimeout,

    space
    
    );
  
  ComBoolean *objMapping = NULL;
  if (sig)
  {
    // Extract "objMapping" values from the Java method
    // signature. Each value is a boolean flag. We allocate a buffer
    // of flags, one for each parameter and one for the return
    // value. Then we call setLMObjectMapping() to populate the
    // buffer. We ignore error conditions returned from
    // setLMObjectMapping().
    objMapping = new (space) ComBoolean[totalNumParams + 1];
    LmResult outcome = setLMObjectMapping(sig, objMapping);
  }
  
  // Add column metadata to the TDB. If a formal column array was
  // passed in, we use those column descriptions. Otherwise we go in
  // order through the input column array followed by the output
  // column array.
  for (i = 0; i < totalNumParams; i++)
  {
    const NAColumn *c;
    Int16 flags = 0;
    
    if (formalColumns &&
        totalNumParams == formalColumns->entries())
    {
      // formalColumns was passed in and it is expected to contain
      // totalNumParams elements
      c = (*formalColumns)[i];
      
      ComColumnDirection dir = ((NAColumn *)c)->getColumnMode();
      switch (dir)
      {
      case COM_INPUT_COLUMN:
        flags |= UDR_PARAM_IN;
        break;
      case COM_OUTPUT_COLUMN:
        flags |= UDR_PARAM_OUT;
        break;
      case COM_INOUT_COLUMN:
        flags |= UDR_PARAM_IN;
        flags |= UDR_PARAM_OUT;
        break;
      }
    }
    else if (i < numInValues)
    {
      // formalColumns was not passed in. We use the inColumns array
      // first while i is less then numInValues.
      c = (*inColumns)[i];
      flags |= UDR_PARAM_IN;
    }
    else
    {
      // formalColumns was not passed in. We use the outColumns array
      // now that i is greater than or equal to numInValues.
      c = (*outColumns)[i - numInValues];
      flags |= UDR_PARAM_OUT;
    }
    
    GenAssert(c, "Unable to locate column description");
    
    const NAType &t = *(c->getType());
    
    if (t.supportsSQLnullLogical())
      flags |= UDR_PARAM_NULLABLE;
    
    if (objMapping && objMapping[i])
      flags |= UDR_PARAM_LM_OBJ_TYPE;
    
    // The type information that we want at runtime is not exactly
    // what the NAType is storing in its fields. Here we ask the
    // expression generator to convert the NAType to an equivalent
    // Attributes object, then put relevant info from the Attributes
    // object into a UdrFormalParamInfo object which gets written into
    // the plan.
    Attributes *attr =
      exp_gen->convertNATypeToAttributes(t, generator->wHeap());
    
    Int16 type = attr->getDatatype();

    // $$$$ It's not clear why we need to know whether the LM type
    // supports scale and precision. Perhaps in the future we could
    // simply copy scale and precision from the Attributes instance
    // into the plan unconditionally without asking LM. If we do that,
    // then the LmTypeSupportsScale/Precision functions could probably
    // be removed.
    Int16 prec = (Int16) (LmTypeSupportsPrecision(t) ?
                          attr->getPrecision() : 0);
    Int16 scale = (Int16) (LmTypeSupportsScale(t) ? attr->getScale() : 0);

    Int16 encodingCharSet = (Int16) CharInfo::UnknownCharSet;
    Int16 collation = (Int16) CharInfo::UNKNOWN_COLLATION;

    if (t.getTypeQualifier() == NA_CHARACTER_TYPE)
    {
      const CharType &chType = * ((CharType*) c->getType());
      encodingCharSet = (Int16) chType.getEncodingCharSet();
      collation = (Int16) chType.getCollation();
    }
    
    char *paramName = AllocStringInSpace(*space, c->getColName().data());

    UdrFormalParamInfo *info = new (space) 
      UdrFormalParamInfo(type, prec, scale, flags,
                         encodingCharSet, collation, paramName);
    
    tdb->setFormalParamInfo(i, info);
    
  } // for (i = 0; i < totalNumParams; i++)


  // Add child Input Table Info to the TDB. 
  if (numChildInputs == 1)
  {
    const NAColumnArray& childInputCols = childInfo->getInputTabCols();
    UdrColumnDescInfoPtr* childColInfo =  (UdrColumnDescInfoPtr *)
      (space->allocateAlignedSpace(numChildInputCols * sizeof(UdrColumnDescInfoPtr)));

    for (i = 0; i < numChildInputCols; i++)
    {
      const NAColumn *c;
      Int16 flags = 0;
     
      c = childInputCols[i];
      const NAType &t = *(c->getType());
      
      if (t.supportsSQLnullLogical())
        flags |= UDR_PARAM_NULLABLE;

      flags |= UDR_PARAM_IN;
      
      // The type information that we want at runtime is not exactly
      // what the NAType is storing in its fields. Here we ask the
      // expression generator to convert the NAType to an equivalent
      // Attributes object, then put relevant info from the Attributes
      // object into a UdrFormalParamInfo object which gets written into
      // the plan.
      Attributes *attr =
        exp_gen->convertNATypeToAttributes(t, generator->wHeap());
      
      Int16 type = attr->getDatatype();

      // $$$$ It's not clear why we need to know whether the LM type
      // supports scale and precision. Perhaps in the future we could
      // simply copy scale and precision from the Attributes instance
      // into the plan unconditionally without asking LM. If we do that,
      // then the LmTypeSupportsScale/Precision functions could probably
      // be removed.
      Int16 prec = (Int16) (LmTypeSupportsPrecision(t) ?
                            attr->getPrecision() : 0);
      Int16 scale = (Int16) (LmTypeSupportsScale(t) ? attr->getScale() : 0);

      Int16 encodingCharSet = (Int16) CharInfo::UnknownCharSet;
      Int16 collation = (Int16) CharInfo::UNKNOWN_COLLATION;

      if (t.getTypeQualifier() == NA_CHARACTER_TYPE)
      {
        const CharType &chType = * ((CharType*) c->getType());
        encodingCharSet = (Int16) chType.getEncodingCharSet();
        collation = (Int16) chType.getCollation();
      }
      
      char *paramName = AllocStringInSpace(*space, c->getColName().data());

      UdrColumnDescInfo *info = new (space) 
        UdrColumnDescInfo(type, prec, scale, flags,
                          encodingCharSet, collation, paramName);
      childColInfo[i] = info;   
    } // for (i = 0; i < numChildInputCols; i++)

    char *childName = 
      AllocStringInSpace(*space, childInfo->getInputTabName().data());

    UdrTableDescInfo * childTabDescInfo = new (space)
      UdrTableDescInfo(childName, (UInt16)numChildInputCols, childInputRowLen,
                       childInputTuppIndex, childColInfo);
    tdb->setTableDescInfo(0,childTabDescInfo);
  }
  
  generator->initTdbFields(tdb);
  
  // Generate EXPLAIN info.
  if ((!generator->explainDisabled())&&(numChildInputs == 0)) 
  {
    generator->setExplainTuple(relExpr.addExplainInfo(tdb, 0, 0, generator));
  }
  
  // Tell the generator about our in/out rows and the new TDB
  generator->setCriDesc(given_desc, Generator::DOWN);
  generator->setCriDesc(returned_desc, Generator::UP);
  generator->setGenObj(&relExpr, tdb);

  if (txAttrs == COM_TRANSACTION_REQUIRED)
  {
    generator->setTransactionFlag(-1);
    // Do not infer that any transaction started can be in READ ONLY
    // mode if UDRs are present.
    generator->setNeedsReadWriteTransaction(TRUE);
  }
  else
  {
    generator->setTransactionFlag(0);
  }

  // Return a TDB pointer to the caller
  newTdb = tdb;
  
  return 0;
  
} // udr_codegen()

short
IsolatedScalarUDF::codeGen(Generator *generator)
{
  short result = 0;
  Space *space = generator->getSpace();

  // Get the Parent routine metadata via the routineDesc
  const NARoutine &metadata = *(getNARoutine());

  // Get the Action routine metadata via the routineDesc
  const NARoutine &actionMetadata = *(getActionNARoutine());

  // Make sure we have a routineDesc.
  CMPASSERT(getRoutineDesc());

  // Get the effective metadata. This will return the same pointer
  // as above unless if we are a UUDF in which case the effective 
  // metadata is that of the action. So in the case of a non UUDF routine
  // the actionMetadata and the metadata references point to the same thing.
  const NARoutine &effectiveMetadata =
    *(getRoutineDesc()->getEffectiveNARoutine());

  // Get the list of inputs ValueIds that the UDF needs.
  const ValueIdList &inVids = getProcInputParamsVids();
  // Get the list of outputs ValueIds that the UDF produces.
  // for scalar, this should always be one, even if we produce a 
  // Multi Valued output (it will be implemented as single Tuple/SqlRecord.
  const ValueIdList &outVids = getProcOutParams();

  // Do we have a Universal Routine?
  NABoolean isUUDF = getRoutineDesc()->isUUDFRoutine();

  // These are the Formal Input Columns from Metadata
  const NAColumnArray &inColumns = effectiveMetadata.getInParams();

  // These are the Formal Output Columns from Metadata
  const NAColumnArray &outColumns =  effectiveMetadata.getOutParams();
 
  
  // These are All the Formal Columns from Metadata
  const NAColumnArray &formalColumns = effectiveMetadata.getParams();

  const ULng32 downQueueMaxSize = getDefault(GEN_UDR_SIZE_DOWN);
  const ULng32 upQueueMaxSize = getDefault(GEN_UDR_SIZE_UP);

  // We pass in buffer sizes of zero and let the udr_codegen
  // subroutine choose minimal but sufficient buffer sizes for
  // single-row interactions. When UDR operators are used for
  // multi-row interactions then we should choose larger buffer sizes.
  const ULng32 defaultBufferSize = getDefault(GEN_UDR_BUFFER_SIZE);
  const ULng32 outputBufferSize = defaultBufferSize;
  const ULng32 requestBufferSize = defaultBufferSize;
  const ULng32 replyBufferSize = defaultBufferSize;


  // We could get by with one buffer in the output pool for standalone
  // CALL statements because they always return one row. However by
  // default we will allocate at least two buffers in the output pool
  // due to some poor logic in the TCB work method. In the non-error
  // case for a CALL, the reply buffer coming back from the UDR Server
  // has two entries, data and end-of-data. The way the work method is
  // currently written, before looking at a reply buffer entry a row
  // is allocated from the output pool. If the reply entry is
  // end-of-data then the output row is not needed and will be
  // recycled. It would be better if the TCB work method did not need
  // to pre-allocate an output row before processing end-of-data. To
  // avoid the pool being blocked for these end-of-data
  // pre-allocations we will create the output pool with 2 buffers.
  //
  // The default value of GEN_UDR_NUM_BUFFERS is 2.
  //
  const ULng32 numOutputBuffers = getDefault(GEN_UDR_NUM_BUFFERS);

  // The TDB stores but makes no use of this estimated row count
  Cardinality estimatedRowCount = 1;


  // Create the collection of optional strings

  Int32 dataEntries = (Int32) effectiveMetadata.getDataNumEntries();

  Int32 passThruIndex = 0;  // SMD metadata tables

  if (CmpCommon::getDefault(COMP_BOOL_191) == DF_ON)
    passThruIndex = 1;    // user metadata tables
  
  Queue *optionalDataQ = new (space) Queue(space);
  const char *optionalBuf = NULL;

  // Provide pass-through inputs for UUDFs.  UUDFs can have any name.
  if (getRoutineDesc()->isUUDFRoutine())
  {
    // [0] Null-terminated action name (for SAS, format or model name).
    //
    // For now, the first pass-through input for a universal function is always the 
    // action name. Eventually we should make the code consistent with the ES
    // which says the action name is provided in the UDRINFO structure.
    //
    // For SAS_SCORE if the model name is annotated with an optional out
    // parameter name, the RelExpr stores the annotated name and the
    // SasRoutine stores the simple name. SAS wants the simple name at
    // runtime. (Might be better in the future if the RelExpr stored
    // both forms of the name.) If this is not a SAS_SCORE operator we
    // always use the name stored in the RelExpr.

    // This name has to be from syntax, it cannot be the name
    // stored in metadata. Sas wants BEST9. to be passed through, 
    // not just BEST!
    // We store the syntax name in the RoutineDesc
    NAString s0;
    if (getRoutineDesc())
      s0 = getRoutineDesc()->getActionNameAsGiven();
    else
      return -1;

    TrimNAStringSpace(s0);
    s0.toUpper();
    optionalBuf = AllocDataInSpace(*space, s0, s0.length() + 1);
    optionalDataQ->insert(optionalBuf);
    
    NAString rName(getRoutineName().getObjectName());
    rName.toUpper();
    // SAS_SCORE and SAS_PUT actions have special handling that is performed here.
    if (rName == "SAS_SCORE" || rName == "SAS_PUT")
    {
      // [1] The SAS XML string (not null-terminated)
      if (CmpCommon::getDefault(COMP_BOOL_191) == DF_OFF)
        passThruIndex = 1; // SMD metadata tables
      else
        passThruIndex = 2; // user metadata tables
      
      Int32 index = passThruIndex - 1;
  
      if (dataEntries >= index+1 && (effectiveMetadata.getDataSize(index) != 0))
      {
        optionalBuf =
          AllocDataInSpace(*space, effectiveMetadata.getData(index), 
                           (UInt32) effectiveMetadata.getDataSize(index));
      }
      else
      {
        optionalBuf = AllocDataInSpace(*space, "", 0);
      }
  
      optionalDataQ->insert(optionalBuf);
      
      if (rName == "SAS_SCORE")
      {
        // [2] Null-terminated model DLL name
        // [3] Null-terminated model DLL entry point
        const NAString &s2 = effectiveMetadata.getDllName();
        optionalBuf = AllocDataInSpace(*space, s2, s2.length() + 1);
        optionalDataQ->insert(optionalBuf);
        
        const NAString &s3 = effectiveMetadata.getDllEntryPoint();
        optionalBuf = AllocDataInSpace(*space, s3, s3.length() + 1);
        optionalDataQ->insert(optionalBuf);
      }
  
      else // SAS_PUT
      {
        // [2] Provide locale as a pass-through input. For now we
        // will always provide a value of 0.
        ComSInt32 locale = getRoutineDesc()->getLocale();
        optionalBuf = AllocDataInSpace(*space, (char *) &locale, 4);
        optionalDataQ->insert(optionalBuf);
      }
  
    } // SAS_PUT or SAS_SCORE
  } // Is UUDF

  // Get pass-through inputs from the effective metadata. Initial
  // elements in the pass-through collection might have been processed
  // above in the code block for SAS functions. So we don't start
  // processing with the first element here. The variable
  // passThruIndex tells us the starting index.

  // NOTES ABOUT PASS-THROUGH INPUTS IN UDF METADATA
  // 1. Pre-v2500 scheme where metadata is in user tables:
  //    * If the user registered pass-through inputs, say N of them,
  //      the SUB_ID values in TEXT should be 1 through N. The value
  //      of dataEntries will be N. We need to call getData(i) where
  //      i ranges from 1 to dataEntries.
  //    * In the case of SAS functions, element 1 has already been
  //      written into the plan so passThruIndex should be set to 2.
  //    * Otherwise passThruIndex should be 1.
  // 2. V2500 scheme where metadata is in SMD tables:
  //    * SUB_ID values start at 1001 and are not really meaningful.
  //    * If the user registered pass-through inputs, say N of them,
  //      the SUB_ID values in TEXT should be 1001 through 1000+N. The
  //      value of dataEntries will be N. We need to call getData(i)
  //      where i ranges from 0 to dataEntries-1.
  //    * In the case of SAS functions, element 0 has already been
  //      written into the plan so passThruIndex should be set to 1.
  //    * Otherwise passThruIndex should be 0.
  //
  // Ideally we would handle these differences earlier in the
  // compiler, perhaps in catman or in the binder but that hasn't been
  // done.

  for (Int32 i = passThruIndex; i < dataEntries; i++)
  {
    Int64 dataSize = effectiveMetadata.getDataSize(i);
    if (dataSize < 0)
      dataSize = 0;

    const char *data = "";
    if (dataSize > 0)
      data = effectiveMetadata.getData(i);

    optionalBuf = AllocDataInSpace(*space, data, (UInt32) dataSize);
    optionalDataQ->insert(optionalBuf);
  }

  // The udr_codegen() subroutine will set this variable to point to
  // the newly created TDB
  ComTdbUdr *newTdb = NULL;

  result = udr_codegen(generator,
                           *this,              // RelExpr &relExpr
                           newTdb,             // ComTdbUdr *&newTdb
                           getRoutineDesc(),   // Pointer to the RoutineDesc
                           &inVids,            // ValueIdList *inVids
                           &outVids,           // ValueIdList *outVids
                           &inColumns,         // NAColumnArray *inColumns
                           &outColumns,        // NAColumnArray *outColumns
                           &formalColumns,     // NAColumnArray *formalColumns
                           estimatedRowCount,
                           downQueueMaxSize,
                           upQueueMaxSize,
                           outputBufferSize,
                           requestBufferSize,
                           replyBufferSize,
                           numOutputBuffers,
                           NULL,               // const char *runtimeOpts
                           NULL,               // const char *runtimeOptDelims
                           NULL,               // TMUDF only
                           optionalDataQ,
                           NULL,               // TMUDF only
                           NULL);              // TMUDF only
  
  if (effectiveMetadata.getRoutineID() > 0)
    generator->objectUids().insert(effectiveMetadata.getRoutineID());

  return result;
}

short CallSP::codeGen(Generator *generator)
{
  short result = 0;
  const NARoutine &metadata = *(getNARoutine());

  const ValueIdList &inVids = getProcInputParamsVids();
  const ValueIdList &outVids = getProcOutputParamsVids();

  const NAColumnArray &inColumns = metadata.getInParams();
  const NAColumnArray &outColumns = metadata.getOutParams();
  const NAColumnArray &formalColumns = metadata.getParams();

  const ULng32 downQueueMaxSize = getDefault(GEN_UDR_SIZE_DOWN);
  const ULng32 upQueueMaxSize = getDefault(GEN_UDR_SIZE_UP);

  // We pass in buffer sizes of zero and let the udr_codegen
  // subroutine choose minimal but sufficient buffer sizes for
  // single-row interactions. When UDR operators are used for
  // multi-row interactions then we should choose larger buffer sizes.
  const ULng32 outputBufferSize = 0;
  const ULng32 requestBufferSize = 0;
  const ULng32 replyBufferSize = 0;

  // We could get by with one buffer in the output pool for standalone
  // CALL statements because they always return one row. However by
  // default we will allocate at least two buffers in the output pool
  // due to some poor logic in the TCB work method. In the non-error
  // case for a CALL, the reply buffer coming back from the UDR Server
  // has two entries, data and end-of-data. The way the work method is
  // currently written, before looking at a reply buffer entry a row
  // is allocated from the output pool. If the reply entry is
  // end-of-data then the output row is not needed and will be
  // recycled. It would be better if the TCB work method did not need
  // to pre-allocate an output row before processing end-of-data. To
  // avoid the pool being blocked for these end-of-data
  // pre-allocations we will create the output pool with 2 buffers.
  //
  // The default value of GEN_UDR_NUM_BUFFERS is 2.
  //
  const ULng32 numOutputBuffers = getDefault(GEN_UDR_NUM_BUFFERS);

  // Look up defaults for JVM startup options. The third parameter to
  // getDefault() is named "errOrWarn" and is explained in commentary
  // in sqlcomp/nadefaults.h. We set it to zero so that lookup
  // failures do not emit errors or warnings.
  NAString opts, delims;
  CmpCommon::getDefault(UDR_JAVA_OPTIONS, opts, 0);
  CmpCommon::getDefault(UDR_JAVA_OPTION_DELIMITERS, delims, 0);
  if (opts.compareTo("OFF", NAString::ignoreCase) == 0)
    opts.toUpper();
  else if (opts.compareTo("ANYTHING", NAString::ignoreCase) == 0)
    opts.toUpper();

  // As the TDB class is enhanced in the future, its collection of
  // data buffers can be used for specific purposes. For example, to
  // support SAS functions we are storing SAS-specific information in
  // the buffer collection. There is no limit on the number of buffers
  // we can store or on the size of a given buffer. Each buffer gets
  // written into the plan.
  Queue *optionalData = NULL;

  // The TDB stores but makes no use of this estimated row count
  Cardinality estimatedRowCount = 1;

  // The udr_codegen() subroutine will set this variable to point to
  // the newly created TDB
  ComTdbUdr *newTdb = NULL;

  result = udr_codegen(generator,
                       *this,              // RelExpr &relExpr
                       newTdb,             // ComTdbUdr *&newTdb
                       getRoutineDesc(),   // NARoutine *metadata
                       &inVids,            // ValueIdList *inVids
                       &outVids,           // ValueIdList *outVids
                       &inColumns,         // NAColumnArray *inColumns
                       &outColumns,        // NAColumnArray *outColumns
                       &formalColumns,     // NAColumnArray *formalColumns
                       estimatedRowCount,
                       downQueueMaxSize,
                       upQueueMaxSize,
                       outputBufferSize,
                       requestBufferSize,
                       replyBufferSize,
                       numOutputBuffers,
                       opts.data(),
                       delims.data(),
                       NULL,                // TMUDF only
                       optionalData,
                       NULL,                // TMUDF only
                       NULL);               // TMUDF only

  if (metadata.getRoutineID() > 0)
    generator->objectUids().insert(metadata.getRoutineID());

  return result;

} // CallSP::codeGen()


short PhysicalSPProxyFunc::codeGen(Generator *generator)
{
  short result = 0;
  ComUInt32 i = 0;

  Space *space = generator->getSpace();

  const ValueIdList &outVids = getTableDesc()->getColumnList();
  const NAColumnArray &outColumns =
    getTableDesc()->getNATable()->getNAColumnArray();

  const ULng32 requestBufferSize = 0;
  const ULng32 replyBufferSize = getDefault(GEN_UDRRS_BUFFER_SIZE);
  const ULng32 outputBufferSize = replyBufferSize;

  const ULng32 downQueueMaxSize = getDefault(GEN_UDRRS_SIZE_DOWN);
  const ULng32 upQueueMaxSize = getDefault(GEN_UDRRS_SIZE_UP);

  const ULng32 numOutputBuffers = getDefault(GEN_UDRRS_NUM_BUFFERS);

  // The TDB stores but makes no use of this estimated row count
  Cardinality estimatedRowCount = 100;

  // Create the collection of optional strings and pass it to
  // udr_codegen(). udr_codegen() currently does not have logic to
  // write these strings into the plan.
  Queue *optionalData = NULL;
  ComUInt32 numStrings = getNumOptionalStrings();
  if (numStrings > 0)
  {
    optionalData = new (space) Queue(space);
    for (i = 0; i < numStrings; i++)
    {
      const char *s = getOptionalString(i);
      if (s)
      {
        const char *s2 = AllocDataInSpace(*space, s, str_len(s));
        optionalData->insert(s2);
      }
    }
  }

  // The udr_codegen() subroutine will set this variable to point to
  // the newly created TDB
  ComTdbUdr *newTdb = NULL;

  result = udr_codegen(generator,
                       *this,              // RelExpr &relExpr
                       newTdb,             // ComTdbUdr *&newTdb
                       NULL,               // RoutineDesc *rdesc
                       NULL,               // ValueIdList *inVids
                       &outVids,           // ValueIdList *outVids
                       NULL,               // NAColumnArray *inColumns
                       &outColumns,        // NAColumnArray *outColumns
                       NULL,               // NAColumnArray *formalColumns
                       estimatedRowCount,
                       downQueueMaxSize,
                       upQueueMaxSize,
                       outputBufferSize,
                       requestBufferSize,
                       replyBufferSize,
                       numOutputBuffers,
                       NULL,               // const char *runtimeOpts
                       NULL,               // const char *runtimeOptDelims
                       NULL,               // TMUDF only
                       optionalData,
                       NULL,               // TMUDF only
                       NULL);              // TMUDF only

  return result;

} // PhysicalSPProxyFunc::codeGen()


  ////////////////////////////////////////////////////////////////////////////
  //
  // Returned atp layout:
  //
  // |------------------------------|
  // | input data  |  sql table row |
  // | ( I tupps ) |  ( 1 tupp )    |
  // |------------------------------|
  // <-- returned row to parent ---->
  //
  // input data:        the atp input to this node by its parent.
  // sql table row:     tupp where the row read from sql table is moved.
  //
  // Input to child:    I tupps
  //
  ////////////////////////////////////////////////////////////////////////////

short
PhysicalTableMappingUDF::codeGen(Generator *generator)
{

  short result = 0;
  Space *space = generator->getSpace();

  // Get the Parent routine metadata via the routineDesc
  const NARoutine &metadata = *(getNARoutine());

  // Get the list of inputs ValueIds that the UDF needs.
  const ValueIdList &inVids = getProcInputParamsVids();

  // Get the list of outputs ValueIds that the UDF produces.
  const ValueIdList &outVids = getProcOutputParamsVids();

  // These are the Formal Input Columns from DLL/Metadata
  const NAColumnArray &inColumns = getScalarInputParams();

  // These are the Formal Output Columns from DLL/Metadata
  const NAColumnArray &outColumns =  getOutputParams();

  const ULng32 downQueueMaxSize = getDefault(GEN_UDR_SIZE_DOWN);
  const ULng32 upQueueMaxSize = getDefault(GEN_UDR_SIZE_UP);


  const ULng32 defaultBufferSize = getDefault(GEN_UDR_BUFFER_SIZE);
  const ULng32 outputBufferSize = defaultBufferSize;
  const ULng32 requestBufferSize = defaultBufferSize;
  const ULng32 replyBufferSize = defaultBufferSize;
  const ULng32 numOutputBuffers = getDefault(GEN_UDR_NUM_BUFFERS);

  // The TDB stores but makes no use of this estimated row count
  Cardinality estimatedRowCount = 1;


  // Create the collection of optional strings

  Int32 dataEntries = (Int32) metadata.getDataNumEntries();
  Int32 passThruIndex = 1; // Sub_ID 0 is never sent to the Routine
  Queue *optionalDataQ = new (space) Queue(space);
  const char *optionalBuf = NULL;
  ComUInt32 tmpLen = 0;

  // Provide pass-through input
  // Get the text element from the effective 
  if (dataEntries >= passThruIndex)
  {
    for (Int32 i=passThruIndex; i< dataEntries; i++)
    {
       if (metadata.getDataSize(i) != 0)
       {
          optionalBuf = AllocDataInSpace(*space, metadata.getData(i), 
                                 (UInt32) metadata.getDataSize(i));
       }
       else
          optionalBuf = AllocDataInSpace(*space, "", 0);

       optionalDataQ->insert(optionalBuf);
    }
  }

  Int32 numChildren = getArity();
  ex_cri_desc * givenDesc = generator->getCriDesc(Generator::DOWN);
  ComTdb ** childTdbs = (ComTdb**) new (space) ComTdb*[numChildren];
  ex_cri_desc ** childDescs =
    new (generator->wHeap()) ex_cri_desc* [numChildren];
  ExplainTuple *firstExplainTuple = 0;
  ExplainTuple *secondExplainTuple = 0;

  short shiftIdx = 0;
  short numInputTupps = (short) givenDesc->noTuples();

  for (Int32 i=0; i<numChildren; i++)
  {
    // Allocate a new map table for this child.
    //
    MapTable *localMapTable = generator->appendAtEnd();

    // The input descriptor passed to the TMUDF node is the
    // input descriptor passed to all children.
    generator->setCriDesc(givenDesc, Generator::DOWN);

    // Generate code for child, and record pointer to generated
    // tdb tree and EXPLAIN tuples.  There is currently a
    // limitation that EXPLAIN information is generated for
    // only two children.  This should be fixed at some point,
    // either by generalizing support for EXPLAIN to allow
    // an arbitrary number of children, or generating special
    // EXPLAIN information for a TMUDF that somehow includes
    // information about all children. Note that so far we only
    // support one child. We may support two in the near future,
    // but more than two are very unlikely to ever be supported.
    child(i)->codeGen(generator);
    childTdbs[i] = (ComTdb *)(generator->getGenObj());
    if (i==0)
      firstExplainTuple = generator->getExplainTuple();
    else if (i==1)
      secondExplainTuple = generator->getExplainTuple();
    else
      // fix or just remove the assert if we ever get here
      GenAssert(i < 2, "Explain for TMUDF with > 2 children not supported");
  }

  // The udr_codegen() subroutine will set this variable to point to
  // the newly created TDB
  ComTdbUdr *newTdb = NULL;

  result = udr_codegen(generator,
                           *this,              // RelExpr &relExpr
                           newTdb,             // ComTdbUdr *&newTdb
                           getRoutineDesc(),   // Pointer to the RoutineDesc
                           &inVids,            // ValueIdList *inVids
                           &outVids,           // ValueIdList *outVids
                           &inColumns,         // NAColumnArray *inColumns
                           &outColumns,        // NAColumnArray *outColumns
                           NULL,	       // NAColumnArray *formalColumns
                           estimatedRowCount,
                           downQueueMaxSize,
                           upQueueMaxSize,
                           outputBufferSize,
                           requestBufferSize,
                           replyBufferSize,
                           numOutputBuffers,
                           NULL,               // const char *runtimeOpts
                           NULL,               // const char *runtimeOptDelims
                           childTdbs,          // array of child TDBs (TMUDF only)
                           optionalDataQ,
                           getInvocationInfo(),
                           planInfo_);

  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
         addExplainInfo(newTdb, firstExplainTuple, secondExplainTuple, generator));
  }

  if (metadata.getRoutineID() > 0)
    generator->objectUids().insert(metadata.getRoutineID());

  return result;
}

TrafDesc *TableMappingUDF::createVirtualTableDesc()
{

  Int32 numOutputCols = getNARoutine()->getOutParamCount();
  ComTdbVirtTableColumnInfo* outColsInfo = 
    new (HEAP) ComTdbVirtTableColumnInfo[numOutputCols];

  for (CollIndex i = 0; i < (CollIndex) numOutputCols; i++)
  {
    const NAColumn* outCol = getNARoutine()->getOutParams()[i];
    outColsInfo[i].colName = outCol->getColName().data();
    outColsInfo[i].datatype = outCol->getType()->getFSDatatype();
    outColsInfo[i].length = outCol->getType()->getNominalSize();
    outColsInfo[i].nullable = (Lng32) outCol->getType()->supportsSQLnull();
  }
  TrafDesc * table_desc =
    Generator::createVirtualTableDesc(getRoutineName().getQualifiedNameAsString().data(),
                                      NULL, // let it decide what heap to use
                                      numOutputCols,
				      outColsInfo,
				      0,
				      NULL);
  return table_desc;
}

