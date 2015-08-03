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
#ifndef PARNAMELOCLIST_H
#define PARNAMELOCLIST_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ParNameLocList.h
 * Description:  definitions of classes ParNameLoc and ParNameLocList.
 *
 *               The ParNameLocList object is used to help with
 *               the computing of the view text, check constraint
 *               search condition text, trigger text, MV text, etc.
 *
 *               Also contains the prototype declarations of file
 *               scope (global) functions relating to ParNameLocList
 *               or ParNameLocListPtr (defined in SqlParser.y).
 *
 *               
 * Created:      5/30/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Collections.h"
#include "ColumnDesc.h"
#include "NAString.h"
#include "nawstring.h" // for wide token strings (tcr)
#include "ObjectNames.h"
#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ParNameLoc;
class ParNameLocList;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class ElemDDLConstraintCheck;
class ElemDDLDivisionClause;
class StmtDDLCreateView;
class StmtDDLCreateTrigger;  
class StmtDDLCreateMV; 
class StmtDDLCreateTable;

// -----------------------------------------------------------------------
// prototype declarations of global (file scope) functions
// defined in file ParNameLocList.C
// -----------------------------------------------------------------------

void ParInsertNameLoc(const StringPos namePos, const size_t nameLen);
void ParInsertNameLocInOrder(const StringPos namePos, const size_t nameLen);
void ParInsertNameLocForStar(ColRefName * pColRefName);
void ParUpdateNameLocForDotStar(const ColRefName * pColRefName);

void ParSetTextStartPosForCheckConstraint(ParNameLocList * pNameLocList);
void ParSetTextStartPosForCreateView(ParNameLocList * pNameLocList);
void ParSetTextStartPosForCreateTrigger(ParNameLocList * pNameLocList);
void ParSetTextStartPosForDivisionByClause(ParNameLocList * pNameLocList);

NABoolean ParSetTextEndPos(ElemDDLConstraintCheck * pCkCnstrntNode);
NABoolean ParSetTextEndPos(StmtDDLCreateView * pCreateViewParseNode);
NABoolean ParSetTextEndPos(StmtDDLCreateTrigger * pCreateTriggerParseNode);
NABoolean ParSetTextEndPos(ElemDDLDivisionClause * pDivisionClauseParseNode);

void ParSetTextStartPosForCreateMV(ParNameLocList * pNameLocList);
NABoolean ParSetTextEndPos(StmtDDLCreateMV * pCreateMVParseNode);

void ParSetEndOfOptionalColumnListPos(ParNameLocList * pNameLocList);
   // Keep position of end-of optional column names list
void ParSetBeginingOfFileOptionsListPos(ParNameLocList * pNameLocList);
   // Keep position of begining-of optional file options column list
void ParSetEndOfFileOptionsListPos(ParNameLocList * pNameLocList);
   // Keep position of end-of optional file options list
void ParSetBeginOfMVQueryPos(ParNameLocList * pNameLocList);
   // Mark the text begining of the MV query
void ParSetEndOfSelectColumnListPos(ParNameLocList * pNameLocList);
   // Keep position of end-of select column list
// ************************************************************

// Mark the text begining of query in the AS clause of a Create Table stmt.
void ParSetBeginOfCreateTableAsQueryPos(ParNameLocList * pNameLocList);
void ParSetBeginOfCreateTableAsAttrList(ParNameLocList * pNameLocList);
void ParSetEndOfCreateTableAsAttrList(ParNameLocList * pNameLocList);
NABoolean ParSetTextEndPos(StmtDDLCreateTable * pCreateTableParseNode);

// Used by MULTI-COMMIT Delete to find the start and end of the WHERE clause.
//
void ParSetTextStartPosForMultiCommit(ParNameLocList * pNameLocList);
NABoolean ParGetTextStartEndPosForMultiCommit(ParNameLocList * pNameLocList,
                                              StringPos &start, StringPos&end);


// -----------------------------------------------------------------------
// Definition of class ParNameLoc
// -----------------------------------------------------------------------
class ParNameLoc : public NABasicObject
{
public:

  //
  // constructors
  //
  
  ParNameLoc (CollHeap * h=PARSERHEAP())
  : namePos_(0),
  nameLen_(0),
  expandedName_(h)
  {
    // expandedName_ is empty
  }

  ParNameLoc (const StringPos namePos, const size_t nameLen, CollHeap * h=PARSERHEAP())
  : namePos_(namePos),
  nameLen_(nameLen),
  expandedName_(h)
  {
    // expandedName_ is empty
  }

  // copy ctor
  ParNameLoc (const ParNameLoc &nameLoc, CollHeap * h=PARSERHEAP())
  : namePos_(nameLoc.namePos_),
  nameLen_(nameLoc.nameLen_),
  expandedName_(nameLoc.expandedName_, h)
  { }


  //
  // virtual destructor
  //
  
  virtual ~ParNameLoc();


  //
  // operators
  //

  ParNameLoc & operator=(const ParNameLoc &rhs);
  
  const NABoolean operator==(const ParNameLoc &rhs) const;

  //
  // accessors
  //

  inline const size_t getNameLength() const;
  inline const StringPos getNamePosition() const;
  const NAString &getExpandedName(NABoolean errorIfEmpty = TRUE) const
  {
  if (errorIfEmpty)
    CMPASSERT(expandedName_ != (const char *)"");
  return expandedName_;
  }

  //
  // mutators
  //

  inline void setNameLength(const size_t nameLen);
  inline void setNamePosition(const StringPos namePos);
  inline void setExpandedName(const NAString &expandedName);
  
private:
  
  StringPos namePos_;
  size_t nameLen_;
  NAString expandedName_;
  
}; // class ParNameLoc

// -----------------------------------------------------------------------
// Definition of class ParNameLocList
// -----------------------------------------------------------------------
class ParNameLocList : private LIST(ParNameLoc *)
{
public:

  //
  // constructors
  //

  ParNameLocList(const char * const pInputString = NULL,
                 CharInfo::CharSet inputStrCS =
                                                CharInfo::UTF8
                 ,
                 const NAWchar * const pInputStrInUTF16 = NULL,
                 CollHeap *heap = PARSERHEAP());

        // heap specifies the heap to allocate space for objects
        // pointed by the elements in this list.

  ParNameLocList(const ParNameLocList &rhs, CollHeap *heap = PARSERHEAP());
  
  //
  // virtual destructor
  //

  virtual ~ParNameLocList();

  //
  // operators
  //
 
  ParNameLocList & operator=(const ParNameLocList &rhs);
  inline const ParNameLoc & operator[](CollIndex index) const;
  inline       ParNameLoc & operator[](CollIndex index);
  
  inline void* operator new(size_t t, CollHeap* h = PARSERHEAP());
  inline void operator delete(void*);

  //
  // accessors
  //

  inline CollIndex entries() const;

  inline const char * getInputStringPtr() const;
  inline const NAWchar * getWInputStringPtr() const;
  inline CharInfo::CharSet getInputStringCharSet() const { return inputStrCharSet_; }
  
  ParNameLoc * getNameLocPtr(const StringPos namePos);

  inline const StringPos getTextStartPosition() const;
  inline const StringPos getWhereStartPosition() const;

  //
  // mutators
  //

  void clear();

        // removes all ParNameLoc object pointed by the pointers
        // in the list; then removes all pointers in the list.

  NABoolean insert(const ParNameLoc &nameLoc);

        // inserts nameLoc to the list and returns TRUE if
        // nameLoc was not already in the list; otherwise,
        // returns FALSE.

  NABoolean insertInOrder(const ParNameLoc &nameLoc);

        // inserts nameLoc to the list such that the name 
        // positions are in order. Required for UDFs.
        // Returns TRUE if nameLoc was not already in the 
        // list; otherwise, returns FALSE.

  inline void setTextStartPosition(const StringPos textStartPos);
  inline void setWhereStartPosition(const StringPos whereStartPos);

private:

  void copy(const ParNameLocList &rhs);

  //
  // the following data member helps keeping track of the
  // starting position of the text (e.g., view text) to be
  // expanded and stored in the metadata table Text.
  //
  
  StringPos textStartPos_;

  // The start postition of the WHERE clause associated with a
  // MULTI-COMMIT Delete.  (Must use a different StringPos so that
  // MULTI-COMMIT Delete can also be used with features that make
  // use of textStartPos_; for example, EXPLAIN.
  //
  StringPos whereStartPos_;

  //
  // the following data member helps keeping track of the
  // pointer to the input string buffer containing the
  // text to be expanded.  Cannot rely on the file scope
  // pointer input_str because it might be changed during
  // the processing of a statement.
  //

  const char * pInputString_;
  CharInfo::CharSet inputStrCharSet_;
  const NAWchar * pwInputString_;
  // wide version (tcr)

  //
  // heap to allocate space for objects pointed by elements in the list.
  //
  CollHeap * heap_;

}; // class ParNameLocList

// -----------------------------------------------------------------------
// definitions of inline methods for class ParNameLoc
// -----------------------------------------------------------------------
//
// accessors
//

inline const size_t
ParNameLoc::getNameLength() const
{
  return nameLen_;
}

inline const StringPos
ParNameLoc::getNamePosition() const
{
  return namePos_;
}


//
// mutators
//

inline void
ParNameLoc::setNameLength(const size_t nameLen)
{
  nameLen_ = nameLen;
}

inline void
ParNameLoc::setNamePosition(const StringPos namePos)
{
  namePos_ = namePos;
}

inline void
ParNameLoc::setExpandedName(const NAString &expandedName)
{
  expandedName_ = expandedName;
}

// -----------------------------------------------------------------------
// definitions of inline methods for class ParNameLocList
// -----------------------------------------------------------------------

//
// operators
//

inline const ParNameLoc &
ParNameLocList::operator[](CollIndex index) const
{
  return *(LIST(ParNameLoc *)::operator[](index));
}

inline ParNameLoc &
ParNameLocList::operator[](CollIndex index)
{
  return *(LIST(ParNameLoc *)::operator[](index));
}

inline void *
ParNameLocList::operator new(size_t t, CollHeap *h)
{
  return LIST(ParNameLoc *)::operator new(t, h);
}

inline void
ParNameLocList::operator delete(void *p)
{
  LIST(ParNameLoc *)::operator delete(p);
}


//
// accessors
//

inline CollIndex
ParNameLocList::entries() const
{
  return LIST(ParNameLoc *)::entries();
}

inline const char *
ParNameLocList::getInputStringPtr() const
{
  return pInputString_;
}

inline const NAWchar *
ParNameLocList::getWInputStringPtr() const
{
  return pwInputString_;
}

inline const StringPos
ParNameLocList::getTextStartPosition() const
{
  return textStartPos_;
}

inline const StringPos
ParNameLocList::getWhereStartPosition() const
{
  return whereStartPos_;
}

//
// mutator
//

inline void
ParNameLocList::setTextStartPosition(const StringPos textStartPos)
{
  textStartPos_ = textStartPos;
}

inline void
ParNameLocList::setWhereStartPosition(const StringPos whereStartPos)
{
  whereStartPos_ = whereStartPos;
}

#endif // PARNAMELOCLIST_H
