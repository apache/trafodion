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
#ifndef GenColumn_H
#define GenColumn_H

#include <fstream>
#include "Pseudo.h"

#if !defined(__TANDEM) || (__CPLUSPLUS_VERSION >= 3)
using namespace std;
#endif

//<pb>
//========================
//  Abstract Column Class.
//========================
class AbstColumn {

  private:
    
  public:

  virtual void getNextColumnValue() = 0;

  virtual void printColumn(ofstream& pStream) const = 0;

}; // Class ColumnAbs

typedef AbstColumn* AbstColumnPtr;

//<pb>
//=======================
//  Integer Column Class.
//=======================
class IntColumn : public AbstColumn {

  private:

    PseudoIntAbs* pseudoInt_;

    long          currentVal_;

    const long*   nullVal_;
    
  public:

  //---------------
  //  Constructors.
  //---------------
  IntColumn(long pSeed, long pRange, long pModulus, long* pNullVal = 0);
  IntColumn(long pRange, long* pNullVal = 0);

  //-------------
  //  Destructor.
  //-------------
  virtual ~IntColumn() { delete pseudoInt_; }

  virtual void getNextColumnValue();

  virtual void printColumn(ofstream& pStream) const;

}; // Class IntColumn
//<pb>
//===============================
//  Character Array Column Class.
//===============================
class CharColumn : public AbstColumn {

  private:

    PseudoCharAbs* pseudoChar_;

    char*          currentVal_;

    const char*    nullVal_;
    
  public:

  //---------------
  //  Constructors.
  //---------------
  CharColumn(long  pSeed, 
             long  pRange,
             long  pCol0Modulus,
             long  pCol1Modulus,
             long  pArraySize,
             char* pNullVal = 0);

  CharColumn(long pCol0Range, 
             long pCol1Range,
             long pArraySize,
             char* pNullVal = 0);

  //-------------
  //  Destructor.
  //-------------
  virtual ~CharColumn() { delete pseudoChar_; delete [] currentVal_; }

  //--------------------------
  //  Public member functions.
  //--------------------------
  virtual void getNextColumnValue();

  virtual void printColumn(ofstream& pStream) const;

}; // Class CharColumn
 
//<pb>
//========================================
//  Variable Character Array Column Class.
//========================================
class VarcharColumn : public AbstColumn {

  private:

    PseudoCharAbs* pseudoChar_;

    long           valuesGenerated_;

    char*          currentMaxVal_;

    char*          currentMinVal_;

    const char*    nullVal_;

  void fillArrays(long pMaxArraySize, long pMinArraySize);
    
  public:

  //---------------
  //  Constructors.
  //---------------
  VarcharColumn(long  pSeed, 
                long  pRange,
                long  pCol0Modulus,
                long  pCol1Modulus,
                long  pMaxArraySize,
                long  pMinArraySize,
                char* pNullVal = 0);

  VarcharColumn(long pCol0Range, 
                long pCol1Range,
                long pMaxArraySize,
                long pMinArraySize,
                char* pNullVal = 0);

  //-------------
  //  Destructor.
  //-------------
  virtual ~VarcharColumn() { delete pseudoChar_; 
                             delete [] currentMaxVal_; 
                             delete [] currentMinVal_; }

  //--------------------------
  //  Public member functions.
  //--------------------------
  virtual void getNextColumnValue();

  virtual void printColumn(ofstream& pStream) const;

}; // Class VarcharColumn
 
//<pb>
//====================
//  Date Column Class.
//====================
class DateColumn : public AbstColumn {

  private:

    PseudoDateAbs* pseudoDate_;

    __int64        currentVal_;

    const __int64* nullVal_;
    
  public:

  //---------------
  //  Constructors.
  //---------------
  DateColumn(long pSeed, long pRange, long pModulus, __int64* pNullVal = 0);

  DateColumn(long pRange, __int64* pNullVal = 0);

  //-------------
  //  Destructor.
  //-------------
  virtual ~DateColumn() { delete pseudoDate_; }

  //--------------------------
  //  Public member functions.
  //--------------------------
  virtual void getNextColumnValue();

  virtual void printColumn(ofstream& pStream) const;

}; // Class CharColumn

//-----------------------------------------------------------------
//  Julian timestamp constant value associated with 1 Jaunary 2100.
//-----------------------------------------------------------------
const __int64 kBaseDateTime = 214969204800000000;

//<pb>
//===========================
//  Fixed Point Column Class.
//===========================
class FixedPointColumn : public AbstColumn {

  private:

    PseudoFixedPointAbs* pseudoFixedPoint_;

    char*                currentVal_;

    const char*          nullVal_;
    
  public:

  //---------------
  //  Constructors.
  //---------------
  FixedPointColumn(long  pSeed,
                   long  pRange,
                   long  pModulus,
                   long  pArraySize,
                   char* pNullVal = 0);

  //-------------
  //  Destructor.
  //-------------
  virtual ~FixedPointColumn() { delete pseudoFixedPoint_; delete [] currentVal_; }

  //--------------------------
  //  Public member functions.
  //--------------------------
  virtual void getNextColumnValue();

  virtual void printColumn(ofstream& pStream) const;

}; // Class FixedPointColumn
  
#endif   // ifndef GenColum_H
