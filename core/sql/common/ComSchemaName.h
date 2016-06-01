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
#ifndef COMSCHEMANAME_H
#define COMSCHEMANAME_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComSchemaName.h
 * Description:  ComSchemaName represents ANSI SQL schema names.
 *               The external-format of the schema name is following:
 *
 *               [ <catalog-name-part> . ] <schema-name-part>
 *
 * Created:      9/12/95
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
#include <iostream>
#include "NABoolean.h"
#include "NAString.h"
#include "ComAnsiNamePart.h"
#include "ComVersionDefs.h"

// -----------------------------------------------------------------------
// definition of class ComSchemaName
// -----------------------------------------------------------------------

class ComSchemaName
{

  //
  // global friend functions
  //

  friend ostream& operator << (ostream &out, const ComSchemaName &name);

  public:

    //
    // constructors
    //

    ComSchemaName ();

        // Default constructor

    ComSchemaName (const NAString &externalSchemaName);

        // Initializing constructor.  The method copies the input
        // schema name if it is legal.  If the input schema name
        // does not include the catalog name part, the catalog
        // name part saved in this object is empty.  If the input
        // schema name is illegal, the constructed object is empty.
        //
        // This method assumes that the parameter  externalSchemaName
        // contains only the external-format schema name.  Therefore,
        // the method treats the following input as illegal because
        // the input contain other characters besides a schema name:
        //
        //   "catalog_name_part.schema_name_part.object_name_part"
        //
        // (The double-quotes in the example are not part of the ANSI
        // SQL name.  They are used to represent a C string constant.)

    ComSchemaName (const NAString &externalSchemaName,
                   size_t &count);

        // Same as above except that this method also returns
        // the length of the number of bytes examined via the
        // parameter  count.
        //
        // If the schema name is not found or is illegal, the
        // parameter  count  contains the number of bytes examined
        // when the name is determined to be invalid.  If the
        // input schema name is legal, the parameter  count
        // contains the length of the input schema name.

    ComSchemaName (const ComAnsiNamePart &schemaNamePart);

        // Initializing constructor.  Only the schema name part is
        // specified.  The catalog name part in the constructed
        // object is empty.

    ComSchemaName (const ComAnsiNamePart &catalogNamePart,
                   const ComAnsiNamePart &schemaNamePart);

        // Initializing constructor.

    //
    // virtual destructor
    //
    virtual ~ComSchemaName ();

    //
    // assignment operators
    //
    ComSchemaName& operator= (const NAString &rhsSchemaName);

    //
    // accessors
    //

    NAString getExternalName(NABoolean = FALSE) const;

        // returns an empty string if this object is empty; otherwise,
        // returns the ANSI SQL schema name, in external-format (the
        // format used by ANSI SQL users).  If the catalog name part
        // does not exist (in this object), it will not be included
        // in the returned object name (in external format).

    const ComAnsiNamePart &getCatalogNamePart () const{return catalogNamePart_;}
    const NAString &getCatalogNamePartAsAnsiString (NABoolean = FALSE) const;

    const ComAnsiNamePart &getSchemaNamePart () const {return schemaNamePart_;}
    const NAString &getSchemaNamePartAsAnsiString (NABoolean = FALSE) const;

    inline NABoolean operator== (const ComSchemaName &rhs) const;

    //
    // mutator
    //
    
    void clear ();

        // Makes all name parts in this object empty.

    inline void setCatalogNamePart (const ComAnsiNamePart &catalogNamePart);
    inline void setSchemaNamePart (const ComAnsiNamePart &schemaNamePart);
           void setDefinitionSchemaName (const COM_VERSION version);

    //
    // other methods
    //

    inline NABoolean isEmpty () const;
    inline NABoolean isValid () const;

  protected:

  private:

    //
    // private methods
    //

    void initializeDataMembers ();

    NABoolean scan (const NAString &schemaName, size_t &bytesScanned);

        // This method scans the specified external-format ANSI
        // SQL schema name.
        //
        // The method returns TRUE and decomposes the input schema
        // name and then stores the name components into the data
        // members if the schema name is valid.  The method also
        // returns the length of the scanned name via the
        // parameter  count.
        //
        // If the schema name is not found or is illegal, the method
        // returns FALSE and the number of bytes examined (via
        // parameter  count) when the name is determined to be
        // invalid.  In this case, the method does not change the
        // contents of the data members.
        //
        // This method assumes that the parameter  schemaName
        // contains only the external-format schema name.  Therefore,
        // the method treats the following input as illegal because
        // the input contain other characters besides a schema name:
        //
        //   "catalog_name_part.schema_name_part.object_name_part"
        //
        // (The double-quotes in the example are not part of the ANSI
        // SQL name.  They are used to represent a C string constant.)

    NABoolean scan (const NAString &schemaName);

        // Same as above except that this method does not return
        // the number of bytes scanned.

    //
    // private data members
    //

    ComAnsiNamePart catalogNamePart_;
    ComAnsiNamePart schemaNamePart_;

};

// -----------------------------------------------------------------------
// definitions of inline methods for class ComSchemaName
// -----------------------------------------------------------------------

NABoolean
ComSchemaName::isEmpty() const
{
  return (schemaNamePart_.isEmpty() AND catalogNamePart_.isEmpty());
}

NABoolean
ComSchemaName::isValid() const
{
  //
  // catalogNamePart_ can be empty.
  //
  return (schemaNamePart_.isValid());
}

void
ComSchemaName::setCatalogNamePart (const ComAnsiNamePart &catalogNamePart)
{
  catalogNamePart_ = catalogNamePart;
}

void
ComSchemaName::setSchemaNamePart (const ComAnsiNamePart &schemaNamePart)
{
  schemaNamePart_ = schemaNamePart;
}

inline NABoolean
ComSchemaName::operator== (const ComSchemaName &rhs) const
{
  return (getSchemaNamePart().getInternalName() EQU rhs.getSchemaNamePart().getInternalName() AND
          getCatalogNamePart().getInternalName() EQU rhs.getCatalogNamePart().getInternalName());
}
#endif // COMSCHEMANAME_H
