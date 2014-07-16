/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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
/* -*-C++-*-
/**************************************************************************
*
* File:         EncodedKeyValue.cpp
* Description:  Functions to compute binary encoded keys that can be written 
                to disk for a given set of desc_structs.
* Origin:       Copy of existing code from GenRFork.cpp
* Created:      10/30/2013
* Language:     C++
*
*************************************************************************
*/

#include "EncodedKeyValue.h"
#include "Generator.h"
#include "GenExpGenerator.h"
#include "ExpCriDesc.h"
#include "ExpAtp.h"
#include "exp_dp2_expr.h"
#include "exp_clause_derived.h"
#include "CmpStatement.h"
#include "NATable.h"

// defined in SynthType.cpp
extern
void emitDyadicTypeSQLnameMsg(Lng32 sqlCode,
			      const NAType &op1,
			      const NAType &op2,
			      const char *str1 = NULL,
			      const char *str2 = NULL,
			      ComDiagsArea * diagsArea = NULL,
                              const Lng32 int1 = -999999);



NAString * getMinMaxValue(desc_struct * column,
				 desc_struct * key,
				 NABoolean highKey,
                                 CollHeap * h)
{
  NAString * minMaxValue = NULL;
  
  NAType * type; // deleted at the end of this method
  if (NAColumn::createNAType(&column->body.columns_desc, NULL, type, NULL))
    return NULL;
  
  Lng32 buflen = type->getTotalSize();
  char * buf = new char[buflen]; // deleted at the end of this method

  NABoolean nullValue = FALSE;
  if (highKey == FALSE)
    {
      // low key needed
      if (key->body.keys_desc.ordering == 0) // ascending
	type->minRepresentableValue(buf, &buflen, 
				    &minMaxValue,
				    h) ;
      else
	{
	  if (type->supportsSQLnull())
	    {
	      minMaxValue = new (h) NAString("NULL");
	      nullValue = TRUE;
	    }
	  else
	    {
	      type->maxRepresentableValue(buf, &buflen, 
					  &minMaxValue,
					  h) ;
	    }
	}
    }
  else
    {
      // high key needed
      if (key->body.keys_desc.ordering == 0) // ascending
	{
	  if (type->supportsSQLnull())
	    {
	      minMaxValue = new (h) NAString("NULL");
	      nullValue = TRUE;
	    }
	  else
	    type->maxRepresentableValue(buf, &buflen, 
					&minMaxValue,
					h) ;
	}
      else
	type->minRepresentableValue(buf, &buflen, 
				    &minMaxValue,
				    h) ;
    }
    
  delete [] buf;
  delete type;

  return minMaxValue;
}

NAString ** createInArrayForLowOrHighKeys(desc_struct   * column_descs,
					  desc_struct   * key_descs,
					  Lng32 numKeys,
					  NABoolean highKey,
                                          CollHeap * h)
{
  NAString ** inValuesArray = new (h) NAString * [numKeys];
  
  desc_struct * column = column_descs;
  desc_struct * key    = key_descs;
  Int32 i = 0;
  while (key)
    {
      column = column_descs;
      for (Int32 j = 0; j < key->body.keys_desc.tablecolnumber; j++)
	column = column->header.next;

      inValuesArray[i] = getMinMaxValue(column, key, highKey, h) ;
      
      i++;
      key = key->header.next;
    }
  
  return inValuesArray;
}

ItemExpr * buildEncodeTree(desc_struct * column,
                           desc_struct * key,
                           NAString * dataBuffer, //IN:contains original value
                           Generator * generator,
                           ComDiagsArea * diagsArea)
{
  ExpGenerator * expGen = generator->getExpGenerator();

  // values are encoded by evaluating the expression:
  //    encode (cast (<dataBuffer> as <datatype>))
  // where <dataBuffer> points to the string representation of the
  //      data value to be encoded, and <datatype> contains the
  //      PIC repsentation of the columns's datatype.

  // create the CAST part of the expression using the parser.
  
  // if this is a nullable column and the key value passed in
  // is a NULL value, then treat it as a special case. A NULL value
  // is passed in as an unquoted string of characters NULL in the
  // dataBuffer. This case has to be treated different since the
  // parser doesn't recognize the syntax "CAST (NULL as <datatype>)".
  NAString ns;
  ItemExpr * itemExpr;
  NABoolean nullValue = FALSE;

  NABoolean caseinsensitiveEncode = FALSE;
  if (column->body.columns_desc.caseinsensitive)
    caseinsensitiveEncode = TRUE;

  if (column->body.columns_desc.null_flag &&
      dataBuffer->length() >= 4 &&
      str_cmp(*dataBuffer, "NULL", 4) == 0)
    {
      nullValue = TRUE;

      ns = "CAST ( @A1 AS ";
      ns += column->body.columns_desc.pictureText;
      ns += ");";
  
      // create a NULL constant
      ConstValue * nullConst = new(expGen->wHeap()) ConstValue();
      nullConst->synthTypeAndValueId();

      itemExpr = expGen->createExprTree(ns,
                                        CharInfo::UTF8,
					ns.length(),
					1, nullConst);
    }
  else
    {
      ns = "CAST ( ";
      ns += *dataBuffer;
      ns += " AS ";
      ns += column->body.columns_desc.pictureText;
      ns += ");";
  
      itemExpr = expGen->createExprTree(ns,
                                        CharInfo::UTF8,
					ns.length());
    }

  CMPASSERT(itemExpr != NULL);
  ItemExpr *boundItemExpr =
  itemExpr->bindNode(generator->getBindWA());
  if (boundItemExpr == NULL)
    return NULL;

  // make sure that the source and target values have compatible type.
  // Do this only if source is not a null value.
  NAString srcval;
  srcval = "";
  srcval += *dataBuffer;
  srcval += ";";
  ItemExpr * srcNode = expGen->createExprTree(srcval, CharInfo::UTF8, srcval.length());
  CMPASSERT(srcNode != NULL);
  srcNode->synthTypeAndValueId();
  if ((NOT nullValue) &&
      (NOT srcNode->getValueId().getType().isCompatible(itemExpr->getValueId().getType())))
    {
      if (diagsArea)
	{
	  emitDyadicTypeSQLnameMsg(-4039, 
				   itemExpr->getValueId().getType(),
				   srcNode->getValueId().getType(),
				   column->body.columns_desc.colname,
				   NULL,
				   diagsArea);
	}

      return NULL;
    }

  if (column->body.columns_desc.null_flag)
    ((NAType *)&(itemExpr->getValueId().getType()))->setNullable(TRUE);
  else
    ((NAType *)&(itemExpr->getValueId().getType()))->setNullable(FALSE);
  
  // Explode varchars by moving them to a fixed field
  // whose length is equal to the max length of varchar.
  ////collation??
  DataType datatype = column->body.columns_desc.datatype;
  if (DFS2REC::isSQLVarChar(datatype))
    {
      char lenBuf[10];
      NAString vc((NASize_T)100);	// preallocate a big-enough buf

      size_t len = column->body.columns_desc.length;
      if (datatype == REC_BYTE_V_DOUBLE) len /= SQL_DBCHAR_SIZE;

      vc = "CAST (@A1 as CHAR(";
      vc += str_itoa(len, lenBuf);
      if ( column->body.columns_desc.character_set == CharInfo::UTF8 ||
           ( column->body.columns_desc.character_set == CharInfo::SJIS &&
             column->body.columns_desc.encoding_charset == CharInfo::SJIS ) )
        {
          vc += " BYTE";
          if (len > 1)
            vc += "S";
        }
      vc += ") CHARACTER SET ";
      vc += CharInfo::getCharSetName(column->body.columns_desc.character_set);
      vc += ");";

      itemExpr = expGen->createExprTree(vc, CharInfo::UTF8, vc.length(), 1, itemExpr);
      itemExpr->synthTypeAndValueId();

      ((NAType *)&(itemExpr->getValueId().getType()))->
        setNullable(column->body.columns_desc.null_flag);
  }

  // add the encode node on top of it.
  short desc_flag = TRUE;
  if (key->body.keys_desc.ordering == 0) // ascending
    desc_flag = FALSE;
  
  itemExpr = new(expGen->wHeap()) CompEncode(itemExpr, desc_flag);
  
  itemExpr->synthTypeAndValueId();
  
  ((CompEncode*)itemExpr)->setCaseinsensitiveEncode(caseinsensitiveEncode);

  return itemExpr;
}

///////////////////////////////////////////////////////////////////
// This function takes as input an array of key values, where each
// key value is in ASCII string format (the way it is stored in
// catalogs). It encodes the key values and returns the encoded
// value in the encodedKeyBuffer. 
// RETURNS: -1, if error. 0, if all Ok.
///////////////////////////////////////////////////////////////////
short encodeKeyValues(desc_struct   * column_descs,
		      desc_struct   * key_descs,
		      NAString      * inValuesArray[],          // INPUT
		      char * encodedKeyBuffer,                  // OUTPUT
                      CollHeap * h,
		      ComDiagsArea * diagsArea)
{
  short error = 0;       // assume all will go well
  NABoolean deleteLater = FALSE;

  // set up binder/generator stuff so expressions could be generated.
  InitSchemaDB();
  CmpStatement cmpStatement(CmpCommon::context());
  ActiveSchemaDB()->createStmtTables();
  BindWA       bindWA(ActiveSchemaDB(), CmpCommon::context());
  Generator    generator(CmpCommon::context());
  ExpGenerator expGen(&generator);
  generator.appendAtEnd(); // alloc a new map table
  generator.setBindWA(&bindWA);
  generator.setExpGenerator(&expGen);
  FragmentDir * compFragDir = generator.getFragmentDir();

  // create the fragment (independent code space) for this expression
  CollIndex myFragmentId = compFragDir->pushFragment(FragmentDir::MASTER);
  
  // space where RCB will be generated
  Space * space = generator.getSpace();

  // Let's start with a list of size 4 rather than resizing continuously
  ValueIdList encodedValueIdList(4);
  desc_struct * column = column_descs;
  desc_struct * key    = key_descs;
  Int32 i = 0;

  if (inValuesArray == NULL)
    deleteLater = TRUE;

  while (key)
    {
      column = column_descs;
      for (Int32 j = 0; j < key->body.keys_desc.tablecolnumber; j++)
	column = column->header.next;

      if (inValuesArray[i] == NULL)
	inValuesArray[i] = getMinMaxValue(column, key, FALSE, h);
      
      ItemExpr * itemExpr = buildEncodeTree(column, key, inValuesArray[i],
					    &generator, diagsArea);
      if (! itemExpr)
	return -1;

      encodedValueIdList.insert(itemExpr->getValueId());
      
      i++;
      key = key->header.next;
    }
  
  // allocate a work cri desc to encode keys. It has
  // 3 entries: 0, for consts. 1, for temps. 
  // 2, for the encoded key.
  ex_cri_desc * workCriDesc = new(space) ex_cri_desc(3, space);
  short keyAtpIndex   = 2;  // where the encoded key will be built
  
  ULng32 encodedKeyLen;
  ex_expr * keExpr = 0;
  expGen.generateContiguousMoveExpr(encodedValueIdList,
				    0 /*don't add conv nodes*/,
				    0 /*atp*/, keyAtpIndex,
				    ExpTupleDesc::SQLMX_KEY_FORMAT,
				    encodedKeyLen,
				    &keExpr);

  // create a DP2 expression and initialize it with the key encode expr.
  ExpDP2Expr * keyEncodeExpr = new(space) ExpDP2Expr(keExpr,
						     workCriDesc,
						     space);
  
  keyEncodeExpr->getExpr()->fixup(0,expGen.getPCodeMode(), 
                           (ex_tcb *)space,space, h);

  atp_struct * workAtp = keyEncodeExpr->getWorkAtp();
  workAtp->getTupp(keyAtpIndex).setDataPointer(encodedKeyBuffer);
  
  if (keyEncodeExpr->getExpr()->eval(workAtp, 0, space) == ex_expr::EXPR_ERROR)
    error = -1;

  if (deleteLater)
    delete [] inValuesArray;

  generator.removeAll(NULL);

  return error;
}
