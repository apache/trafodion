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
#include <fstream>
#include <stdlib.h>
#include <string.h> 
#include "GenColumn.h"

#if !defined(__TANDEM) || (__CPLUSPLUS_VERSION >= 3)
using namespace std;
#endif

IntColumn::IntColumn(long pSeed, long pRange, long pModulus, long* pNullVal)
: pseudoInt_  (new PseudoInt(pSeed,pRange,pModulus)),
  currentVal_ (0),
  nullVal_    (pNullVal)
{
} //IntColumn Constructor 

IntColumn::IntColumn(long pRange, long* pNullVal)
: pseudoInt_  (new PseudoIntKey(pRange)),
  currentVal_ (0),
  nullVal_    (pNullVal)
{
} //IntColumn Constructor for Integer Keys

void
IntColumn::getNextColumnValue()
{

  currentVal_ = pseudoInt_->getNextPseudo();

} // IntColumn::getNextColumnValue()

void
IntColumn::printColumn(ofstream& pStream) const
{

  if (nullVal_ != 0)
    {
      if (currentVal_ == *nullVal_)
        {
          pStream << "NULL";
          return;
        }
    }

  pStream.width(4);
  pStream << currentVal_;
    
} // IntColumn::printColumn()

CharColumn::CharColumn(long  pSeed, 
                       long  pRange,
                       long  pCol0Modulus,
                       long  pCol1Modulus,
                       long  pArraySize,
                       char* pNullVal)
: pseudoChar_(new PseudoChar(pSeed, pRange, pCol0Modulus, pCol1Modulus) ),
  currentVal_(new char[pArraySize+1]),
  nullVal_(pNullVal)
{

  for (long charIdx = 0; charIdx < pArraySize; charIdx++)
    {
      currentVal_[charIdx] = 'A';
    }

  currentVal_[pArraySize] = '\0';

} // CharColumn Constructor

CharColumn::CharColumn(long  pCol0Range,
                       long  pCol1Range,
                       long  pArraySize,
                       char* pNullVal)
: pseudoChar_(new PseudoCharKey(pCol0Range,pCol1Range) ),
  currentVal_(new char[pArraySize+1]),
  nullVal_(pNullVal)
{

  for (long charIdx = 0; charIdx < pArraySize; charIdx++)
    {
      currentVal_[charIdx] = 'A';
    }

  currentVal_[pArraySize] = '\0';

} // CharColumn Constructor for key values

void
CharColumn::getNextColumnValue()
{

  currentVal_[0] = 'A';
  currentVal_[1] = 'A';
  pseudoChar_->getNextPseudo(currentVal_);

} // CharColumn::getNextColumnValue()

void
CharColumn::printColumn(ofstream& pStream) const
{

  if (nullVal_ != 0)
    {
      if (strncmp(currentVal_,nullVal_,strlen(nullVal_)) == 0)
        {
          pStream.width(strlen(currentVal_)+2);
          pStream << "NULL";
          return;
        }
    }
  
  pStream << "'" << currentVal_ << "'";
    
} // CharColumn::printColumn()

void
VarcharColumn::fillArrays(long pMaxArraySize, long pMinArraySize)
{

  long charIdx;
  for (charIdx = 0; charIdx < pMaxArraySize; charIdx++)
    {
      currentMaxVal_[charIdx] = 'A';
    }

  currentMaxVal_[pMaxArraySize] = '\0';

  for (charIdx = 0; charIdx < pMinArraySize; charIdx++)
    {
      currentMinVal_[charIdx] = 'A';
    }

  currentMinVal_[pMinArraySize] = '\0';

} // VarcharColumn::fillArrays()

VarcharColumn::VarcharColumn(long  pSeed, 
                             long  pRange,
                             long  pCol0Modulus,
                             long  pCol1Modulus,
                             long  pMaxArraySize,
                             long  pMinArraySize,
                             char* pNullVal)
: pseudoChar_(new PseudoChar(pSeed, pRange, pCol0Modulus, pCol1Modulus) ),
  valuesGenerated_(0),
  currentMaxVal_(new char[pMaxArraySize+1]),
  currentMinVal_(new char[pMinArraySize+1]),
  nullVal_(pNullVal)
{

  fillArrays(pMaxArraySize,pMinArraySize);

} // VarharColumn Constructor

VarcharColumn::VarcharColumn(long  pCol0Range,
                             long  pCol1Range,
                             long  pMaxArraySize,
                             long  pMinArraySize,
                             char* pNullVal)
: pseudoChar_(new PseudoCharKey(pCol0Range,pCol1Range) ),
  valuesGenerated_(0),
  currentMaxVal_(new char[pMaxArraySize+1]),
  currentMinVal_(new char[pMinArraySize+1]),
  nullVal_(pNullVal)
{

  fillArrays(pMaxArraySize,pMinArraySize);

} // CharColumn Constructor for key values

void
VarcharColumn::getNextColumnValue()
{

  currentMaxVal_[0] = 'A';
  currentMaxVal_[1] = 'A';
  pseudoChar_->getNextPseudo(currentMaxVal_);
  currentMinVal_[0] = currentMaxVal_[0];
  currentMinVal_[1] = currentMaxVal_[1];
  ++valuesGenerated_;

} // VarcharColumn::getNextColumnValue()

void
VarcharColumn::printColumn(ofstream& pStream) const
{

  if (nullVal_ != 0)
    {
      if (strncmp(currentMaxVal_,nullVal_,strlen(nullVal_)) == 0)
        {
          pStream.width(strlen(currentMaxVal_) + 2);
          pStream << "NULL";
          return;
        }
    }
  
  if ((valuesGenerated_ & 1) == 0)
    {
      
      pStream.width(strlen(currentMaxVal_) - strlen(currentMinVal_) + 1);
      pStream << "'" << currentMinVal_ << "'";
    }
  else
    {
      pStream << "'" << currentMaxVal_ << "'";
    }
    
} // VarcharColumn::printColumn()

DateColumn::DateColumn(long     pSeed,
                       long     pRange,
                       long     pModulus,
                       __int64* pNullVal)
: pseudoDate_(new PseudoDate(pSeed, pRange, pModulus) ),
  currentVal_(kBaseDateTime),
  nullVal_(pNullVal)
{

} // DateColumn Constructor

DateColumn::DateColumn(long pRange, __int64* pNullVal)
: pseudoDate_(new PseudoDateKey(pRange) ),
  currentVal_(kBaseDateTime),
  nullVal_(pNullVal)
{

} // DateColumn Constructor for key values

void
DateColumn::getNextColumnValue()
{

  currentVal_ = kBaseDateTime;
  pseudoDate_->getNextPseudo(currentVal_);

} // DateColumn::getNextColumnValue()

void
DateColumn::printColumn(ofstream& pStream) const
{

  if (nullVal_ != 0)
    {
      if (currentVal_ == *nullVal_) 
        {
          pStream << "NULL";
          return;
        }
    }

  char currentValAsString[32];
 (void) _i64toa( currentVal_, currentValAsString, 10 );
  pStream << "cast(converttimestamp("
          << currentValAsString 
          << ") as date)";
    
} // DateColumn::printColumn()

FixedPointColumn::FixedPointColumn(long  pSeed, 
                       long  pRange,
                       long  pModulus,
                       long  pArraySize,
                       char* pNullVal)
: pseudoFixedPoint_(new PseudoFixedPoint(pSeed, pRange, pModulus) ),
  currentVal_(new char[pArraySize]),
  nullVal_(pNullVal)
{

  for (long charIdx = 0; charIdx < pArraySize; charIdx++)
    {
      currentVal_[charIdx] = '\0';
    }

} // FixedPointColumn Constructor

void
FixedPointColumn::getNextColumnValue()
{

  pseudoFixedPoint_->getNextPseudo(currentVal_);

} // FixedPointColumn::getNextColumnValue()

void
FixedPointColumn::printColumn(ofstream& pStream) const
{

  if (nullVal_ != 0)
    {
      if (strcmp(currentVal_,nullVal_) == 0)
        {
          pStream << "NULL";
          return;
        }
    }

  pStream << currentVal_;
    
} // FixedPointColumn::printColumn()
