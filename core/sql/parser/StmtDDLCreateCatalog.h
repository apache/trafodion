#ifndef STMTDDLCREATECATALOG_H
#define STMTDDLCREATECATALOG_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLCreateCatalog.h
 * Description:  class for parse nodes representing Create Catalog
 *               statements
 *
 *
 * Created:      3/9/95
 * Language:     C++
 *
 *
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
 *
 *
 *****************************************************************************
 */


#include "ComLocationNames.h"
#include "ElemDDLLocation.h"
#include "StmtDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLCreateCatalog;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Create Catalog statement
// -----------------------------------------------------------------------
class StmtDDLCreateCatalog : public StmtDDLNode
{

public:

  // default constructor
  StmtDDLCreateCatalog(const NAString & aCatalogName,
                       ElemDDLNode * pAttributeList = NULL);

  // virtual destructor
  virtual ~StmtDDLCreateCatalog();

  // cast
  virtual StmtDDLCreateCatalog * castToStmtDDLCreateCatalog();

  //
  // accessors
  //

  virtual Int32 getArity() const;

  inline const NAString & getCatalogName() const;

  virtual ExprNode * getChild(Lng32 index);

  inline const NAString & getLocation() const;

        // returns the location name specified in the LOCATION
        // clause associating with the statement; returns
        // an empty string if the LOCATION clause does not
        // appear.

  inline const NAString & getLocationName() const;

        // same as getLocationName()

  inline ComLocationName::inputFormat getLocationNameInputFormat() const;

        // returns ComLocationName::INPUT_NOT_SPECIFIED if LOCATION
        // clause does not appear; returns ComLocationName::UNKNOWN_
        // INPUT_FORMAT if the location name appears in the specified
        // LOCATION clause is not one of the recognized location name
        // formats; otherwise, returns an enumerated constant of type
        // ComLocationName::inputFormat to described the format of the
        // specified location name.
 
  inline ElemDDLLocation::locationNameTypeEnum getLocationNameType() const;

        // returns the type of the location name (e.g., an OSS
        // path name, a Guardian device name, an OSS environment
        // variable name, etc.)  If LOCATION clause does not
        // appear, the returned value has no meaning.
        //
        // Currently, the LOCATION clause only accepts an OSS
        // path name.

  inline NABoolean isLocationSpecified() const;

        // returns TRUE if the location clause/phrase appears;
        // returns FALSE otherwise.

  // mutator
  virtual void setChild(Lng32 index, ExprNode * pChildNode);

  // for binding
  ExprNode * bindNode(BindWA *bindWAPtr);

  // for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual const NAString displayLabel3() const;
  virtual const NAString getText() const;


private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  //
  // accessors
  //

  inline const ElemDDLLocation * getLocationNode() const;
  inline       ElemDDLLocation * getLocationNode();

        // returns the pointer pointing to the parse node representing
        // the specified Location clause;  return NULL if the Location
        // clause does not appear.

  //
  // mutator
  //

  void setAttribute(ElemDDLNode * pAttrNode);

        // Get the information in the parse node pointed by parameter
        // pAttrNode.  Update the corresponding data member (in this
        // class) accordingly.  Also check for duplicate clauses.

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  NAString catalogName_;

  // The flags is...Spec_ are used to
  // check for duplicate clauses

  // LOCATION clause

  NABoolean isLocationClauseSpec_;
  NAString locationName_;
  ComLocationName::inputFormat locationNameInputFormat_;
  ElemDDLLocation::locationNameTypeEnum locationNameType_;
  ElemDDLLocation * pLocationNode_;

  // pointer to child parse node

  enum { INDEX_CREATE_CATALOG_ATTRIBUTE_LIST = 0,
         MAX_STMT_DDL_CREATE_CATALOG_ARITY };

  ElemDDLNode * attributeList_;

}; // class StmtDDLCreateCatalog

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLCreateCatalog
// -----------------------------------------------------------------------

//
// accessors
//

inline const NAString &
StmtDDLCreateCatalog::getCatalogName() const
{
  return catalogName_;
}

inline const NAString &
StmtDDLCreateCatalog::getLocation() const
{
  return locationName_;
}

// same as getLocation()
inline const NAString &
StmtDDLCreateCatalog::getLocationName() const
{
  return locationName_;
}

inline ComLocationName::inputFormat
StmtDDLCreateCatalog::getLocationNameInputFormat() const
{
  return locationNameInputFormat_;
}

inline ElemDDLLocation::locationNameTypeEnum
StmtDDLCreateCatalog::getLocationNameType() const
{
  return locationNameType_;
}

inline const ElemDDLLocation *
StmtDDLCreateCatalog::getLocationNode() const
{
  return pLocationNode_;
}

inline ElemDDLLocation *
StmtDDLCreateCatalog::getLocationNode()
{
  return pLocationNode_;
}

// is location clause/phrase specified?
inline NABoolean
StmtDDLCreateCatalog::isLocationSpecified() const
{
  return isLocationClauseSpec_;
}

#endif // STMTDDLCREATECATALOG_H
