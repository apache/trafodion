/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1999-2015 Hewlett-Packard Development Company, L.P.
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
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//  Define a symbol to prevent multiple inclusions of this header file.
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
#ifndef Pseudo_H
#define Pseudo_H
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h> 

#if !defined(__TANDEM) || (__CPLUSPLUS_VERSION >= 3)
using namespace std;
#endif

//====================================================
//  Base Class for all Pseudo Random Value Generators.
//====================================================
class PseudoBase {

  private:

    long seed_;
    long limit_;

  protected:
   
    // Constructor.
   PseudoBase(long pSeed,long pLimit)
     : seed_(pSeed),
       limit_(pLimit)
   {
     if (limit_ > 15000) 
     {
       cout << "maximum limit is 15,000" << endl;
       exit(5);
     }
   }

   long rand15();
    
  public:

  //-------------
  //  Destructor.
  //-------------
  virtual ~PseudoBase () {};

}; //class PseudoBase

//<pb>
//========================================
//  Pseudo Random Integers Abstract Class.
//========================================
class PseudoIntAbs : public PseudoBase {

  private:
    
  protected:

  // Constructors
  PseudoIntAbs (long pSeed, long pRange)
    : PseudoBase(pSeed, pRange)
  {}

  PseudoIntAbs ()
    : PseudoBase(0, 0)
  {}

  public:

  //--------------------------
  //  Public member functions.
  //--------------------------
  virtual long getNextPseudo() = 0;

}; //class PseudoIntAbs

//<pb>
//=========================
//  Pseudo Random Integers.
//=========================
class PseudoInt : public PseudoIntAbs {

  private:
    long modulus_;
    
  public:

  //=========================================================================
  //  Constructor for returning pseudo random integers.
  //
  // Input:
  //  pSeed    -- seed for priming pseudo random generator
  //  pRange   -- range of random values from 0 through (pRange - 1) 
  //              inclusive to generate
  //  pModulus -- range of random values from 0 through (pModulus - 1) 
  //              inclusive to actually return 
  //
  // Output:
  //  none
  //
  // Return:
  //  none
  //
  // Exceptions:
  //  none
  //
  //=========================================================================
  PseudoInt (long pSeed, long pRange, long pModulus)
    : PseudoIntAbs(pSeed, pRange),
      modulus_(pModulus) 
  {} 

  //-------------
  //  Destructor.
  //-------------
  virtual ~PseudoInt () {};

  //--------------------------
  //  Public member functions.
  //--------------------------
  virtual long getNextPseudo();

}; //class PseudoInt

//<pb>
//======================================
//  Pseudo Random Integers in Key Order.
//======================================
class PseudoIntKey : public PseudoIntAbs {

  private:
    
    // Range of numbers is 0 to (range_ - 1) inclusive.  The constructor
    // sets range_, after which it never changes.
    long range_;

    // Next value to return.
    long nextValue_;

  public:

  //--------------
  //  Constructor.
  //--------------
  PseudoIntKey (long pRange)
  : PseudoIntAbs(),
    range_(pRange),
    nextValue_(0)
  {}

  //-------------
  //  Destructor.
  //-------------
  virtual ~PseudoIntKey () {};

  //--------------------------
  //  Public member functions.
  //--------------------------
  virtual long getNextPseudo();

}; // class PseudoIntKey

//<pb>
//===========================================================
//  Pseudo Random Alphabetic Character String Abstract Class.
//===========================================================
class PseudoCharAbs : public PseudoBase {

  private:
    
  protected:

  // Constructors
  PseudoCharAbs (long pSeed, long pRange)
    : PseudoBase(pSeed, pRange)
  {}

  PseudoCharAbs ()
    : PseudoBase(0, 0)
  {}

  public:

  //--------------------------
  //  Public member functions.
  //--------------------------
  virtual void getNextPseudo(char* pCharArray) = 0;

}; //class PseudoCharAbs

//<pb>
//============================================
//  Pseudo Random Alphabetic Character String.
//============================================
class PseudoChar : public PseudoCharAbs {

  private:
    long col0Modulus_;
    long col1Modulus_;
    
  public:

  //=========================================================================
  //  Constructor for returning pseudo random, null terminated char arrays.
  //
  // Input:
  //  pSeed        -- seed for priming pseudo random generator
  //  pRange       -- range of random values to generate
  //  pCol1Modulus -- range of random characters actually returned in first
  //                    column of array 
  //  pCol2Modulus -- range of random characters actually returned in second 
  //                    column of array
  //
  // Output:
  //  none
  //
  // Return:
  //  none
  //
  // Exceptions:
  //  none
  //
  //=========================================================================
  PseudoChar (long pSeed, 
              long pRange, 
              long pCol0Modulus,
              long pCol1Modulus) 
    : PseudoCharAbs(pSeed,pRange),
      col0Modulus_(pCol0Modulus),
      col1Modulus_(pCol1Modulus)
  {}

  //-------------
  //  Destructor.
  //-------------
  virtual ~PseudoChar () {};

  //--------------------------
  //  Public member functions.
  //--------------------------
  virtual void getNextPseudo(char* pCharArray);

}; // class PseudoChar

//<pb>
//=========================================================
//  Pseudo Random Alphabetic Character String in Key Order.
//=========================================================
class PseudoCharKey : public PseudoCharAbs {

  private:
    long col0Range_;
    long col1Range_;
    long col0NextVal_;
    long col1NextVal_;
    
  public:

  //--------------
  //  Constructor.
  //--------------
  PseudoCharKey (long pCol0Range, long pCol1Range) 
    : PseudoCharAbs(),
      col0Range_(pCol0Range),
      col1Range_(pCol1Range),
      col0NextVal_(0),
      col1NextVal_(0)
  {}


  //-------------
  //  Destructor.
  //-------------
  virtual ~PseudoCharKey () {};

  //--------------------------
  //  Public member functions.
  //--------------------------
  virtual void getNextPseudo(char* pCharArray);

}; // class PseudoCharKey

//<pb>
//=====================================
//  Pseudo Random Dates Abstract Class.
//=====================================
class PseudoDateAbs : public PseudoBase {

  private:
    
  protected:

  // Constructors
  PseudoDateAbs (long pSeed, long pRange)
    : PseudoBase(pSeed, pRange)
  {}

  PseudoDateAbs ()
    : PseudoBase(0, 0)
  {}

  public:

  //--------------------------
  //  Public member functions.
  //--------------------------
  virtual void getNextPseudo(__int64& pBaseDateTime) = 0;

}; //class PseudoDateAbs

//<pb>
//======================
//  Pseudo Random Dates.
//======================
class PseudoDate : public PseudoDateAbs {

  private:
    long modulus_;
    
  public:

  //=========================================================================
  //  Constructor for returning pseudo random dates represented as 64 bit 
  //  Julian timestamps.
  //
  // Input:
  //  pSeed    -- seed for priming pseudo random generator
  //  pRange   -- range of random values from 0 through (pRange - 1) 
  //              inclusive to generate
  //  pModulus -- range of random values from 0 through (pModulus - 1) 
  //              inclusive to actually return 
  //
  //
  // Output:
  //  none
  //
  // Return:
  //  none
  //
  // Exceptions:
  //  none
  //
  //=========================================================================
  PseudoDate (long pSeed, long pRange, long pModulus)
    : PseudoDateAbs(pSeed,pRange),
      modulus_(pModulus)
  {}

  //-------------
  //  Destructor.
  //-------------
  virtual ~PseudoDate (void) {};

  //--------------------------
  //  Public member functions.
  //--------------------------
  virtual void getNextPseudo(__int64& pBaseDateTime);

}; // Class PseudoDate

//<pb>
//======================
//  Pseudo Random Dates.
//======================
class PseudoDateKey : public PseudoDateAbs {

  private:
    long range_;
    long nextValue_;
    
  public:

  //--------------
  //  Constructor.
  //--------------
  PseudoDateKey (long pRange)
    : PseudoDateAbs(),
      range_(pRange),
      nextValue_(0)
  {}

  //-------------
  //  Destructor.
  //-------------
  virtual ~PseudoDateKey () {};

  //--------------------------
  //  Public member functions.
  //--------------------------
  virtual void getNextPseudo(__int64& pBaseDateTime);

}; // Class PseudoDatekey

//<pb>
//====================================================
//  Pseudo Random Fixed Point Numerics Abstract Class.
//====================================================
class PseudoFixedPointAbs : public PseudoBase {

  private:
    
  protected:

  // Constructors
  PseudoFixedPointAbs (long pSeed, long pRange)
    : PseudoBase(pSeed, pRange)
  {}

  PseudoFixedPointAbs ()
    : PseudoBase(0, 0)
  {}

  public:

  //--------------------------
  //  Public member functions.
  //--------------------------
  virtual void getNextPseudo(char* pCharArray) = 0;

}; //class PseudoFixedPointAbs

//<pb>
//=====================================
//  Pseudo Random Fixed Point Numerics.
//=====================================
class PseudoFixedPoint : public PseudoFixedPointAbs {

  private:
    long modulus_;
    
  public:

  //--------------
  //  Constructor.
  //--------------
  //=========================================================================
  //  Constructor for returning pseudo random, fixed point value represented 
  //  as null terminated character arrays.
  //
  // Input:
  //  pSeed    -- seed for priming pseudo random generator
  //  pRange   -- range of random values from 0 through (pRange - 1) 
  //              inclusive to generate
  //  pModulus -- range of random values from 0 through (pModulus - 1) 
  //              inclusive to actually return 
  //
  // Output:
  //  none
  //
  // Return:
  //  none
  //
  // Exceptions:
  //  none
  //
  //=========================================================================
  PseudoFixedPoint (long pSeed, long pRange, long pModulus)
    : PseudoFixedPointAbs(pSeed, pRange),
      modulus_(pModulus)
  {}

  //-------------
  //  Destructor.
  //-------------
  virtual ~PseudoFixedPoint () {};

  //--------------------------
  //  Public member functions.
  //--------------------------
  virtual void getNextPseudo(char* pCharArray);

  protected:
    void reverse(char* s);
    void ltoa(long n, char* s);
    void zeropad(char* s, long precision); 

}; // Class PseudoFixedPoint

#endif   // ifndef Pseudo_H
