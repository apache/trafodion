/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1996-2014 Hewlett-Packard Development Company, L.P.
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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         GenRfork.C
 * Description:  
 *
 *
 * Created:      10/27/1996
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"

#include <ctype.h>
#include <math.h>
#include "CmpContext.h"
#include "CmpStatement.h"
#include "CmpMain.h"
#include "GenExpGenerator.h"
#include "GenRfork.h"
#include "Sqlcomp.h"
#include "ControlDB.h"
#include "CatLiterals.h"
#include "CatErrorCodes.h"
#include "CatGlobals.h"
#include "EncodedKeyValue.h"


#ifdef __CAT_RFORK
#include "ComObjectName.h"
#include "CatKeyList.h"
#include "CatPartitioningKeyList.h"
#include "CatColumnList.h"
#else
#include "BindWA.h"
#include "FragmentDir.h"
//#include "ex_ex.h"
#include "ExpCriDesc.h"
#include "ExpAtp.h"
#include "exp_clause_derived.h"
#include "exp_attrs.h"
#include "exp_tuple_desc.h"
#include "ComRCB.h"
#include "exp_dp2_expr.h"
#endif


extern
NABoolean createNAColumns(desc_struct *column_desc_list	/*IN*/,
			  NATable *table		/*IN*/,
			  NAColumnArray &colArray	/*OUT*/,
			  CollHeap *heap		/*IN*/);

extern
NABoolean createNAKeyColumns(desc_struct *key_desc_list	/*IN*/,
			     NAColumnArray &colArray	/*IN*/,
			     NAColumnArray &keyColArray /*OUT*/,
			     CollHeap *heap		/*IN*/);

// Not static functions, these are called from catman:

short generateRCB(const char    * object_name,
                  NABoolean       isEntrySeq,
                  desc_struct   * column_descs,
                  desc_struct   * key_descs,
                  desc_struct   * partitioningKey_descs,
                  NABoolean       isPartitioned,
                  NABoolean       partnClustKeySame,
                  void*         & rcbPtr,
                  ULng32 & rcbLen,
                  ULng32 & recordLen,
                  ULng32 & keyLen,
                  ULng32 & partitioningKeyLen,
                  NABoolean       doKeyEncodeOpt,
                  ComDiagsArea  * diagsArea,
                  ULng32   maxRecordLen,
                  NABoolean       newSqlmxRecordFormat,   // IN
                  COM_VERSION     objectSchemaVersion,
                  NABoolean       forIndex );

NAString ** createNAStringFromCharArray(char ** inValuesArray,
					Lng32 numKeys,
                                        CollHeap * h);


#ifdef __CAT_RFORK
static void convertCatColToDescStruct( const CatColumn * catCol,
                                       const ComObjectName * objectName,
				       desc_struct * column_desc,
				       NABoolean isMetadata)
{
  column_desc->body.columns_desc.tablename = 
    convertNAString(objectName->getExternalName(), HEAP);
  
  column_desc->body.columns_desc.colname =
    convertNAString(catCol->getColumnName(), HEAP);
  
  column_desc->body.columns_desc.colnumber = (Int32)catCol->getColumnNumber();
  column_desc->body.columns_desc.datatype  = catCol->getFSDataType();
  
  column_desc->body.columns_desc.length    = catCol->getColumnSize();
  column_desc->body.columns_desc.scale     = catCol->getScale();
  column_desc->body.columns_desc.precision = catCol->getPrecision();
  column_desc->body.columns_desc.datetimestart =
    (rec_datetime_field)catCol->getDateTimeStartField();
  column_desc->body.columns_desc.datetimeend =
    (rec_datetime_field)catCol->getDateTimeEndField();
  
  column_desc->body.columns_desc.datetimefractprec =
    (short)catCol->getDateTimeTrailingP();
  column_desc->body.columns_desc.intervalleadingprec =
    (short)catCol->getDateTimeLeadingP();
  
  column_desc->body.columns_desc.offset    = -1;
#pragma nowarn(1506)   // warning elimination 
  column_desc->body.columns_desc.null_flag = catCol->isNullIndicatorPresent();
#pragma warn(1506)  // warning elimination 
#pragma nowarn(1506)   // warning elimination 
  column_desc->body.columns_desc.upshift   = catCol->isUpshifted();
#pragma warn(1506)  // warning elimination 

  if ((DFS2REC::isAnyCharacter(column_desc->body.columns_desc.datatype)) &&
      (catCol->getCaseInsensitiveComparison()))
    column_desc->body.columns_desc.caseinsensitive = (short)TRUE;
  else
    column_desc->body.columns_desc.caseinsensitive = (short)FALSE;

  column_desc->body.columns_desc.character_set = catCol->getCharSet();
  column_desc->body.columns_desc.encoding_charset = catCol->getEncodingCharSet();
  column_desc->body.columns_desc.collation_sequence = catCol->getCollation();
  
  // Replace allocation of space for picture text and
  // the call to CmpDescribePictureText() by:
  //     column_desc->body.columns_desc.pictureText = 
  //            convertNAString(catCol->getPictureText(), HEAP);
  // after catalog manager has put in support to update
  // the pictureText field.
  //
  // Remember to do this when the following assertion fails!
  CMPASSERT(catCol->getPictureText().isNull());
  column_desc->body.columns_desc.pictureText =
    (char *)HEAP->allocateMemory(340);
  NAType::convertTypeToText(column_desc->body.columns_desc.pictureText,	 //OUT
			    column_desc->body.columns_desc.datatype,
			    column_desc->body.columns_desc.length,
			    column_desc->body.columns_desc.precision,
			    column_desc->body.columns_desc.scale,
			    column_desc->body.columns_desc.datetimestart,
			    column_desc->body.columns_desc.datetimeend,
			    column_desc->body.columns_desc.datetimefractprec,
			    column_desc->body.columns_desc.intervalleadingprec,
			    column_desc->body.columns_desc.upshift,
			    column_desc->body.columns_desc.caseinsensitive,
			    (CharInfo::CharSet)column_desc->body.columns_desc.character_set,
                            (CharInfo::Collation)column_desc->body.columns_desc.collation_sequence,
                            catCol->getDisplayDataType().data()
                            );
  
  if (catCol->getColumnClass() == COM_SYSTEM_COLUMN)
    column_desc->body.columns_desc.colclass = 'S';
  else if (catCol->getColumnClass() == COM_USER_COLUMN)
    column_desc->body.columns_desc.colclass = 'U';
  else if (catCol->getColumnClass() == COM_ADDED_USER_COLUMN)
    column_desc->body.columns_desc.colclass = 'A'; 
  else if (COM_MV_SYSTEM_ADDED_COLUMN == catCol->getColumnClass())
    column_desc->body.columns_desc.colclass = 'M'; //++ MV

  column_desc->body.columns_desc.uec     = (Cardinality)0;
  column_desc->body.columns_desc.highval = 0;
  column_desc->body.columns_desc.lowval  = 0;

  column_desc->body.columns_desc.defaultClass = catCol->getDefaultClass();

// UR2
  column_desc->body.columns_desc.defaultvalue = 
    convertNAString(catCol->getDefaultValue(), HEAP, TRUE);
  column_desc->body.columns_desc.stored_on_disk = catCol->getColumnOnDisk();

  const ComString *ccText = catCol->getComputedColumnExprText(NULL);

  if (ccText)
    column_desc->body.columns_desc.computed_column_text = 
      convertNAString(*ccText, HEAP);
  else
    column_desc->body.columns_desc.computed_column_text = NULL;
}

desc_struct * convertCatColListToDescStructs(const ComObjectName * objectName,
					     const CatColumnList * columnList,
					     NABoolean isMetadata)
{
  desc_struct * prev_column_desc  = NULL;
  desc_struct * first_column_desc = NULL;
  for (Int32 i = 0; i < columnList->entries(); i++)
  {
    CatColumn * catCol = (CatColumn *)columnList->getColumnPtr(i);

    // ignore this cat col, if its size has not been set.
    if (catCol->isColSizeNotSet())
      continue;

    // readtabledef_allocate_desc() requires that HEAP (STMTHEAP) 
    // be used for operator new herein
    
    desc_struct * column_desc = readtabledef_allocate_desc(DESC_COLUMNS_TYPE);
    if (prev_column_desc != NULL)
      prev_column_desc->header.next = column_desc;
    else
      first_column_desc = column_desc;      
    
    prev_column_desc = column_desc;
    convertCatColToDescStruct(catCol, objectName, column_desc, isMetadata);
  }

  return first_column_desc;
}

desc_struct * convertAccessListToDescStructs(const ComObjectName * objectName,
					     const CatColumnList * columnList,
					     const CatAccessPathColList *accessList,
					     NABoolean isMetadata)
{
  desc_struct * prev_column_desc  = NULL;
  desc_struct * first_column_desc = NULL;
  for (Int32 i = 0; i < accessList->entries(); i++)
    {
      const CatColumn *catCol =
	   columnList->getColumnPtr((*accessList)[i]->getColumnNumber() );
      
      // readtabledef_allocate_desc() requires that HEAP (STMTHEAP) 
      // be used for operator new herein

      desc_struct * column_desc = readtabledef_allocate_desc(DESC_COLUMNS_TYPE);
      if (prev_column_desc != NULL)
	prev_column_desc->header.next = column_desc;
      else
       first_column_desc = column_desc;      
      
      prev_column_desc = column_desc;
      
      convertCatColToDescStruct(catCol, objectName, column_desc, isMetadata);
  }

  return first_column_desc;
}

static desc_struct * convertCatKeyListToDescStructs(const CatKeyList &keyList,
						    const CatColumnList * columnList)
{
  desc_struct * prev_key_desc  = NULL;
  desc_struct * first_key_desc = NULL;
  for (Int32 i = 0; i < keyList.entries(); i++)
    {
      CatColumn * catCol = (CatColumn *)
          columnList->getColumnPtr((Int32) keyList[i]->getColumnNumber());

      // ignore this cat col, if its size has not been set.
      if (catCol->isColSizeNotSet())
        continue;

      desc_struct * key_desc = readtabledef_allocate_desc(DESC_KEYS_TYPE);
      if (prev_key_desc != NULL)
	prev_key_desc->header.next = key_desc;
      else
       first_key_desc = key_desc;      
      
      prev_key_desc = key_desc;
      
      key_desc->body.keys_desc.tablecolnumber = 
	(Int32) keyList[i]->getColumnNumber();

      key_desc->body.keys_desc.keyseqnumber = i;

      key_desc->body.keys_desc.ordering =
	((keyList[i]->getOrdering() == COM_ASCENDING_ORDER) ? 0 : -1);
    }

  return first_key_desc;
}

static desc_struct * convertCatPartitioningKeyListToDescStructs
		                     (const CatPartitioningKeyList &keyList,
						    const CatColumnList * columnList)
{
  desc_struct * prev_key_desc  = NULL;
  desc_struct * first_key_desc = NULL;
  const CatColumn * catCol = NULL;
  const CatKey * catKey = NULL;
  Int32 i = 0;
  Int32 numPartitionKeyCols = (Int32)keyList.entries();
  for (i = 0; i < numPartitionKeyCols; i++)
    {
      catKey = keyList[i];
      catCol = columnList->getColumnPtr((Int32) catKey->getColumnNumber());

      // ignore this cat col, if its size has not been set.
      if (catCol->isColSizeNotSet())
        continue;

      desc_struct * key_desc = readtabledef_allocate_desc(DESC_KEYS_TYPE);
      if (prev_key_desc != NULL)
	prev_key_desc->header.next = key_desc;
      else
       first_key_desc = key_desc;      
      
      prev_key_desc = key_desc;
      
      key_desc->body.keys_desc.tablecolnumber = 
        (Int32) catKey->getColumnNumber();
      
      key_desc->body.keys_desc.keyseqnumber = i;

      key_desc->body.keys_desc.ordering =
        ((catKey->getOrdering() == COM_ASCENDING_ORDER) ? 0 : -1);
    }

  return first_key_desc;
}

#endif

#ifndef __CAT_RFORK
static short generateKeyEncodeExpr(desc_struct * key_descs,
				   NAColumnArray &keyColArray,
				   ValueIdList &keyColValIdList,
				   NAColumnArray &colArray,
				   ValueIdList &colValIdList,
				   ex_cri_desc * workCriDesc,
				   short atp, short atpindex,
				   ExpDP2KeyEncodeExpr ** dp2KeyExpr,
				   ULng32 &keyLen,
				   NABoolean doKeyEncodeOpt,
				   Generator * generator)
{
  keyLen = 0;
  ULng32 firstKeyColumnOffset = 0;

  *dp2KeyExpr = NULL;

  NABoolean needKeyEncodeExpr = TRUE;
  if (doKeyEncodeOpt)
    {
      needKeyEncodeExpr = 
	generator-> getExpGenerator()->processKeyEncodingOptimization(
	     colArray,
	     keyColArray,
	     keyColValIdList,
	     0, /* primary key */
	     keyLen,
	     firstKeyColumnOffset);
    }

#ifdef _DEBUG
  if (getenv("NO_KEY_ENCODE_OPT"))
    needKeyEncodeExpr = TRUE;
#endif
    
  MapTable * mapTable = generator->getMapTable();
  
  ex_expr * keyEncodeExpr = NULL;

  desc_struct * key_desc = key_descs;
  ValueIdList encode_val_id_list;
  while (key_desc)
    {
      // Allocate encode node
      // to move the key value to the key buffer.
      ItemExpr * colNode = 
	colValIdList[key_desc->body.keys_desc.tablecolnumber].getItemExpr();
      const NAType &colType = colNode->getValueId().getType();

      NABoolean caseinsensitiveEncode = FALSE;
      if ((colType.getTypeQualifier() == NA_CHARACTER_TYPE) &&
	  (((CharType&)colType).isCaseinsensitive()))
	caseinsensitiveEncode = TRUE;
      
      NABoolean desc_flag;
      if (key_desc->body.keys_desc.ordering == 0) // ascending
	desc_flag = FALSE;
      else
	desc_flag = TRUE;
      
      if (colType.getVarLenHdrSize() > 0)
	{
	  // Explode varchars by moving them to a fixed field
	  // whose length is equal to the max length of varchar.
          
	  CharType& colCharType = (CharType&)colType;
	  
	  if (!CollationInfo::isSystemCollation(colCharType.getCollation()))
	  {
	    colNode = 
	    new(generator->wHeap()) 
	    Cast (colNode, 
		  (new(generator->wHeap())
		   SQLChar(CharLenInfo(colCharType.getStrCharLimit(), colCharType.getDataStorageSize()),
			   colCharType.supportsSQLnull(),
			   FALSE, FALSE, FALSE,
			   colCharType.getCharSet(),
			   colCharType.getCollation(),
			   colCharType.getCoercibility()
			   )));
	  }
	}
      
      CompEncode * enode = 
#pragma nowarn(1506)   // warning elimination 
	new(generator->wHeap()) CompEncode(colNode, desc_flag);
#pragma warn(1506)  // warning elimination 

      enode->bindNode(generator->getBindWA());    

      enode->setCaseinsensitiveEncode(caseinsensitiveEncode);
      
      encode_val_id_list.insert(enode->getValueId());
      
      key_desc = key_desc->header.next;
    }
  
  generator->getExpGenerator()->
    generateContiguousMoveExpr(encode_val_id_list, 
			       0, // don't add convert nodes,
			       atp,
			       atpindex,
			       ExpTupleDesc::SQLMX_KEY_FORMAT,
			       keyLen,
			       &keyEncodeExpr);

  // create a DP2 expression and initialize it with the key encode expr.
  *dp2KeyExpr =
    new(generator->getSpace()) ExpDP2KeyEncodeExpr(keyEncodeExpr,
						   workCriDesc,
						   generator->getSpace(),1);

  if (needKeyEncodeExpr == FALSE)
    {
      (*dp2KeyExpr)->setKeyEncodeOpt1(TRUE);
      (*dp2KeyExpr)->setFirstKeyColOffset((UInt16)firstKeyColumnOffset);
      (*dp2KeyExpr)->setKeyLen((UInt16)keyLen);
    }

  return 0;
}
#endif

#ifndef __CAT_RFORK
static short addDefaultValuesToRCB(Generator * generator,
				   NABoolean isEntrySeq,
				   NAColumnArray * colArray,
				   ExpTupleDesc * tupleDesc,
				   ExpTupleDesc::TupleDataFormat tdf,
				   ComDiagsArea * diagsArea, 
				   COM_VERSION     objectSchemaVersion)
{
  // move default values to RCB
  Int32 j = 0;
  NABoolean setAddedColumn = FALSE;

  for (Int32 i = 0; i < (Int32) colArray->entries(); i++)
    {
      NAColumn * col = (*colArray)[i];
      Attributes * attr = NULL;

      if (((isEntrySeq == FALSE)
           || (tdf == ExpTupleDesc::SQLMX_FORMAT)
           || (tdf == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
           || (! col->isSyskeyColumn()))
          && col->isStoredOnDisk())
	{
	  attr = tupleDesc->getAttr(j);
	  j++;
	  
	  short rc = generator->getExpGenerator()->
	    addDefaultValue(col, attr, diagsArea, objectSchemaVersion);
	  if (rc)
	    return rc;
	  
	} // add defval to tuple desc
      if (col->isAddedColumn())
	setAddedColumn = TRUE;
    } // for
  
  if (setAddedColumn)
    tupleDesc->setAddedField();

  return 0;
}
#endif

#ifndef __CAT_RFORK
// RETURNS: -1, if error. 0, if all Ok.
short generateRCB(const char    * object_name,
		  NABoolean       isEntrySeq,
		  desc_struct   * column_descs,
		  desc_struct   * key_descs,
		  desc_struct   * partitioningKey_descs,
		  NABoolean       isPartitioned,
		  NABoolean       partnClustKeySame,
		  void*         & rcbPtr,
		  ULng32 & rcbLen,
		  ULng32 & recordLen,
		  ULng32 & keyLen,
		  ULng32 & partitioningKeyLen,
		  NABoolean       doKeyEncodeOpt,
		  ComDiagsArea  * diagsArea,
		  ULng32   maxRecordLen,
		  NABoolean       newSqlmxRecordFormat,
                  COM_VERSION     objectSchemaVersion,
		  NABoolean       forIndex)
{
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
  
  // Start generation by creating the RCB. It will be initialized later.
  ExRCB * rcb = new(space) ExRCB();
  
  // Set the version to the current plan version.
  Lng32 rcbVersion = ComVersion_GetCurrentPlanVersion();


  rcb->setRcbVersion((COM_VERSION)rcbVersion);

  // create the NAColumnArray from column_descs.
  // This would create NATypes for each column.
  NAColumnArray colArray;
  if (createNAColumns(column_descs, 	
		      NULL,	// no NATable, it's irrelevant
		      colArray,
		      CmpCommon::statementHeap()))
    return -1; // error

  // allocate a work cri desc to extract and encode keys. It has
  // 4 entries: 0, for consts. 1, for temps. 2, for the table record.
  // 3, for the encoded key.
  ex_cri_desc * workCriDesc = new(space) ex_cri_desc(4, space);
  short workAtp = 0;
  short tupleAtpIndex = 2;  // where the base table record will be moved
  short keyAtpIndex   = 3;  // where the encoded key will be built
  
  ExpTupleDesc::TupleDataFormat tupleFormat = ExpTupleDesc::SQLMX_FORMAT;

  if ( newSqlmxRecordFormat )
    tupleFormat = ExpTupleDesc::SQLMX_ALIGNED_FORMAT;
  
  NABoolean sqlmpTable = FALSE;

#if !defined(NA_NSK) && defined(_DEBUG)
  // This for testing use only.
  // This allows SQLMP_FORMAT tables in conjunction with SQLMX_FORMAT tables
  // on NT only.
  // 
  if ( getenv("SQLMP_FORMAT") )
    {
      UInt32 objLen = strlen(object_name);
      if ((objLen > 2)
	  && (object_name[objLen - 1] == '_')
	  && (object_name[objLen - 2] == '_'))
	{
	  tupleFormat = ExpTupleDesc::SQLMP_FORMAT;
	  sqlmpTable = TRUE;
	}
    }
#endif  // NA_NSK
			 
  // create a list of ItemExprs to correspond to the columns.
  // This list will be used to generate the tuple descriptor and
  // the keyEncodeExpr.
  ValueIdList colValIdList;
  ValueIdList tupleValIdList;
  NAColumn *col;
  for (Int32 i = 0; i < (Int32)colArray.entries(); i++)
    {
      ItemExpr * colNode = new(HEAP)  NATypeToItem((NAType *)(colArray[i]->getType()));
      colNode->synthTypeAndValueId();
      colValIdList.insert(colNode->getValueId());
      col = colArray[i];

      // If this is an entry seq table, then the SYSKEY is not stored
      // as part of the tuple for SQLMP tables; do not add it to
      // tupleValIdList.  SQLMX tables do store the SYSKEY.
      if (((isEntrySeq == FALSE)
           || (tupleFormat == ExpTupleDesc::SQLMX_FORMAT)
           || (tupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
           || (! col->isSyskeyColumn()))
          && col->isStoredOnDisk())
	{
	  tupleValIdList.insert(colNode->getValueId());
	}
      else if (col->isComputedColumn())
        {
          // check for computed key columns that are not stored on disk
          desc_struct * key_desc = key_descs;

          while (key_desc)
            {
              if (key_desc->body.keys_desc.tablecolnumber = col->getPosition())
                {
                  // We don't support computed key columns that are not stored
                  // on disk.  At this point we would need to parse and bind
                  // the computed column expression (or get that parsed
                  // expression passed in), so we can generate the key
                  // encoding expression.
                  CMPASSERT(col->isStoredOnDisk());
                }
              key_desc = key_desc->header.next;
            }
        }
    }

  // Compute the length of the partitioning key
  ValueIdList encode_val_id_list;
  desc_struct * local_partitioningKey_descs = partitioningKey_descs;

   while (local_partitioningKey_descs)
    {
      // Allocate encode node
      // to move the key value to the key buffer.
      ItemExpr * colNode = 
		  colValIdList[local_partitioningKey_descs->body.keys_desc.tablecolnumber].getItemExpr();
      const NAType &colType = colNode->getValueId().getType();

      NABoolean caseinsensitiveEncode = FALSE;
      if ((colType.getTypeQualifier() == NA_CHARACTER_TYPE) &&
	  (((CharType&)colType).isCaseinsensitive()))
	caseinsensitiveEncode = TRUE;
      
      NABoolean desc_flag;
      if (local_partitioningKey_descs->body.keys_desc.ordering == 0) // ascending<
	desc_flag = FALSE;
      else
	desc_flag = TRUE;
      
	if (colType.getVarLenHdrSize() > 0)
	{
	  // Explode varchars by moving them to a fixed field
	  // whose length is equal to the max length of varchar.


         // 4/6/98: adding code segment to take care of different 
         // char set case.
          CharType& colCharType = (CharType&)colType;

	  if (!CollationInfo::isSystemCollation(colCharType.getCollation()))
	  {
	    colNode = new(generator.wHeap()) Cast (colNode, 
			      (new(generator.wHeap())
			           SQLChar(CharLenInfo(colCharType.getStrCharLimit(),
                                                       colCharType.getDataStorageSize()),
					   colCharType.supportsSQLnull(),
                                           FALSE, FALSE, FALSE,
                                           colCharType.getCharSet(),
                                           colCharType.getCollation(),
                                           colCharType.getCoercibility()
                                           )));
	  }
	}
      
      CompEncode * enode = 
#pragma nowarn(1506)   // warning elimination 
	new(generator.wHeap()) CompEncode(colNode, desc_flag);
#pragma warn(1506)  // warning elimination 

      enode->bindNode(generator.getBindWA());    
      
      enode->setCaseinsensitiveEncode(caseinsensitiveEncode);

      encode_val_id_list.insert(enode->getValueId());

      if ((isPartitioned) &&
	  (DFS2REC::isFloat(colType.getFSDatatype())))
	{
	  if (CmpCommon::getDefault(MARIAQUEST_PROCESS) == DF_OFF) {
	    // ERROR 1120 Cannot have float as a partitioning key...
	    *diagsArea
	      << DgSqlCode(-1120);
	    return -1;
	  }
	}
      else if ((colType.getFSDatatype() == REC_BLOB) ||
	       (colType.getFSDatatype() == REC_CLOB))
	{
	  // ERROR 1383 Cannot have lob as a primary key...
	  *diagsArea
	    << DgSqlCode(-1383);
	  return -1;
	}
      
      local_partitioningKey_descs = local_partitioningKey_descs->header.next;
    }
  ExpTupleDesc * dummyTupleDesc = NULL;
  if (expGen.processValIdList(encode_val_id_list,
			      ExpTupleDesc::SQLMX_KEY_FORMAT,
			      partitioningKeyLen,
			      workAtp, tupleAtpIndex,
			      &dummyTupleDesc, 
                              ExpTupleDesc::SHORT_FORMAT) == -1)
    return -1;
  // End of Computing the length of the partitioning key

  // now generate the tuple descriptor
  ExpTupleDesc * tupleDesc = NULL;
  if ( expGen.processValIdList(colValIdList,
                               tupleFormat,
                               recordLen,
                               workAtp, tupleAtpIndex,
                               &tupleDesc, ExpTupleDesc::LONG_FORMAT,
                               0, NULL,
                               (forIndex ? NULL : &colArray) )
       == -1)
    return -1;

  workCriDesc->setTupleDescriptor(tupleAtpIndex, tupleDesc);
  ExpTupleDesc * rcbTupleDesc = NULL;
  ULng32 tupleLen;
  if ( expGen.processValIdList(tupleValIdList,
                               tupleFormat,
                               tupleLen,
                               workAtp, tupleAtpIndex,
                               &rcbTupleDesc, ExpTupleDesc::LONG_FORMAT,
                               0, NULL,
                               (forIndex ? NULL : &colArray) )
      == -1)
    return -1;

  // VO, Fix genesis solution 10-040319-4367:
  //     Catch record lengths that are too large before we attempt
  //     to allocate RCB space for them
  if ((maxRecordLen > 0)      &&            // Don't bother if no max specified
      (recordLen - EXECUTOR_HEADER_SIZE > maxRecordLen)
     )
  {
     *diagsArea << DgSqlCode (-CAT_REC_LEN_TOO_LARGE)
                << DgInt0 ((Lng32)recordLen - EXECUTOR_HEADER_SIZE)
                << DgInt1 ((Lng32)maxRecordLen)
                << DgTableName (object_name);
     return -1;
  }

  if (addDefaultValuesToRCB(&generator, isEntrySeq,
			    &colArray, rcbTupleDesc, tupleFormat,
			    diagsArea, objectSchemaVersion))
    return -1;

  Queue * colNameList = NULL;
  if (expGen.genColNameList(colArray, colNameList))
    return -1;

  Int16 savedPCodeMode = (Int16)expGen.getPCodeMode();

  // For certain cases (i.e., variable field present and an added field in row)
  // do NOT generate any PCode.
  // 1 says generate NO PCode
  Int16 genNoPCode = (Int16)(rcbTupleDesc->addedFieldPresent() ? 1 : 0);

  if (genNoPCode)
    expGen.setPCodeMode(ex_expr::PCODE_NONE);

  // Check if key encoding optimization should be turned off.
  if ( rcbTupleDesc->addedFieldPresent() && rcbTupleDesc->isKeyShifted() )
    doKeyEncodeOpt = FALSE;

  // Key expressions in resource fork need to keep their clauses (e.g. showlabel
  // command).  So don't generate leaner expressions.
  expGen.setPCodeMode(expGen.getPCodeMode() | ex_expr::PCODE_NO_LEANER_EXPR);

  // create the NAColumnArrays from keys_descs and partKey_desc, 
  // if key encode opt is being done.
  NAColumnArray keyColArray;
  NAColumnArray partKeyColArray;
  ValueIdList keyColValIdList;
  ValueIdList partKeyColValIdList;
  ValueId vid;

  if (doKeyEncodeOpt)
    {
      if (createNAKeyColumns(key_descs, 	
			     colArray,
			     keyColArray,
			     CmpCommon::statementHeap()))
	return -1; // error
      
      // create a list of value ids corresponding to the key columns.
      // This list will be used to generate the keyEncodeExpr.
      for (Int32 ii = 0; ii < (Int32)keyColArray.entries(); ii++)
	{
	  vid = colValIdList[keyColArray[ii]->getPosition()];
	  keyColValIdList.insert(vid);
	}

      if (createNAKeyColumns(partitioningKey_descs, 	
			     colArray,
			     partKeyColArray,
			     CmpCommon::statementHeap()))
	return -1; // error
      
      // create a list of value ids corresponding to the 
      // partitioning key columns.
      // This list will be used to generate the keyEncodeExpr.
      for (Int32 jj = 0; jj < (Int32)partKeyColArray.entries(); jj++)
	{
	  vid = colValIdList[partKeyColArray[jj]->getPosition()];
	  partKeyColValIdList.insert(vid);
	}
    }

  // generate expression to extract and encode the clustering key and
  // the partitioning key (if different).
  ExpDP2KeyEncodeExpr * dp2KeyExpr = NULL;
  ExpDP2KeyEncodeExpr * partKeyExpr = NULL;

  if (key_descs && generateKeyEncodeExpr(key_descs, 
			    keyColArray,
			    keyColValIdList,
			    colArray,
			    colValIdList,
			    workCriDesc,
			    workAtp, keyAtpIndex,
			    &dp2KeyExpr, keyLen,
			    doKeyEncodeOpt,
			    &generator) == -1)
    return -1;

  if (!partnClustKeySame)
    {
      
      if (!partitioningKey_descs ||
	  (generateKeyEncodeExpr(partitioningKey_descs, 
			    partKeyColArray,
			    partKeyColValIdList,
			    colArray,
			    colValIdList,
			    workCriDesc,
			    workAtp, keyAtpIndex,
			    &partKeyExpr, partitioningKeyLen,
			    doKeyEncodeOpt,
			    &generator) == -1) )
	return -1;
    }
  else
    partitioningKeyLen = keyLen;

 // restore the saved PCode generation mode
  if (genNoPCode) expGen.setPCodeMode(savedPCodeMode);

  // Reset support for leaner expression generation
  expGen.setPCodeMode(expGen.getPCodeMode() & ~ex_expr::PCODE_NO_LEANER_EXPR);

  // allocate space to hold the object name in RCB
  char * oname = (char *)(space->allocateMemory(40));
  str_cpy(oname, object_name, 40, ' ');
  
  // find out the total allocated space
  ULng32 totalLen = space->getAllocatedSpaceSize();
  
  rcb->initialize(oname, 
		  isEntrySeq,
		  40,
		  totalLen, 
		  rcbTupleDesc, recordLen,
		  dp2KeyExpr, keyLen,
		  partKeyExpr, partitioningKeyLen,
		  colNameList, sqlmpTable,
		  space);

  
  if ((rcb->getKeyEncodeExpr()) &&
      (rcb->getKeyEncodeExpr()->getExpr()) &&
      (rcb->getKeyEncodeExpr()->getExpr()->getTempsLength() > 0))
    rcb->setUseGetKeyNew(FALSE);
  else
    rcb->setUseGetKeyNew(TRUE);

  if (partnClustKeySame)
    rcb->setPartnClustKeySame(TRUE);

  // pack rcb. This would convert pointers to offsets.
  ExRCBPtr(rcb).pack(space);

  // the generated RCB is generated in chunks internally by
  // the space class. Make it contiguous by allocating and
  // moving it to a contiguous area.
  rcb = (ExRCB *)(new char[totalLen]);
  space->makeContiguous((char *)rcb, totalLen);

  // return the generated RCB
  rcbPtr = (void *)rcb;
  rcbLen = totalLen;

  generator.removeAll(NULL);

  return 0; // ok.
}

short generateRCB(const ComObjectName * objectName,    // INPUT
		  const ComClusteringScheme clusScheme,// INPUT
		  const CatColumnList * columnList,    // INPUT
		  const CatKeyList    * keyList,       // INPUT
		  const CatPartitioningKeyList    * partitioningKeyList,    
                                                       // INPUT
		  NABoolean       isMetadata,          // INPUT
		  void*         & rcbPtr,              // OUTPUT
		  ULng32 & rcbLen,              // OUTPUT
		  ULng32 & recordLen,           // OUTPUT
		  ULng32 & keyLen,              // OUTPUT
		  ULng32 & partitioningKeyLen,  // OUTPUT
		  ComDiagsArea  * diagsArea)           // INPUT
{
	return generateRCB (objectName, 
	                       clusScheme,
	                       columnList,
	                       keyList,
	                       partitioningKeyList,
			       FALSE,
			       isMetadata,
	                       rcbPtr,
	                       rcbLen,
	                       recordLen,
	                       keyLen,
	                       partitioningKeyLen,
	                       diagsArea,
	                       0,
			       FALSE,
                               COM_VERS_UNKNOWN);
}

#endif

#ifndef __CAT_RFORK
// RETURNS: -1, if error. 0, if all Ok.
short generateCompositeVpRCB(ULng32 rcbCount, // input
			     void *rcbs[],            // input
			     void*         & rcbPtr,  // OUTPUT
			     ULng32 & rcbLen,  // OUTPUT
			     ComDiagsArea  * diagsArea)//INPUT
{
  CMPASSERT(rcbCount>1);

  // the root VP RCB must be first in the array
  ExRCB *rootSrcRcb = (ExRCB*) rcbs[0];

  // construct space object in which to allocate composite RCB
  Space *space = new Space(Space::GENERATOR_SPACE);
  if (!space) return -1;

  // Copy root VP RCB to memory in space object.
  //
  ULng32 rootRcbLen = rootSrcRcb->getLength();
  ExRCB *rootTgtRcb = (ExRCB*) new (space) char[rootRcbLen];
  if (!rootTgtRcb) 
    {
      delete space;
      return -1;
    }
#pragma nowarn(1506)   // warning elimination 
  str_cpy_all((char*)rootTgtRcb, (char*)rootSrcRcb, rootRcbLen);
#pragma warn(1506)  // warning elimination 


  // Here, we have just freshly prepared the rootTgtRcb using the latest
  // version of the compiler. Unpacking should be uneventful (i.e. no
  // object version migration should happen). Therefore, it should not
  // be necessary to reallocate space.
  //
  ExRCB *rootTgtRcbCopy = rootTgtRcb;

  ExRCB dummyRCB;
  if ( (rootTgtRcb = (ExRCB *)
        rootTgtRcb->driveUnpack((void *)rootTgtRcb,&dummyRCB,NULL)) == NULL )
  {
    // ERROR during unpacking. Shouldn't occur here since we have just
    // freshly prepared the rootTgtRcb using the latest version of the
    // compiler. This is an unexpected error.
    //
    delete space;
    return -1;
  }

  // Don't expect migration to occur since rootTgtRcb has been generated by
  // the most recent compiler.
  //
  CMPASSERT(rootTgtRcb == rootTgtRcbCopy);

  // Set VP RCB array pointer in root RCB.
  //
  ExRCB** vpRcbArr = new (space) ExRCB*[rcbCount-1];
  if (!rootTgtRcb) 
    {
      delete space;
      return -1;
    }

  rootTgtRcb->setVpRcbArray(rcbCount-1, vpRcbArr, space);

  // Copy individual VP RCB objects into root RCB.
  //
  for (ULng32 i=1; i<rcbCount; i++)
    {
      // get pointer and length
      ExRCB *vpSrcRcb = (ExRCB*) rcbs[i];
      ULng32 totLen = vpSrcRcb->getLength();

      // allocate target object and perform copy
      ExRCB *vpTgtRcb = (ExRCB*) new (space) char[totLen];
      if (!vpTgtRcb) 
	{
	  delete space;
	  return -1;
	}
#pragma nowarn(1506)   // warning elimination 
      str_cpy_all((char*)vpTgtRcb, (char*)vpSrcRcb, totLen);
#pragma warn(1506)  // warning elimination 

      // unpack copy relative to start of object, which will convert
      // offsets to pointers.  when the (composite) rootTgtRcb is
      // packed below, these pointers will be converted back to
      // offsets, but this time offsets will be relative to the start
      // of the composite object and not the individual VP RCB.
      //
      ExRCB *vpTgtRcbCopy = vpTgtRcb;

      // Again, we have just freshly prepared the vpTgtRcb using the latest
      // version of the compiler. Unpacking should be uneventful (i.e. no
      // object version migration should happen). Therefore, it should not
      // be necessary to reallocate space.
      //
      if ( (vpTgtRcb = (ExRCB *)
            vpTgtRcb->driveUnpack((void *)vpTgtRcb,&dummyRCB,NULL)) == NULL )
      {
        // ERROR during unpacking. Shouldn't occur here since we have just
        // freshly prepared the rootTgtRcb using the latest version of the
        // compiler. This is an unexpected error.
        //
        delete space;
        return -1;
      }

      // Don't expect migration to occur since rootTgtRcb has been
      // generated by the most recent compiler.
      CMPASSERT(vpTgtRcb == vpTgtRcbCopy);

      rootTgtRcb->setVpRcb(i-1, vpTgtRcb);
    }

  // find out the total allocated space
  rcbLen = space->getAllocatedSpaceSize();
  
  rootTgtRcb->setLength(rcbLen);
  rootTgtRcb->drivePack(space);

  rcbPtr = (ExRCB *)new char[rcbLen];
  if (!rcbPtr) 
    {
      delete space;
      return -1;
    }  
  space->makeContiguous((char *)rcbPtr, rcbLen);

  delete space;

  return 0; // successful return
}
#endif

#ifndef __CAT_RFORK
// RETURNS: -1, if error. 0, if all Ok.
short generateIndexRCB(const char    * object_name,
		       desc_struct   * column_descs,
		       desc_struct   * key_descs,
		       desc_struct   * partitioningKey_descs,
		       NABoolean       isPartitioned,
		       NABoolean       partnClustKeySame,
		       void*         & rcbPtr,             // OUTPUT
		       ULng32 & rcbLen,             // OUTPUT
		       ULng32 & recordLen,          // OUTPUT
		       ULng32 & keyLen,             // OUTPUT
		       ULng32 & partitioningKeyLen, //OUTPUT
		       ComDiagsArea  * diagsArea,
		       NABoolean       newSqlmxRecordFormat)

{
  // Adjust the index column list: 
  //   the tablecolumn number in the key descriptor corresponds
  //   to the column number in the descriptor list for the base table.
  //   Change them so they correspond to the column number of
  //   the index column descriptor list. Since all column of the
  //   index key descriptor list are also part of the index column 
  //   descriptor list, the column number is the same as the key sequence
  //   number.
  desc_struct * key_desc = key_descs;
  desc_struct * partitioningKey_desc = partitioningKey_descs;
  while (key_desc)
    {
      key_desc->body.keys_desc.tablecolnumber = 
	key_desc->body.keys_desc.keyseqnumber;
      
      key_desc = key_desc->header.next;
    }

  Int32 columnCount = 0;
  desc_struct * column_desc = column_descs;

  while (partitioningKey_desc)
    {
      // the tablecolumn number in the key descriptor corresponds
      // to the column number in the descriptor list for the base table.
      // Change them so they correspond to the column number of
      // the index column descriptor list. 

      column_desc = column_descs;
      columnCount = 0;
      while (column_desc)
        {
          // if the base table column number of this column is same
          // as the base table column number of the partitioning key
          // column, then we have found the index column which        
          // corresponds to this partitioning key column.
          if(column_desc->body.columns_desc.colnumber == 
                          partitioningKey_desc->body.keys_desc.tablecolnumber)
            {
              partitioningKey_desc->body.keys_desc.tablecolnumber = columnCount;
	      column_desc = NULL; // get out of the loop
            }
	  else
            column_desc = column_desc->header.next;
	  columnCount++;
        }
      
      partitioningKey_desc = partitioningKey_desc->header.next;
    }
  
  if (generateRCB(object_name, FALSE, column_descs, key_descs,
		  partitioningKey_descs, isPartitioned,
		  partnClustKeySame, 
		  rcbPtr, rcbLen, 
		  recordLen, keyLen, partitioningKeyLen, 
		  FALSE, // don't do key encode opt. Not supported for
		         // indexes yet.
		  diagsArea, 0, newSqlmxRecordFormat,COM_VERS_UNKNOWN,TRUE) == -1)
    return -1;
  
  return 0;
}
#endif

#ifdef __CAT_RFORK
//NA_LINUX #ifndef NA_UNIX
// RETURNS: -1, if error. 0, if all Ok.
short generateRCB(const ComObjectName * objectName,   // INPUT
		  const ComClusteringScheme clusScheme,// INPUT
		  const CatColumnList * columnList,   // INPUT
		  const CatKeyList    * keyList,      // INPUT
		  const CatPartitioningKeyList    * partitioningKeyList,    
                                                      // INPUT
		  NABoolean isPartitioned,            // INPUT
		  NABoolean isMetadata,               // INPUT
		  void*         & rcbPtr,             // OUTPUT
		  ULng32 & rcbLen,             // OUTPUT
		  ULng32 & recordLen,          // OUTPUT
		  ULng32 & keyLen,             // OUTPUT
		  ULng32 & partitioningKeyLen, // OUTPUT
		  ComDiagsArea  * diagsArea,
                  ULng32   maxRecordLen,
		  NABoolean       newSqlmxRecordFormat,
		  COM_VERSION     objectSchemaVersion)
{
  // create a list of desc_structs from keyList
  desc_struct * key_descs = convertCatKeyListToDescStructs(*keyList, columnList);

  desc_struct * partitioningKey_descs =  NULL;
  NABoolean partnClustKeySame = TRUE;

  if (partitioningKeyList != NULL)
  {
    // create a list of desc_structs from partitioningKeyList
    partitioningKey_descs = 
            convertCatPartitioningKeyListToDescStructs(*partitioningKeyList, columnList);

    // compare the partitioning and clustering keys.
    if (partitioningKeyList->entries() == keyList->entries())
      {
	for (Int32 i = 0; (i < keyList->entries() &&
			 partnClustKeySame); i++)
	  {
	    const CatKey * pk = (*partitioningKeyList)[i];
	    const CatKey * ck = (*keyList)[i];
	    if (!(*pk == ck))
	      {
		partnClustKeySame = FALSE;
	      }
	  }
      }
    else
      partnClustKeySame = FALSE;
  }

  NAString object_name(objectName->getExternalName());

  if (objectName->getNameSpace() == COM_INDEX_NAME || objectName->getNameSpace() == COM_GHOST_INDEX_NAME)
  {
    const CatAccessPathColList *accessList = keyList;
    desc_struct * column_descs = 
      convertAccessListToDescStructs(objectName,
				     columnList,
				     accessList,
				     isMetadata);
    return generateIndexRCB(object_name, column_descs, key_descs,
			    partitioningKey_descs, isPartitioned,
			    partnClustKeySame,
			    rcbPtr, rcbLen, 
			    recordLen, keyLen, partitioningKeyLen,
			    diagsArea,
			    newSqlmxRecordFormat);
  }
  else
  {
    desc_struct * column_descs = 
      convertCatColListToDescStructs(objectName,
				     columnList,
				     isMetadata);

    return generateRCB(object_name, (clusScheme == COM_ENTRY_SEQ_CLUSTERING),
		       column_descs, key_descs, 
		       partitioningKey_descs, isPartitioned,
		       partnClustKeySame,
		       rcbPtr, rcbLen, 
		       recordLen, keyLen, partitioningKeyLen,
		       TRUE, // support key encode opt
		       diagsArea, maxRecordLen,
		       newSqlmxRecordFormat, 
		       objectSchemaVersion,
		       FALSE);
  }
}
#endif  // __CAT_RFORK

#ifndef __CAT_RFORK
NAString ** createNAStringFromCharArray(char ** inValuesArray,
					Lng32 numKeys,
                                        CollHeap * h)
{
  NAString ** nastringArray = new (h) NAString * [numKeys];

  for (Int32 i = 0; i < numKeys; i++)
    {
      if (inValuesArray[i] != NULL)
	nastringArray[i] = new (h) NAString(inValuesArray[i], h);
      else
	nastringArray[i] = NULL;
    }
  
  return nastringArray;
}

#endif
				     
#ifdef __CAT_RFORK
///////////////////////////////////////////////////////////////////
// This function takes as input an array of key values, where each
// key value is in ASCII string format (the way it is stored in
// catalogs). It encodes the key values and returns the encoded
// value in the encodedKeyBuffer. 
// If inValuesArray is NULL, then low or high key is returned
// in encodedKeyBuffer depending on highKey flag (TRUE = highkey).
// RETURNS: -1, if error. 0, if all Ok.
///////////////////////////////////////////////////////////////////
short encodeKeyValues(const ComObjectName * objectName, // INPUT
		      const CatColumnList * columnList, // INPUT
                      const CatKeyList* keyList,        // INPUT
		      const CatPartitioningKeyList* partitioningKeyList,    
                                                        // INPUT
		      NABoolean isMetadata,             // INPUT
		      char* inValuesArray[],            // INPUT
		      char * encodedKeyBuffer,          // OUTPUT
		      NABoolean highKey,
                      NABoolean encodePartitioningKey,
                      CollHeap * h,
		      ComDiagsArea * diagsArea)
{
  short error = -1;    // assume error
  short keyCount = 0;

  // create a list of desc_structs from columnList
  desc_struct * column_descs = 
    convertCatColListToDescStructs(objectName,
				   columnList,
				   isMetadata);
  
  // create a list of desc_structs from keyList
  desc_struct * key_descs;
  if (encodePartitioningKey)
  {
    key_descs = 
      convertCatPartitioningKeyListToDescStructs(*partitioningKeyList, columnList);
    keyCount = (short)partitioningKeyList->entries();
  }
  else
  {
    key_descs = convertCatKeyListToDescStructs(*keyList, columnList);
	keyCount = (short)keyList->entries();
  }

  // if no inValuesArray has been passed in, then create one.
  NAString ** inArray = NULL;
  if (inValuesArray == NULL)
    {
      inArray = createInArrayForLowOrHighKeys(column_descs, key_descs,
					      keyCount,
					      highKey, 
                                              FALSE, // isIndex
                                              h);
    }
  else
    {
      inArray = createNAStringFromCharArray(inValuesArray, keyCount, h);
    }

  if (inArray != NULL)
    {
      error = encodeKeyValues(column_descs, key_descs, inArray, FALSE, //IsIndex
			      encodedKeyBuffer, h, diagsArea);
      if (h == NULL)
      {
        for (ComSInt32 i = 0; i < keyCount; i++)
          delete inArray[i];
        delete [] inArray;
      } 
    }

  return error;
}
////////////////////////////////////////////////////////////////////
//
// Simplified version of the above, for hash key values.                                                                
// It creates one hard-coded version of an integer "key" column.
//
///////////////////////////////////////////////////////////////////
short encodeHashKeyValues(char* inValuesArray[],            // INPUT
                          char* encodedKeyBuffer,          // OUTPUT
			  ComDiagsArea  * diagsArea)       // INPUT
{

  short error = -1;    // assume error

  // create one desc struct for an INT column

  desc_struct  column;

  memset (&column, 0, sizeof (desc_struct));
        
  column.body.columns_desc.null_flag = 0;
  column.body.columns_desc.pictureText = (char *) "INT";
  column.body.columns_desc.datatype = REC_BIN32_SIGNED;
  column.header.next = 0;

  desc_struct key;
  memset (&key, 0, sizeof (desc_struct));

  NAString ** inArray = NULL;

  inArray = createNAStringFromCharArray(inValuesArray, 1, (CollHeap*)0);

  if (inArray != NULL)
    {
      error = encodeKeyValues(&column, &key, inArray, FALSE, //IsIndex
                              encodedKeyBuffer, (CollHeap*)0, diagsArea);
      delete inArray[0];
      delete [] inArray;
    }

  return error;
}

#endif

#ifndef __CAT_RFORK
/*
static void writeOut(FILE * f, char * str)
{
  if (str != NULL)
    {
#pragma nowarn(1506)   // warning elimination 
      int rc = fwrite(str, strlen(str), 1, f);
#pragma warn(1506)  // warning elimination 
      if (rc == 0)
	// assert. TBD.
	;
      
      rc = fprintf(f, "\n");
      if (rc == 0)
	// assert. TBD.
	;

      cout << str << "\n";
    }
} */

/////////////////////////////////////////////////////////////////////////
// The DDL for a ResourceFork is (from fs_rfork.h):
// CREATE TABLE RESOURCEFORK (
// (
//    RFSECTION                      SMALLINT UNSIGNED NO DEFAULT NOT NULL
//  , RFSUBSECTION                   SMALLINT UNSIGNED NO DEFAULT NOT NULL
//  , RESERVE1                       SMALLINT DEFAULT NULL
//  , RESERVE2                       SMALLINT DEFAULT NULL
//  , RESERVE3                       INT DEFAULT NULL
//  , RESERVE4                       INT DEFAULT NULL
//  , SECTIONLENGTH                  INT DEFAULT NULL
//  , DATA                           VARCHAR(3992) DEFAULT NULL
//  , PRIMARY KEY (RFSECTION, RFSUBSECTION)
//  );
////////////////////////////////////////////////////////////////////////

static const columns_desc_struct rfork_col_descs[] =
{
//
// columns_desc_struct is defined in sqlcat/desc.h:
//
// tablename colname colnum datatype pictureText length
// scale precision dtstart dtend dtfract intlead offset null_flag
// upshift colclass addedCol cardinality highval lowval defclass defval heading
// charset collation
//
  #define RF_SCALE_TO_OFFSET						    \
	    0, 0, REC_DATE_NotApplicable, REC_DATE_NotApplicable, 0, 0, 0
  #define RF_UPSHIFT_TO_COLLATION					    \
	    0, 0, 'U', 0, (Cardinality)0, 0, 0, COM_NO_DEFAULT, 0, 0,	    \
	    CharInfo::DefaultCharSet, CharInfo::DefaultCharSet,             \
            CharInfo::DefaultCollation
  #define RF_COL(colname, colnum, datatype, picText, len, nullable)	    \
	    { 0, colname, colnum, datatype, picText, len,		    \
	      RF_SCALE_TO_OFFSET, nullable, RF_UPSHIFT_TO_COLLATION }

  RF_COL((char *) "RFSECTION",     0, REC_BIN16_UNSIGNED, (char *) "SMALLINT UNSIGNED", 2, 0),
  RF_COL((char *) "RFSUBSECTION",  1, REC_BIN16_UNSIGNED, (char *) "SMALLINT UNSIGNED", 2, 0),
  RF_COL((char *) "RESERVE1",      2, REC_BIN16_SIGNED,   0, 2, -1),
  RF_COL((char *) "RESERVE2",      3, REC_BIN16_SIGNED,   0, 2, -1),
  RF_COL((char *) "RESERVE3",      4, REC_BIN32_SIGNED,   0, 4, -1),
  RF_COL((char *) "RESERVE4",      5, REC_BIN32_SIGNED,   0, 4, -1),
  RF_COL((char *) "SECTIONLENGTH", 6, REC_BIN32_SIGNED,   0, 4, -1),
  RF_COL((char *) "DATA",          7, REC_BYTE_V_ASCII,   0, 3992, -1)
};

// keys_desc_struct is defined in sqlcat/desc.h
static const keys_desc_struct rfork_key_descs[] =
{    
  // indexname keyname keyseqnumber tablecolnumber ordering
  {    0,                     0,          1,                         0,            0  },
  {    0,                     0,          2,                         1,            0  }
};
    
static void initColumnDescStruct(columns_desc_struct * tgt,
				 const columns_desc_struct * src)
{
  tgt->colname   = src->colname;
  tgt->colnumber = src->colnumber;
  tgt->datatype  = src->datatype;
  tgt->pictureText = src->pictureText;
  tgt->length    = src->length;
  tgt->scale     = src->scale;
  tgt->precision = src->precision;
  tgt->colclass  = src->colclass;
  tgt->null_flag = src->null_flag;
  tgt->upshift   = src->upshift;
  tgt->caseinsensitive   = src->caseinsensitive;
  tgt->defaultClass = src->defaultClass;
  tgt->defaultvalue = src->defaultvalue;
  tgt->character_set = src->character_set;
  tgt->encoding_charset = src->encoding_charset;
  tgt->collation_sequence = src->collation_sequence;
}

static void initKeyDescStruct(keys_desc_struct * tgt, Int32 keyIndex)
{
  tgt->keyseqnumber     = rfork_key_descs[keyIndex].keyseqnumber;
  tgt->tablecolnumber   = rfork_key_descs[keyIndex].tablecolnumber;
  tgt->ordering         = rfork_key_descs[keyIndex].ordering;
}

static short createDescStructsForRfork(char * rforkName,
				       desc_struct* &colDescs,
				       desc_struct* &keyDescs)
{
  // create columns descs
  desc_struct * prev_column_desc  = NULL;
  desc_struct * first_column_desc = NULL;
  for (Int32 colNum = 0; colNum < 8; colNum++)
    {
      desc_struct * column_desc = readtabledef_allocate_desc(DESC_COLUMNS_TYPE);
      if (prev_column_desc != NULL)
	prev_column_desc->header.next = column_desc;
      else
       first_column_desc = column_desc;      
      
      prev_column_desc = column_desc;

      initColumnDescStruct(&(column_desc->body.columns_desc),
			   &(rfork_col_descs[colNum]));
     
      column_desc->body.columns_desc.stored_on_disk = 1; 
      column_desc->body.columns_desc.tablename = rforkName;
    }

  // create key descs
  desc_struct * prev_key_desc  = NULL;
  desc_struct * first_key_desc = NULL;
  for (Int32 keyNum = 0; keyNum < 2; keyNum++)
    {
      desc_struct * key_desc = readtabledef_allocate_desc(DESC_KEYS_TYPE);
      if (prev_key_desc != NULL)
	prev_key_desc->header.next = key_desc;
      else
       first_key_desc = key_desc;      
      
      prev_key_desc = key_desc;

      initKeyDescStruct(&(key_desc->body.keys_desc), keyNum);
    }
  
  colDescs = first_column_desc;
  keyDescs = first_key_desc;
  
  return 0;
}

desc_struct * generateRforkTableDesc(const CorrName *rforkName)
{
  // readtabledef_allocate_desc() requires that HEAP (STMTHEAP) 
  // be used for operator new herein

  NAString rforkNameStr(rforkName->getQualifiedNameAsString());
  char * objName = (char *) HEAP->allocateMemory(rforkNameStr.length()+1);
  memcpy(objName, rforkNameStr.data(), rforkNameStr.length());
  objName[rforkNameStr.length()] = '\0';

  desc_struct * table_desc = readtabledef_allocate_desc(DESC_TABLE_TYPE);
  table_desc->body.table_desc.tablename = objName;

  table_desc->body.table_desc.record_length = 4036;
  table_desc->body.table_desc.colcount = 8;
  table_desc->body.table_desc.issystemtablecode = 1;
  table_desc->body.table_desc.underlyingFileType = SQLMX;

  desc_struct * files_desc = readtabledef_allocate_desc(DESC_FILES_TYPE);
  files_desc->body.files_desc.audit = -1; // audited table
 
  table_desc->body.table_desc.files_desc = files_desc;
  files_desc->body.files_desc.fileorganization = KEY_SEQUENCED_FILE;
  
  desc_struct * cols_descs = NULL;
  desc_struct * keys_descs = NULL;
  createDescStructsForRfork(objName, cols_descs, keys_descs);
  
  desc_struct * index_desc = readtabledef_allocate_desc(DESC_INDEXES_TYPE);
  index_desc->body.indexes_desc.tablename = objName;
  index_desc->body.indexes_desc.indexname = objName;
  index_desc->body.indexes_desc.keytag = 0; // primary index
  index_desc->body.indexes_desc.record_length = table_desc->body.table_desc.record_length;
  index_desc->body.indexes_desc.colcount = table_desc->body.table_desc.colcount;
  index_desc->body.indexes_desc.isVerticalPartition = 0;
  index_desc->body.indexes_desc.blocksize = 4096; // doesn't matter.

  // cannot simply point to same files desc as the table one,
  // because then ReadTableDef::deleteTree frees same memory twice (error)
  desc_struct * i_files_desc = readtabledef_allocate_desc(DESC_FILES_TYPE);
  i_files_desc->body.files_desc.audit = -1; // audited table

  index_desc->body.indexes_desc.files_desc = i_files_desc;
  i_files_desc->body.files_desc.fileorganization = KEY_SEQUENCED_FILE;

  index_desc->body.indexes_desc.keys_desc  = keys_descs;
  table_desc->body.table_desc.columns_desc = cols_descs;
  table_desc->body.table_desc.indexes_desc = index_desc;
 
  return table_desc;
}

///////////////////////////////////////////////////////////////////////
// This function generates an RCB for a Resource Fork. RFork is a SQL
// table that is not registered in catalog and has a fixed, pre-determined
// DDL. See function createDescStructsForRfork.
//
// This function first generates an RCB and then produces the file
// ex_rfork_rcb.h in ../executor containing 
// the actual RCB. The generated code out here is 'written' out to that
// file as a readonly array. 
// This rcb array is returned to filesystem at runtime.
// See ex_rfork.C for details.
//
// Doing a 'make genExRFork' from bin creates the final object genExRFork
// which can be run to create a new version of ex_rfork.C.
//
// IMPORTANT NOTE: After first release, if RCB structure or any of the
// classes contained in it is changed, or resource fork structure is
// is changed, then this method has to be changed. Also, the genExRfork
// utility has to be run again. And versioning code needs to be put in.
// 
////////////////////////////////////////////////////////////////////////////
short generateRCBforResourceFork(void *& rcbPtr,
                                 ULng32 & rcbLen,
                                 ULng32 & recordLen,
                                 ULng32 & keyLen)
{
  // create objectName, keyDescs and colDescs that corresponds to
  // the resource fork structure. 
  char * objectName      = NULL;
  desc_struct * colDescs = NULL;
  desc_struct * keyDescs = NULL;
  desc_struct * dummyDescs = NULL;

  if (createDescStructsForRfork(objectName, colDescs, keyDescs) == -1)
    return -1;
  
  // allocate a dummy name for rfork. The real name will be filled
  // in at runtime when the RCB for resource fork is returned by
  // executor to filesystem.
  char dummyName = '\0';
  objectName = &dummyName;
  
  // Call generateRCB to get the RCB.
  rcbPtr = NULL;
  rcbLen = 0;
  recordLen = 0;
  keyLen = 0;
  ULng32 dummyLen = 0;
  if (generateRCB(objectName, FALSE, colDescs, keyDescs, dummyDescs,
		  FALSE, TRUE,
    		  rcbPtr, rcbLen, recordLen, keyLen, dummyLen, 
		  FALSE, NULL, 0, NULL,COM_VERS_UNKNOWN, FALSE) == -1)
    return -1;

  return 0;

}

#endif
