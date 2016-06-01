//*****************************************************************************
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
//*****************************************************************************
  
// ==========================================================================
// Contains non inline methods in the following classes
//   PrivMgrCoreDesc
//   PrivMgrDesc
// ==========================================================================

#include "PrivMgrDesc.h"
#include "PrivMgrPrivileges.h"
#include <bitset>
#include <string>
#include <vector>

// *****************************************************************************
//    PrivMgrCoreDesc methods
// *****************************************************************************
bool PrivMgrCoreDesc::operator==(const PrivMgrCoreDesc& other) const
{
     //  Check for pathological case of X = X.
   if ( this == &other )
      return true;

   return ( ( priv_ == other.priv_ ) &&
            ( wgo_  == other.wgo_  ) );
}

void PrivMgrCoreDesc::setPriv(const PrivType which,
                              const bool value)
{
   switch(which) {

    case ALL_PRIVS:

      priv_.set();
      break;

    case ALL_DML:

      priv_.set(SELECT_PRIV, value);
      priv_.set(INSERT_PRIV, value);
      priv_.set(UPDATE_PRIV, value);
      priv_.set(USAGE_PRIV, value);
      priv_.set(DELETE_PRIV, value);
      priv_.set(REFERENCES_PRIV, value);
      priv_.set(EXECUTE_PRIV, value);

      break;

    case ALL_DDL:

      priv_.set(CREATE_PRIV, value);
      priv_.set(ALTER_PRIV, value);
      priv_.set(DROP_PRIV, value);
      break;

    default:
      priv_.set(which, value);
      break;
  }
}

void PrivMgrCoreDesc::setWgo(const PrivType which,
                          const bool value)
{
   switch(which) {

    case ALL_PRIVS:

      wgo_.set();
      break;

    case ALL_DML:

      wgo_.set(SELECT_PRIV, value);
      wgo_.set(INSERT_PRIV, value);
      wgo_.set(UPDATE_PRIV, value);
      wgo_.set(USAGE_PRIV, value);
      wgo_.set(DELETE_PRIV, value);
      wgo_.set(REFERENCES_PRIV, value);
      wgo_.set(EXECUTE_PRIV, value);

      break;

    case ALL_DDL:

      wgo_.set(CREATE_PRIV, value);
      wgo_.set(ALTER_PRIV, value);
      wgo_.set(DROP_PRIV, value);
      break;

    default:
      wgo_.set(which, value);
      break;
  }
}

// Set all privilege indicators for Grant to Table
// (Sel/Ins/Upd/Del/Ref only) (Sets these "priv" to True,
//  with wgo indicators set as specified).
//   If updatable=False, suppress Insert,Delete,Update.
//   If insertable=False, suppress Insert.
void PrivMgrCoreDesc::setAllDMLGrantPrivileges(const bool wgo,
                                        const bool updatable,
                                        const bool insertable,
                                        const bool deletable)
{
  this->setPriv(SELECT_PRIV, true);
  this->setWgo(SELECT_PRIV, wgo);

  if (updatable)
   {
      this->setPriv(UPDATE_PRIV, true);
      this->setWgo(UPDATE_PRIV, wgo);
      this->setPriv(REFERENCES_PRIV, true);
      this->setWgo(REFERENCES_PRIV, wgo);

      if ( insertable)
      {
         this->setPriv(INSERT_PRIV, true);
         this->setWgo(INSERT_PRIV, wgo);
      }
      if ( deletable )
      {
         this->setPriv(DELETE_PRIV, true);
         this->setWgo(DELETE_PRIV, wgo);
      }
   }
}

void PrivMgrCoreDesc::setAllDDLGrantPrivileges(const bool wgo)

{
  this->setPriv(ALL_DDL, true);
  this->setWgo (ALL_DDL, wgo);

}

/*
 * The following setAllRevoke.. functions are used to set a mask in revoking privileges.
 *
 * When grantOptionFor is specified, we set the wgo bits because we want to use the mask
 * in revoking the with grant option for those privileges.
 *
 * Otherwise, we set the priv bits because we want to use the mask to revoke those 
 * privileges.  
 *
*/

void PrivMgrCoreDesc::setAllDMLRevokePrivileges(const bool grantOption)
{
  if (grantOption)
  {
    //set all dml privs in wgo to true
    //set all dml privs in priv to false
    this->setPriv(ALL_DML, false);
    this->setWgo(ALL_DML, true);
  }
  else
  {
    //set all dml privs in wgo to false
    //set all dml privs in priv to true
    this->setPriv(ALL_DML, true);
    this->setWgo(ALL_DML, false);
  }
 }

void PrivMgrCoreDesc::setAllDDLRevokePrivileges(const bool grantOption)
{
  if (grantOption) 
  {
    //set all ddl privs in wgo to true
    //set all ddl privs in priv to false
    this->setPriv(ALL_DDL, false);
    this->setWgo(ALL_DDL, true);
  }
  else 
  {
    //set all ddl privs in wgo to false
    //set all ddl privs in priv to true
    this->setPriv(ALL_DDL, true);
    this->setWgo(ALL_DDL, false);
  }
}

// Set all privilege indicators for Revoke.
void PrivMgrCoreDesc::setAllRevokePrivileges(const bool grantOption)
{
  if (grantOption)
  {
     priv_.reset();   // For "Revoke Grant Option for.."
     wgo_.set();     //   get priv=F, wgo=T.
  }
  else
  {
     priv_.set();    // For "Revoke ..."
     wgo_.reset();    //   get priv=T, wgo=F.
  }
}

// ----------------------------------------------------------------------------
// method: setAllObjectGrantPrivilege
//
// This method sets all the relevant DML privileges for the specified 
// object type.  Optionally all corresponding WITH GRANT OPTIONS privileges
// are also set.
//
// Params:
//     objectType - The type of object.  Based on the object type (e.g. table,
//                  routine, sequence, etc.) all the relevant privs are set
//     wgo - WITH GRANT OPTION.  If true, the corresponding WGO bits are set.
//
// ---------------------------------------------------------------------------- 
void PrivMgrCoreDesc::setAllObjectGrantPrivilege(
   const ComObjectType objectType,
   const bool wgo)

{

   switch (objectType)
   {
      case COM_BASE_TABLE_OBJECT:
         setAllTableGrantPrivileges(wgo);
         break;
      case COM_LIBRARY_OBJECT:
         setAllLibraryGrantPrivileges(wgo);
         break;
      case COM_SEQUENCE_GENERATOR_OBJECT:
         setAllSequenceGrantPrivileges(wgo);
         break;
      case COM_USER_DEFINED_ROUTINE_OBJECT:
      case COM_STORED_PROCEDURE_OBJECT:
         setAllUdrGrantPrivileges(wgo);
         break;
      default:
         ; //TODO: internal error?
   }   

}



// Set all privilege and wgo indicators to the specified value.
void  PrivMgrCoreDesc::setAllPrivAndWgo(const bool val)
{
  if (val)
  {
    priv_.set();
    wgo_.set();
  }
  else
  {
    priv_.reset();
    wgo_.reset();
  }
}


// AndNot -- sets "this" to "this" AND NOT "other"
void PrivMgrCoreDesc::AndNot( const PrivMgrCoreDesc& other )
{
  PrivMgrCoreDesc temp(other);
  temp.complementPrivs();
  this->intersectionOfPrivs(temp);
}

// -------------------------------------------------------------------
// grantPrivs -- modify the privilege settings of target PrivMgrCorePrivs
//    by ORing values of this one.  Change values of 'this'
//    to carry only the privileges actually granted.
//    Return a value to indicate how many privs actually granted.
// This function does three things:
//   It modifies the privilege settings of the passed PrivMgrCorePrivs parameter,
//     by applying the current object's settings as a grant against it;
//   It modifies the current object's settings to indicate only those
//     which were actually granted to the passed PrivMgrCorePrivs param;
//   It returns a value indicating how many (all/some/none) of the settings
//     which were to be granted actually had an effect.  If no privs were
//     to be granted, return a "neutral" value.

//     this object|passed param|passed param|this object
//      on input: |  on input: | on output: | on output: 
//     priv_ wgo_ | priv_ wgo_ | priv_ wgo_ | priv_ wgo_
//      ---  ---  |  ---  ---  |  ---  ---  |  ---  ---  
//       T    T  ->   T    T  =>   T    T       F    F    
//                    T    F       T    T       F    T    
//                    F    F       T    T       T    T    
//       T    F  ->   T    T  =>   T    T       F    F      
//                    T    F       T    F       F    F     
//                    F    F       T    F       T    F     
//       F    F  ->   x    y  =>   x    y       F    F    
//       F    T (does not occur -- cannot grant wgo without granting priv)

// The algorithm is:
//   Output param = input param OR input this;
//   Output this = input this AND NOT input param.

// The return value is computed as follows:
//   If no privs were specified in this on input, return Neutral.
//   Otherwise :
//     Return All if each T specified in input 'this'
//       remains T in output 'this';
//     Return None if each T specified in input 'this'
//       becomes F on output 'this';
//     Return Some for other combinations.
// ----------------------------------------------------------------------------
PrivMgrCoreDesc::PrivResult PrivMgrCoreDesc::grantPrivs(PrivMgrCoreDesc& param)
{
   if ( this->isNull() )
      return NEUTRAL;   // Do nothing.  No privs to be granted.

   std::bitset<NBR_OF_PRIVS> beforePriv (param.priv_);  // save input values
   std::bitset<NBR_OF_PRIVS> beforeWgo  (param.wgo_);
   std::bitset<NBR_OF_PRIVS> inPriv (priv_);
   std::bitset<NBR_OF_PRIVS> inWgo  (wgo_);

   param.priv_ |= priv_;    // param(out) gets param(in) OR this(in).
   param.wgo_  |= wgo_;

   priv_ &= ( beforePriv.flip() );  // this(out) gets this(in)
   wgo_  &= ( beforeWgo.flip() );   //     AND NOT  param(in)

   PrivResult result(NEUTRAL);

   // Look at each privilege type
   for ( size_t i = 0; i < NBR_OF_PRIVS; i++ )
   {
      PrivType pType = PrivType(i);
      PrivMgrCoreDesc::interpretChanges(inPriv.test(pType),
                                     priv_.test(pType),
                                     result);
      PrivMgrCoreDesc::interpretChanges(inWgo.test(pType),
                                     wgo_.test(pType),
                                     result);
   }

   return result;
}

// -------------------------------------------------------------------
// revoke -- modify the privilege settings of target PrivMgrCoreDesc
//    by ANDing the NOT of values of this one. 
// Change values of 'this' to carry only the privileges 
//    which were actually revoked.
// This function does three things:
//   It modifies the privilege settings of the passed PrivMgrCoreDesc parameter,
//     by applying the current object's settings as a revoke against it;
//   It modifies the current object's settings to indicate only those
//     which were actually revoked from the passed PrivMgrCoreDesc param;
//   It returns a value indicating how many (all/some/none) of the settings
//     which were to be revoked actually had an effect.

//     this object|passed param|passed param|this object
//      on input: |  on input: | on output: | on output:
//     priv_ wgo_ | priv_ wgo_ | priv_ wgo_ | priv_ wgo_
//      ---  ---  |  ---  ---  |  ---  ---  |  ---  --- 
//       T    T  ->   T    T  =>   F    F       T    T 
//                    T    F       F    F       T    F 
//                    F    F       F    F       F    F 
//       F    T  ->   T    T  =>   T    F       F    T
//                    T    F       T    F       F    F
//                    F    F       F    F       F    F  
//       F    F  ->   x    y  =>   x    y       F    F 
//       T    F (does not occur -- cannot revoke priv without revoking wgo)
// 
// The algorithm is:
//   Output param = input param AND NOT input this;
//   Output this = input this AND input param.
//   Return value is computed as described in grantPrivs.
// ---------------------------------------------------------------------------- 
PrivMgrCoreDesc::PrivResult PrivMgrCoreDesc::revokePrivs(PrivMgrCoreDesc& param)
{
   if ( this->isNull() )
      return NEUTRAL;   // Do nothing.  No privs to be revoked.

   std::bitset<NBR_OF_PRIVS> beforePriv  = param.priv_;  // save input values.
   std::bitset<NBR_OF_PRIVS> beforeWgo =  param.wgo_;
   std::bitset<NBR_OF_PRIVS> inPriv (priv_);
   std::bitset<NBR_OF_PRIVS> inWgo  (wgo_);

   param.priv_ &= ( priv_.flip() );   // param(out) gets param(in) 
   //param.priv_ &= ( priv_ );   // param(out) gets param(in) 
   param.wgo_  &= ( wgo_.flip());     //   AND NOT( this(in) ).
   //param.wgo_  &= ( wgo_);     //   AND NOT( this(in) ).
   param.wgo_  &= param.priv_;  // Assure valid new setting.

   priv_ &= beforePriv;       // this(out) gets this(in)
   wgo_  &= beforeWgo;        //     AND param(in).

   PrivResult result(NEUTRAL);
   // Look at each privilege type
   for ( size_t i = 0; i < NBR_OF_PRIVS; i++ )
   {
      PrivType pType = PrivType(i);
      PrivMgrCoreDesc::interpretChanges(inPriv.test(pType),
                                     priv_.test(pType),
                                     result);
      PrivMgrCoreDesc::interpretChanges(inWgo.test(pType),
                                     wgo_.test(pType),
                                     result);
   }

   return result;
}

// limitToGrantable --remove from this descriptor any privileges 
//   (both priv and WGO) which are not held WGO in other.
//   Return True if any privileges were removed; else False. 
bool PrivMgrCoreDesc::limitToGrantable( const PrivMgrCoreDesc& other )
{
   std::bitset<NBR_OF_PRIVS> beforePriv (this->priv_);  // save the original
   std::bitset<NBR_OF_PRIVS> beforeWgo  (this->wgo_);
   std::bitset<NBR_OF_PRIVS>  allTrue;
   allTrue.set();

   priv_ &= other.wgo_;    // priv gets priv AND other wgo
   wgo_  &= other.wgo_;    // wgo  get  wgo  AND other wgo
   beforePriv ^= priv_;     // perform exclusive OR
   beforeWgo  ^= wgo_;      //   to see if anything was lost.

   // return false iff the exOR gave false in all positions.
   return ! ( beforePriv.none() &&
              beforeWgo.none() );
}

// suppressDuplicatedPrivs --remove from this descriptor any priv/wgo
//   settings which also appear in other.  (this = this AND NOT other)
//   Return True if any privileges were removed; else False. 
bool PrivMgrCoreDesc::suppressDuplicatedPrivs( const PrivMgrCoreDesc& other )
{
   std::bitset<NBR_OF_PRIVS> beforePriv (this->priv_);  // save the original
   std::bitset<NBR_OF_PRIVS> beforeWgo  (this->wgo_);
   std::bitset<NBR_OF_PRIVS> otherPriv (other.priv_);   // modifiable copies
   std::bitset<NBR_OF_PRIVS> otherWgo  (other.wgo_);

   priv_ &= ( otherPriv.flip() );    // priv gets priv AND NOT other priv
   wgo_  &= ( otherWgo.flip() );     // wgo  gets wgo  AND NOT other wgo

   beforePriv ^= priv_;     // perform exclusive OR
   beforeWgo  ^= wgo_;      //   to see if anything was lost.

     // return false iff the exOR gave false in all positions.
   return ! ( beforePriv.none() &&
              beforeWgo.none() );
}

// Return true iff any flag set in "this" is not set in "other".
// (Evaluate whether "this" && !"other" is null.)
bool PrivMgrCoreDesc::anyNotSet( const PrivMgrCoreDesc& other ) const
{
   std::bitset<NBR_OF_PRIVS> priv(this->priv_);   // modifiable copies
   std::bitset<NBR_OF_PRIVS> wgo(this->wgo_);
   std::bitset<NBR_OF_PRIVS> otherPriv (other.priv_);
   std::bitset<NBR_OF_PRIVS> otherWgo  (other.wgo_);

   priv &= ( otherPriv.flip());    // priv gets priv AND NOT other priv
   wgo  &= ( otherWgo.flip() );     // wgo  gets wgo  AND NOT other wgo

   if ( priv.none() &&
        wgo.none() )
      return false;    // All were set.
   else
      return true;     // Something was not set.
}

// If any priv/wgo flag is set in "this" and is not set in "other",
// unset the value in "this".
// Return true if any change was made.
bool PrivMgrCoreDesc::cascadeAnyNotSet( const PrivMgrCoreDesc& other )
{
   std::bitset<NBR_OF_PRIVS> priv(this->priv_);
   std::bitset<NBR_OF_PRIVS> wgo(this->wgo_);
   std::bitset<NBR_OF_PRIVS> otherPriv (other.priv_);
   std::bitset<NBR_OF_PRIVS> otherWgo  (other.wgo_);
   bool anyCleared = false;

   priv &= otherPriv;    // priv gets priv AND other priv
   wgo  &= otherWgo;     // wgo  gets wgo  AND other wgo

   if ( ( this->priv_ == priv) &&
        ( this->wgo_  == wgo ) )
      anyCleared = false;    // Nothing changed.
   else
      anyCleared = true;     // Some privilege was reduced.

   this->priv_ = priv;  // Set the new values into "this".
   this->wgo_  = wgo;

   return anyCleared;
}

// Private helper function to interpret changes to specified
//   grant/revoke privileges.  
void PrivMgrCoreDesc::interpretChanges( const bool before,       // in
                                     const bool after,  // in
                                     PrivResult& result )     // in/out
{
   if ( before )    // In value T
   {
      if ( after )  // Out value T too
      {
         if ( ( result == NEUTRAL ) || ( result == ALL ) )
            result = ALL;
         else
            result = SOME;
      }
      else         // In value T, Out value F
      {
         if ( ( result == NEUTRAL ) || ( result == NONE ) )
            result = NONE;
         else
            result = SOME;
      }
   }
    // No change to result, if in value F.
}


// *****************************************************************************
//    PrivMgrDesc methods
// *****************************************************************************

// ----------------------------------------------------------------------------
// method:  limitToGrantable
//
// Remove from this descriptor any privileges 
// (both priv and WGO) which are not held WGO in other.
// Return True if any privileges were removed; else False. 
// ----------------------------------------------------------------------------
bool PrivMgrDesc::limitToGrantable( const PrivMgrDesc& other )
{

  bool result = false;
  PrivMgrCoreDesc logicalTablePrivs(other.tableLevel_);
  // TDB - include column level privileges

 if ( tableLevel_.limitToGrantable(logicalTablePrivs) )
    result = TRUE;

   return result;
}

