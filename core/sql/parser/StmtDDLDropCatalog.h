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
#ifndef STMTDDLDROPCATALOG_H
#define STMTDDLDROPCATALOG_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLDropCatalog.h
 * Description:  class for parse node representing Drop Catalog statements
 *
 *
 * Created:      11/14/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ComSmallDefs.h"
#include "StmtDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLDropCatalog;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Create Catalog statement
// -----------------------------------------------------------------------
class StmtDDLDropCatalog : public StmtDDLNode
{

public:

  // constructor
  StmtDDLDropCatalog(const NAString & catalogName,
                    ComDropBehavior dropBehavior);

  // virtual destructor
  virtual ~StmtDDLDropCatalog();

  // cast
  virtual StmtDDLDropCatalog * castToStmtDDLDropCatalog();

  // accessor
  inline const NAString & getCatalogName() const;
  inline ComDropBehavior getDropBehavior() const;

  // for binding
  ExprNode * bindNode(BindWA *bindWAPtr);

  // for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;


private:

  NAString catalogName_;
  ComDropBehavior dropBehavior_;

}; // class StmtDDLDropCatalog

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLDropCatalog
// -----------------------------------------------------------------------

//
// accessor
//

inline const NAString &
StmtDDLDropCatalog::getCatalogName() const
{
  return catalogName_;
}

inline ComDropBehavior
StmtDDLDropCatalog::getDropBehavior() const
{
  return dropBehavior_;
}

#endif // STMTDDLDROPCATALOG_H
