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
* File:         GenKey.C
* Description:  Generate code for key expressions
*
* Created:      11/25/96
* Language:     C++
*
*
******************************************************************************
*/



#include "Sqlcomp.h"
#include "SearchKey.h"
#include "Generator.h"
#include "GenExpGenerator.h"
//#include "ex_stdh.h"
#include "ComTdb.h"
//#include "ex_tcb.h"
#include "ComKeySingleSubset.h"
#include "ComKeyMDAM.h"  // generator Mdam classes
#include "mdamkey.h"   // optimizer Mdam classes
#include "ExpCriDesc.h"

// the next include file is here solely to make the horrible kludge work
#include "NAFileSet.h"

/////////////////////////////////////////////////////////////////////
//
// Contents:
//    
//   ExpGenerator::buildKeyInfo()
//   This method is used to generate the key information for both the
//   subset/range operators and unique operators.
//
//////////////////////////////////////////////////////////////////////


short ExpGenerator::buildKeyInfo(keyRangeGen ** keyInfo, // out -- generated object
                                 Generator * generator,
                                 const NAColumnArray & keyColumns,
                                 const ValueIdList & listOfKeyColumns,
                                 const ValueIdList & beginKeyPred,
                                 const ValueIdList & endKeyPred,
                                 const SearchKey * searchKey,
                                 const MdamKey * mdamKeyPtr,
                                 const NABoolean reverseScan,
                                 const ExpTupleDesc::TupleDataFormat tf
                                 )

{
  Space * space = generator->getSpace();

  const Int32 work_atp = 1;
  const Int32 key_atp_index = 2;
  const Int32 exclude_flag_atp_index = 3;
  const Int32 data_conv_error_atp_index = 4;
  const Int32 key_column_atp_index = 5; // used only for Mdam
  const Int32 key_column2_atp_index = 6; // used only for Mdam MDAM_BETWEEN pred;
                                         //   code in BiLogic::mdamPredGenSubrange
                                         //   and MdamColumn::buildDisjunct
                                         //   requires this to be 1 more than
                                         //   key_column_atp_index
  ULng32 keyLen;

  // add an entry to the map table for work Atp
  MapTable *keyBufferPartMapTable = generator->appendAtEnd();

  // generate a temporary variable, which will be used for handling
  // data conversion errors during key building
  ValueIdList temp_varb_list;

  ItemExpr * dataConversionErrorFlag = new(generator->wHeap())
    HostVar("_sys_dataConversionErrorFlag",
            new(generator->wHeap()) SQLInt(TRUE,FALSE), // int not null
            TRUE);
  ULng32 temp_varb_tupp_len;

  dataConversionErrorFlag->bindNode(generator->getBindWA());
  temp_varb_list.insert(dataConversionErrorFlag->getValueId());
  processValIdList(temp_varb_list,
                   ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
                   temp_varb_tupp_len,  // out
                   work_atp,
                   data_conv_error_atp_index);

  NABoolean doEquiKeyPredOpt = FALSE;
#ifdef _DEBUG
  if (getenv("DO_EQUI_KEY_PRED_OPT"))
    doEquiKeyPredOpt 
      = (searchKey ? searchKey->areAllChosenPredsEqualPreds() : FALSE);
#endif
  if (mdamKeyPtr == NULL)
    {
      // check to see if there is a begin key expression; if there
      // isn't, don't generate a key object
      if (beginKeyPred.entries() == 0)
        *keyInfo = 0;
      else
        {
          // For subset and range operators, generate the begin key
          // expression, end key expression, begin key exclusion expression
          // and end key exclusion expression.  For unique operators,
          // generate only the begin key exppression.
          ex_expr *bk_expr = 0;
          ex_expr *ek_expr = 0;
          ex_expr *bk_excluded_expr = 0;
          ex_expr *ek_excluded_expr = 0;
          
          short bkey_excluded = 0;
          short ekey_excluded = 0;
          
          generateKeyExpr(keyColumns,
                          beginKeyPred,
                          work_atp,
                          key_atp_index,
                          dataConversionErrorFlag,
                          tf,
                          keyLen, // out
                          &bk_expr,  // out
                          doEquiKeyPredOpt);
          
          if (&endKeyPred)
             generateKeyExpr(keyColumns,
                             endKeyPred,
                             work_atp,
                             key_atp_index,
                             dataConversionErrorFlag,
                             tf,
                             keyLen, // out -- should be the same as above
                             &ek_expr,  // out
                             doEquiKeyPredOpt);
              
          if (reverseScan)
            {
              // reverse scan - swap the begin and end key predicates
              
              // Note: evidently, the Optimizer has already switched
              // the key predicates in this case, so what we are
              // really doing is switching them back.
              
              ex_expr *temp = bk_expr;
              bk_expr = ek_expr;
              ek_expr = temp;
            }
          if (searchKey)
            {
              generateExclusionExpr(searchKey->getBeginKeyExclusionExpr(),
                  work_atp,
                  exclude_flag_atp_index,
                  &bk_excluded_expr); // out
              
              bkey_excluded = (short) searchKey->isBeginKeyExclusive(); 
              
              generateExclusionExpr(searchKey->getEndKeyExclusionExpr(),
                  work_atp,
                  exclude_flag_atp_index,
                  &ek_excluded_expr); // out
              
              ekey_excluded = (short) searchKey->isEndKeyExclusive(); 

              if (reverseScan)
                {
                  NABoolean x = bkey_excluded;
                  bkey_excluded = ekey_excluded;
#pragma nowarn(1506)   // warning elimination 
                  ekey_excluded = x;
#pragma warn(1506)  // warning elimination 

                  ex_expr* temp = bk_excluded_expr;
                  bk_excluded_expr = ek_excluded_expr;
                  bk_excluded_expr = temp;
                }
            } // if searchKey
          
          // Build key info

          // the work cri desc is used to build key values (entry 2) and
          // to compute the exclusion flag (entry 3) to monitor for data
          // conversion errors (entry 4) and to compute values on a column
          // basis (entry 5 - Mdam only)
          ex_cri_desc * work_cri_desc 
            = new(space) ex_cri_desc(6, space);
          
          *keyInfo = new(space) keySingleSubsetGen(
               keyLen,
               work_cri_desc,
               key_atp_index,
               exclude_flag_atp_index,
               data_conv_error_atp_index,
               bk_expr,
               ek_expr,
               bk_excluded_expr,
               ek_excluded_expr,
               // static exclude flags (if exprs are NULL)
               bkey_excluded,
               ekey_excluded); 
    }
    }  // end of non-mdam case
  else // Mdam case
    {
      // the work cri desc is used to build key values (entry 2) and
      // to compute the exclusion flag (entry 3) to monitor for data
      // conversion errors (entry 4) and to compute values on a column
      // basis (entry 5 - Mdam only, and entry 6 - Mdam only, and only
      // for MDAM_BETWEEN predtype)
      ex_cri_desc * work_cri_desc 
            = new(space) ex_cri_desc(7, space);

      // compute the format of the key buffer -- We need this
      // so that Mdam will know, for each column, where in the buffer
      // to move a value and how many bytes that value takes.  The
      // next few lines of code result in this information being stored
      // in the attrs array.

      // Some words on the technique:  We create expressions whose
      // result datatype matches the key buffer datatypes for each key
      // column.  Then we use the datatypes of these expressions to
      // compute buffer format.  The expressions themselves are not
      // used any further; they do not result in compiled expressions
      // in the plan.  At run time we use string moves to move key
      // values instead.

 
      const CollIndex keyCount = listOfKeyColumns.entries();
      CollIndex i;

      // assert at least one column
      GenAssert(keyCount > 0,"MDAM:  at least one key column required.");

      Attributes ** attrs = new(generator->wHeap()) Attributes * [keyCount];
      
      for (i = 0; i < keyCount; i++)
        {
          ItemExpr * col_node =
              listOfKeyColumns[i].getItemExpr(); 
          ItemExpr *enode = col_node;

          if ((tf == ExpTupleDesc::SQLMX_KEY_FORMAT) &&
              (enode->getValueId().getType().getVarLenHdrSize() > 0))
            {
              // varchar keys in SQL/MP tables are converted to
              // fixed length chars in key buffers

              const CharType& char_type =
                          (CharType&)(enode->getValueId().getType());

              if (!CollationInfo::isSystemCollation(char_type.getCollation()))
                {
                  enode = new(generator->wHeap()) 
                             Cast(enode, 
                                   (new (generator->wHeap())
                                          SQLChar( CharLenInfo(char_type.getStrCharLimit(),
                                                   char_type.getDataStorageSize()),
                                          char_type.supportsSQLnull(),
                                          FALSE, FALSE, FALSE,
                                          char_type.getCharSet(),
                                          char_type.getCollation(),
                                          char_type.getCoercibility())));
                }
            }

          NABoolean desc_flag;

          if (keyColumns.isAscending(i))
            desc_flag = reverseScan;
          else
            desc_flag = !reverseScan;

#pragma nowarn(1506)   // warning elimination 
          enode = new(generator->wHeap()) CompEncode(enode,desc_flag); 
#pragma warn(1506)  // warning elimination 
          enode->bindNode(generator->getBindWA());

          attrs[i] = 
            (generator->
             addMapInfoToThis(keyBufferPartMapTable, enode->getValueId(), 0))->getAttr();
        }  // for, over keyCount

      // Compute offsets, lengths, etc. and assign them to the right
      // atp and atp index

      processAttributes((ULng32)keyCount,
                        attrs, tf,
                        keyLen,
                        work_atp, key_atp_index);

      // Now we have key column offsets and lengths stored in attrs.
 
      // Next, for each column, generate expressions to compute hi,
      // lo, non-null hi and non-null lo values, and create
      // MdamColumnGen structures.

      // Notes: In the Mdam network itself, all key values are
      // encoded.  Hence, we generate CompEncode nodes in all of the
      // expressions, regardless of tuple format.  In the Simulator
      // case, we must at run-time decode the encoded values when
      // moving them to the key buffer.  $$$ We need an expression to
      // do this.  This decoding work has not yet been done, so the
      // simulator only works correctly for columns that happen to be
      // correctly aligned and whose encoding function does not change
      // the value. $$$

      MdamColumnGen * first = 0;
      MdamColumnGen * last = 0;
      LIST(NAType *) keyTypeList(generator->wHeap());//to keep the type of the keys for later

      for (i = 0; i < keyCount; i++)
        {
          // generate expressions to compute hi, lo, non-null hi, non-null lo
          NAType * targetType = (keyColumns[i]->getType())->newCopy(generator->wHeap());

          // Genesis case 10-971031-9814 fix: desc_flag must take into account
          // both the ASC/DESC attribute of the key column and the reverseScan
          // attribute. Before this fix, it only took into account the first of
          // these.
          NABoolean desc_flag;

          if (keyColumns.isAscending(i))
            desc_flag = reverseScan;
          else
            desc_flag = !reverseScan;
          // End Genesis case 10-971031-9814 fix.

          if ((tf == ExpTupleDesc::SQLMX_KEY_FORMAT) &&
              (targetType->getVarLenHdrSize() > 0))
            {

// 5/9/98: add support for VARNCHAR
              const CharType* char_type = (CharType*)(targetType);

              if (!CollationInfo::isSystemCollation(char_type->getCollation()))
                {
                  targetType = new(generator->wHeap()) 
                                      SQLChar( CharLenInfo(char_type->getStrCharLimit(),
                                                           char_type->getDataStorageSize()),
                                      char_type -> supportsSQLnull(),
                                      FALSE, FALSE, FALSE,
                                      char_type -> getCharSet(),
                                      char_type -> getCollation(),
                                      char_type -> getCoercibility());
/*
                  targetType->getNominalSize(),
                  targetType->supportsSQLnull()
*/
                }
            }

          keyTypeList.insert(targetType);  // save in ith position for later
  
          // don't need to make copy of targetType in next call
          ItemExpr * lo = new(generator->wHeap()) ConstValue(targetType, 
                                !desc_flag, 
                                TRUE /* allow NULL */);
#pragma nowarn(1506)   // warning elimination 
          lo = new(generator->wHeap()) CompEncode(lo,desc_flag); 
#pragma warn(1506)  // warning elimination 
          lo->bindNode(generator->getBindWA());

          ValueIdList loList;
          loList.insert(lo->getValueId());

          ex_expr *loExpr = 0;
          ULng32 dataLen = 0;

          generateContiguousMoveExpr(loList,
                   0, // don't add convert nodes
                   work_atp,
                   key_column_atp_index,
                   tf,
                   dataLen,
                   &loExpr);

          ItemExpr * hi = new(generator->wHeap()) ConstValue(targetType->newCopy(generator->wHeap()),
                                desc_flag,
                                TRUE /* allow NULL */);
#pragma nowarn(1506)   // warning elimination 
          hi = new(generator->wHeap()) CompEncode(hi,desc_flag);
#pragma warn(1506)  // warning elimination 
          hi->bindNode(generator->getBindWA());

          ValueIdList hiList;
          hiList.insert(hi->getValueId());

          ex_expr *hiExpr = 0;
          
          generateContiguousMoveExpr(hiList,
                   0, // don't add convert nodes
                   work_atp,
                   key_column_atp_index,
                   tf,
                   dataLen,
                   &hiExpr);

          ex_expr *nonNullLoExpr = loExpr;
          ex_expr *nonNullHiExpr = hiExpr;

          if (targetType->supportsSQLnull())
            {
              if (desc_flag)
                {
                  ItemExpr * nonNullLo = new(generator->wHeap())
                    ConstValue(targetType->newCopy(generator->wHeap()),
                               !desc_flag, 
                               FALSE /* don't allow NULL */);
      #pragma nowarn(1506)   // warning elimination 
                  nonNullLo = new(generator->wHeap()) CompEncode(nonNullLo,desc_flag); 
      #pragma warn(1506)  // warning elimination 
                  nonNullLo->bindNode(generator->getBindWA());

                  ValueIdList nonNullLoList;
                  nonNullLoList.insert(nonNullLo->getValueId());
                  nonNullLoExpr = 0;  // so we will get an expression back

                  generateContiguousMoveExpr(nonNullLoList,
                           0, // don't add convert nodes
                           work_atp,
                           key_column_atp_index,
                           tf,
                           dataLen,
                           &nonNullLoExpr);
                }
              else
                {
                  ItemExpr * nonNullHi = new(generator->wHeap())
                    ConstValue(targetType->newCopy(generator->wHeap()),
                         desc_flag, 
                         FALSE /* don't allow NULL */);
#pragma nowarn(1506)   // warning elimination 
                  nonNullHi = new(generator->wHeap()) CompEncode(nonNullHi,desc_flag); 
#pragma warn(1506)  // warning elimination 
                  nonNullHi->bindNode(generator->getBindWA());
                  
                  ValueIdList nonNullHiList;
                  nonNullHiList.insert(nonNullHi->getValueId());
                  nonNullHiExpr = 0;  // so we will get an expression back
                  
                  generateContiguousMoveExpr(nonNullHiList,
                           0, // don't add convert nodes
                           work_atp,
                           key_column_atp_index,
                           tf,
                           dataLen,
                           &nonNullHiExpr);
                }
            }

          NABoolean useSparseProbes = mdamKeyPtr->isColumnSparse(i); 
          
          // calculate offset to the beginning of the column value
          // (including the null indicator and the varchar length
          // indicator if present)

          ULng32 column_offset = attrs[i]->getOffset();

          if (attrs[i]->getNullFlag())
            column_offset = attrs[i]->getNullIndOffset();
          else if (attrs[i]->getVCIndicatorLength() > 0)
            column_offset = attrs[i]->getVCLenIndOffset();

          last = new(space) MdamColumnGen(last,
                  dataLen,
                  column_offset,
                  useSparseProbes,
                  loExpr,
                  hiExpr,
                  nonNullLoExpr,
                  nonNullHiExpr);
          if (first == 0)
            first = last;
        }  // for over keyCount

      // generate MdamPred's and attach to MdamColumnGen's

      const ColumnOrderListPtrArray &columnOrderListPtrArray =
           mdamKeyPtr->getColumnOrderListPtrArray();

#ifdef _DEBUG
      // Debug print stataments below depend on this
      // variable:
      char *ev = getenv("MDAM_PRINT");
      const NABoolean mdamPrintOn = (ev != NULL AND strcmp(ev,"ON")==0);
#endif

#ifdef _DEBUG
      if (mdamPrintOn)
        {
          fprintf(stdout, "\n\n***Generating the MDAM key for table with index"
                  " columns: ");
          listOfKeyColumns.display();
        }
#endif

      for (CollIndex n = 0; n < columnOrderListPtrArray.entries(); n++)
        {
          // get the list of key predicates associated with the n disjunct:
          const ColumnOrderList &columnOrderList = *columnOrderListPtrArray[n];

#ifdef _DEBUG
          if (mdamPrintOn)
            {
              fprintf(stdout,"\nDisjunct[%d]:----------------\n",n);
              columnOrderList.print();
            }
#endif
          MdamColumnGen * cc = first;

          CMPASSERT(keyCount == columnOrderList.entries());
          const ValueIdSet *predsPtr = NULL;
          for (i = 0; i < keyCount; i++)
            {
#ifdef _DEBUG
              if (mdamPrintOn)
                {
                  fprintf(stdout, "Column(%d) using: ", i);
                  if ( mdamKeyPtr->isColumnSparse(i) )
                    fprintf(stdout,"SPARSE probes\n");
                  else
                    fprintf(stdout, "DENSE probes\n");
                }
#endif
              // get predicates for column order i:
              predsPtr = columnOrderList[i];

              NAType * keyType = keyTypeList[i];

              NABoolean descending;

              if (keyColumns.isAscending(i))
                descending = reverseScan;
              else
                descending = !reverseScan;

              ValueId keyColumn = listOfKeyColumns[i];

              MdamCodeGenHelper mdamHelper(
                   n,
                   keyType,
                   descending,
                   work_atp,
                   key_column_atp_index,
                   tf,
                   dataConversionErrorFlag,
                   keyColumn);

              MdamPred * lastPred = cc->getLastPred();

              if (predsPtr != NULL)
                {
                  for (ValueId predId = predsPtr->init(); 
                       predsPtr->next(predId); predsPtr->advance(predId))
                    {
                      MdamPred * head = 0;  // head of generated MdamPred's
                      MdamPred * tail = 0;

                      ItemExpr * orGroup = predId.getItemExpr();

                      orGroup->mdamPredGen(generator,&head,&tail,mdamHelper,NULL);

                      if (lastPred)
                        {
                          if ( CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON )
                            {
                              MdamPred* curr = lastPred;
                              while(curr->getNext() != NULL)
                                curr=curr->getNext();
                              curr->setNext(head);
                            }
                          else
                            lastPred->setNext(head);
                        }
                      cc->setLastPred(tail);
                      lastPred = tail;  //@ZXmdam if 1st pred has head != tail, head is lost
                    } // for over preds
                } // if (predsPtr != NULL)
              cc = cc->getNext();
            } // for every order...
        } // for every column order list in the array (of disjuncts)

      // build the Mdam key info
      *keyInfo = new(space) keyMdamGen(keyLen,
                                       work_cri_desc,
                                       key_atp_index,
                                       exclude_flag_atp_index,
                                       data_conv_error_atp_index,
                                       key_column_atp_index,
                                       first,
                                       last,
                                       reverseScan,
                                       generator->wHeap());

    }  // end of mdam case

  // reset map table to forget about the key object's work Atp
  
  // aside: this logic is more bloody than it should be because the
  // map table implementation doesn't accurately reflect the map table
  // abstraction
  
  generator->removeAll(keyBufferPartMapTable); // deletes anything that might have been
  // added after keyBufferPartMapTable (at
  // this writing we don't expect there to
  // be anything, but we want to be safe)
  // at this point keyBufferPartMapTable should be the last map table in the
  // global map table chain
  generator->removeLast();  // unlinks keyBufferPartMapTable and deletes it
  
  return 0;
};
