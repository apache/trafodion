#ifndef STMTDDLREGISTERCOMPONENT_H
#define STMTDDLREGISTERCOMPONENT_H
/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
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
 *****************************************************************************
 *
 * File:         StmtDDLRegisterComponent.h
 * Description:  class for parse nodes representing register and unregister
 *                 component statements
 *
 * Created:      June 22, 2011     
 * Language:     C++
 *
 *****************************************************************************
 */

#include "ComSmallDefs.h"
#include "StmtDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLRegisterComponent;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Register and unregister component statements
// -----------------------------------------------------------------------
class StmtDDLRegisterComponent : public StmtDDLNode
{

public:

  enum RegisterComponentType { REGISTER_COMPONENT, UNREGISTER_COMPONENT };

  // constructors of (un)register component
  StmtDDLRegisterComponent(RegisterComponentType eRegComponentParseNodeType,
                           const NAString & componentName,
                           const NABoolean isSystem,
                           const NAString & latin1DetailInfo,
                           CollHeap * heap = PARSERHEAP());
  StmtDDLRegisterComponent(RegisterComponentType eRegComponentParseNodeType,
                           const NAString & componentName,
                           ComDropBehavior dropBehavior, 
                           CollHeap * heap = PARSERHEAP());

  // virtual destructor
  virtual ~StmtDDLRegisterComponent();

  // cast
  virtual StmtDDLRegisterComponent * castToStmtDDLRegisterComponent();

  // accessors

  inline const NAString & getExternalComponentName() const;
  inline const RegisterComponentType getRegisterComponentType() const;
  inline const NAString & getRegisterComponentDetailInfo() const;
  inline const NABoolean isSystem() const;
  inline const ComDropBehavior getDropBehavior() const;

  // for tracing

  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual const NAString getText() const;
  
private:

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  RegisterComponentType registerComponentType_;
  NAString componentName_;
  NAString componentDetailInfo_;
  NABoolean isSystem_;
  ComDropBehavior dropBehavior_; 
 
}; // class StmtDDLRegisterComponent

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLRegisterComponent
// -----------------------------------------------------------------------

//
// accessors
//

inline const NAString &
StmtDDLRegisterComponent::getExternalComponentName() const
{
  return componentName_;
}

inline const StmtDDLRegisterComponent::RegisterComponentType 
StmtDDLRegisterComponent::getRegisterComponentType() const
{
  return registerComponentType_;
}

inline const NAString &
StmtDDLRegisterComponent::getRegisterComponentDetailInfo() const
{
  return componentDetailInfo_;
}

inline const NABoolean
StmtDDLRegisterComponent::isSystem() const
{
  return isSystem_;
}
inline const ComDropBehavior
StmtDDLRegisterComponent::getDropBehavior() const
{
  return dropBehavior_;
}

#endif // STMTDDLREGISTERCOMPONENT_H
