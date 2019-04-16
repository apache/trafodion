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
#ifndef OBJECTNAMES_H
#define OBJECTNAMES_H
/* -*-C++-*-
******************************************************************************
*
* File:         ObjectNames.h
* Description:  External SQL object names
* Created:      4/27/94
* Language:     C++
*
*
*	I am bound to call upon you;
*	And I pray you, your name?
*		-- Duke Vincentio, Measure for Measure, III:ii
*
******************************************************************************
*/

#include <time.h>
#include <sys/time.h>

#include "CmpCommon.h"
#include "NAString.h"
#include "NAStringDef.h"
#include "charinfo.h"
#include "ComMisc.h"

// -----------------------------------------------------------------------
// forward declarations
// -----------------------------------------------------------------------

class BindWA;
class ComObjectName;
class HostVar;
class ColumnDescList;
class RETDesc;

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------

class CatalogName;
class SchemaName;
class QualifiedName;
class CorrName;
class PartitionClause;
class ExtendedQualName;
class ColRefName;
class TableRefName;
class ObjectCounter;
class TaskMonitor;

// -----------------------------------------------------------------------
// Helpful Definitions. Moved here from StmtDDLNode.h
// -----------------------------------------------------------------------
typedef LIST(const NAString *)    ConstStringList;

// -----------------------------------------------------------------------
// Function GetAnsiNameCharSet() returns the character set attribute
// of the ANSI SQL names returned by the methods declared in this
// header file (and many others like those in w:/smdio, etc.)
// -----------------------------------------------------------------------

#define OBJECTNAMECHARSET ((CharInfo::CharSet)ComGetNameInterfaceCharSet())

CharInfo::CharSet GetAnsiNameCharSet();

// -----------------------------------------------------------------------
// Catalog name
// -----------------------------------------------------------------------
class CatalogName : public NABasicObject
{

public:

  // constructors
  CatalogName(const NAString & catalogName = "",
              CollHeap * h=0,
              StringPos namePos = 0) :
       catalogName_(catalogName, h),
       isDelimited_(FALSE)
  { if (catalogName.isNull() && namePos == 0) namePos_ = 0;
    else 
    {
      setNamePosition(namePos,FALSE);
    }
  }

  // copy ctor
  CatalogName (const CatalogName & cat, NAMemory * h) :
       catalogName_(cat.catalogName_, h),
       namePos_(cat.namePos_),
       isDelimited_(cat.isDelimited_)
  {}

  // accessors
  const NAString& getCatalogName() const	{ return catalogName_;  }
  const NAString  getCatalogNameAsAnsiString() const
				{ return ToAnsiIdentifier(catalogName_); }
  StringPos getNamePosition() const		{ return namePos_; }

  // Method isDelimited() is implemented enough to solve all the cases
  // of Genesis 10-970902-0878, but no further.
  //
  // Additional setNamePosition() or setIsDelimited() calls would need to be
  // made in SqlParser.y to have this return TRUE in all other cases of
  // a one-part identifier (CorrName or ColRefName),
  // and more work still here in ObjectNames.* for multi-part names.
  //
  NABoolean isDelimited() const			{ return isDelimited_; }

  // mutators
  void setCatalogName(const NAString &catName)  { catalogName_ = catName; }
  void setNamePosition(StringPos namePos, NABoolean setDelimited = TRUE);

  // Display/print, for debugging.
  void display() const;
  virtual void print(FILE* ofd = stdout,
		     const char* indent = DEFAULT_INDENT,
		     const char* title = "CatalogName") const;
protected:

  void setIsDelimited(NABoolean isDelimited)	{ isDelimited_ = isDelimited; }
  NAString catalogName_;
  StringPos namePos_;
  NABoolean isDelimited_;

}; // CatalogName

// -----------------------------------------------------------------------
// Schema names
//
// A schema name is used to identify a SQL schema.  It consists of
// a one or two part name, [ catalog . ] schema.
// -----------------------------------------------------------------------
class SchemaName : public CatalogName
{

public:

  // constructors

  SchemaName (CollHeap * h) :
       CatalogName("",h),
       schemaName_(h)
  {}

  SchemaName(const NAString & schemaName  = "",
	     const NAString & catalogName = "",
             CollHeap * h=0,
             StringPos namePos = 0) :
    CatalogName(catalogName, h, namePos),
    schemaName_(schemaName, h)
  { CMPASSERT(getCatalogName().isNull() OR NOT getSchemaName().isNull()); 
  }

  // copy ctor
  SchemaName (const SchemaName & sch, NAMemory * h) :
       CatalogName(sch, h),
       schemaName_(sch.schemaName_, h)
  {}

  // accessors
  const NAString& getSchemaName() const		{ return schemaName_;    }
  const NAString  getSchemaNameAsAnsiString() const;
  const NAString  getUnqualifiedSchemaNameAsAnsiString() const
				 { return ToAnsiIdentifier(schemaName_); }

  // Translate Trafodion to Hive schema
  void getHiveSchemaName(NAString &hiveSchemaName) const;

  // mutator
  void setSchemaName(const NAString &schName)   { schemaName_ = schName; }

  // Display/print, for debugging.
  virtual void print(FILE* ofd = stdout,
		     const char* indent = DEFAULT_INDENT,
		     const char* title = "SchemaName") const;

  // for DEFAULT_SCHEMA_ACCESS_ONLY
  NABoolean matchDefaultPublicSchema();

protected:

  NAString schemaName_;

}; // SchemaName

// -----------------------------------------------------------------------
// Qualified names
//
// A qualified name is used to identify a SQL object.  It consists of
// a one, two, or three part name, [ [ catalog . ] schema . ] object.
// -----------------------------------------------------------------------
class QualifiedName : public SchemaName
{

public:

  // constructors
  //
  // CenterLine C++ won't handle const parameters with defaults, so
  //      const NAString &objectName  = "",
  // won't work.
  // Likewise, c89 can't disambiguate (for no good reason!) the ctor
  // if the first parameter has a default.
  //
  QualifiedName(CollHeap * h=0) :
       SchemaName(h),
       objectName_(h),
       objectNameSpace_(COM_UNKNOWN_NAME),
       flagbits_(0)
  {}

  QualifiedName(const NAString & objectName /* = "" */ ,
	        const NAString & schemaName  = "",
	        const NAString & catalogName = "",
                CollHeap * h=0,
                StringPos namePos = 0) :
    SchemaName(schemaName, catalogName, h, namePos),
    objectName_(objectName, h),
    objectNameSpace_(COM_UNKNOWN_NAME),
    flagbits_(0)
  {}

  QualifiedName(const NAString & ansiString,
  		Int32 minNameParts,
                CollHeap * h=0,
  		BindWA *bindWA = NULL);

  QualifiedName(const ComObjectName & comObjNam, CollHeap * h=0);

  // copy ctor
  QualifiedName (const QualifiedName & qual, NAMemory * h) :
       SchemaName(qual, h),
       objectName_(qual.objectName_, h),
       objectNameSpace_(qual.objectNameSpace_),
       flagbits_(qual.flagbits_)
  {}

  // operators
  QualifiedName & operator=(const QualifiedName &rhs);
  inline NABoolean operator==(const QualifiedName&) const;
  inline NABoolean operator!=(const QualifiedName&) const;
  NABoolean operator==(const NAString&) const;
  inline NABoolean operator!=(const NAString&) const;

  // accessors
  ULng32 hash() const;
  static ULng32 hash(const QualifiedName& name) { return name.hash(); }

  NAString  getQualifiedNameAsString(NABoolean formatForDisplay = FALSE,
  				     NABoolean externalDisplay = FALSE) const;
  const NAString  getQualifiedNameAsAnsiString(NABoolean formatForDisplay = FALSE,
  					       NABoolean externalDisplay = FALSE) const;
  const NAString  getQualifiedNameAsAnsiString(
		     size_t *lenArray /* array[5] */) const;
  const NAString  getQualifiedNameAsAnsiNTFilenameString() const;
  const NAString& getObjectName() const		{ return objectName_; }
  const NAString  getUnqualifiedObjectNameAsAnsiString() const
                                { return ToAnsiIdentifier(objectName_); }

  ComAnsiNameSpace getObjectNameSpace() const { return objectNameSpace_; }


  NABoolean fullyExpanded() const	{ return catalogName_ != (const char *)""; }

  Int32 numberExpanded() const {
    if(NOT catalogName_.isNull()) return 3;
    if(NOT schemaName_.isNull()) return 2;
    if(NOT objectName_.isNull()) return 1;
    return 0;
  }

  // Name parts return in string parameters, defaulted if not yet present;
  // this object is not modified.
  // Function return value is the number of names that match the default,
  // {0, 1, 2} = {no matches, catalog matches, catalog&schema match}.
  Int32		  extractAndDefaultNameParts( const SchemaName& defCatSch
					    , NAString& catName
					    , NAString& schName
					    , NAString& objName
                                            ) const;

  // Mutator to fill in default catalog & schema if not already present.
  Int32 applyDefaults(const SchemaName& defCatSch);

  // apply defaults and validate the object existence
  Int32 applyDefaultsValidate(const SchemaName& defCatSch,
                            ComAnsiNameSpace nameSpace = COM_TABLE_NAME);

  NABoolean applyShortAnsiDefault(NAString& catName, NAString& schName) const;
  
  NABoolean verifyAnsiNameQualification(Int32 nbrPartsDefaulted) const;

  static NABoolean isHive(const NAString &catName);
  static NABoolean isHbase(const NAString &catName);
  static NABoolean isSeabase(const NAString &catName);

  NABoolean isHive() const;
  NABoolean isSeabase() const;
  NABoolean isHbase() const;
  NABoolean isSeabaseMD() const;
  NABoolean isSeabasePrivMgrMD() const;
  NABoolean isHbaseMappedName() const;

  NABoolean isHbaseCell() const;
  NABoolean isHbaseRow() const;
  NABoolean isHbaseCellOrRow() const;

  NABoolean isHistograms() const;
  NABoolean isHistogramIntervals() const;
  NABoolean isHistogramTable() const;
  NABoolean isLOBDesc() const;
  void setObjectName(const NAString &objName)   { objectName_ = objName; }
  void setObjectNameSpace(ComAnsiNameSpace objNameSpace)   { objectNameSpace_ = objNameSpace; }

  NABoolean isVolatile() const      { return (flagbits_ & IS_VOLATILE) != 0; }
  void setIsVolatile(NABoolean v)
  { (v ? flagbits_ |= IS_VOLATILE : flagbits_ &= ~IS_VOLATILE);}

  NABoolean isGhost() const      { return (flagbits_ & IS_GHOST) != 0; }
  void setIsGhost(NABoolean v)
  { (v ? flagbits_ |= IS_GHOST : flagbits_ &= ~IS_GHOST);}


  // Display/print, for debugging.
  virtual void print(FILE* ofd = stdout,
		     const char* indent = DEFAULT_INDENT,
		     const char* title = "QualifiedName") const;
private:
  enum FlagBits 
  {
    IS_VOLATILE   = 0x01,
    IS_GHOST        = 0x02
  };

  NAString objectName_;
  ComAnsiNameSpace objectNameSpace_;

  Int32 flagbits_;

}; // QualifiedName

// -----------------------------------------------------------------------
// Partition Name, Number or Range.
//
// Holds the partition name, number, location name or
// a partition range specfied by the user in DML.
// These could be specified as part of 'special table name' clause and
// restrict DML actions to these partitions only.
// -----------------------------------------------------------------------
class PartitionClause : public NABasicObject
{

public:

  // constructors
  PartitionClause(const NAString& pname,CollHeap * h=0) :
       partName_(pname, h),
       beginPartNumber_(-1),
       endPartNumber_(-1),
       locationName_("",h),
       partnNameSpecified_(TRUE),
       partnNumSpecified_(FALSE),
       partnRangeSpecified_(FALSE)
  {}

  PartitionClause(const Int32 pnum, CollHeap * h=0) :
       partName_("", h),
       beginPartNumber_(pnum),
       endPartNumber_(-1),
       locationName_("",h),
       partnNameSpecified_(FALSE),
       partnNumSpecified_(TRUE),
       partnRangeSpecified_(FALSE)
  {}

  PartitionClause(CollHeap * h=0) :
       partName_("", h),
       beginPartNumber_(-1),
       endPartNumber_(-1),
       locationName_("",h),
       partnNameSpecified_(FALSE),
       partnNumSpecified_(FALSE),
       partnRangeSpecified_(FALSE)
  {}

  PartitionClause(const Int32 beginPnum, const Int32 endPnum,
		  CollHeap * h=0) :
       partName_("", h),
       beginPartNumber_(beginPnum),
       endPartNumber_(endPnum),
       locationName_("",h),
       partnNameSpecified_(FALSE),
       partnNumSpecified_(FALSE),
       partnRangeSpecified_(TRUE)
  {}

  // copy ctor
  PartitionClause (const PartitionClause& pClause , CollHeap * h=0) :
       partName_(pClause.partName_, h),
       beginPartNumber_(pClause.beginPartNumber_),
       endPartNumber_(pClause.endPartNumber_),
       locationName_(pClause.locationName_,h),
       partnNameSpecified_(pClause.partnNameSpecified_),
       partnNumSpecified_(pClause.partnNumSpecified_),
       partnRangeSpecified_(pClause.partnRangeSpecified_)
  {}

  // operators
  NABoolean operator==(const PartitionClause&) const;
  NABoolean operator!=(const PartitionClause&) const;
  PartitionClause& operator= (const PartitionClause&) ;
  ULng32 hash() const ;
  NABoolean isEmpty() const;

  // accessors
  const NAString& getPartitionName() const	{ return partName_; }
  const Int32 getPartitionNumber() const		{ return beginPartNumber_; }
  const Int32 getBeginPartitionNumber() const	{ return beginPartNumber_; }
  const Int32 getEndPartitionNumber() const	{ return endPartNumber_; }

  const NAString& getLocationName() const	{ return locationName_; }

  const NABoolean partnNameSpecified() const    { return partnNameSpecified_;}
  const NABoolean partnNumSpecified() const     { return partnNumSpecified_;}
  const NABoolean partnRangeSpecified() const   { return partnRangeSpecified_;}

  //mutator
  void setLocationName(const NAString& locName) { locationName_ = locName; }

private:

  NAString  partName_;
  Int32       beginPartNumber_;
  Int32       endPartNumber_;
  NAString  locationName_;

  NABoolean partnNameSpecified_;
  NABoolean partnNumSpecified_;
  NABoolean partnRangeSpecified_;
}; // PartitionClause

// -----------------------------------------------------------------------
// Extended Qualified Name
// This class is introduced to abstract the Qualified ANSI name,
// special type and the Location Name, so that we can lookup NATableDB
// on the ExtendedQualName.
// -----------------------------------------------------------------------
class ExtendedQualName : public NABasicObject
{
public:
  enum SpecialTableType { NORMAL_TABLE,
			  INDEX_TABLE,
			  ISP_TABLE, 		// Internal Stored Procedure
			  LOAD_TABLE,
			  PARTITION_TABLE,
			  VIRTUAL_TABLE,
			  IUD_LOG_TABLE, 
			  RANGE_LOG_TABLE,
                          HBMAP_TABLE,
			  MVS_UMD,
			  MV_TABLE,
			  TRIGTEMP_TABLE,      // Temporary Tables for Triggers
			  SCHEMA_SECURITY_TABLE,
			  GHOST_TABLE,
			  GHOST_INDEX_TABLE,
			  GHOST_MV_TABLE,
			  GHOST_TRIGTEMP_TABLE,
			  EXCEPTION_TABLE,
			  GHOST_IUD_LOG_TABLE,
			  SG_TABLE,
			  LIBRARY_TABLE,
                          SCHEMA_TABLE
                        };

  // constructors
  ExtendedQualName(NAString objectName  = "",
                   NAString schemaName 	= "",
                   NAString catalogName	= "",
                   CollHeap * h=0,
                   NAString locName     = "",
                   StringPos namePos = 0) :
    qualName_(objectName, schemaName, catalogName, h, namePos),
    type_(NORMAL_TABLE),
    utilityOpenId_(-1),
    isNSAOperation_(FALSE),
    partnClause_(h)
  {
    setLocationName(locName);
  }

  ExtendedQualName(const QualifiedName &qualName,
                   CollHeap * h=0,
                   NAString locName   = (const char *)"") :
    qualName_(qualName, h),
    type_(NORMAL_TABLE),
    utilityOpenId_(-1),
    isNSAOperation_(FALSE),
    partnClause_(h)
  {
    setLocationName(locName);
  }

  ExtendedQualName(const ExtendedQualName & orig, CollHeap * h=0) :
    qualName_(orig.qualName_, h),
    type_(orig.type_),
    utilityOpenId_(orig.utilityOpenId_),
    isNSAOperation_(orig.isNSAOperation_),
    partnClause_(orig.partnClause_,h)
    {}

  // operators
  NABoolean operator==(const ExtendedQualName&) const;
  NABoolean operator!=(const ExtendedQualName&) const;

  // accessors
  ULng32 hash() const;
  NABoolean isEmpty() const;
  NABoolean isLocationNameSpecified() const { return NOT getLocationName().isNull(); }
  NABoolean isPartitionNameSpecified() const 
  { return (getPartnClause().partnNameSpecified() || 
	    getPartnClause().partnNumSpecified()); }
  NABoolean isPartitionRangeSpecified() const 
  { return (getPartnClause().partnRangeSpecified()); }
  NABoolean isUtilityOpenIdSpecified() const {return (utilityOpenId_ != -1); }
  NABoolean isNSAOperation() const {return isNSAOperation_; }
  NABoolean isSpecialTable() const	  
	{ return    (type_ != NORMAL_TABLE)
	         && (type_ != TRIGTEMP_TABLE)
	         && (type_ != EXCEPTION_TABLE)
	         && (type_ != IUD_LOG_TABLE)
	         && (type_ != RANGE_LOG_TABLE)
	         && (type_ != GHOST_TABLE)
	         && (type_ != GHOST_IUD_LOG_TABLE)
	; }

  NABoolean isGhost() const	  
	{ return    (type_ == GHOST_TABLE)
	         || (type_ == GHOST_INDEX_TABLE)
	         || (type_ == GHOST_MV_TABLE)
	         || (type_ == GHOST_IUD_LOG_TABLE)
	; }

  //can this type of object be put in the NATable cache
  NABoolean isCacheable() const
  { 
    return    (type_ == NORMAL_TABLE)
      || (type_ == TRIGTEMP_TABLE)
      || (type_ == EXCEPTION_TABLE)
      //|| (type_ == IUD_LOG_TABLE)
      //|| (type_ == RANGE_LOG_TABLE)
      || (type_ == INDEX_TABLE)
      || (type_ == SCHEMA_TABLE)
      || (type_ == SG_TABLE)
      || ((type_ == GHOST_TABLE))
  ; }
  
  NABoolean hasPartnClause() const {return NOT partnClause_.isEmpty();}

  StringPos getNamePosition() const	  { return qualName_.getNamePosition();}
  const NAString &getLocationName() const { return partnClause_.getLocationName(); }
  Int64 getUtilityOpenId() const { return utilityOpenId_; }
  const QualifiedName& getQualifiedNameObj() const	{ return qualName_; }
  QualifiedName& getQualifiedNameObj()		        { return qualName_; }
  SpecialTableType getSpecialType() const		{ return type_;     }
  static ComAnsiNameSpace convSpecialTableTypeToAnsiNameSpace( const SpecialTableType type ); // ++MV
  ComAnsiNameSpace getAnsiNameSpace() const;		// ++MV
  const PartitionClause& getPartnClause() const {return partnClause_ ;}
  NAString getExtendedQualifiedNameAsString() const;

  // Mutator to fill in qualified name from prototype value, if one.
  void setLocationName(const NAString& locName) { partnClause_.setLocationName(locName); }
  void setUtilityOpenId(Int64 &openId) { utilityOpenId_ = openId; }
  void setIsNSAOperation(NABoolean isNSAOperation) 
                               {isNSAOperation_ = isNSAOperation;}
  void setNamePosition(StringPos pos, NABoolean setD = TRUE)
				    { qualName_.setNamePosition(pos, setD); }
  void setSpecialType(SpecialTableType stt)	 	{ type_ = stt;      }
  void setPartnClause(const PartitionClause& pClause) {partnClause_ = pClause;}

  const NAString getSpecialTypeName() const;

  static NABoolean isDescribableTableType(SpecialTableType type)
  { 
    return (type == NORMAL_TABLE	||
	    type == INDEX_TABLE         ||
	    type == IUD_LOG_TABLE	||
	    type == RANGE_LOG_TABLE	||
	    type == MVS_UMD		||
	    type == MV_TABLE		||
	    type == TRIGTEMP_TABLE      ||
	    type == EXCEPTION_TABLE     ||
	    type == GHOST_TABLE         ||
	    type == GHOST_INDEX_TABLE   ||
	    type == GHOST_MV_TABLE      ||
	    type == GHOST_IUD_LOG_TABLE ||
            type == SG_TABLE
 );
  }

  // Display/print, for debugging.
  const NAString getText() const;
  const NAString getTextWithSpecialType() const;
  void display() const;
  void print(FILE* ofd = stdout,
	     const char* indent = DEFAULT_INDENT,
             const char* title = "ExtendedQualName") const;

private:

  QualifiedName		qualName_;		// identity-discriminant
  SpecialTableType	type_;			// identity-discriminant

  // The following two memebers are used by SQL/MX utility
  // along with LOCATION clause. Think about factoring out
  // the LOCATION clause info into a seperate class if this
  // list gets larger.

  // Stores the 64-bit id that SQL/MX Utility passes 
  // in the LOCATION clause. 
  // Eg: LOCATION <locationName_> OPEN <utilityOpenId_>. Note that
  // the OPEN clause is optional.
  Int64                 utilityOpenId_;

  // This member is set to TRUE, if the SQL/MX Utility 
  // specifies WITH SHARE ACCESS along with LOCATION clause.
  // Eg. LOCATION <locationName_> WITH SHARE ACCESS.
  NABoolean		isNSAOperation_;

  // Holds the partition name/number specified by PARTITION clause of DML
  PartitionClause	partnClause_;	      // identity-discriminant

}; //ExtendedQualName


// -----------------------------------------------------------------------
// Correlation names
//
// A correlation name is used to identify a table.  It consists of a
// qualified name ([ [ catalog . ] schema . ] table) and of a correlation
// (shortcut) name for the table.  If no user-assigned correlation name
// is specified, then the entire table name becomes exposed,
// that is it becomes the implicit correlation name.
// See ANSI 6.3 and 6.4.
//
// CorrName also (optionally) contains information if this is a 'special' table.
// These are 'virtual' tables for special purpose needs.
// Binder uses this information to 'generate' table descriptors.
// Ustat does not fetch real histograms for virtual tables.
// -----------------------------------------------------------------------
class CorrName : public NABasicObject
{
private:
  enum FlagBits 
  { 
    OFF           = 0, 
    ISFABRICATED = 0x1, 
    IS_VOLATILE  = 0x2,
    IS_EXTERNAL  = 0x4
  };

public:

  // constructors
  CorrName(const NAString & objectName  = "",
           CollHeap * h=0,
	   const NAString & schemaName 	= "",
	   const NAString & catalogName	= "",
	   const NAString & corrName   	= "",
           const NAString & locName     = "",
           StringPos namePos = 0,
	   HostVar *prototype   = NULL) :
    qualName_(objectName, schemaName, catalogName, h, namePos),
    corrName_(corrName, h),
    //ct-bug-10-030102-3803 -Begin
    ugivenName_("", h),
    //ct-bug-10-030102-3803 -End
    prototype_(prototype),
    bound_(FALSE),
    defaultMatchCount_(-1),
    flagbits_(0)
  {
    setIsExternal(ComIsTrafodionExternalSchemaName(schemaName));
    setLocationName (locName) ;
  }

  CorrName(const QualifiedName & qualName,
           CollHeap * h=0,
           const NAString & corrName  = "",
           const NAString & locName   = "",
	   HostVar *prototype = NULL) :
    qualName_(qualName, h),
    corrName_(corrName, h),
    //ct-bug-10-030102-3803 -Begin
    ugivenName_("", h),
    //ct-bug-10-030102-3803 -End
    prototype_(prototype),
    bound_(FALSE),
    defaultMatchCount_(-1),
    flagbits_(0)
  {
    setIsExternal(ComIsTrafodionExternalSchemaName(qualName.getSchemaName()));
    setLocationName (locName) ;
  }

  // copy ctor
  CorrName (const CorrName & corr, CollHeap * h=0) :
       qualName_(corr.qualName_, h),
       corrName_(corr.corrName_, h),
       //ct-bug-10-030102-3803 -Begin
       ugivenName_(corr.ugivenName_, h),
       //ct-bug-10-030102-3803 -End
       prototype_(corr.prototype_),
       bound_(corr.bound_),
       defaultMatchCount_(corr.defaultMatchCount_),
       flagbits_(corr.flagbits_)
  {}

  CorrName(const NAString& corrName,
  	   NABoolean isFabricated,
           CollHeap * h=0,
	   HostVar *prototype = NULL) :
    qualName_("","", h),
    corrName_(corrName, h),
    //ct-bug-10-030102-3803 -Begin
    ugivenName_("", h),
    //ct-bug-10-030102-3803 -End
    prototype_(prototype),
    bound_(FALSE),
    defaultMatchCount_(-1),
    flagbits_(isFabricated ? ISFABRICATED : 0)
  {}

  // assignment
  CorrName& operator= (const CorrName&) ;

  // operators
  NABoolean operator==(const CorrName&) const;
  NABoolean operator!=(const CorrName&) const;
  NABoolean operator==(const NAString&) const;
  NABoolean operator!=(const NAString&) const;

  // accessors
  StringPos getNamePosition() const 	 { return qualName_.getNamePosition();}
  HostVar *getPrototype() const				{ return prototype_;  }
  ULng32 hash() const;
  NABoolean isEmpty() const;
  NABoolean isFabricated() const      { return flagbits_ & ISFABRICATED; }

  NABoolean isVolatile() const      { return (qualName_.getQualifiedNameObj().isVolatile()); }
  void setIsVolatile(NABoolean v)   { qualName_.getQualifiedNameObj().setIsVolatile(v);      }

  NABoolean nodeIsBound() const				{ return bound_; }
  void markAsBound()					{ bound_ = TRUE; }

  NABoolean isSpecialTable() const	  { return qualName_.isSpecialTable(); }
  NABoolean isCacheable() const       { return qualName_.isCacheable(); }
  ExtendedQualName::SpecialTableType getSpecialType() const
					  { return qualName_.getSpecialType(); }
  ComAnsiNameSpace getAnsiNameSpace() const
					  { return qualName_.getAnsiNameSpace(); }
  void setSpecialType(ExtendedQualName::SpecialTableType stt)
					  { qualName_.setSpecialType (stt); }
  void setSpecialType(const CorrName &cn)
			   { qualName_.setSpecialType (cn.getSpecialType()); }

  const ExtendedQualName& getExtendedQualNameObj() const { return qualName_; }

  // unconditional returns (in particular, regardless of isFabricated!)
  const QualifiedName& getQualifiedNameObj() const
				       {return qualName_.getQualifiedNameObj();}
  QualifiedName& getQualifiedNameObj() {return qualName_.getQualifiedNameObj();}
  NAString getCorrNameAsString() const;

  // build strings to be returned, based on condition
  NAString getQualifiedNameAsString(NABoolean formatForDisplay = FALSE) const;
  const NAString getExposedNameAsString(NABoolean debug = FALSE,
					NABoolean formatForDisplay = FALSE) const;
  const NAString getExposedNameAsAnsiString(NABoolean debug = FALSE,
					    NABoolean formatForDisplay = FALSE) const;
  const NAString getExposedNameAsStringWithPartitionNames() const;

  const NAString &getLocationName() const { return qualName_.getLocationName();}
  NABoolean isLocationNameSpecified() const
				 { return qualName_.isLocationNameSpecified(); }
  NABoolean isPartitionNameSpecified() const 
				  { return qualName_.isPartitionNameSpecified(); }
  NABoolean isPartitionRangeSpecified() const 
                                  { return qualName_.isPartitionRangeSpecified(); }
  //ct-bug-10-030102-3803 -Begin
  const NAString getUgivenName() const { return ugivenName_; }
  //ct-bug-10-030102-3803 -End

  Int64 getUtilityOpenId() const { return qualName_.getUtilityOpenId(); }
  NABoolean isUtilityOpenIdSpecified() const 
                        {return qualName_.isUtilityOpenIdSpecified(); }
  NABoolean isNSAOperation() 
    const {return qualName_.isNSAOperation(); }

  const PartitionClause& getPartnClause () const {return qualName_.getPartnClause();}
  NABoolean hasPartnClause () const {return qualName_.hasPartnClause();}
  NABoolean isEqualWithPartnClauseSkipped(const CorrName&) const ;

  // mutators

  // See QualifiedName::extractAndDefaultNameParts.
  // In addition, if this CorrName contains a correlation name
  // (masking a qualified name), the function return is always 0
  // and the name parts are *not* filled in (they're meaningless).
  Int32 extractAndDefaultNameParts(BindWA *bindWA,
  				 const SchemaName& defCatSch,
				 NAString& catName,
				 NAString& schName,
				 NAString& objName);

  // Mutator to fill in default catalog & schema if not already present.
  // Does *NOT* look at this CorrName's corr name
  // (unlike CorrName::extractAndDefaultNameParts above).
  Int32 applyDefaults(BindWA *bindWA, const SchemaName& defCatSch);

  // Mutator to fill in qualified name from prototype value, if one.
  void applyPrototype(BindWA *bindWA);

  // Only Parser should call this ('table_reference: table_name as_clause')!
  void setCorrName(const NAString& corrName);
  //ct-bug-10-030102-3803 -Begin
  void setUgivenName(const NAString& corrName);
  //ct-bug-10-030102-3803 -End
  void setLocationName(const NAString& loc) { qualName_.setLocationName(loc); }
  void setUtilityOpenId(Int64 &openId) { qualName_.setUtilityOpenId(openId); }
  void setNamePosition(StringPos pos, NABoolean setD = TRUE)
				      { qualName_.setNamePosition(pos, setD); }

  void setIsNSAOperation(NABoolean isNSAOperation) 
                               {qualName_.setIsNSAOperation(isNSAOperation);}

  void setPrototype(HostVar *hv)		{ prototype_ = hv; }
					       // hv->setPrototypeTarget(this);
  void setPartnClause(const PartitionClause& pClause)		
			      { qualName_.setPartnClause(pClause); }

  NABoolean isATriggerTransitionName(BindWA *bindWA, NABoolean onlyNew=FALSE) const;

  NABoolean isHive() const;
  NABoolean isInHiveDefaultSchema() const;
  NABoolean isSeabase() const;
  NABoolean isHbase() const;
  NABoolean isSeabaseMD() const;
  NABoolean isSeabasePrivMgrMD() const;
  NABoolean isHbaseCell() const;
  NABoolean isHbaseRow() const;
  NABoolean isHbaseMap() const;
 

  NABoolean isExternal() const { return (flagbits_ & IS_EXTERNAL) != 0; }
  void setIsExternal(NABoolean v) 
   { (v ? flagbits_ |= IS_EXTERNAL : flagbits_ &= ~IS_EXTERNAL); }

  // Display/print, for debugging.
  const NAString getTextWithSpecialType() const 
  { return qualName_.getTextWithSpecialType(); }

  const NAString getText() const;			// see RelExpr.C
  void display() const;
  void print(FILE* ofd = stdout,
	     const char* indent = DEFAULT_INDENT,
             const char* title = "CorrName") const;
  // To quote reserved words in define value. The caller is expected to provide sufficent
  // space in the INOUT parameter, defineValue should be at least 40 characters in length
  static unsigned char *quoteDefineValue(unsigned char *defineValue); 
private:

  ExtendedQualName      qualName_;              // identity-discriminant
  NAString		corrName_;		// identity-discriminant
  //ct-bug-10-030102-3803 -Begin
  NAString		ugivenName_;
  //ct-bug-10-030102-3803 -End
  NABoolean		bound_;
  Int32			flagbits_;		// identity-discriminant
  HostVar		*prototype_;
  Int32			defaultMatchCount_;

  // Before adding new members to this class, think about whether it is
  // an identity-discriminating field and so should be compared against
  // in CorrName::operator==, and be sure to read the notes in that method's
  // implementation about what other changes will be necessary.

}; // CorrName

// -----------------------------------------------------------------------
// Column reference names
//
// A column reference may be
// qualified (table.col) or unqualified (col) or a star (table.* or *).
// -----------------------------------------------------------------------
class ColRefName : public NABasicObject
{

public:

  // constructors

  ColRefName(CollHeap * h=0) :
       colName_("", h),
       corrName_("", h),
       isStar_(NO_STAR)
  {}

  // Only Parser uses the first two ctors, which set the isStar flag.
  // Parser and Binder both use the others.
  ColRefName(Int32 forceStar, CollHeap * h=0) :
       colName_("", h),
       corrName_("", h),
       isStar_(IS_STAR)
  { CMPASSERT(forceStar == TRUE); }

  ColRefName(Int32 forceStar, const CorrName& corr, CollHeap * h=0) :
       colName_("", h),
       corrName_(corr, h),
       isStar_(IS_STAR)
  { CMPASSERT(forceStar == TRUE); }

  ColRefName(const NAString& col, CollHeap * h=0) :
       colName_(col, h),
       corrName_("", h),
       isStar_(NO_STAR)
  {}

  ColRefName(const NAString& col, const CorrName& corr, CollHeap * h=0) :
       colName_(col, h),
       corrName_(corr, h),
       isStar_(NO_STAR)
  {}

  // copy ctor
  ColRefName (const ColRefName & colref, CollHeap * h=0) :
       colName_(colref.colName_, h),
       corrName_(colref.corrName_, h),
       isStar_(colref.isStar_)
  {}

  // operators
  NABoolean operator==(const ColRefName&) const;
  NABoolean operator!=(const ColRefName&) const;

  // accessors
  StringPos getNamePosition() const 	{ return corrName_.getNamePosition();}
  ULng32 hash() const;
  NABoolean isEmpty() const;
  NABoolean isFabricated() const	  { return corrName_.isFabricated(); }

  // return True if reference is a literal *, not a delimited name "*"
  // (only the Parser should be setting this flag)
  NABoolean isStar() const	 		{ return isStar_!=NO_STAR; }
  NABoolean getStarWithSystemAddedCols() const	
			      { return isStar_==STAR_WITH_SYSTEM_ADDED_COLS; }

  // return the correlation name and simple column name
  const CorrName& getCorrNameObj() const	{ return corrName_; }
        CorrName& getCorrNameObj()      	{ return corrName_; }
  const NAString& getColName() const		{ return colName_; }
  const NAString  getColNameAsAnsiString() const;

  // return "col", "corr.col", or "cat.sch.tbl.col", whichever is appropriate
  const NAString  getColRefAsAnsiString(NABoolean debug = FALSE,
					NABoolean formatForDisplay = FALSE) const;
  const NAString  getColRefAsString(NABoolean debug = FALSE,
				    NABoolean formatForDisplay = FALSE) const;
  static
  const NAString  getColRefAsString(const NAString& col,
				    const NAString& corr)
  {
    if (corr == (const char *)"") return col;
    else
    {
       NAString returnVal(corr.length() +1 + col.length());
       returnVal.append(corr);
       returnVal.append(".");
       returnVal.append(col);
       return returnVal;
    }
  }

  NABoolean isDelimited() const
		      { return corrName_.getQualifiedNameObj().isDelimited(); }

  // mutators
  void setSpecialType(ExtendedQualName::SpecialTableType stt)
                                         { corrName_.setSpecialType(stt);  }
  void setNamePosition(StringPos pos, NABoolean setD = TRUE)
				      { corrName_.setNamePosition(pos, setD); }
  void setStarWithSystemAddedCols() { isStar_ = STAR_WITH_SYSTEM_ADDED_COLS; }
  void setPartnClause(PartitionClause pClause) { corrName_.setPartnClause(pClause);  }

  // Display/print, for debugging.
  void display() const;
  void print(FILE*       ofd 	= stdout,
	     const char* indent = DEFAULT_INDENT,
             const char* title 	= "ColRefName",
	     NABoolean   brief 	= FALSE) const;

  NABoolean isQualified() const;

private:
  enum starType { NO_STAR = FALSE, IS_STAR = TRUE, STAR_WITH_SYSTEM_ADDED_COLS };

  CorrName corrName_;
  NAString colName_;
  starType isStar_;

}; // ColRefName


// -----------------------------------------------------------------------
// -- Triggers
// The TableRefName class is used to pass the REFERENCING clause of
// trigger declarations from the parser to the binder.
// Each object holds a table name (or correlation name to a table) and
// a referencing name. For example, if the trigger contains the text:
// "REFERENCING NEW AS MY_NEW", the referencing name is "MY_NEW", and
// the table name is "NEW@" (or whatever NEWCorr turnes out to be).
// -----------------------------------------------------------------------
class TableRefName : public NABasicObject
{

public:

  // constructors
  TableRefName(CollHeap *h = CmpCommon::statementHeap()) : 
	tableName_("",h), refName_("",h), tableCorr_("",h), columnList_(NULL)
	{}

  TableRefName(const NAString& tableName, const NAString& refName,
               CollHeap *h = CmpCommon::statementHeap()) :
	tableName_(tableName, h), refName_(refName, h), tableCorr_(tableName_, h),
        columnList_(NULL)
	{}

  TableRefName(const TableRefName& other, CollHeap *h = CmpCommon::statementHeap()) :
	tableName_(other.tableName_, h), refName_(other.refName_, h),
        tableCorr_(other.tableCorr_, h), columnList_(other.columnList_)
	{}

  // operators
  NABoolean operator==(const TableRefName& other) const
  {	return ((getTableName() == other.getTableName()) &&
			(getRefName()   == other.getRefName()));  }

  NABoolean operator!=(const TableRefName& other) const
  { return !(*this == other); }

  TableRefName &operator=(const TableRefName& rhs)
  { 
//	TableRefName *newMe = new(CmpCommon::statementHeap()) TableRefName(rhs);
//	this = newMe;
	  tableName_  = rhs.tableName_;
	  refName_    = rhs.refName_;
	  tableCorr_  = rhs.tableCorr_;
	  columnList_ = rhs.columnList_;
	  return *this; 
  }

  // accessors
  inline const NAString&       getTableName()  const { return tableName_; }
  inline const NAString&       getRefName()    const { return refName_;   }
  inline const CorrName&       getTableCorr()  const { return tableCorr_; }
  inline const ColumnDescList *getColumnList() const { return columnList_; }
  inline void setColumnList(ColumnDescList *columnList) 
	{ columnList_ = columnList; }

  Int32 lookupTableName(RETDesc *retDesc);
  void bindRefColumns(BindWA *bindWA) const;

private:

  NAString        tableName_;
  NAString        refName_;
  CorrName        tableCorr_;
  ColumnDescList *columnList_;
}; // TableRefName

class TableRefList : public LIST(TableRefName)
{
public:
	TableRefList(CollHeap *heap = CmpCommon::statementHeap());

	const TableRefName *findTable(const CorrName& tableCorr) const;
}; // TableRefList

// -----------------------------------------------------------------------
// MV
// This class contains the neccessary details of an MV that is using 
// a table. The NATable of a base table contains a list of these
// objects, with the details of all the MVs that are using this table.
// This information is used for logging and immediate refresh operations.
// -----------------------------------------------------------------------
class UsingMvInfo : public NABasicObject
{
public:
  UsingMvInfo(  NAString	  mvAnsiName, 
		ComMVRefreshType  refreshType, 
		NABoolean	  isRewriteEnabled,	
		NABoolean	  isInitialized,
                CollHeap         *heap) 
  : mvName_(mvAnsiName, 3, heap),
    refreshType_(refreshType),
    isRewriteEnabled_(isRewriteEnabled),
    isInitialized_(isInitialized)
  {}

  const QualifiedName&	 getMvName()        const { return mvName_; }
  ComMVRefreshType       getRefreshType()   const { return refreshType_; }
  NABoolean	         isRewriteEnabled() const { return isRewriteEnabled_; }
  NABoolean	         isInitialized()    const { return isInitialized_; }

private:
  const QualifiedName	 mvName_;
  const ComMVRefreshType refreshType_; // unknown here means "non incremental"
  const NABoolean	 isRewriteEnabled_;
  const NABoolean	 isInitialized_;
};

typedef LIST(UsingMvInfo *) UsingMvInfoList;

// -----------------------------------------------------------------------
// Inline methods for class QualifiedName
// -----------------------------------------------------------------------

inline
NABoolean QualifiedName::operator==(const QualifiedName& other) const
{
  return
    getObjectName()  == other.getObjectName()  &&
    getSchemaName()  == other.getSchemaName()  &&
    getCatalogName() == other.getCatalogName() &&
    ( getObjectNameSpace() == other.getObjectNameSpace() ||
      getObjectNameSpace() == COM_UNKNOWN_NAME           ||  // keep pre-R2.5 behavior
      COM_UNKNOWN_NAME     == other.getObjectNameSpace() );  // keep pre-R2.5 behavior
}

inline
NABoolean QualifiedName::operator!=(const QualifiedName& other) const
{
  return NOT operator==(other);
}

inline
NABoolean QualifiedName::operator!=(const NAString& simpleName) const
{
  return NOT operator==(simpleName);
}

inline
ULng32 QualifiedName::hash() const
{
  return getObjectName().hash() ^
	 getSchemaName().hash() ^
	 getCatalogName().hash();
}

// -----------------------------------------------------------------------
// Inline methods for class ExtendedQualName
// -----------------------------------------------------------------------

inline
NABoolean ExtendedQualName::operator!=(const ExtendedQualName& other) const
{
  return NOT operator==(other);
}

inline
ULng32 ExtendedQualName::hash() const
{
    return getQualifiedNameObj().hash()^
	    getPartnClause().hash();
}

inline
NABoolean ExtendedQualName::isEmpty() const
{
  ExtendedQualName empty;
  return *this == empty;
}

// -----------------------------------------------------------------------
// Inline methods for class CorrName
// -----------------------------------------------------------------------

inline
NABoolean CorrName::operator!=(const CorrName& other) const
{
  return NOT operator==(other);
}

// Comparison operators just for coding simplicity in callers
// ( Remove the cutesy == and != operators overloaded for NAString& ? )
inline
NABoolean CorrName::isEmpty() const
{
  // beware of stack variable declarations like this "CorrName empty;"
  // their implicit constructor & destructor calls can slow us down.
  static const CorrName empty;
  return *this == empty;
}

inline
NABoolean CorrName::operator==(const NAString& other) const
{
  CMPASSERT(other == (const char *)"");
  return isEmpty();
}

inline
NABoolean CorrName::operator!=(const NAString& other) const
{
  return NOT operator==(other);
}

// -----------------------------------------------------------------------
// Inline methods for class ColRefName
// -----------------------------------------------------------------------
inline
NABoolean ColRefName::isEmpty() const
{
  return getColName() == (const char *)"";
}

inline
ULng32 ColRefName::hash() const
{
  return getCorrNameObj().hash() ^
  	 getColName().hash();
}

// -----------------------------------------------------------------------
// Inline methods for class PartitionClause
// -----------------------------------------------------------------------

inline
NABoolean PartitionClause::operator==(const PartitionClause& other) const
{
  return (getPartitionName()          == other.getPartitionName() &&
	  getBeginPartitionNumber()   == other.getBeginPartitionNumber() &&
	  getEndPartitionNumber()     == other.getEndPartitionNumber() &&
	  getLocationName()	      == other.getLocationName());
}

inline
NABoolean PartitionClause::operator!=(const PartitionClause& other) const
{
  return NOT operator==(other);
}

inline
ULng32 PartitionClause::hash() const
{
  ULng32 hashVal = 
    getPartitionName().hash() ^
    getBeginPartitionNumber() ^
    getLocationName().hash();

  if (getEndPartitionNumber() > 0)
    hashVal ^= getEndPartitionNumber();

  return hashVal;
}

inline
NABoolean PartitionClause::isEmpty() const
{
  // beware of stack variable declarations like this "PartitionClause empty;"
  // their implicit constructor & destructor calls can slow us down.
  static const PartitionClause empty;
  return *this == empty;
}

// -----------------------------------------------------------------------
// External declarations
// -----------------------------------------------------------------------

// For NAKeyLookup
ULng32 hashKey(const QualifiedName&);
ULng32 hashKey(const ExtendedQualName&);
ULng32 hashKey(const CorrName&);
ULng32 hashKey(const ColRefName&);

// -----------------------------------------------------------------------
// ObjectCounter class
// -----------------------------------------------------------------------

class ObjectCounter
{
public:
  ObjectCounter(Lng32 startPoint=0)
  {
    count_ = waterMark_ = totalAllocations_ = startPoint;
  };
  inline Lng32 counter() {return count_;};
  inline Lng32 waterMark() {return waterMark_;};
  inline Lng32 totalAllocations() {return totalAllocations_;};
  inline void incrementCounter()
  {
    #ifdef OPT_ANALYSIS
    ++ count_;
    if (count_>waterMark_)
      waterMark_ = count_;
    ++ totalAllocations_;
    #endif /* OPT_ANALYSIS */
  };
  inline void decrementCounter()
  {
    #ifdef OPT_ANALYSIS
    -- count_;
    #endif /* OPT_ANALYSIS */
  };
  inline void initializeCounter() { count_ = waterMark_ = 0;};

private:
  Lng32 count_;
  Lng32 waterMark_;
  Lng32 totalAllocations_;
};


//------------------------------------------------------------
// a counter class that keeps track of counts of time tasks
// been executed and time elapsed in execution
//------------------------------------------------------------

class TaskMonitor
{
public:
  TaskMonitor() : count_(0), goodCounts_(0), timer_(0), et_(0.0)
   {
   }

  // This function was added to reset counters to initial values,
  // usually zeros, to allow monitor statistics being collected 
  // and printed several times inside the loop.
  inline void init(Lng32 initval)
  {
    count_ = goodCounts_ = initval;
    timer_ = (clock_t)initval;
    et_ = 0.0;
  } 
  inline void enter()
  {
    currTime_ = clock();
    count_++;

    struct timeval x;
    gettimeofday(&x, NULL);

    et_ = double(x.tv_sec) + double(x.tv_usec) / 1000000; 
  }

  inline void exit()
  {
    currTime_ = clock() - currTime_;

    struct timeval x;
    gettimeofday(&x, NULL);
    
    double new_et = double(x.tv_sec) + double(x.tv_usec) / 1000000; 
    et_ = new_et - et_;

    timer_ += currTime_;
    if (currTime_) goodCounts_++;
  }
  inline Lng32 count() { return count_;}
  inline Lng32 goodcount() { return goodCounts_;}
  inline clock_t timer() { return timer_;}

  // return the elapsed time in seconds 
  float elapsed_time()  { return et_; }

private:
  Lng32 count_;
  Lng32 goodCounts_;     // counts of tasks that took more than 1 ms
                        // to execute (not accurate).
  clock_t timer_;
  clock_t currTime_;

  double et_;
};


ostream& operator<< (ostream&, TaskMonitor);


// -----------------------------------------------------------------------

#endif /* OBJECTNAMES_H */
