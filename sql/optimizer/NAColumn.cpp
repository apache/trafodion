/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
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
  if(nac.defaultValue_)
  {
    UInt32 length = na_wcslen((NAWchar*)nac.defaultValue_);
    NAWchar* newDefaultValue = new (heap) NAWchar[length+1];
    na_wcscpy(newDefaultValue, (NAWchar*)nac.defaultValue_);
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

// LCOV_EXCL_START :cnu
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
// LCOV_EXCL_STOP

// LCOV_EXCL_START :dpm
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
#pragma nowarn(1506)   // warning elimination
  BUMP_INDENT(indent);
#pragma warn(1506)  // warning elimination
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
// LCOV_EXCL_STOP

// ***********************************************************************
// functions for NAColumnArray
// ***********************************************************************

NAColumnArray::~NAColumnArray()
{

}

#pragma nowarn(1506)   // warning elimination
void NAColumnArray::deepDelete()
{
#pragma warning (disable : 4244)   //warning elimination
  unsigned short members = this->entries();
#pragma warning (default : 4244)   //warning elimination
  for( unsigned short i=0;i<members;i++)
  {
    (*this)[i]->deepDelete();
    delete (*this)[i];
  }
}
#pragma warn(1506)  // warning elimination
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

// LCOV_EXCL_START :cnu
// removes the column that has the same position
void NAColumnArray::removeByPosition(Lng32 position)
{
  for(CollIndex i=0;i < entries();i++)
  {
    NAColumn * column = (*this)[i];
    if(column->getPosition() == position)
    {
      this->removeAt(i);
      break;
    }
  }
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
// LCOV_EXCL_STOP

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

// LCOV_EXCL_START :dd
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
// LCOV_EXCL_STOP

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

