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
#ifndef ComdbUser_H
#define ComdbUser_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComdbUser.h
 * Description:  ComdbUser represents SQL user names.
 * Created:      
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

class ComdbUser;
class ComdbUserList
class ComExternalUser;

// -----------------------------------------------------------------------
// definition of class ComdbUser
// -----------------------------------------------------------------------
class ComdbUser
{

  public:

    //
    // constructors
    //

    ComdbUser ();

        // Default constructor

    ComdbUser (const NAString &UserName);

    //
    // virtual destructor
    //
   virtual ~ComdbUser ();

    inline NAString getdbUserName() const;

    inline void setdbUserName (const NAString &dbUserName);
    //
    // other methods
    //

    inline NABoolean isEmpty () const;
    inline NABoolean isValid () const;

  protected:

  private:

    NABoolean scan (const NAString &UserName);

    NAString dbUserName_;
};

// -----------------------------------------------------------------------
// definitions of inline methods for class ComdbUser
// -----------------------------------------------------------------------

NABoolean
ComdbUser::isEmpty() const
{
    return FALSE; //Need to change
}

NABoolean
ComdbUser::isValid() const
{
  //Need to change
  return TRUE;
}

void
ComdbUser::setdbUserName (const NAString &dbUserName)
{
  dbUserName_ = dbUserName;
}

NAString
ComdbUser::getdbUserName () const
{
  return dbUserName_;
}

class ComdbUserList : publis LIST(ComdbUser *)
{
public
  ComdbUserList();
  ComdbUserList(const ComdbUserList&);
  virtual ~ComdbUserList();

  const ComBoolean exists ( const ComdbUser dbUser,
                            const CatLockMode lockMode = CAT_LOCK_READONLY );
  ComdbUser &open ( const NAString &userName );
  ComdbUser *find (const NAString &userName );
}
class ComExternalUser
{

  public:

    //
    // constructors
    //

    ComExternalUser ();

        // Default constructor

    ComExternalUser (const NAString &externalUserName);

    //
    // virtual destructor
    //
    virtual ~ComExternalUser ();

    NAString getexternalUserName() const;

    inline void setexternalUserName (const NAString &externalUserName);
    //
    // other methods
    //

    inline NABoolean isEmpty () const;
    inline NABoolean isValid () const;

  protected:

  private:

    // private data members
    //

    NAString externalUserName_;
    NABoolean scan (const NAString &UserName);

};

// -----------------------------------------------------------------------
// definitions of inline methods for class ComExternalUser
// -----------------------------------------------------------------------

NABoolean
ComExternalUser::isEmpty() const
{
  return FALSE; //require changes
}

NABoolean
ComExternalUser::isValid() const
{
  //require changes
  return TRUE;
}

void
ComExternalUser::setexternalUserName (const NAString &externalUserName)
{
  externalUserName_ = externalUserName;
}

NAString
ComExternalUser::getexternalUserName () const
{
  return externalUserName_;
}


#endif // ComExternalUser_H
