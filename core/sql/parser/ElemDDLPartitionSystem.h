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
#ifndef ELEMDDLPARTITIONSYSTEM_H
#define ELEMDDLPARTITIONSYSTEM_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLPartitionSystem.h
 * Description:  class representing a system partition element
 *               specified in a DDL statement
 *
 *
 * Created:      4/6/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "NAString.h"
#include "ElemDDLNode.h"
#include "ElemDDLFileAttrMaxSize.h"
#include "ElemDDLFileAttrExtents.h"
#include "ElemDDLFileAttrMaxExtents.h"
#include "ElemDDLLocation.h"
#include "ElemDDLPartition.h"
#include "ItemConstValueArray.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLPartitionSystem;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class BindWA;

// -----------------------------------------------------------------------
// definition of class ElemDDLPartitionSystem
// -----------------------------------------------------------------------
class ElemDDLPartitionSystem : public ElemDDLPartition
{

public:

  // constructors
  ElemDDLPartitionSystem();
  ElemDDLPartitionSystem(ElemDDLPartition::optionEnum,
                         ElemDDLNode * pLocation,
                         ElemDDLNode * pPartitionSystemAttrList);
  ElemDDLPartitionSystem(OperatorTypeEnum operType,
                         ElemDDLPartition::optionEnum,
                         ElemDDLNode * pLocation,
                         ElemDDLNode * pPartitionSystemAttrList);

  // virtual destructor
  virtual ~ElemDDLPartitionSystem();

  // cast
  virtual ElemDDLPartitionSystem * castToElemDDLPartitionSystem();

  //
  // accessors
  //

  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline unsigned short getDSlackPercentage() const;
  inline unsigned short getISlackPercentage() const;

  inline const NAString & getGuardianLocation() const;

        // returns an empty string unless the parse node
        // is bound (the method bindNode is invoked).
        // After the parse node is bound, returns the
        // location of the primary partition in Guardian
        // physical device name format.  If the LOCATION
        // clause is not specified, a default location is
        // used.

  inline const NAString & getLocation() const;

        // returns the location name specified in the LOCATION
        // clause/phrase associating with the primary or
        // secondary partition; returns an empty string if the
        // LOCATION clause associating with the primary partition
        // does not appear.
  
  inline const NAString & getLocationName() const;

        // same as getLocation()

  inline const NAString & getPartitionName() const;

        // returns the partition name specified in the LOCATION clause.
        // returns an empty string if the LOCATION clause is not specified.

  inline ElemDDLLocation::locationNameTypeEnum getLocationNameType() const;

        // returns the type of the location name (e.g., an OSS
        // path name, a Guardian device name, an OSS environment
        // variable name, etc.)  If LOCATION clause does not
        // appear, the returned value has no meaning.

  inline ElemDDLNode * getLocationNode() const;

        // returns the pointer pointing to the parse node
        // representing the specified LOCATION clause. If the
        // LOCATION clause does not appear, returns NULL.

  inline ULng32 getMaxSize() const;
  inline ComUnits      getMaxSizeUnit() const;
  inline ULng32 getPriExt() const;
  inline ULng32 getSecExt() const;
  inline ULng32 getMaxExt() const;

  inline ElemDDLPartition::optionEnum getOption() const;

        // returns either ElemDDLPartition::ADD_OPTION or
        // ElemDDLPartition::DROP_OPTION depending on whether
        // ADD or DROP is specified.

  NAString getOptionAsNAString() const;

        // Same as getOption() except that this method returns
        // NAString("ADD") or NAString("DROP").

  inline NABoolean isDSlackSpecified() const;
  inline NABoolean isISlackSpecified() const;
  inline NABoolean isMaxSizeSpecified() const;
  inline NABoolean isMaxSizeUnbounded() const;
  inline NABoolean isExtentSpecified() const;
  inline NABoolean isMaxExtentSpecified() const;

  //
  // mutators
  //

  virtual void setChild(Lng32 index, ExprNode * pChildNode);
  inline void setDSlackPercentage(unsigned short dSlackPercentage);
  inline void setISlackPercentage(unsigned short iSlackPercentage);
  inline void setIsDSlackSpecified(NABoolean isDSlackSpecified);
  inline void setIsISlackSpecified(NABoolean isISlackSpecified);
  inline void setIsMaxSizeSpecified(NABoolean isMaxSizeSpecified);
  inline void setIsMaxSizeUnbounded(NABoolean isMaxSizeUnbounded);
  inline void setIsExtentSpecified(NABoolean isExtentSpecified);
  inline void setIsMaxExtentSpecified(NABoolean isMaxExtentSpecified);
  inline void setLocationName(const NAString &locationName);
  inline void setPartitionName(const NAString &partitionName);
  inline void setLocationNameType(ElemDDLLocation::locationNameTypeEnum
                                  locationNameType);
  inline void setMaxSize(ULng32 maxSize);
  inline void setMaxSizeUnit(ComUnits maxSizeUnit);
  inline void setPriExt(ULng32 priExt);
  inline void setSecExt(ULng32 secExt);
  inline void setMaxExt(ULng32 maxExt);

  //
  // method for binding
  //

  virtual ExprNode * bindNode(BindWA * pBindWA);

  //
  // methods for tracing
  //

  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual const NAString displayLabel3() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

  // method for building text
  virtual NAString getSyntax() const;


protected:

  //
  // pointer to child parse nodes
  //

  enum { INDEX_LOCATION = 0,
         INDEX_PARTITION_ATTR_LIST,
         MAX_ELEM_DDL_PARTITION_SYSTEM_ARITY };

  ElemDDLNode * children_[MAX_ELEM_DDL_PARTITION_SYSTEM_ARITY];


private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  // accessor
  inline ElemDDLNode * getPartitionSystemAttrList() const;

  // mutators
  void initializeDataMembers();
  void setPartitionAttr(ElemDDLNode * pOptionNode);

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  ElemDDLPartition::optionEnum option_;

  //
  // location
  //

  NAString locationName_;
  ElemDDLLocation::locationNameTypeEnum locationNameType_;

  // guardianLocation_ is empty until the parse node is
  // bound (the method bindNode() is invoked).  The method
  // bindNode() converts the specified location to a
  // Guardian physical device and then saves the computed
  // name in this data member.  If the LOCATION clause does
  // not appear, a default location name is selected.
  NAString guardianLocation_;

  NAString partitionName_;

  //
  // file attributes
  //

  // The flags is...Spec_ shows whether the corresponding partitionSystem
  // attributes were specified in the PARTITIONSYSTEM clause or not.  They
  // are used by the parser to look for duplicate clauses.

  // MAXSIZE
  NABoolean isMaxSizeSpec_;
  NABoolean isMaxSizeUnbounded_;
  ULng32 maxSize_;
  ComUnits maxSizeUnit_;

  // EXTENT
  NABoolean isExtentSpec_;
  ULng32 priExt_;
  ULng32 secExt_;

  // MAXEXTENT
  NABoolean isMaxExtentSpec_;
  ULng32 maxExt_;

  //
  // load options
  //

  // DSLACK
  NABoolean isDSlackSpec_;
  unsigned short dSlackPercentage_;     // percentage of free data blocks

  // ISLACK
  NABoolean isISlackSpec_;
  unsigned short iSlackPercentage_;     // percentage of free index blocks

}; // class ElemDDLPartitionSystem

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLPartitionSystem
// -----------------------------------------------------------------------

//
// accessors
//

inline unsigned short
ElemDDLPartitionSystem::getDSlackPercentage() const
{
  return dSlackPercentage_;
}

inline unsigned short
ElemDDLPartitionSystem::getISlackPercentage() const
{
  return iSlackPercentage_;
}

inline const NAString &
ElemDDLPartitionSystem::getGuardianLocation() const
{
  return guardianLocation_;
}

inline const NAString &
ElemDDLPartitionSystem::getLocation() const
{
  return locationName_;
}

// same as getLocation()
inline const NAString &
ElemDDLPartitionSystem::getLocationName() const
{
  return locationName_;
}

inline const NAString &
ElemDDLPartitionSystem::getPartitionName() const
{
  return partitionName_;
}

inline ElemDDLLocation::locationNameTypeEnum
ElemDDLPartitionSystem::getLocationNameType() const
{
  return locationNameType_;
}

inline ElemDDLNode *
ElemDDLPartitionSystem::getLocationNode() const
{
  return children_[INDEX_LOCATION];
}

inline ULng32
ElemDDLPartitionSystem::getMaxSize() const
{
  return maxSize_;
}

inline ComUnits
ElemDDLPartitionSystem::getMaxSizeUnit() const
{
  return maxSizeUnit_;
}

inline ULng32
ElemDDLPartitionSystem::getPriExt() const
{
  return priExt_;
}

inline ULng32
ElemDDLPartitionSystem::getSecExt() const
{
  return secExt_;
}

inline ULng32
ElemDDLPartitionSystem::getMaxExt() const
{
  return maxExt_;
}

// is ADD or DELETE option?
inline ElemDDLPartition::optionEnum
ElemDDLPartitionSystem::getOption() const
{
  return option_;
}

inline ElemDDLNode *
ElemDDLPartitionSystem::getPartitionSystemAttrList() const
{
  return children_[INDEX_PARTITION_ATTR_LIST];
}

inline NABoolean
ElemDDLPartitionSystem::isDSlackSpecified() const
{
  return isDSlackSpec_;
}

inline NABoolean
ElemDDLPartitionSystem::isISlackSpecified() const
{
  return isISlackSpec_;
}

inline NABoolean
ElemDDLPartitionSystem::isMaxSizeSpecified() const
{
  return isMaxSizeSpec_;
}

inline NABoolean
ElemDDLPartitionSystem::isExtentSpecified() const
{
  return isExtentSpec_;
}

inline NABoolean
ElemDDLPartitionSystem::isMaxExtentSpecified() const
{
  return isMaxExtentSpec_;
}

inline NABoolean
ElemDDLPartitionSystem::isMaxSizeUnbounded() const
{
  return isMaxSizeUnbounded_;
}

//
// mutators
//

inline void 
ElemDDLPartitionSystem::setDSlackPercentage(unsigned short dSlackPercentage)
{
  dSlackPercentage_ = dSlackPercentage;
}

inline void 
ElemDDLPartitionSystem::setISlackPercentage(unsigned short iSlackPercentage)
{
  iSlackPercentage_ = iSlackPercentage;
}

inline void
ElemDDLPartitionSystem::setIsDSlackSpecified(NABoolean isDSlackSpecified)
{
  isDSlackSpec_ = isDSlackSpecified;
}

inline void
ElemDDLPartitionSystem::setIsISlackSpecified(NABoolean isISlackSpecified)
{
  isISlackSpec_ = isISlackSpecified;
}

inline void
ElemDDLPartitionSystem::setIsMaxSizeSpecified(NABoolean isMaxSizeSpecified)
{
  isMaxSizeSpec_ = isMaxSizeSpecified;
}

inline void
ElemDDLPartitionSystem::setIsMaxSizeUnbounded(NABoolean isMaxSizeUnbounded)
{
  isMaxSizeUnbounded_ = isMaxSizeUnbounded;
}

inline void
ElemDDLPartitionSystem::setIsExtentSpecified(NABoolean isExtentSpecified)
{
  isExtentSpec_ = isExtentSpecified;
}

inline void
ElemDDLPartitionSystem::setIsMaxExtentSpecified(NABoolean isMaxExtentSpecified)
{
  isMaxExtentSpec_ = isMaxExtentSpecified;
}

inline void
ElemDDLPartitionSystem::setLocationName(const NAString & locationName)
{
  locationName_ = locationName;
}
  
inline void
ElemDDLPartitionSystem::setPartitionName(const NAString & partitionName)
{
  partitionName_ = partitionName;
}

inline void
ElemDDLPartitionSystem::setLocationNameType(
     ElemDDLLocation::locationNameTypeEnum locationNameType)
{
  locationNameType_ = locationNameType;
}
  
inline void
ElemDDLPartitionSystem::setMaxSize(ULng32 maxSize)
{
  maxSize_ = maxSize;
}

inline void
ElemDDLPartitionSystem::setMaxSizeUnit(ComUnits maxSizeUnit)
{
  maxSizeUnit_ = maxSizeUnit;
}

inline void
ElemDDLPartitionSystem::setPriExt(ULng32 priExt)
{
  priExt_ = priExt;
}

inline void
ElemDDLPartitionSystem::setSecExt(ULng32 secExt)
{
  secExt_ = secExt;
}

inline void
ElemDDLPartitionSystem::setMaxExt(ULng32 maxExt)
{
  maxExt_ = maxExt;
}

#endif // ELEMDDLPARTITIONSYSTEM_H
