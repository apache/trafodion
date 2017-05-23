#ifndef COMOBJECTNAME_H
#define COMOBJECTNAME_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComObjectName.h
 * Description:  ComObjectName represents a three part ANSI name.  The
 *               three parts being the catalog name part, the schema
 *               name part, and the object name part.  The syntax of
 *               the external-format ANSI SQL name is:
 *               [ [ <catalog-name-part> . ] <schema-name-part> . ]
 *                      <object-name-part>
 *
 *               Class ComRoutineActionName represents a partially- or fully-
 *               qualified routine action name.  The UID of the associating
 *               universal user-defined function (UUDF - e.g. sas_score)
 *               must be provided to distinguish 2 different actions with
 *               with the same name (note that their corresponding associating
 *               UUDF's are not the same).
 *
 * Created:      7/21/95
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

#ifdef NA_STD_NAMESPACE
#include <iosfwd>
using namespace std;
#else
#include <iostream>
#endif

#include "NABoolean.h"
#include "NAString.h"
#include "ComOperators.h"
#include "ComSmallDefs.h"
#include "ComAnsiNamePart.h"
#include "ComRoutineActionNamePart.h"
#include "ComSchemaName.h"
#include "ComMisc.h"

// -----------------------------------------------------------------------
// forward declarations
// -----------------------------------------------------------------------
class ComObjectName;
class ComRoutineActionName;

// -----------------------------------------------------------------------
// definition of class ComObjectName
// -----------------------------------------------------------------------

class ComObjectName : public NABasicObject
{

  //
  // global friend functions
  //

  friend ostream& operator<<(ostream &out, const ComObjectName &name);
  friend void ComObjTest(const NAString &e);	//##

  public:

  //
  // constructors
  //

    // Default constructor
    ComObjectName(CollHeap * h=0);

    // Copy constructor
    ComObjectName( const ComObjectName & rhs
                 , CollHeap * h=0
		 );

    ComObjectName( const NAString         &objectName
		 , const ComAnsiNameSpace  nameSpace = COM_UNKNOWN_NAME
                 , NABoolean createDropAlias = FALSE
                 , CollHeap * h=0
                 , unsigned short toInternalIdentifierFlags = NASTRING_ALLOW_NSK_GUARDIAN_NAME_FORMAT
		 );

        // Initializing constructor.  The method scans the input
        // object name to find out whether the input name is legal.
        // If the input object name is legal, the method decomposes
        // the input into separate name components.  If the input
        // name does not include the catalog or schema name part,
        // the methods treats the corresponding name part to be empty.
        //
        // If the input object name is illegal, the constructed
        // object is empty.
        //
        // Examples of the contents of the input objectName parameter:
        //
        //   cat.sch."A USER-DEFINED TABLE"
        //   catalog_name_part.schema_name_part.object_name_part
        //
        // Parameter toInternalIdentifierFlags is the same as
        // parameter pv_flags in the declaration of routine
        // ToInternalIdentifier() in header file NAString.h --
        // It is different from the data member flags_.

    ComObjectName( const NAString         &objectName
		 , size_t                 &count
		 , const ComAnsiNameSpace  nameSpace = COM_UNKNOWN_NAME
                 , CollHeap * h=0
		 );

        // Similar to the previous ctor, except:
        // - the number of bytes scanned is returned via the parameter  count.
	// - successful scans proceed up to the first illegal unquoted char
	// (or end-of-string); e.g.
	//	"cat . sch . tbl . col"
	//	 123456789 123456789
	// returns an object "cat.sch.tbl" and a count of 16.
	// The caller can then continue scanning the rest of the string.
        //
        // If the object name is not found or is illegal, the
        // parameter  count  contains the number of bytes examined
        // when the name is determined to be invalid.  If the
        // input object name is legal, the parameter  count
        // contains the length of the input object name.

    ComObjectName( const NAString  &catalogNamePart
                 , const NAString  &schemaNamePart
                 , const NAString  &objectNamePart
		 , const ComAnsiNameSpace  nameSpace = COM_UNKNOWN_NAME
                 , const ComAnsiNamePart::formatEnum  format =
                         ComAnsiNamePart::EXTERNAL_FORMAT
                 , CollHeap * h=0
		 );

        // Initializing constructor - If parameter  format  contains
        // the value ComAnsiNamePart::EXTERNAL_FORMAT (the default),
        // the first three (3) parameters (whose names contain the
        // NamePart suffix) must contain the name parts in external
        // format (the format used by ANSI SQL users).  If parameter
        // format  contains the value ComAnsiNamePart::INTERNAL_FORMAT,
        // the parameters ...NamePart must contain the name parts in
        // internal format (the format stored in the Metadata Dictionary
        // tables).
        //
        // If an input name part is illegal, the constructed object is
        // empty.  If the catalog name part is not empty and the schema
        // name part is empty, the constructed object is empty (that is,
        // the input name parts are considered as being illegal).

    ComObjectName( const NAString  &catalogNamePart
                 , const NAString  &schemaNamePart
                 , const NAString  &objectNamePart
                 , const ComAnsiNameSpace  nameSpace
                 , NABoolean doNotParse // must be TRUE
                 );

        // This method constructs ComObjectName directly from input strings.
        // Use this constructor only if the input strings have already been
        // parsed, such as calling from CatExecCreateAlias, CatExecDropalias.
        // The last param doNotParse is used to differentiate from the previous
        // constructor.

    ComObjectName( const ComAnsiNamePart  &objectNamePart
		 , const ComAnsiNameSpace  nameSpace = COM_UNKNOWN_NAME
		 );

        // Initializing constructor.  The catalog and schema name
        // parts in the constructed object is empty.

    ComObjectName( const ComAnsiNamePart  &schemaNamePart
                 , const ComAnsiNamePart  &objectNamePart
		 , const ComAnsiNameSpace  nameSpace = COM_UNKNOWN_NAME
		 );

        // Initializing constructor.  The catalog name part in the
        // constructed object is empty.

    ComObjectName( const ComAnsiNamePart  &catalogNamePart
                 , const ComAnsiNamePart  &schemaNamePart
                  , const ComAnsiNamePart  &objectNamePart
		  , const ComAnsiNameSpace  nameSpace = COM_UNKNOWN_NAME
		  );

        // Initializing constructor.

    //
    // virtual destructor
    //

    virtual ~ComObjectName();

    //
    // virtual cast methods
    //

    virtual const ComRoutineActionName * castToComRoutineActionName() const;
    virtual       ComRoutineActionName * castToComRoutineActionName();

    //
    // operators
    //

		 ComObjectName&   operator=(const NAString &rhsObjectName);
		 ComObjectName&   operator=(const ComObjectName &rhsObjectName);
    inline       ComBoolean       operator==(const ComObjectName &rhs) const;
    inline       ComBoolean       operator>=(const ComObjectName &rhs) const;

    //
    // accessors
    //

    const ComAnsiNamePart &getCatalogNamePart() const {return catalogNamePart_;}
    const ComAnsiNamePart &getSchemaNamePart()  const {return schemaNamePart_;}
    const ComAnsiNamePart &getObjectNamePart()  const {return objectNamePart_;}
    
    const NAString &getCatalogNamePartAsAnsiString(NABoolean = FALSE) const;
    const NAString &getSchemaNamePartAsAnsiString (NABoolean = FALSE) const;
    const NAString &getObjectNamePartAsAnsiString (NABoolean = FALSE) const;

    virtual NAString getExternalName(NABoolean formatForDisplay = FALSE,
                                     NABoolean displayedExternally = FALSE) const;

        // returns an empty string if this object is empty; otherwise,
        // returns the ANSI SQL object name, in external-format
        // (the format used by ANSI SQL users).  If the catalog name part
        // does not exist (in this object), it will not be included
        // in the returned object name (in external format).  If the
        // schema name part does not exist, only the object name part
        // (in external format) is returned.  If the name is to be displayed
        // externall, for example in SHOWDDL, the catalog name is not included.

    ComAnsiNameSpace getNameSpace() const	  { return nameSpace_; }
    NAString getExternalNameWithNameSpace(NABoolean formatForDisplay = FALSE) const;

    virtual inline NABoolean isEmpty() const;
    virtual inline NABoolean isValid() const;

    inline void getComSchemaNameObject(ComSchemaName & comSchNameObj) const; // out

    //
    // mutators
    //

    virtual void clear();
    virtual void clearIfInvalid(); // Make all data members in this object empty.

    void PLACEHOLDER_FOR_NONVIRTUAL_METHOD();	// ## remove, or rename+reuse
        
    inline void setCatalogNamePart(const ComAnsiNamePart  &catalogNamePart);
    inline void setSchemaNamePart (const ComAnsiNamePart  &schemaNamePart);
    inline void setObjectNamePart (const ComAnsiNamePart  &objectNamePart);
    inline void setNameSpace      (const ComAnsiNameSpace  nameSpace);

    void applyDefaults( const ComAnsiNamePart &catalogNamePart
                      , const ComAnsiNamePart &schemaNamePart
                      );

    NABoolean isVolatile() const      { return (flags_ & IS_VOLATILE) != 0; }
    void setIsVolatile(NABoolean v)
    { (v ? flags_ |= IS_VOLATILE : flags_ &= ~IS_VOLATILE);}

    inline NABoolean isExternalHive() const;  
    inline NABoolean isExternalHbase() const;

    // external format of an HBase mapped table: HBASE."_MAP_".<tablename>
    inline NABoolean isHBaseMappedExtFormat() const;

    // internal format of HBase mapped table: TRAFODION."_HB_MAP_".<tablename>
    inline NABoolean isHBaseMappedIntFormat() const;

  protected:

    // The following constructor is invoked by a constructor of class CatRoutineActionName
    ComObjectName( const ComUID    &uudfUid
                 , const NAString  &catalogNamePart
                 , const NAString  &schemaNamePart
                 , const NAString  &routineActionNamePart
                 , const ComAnsiNameSpace  nameSpace  // must be COM_UUDF_ACTION_NAME
                 , const ComAnsiNamePart::formatEnum  format = ComAnsiNamePart::EXTERNAL_FORMAT
                 , ComBoolean parseInputNameParts = TRUE
                 , CollHeap * h=0
                 );

    // The following method is invoked by the corresponding initializing constructor
    // and one of theCatRoutineActionName::set() method.  The following method clears
    // the object and returns FALSE, if the input parameters are invalid.
    NABoolean initialize ( const ComUID    &uudfUid     // UID of UUDF associating with UDF action
                         , const NAString  &catalogNamePart
                         , const NAString  &schemaNamePart
                         , const NAString  &routineActionNamePart
                         , const ComAnsiNameSpace  nameSpace  // must be COM_UUDF_ACTION_NAME
                         , const ComAnsiNamePart::formatEnum  format = ComAnsiNamePart::EXTERNAL_FORMAT
                         , ComBoolean       parseInputNameParts = TRUE
                         );

    inline Lng32 getFlags() const { return flags_; }

  private:
    enum Flags { IS_VOLATILE = 0x1 };

    ComAnsiNamePart   catalogNamePart_;
    ComAnsiNamePart   schemaNamePart_;
    ComAnsiNamePart   objectNamePart_;
    ComAnsiNameSpace  nameSpace_;
    CollHeap          *heap_;

    Lng32              flags_;

    NABoolean scan(const NAString &objectName, size_t &count
                   , NABoolean createDropAlias = FALSE
                   , unsigned short toInternalIdentifierFlags = NASTRING_ALLOW_NSK_GUARDIAN_NAME_FORMAT
                  );

        // This method scans the specified external-format ANSI
        // SQL object name.
        //
        // The method returns TRUE and decomposes the input object
        // name and then stores the name components into the data
        // members if the object name is valid.  The method also
        // returns the length of the scanned name via the
        // parameter  count.
        //
        // If the object name is not found or is illegal, the method
        // returns FALSE and the number of bytes examined (via
        // parameter  count) when the name is determined to be
        // invalid.  In this case, the method does not change the
        // contents of the data members.
        //
        // This method assumes that the parameter  objectName
        // contains only the external-format object name.  Therefore,
        // the method treats the following input as illegal because
        // the input contain other characters besides an object name:
        //
        //   "catalog_name_part.schema_name_part.object_name_part "
        //
        // (The double-quotes in the example are not part of the ANSI
        // SQL name.  They are used to represent a C string constant.)

    NABoolean scan(const NAString &objectName
                   , NABoolean createDropAlias = FALSE
                   , unsigned short toInternalIdentifierFlags = NASTRING_ALLOW_NSK_GUARDIAN_NAME_FORMAT
                  );

        // Same as above except that this method does not return
        // the number of bytes scanned.

}; // class ComObjectName

// -----------------------------------------------------------------------
// definition of class ComRoutineActionName
// -----------------------------------------------------------------------

class ComRoutineActionName : public ComObjectName
{

  //
  // global friend functions
  //

  friend ostream& operator<<(ostream &out, const ComRoutineActionName &name);


public:

  // ---------------------------------------------------------------------
  // static (global) functions:
  // ---------------------------------------------------------------------

  static ComBoolean extractUudfUidAndIntActNameFromFunnyIntActNameInOBJECTS_OBJECT_NAME
                    ( const ComString & funnyIntActNameInOBJECTS_OBJECT_NAME  // in
                    , ComUID          & uudfUid                               // out
                    , ComString       & computedIntActName                    // out
                    );

  // ---------------------------------------------------------------------
  // constructors
  // ---------------------------------------------------------------------
  //

  // Default constructor
  ComRoutineActionName(CollHeap * h = NULL); // default is to use C/C++ runtime heap

  // Copy constructor
  ComRoutineActionName( const ComRoutineActionName &rhs
                      , CollHeap * h = NULL
                      );

  // Initializing constructors

  ComRoutineActionName( const ComUID        & uudfUid  // e.g., UID of UUDF sas_score
                      , const NAString      & uudfNameAsAnsiName
                      , const NAString      & routineActionNameAsAnsiName
                      , CollHeap            * h = NULL
                      );

        // Initializing constructor.  The method scans the input
        // routine action name to find out whether the input name is legal.
        // If the input routine action name is legal, the method decomposes
        // the input into separate name components.  If the input
        // name does not include the catalog and schema name parts, the
        // corresponding data members of those name parts are to be empty.
        //
        // If the input routine action name is illegal, the constructed
        // object is empty.
        //
        // Examples of the contents of the input
        // routineActionNameAsAnsiName parameter:
        //
        //   routine_action
        //   sch.routine_action
        //   cat.sch."$A ROUTINE ACTION"

  ComRoutineActionName( const ComUID        & uudfUid             // e.g., UID of UUDF sas_score
                      , const ComObjectName & uudfName
                      , const NAString      & catalogNamePart
                      , const NAString      & schemaNamePart
                      , const NAString      & routineActionNamePart
                      , const ComAnsiNamePart::formatEnum  format = ComAnsiNamePart::EXTERNAL_FORMAT
                      , ComBoolean parseInputNameParts = TRUE
                      , CollHeap * h = NULL
                      );

        // Initializing constructor - If parameter  format  contains
        // the value ComAnsiNamePart::EXTERNAL_FORMAT (the default),
        // the first three (3) parameters (whose names contain the
        // NamePart suffix) must contain the name parts in external
        // format (the format used by ANSI SQL users).  If parameter
        // format  contains the value ComAnsiNamePart::INTERNAL_FORMAT,
        // the parameters ...NamePart must contain the name parts in
        // internal format (the format stored in the Metadata Dictionary
        // tables).
        //
        // When parseInputNameParts is set to TRUE, the contents of the
        // input parameters catalogNamePart, schemaNamePart, and
        // routineActionNamePart will be scanned/parsed for correctness;
        // otherwise, the input name parts are assumed to be legal
        // without any checking (thus saving CPU cycles).
        //
        // If an input name part is illegal, the constructed object is
        // empty.  If the catalog name part is not empty and the schema
        // name part is empty, the constructed object is empty (that is,
        // the input name parts are considered as being illegal).

  // ---------------------------------------------------------------------
  // virtual destructor
  // ---------------------------------------------------------------------

  virtual ~ComRoutineActionName();

  // ---------------------------------------------------------------------
  // virtual cast methods
  // ---------------------------------------------------------------------

  virtual const ComRoutineActionName * castToComRoutineActionName() const;
  virtual       ComRoutineActionName * castToComRoutineActionName();

  // ---------------------------------------------------------------------
  // accessors
  // ---------------------------------------------------------------------

  const ComUID                   & getUudfUID()           const { return routineActionNamePart_.getUudfUID(); }
  const ComObjectName            & getUudfComObjectName() const { return uudfComObjectName_; }
  const ComRoutineActionNamePart & getRoutineActionNamePart() const { return routineActionNamePart_; }
  const NAString                 & getRoutineActionNamePartAsAnsiString(NABoolean = FALSE) const;

  virtual NAString getExternalName(NABoolean formatForDisplay = FALSE,
                                   NABoolean displayedExternally = FALSE) const;

        // returns an empty string if this object is empty; otherwise,
        // returns the ANSI SQL object name, in external-format
        // (the format used by ANSI SQL users).  If the catalog name part
        // does not exist (in this object), it will not be included
        // in the returned object name (in external format).  If the
        // schema name part does not exist, only the object name part
        // (in external format) is returned.  If the name is to be displayed
        // externall, for example in SHOWDDL, the catalog name is not included.

  // ---------------------------------------------------------------------
  // mutators
  // ---------------------------------------------------------------------

  NABoolean set ( const ComUID   & uudfUid  // e.g., UID of UUDF sas_score
                , const NAString & uudfNameAsAnsiName
                , const NAString & routineActionNameAsAnsiName
                );

        // The method scans the input routine action name to find out
        // whether the input name is legal.
        //
        // If the input routine action name is legal, the method decomposes
        // the input into separate name components.  If the input
        // name does not include the catalog and schema name parts, the
        // corresponding data members of those name parts are to be empty.
        // The method returns TRUE for this case.
        //
        // If the input routine action name is illegal, the object will
        // be cleared and the method will return FALSE.
        //
        // Examples of the contents of the input
        // routineActionNameAsAnsiName parameter:
        //
        //   routine_action
        //   sch.routine_action
        //   cat.sch."$A ROUTINE ACTION"

  NABoolean set ( const ComUID        & uudfUid                 // e.g., UID of UUDF sas_score
                , const ComObjectName & uudfObjName             // in
                , const NAString      & catalogNamePart         // in
                , const NAString      & schemaNamePart          // in
                , const NAString      & routineActionNamePart   // in
                , const ComAnsiNamePart::formatEnum  format = ComAnsiNamePart::EXTERNAL_FORMAT
                , ComBoolean parseInputNameParts = TRUE
                );
  void           setUudfComObjectName(const ComObjectName &src) { uudfComObjectName_ = src; }
  ComObjectName &getUudfComObjectName()                         { return uudfComObjectName_; }

  inline void setRoutineActionNamePart (const ComRoutineActionNamePart  &actionNamePart);
 
  // ---------------------------------------------------------------------
  // helpers
  // ---------------------------------------------------------------------

  virtual NABoolean isEmpty() const;
  virtual NABoolean isValid() const;

  virtual void clear();
  virtual void clearIfInvalid();  // Make all data members in this object empty.


private:

  ComRoutineActionNamePart   routineActionNamePart_;
  ComObjectName              uudfComObjectName_;

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  // The following methods are not defined - DO NOT USE them

        ComRoutineActionName& operator= (const NAString             &rhs);       // DO NOT USE
        ComRoutineActionName& operator= (const ComRoutineActionName &rhs);       // DO NOT USE
  const ComBoolean            operator==(const ComRoutineActionName &rhs) const; // DO NOT USE
  const ComBoolean            operator>=(const ComRoutineActionName &rhs) const; // DO NOT USE

  // The following method is called by one of the initializing constructors
  // and the set() method that is similar to the initializing constructor.
  // The following method should be not called from any other places.

  NABoolean initializeRoutineActionNamePart
                      ( const ComUID        & uudfUid                 // e.g., UID of UUDF sas_score
                      , const ComObjectName & uudfObjName             // in
                      , const NAString      & catalogNamePart         // in
                      , const NAString      & schemaNamePart          // in
                      , const NAString      & routineActionNamePart   // in
                      , const ComAnsiNamePart::formatEnum  format = ComAnsiNamePart::EXTERNAL_FORMAT
                      , ComBoolean parseInputNameParts = TRUE
                      );

}; // class ComRoutineActionName

// -----------------------------------------------------------------------
// definitions of inline methods for class ComObjectName
// -----------------------------------------------------------------------

// virtual inline method
NABoolean
ComObjectName::isEmpty() const
{
  return (objectNamePart_.isEmpty() AND
	  schemaNamePart_.isEmpty() AND
	  catalogNamePart_.isEmpty());
}

// virtual inline method
NABoolean
ComObjectName::isValid() const
{
  return (NOT objectNamePart_.isEmpty());
}

// ----------------------------------------------------------------------------
// Method: isExternalHive
//
// Looks at the prefix and suffix of the schema name to see the schema contains
// external (native) hive table information.
//
// returns TRUE if it is a HIVE schema
// ----------------------------------------------------------------------------
NABoolean
ComObjectName::isExternalHive() const
{
  NAString schemaName(schemaNamePart_.getInternalName());

  if (ComIsTrafodionExternalSchemaName(schemaName))
    return (schemaName(0,sizeof(HIVE_EXT_SCHEMA_PREFIX)-1) == HIVE_EXT_SCHEMA_PREFIX); 
  return FALSE;
}
  
// ----------------------------------------------------------------------------
// Method: isExternalHbase
//
// Looks at the prefix and suffix of the schema name to see the schema contains
// external (native) hbase table information.
//
// returns TRUE if it is a hbase schema
// ----------------------------------------------------------------------------
NABoolean
ComObjectName::isExternalHbase() const
{
  NAString schemaName(schemaNamePart_.getInternalName());

  if (ComIsTrafodionExternalSchemaName(schemaName))
    return (schemaName(0,sizeof(HBASE_EXT_SCHEMA_PREFIX)-1) == HBASE_EXT_SCHEMA_PREFIX); 
  return FALSE;
}

NABoolean ComObjectName::isHBaseMappedExtFormat() const
{
  return ComIsHBaseMappedExtFormat(getCatalogNamePartAsAnsiString(), 
                                   getSchemaNamePartAsAnsiString(TRUE));
}

NABoolean ComObjectName::isHBaseMappedIntFormat() const
{
  return ComIsHBaseMappedIntFormat(getCatalogNamePartAsAnsiString(), 
                                   getSchemaNamePartAsAnsiString(TRUE));
}

void
ComObjectName::setCatalogNamePart(const ComAnsiNamePart &catalogNamePart)
{
  catalogNamePart_ = catalogNamePart;
}

void
ComObjectName::setSchemaNamePart(const ComAnsiNamePart &schemaNamePart)
{
  schemaNamePart_ = schemaNamePart;
}

void
ComObjectName::setObjectNamePart(const ComAnsiNamePart &objectNamePart)
{
  objectNamePart_ = objectNamePart;
}

void
ComObjectName::setNameSpace(const ComAnsiNameSpace nameSpace)
{
  nameSpace_ = nameSpace;
}

ComBoolean       
ComObjectName::operator==(const ComObjectName &rhs) const
{
  return ((catalogNamePart_ EQU rhs.catalogNamePart_) AND
	  (schemaNamePart_  EQU rhs.schemaNamePart_) AND
	  (objectNamePart_  EQU rhs.objectNamePart_) AND
	  (nameSpace_       EQU rhs.nameSpace_));
}

ComBoolean       
ComObjectName::operator>=(const ComObjectName &rhs) const
{
  if (nameSpace_ GT rhs.nameSpace_) return FALSE;
  if (nameSpace_ LT rhs.nameSpace_) return TRUE;
  
  Int32 compResult = catalogNamePart_.compareTo (rhs.catalogNamePart_);
  if (compResult GT 0) return FALSE;
  if (compResult LT 0) return TRUE;
  
  compResult = schemaNamePart_.compareTo (rhs.schemaNamePart_);
  if (compResult GT 0) return FALSE;
  if (compResult LT 0) return TRUE;
  
  compResult = objectNamePart_.compareTo (rhs.objectNamePart_);
  if (compResult GT 0) return FALSE;
  else return TRUE;
}

inline void
ComObjectName::getComSchemaNameObject(ComSchemaName & comSchNameObj) const // out
{
  comSchNameObj.setCatalogNamePart(getCatalogNamePart());
  comSchNameObj.setSchemaNamePart (getSchemaNamePart());
}

// -----------------------------------------------------------------------
// definitions of inline methods for class ComRoutineActionName
// -----------------------------------------------------------------------

void
ComRoutineActionName::setRoutineActionNamePart(const ComRoutineActionNamePart &actionNamePart)
{
  routineActionNamePart_ = actionNamePart;
}

#endif // COMOBJECTNAME_H

