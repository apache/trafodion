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
* File:         GenMdamPred.C
* Description:  Generate MdamPred's from Item expressions
*
* Created:      12/13/96
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "AllItemExpr.h"
#include "GenExpGenerator.h"
#include "ComKeyMDAM.h"
// -----------------------------------------------------------------------
// 
// -----------------------------------------------------------------------
#include <iostream>    // $$$ for testing

/////////////////////////////////////////////////////////////////////
//
// Contents:
//    
//   BiLogic::mdamPredGen()
//   BiRelat::mdamPredGen()
//   ItemExpr::mdamPredGen()
//   TriRelational::mdamPredGen()
//   UnLogic::mdamPredGen()
//
//////////////////////////////////////////////////////////////////////

short BiLogic::mdamPredGen(Generator * generator,
                           MdamPred ** head,
                           MdamPred ** tail,
                           MdamCodeGenHelper & mdamHelper,
                           ItemExpr * parent)
{
  short rc = 0;
  
  GenAssert(((getOperatorType() == ITM_OR)||(getOperatorType() == ITM_AND)),
            "mdamPredGen: unexpected logical operator.");
  
  ItemExpr * child0 = child(0);
  ItemExpr * child1 = child(1);
  
  if (getOperatorType() == ITM_OR)
    {
      // The assertions below do not apply if disjuncts were generated from a
      // rangespec. For example, a range like {1, [5..10]} would produce the
      // disjunct a=1 OR (a>=5 AND a<=10).
      if(CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) != DF_ON)
        {
          // assert that there is no AND under an OR
          GenAssert(child0->getOperatorType() != ITM_AND,
                    "mdamPredGen: unexpected AND under an OR.");
          GenAssert(child1->getOperatorType() != ITM_AND,
                    "mdamPredGen: unexpected AND under an OR.");
        }
    }
  else // getOperatorType() must be ITM_AND  
    {
      // AND under OR must be a case of an interval subrange in a disjunct
      // derived from a RangeSpecRef. Bypass the asserts below, we use more
      // stringent ones for this case.
      if (parent->getOperatorType() == ITM_OR)
        {
          mdamPredGenSubrange(generator, head, tail, mdamHelper);
          return rc;
        }

      // assert that there is no OR under an AND
      GenAssert(child0->getOperatorType() != ITM_OR,
                  "mdamPredGen: unexpected OR under an AND.");
      GenAssert(child1->getOperatorType() != ITM_OR,
                  "mdamPredGen: unexpected OR under an AND.");
    }

  // Note that the remaining code is not executed for an AND under an OR, as
  // we call a different function and do an early return above for that case.

  rc = child0->mdamPredGen(generator,head,tail,mdamHelper,this);
  
  if (rc == 0)
    {
      MdamPred *leftTail = *tail;
      MdamPred *rightHead = 0;

      rc = child1->mdamPredGen(generator,&rightHead,tail,mdamHelper,this);
      if (rc == 0)
        {
          if (getOperatorType() == ITM_OR)
            {
              // indicate right predicate is not 1st in an OR group
              rightHead->setOr();
              if ( CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON )
                {
                  MdamPred* curr = *head;
                  while(curr->getNext() != NULL)
                    curr=curr->getNext();
                  curr->setNext(rightHead); //@ZXmdam Only diff from else if head and tail not same
                  *tail= *head;             //@ZXmdam Caller sees head and tail as same, although they aren't
                }
              else
                leftTail->setNext(rightHead); // link left and right together
            }   
        }
    }

  return rc;
}


short BiRelat::mdamPredGen(Generator * generator,
                                MdamPred ** head,
                                MdamPred ** tail,
                                MdamCodeGenHelper & mdamHelper,
                                ItemExpr * parent)
{
  short rc = 0;     // assume success
  
  enum MdamPred::MdamPredType predType = 
    MdamPred::MDAM_EQ; // just to initialize

  // Find out what kind of predicate this is.  Note that for DESCending
  // columns, we reverse the direction of any comparison.
  switch (getOperatorType())
    {
      case ITM_EQUAL:
      {
        predType = MdamPred::MDAM_EQ;
        break;
      }
      
      case ITM_LESS:
      {
        predType = MdamPred::MDAM_LT;
        if (mdamHelper.isDescending()) predType = MdamPred::MDAM_GT;
        break;
      }
      
      case ITM_LESS_EQ:
      {
        predType = MdamPred::MDAM_LE;
        if (mdamHelper.isDescending()) predType = MdamPred::MDAM_GE;
        break;
      }
      
      case ITM_GREATER:
      {
        predType = MdamPred::MDAM_GT;
        if (mdamHelper.isDescending()) predType = MdamPred::MDAM_LT;
        break;
      }
      
      case ITM_GREATER_EQ:
      {
        predType = MdamPred::MDAM_GE;
        if (mdamHelper.isDescending()) predType = MdamPred::MDAM_LE;
        break;
      }
      
      case ITM_ITEM_LIST:
      {
        GenAssert(0, "mdamPredGen: encountered multivalued predicate.");
        break;
      }

      default:
      {
        GenAssert(0, "mdamPredGen: unsupported comparison.");
        break;
      }
    }
  
  ItemExpr * child0 = child(0);
  ItemExpr * child1 = child(1);
  ValueId keyColumn = mdamHelper.getKeyColumn();
  
  //  assume predicate is <key> <compare> <value>
  ItemExpr * keyValue = child1;

  if (child1->getValueId() == keyColumn)
    {
      // we guessed wrong -- predicate is <value> <compare> <key>
      keyValue = child0;
      GenAssert(child0->getValueId() != keyColumn,
        "mdamPredGen:  unexpected form for key predicate.");
      // Reverse the comparison operator if it is <, <=, > or >=.
      switch (predType)
        {
          case MdamPred::MDAM_LT:
          {
            predType = MdamPred::MDAM_GT;
            break;
          }

          case MdamPred::MDAM_LE:
          {
            predType = MdamPred::MDAM_GE;
            break;
          }

          case MdamPred::MDAM_GT:
          {
            predType = MdamPred::MDAM_LT;
            break;

          }
          case MdamPred::MDAM_GE:
          {
            predType = MdamPred::MDAM_LE;
            break;
          }
        }  // switch (predType)
    }
  else
    {
      GenAssert(child0->getValueId() == keyColumn,
        "mdamPredGen:  unexpected form for key predicate.");
    }
  
  // generate an expression to convert the key value to the
  // type of the key column (in its key buffer) and encode it

  ItemExpr * vnode = 0;
  
  // errorsCanOccur() determines if errors can occur converting the class
  // datatype to the target datatype.  The object on whose behalf the
  // member function is called is expected to be a NAType.

  NABoolean generateNarrow = 
    keyValue->getValueId().getType().errorsCanOccur(*mdamHelper.getTargetType());

#ifdef _DEBUG
  if ((generateNarrow) &&
      (getenv("NO_NARROWS"))) // for testing -- allows turning off Narrows
    generateNarrow = FALSE; 
#endif

  if (generateNarrow)
    {
      vnode = new(generator->wHeap()) Narrow(keyValue,
                                 mdamHelper.getDataConversionErrorFlag(),
                                 mdamHelper.getTargetType()->newCopy(generator->wHeap()));
    }
  else
    {
      vnode = new(generator->wHeap()) Cast(keyValue,mdamHelper.getTargetType()->newCopy(generator->wHeap()));
    }
  vnode->bindNode(generator->getBindWA());
  vnode->preCodeGen(generator);

  vnode = new(generator->wHeap()) CompEncode(vnode,mdamHelper.isDescending());
  vnode->bindNode(generator->getBindWA());
  
  ValueIdList vnodeList;
  vnodeList.insert(vnode->getValueId());
  
  ex_expr *vexpr = 0;
  ULng32 dummyLen = 0;
  
  rc = generator->getExpGenerator()->generateContiguousMoveExpr(
       vnodeList,
       0, // don't add convert nodes
       mdamHelper.getAtp(),
       mdamHelper.getAtpIndex(),
       mdamHelper.getTupleDataFormat(),
       dummyLen, // out
       &vexpr);

  *head = *tail = new(generator->getSpace()) MdamPred(
       mdamHelper.getDisjunctNumber(),
       predType,
       vexpr);
  
  return rc;
}

// BiRelat for which the following is called will be a predicate for one of the
// endpoints of an MDAM_BETWEEN.
void BiRelat::getMdamPredDetails(Generator* generator,
                                 MdamCodeGenHelper& mdamHelper, 
                                 MdamPred::MdamPredType& predType,
                                 ex_expr** vexpr)
{
  // Find out what kind of predicate this is. Inequality preds are not inverted
  // for descending keys here; instead, the endpoints of the MDAM_BETWEEN
  // interval are switched during creation of the mdam network in the executor.
  switch (getOperatorType())
    {
      case ITM_LESS:
        predType = MdamPred::MDAM_LT;
        break;
      
      case ITM_LESS_EQ:
        predType = MdamPred::MDAM_LE;
        break;
      
      case ITM_GREATER:
        predType = MdamPred::MDAM_GT;
        break;
      
      case ITM_GREATER_EQ:
        predType = MdamPred::MDAM_GE;
        break;
      
      default:
        GenAssert(0, "mdamPredGen: invalid comparison for subrange.");
        break;
    }
  
  ItemExpr* child0 = child(0);
  ItemExpr* child1 = child(1);
  ValueId keyColumn = mdamHelper.getKeyColumn();
  
  // Canonical form used by rangespec is <key> <compare> <value>.
  ItemExpr* keyValue = child1;
  GenAssert(child0->getValueId() == keyColumn,
            "mdamPredGen:  unexpected form for key predicate.");

  // generate an expression to convert the key value to the
  // type of the key column (in its key buffer) and encode it
  ItemExpr* vnode = NULL;
  
  // errorsCanOccur() determines if errors can occur converting the class
  // datatype to the target datatype.  The object on whose behalf the
  // member function is called is expected to be a NAType.
  NABoolean generateNarrow = 
      keyValue->getValueId().getType().errorsCanOccur(*mdamHelper.getTargetType());

#ifdef _DEBUG
  if ((generateNarrow) &&
      (getenv("NO_NARROWS"))) // for testing -- allows turning off Narrows
    generateNarrow = FALSE; 
#endif

  if (generateNarrow)
    vnode = new(generator->wHeap())
              Narrow(keyValue,
                     mdamHelper.getDataConversionErrorFlag(),
                     mdamHelper.getTargetType()->newCopy(generator->wHeap()));
  else
    vnode = new(generator->wHeap()) 
              Cast(keyValue,
                   mdamHelper.getTargetType()->newCopy(generator->wHeap()));

  vnode->bindNode(generator->getBindWA());
  vnode->preCodeGen(generator);

  vnode = new(generator->wHeap()) CompEncode(vnode,mdamHelper.isDescending());
  vnode->bindNode(generator->getBindWA());
  
  ValueIdList vnodeList;
  vnodeList.insert(vnode->getValueId());
  
  ULng32 dummyLen = 0;
  
  short rc = 
       generator->getExpGenerator()
                ->generateContiguousMoveExpr(vnodeList,
                                             0, // don't add convert nodes
                                             mdamHelper.getAtp(),
                                             mdamHelper.getAtpIndex(),
                                             mdamHelper.getTupleDataFormat(),
                                             dummyLen, // out
                                             vexpr);
       
  GenAssert(rc == 0, "generateContiguousMoveExpr() returned error when called "
                     "from BiRelat::getMdamPredDetails().");
}


// Generate MDAM_BETWEEN mdam predicate.
void BiLogic::mdamPredGenSubrange(Generator* generator,
                                  MdamPred** head,
                                  MdamPred** tail,
                                  MdamCodeGenHelper& mdamHelper)
{
  ItemExpr* child0 = child(0);
  ItemExpr* child1 = child(1);
  
  ex_expr *vexpr1 = NULL, *vexpr2 = NULL;
  MdamPred::MdamPredType predType1, predType2;
  Int16 val1Inclusive = 0, val2Inclusive = 0;
  
  child0->getMdamPredDetails(generator, mdamHelper, predType1, &vexpr1);

  // The two endpoint values are stored in successive tupps in the ATP.
  mdamHelper.setAtpIndex(mdamHelper.getAtpIndex() + 1);
  child1->getMdamPredDetails(generator, mdamHelper, predType2, &vexpr2);
  mdamHelper.setAtpIndex(mdamHelper.getAtpIndex() - 1);

  if (predType1 == MdamPred::MDAM_GE)
    val1Inclusive = 1;
  else if (predType1 == MdamPred::MDAM_GT)
    val1Inclusive = 0;
  else
    GenAssert(FALSE, "mdamPredGenSubrange: subrange does not conform to "
                     "rangespec canonical form.");
  
  if (predType2 == MdamPred::MDAM_LE)
    val2Inclusive = 1;
  else if (predType2 == MdamPred::MDAM_LT)
    val2Inclusive = 0;
  else
    GenAssert(FALSE, "mdamPredGenSubrange: subrange does not conform to "
                     "rangespec canonical form.");
  
  *head = *tail = new(generator->getSpace()) MdamPred(
       mdamHelper.getDisjunctNumber(),
       MdamPred::MDAM_BETWEEN,
       vexpr1, vexpr2,
       val1Inclusive, val2Inclusive,
       (Int16)mdamHelper.isDescending());
}


short ItemExpr::mdamPredGen(Generator *,
                MdamPred **,
                MdamPred **,
                MdamCodeGenHelper &,
                ItemExpr *)
{
  GenAssert(0, "Should never reach ItemExpr::mdamPredGen");
  return -1;
}

short TriRelational::mdamPredGen(Generator * generator,
                   MdamPred ** head,
                   MdamPred ** tail,
                   MdamCodeGenHelper & mdamHelper,
                   ItemExpr * parent)
{
  // temp -- haven't been able to unit test this code yet because I haven't been
  // able to figure out how to get the Optimizer to pick a TriRelational guy as
  // a key predicate -- seems better to have the code abort rather than run unverified
  // and possibly fail in strange ways
  GenAssert(0, "Reached TriRelational::mdamPredGen");
  return -1;
  // end temp code

  short rc = 0;

  enum MdamPred::MdamPredType predType = 
    MdamPred::MDAM_EQ; // just to initialize

  // Find out what kind of predicate this is.  Note that for DESCending
  // columns, we reverse the direction of any comparison.
  switch (getOperatorType())
    {
      case ITM_LESS_OR_LE:
      {
        predType = MdamPred::MDAM_LT;
        if (mdamHelper.isDescending())
          predType = MdamPred::MDAM_GT;
        break;
      }
      
      case ITM_GREATER_OR_GE:
      {
        predType = MdamPred::MDAM_GT;
        if (mdamHelper.isDescending())
          predType = MdamPred::MDAM_LT;
        break;
      }

      default:
      {
        GenAssert(0, "mdamPredGen: unsupported TriRelational comparison.");
        break;
      }
    }
  
  ItemExpr * child0 = child(0);
  ItemExpr * child1 = child(1);
  ValueId keyColumn = mdamHelper.getKeyColumn();
  
  //  assume predicate is <key> <compare> <value>
  ItemExpr * keyValue = child1;

  if (child1->getValueId() == keyColumn)
    {
      // we guessed wrong -- predicate is <value> <compare> <key>
      keyValue = child0;
      GenAssert(child0->getValueId() != keyColumn,
        "mdamPredGen:  unexpected form for key predicate.");
      // $$$ Add code here to reverse the comparison?
    }
  else
    {
      GenAssert(child0->getValueId() == keyColumn,
        "mdamPredGen:  unexpected form for key predicate.");
    }
  
  // generate an expression to convert the key value to the
  // type of the key column (in its key buffer) and encode it

  ItemExpr * vnode = 0;
  
  // errorsCanOccur() determines if errors can occur converting the class
  // datatype to the target datatype.  The object on whose behalf the
  // member function is called is expected to be a NAType.


  NABoolean generateNarrow = 
    keyValue->getValueId().getType().errorsCanOccur(*mdamHelper.getTargetType());

  if ((generateNarrow) &&
      (getenv("NO_NARROWS"))) // for testing -- allows turning off Narrows
    generateNarrow = FALSE;  

  if (generateNarrow)
    {
      vnode = new(generator->wHeap())
                Narrow(keyValue,
                       mdamHelper.getDataConversionErrorFlag(),
                       mdamHelper.getTargetType()->newCopy());
    }
  else
    {
      vnode = new(generator->wHeap()) 
                Cast(keyValue,mdamHelper.getTargetType()->newCopy());
    }

  vnode = new CompEncode(vnode,mdamHelper.isDescending());

  vnode->bindNode(generator->getBindWA());
 
  // add CASE 
  //      WHEN child(2)
  //         CAST(round up/round down)
  //      ELSE
  //         no-op

  ItemExpr * hnode = 0;

  if (predType == MdamPred::MDAM_LT)
    hnode = new ConstValue(2 /* ex_conv_clause::CONV_RESULT_ROUNDED_UP_TO_MIN */);
  else
    hnode = new ConstValue(-2 /* ex_conv_clause::CONV_RESULT_ROUNDED_DOWN_TO_MAX */);
                  
  hnode = generator->getExpGenerator()->
          createExprTree("CASE WHEN @B1 THEN @A2 ELSE @A3 END",
                         0,
                         3, // number of subtree parameters
                         child(2),  // @B1
                         hnode,     // @A2
                         0);        // @A3 -- results in no operation

  hnode->bindNode(generator->getBindWA());

  // Assign attributes for result value 

  ValueId vnodeId = vnode->getValueId();
  ValueId hnodeId = hnode->getValueId(); 
  ULng32 tupleLength = 0;

  ValueIdList vnodeList;
  vnodeList.insert(vnode->getValueId());
  
  generator->getExpGenerator()->processValIdList(
                vnodeList,
                mdamHelper.getTupleDataFormat(),
                tupleLength,  // out
                mdamHelper.getAtp(),
                mdamHelper.getAtpIndex());

  // Assign attributes for modifying data conversion error flag
  // Note that all we do is copy the already-assigned attributes

  ItemExpr * dataCEF = mdamHelper.getDataConversionErrorFlag();
  ValueId dataCEFId = dataCEF->getValueId();
  Attributes * dataCEFAttr = 
     (generator->getMapInfo(dataCEFId))->getAttr();
   
  generator->addMapInfoToThis(generator->getLastMapTable(), hnodeId,dataCEFAttr);
  
  // finally generate the expression and hang it off an MdamPred
  
  ex_expr *vexpr = 0;

  vnodeList.insert(hnode->getValueId());  // put hnode in there too
  rc = generator->getExpGenerator()->generateListExpr(
                                        vnodeList,
                                        ex_expr::exp_ARITH_EXPR,
                                        &vexpr);

  *head = *tail = new(generator->getSpace()) 
                    MdamPred(mdamHelper.getDisjunctNumber(),
                             predType,
                             vexpr);
  
  return rc;
}

short UnLogic::mdamPredGen(Generator * generator,
                           MdamPred ** head,
                           MdamPred ** tail,
                           MdamCodeGenHelper & mdamHelper,
                           ItemExpr * parent)
{
  short rc = 0;     // assume success
  
  enum MdamPred::MdamPredType predType = 
    MdamPred::MDAM_ISNULL; // just to initialize

  // find out what kind of predicate this is
  switch (getOperatorType())
    {
      case ITM_IS_NULL:
      {
        // We distinguish the ASCending and DESCending cases, because
        // the NULL value is considered high for ASCending keys, but
        // low for DESCending.
        predType = MdamPred::MDAM_ISNULL;
        if (mdamHelper.isDescending()) predType = MdamPred::MDAM_ISNULL_DESC;
        break;
      }
      
      case ITM_IS_NOT_NULL:
      {
        predType = MdamPred::MDAM_ISNOTNULL;
        break;
      }
      
      default:
      {
        GenAssert(0, "mdamPredGen: unsupported unary operator.");
        break;
      }
    }

  *head = *tail = new(generator->getSpace())
                    MdamPred(mdamHelper.getDisjunctNumber(),
                             predType,
                             0 /* no expression for IS NULL and IS NOT NULL */);
  
  return rc;
}
