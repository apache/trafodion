/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2014 Hewlett-Packard Development Company, L.P.
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
/**********************************************************************/
#ifndef UDFDLLINTERACTION_H
#define UDFDLLINTERACTION_H
/* -*-C++-*-
/**************************************************************************
*
* File:         UdfDllInteraction.h
* Description:  Classes that define interaction of a TMUDF with a DLL
* Created:      3/01/2010
* Language:     C++
*
*************************************************************************
*/

#include "LmCommon.h"
#include "sqludr.h"
#include "BindWA.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------

class TMUDFDllInteraction;

// forward references
class TableMappingUDF;


// if other RelExprs need a Dll interation we may need
// to define a common base class then.
class TMUDFDllInteraction : public NABasicObject
{

  public :
    TMUDFDllInteraction(TableMappingUDF * TMUDFNode);
    NABoolean ValidateScalarInputs(TableMappingUDF * TMUDFNode, BindWA * bindWA);
    NABoolean DescribeMaxOutputs(TableMappingUDF * TMUDFNode, BindWA * bindWA);
    NABoolean DescribeInputsAndOutputs(TableMappingUDF * TMUDFNode);
    NABoolean DescribeInputPartitionAndOrder(TableMappingUDF * TMUDFNode);
    NABoolean PredicatePushDown(TableMappingUDF * TMUDFNode);
    NABoolean Cardinality(TableMappingUDF * TMUDFNode);
    NABoolean Constraints(TableMappingUDF * TMUDFNode);
    NABoolean Cost(TableMappingUDF * TMUDFNode);
    NABoolean DegreeOfParallelism(TableMappingUDF * TMUDFNode);
    NABoolean GenerateInputPartitionAndOrder(TableMappingUDF * TMUDFNode);
    NABoolean DescribeOutputOrder(TableMappingUDF * TMUDFNode);
    void setDllPtr(LmHandle val) {dllPtr_ = val;}
    LmHandle getDllPtr() {return dllPtr_;}
    void setFunctionPtrs(const NAString& entryName);
    void processReturnStatus(ComSInt32 retcode, ComDiagsArea *diags,
                             const char* routineName);
    void setParamInfo(SQLUDR_PARAM * inpParam, const NAType * p, 
                      const NAString & name);
    void setScalarInputParamInfo(const ValueIdList & vids);
    void setScalarInputValues(const ValueIdList & vids);
    SQLUDR_PARAM * copyParams(Int32 num, SQLUDR_PARAM * src);
    SQLUDR_PARAM * createEmptyParams(Int32 num, Int32 nameLen);





  private :
    enum CallType
    {
      VALIDATE_SCALAR_INPUT,
      DESCRIBE_MAX_INPUT,
      DESCRIBE_INP_OUT,
      DESCRIBE_INP_PART_ORDER,
      PRED_PUSHDOWN,
      CARDINALITY,
      CONSTRAINTS,
      COST,
      DOP,
      GENERATE_INP_PART_ORDER,
      GENERATE_OUT_ORDER
    };

    LmHandle dllPtr_ ;
    LmHandle validateScalarInputsPtr_ ;
    LmHandle describeMaxOutputsPtr_;
    LmHandle describeInputsAndOutputsPtr_;
    LmHandle describeInputPartitionAndOrderPtr_;
    LmHandle predicatePushDownPtr_;
    LmHandle cardinalityPtr_;
    LmHandle constraintsPtr_;
    LmHandle costPtr_;
    LmHandle degreeOfParallelismPtr_;
    LmHandle generateInputPartitionAndOrderPtr_;
    LmHandle describeOutputOrderPtr_;

    //static THREAD_P SQLUDR_CHAR *host_data_;     // Static data member

    SQLUDR_CHAR         sqlState_[SQLUDR_SQLSTATE_SIZE];
    SQLUDR_CHAR         msgText_[SQLUDR_MSGTEXT_SIZE];
    SQLUDR_STATEAREA    *stateArea_;
    SQLUDR_TMUDFINFO    *tmudfInfo_;
    SQLUDR_CHAR         **inData_;
    SQLUDR_INT16        *nullInData_;
};
#endif /* UDFDLLINTERACTION_H */
