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
#ifndef ACCESS_SET_H
#define ACCESS_SET_H

/* -*-C++-*-
******************************************************************************
*
* File:         AccessSets.h
* Description:  Definition of class InliningInfo.
*
* Created:      6/23/98
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/

//////////////////////////////////////////////////////////////////////////////
// This file provides the classes used for collecting and using access sets.
// Access sets are defined at table granularity (not comparing columns).
// The access sets are collected during a new pass on the RelExpr tree,
// using the new RelExpr::calcAccessSets() virtual method. This pass is
// called during the optimizer pilot phase. The results are used during the 
// trigger transformation pass (during the reorderTree() pass) to find 
// which triggers have conflicting access sets, and cannot be executed in 
// parallel. The NATable pointer is used to decide if two tables accesses are
// on the same table, since it is guaranteed to have a single NATable per 
// table. A TableAccess object is initialized to an EMPTY state only by the 
// default constructor, (which must be provided for it to be used in 
// collections). Therefore ASSERTs are used to verify that no EMPTY objects 
// are actually used.
//
// OPAQUE objects are for "black box" operators such as stored procedures,
// that we don't know which tables they affect. Both the ReadTableAccess and
// the WriteTableAccess objects can be constructed as opaque, to represent 
// read-only and read-write opaque objects. Since stored procedures are not
// ready yet, this support was never tested. It looks OK though...
//////////////////////////////////////////////////////////////////////////////

class TableAccess : public NABasicObject
{
public:
  // Default ctor - to EMPTY.
  TableAccess();
  // Explicit ctor (a NULL value for theTable is allowed for 
  // non-specific (opaque) operations.
  TableAccess(const NATable *theTable);
  // Copy ctor
  TableAccess(const TableAccess &other);
  // Assignment operator
  TableAccess& operator= (const TableAccess& other);
  // Comparison operator
  virtual NABoolean operator ==(const TableAccess &other) const = 0;

  NABoolean match(const TableAccess& other) const;
  virtual NABoolean isConflicting(const TableAccess& other) const = 0;
  virtual NABoolean isReadAccess() const = 0;

  inline NABoolean isEmpty() const
    { return empty_; }
  inline NABoolean isOpaque() const
    { return (!isEmpty() && (tableID_==NULL)); }

  // print, for debugging.
virtual void print(FILE *ofd          = stdout, 
		   const char *indent = DEFAULT_INDENT, 
		   const char *title  = "TableAccess") const;

protected:
  NATable  *tableID_;
  NABoolean empty_;
};

class ReadTableAccess : public TableAccess
{
public:
  // Default ctor - to EMPTY.
  ReadTableAccess() 
    : TableAccess() {}

  // Explicit ctor
  ReadTableAccess(const NATable *theTable) 
    : TableAccess(theTable) {}

  // Copy ctor
  ReadTableAccess(const ReadTableAccess &other) 
    : TableAccess(other) {}

  // Assignment operator
  ReadTableAccess& operator= (const ReadTableAccess& other);
  // Comparison operator
  virtual NABoolean operator ==(const TableAccess &other) const;

  virtual NABoolean isReadAccess() const 
    { return TRUE; }

  virtual NABoolean isConflicting(const TableAccess& other) const;

  // print, for debugging.
  virtual void print(FILE *ofd  = stdout,
		     const char *indent = DEFAULT_INDENT,
		     const char *title  = "ReadTableAccess") const
    { TableAccess::print(ofd, indent, title); }
};

class WriteTableAccess : public TableAccess
{
public:
  // Default ctor - to EMPTY.
  WriteTableAccess() 
    :  TableAccess() {}

  // Explicit ctor
  WriteTableAccess(const NATable *theTable) 
    :  TableAccess(theTable) {}

  // Copy ctor
  WriteTableAccess(const WriteTableAccess &other) 
    :  TableAccess(other) {}

  // Assignment operator
  WriteTableAccess& operator= (const WriteTableAccess& other);
  // Comparison operator
  virtual NABoolean operator ==(const TableAccess &other) const;

  virtual NABoolean isReadAccess() const 
    { return FALSE; }

  virtual NABoolean isConflicting(const TableAccess& other) const;

  // print, for debugging.
  virtual void print(FILE *ofd  = stdout,
		     const char *indent = DEFAULT_INDENT,
		     const char *title  = "WriteTableAccess") const
    { TableAccess::print(ofd, indent, title); }
};

typedef const ReadTableAccess  *ReadTableAccessPtr;
typedef const WriteTableAccess *WriteTableAccessPtr;

class SubTreeAccessSet : public NABasicObject
{
public:
  // Default ctor
  SubTreeAccessSet(CollHeap *heap);

  // dtor
  ~SubTreeAccessSet();

  void add(ReadTableAccessPtr  snas);
  void add(WriteTableAccessPtr snas);
  void add(const SubTreeAccessSet *stas);

  NABoolean isConflicting(const TableAccess      *snas)  const;
  NABoolean isConflicting(const SubTreeAccessSet *other) const;
  NABoolean isEmpty() const;

  // print, for debugging.
  void print(FILE *ofd          = stdout,
	     const char *indent = DEFAULT_INDENT,
	     const char *title  = "SubTreeAccessSet") const;

private:
  // Since these are currently SETs of pointers, and not of objects,
  // the SET container does not detect and avoid duplicates.
  SET(ReadTableAccessPtr)  *readSet_;
  SET(WriteTableAccessPtr) *writeSet_;
  NABoolean opaqueForRead_;
  NABoolean opaqueForWrite_;
};

#endif
