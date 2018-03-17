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
//   https://www.apache.org/licenses/LICENSE-2.0
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
* File:         AccessSets.cpp
* Description:  Definition of class InliningInfo.
*
* Created:      6/23/98
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/

#include "Sqlcomp.h"
#include "AllItemExpr.h"
#include "AllRelExpr.h"
#include "GroupAttr.h"
#include "AccessSets.h"

static const Int32 INITIAL_SIZE_OF_READ_SET  = 10;
static const Int32 INITIAL_SIZE_OF_WRITE_SET =  5;

//////////////////////////////////////////////////////////////////////////////
// Methods of class TableAccess
//////////////////////////////////////////////////////////////////////////////

// Default ctor - to EMPTY.
TableAccess::TableAccess() 
  : tableID_(NULL), 
    empty_(TRUE)
{
}

// Explicit ctor
TableAccess::TableAccess(const NATable *theTable) :
    tableID_(const_cast<NATable *>(theTable)),
    empty_(FALSE)
{
}

// Copy ctor
TableAccess::TableAccess(const TableAccess &other) 
  : tableID_ (other.tableID_),
    empty_   (other.empty_)
{
}

// Assignment operator
TableAccess& TableAccess::operator= (const TableAccess& other)
{
  tableID_ = other.tableID_;
  empty_   = other.empty_;
  return *this;
}

NABoolean TableAccess::match(const TableAccess& other) const
{
  return ( (tableID_  == other.tableID_ ) &&
	   (empty_    == other.empty_   ) );
}

void TableAccess::print(FILE *ofd, const char *indent, const char *title) const
{
  if (isEmpty())
    fprintf(ofd,"%s - Empty.\n", title);
  else if (tableID_ == NULL)
    fprintf(ofd,"%s - Opaque.\n", title);
  else
  {
    BUMP_INDENT(indent);
    fprintf(ofd,"%s%s this=%p, NATable=%p, ",
	    NEW_INDENT, title, this, tableID_);
    fprintf(ofd,"%s\n", 
	    tableID_->getTableName().getQualifiedNameAsString().data());
  }
}

//////////////////////////////////////////////////////////////////////////////
// Methods of class ReadTableAccess
//////////////////////////////////////////////////////////////////////////////

// Assignment operator
ReadTableAccess& ReadTableAccess::operator= (const ReadTableAccess& other)
{
  tableID_ = other.tableID_;
  empty_   = other.empty_;
//	// Call the same operator on the parent class.
//	(*(TableAccess *)this) = other;
  return *this;
}

// Comparison operator
NABoolean ReadTableAccess::operator ==(const TableAccess& other) const
{
  return (match(other) && other.isReadAccess());
}

// A ReadTableAccess object only conflicts with another WriteTableAccess
// object, on the same table.
NABoolean ReadTableAccess::isConflicting(const TableAccess& other) const
{
  CMPASSERT(!isEmpty() && !other.isEmpty());
  if (!isOpaque() && !other.isOpaque())
    if (!match(other))
      return FALSE;

  // The tables match.
  return (!other.isReadAccess());
}

//////////////////////////////////////////////////////////////////////////////
// Methods of class WriteTableAccess
//////////////////////////////////////////////////////////////////////////////

// Assignment operator
WriteTableAccess& WriteTableAccess::operator= (const WriteTableAccess& other)
{
  tableID_ = other.tableID_;
  empty_   = other.empty_;
//	// Call the same operator on the parent class.
//	(*(TableAccess *)this) = other;
  return *this;
}

// Comparison operator
NABoolean WriteTableAccess::operator ==(const TableAccess& other) const
{
  return (match(other) && !other.isReadAccess());
}

// A WriteTableAccess object conflicts with all other TableAccess objects, 
// on the same table.
NABoolean WriteTableAccess::isConflicting(const TableAccess& other) const
{
  CMPASSERT(!isEmpty() && !other.isEmpty());
  if (!isOpaque() && !other.isOpaque())
    if (!match(other))
      return FALSE;

  // The tables match.
  return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// Methods of class SubTreeAccessSet
//////////////////////////////////////////////////////////////////////////////

// Ctor
SubTreeAccessSet::SubTreeAccessSet(CollHeap *heap) 
  : opaqueForRead_(FALSE),
    opaqueForWrite_(FALSE)
{
  readSet_ = new(heap) SET(ReadTableAccessPtr )(heap,INITIAL_SIZE_OF_READ_SET);
  writeSet_= new(heap) SET(WriteTableAccessPtr)(heap,INITIAL_SIZE_OF_WRITE_SET);
}

// dtor
SubTreeAccessSet::~SubTreeAccessSet()
{
  delete readSet_;
  delete writeSet_;
}

// Add a single ReadTableAccess object to the read set.
void SubTreeAccessSet::add(ReadTableAccessPtr snas)
{
  // Since readSet_ is a set of pointers, it may contain duplicates.
  // So before inserting - check if it's already in there.
  for (CollIndex i=0; i<readSet_->entries(); i++) 
    if (*(readSet_->at(i)) == *snas)
      return;

  CMPASSERT(!snas->isEmpty());

  if (snas->isOpaque())
    opaqueForRead_ = TRUE;
  readSet_->insert(snas);
}

// Add a single WriteTableAccess object to the write set.
void SubTreeAccessSet::add(WriteTableAccessPtr snas)
{
  // Since writeSet_ is a set of pointers, it may contain duplicates.
  // So before inserting - check if it's already in there.
  for (CollIndex i=0; i<writeSet_->entries(); i++) 
    if (*(writeSet_->at(i)) == *snas)
      return;

  CMPASSERT(!snas->isEmpty());

  if (snas->isOpaque())
    opaqueForWrite_ = TRUE;
  writeSet_->insert(snas);
}

// Merge another SubTreeAccessSet into this object.
void SubTreeAccessSet::add(const SubTreeAccessSet *stas)
{
  if (stas == NULL)
    return;

  CollIndex i=0;
  for (i=0; i<stas->readSet_->entries(); i++)
    add((*stas->readSet_)[i]);

  for (i=0; i<stas->writeSet_->entries(); i++)
    add((*stas->writeSet_)[i]);
}

// Does this access set conflict with a single TableAccess?
// Check if it conflicts with any of the contained objects.
NABoolean SubTreeAccessSet::isConflicting(const TableAccess *snas) const
{
  CMPASSERT(!snas->isEmpty());

  // First check if is conflicting with the write set
  if (opaqueForWrite_)
    return TRUE;

  CollIndex i=0;
  for (i=0; i<writeSet_->entries(); i++)
    if((*writeSet_)[i]->isConflicting(*snas))
      return TRUE;

  // For write operations, check the read set too.
  if (!snas->isReadAccess())
  {
    if (opaqueForRead_)
      return TRUE;

    for (i=0; i<readSet_->entries(); i++)
      if((*readSet_)[i]->isConflicting(*snas))
	return TRUE;
  }

  return FALSE;
}

// Does this access set conflict with a another SubTreeAccessSet?
// Check it against each and every contained object.
NABoolean SubTreeAccessSet::isConflicting(const SubTreeAccessSet *other) const
{
  if (other == NULL)
    return FALSE;

  // First check if this set is OPAQUE.
  if (opaqueForWrite_ && !other->isEmpty())
    return TRUE;

  // Check against each and every table access in the write set.
  CollIndex i=0;
  for (i=0; i<writeSet_->entries(); i++)
    if(other->isConflicting((*writeSet_)[i]))
      return TRUE;

  // Check against each and every table access in the read set.
  for (i=0; i<readSet_->entries(); i++)
    if(other->isConflicting((*readSet_)[i]))
      return TRUE;

  return FALSE;
}

NABoolean SubTreeAccessSet::isEmpty() const
{
  return (readSet_->isEmpty() && writeSet_->isEmpty());
}

void SubTreeAccessSet::print(FILE *ofd, const char *indent, const char *title) const
{
  CollIndex i;

  BUMP_INDENT(indent);
  fprintf(ofd,"%s%s %p \n", NEW_INDENT, title, this);

  if (this == NULL)
    fprintf(ofd, "NULL\n");
  else
  {
    for (i = 0; i < readSet_->entries(); i++)
    (*readSet_)[i]->print(ofd, indent);
    if (i) 
      fprintf(ofd,"\n");

    for (i = 0; i < writeSet_->entries(); i++)
    (*writeSet_)[i]->print(ofd, indent);
    if (i) 
      fprintf(ofd,"\n");
  }
}

//////////////////////////////////////////////////////////////////////////////
// Methods of other classes using access sets.
//////////////////////////////////////////////////////////////////////////////

// Collect SubTreeAccessSet information from the sub-tree below, and return
// it to the parent.
// When the isAccessSetNeeded flag is set, it means that the children's
// access sets should also be saved in this node.
SubTreeAccessSet * RelExpr::calcAccessSets(CollHeap *heap)
{
  if (getInliningInfo().isAccessSetNeeded())
  { 
    // This is the Trigger backbone - save the children's access sets
    // as data members of the RelExpr class.
    // If no access set - put an empty set, in order to simplify
    // the checkswhen checking for conflicts.
    if (child(0) != NULL) 
    {
      setAccessSet0(child(0)->calcAccessSets(heap));
      if (getAccessSet0() == NULL)
	setAccessSet0(new(heap) SubTreeAccessSet(heap));
    }

    if (child(1) != NULL) 
    {
      setAccessSet1(child(1)->calcAccessSets(heap));
      if (getAccessSet1() == NULL)
	setAccessSet1(new(heap) SubTreeAccessSet(heap));
    }

    #ifndef NDEBUG
    if (getenv("DEBUG_ACCESS_SETS"))
    {
      ExprNode::print();
      getAccessSet0()->print();
      getAccessSet1()->print();
    }
    #endif

    // Create a fresh new object, add to it the children's access sets
    // and return it up to the parent node.
    SubTreeAccessSet *result = new(heap) SubTreeAccessSet(heap);
    result->add(getAccessSet0());
    result->add(getAccessSet1());
    return(result);
  } 
  else 
  {
    // All the children's access sets are merged into one
    // (the first one returned).
    SubTreeAccessSet *stas = NULL;

    for (Int32 i=0; i<getArity(); i++)
      if (child(i))
	if (stas==NULL)
	  stas = child(i)->calcAccessSets(heap);
	else
	  stas->add(child(i)->calcAccessSets(heap));
    return (stas);
  }
}

// Create a new SubTreeAccessSet object and add to it the children's access
// sets and this node's read access.
SubTreeAccessSet * Scan::calcAccessSets(CollHeap *heap)
{
  CMPASSERT(!getInliningInfo().isAccessSetNeeded());

  // Ignore index tables, resource forks and other special tables.
  switch (getTableDesc()->getNATable()->getSpecialType())
  {
    case ExtendedQualName::NORMAL_TABLE   :
    case ExtendedQualName::TRIGTEMP_TABLE : 
    case ExtendedQualName::MV_TABLE       :
    case ExtendedQualName::EXCEPTION_TABLE:
    case ExtendedQualName::GHOST_TABLE   :
    case ExtendedQualName::GHOST_MV_TABLE       :
      break;
    default:						  
      return RelExpr::calcAccessSets(heap);
  }

  SubTreeAccessSet *result = RelExpr::calcAccessSets(heap);
  if (result == NULL)
    result = new(heap) SubTreeAccessSet(heap);

  const ReadTableAccessPtr readAccess = new(heap) 
    ReadTableAccess(getTableDesc()->getNATable());
  result->add(readAccess);
  
  return(result);
}

// Create a new SubTreeAccessSet object and add to it the children's access 
// sets and this node's write access.
SubTreeAccessSet * GenericUpdate::calcAccessSets(CollHeap *heap)
{
  // Ignore index tables, resource forks and other special tables.
  switch (getTableDesc()->getNATable()->getSpecialType())
  {
    case ExtendedQualName::NORMAL_TABLE   :
    case ExtendedQualName::TRIGTEMP_TABLE :
    case ExtendedQualName::MV_TABLE       : 
    case ExtendedQualName::EXCEPTION_TABLE: 
    case ExtendedQualName::GHOST_TABLE   :
    case ExtendedQualName::GHOST_MV_TABLE       : 
      break;
    default:						  
      return RelExpr::calcAccessSets(heap);
  }

  SubTreeAccessSet *result = RelExpr::calcAccessSets(heap);
  if (result == NULL)
    result = new(heap) SubTreeAccessSet(heap);

  const WriteTableAccessPtr writeAccess = new(heap)
    WriteTableAccess(getTableDesc()->getNATable());
  result->add(writeAccess);
  
  return(result);
}
