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
* File:         NAColumn.C
* Description:  Methods on the Non-Alcoholic Column class
* Created:      4/27/94
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "Platform.h"
#include "NATable.h"
#include "Sqlcomp.h"
#include "ex_error.h"
#include "TrafDDLdesc.h"

// -----------------------------------------------------------------------
// Can't be inline because then circular #include-dependencies
// would obtain between NAColumn.h and NATable.h
// -----------------------------------------------------------------------
const QualifiedName* NAColumn::getTableName(NABoolean okIfNoTable) const
{
  if (table_) return &table_->getTableName();
  if (!okIfNoTable) CMPASSERT(okIfNoTable);
  return NULL;
}

NABoolean NAColumn::isNumeric() const
{
  if(!type_)
    return FALSE;

  return type_->isNumeric();
}

NAColumn::~NAColumn()
{}

NABoolean NAColumn::operator==(const NAColumn& other) const
{
  return ((getColName() == other.getColName()) &&
	  (*getType() == *other.getType()));
}


NABoolean NAColumn::operator==(const NAString& otherColName) const
{
  return (getColName() == otherColName);
}

void NAColumn::deepDelete()
{
  if(defaultValue_)
  NADELETEBASIC(defaultValue_,heap_);
  if(heading_)
  NADELETEBASIC(heading_,heap_);
  delete type_;
  delete isNotNullNondroppable_;
  if (computedColumnExpression_)
    NADELETEBASIC(computedColumnExpression_,heap_);
}
//-------------------------------------------------------------------------
// static NAColumn::deepCopy
// Makes a new NAColumn then makes a deepCopy of it char pointer members and
// NAType pointer member
//-------------------------------------------------------------------------

NAColumn *NAColumn::deepCopy(const NAColumn & nac, NAMemory * heap)
{
  NAColumn *column = new(heap) NAColumn(nac,heap);

  if (nac.defaultValue_)
  {
    UInt32 length = strlen(nac.defaultValue_);
    char* newDefaultValue = new (heap) char[length+1];
    strcpy(newDefaultValue, (char*)nac.defaultValue_);
    column->defaultValue_ = (char*)newDefaultValue;
  }

  if(nac.heading_)
  {
    Int32 length = str_len(nac.heading_) + 1;
    column->heading_ = new(heap) char[length];
    memcpy(column->heading_,nac.heading_,length);
  }
  if(nac.type_)
    column->type_ = nac.type_->newCopy(heap);
  if(nac.isNotNullNondroppable_)
    column->isNotNullNondroppable_= (CheckConstraint *)nac.isNotNullNondroppable_->copyTopNode(NULL,heap);
  if(nac.computedColumnExpression_)
  {
    Int32 length = str_len(nac.computedColumnExpression_) + 1;
    column->computedColumnExpression_ = new(heap) char[length];
    memcpy(column->computedColumnExpression_,nac.computedColumnExpression_,length);
  }

  return column;
}

// -----------------------------------------------------------------------
// See NAColumn.h and BindRelExpr.C (bindCheckConstraint function)
// for the reason for these two methods.
// NAColumn::getNotNullViolationCode() should be kept in synch with
// TableDesc::addCheckConstraint().
// -----------------------------------------------------------------------

Lng32 NAColumn::getNotNullViolationCode() const
{
  if (!isNotNullNondroppable_)
    return 0;						// no error

  if (!isNotNullNondroppable_->isViewWithCheckOption())
    return EXE_TABLE_CHECK_CONSTRAINT;

  if (isNotNullNondroppable_->isTheCascadingView())
    return EXE_CHECK_OPTION_VIOLATION;			// cascadING WCO view
  else
    return EXE_CHECK_OPTION_VIOLATION_CASCADED;		// cascadED WCO view
}

const QualifiedName& NAColumn::getNotNullConstraintName() const
{
  CMPASSERT(isNotNullNondroppable_);
  return isNotNullNondroppable_->getConstraintName();
}

static NAString makeTableName(const NATable *table,
			      const TrafColumnsDesc *column_desc)
{
  return NAString(
       table ?
       table->getTableName().getQualifiedNameAsAnsiString().data() : "");
}

static NAString makeColumnName(const NATable *table,
			       const TrafColumnsDesc *column_desc)
{
  NAString nam(makeTableName(table, column_desc));
  if (!nam.isNull()) nam += ".";
  nam += column_desc->colname;
  return nam;
}

// -----------------------------------------------------------------------
// Method for creating NAType from TrafDesc.
// -----------------------------------------------------------------------
NABoolean NAColumn::createNAType(TrafColumnsDesc *column_desc	/*IN*/,
				 const NATable *table  		/*IN*/,
				 NAType *&type       		/*OUT*/,
				 NAMemory *heap			/*IN*/,
				 Lng32 * errorCode
				 )
{
  //
  // Compute the NAType for this column
  //
  #define REC_INTERVAL REC_MIN_INTERVAL

  Int16 datatype = column_desc->datatype;
  if (REC_MIN_INTERVAL <= datatype && datatype <= REC_MAX_INTERVAL)
    datatype = REC_INTERVAL;

  Lng32 charCount = column_desc->length;

  if ( DFS2REC::isAnyCharacter(column_desc->datatype) )
  {
     if ( CharInfo::isCharSetSupported(column_desc->characterSet()) == FALSE ) {
       if (!errorCode)
       {
         *CmpCommon::diags() << DgSqlCode(-4082)
	       << DgTableName(makeTableName(table, column_desc));
       }
       else
       {
         *errorCode = 4082;
       }
       return TRUE; // error
     }

     if ( CharInfo::is_NCHAR_MP(column_desc->characterSet()) )
        charCount /= SQL_DBCHAR_SIZE;
  }

  switch (datatype)
    {

    case REC_BPINT_UNSIGNED :
      type = new (heap)
      SQLBPInt(heap, column_desc->precision, column_desc->isNullable(), FALSE);
      break;

    case REC_BIN8_SIGNED:
      if (column_desc->precision > 0)
	type = new (heap)
	SQLNumeric(heap, column_desc->length,
		   column_desc->precision,
		   column_desc->scale,
		   TRUE,
		   column_desc->isNullable()
		   );
      else
	type = new (heap)
	SQLTiny(heap, TRUE,
		 column_desc->isNullable()
		 );
      break;
    case REC_BIN8_UNSIGNED:
      if (column_desc->precision > 0)
	type = new (heap)
	SQLNumeric(heap, column_desc->length,
		   column_desc->precision,
		   column_desc->scale,
		   FALSE,
		   column_desc->isNullable()
		   );
      else
	type = new (heap)
	SQLTiny(heap, FALSE,
		 column_desc->isNullable()
		 );
      break;

   case REC_BIN16_SIGNED:
      if (column_desc->precision > 0)
	type = new (heap)
	SQLNumeric(heap, column_desc->length,
		   column_desc->precision,
		   column_desc->scale,
		   TRUE,
		   column_desc->isNullable()
		   );
      else
	type = new (heap)
	SQLSmall(heap, TRUE,
		 column_desc->isNullable()
		 );
      break;
    case REC_BIN16_UNSIGNED:
      if (column_desc->precision > 0)
	type = new (heap)
	SQLNumeric(heap, column_desc->length,
		   column_desc->precision,
		   column_desc->scale,
		   FALSE,
		   column_desc->isNullable()
		   );
      else
	type = new (heap)
	SQLSmall(heap, FALSE,
		 column_desc->isNullable()
		 );
      break;

    case REC_BIN32_SIGNED:
      if (column_desc->precision > 0)
	type = new (heap)
	SQLNumeric(heap, column_desc->length,
		   column_desc->precision,
		   column_desc->scale,
		   TRUE,
		   column_desc->isNullable()
		   );
      else
	type = new (heap)
	SQLInt(heap, TRUE,
	       column_desc->isNullable()
	       );
      break;
    case REC_BIN32_UNSIGNED:
      if (column_desc->precision > 0)
	type = new (heap)
	SQLNumeric(heap, column_desc->length,
		   column_desc->precision,
		   column_desc->scale,
		   FALSE,
		   column_desc->isNullable()
		   );
      else
	type = new (heap)
	SQLInt(heap, FALSE,
	       column_desc->isNullable()
	       );
      break;
    case REC_BIN64_SIGNED:
      if (column_desc->precision > 0)
	type = new (heap)
	SQLNumeric(heap, column_desc->length,
		   column_desc->precision,
		   column_desc->scale,
		   TRUE,
		   column_desc->isNullable()
		   );
      else
	type = new (heap)
	SQLLargeInt(heap, TRUE,
		    column_desc->isNullable()
		    );
      break;
    case REC_BIN64_UNSIGNED:
      if (column_desc->precision > 0)
	type = new (heap)
	SQLNumeric(heap, column_desc->length,
		   column_desc->precision,
		   column_desc->scale,
		   FALSE,
		   column_desc->isNullable()
		   );
      else
	type = new (heap)
        SQLLargeInt(heap, FALSE,
		    column_desc->isNullable()
		    );
      break;
    case REC_DECIMAL_UNSIGNED:
      type = new (heap)
	SQLDecimal(heap, column_desc->length,
		   column_desc->scale,
		   FALSE,
		   column_desc->isNullable()
		   );
      break;
    case REC_DECIMAL_LSE:
      type = new (heap)
	SQLDecimal(heap, column_desc->length,
		   column_desc->scale,
		   TRUE,
		   column_desc->isNullable()
		   );
      break;
    case REC_NUM_BIG_UNSIGNED:
      type = new (heap)
	SQLBigNum(heap, column_desc->precision,
		  column_desc->scale,
		  TRUE, // is a real bignum
		  FALSE,
		  column_desc->isNullable()
		  );
      break;
    case REC_NUM_BIG_SIGNED:
      type = new (heap)
	SQLBigNum(heap, column_desc->precision,
		  column_desc->scale,
		  TRUE, // is a real bignum
		  TRUE,
		  column_desc->isNullable()
		  );
      break;

    case REC_FLOAT32:
      type = new (heap)
	SQLReal(heap, column_desc->isNullable(), column_desc->precision);
      break;

    case REC_FLOAT64:
      type = new (heap)
	SQLDoublePrecision(heap, column_desc->isNullable(), column_desc->precision);
      break;

    case REC_BYTE_F_DOUBLE:
      charCount /= SQL_DBCHAR_SIZE;	    // divide the storage length by 2
      type = new (heap)
	SQLChar(heap, charCount,
		column_desc->isNullable(),
		column_desc->isUpshifted(),
		column_desc->isCaseInsensitive(),
		FALSE,
		column_desc->characterSet(),
		column_desc->collationSequence(),
		CharInfo::IMPLICIT
		);
      break;

    case REC_BYTE_F_ASCII:
      if (column_desc->characterSet() == CharInfo::UTF8 ||
          (column_desc->characterSet() == CharInfo::SJIS &&
           column_desc->encodingCharset() == CharInfo::SJIS))
      {
        Lng32 maxBytesPerChar = CharInfo::maxBytesPerChar(column_desc->characterSet());
        Lng32 sizeInChars = charCount ;  // Applies when CharLenUnit == BYTES
        if ( column_desc->precision > 0 )
           sizeInChars = column_desc->precision;
        type = new (heap)
	SQLChar(heap, CharLenInfo(sizeInChars, charCount/*in_bytes*/),
		column_desc->isNullable(),
		column_desc->isUpshifted(),
		column_desc->isCaseInsensitive(),
		FALSE, // varLenFlag
		column_desc->characterSet(),
		column_desc->collationSequence(),
		CharInfo::IMPLICIT, // Coercibility
		column_desc->encodingCharset()
		);
      }
      else // keep the old behavior
      type = new (heap)
	SQLChar(heap, charCount,
		column_desc->isNullable(),
		column_desc->isUpshifted(),
		column_desc->isCaseInsensitive(),
		FALSE,
		column_desc->characterSet(),
		column_desc->collationSequence(),
		CharInfo::IMPLICIT
		);
      break;

    case REC_BYTE_V_DOUBLE:
      charCount /= SQL_DBCHAR_SIZE;	    // divide the storage length by 2
      // fall thru
    case REC_BYTE_V_ASCII:
      if (column_desc->characterSet() == CharInfo::SJIS ||
          column_desc->characterSet() == CharInfo::UTF8)
      {
        Lng32 maxBytesPerChar = CharInfo::maxBytesPerChar(column_desc->characterSet());
        Lng32 sizeInChars = charCount ;  // Applies when CharLenUnit == BYTES
        if ( column_desc->precision > 0 )
           sizeInChars = column_desc->precision;
        type = new (heap)
	SQLVarChar(heap, CharLenInfo(sizeInChars, charCount/*in_bytes*/),
		   column_desc->isNullable(),
		   column_desc->isUpshifted(),
		   column_desc->isCaseInsensitive(),
		   column_desc->characterSet(),
		   column_desc->collationSequence(),
		   CharInfo::IMPLICIT, // Coercibility
		   column_desc->encodingCharset()
		   );
      }
      else // keep the old behavior
      type = new (heap)
	SQLVarChar(heap, charCount,
		   column_desc->isNullable(),
		   column_desc->isUpshifted(),
		   column_desc->isCaseInsensitive(),
		   column_desc->characterSet(),
		   column_desc->collationSequence(),
		   CharInfo::IMPLICIT
		   );
      break;

    case REC_BYTE_V_ASCII_LONG:
      type = new (heap)
	SQLLongVarChar(heap, charCount,
		       FALSE,
		       column_desc->isNullable(),
		       column_desc->isUpshifted(),
		       column_desc->isCaseInsensitive(),
		       column_desc->characterSet(),
		       column_desc->collationSequence(),
		       CharInfo::IMPLICIT
		      );
      break;
    case REC_DATETIME:
      type = DatetimeType::constructSubtype(
					    column_desc->isNullable(),
					    column_desc->datetimeStart(),
					    column_desc->datetimeEnd(),
					    column_desc->datetimefractprec,
					    heap
					    );
      CMPASSERT(type);
      if (!type->isSupportedType())
	{
          column_desc->setDefaultClass(COM_NO_DEFAULT);           // can't set a default for these, either.
	  // 4030 Column is an unsupported combination of datetime fields
          if (!errorCode)
            {
              *CmpCommon::diags() << DgSqlCode(4030)
                                  << DgColumnName(makeColumnName(table, column_desc))
                                  << DgInt0(column_desc->datetimeStart())
                                  << DgInt1(column_desc->datetimeEnd())
                                  << DgInt2(column_desc->datetimefractprec);
            }
          else
            {
              *errorCode = 4030;
            }
	}
      break;
    case REC_INTERVAL:
      type = new (heap)
         SQLInterval(heap, column_desc->isNullable(),
		    column_desc->datetimeStart(),
		    column_desc->intervalleadingprec,
		    column_desc->datetimeEnd(),
		    column_desc->datetimefractprec
		    );
      CMPASSERT(type);
      if (! ((SQLInterval *)type)->checkValid(CmpCommon::diags()))
         return TRUE;                                            // error
      if (!type->isSupportedType())
      {
        column_desc->setDefaultClass(COM_NO_DEFAULT);           // can't set a default for these, either.
        if (!errorCode)
          *CmpCommon::diags() << DgSqlCode(3044) << DgString0(column_desc->colname);
        else
          *errorCode = 3044;

      }
      break;
    case REC_BLOB :
      type = new (heap)
	SQLBlob(heap, column_desc->precision, Lob_Invalid_Storage,
		column_desc->isNullable());
      break;

    case REC_CLOB :
      type = new (heap)
	SQLClob(heap, column_desc->precision, Lob_Invalid_Storage,
		column_desc->isNullable());
      break;

    case REC_BOOLEAN :
      type = new (heap) SQLBooleanNative(heap, column_desc->isNullable());
      break;

    case REC_BINARY_STRING :
      type = new (heap) SQLBinaryString(heap, column_desc->length,
                                        column_desc->isNullable(), FALSE);
      break;
      
    case REC_VARBINARY_STRING :
      type = new (heap) SQLBinaryString(heap, column_desc->length,
                                        column_desc->isNullable(), TRUE);
      break;

    default:
      {
	// 4031 Column %s is an unknown data type, %d.
        if (!errorCode)
        {
	*CmpCommon::diags() << DgSqlCode(-4031)
	  << DgColumnName(makeColumnName(table, column_desc))
	  << DgInt0(column_desc->datatype);
        }
        else
        {
          *errorCode = 4031;
        }
	return TRUE;						// error
      }
    } // end switch (column_desc->datatype)

  CMPASSERT(type);

  if (type->getTypeQualifier() == NA_CHARACTER_TYPE) {
    CharInfo::Collation co = ((CharType *)type)->getCollation();

    // a "mini-cache" to avoid proc call, for perf
    static THREAD_P CharInfo::Collation cachedCO = CharInfo::UNKNOWN_COLLATION;
    static THREAD_P Int32         cachedFlags = CollationInfo::ALL_NEGATIVE_SYNTAX_FLAGS;

    if (cachedCO != co) {
      cachedCO = co;
      cachedFlags = CharInfo::getCollationFlags(co);
    }

    if (cachedFlags & CollationInfo::ALL_NEGATIVE_SYNTAX_FLAGS) {
      //
      //## The NCHAR/COLLATE NSK-Rel1 project is forced to disallow all user-
      //	defined collations here.  What we can't handle is:
      //	- full support!  knowledge of how to really collate!
      //	- safe predicate-ability of collated columns, namely
      //	  . ORDER/SEQUENCE/SORT BY
      //	    MIN/MAX
      //	    < <= > >=
      //		These *would* have been disallowed by the
      //		CollationInfo::ORDERED_CMP_ILLEGAL flag.
      //	  . DISTINCT
      //	    GROUP BY
      //	    = <>
      //		These *would* have been disallowed by the
      //		CollationInfo::EQ_NE_CMP_ILLEGAL flag.
      //	  . SELECTing a collated column which is a table or index key
      //		We *would* have done full table scan only, based on flag
      //	  . INS/UPD/DEL involving a collated column which is a key
      //		We *would* have had to disallow this, based on flag;
      //		otherwise we would position in wrong and corrupt either
      //		our partitioning or b-trees or both.
      //	See the "MPcollate.doc" document, and
      //	see sqlcomp/DefaultValidator.cpp ValidateCollationList comments.
      //
	{
	  // 4069 Column TBL.COL uses unsupported collation COLLAT.
	  if (!errorCode)
	  {
	  *CmpCommon::diags() << DgSqlCode(-4069)
	    << DgColumnName(makeColumnName(table, column_desc));
	  }
	  else
	  {
	    *errorCode= 4069;
	  }
	  return TRUE;						// error
	}
    }
  }

  return FALSE;							// no error

} // createNAType()

// -----------------------------------------------------------------------
// Print function for debugging
// -----------------------------------------------------------------------
void NAColumn::print(FILE* ofd, const char* indent, const char* title,
                     CollHeap *c, char *buf) const
{
    const Int32 A = 0x18, D = 0x19; // up-arrow, down-arrow; just to be cute
  char ckstr[3+1];
  SortOrdering cko = getClusteringKeyOrdering();
  if (cko == ASCENDING)		sprintf(ckstr, "ck%c", A);
  else if (cko == DESCENDING)	sprintf(ckstr, "ck%c", D);
  else				ckstr[0] = '\0';

  Space * space = (Space *)c;
  char mybuf[1000];
  sprintf(mybuf,"%s%s %-8s %-16s %d\t| %2s %2s %2s %3s %s %s\n",
          title, indent, colName_.data(),
          type_->getTypeSQLname(TRUE/*terse*/).data(), position_,
          isIndexKey()        ? "ik"  : "",
          isPartitioningKey() ? "hp"  : "",
          isPrimaryKey()      ? "pk"  : "",
          ckstr,
          isComputedColumn() ? computedColumnExpression_ : "",
          isReferencedForHistogram() ? "refForHist" :
          isReferenced() ? "ref" : "");
  PRINTIT(ofd, c, space, buf, mybuf);
}

void NAColumnArray::print(FILE* ofd, const char* indent, const char* title,
                          CollHeap *c, char *buf) const
{
  Space * space = (Space *)c;
  char mybuf[1000];
  BUMP_INDENT(indent);
  for (CollIndex i = 0; i < entries(); i++)
  {
    snprintf(mybuf, sizeof(mybuf), "%s%s[%2d] =", NEW_INDENT, title, i);
    PRINTIT(ofd, c, space, buf, mybuf);
    at(i)->print(ofd, "", "", c, buf);
  }
}

void NAColumn::trace(FILE* f) const
{
  fprintf(f,"%s", colName_.data());
}

void NAColumnArray::trace(FILE* ofd) const
{
  if (!entries()) return;

  CollIndex i = 0;
  for (;;)
  {
    at(i)->trace(ofd);
    i++;
    if (i < entries())
      fprintf(ofd, ",");
    else
    {
      fprintf(ofd, " ");
      return;
    }
  }
}

// ***********************************************************************
// functions for NAColumnArray
// ***********************************************************************

NAColumnArray::~NAColumnArray()
{

}

void NAColumnArray::deepDelete()
{
  unsigned short members = this->entries();
  for( unsigned short i=0;i<members;i++)
  {
    (*this)[i]->deepDelete();
    delete (*this)[i];
  }
}
void NAColumnArray::insertAt(CollIndex index, NAColumn * newColumn)
{
  LIST(NAColumn *)::insertAt(index,newColumn);
  ascending_.insertAt(index,TRUE);
}

NAColumnArray & NAColumnArray::operator= (const NAColumnArray& other)
{
  LIST(NAColumn *)::operator= (other);
  ascending_ = other.ascending_;
  return *this;
}

NAColumn *NAColumnArray::getColumn(Lng32 index) const
{
  NAColumn *column = (*this)[index];
  return column;
}

NAColumn *NAColumnArray::getColumn(const char *colName) const
{
  for (CollIndex i = 0; i < entries(); i++)
    {
      NAColumn *column = (*this)[i];
      if (column->getColName() == colName) return column;
    }
  return NULL;
}

//gets the NAColumn that has the same position
NAColumn * NAColumnArray::getColumnByPos(Lng32 position) const
{
  for (CollIndex i = 0; i < entries(); i++)
    {
      NAColumn *column = (*this)[i];
      if (column->getPosition() == position) return column;
    }
  return NULL;
}

Lng32 NAColumnArray::getOffset(Lng32 position) const
{
  Lng32 result = 0;

  for (Lng32 i = 0; i < position; i++)
    {
      const NAType *t = (*this)[i]->getType();
      Lng32 align = t->getTotalAlignment();
      if (result % align != 0)
	result += (result + align - 1) / align * align;
    }

  return result;
}

ULng32 NAColumnArray::getMaxTrafHbaseColQualifier() const
{
  NAColumn *column;
  char * colQualPtr;
  Lng32 colQualLen;
  Int64 colQVal;
  ULng32 maxVal = 0;

  for (CollIndex i = 0; i < entries(); i++)
    {
      column = (*this)[i];
      colQualPtr = (char*)column->getHbaseColQual().data();
      colQualLen = column->getHbaseColQual().length();
      colQVal = str_atoi(colQualPtr, colQualLen);
      if (colQVal > maxVal)
	maxVal = colQVal ;
    }
  return maxVal;
}

//method to reset an NAColumn object after a statement
//In the ideal world NAColumn objects should not be having any state
//that changes over the course of a statement, since it represents metadata
//info. But NAColumn objects are apparently being used as a vehical to carry
//information through the compiler as we go along compiling a statement, this
//information is statment specific (i.e. it could be different for a
//different statement).
//There is need for such a method because of NATable caching, as NAColumn
//constitute part of the information that is reference via NATable objects.
//Since NATable object will persist across statements (i.e. if they are cached)
//the NAColumn object refered to by an NATable object also persists and so
//we have to make sure that each NAColumn object is reset to its state as it
//was just after NATable construction.
void NAColumn::resetAfterStatement()
{
  //These are set in the binder
  referenced_ = NOT_REFERENCED;
  hasJoinPred_ = FALSE;
  hasRangePred_ = FALSE;

  //This points to an object on the statement heap
  //this is set in the binder after NATable construction
  isNotNullNondroppable_ = NULL;

  needHistogram_ = DONT_NEED_HIST;
  return;
}

NABoolean NAColumnArray::operator==(const NAColumnArray &other) const
{
  if (entries() != other.entries())
    return FALSE;

  for (CollIndex i=0;i<entries();i++)
    {
     if (NOT (*at(i)== *other.at(i)))
       return FALSE;
     if (NOT (isAscending(i) == other.isAscending(i)))
       return FALSE;
    }
  return TRUE;
}

Int32 NAColumnArray::getTotalStorageSize() const
{
  Int32 total = 0;
  for (CollIndex i=0;i<entries();i++)
    {
       total += at(i)->getType()->getNominalSize();
    }

  return total;
}


Int32 NAColumnArray::getColumnPosition(NAColumn& nc) const
{
  for (CollIndex j = 0; j < entries(); j++) {
     if ( nc == (* at(j)) )  // compare via NAColumn::operator==()
       return j;
  }
  return -1;
}

Int32 NAColumnArray::getColumnPosition(NAString& nc) const
{
  for (CollIndex j = 0; j < entries(); j++) {
    if ( (* at(j)) == nc )  // compare via NAColumn::operator==()
      return j;
  }
  return -1;
}

NAString NAColumnArray::getColumnNamesAsString(char separator) const
{
   return getColumnNamesAsString(separator, entries());
}

NAString NAColumnArray::getColumnNamesAsString(char separator, UInt32 ct) const
{
  NAString nmList;

  if ( ct == 0 )
    return NAString();

  if ( ct > entries() )
    ct = entries();

  for (CollIndex i = 0; i < ct-1; i++)
  {
     nmList += ToAnsiIdentifier(at(i)->getColName());
     nmList += separator;
  }
     
  nmList += ToAnsiIdentifier(at(ct-1)->getColName());
  return nmList;
}
