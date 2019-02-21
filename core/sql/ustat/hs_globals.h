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
#ifndef HSGLOBALS_H
#define HSGLOBALS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         hs_globals.h
 * Description:  Global structures.
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "hs_const.h"
#include "hs_cont.h"
#include "hs_cli.h"
#include "hs_la.h"
#include "BloomFilter.h"
#include "nawstring.h"
#include "Collections.h"
#include "ComVersionDefs.h" 	
#include "ComSmallDefs.h"
#include "NABitVector.h"
#include <exp_function.h>

// -----------------------------------------------------------------------
// Externals.
// -----------------------------------------------------------------------
class ComDiagsArea;

extern THREAD_P Int32       lengthOfSortBufrs ;
extern THREAD_P char *    sortBuffer1 ;
extern THREAD_P char *    sortBuffer2 ;

typedef NAHashDictionary<NAString, double> JitLogHashType;

// -----------------------------------------------------------------------
// Forward.
// -----------------------------------------------------------------------
struct HSColumnStruct;
struct HSColGroupStruct;
struct HSColDesc;
class HSGlobalsClass;
class HSInterval;
class HSHistogram;
class HSInMemoryTable;
class AbstractFastStatsHist;

Lng32 AddNecessaryColumns();
Lng32 AddAllColumnsForIUS();

void createSampleOption(Lng32 sampleType, double samplePercent, NAString &sampleOpt,
                        Int64 sampleValue1=0, Int64 sampleValue2=0);
Lng32 doubleToHSDataBuffer(const double dbl, HSDataBuffer& dbf);
Lng32 managePersistentSamples();

template <class T>
Lng32 setBufferValue(T& value,
                      const HSColGroupStruct *group,
                      HSDataBuffer &boundary);


// An instance of ISFixedChar represents a value of a fixed-length character
// string (either single or double-byte) retrieved into memory for use by
// internal sort. A pointer to the actual string is maintained, and definitions
// are provided for all operators used by the template functions that implement
// the internal sort processing.
//
// The static member variable 'length' is used to store the fixed length of the
// referenced strings, avoiding the need to store it separately for each
// instance. However, the class is used for strings of any length, so before
// processing a char(n) column for internal sort, setLength(n) must be called.
// The length is in bytes, not characters.
//
// memcmp is used in the implementation of equality operators, because this
// will work for both single and double-byte strings. For UCS2 comparison we
// use na_wcsnncmp(), but for regular character columns we use memcmp.
//
// When a char column is sorted, the actual strings remain in place in the
// buffer they are originally read into. An array of ISFixedChar objects that
// reference those strings are exchanged instead to perform the sort.
//
class ISFixedChar
{
  public:
    ISFixedChar()
      : content(NULL)
    {}

    // Copy ctor used by placeWidePivot() to create an instance that is the
    // pivot value. Can't use reference to existing element in array, because
    // it will be overwritten as elems are moved around during sort.
    ISFixedChar(const ISFixedChar& other)
    {
      content = other.content;
    }

    void static setLength(Int32 len)
    {
      length = len;
    }

    Int32 static getLength()
    {
      return length;
    }

    void static setCaseInsensitive(NABoolean ci)
    {
      caseInsensitive = ci;
    }

    static void setColCollation(CharInfo::Collation Collation)
    {
      colCollation = Collation;
    }

    static void setCharSet(CharInfo::CharSet CharSet)
    {
      charset = CharSet;
    }

    char* getContent()
    {
      return content;
    }

    void setContent(char* ptr)
    {
      content = ptr;
    }

    // Have to define new[] and delete[] here if we want to use NAHeap, because
    // NABasicObject does not define array versions of those operators. Even if
    // it did, we wouldn't subclass it because it would make the objects bigger
    // (NABasicObject has a heap ptr member variable).
    /*
    void* operator new[](size_t size)
    {
      return STMTHEAP->allocateMemory(size, FALSE);
    }

    void operator delete[](void *addr)
    {
      STMTHEAP->deallocateMemory(addr);
    }
*/

    // Note that we forego the usual convention of having operator= return a
    // reference to the assigned-to object. This is just to make this operation
    // as efficient as possible, since it will be performed many times.
    void operator=(const ISFixedChar &rhs)
    {
      content = rhs.content;
    }

    void operator=(char* ptr)
    {
      content = ptr;
    }

    // Compare this object to rhs, returning negative value if less, 0 if equal,
    // and positive value if greater.
    Int32 compare(const ISFixedChar &rhs);

    Int32 operator==(const ISFixedChar &rhs)
    {
      // Note that case insensitive is not supported with non-binary collation.
      if (CollationInfo::isSystemCollation(colCollation))
          return (Collated_cmp(content, rhs.content, length, colCollation,
                               sortBuffer1, sortBuffer2 ) == 0);

      // UCS2 cols not supported in MODE_SPECIAL_1 or 2 and do not support case insensitivity.
      // memcmp() can be used here because we are looking for equality.
      if (!caseInsensitive) return !memcmp(content, rhs.content, length);
      else                  return !hs_strncasecmp(content, rhs.content, length);
    }

    Int32 operator!=(const ISFixedChar &rhs)
    {
      return !(*this == rhs);
    }

    Int32 operator<(const ISFixedChar &rhs)
    {
      return (compare(rhs) < 0);
    }

    Int32 operator<=(const ISFixedChar &rhs)
    {
      return (compare(rhs) <= 0);
    }

    Int32 operator>(const ISFixedChar &rhs)
    {
      return (compare(rhs) > 0);
    }

    Int32 operator>=(const ISFixedChar &rhs)
    {
      return (compare(rhs) >= 0);
    }

    // These operators must be defined to allow this type to be used with
    // existing templates.
    operator Int64() { fail("Int64()", __LINE__); return 0; };
    operator Int32() { fail("int()", __LINE__); return 0; };
    Int32 operator/(Int32 i) { fail("/", __LINE__); return 0; };
    Int32 operator%(Int32 i) { fail("%", __LINE__); return 0; };
    Int32 operator>=(Int32 i) { fail(">=", __LINE__); return 0; };
    Int32 operator<(Int32 i) { fail("<", __LINE__); return 0; };
    ISFixedChar& operator-() { fail("-", __LINE__); return *this; };

  protected:
    // Give internal error if undefined operator invoked.
    void fail(const char* opName, Lng32 line);

    // To make ISFixedChar as lightweight as possible, we use a static member to hold
    // the length and case sensitivity, rather than repeating it for each instance. 
    // These must be set before each char column is processed.
    // Likewise with column collation.
    static THREAD_P Int32 length;
    static THREAD_P NABoolean caseInsensitive;
    static THREAD_P CharInfo::Collation colCollation;
    static THREAD_P CharInfo::CharSet charset;

    // The content is a fixed-length string, where the length is the current
    // value of the static 'length' member variable.
    char* content;
};


// This class extends ISFixedChar and is used to represent values of a fixed-length
// character string being processed for IUS. It is used in two ways:
//   1) In the arrays of interval boundary values and MFV values used in
//      processIUSColumns. When used for this purpose, the 'content' (inherited)
//      member variable has a single, fixed value, which the instance owns and
//      takes responsibility for deleting.
//   2) To serially represent the values of a column in an in-memory table. In
//      this case, a single instance of the class is used in conjunction with
//      the IUSValueIterator class and assumes each in-memory value of the
//      column in turn as the next() member function is called to move the
//      content ptr to the next value. These values assumed by the ptr are
//      addresses within the strData buffer of HSColGroupStruct, and so are
//      not owned by this class.
class IUSFixedChar : public ISFixedChar
{
  public:
    IUSFixedChar(NABoolean ownsContent = TRUE)
      : ISFixedChar(),
        ownsContent_(ownsContent)
    {}

    virtual ~IUSFixedChar()
    {
      // Content may be allocated and owned for this subclass, but not ISFixedChar.
      if (ownsContent_)
        NADELETEBASIC(content, STMTHEAP);
    }

    // Assignment from an HSDataBuffer is how the object is initialized to an
    // interval boundary or MFV value.
    void operator=(const HSDataBuffer& buff);

    // Move content ptr to start of next value.
    void next()
      {
        content += (length * (charset == CharInfo::UNICODE ? sizeof(NAWchar) : 1));
      }

  private:
    // If TRUE, content must be deleted when this object goes away.
    NABoolean ownsContent_;
};


// This class performs the same function as ISFixedChar, except for varying
// length character strings. See the documentation of ISFixedChar for more
// information. The essential difference is that an ISVarChar object points
// to storage that includes the actual length of the string in the first two
// bytes, followed immediately by the string itself.
class ISVarChar
{
  public:
    ISVarChar()
      : content(NULL)
    {}

    char* getContent()
    {
      return content;
    }

    short getLength()
    {
      return *(Int16*)content; 
    }

    void setContent(char* ptr)
    {
      content = ptr;
    }

    void static setDeclaredLength(Int32 len)
    {
      declaredLength = len;
    }

    void static setCaseInsensitive(NABoolean ci)
    {
      caseInsensitive = ci;
    }

    static void setColCollation(CharInfo::Collation Collation)
    {
      colCollation = Collation;
    }

    static void setCharSet(CharInfo::CharSet CharSet)
    {
      charset = CharSet;
    }

    // Have to define new[] and delete[] here if we want to use NAHeap. Even if
    // NABasicObject defined the array forms of these operators, we wouldn't
    // subclass it because it would make the objects bigger (NABasicObject has
    // a heap ptr member variable).
    void* operator new[](size_t size)
    {
      return STMTHEAP->allocateMemory(size, FALSE);
    }

    void operator delete[](void *addr)
    {
      STMTHEAP->deallocateMemory(addr);
    }

    // Note that we forego the usual convention of having operator= return a
    // reference to the assigned-to object. This is just to make this operation
    // as efficient as possible, since it will be performed many times.
    void operator=(const ISVarChar &rhs)
    {
      content = rhs.content;
    }

    void operator=(char* ptr)
    {
      content = ptr;
    }

    // Compare this object to rhs, returning negative value if less, 0 if equal,
    // and positive value if greater.
    Int32 compare(const ISVarChar &rhs);

    Int32 operator==(const ISVarChar &rhs);

    Int32 operator!=(const ISVarChar &rhs)
    {
      return !(*this == rhs);
    }

    Int32 operator<(const ISVarChar &rhs)
    {
      return compare(rhs) < 0;
    }

    Int32 operator>(const ISVarChar &rhs)
    {
      return compare(rhs) > 0;
    }

    Int32 operator<=(const ISVarChar &rhs)
    {
      return compare(rhs) <= 0;
    }

    Int32 operator>=(const ISVarChar &rhs)
    {
      return compare(rhs) >= 0;
    }

    // These operators must be defined to allow this type to be used with
    // existing templates.
    operator Int64() { fail("Int64()", __LINE__); return 0; };
    operator Int32() { fail("int()", __LINE__); return 0; };
    Int32 operator/(Int32 i) { fail("/", __LINE__); return 0; };
    Int32 operator*(Int32 i) { fail("*", __LINE__); return 0; };
    Int32 operator%(Int32 i) { fail("%", __LINE__); return 0; };
    Int32 operator>=(Int32 i) { fail(">=", __LINE__); return 0; };
    Int32 operator<(Int32 i) { fail("<", __LINE__); return 0; };
    ISVarChar& operator-() { fail("-", __LINE__); return *this; };

  protected:
    // Give internal error if undefined operator invoked.
    void fail(const char* opName, Lng32 line);

    // To make ISVarChar as lightweight as possible, we use a static members to
    // hold column attributes, rather than repeating them for each instance. 
    // They must be set before each char column is processed.
    static THREAD_P Int32 declaredLength;
    static THREAD_P NABoolean caseInsensitive;
    static THREAD_P CharInfo::Collation colCollation;
    static THREAD_P CharInfo::CharSet charset;

    // The content pointed to by objects of this class consists of a 2-byte
    // field giving the length in bytes, immediately followed by a string
    // represented by that number of bytes.
    char* content;
};


// IUSVarChar extends ISVarChar in much the same way and for the same purposes
// that IUSFixedChar does for ISFixedChar. The implementations of next() and
// the assignment from HSDataBuffer differ due to the presence of a length
// indicator for varchars.
class IUSVarChar : public ISVarChar
{
  public:
    IUSVarChar(NABoolean ownsContent = TRUE)
      : ISVarChar(),
        ownsContent_(ownsContent)
    {}

    virtual ~IUSVarChar()
    {
      // Content may be allocated and owned for this subclass, but not ISVarChar.
      if (ownsContent_)
        NADELETEBASIC(content, STMTHEAP);
    }

    void operator=(const HSDataBuffer& buff);

    void next()
      {
        // strData contains declared (not just actual) number of chars.
        content += (sizeof(Int16) +     // # bytes in length field
                    (declaredLength * (charset == CharInfo::UNICODE ? sizeof(NAWchar) : 1)));

        // Each piece of varchar data (including the data length) is placed
        // at the even-address boundary. See switch statement case for 
        // VARCHAR in method HSGlobalsClass::processInternalSortNulls(). 
        // Here we follow the same logic (for pointer "content").
        if ( ( (ULong(content)) & 1) == 1 )
          content++;
      }

  private:
    NABoolean ownsContent_;
};


//---------------------------------- FOR MC CHANGES --------------------------------------------------

// for MC 
//  The iterator classes are used by the MCWrapper objects to help compare columns of a given type.
//
//  The MCIterator class hierarchy is as follows:
//
//                                                    MCIterator
//                                                        |
//                                                        |
//                             -------------------------------------------------------
//                            |                           |                           |
//                            |                           |                           |
//                    MCiFixedCharIterator         MCNonCharIterator            MCVarCharIterator 
//
//
class MCIterator
{
public:
   MCIterator() : nullInd(NULL) {};
   virtual ~MCIterator() {}

   // compares two values in athe array of data maintained by this
   // iterator. "left" and "right" and indices into this array.
   // The method returns a negative value if less, 0 if equal,
   // and positive value if greater.
   virtual Int32 compare (Int32 left, Int32 right) = 0;
   virtual void print (ofstream& fout, Int32 index) = 0;

   NABoolean isNull(Int32 index)
   {
      return (nullInd && nullInd->testBit(index));
   }

   void dumpBits (const char* f_name, Int32 nRows)
   {
       ofstream fileout(f_name, ios::app);

       fileout << "================ Printing bit Set ================\n";
       
       if (!nullInd)
          fileout << "no nullable";
       else
       {
            for (Int32 i =0; i < nRows; i++)
            {
               if (nullInd->testBit(i))
                 fileout << "NULL\n";
               else
                 fileout << "NOT NULL\n";
            }
       }

       fileout << "================ Printing bit Set ================\n";
   }

   Lng32      ISdatatype;  
   // bitmap of null (data) columns
   NABitVector* nullInd;
};


// template class for all non-character datatypes iterators
template <class T>
class MCNonCharIterator : public MCIterator
{
  public:
    MCNonCharIterator(T* ptr)
      : vp(ptr)
    {}

    virtual ~MCNonCharIterator()
    {}

    T* getContent(Int32 index)
    {
       T* vp1 = vp + index;
       return (vp1);
    }

    Int32 compare (Int32 left, Int32 right)
    {
       if (this->nullInd)
       {
          NABoolean leftNull  = (this->nullInd->testBit(left));
          NABoolean rightNull = (this->nullInd->testBit(right));

          if (leftNull || rightNull)
          {
             if (leftNull && rightNull)
               return 0;
             else if (leftNull) // null sorts higher than non-null
               return 1;
             return -1;
          }
       }

       T* vp1 = vp + left;
       T* vp2 = vp + right;

       if (*vp1 == *vp2)
         return 0;
       else if (*vp1 > *vp2)
         return 1;

         return -1;
    }
 
    void print (ofstream& fout, Int32 index) 
    { 
       T* vp1 = vp + index;

       if (nullInd && nullInd->testBit(index))
         fout << "NULL";
       else
         fout << *vp1;
    }

    T* vp;
};

// fixed charater type iterator
class MCFixedCharIterator : public MCIterator
{
  public:
    MCFixedCharIterator(char* ptr, Int32 newLength)
      : vp(ptr)
    {
      length=newLength;
    }

    virtual ~MCFixedCharIterator()
    {}
  
    void copyToISFixChar(ISFixedChar& fixChar, Int32 index)
    {
       fixChar.setLength(length);
       fixChar.setContent(vp + (index*length));
    }

    Int32 compare (Int32 leftIndex, Int32 rhIndex)
    {
      if (nullInd)
      {
         NABoolean leftNull  = (nullInd->testBit(leftIndex));
         NABoolean rightNull = (nullInd->testBit(rhIndex));
  
         if (leftNull || rightNull)
         {
            if (leftNull && rightNull)
              return 0;
            else if (leftNull) // null sorts higher than non-null
              return 1;
            return -1;
         }
      }

      char* vp1 = vp + (leftIndex*length);
      char* vp2 = vp + (rhIndex*length);

      // Note that case insensitive is not supported with non-binary collation.
      if (CollationInfo::isSystemCollation(colCollation))
         return Collated_cmp(vp1, vp2, length, colCollation, sortBuffer1, sortBuffer2);

      // UCS2 cols not supported in MODE_SPECIAL_1 or 2 and do not support case insensitivity.
      if (!caseInsensitive)
      {
        if (charset != CharInfo::UNICODE)
          return memcmp(vp1, vp2, length);
        else
          return na_wcsnncmp((const wchar_t *)vp1, length/sizeof(NAWchar),
                             (const wchar_t *)vp2, length/sizeof(NAWchar));
      }
      else
        return hs_strncasecmp(vp1, vp2, length);
    }

    void print (ofstream& fout, Int32 index)
    {
      char* vp2 = vp + (index*length);
  
      if (nullInd && nullInd->testBit(index))
         fout << "NULL";
      else
      {
        char *temp = new char[length+1];
        strncpy (temp, vp2, length);
        temp[length] = '\0';
        fout << temp;
        delete temp;
      }
    }

    // These must be set before each char column is processed.
    NABoolean caseInsensitive;
    CharInfo::Collation colCollation;
    CharInfo::CharSet charset;

  protected:
    char* vp;
    Int32 length;
};

// variable charater type iterator
class MCVarCharIterator : public MCIterator
{
  public:
    MCVarCharIterator(char* ptr)
      : vp(ptr)
    {
    }

    MCVarCharIterator(MCVarCharIterator& rh)
    {
      vp = rh.vp;
      rowLength = rh.rowLength;
    }

    void copyToISVarChar (ISVarChar& varChar, Int32 index)
    {
       varChar.setContent(vp + (index*rowLength));
    }

    virtual ~MCVarCharIterator()
    {}

    Int32 compare (Int32 leftIndex, Int32 rhIndex)
    {
      if (nullInd)
      {
         NABoolean leftNull  = (nullInd->testBit(leftIndex));
         NABoolean rightNull = (nullInd->testBit(rhIndex));
  
         if (leftNull || rightNull)
         {
            if (leftNull && rightNull)
              return 0;
            else if (leftNull) // null sorts higher than non-null
              return 1;
            return -1;
         }
      }

      char* vp1 = vp + (leftIndex*rowLength);
      char* vp2 = vp + (rhIndex*rowLength);

      short len1 = *(short *) vp1;
      short len2 = *(short *) vp2;

      // Note that case insensitive is not supported with non-binary collation.
      if (CollationInfo::isSystemCollation(colCollation))
         return Collated_cmp(vp1+VARCHAR_LEN_FIELD_IN_BYTES,
                             vp2+VARCHAR_LEN_FIELD_IN_BYTES,
                             MAXOF(len1, len2),
                             colCollation, sortBuffer1, sortBuffer2);

      // UCS2 cols not supported in MODE_SPECIAL_1 or 2 and do not support case insensitivity.
      if (!caseInsensitive) 
      {
        if (charset != CharInfo::UNICODE)
          return memcmp(vp1+VARCHAR_LEN_FIELD_IN_BYTES,
                        vp2+VARCHAR_LEN_FIELD_IN_BYTES,
                        MAXOF(len1, len2));
        else
          return na_wcsnncmp((const wchar_t*)(vp1+VARCHAR_LEN_FIELD_IN_BYTES), len1/sizeof(NAWchar),
                             (const wchar_t*)(vp2+VARCHAR_LEN_FIELD_IN_BYTES), len2/sizeof(NAWchar));
      }
      else
        return hs_strncasecmp(vp1+VARCHAR_LEN_FIELD_IN_BYTES,
                              vp2+VARCHAR_LEN_FIELD_IN_BYTES,
                              MAXOF(len1, len2));
    }
  
    void print (ofstream& fout, Int32 index)
    {
      char* vp2 = vp + (index*rowLength);
  
      if (nullInd && nullInd->testBit(index))
         fout << "NULL";
      else
      {
        short strLen = *(short *) vp2;
        char *temp = new char[strLen+1];
        strncpy (temp, vp2+sizeof(short), strLen);
        temp[strLen] = '\0';
        fout << temp;
        delete temp;
      }
    }

    Int32 rowLength;
  
    // These must be set before each char column is processed.
    NABoolean caseInsensitive;
    CharInfo::Collation colCollation;
    CharInfo::CharSet charset;

  protected:
    char* vp;
};

// MCWrapper class is used to encapsulte MC rows
//
// Each MCWrapper object represents a row of the MC. The MCWrapper class has static iterators to encapsulte columns. 
// These iterators are used by the internal sort to compare rows.
//
// Example: let's assume our data consists of 3 rows with each row has 2 columns. Column 1 of type Int32 and Column 2 
//          of type Int64. Column 2 is a nullable column. Column1 and Column 2 iterators point to where the actual data
//          is.
//
//             MCWrapper objects                     Iterator objects
//   row1: index=0, cols  ----|                         |- col1: MCNonCharIterator<Int32> --> Int32* vp --> 1,12,3
//   row2: index=1, cols  ----| ------------------------|- col2: MCNonCharIterator<Int64> --> Int64* vp --> 10,4,NULL
//   row3: index=2, cols  ----|
//
//
//
class MCWrapper
{
  public:
    MCWrapper()
      : index_ (0)
    {} 

    void setIndex (Int32 newIndex)
    {
        index_ = newIndex;
    }

    // Have to define new[] and delete[] here if we want to use NAHeap, because
    // NABasicObject does not define array versions of those operators. Even if
    // it did, we wouldn't subclass it because it would make the objects bigger
    // (NABasicObject has a heap ptr member variable).
    MCWrapper(const MCWrapper &other)
    {
       index_ = other.index_;
    }

    // Have to define new[] and delete[] here if we want to use NAHeap, because
    // NABasicObject does not define array versions of those operators. Even if
    // it did, we wouldn't subclass it because it would make the objects bigger
    // (NABasicObject has a heap ptr member variable).
    void* operator new[](size_t size)
    {
      return STMTHEAP->allocateMemory(size, FALSE);
    }

    void operator delete[](void *addr)
    {
      STMTHEAP->deallocateMemory(addr);
    }

    // Note that we forego the usual convention of having operator= return a
    // reference to the assigned-to object. This is just to make this operation
    // as efficient as possible, since it will be performed many times.
    void operator=(const MCWrapper& rh)
    {
       index_ = rh.index_;
    }

    Int32 operator==(const MCWrapper& rh)
    {
       Int32 i = 0;

       //if (index_ == rh.index_)
          //return (TRUE);

       while ((i < numOfCols_) && (cols_[i]->compare(index_, rh.index_) == 0))
       {
          i++;
       }
       return (i == numOfCols_);
    }

    Int32 operator!=(const MCWrapper& rh)
    {
       return !(*this == rh);
    }

    Int32 operator<(const MCWrapper& rh)
    {
       Int32 i = 0;
       Int32 result  = 0;
  
       while ((i < numOfCols_) && ((result = (cols_[i]->compare(index_, rh.index_))) == 0))
       {
          i++;
       }

       return (result < 0);
    }

    // are all MC columns nullable?
    static NABoolean areAllMCColsNullable()
    {
       Int32 i = 0;
       NABoolean allNullable = TRUE;

       while ((i < numOfCols_) && allNullable)
       {
          if (!cols_[i++]->nullInd)
            allNullable = FALSE;
       }

       return allNullable;
    }

    // are all MC columns' values null?
    static NABoolean areAllMCColsNull(Int32 rowIndex)
    {
       Int32 i = 0;
       NABoolean allNulls = TRUE;

       // first check is defensive since we should not be
       // calling this method if any of the columns is
       // not nullable
       while ((i < numOfCols_) && allNulls)
       {
          if (!cols_[i]->nullInd || !(cols_[i]->nullInd->testBit(rowIndex)))
            allNulls = FALSE;
          i++;
       }

       return allNulls;
    }

    // for debugging - print all the values of a given column
    void print_column (const char* f_name, NABoolean printHeader, NABoolean printFooter, Int32 col)
    {
       if (!f_name)
         return;
 
       ofstream fileout(f_name, ios::app);

       if (printHeader)
          fileout << "================ Printing new MC Data Set ================\n";

       allCols_[col]->print(fileout, index_);
       fileout << "\n";

       if (printFooter)
          fileout << "================ Done Printing MC Data Set ================\n\n";
    }

    // for debugging - print all the values of all columns
    void print (const char* f_name, NABoolean printHeader, NABoolean printFooter)
    {
       if (!f_name)
         return;
 
       ofstream fileout(f_name, ios::app);

       if (printHeader)
          fileout << "================ Printing new MC Data Set ================\n";

       Int32 i = 0;
       while (i < numOfCols_)
       {
          cols_[i++]->print(fileout, index_);
          fileout << " ";
       }
       fileout << "\n";

       if (printFooter)
          fileout << "================ Done Printing MC Data Set ================\n\n";
    }

    static Lng32 setupMCColumnIterator (HSColGroupStruct *group, MCIterator** iter, MCIterator** iter2, 
                                        Int32 &currentLoc, Int32 &notNullLoc, Int32 numRows);

    static void setupMCIterators(HSColGroupStruct *mgroup, Int32 numRows);

    // free up all memory allocated by the iterators (columns)
    void freeColsMem()
    {
       Int32 numAllCols = numOfAllCols_-1;

       while (numAllCols >= 0)
       {
         NADELETEBASIC(allCols_[numAllCols], STMTHEAP);
         numAllCols--;
       }

       NADELETEBASIC(allCols_, STMTHEAP);
       NADELETEBASIC(cols_, STMTHEAP);
       cols_ = NULL;
       allCols_ = NULL;
    }

    // all MC columns that have data (columns that are all nulls are excluded)
    static THREAD_P MCIterator** cols_;
    // all MC columns 
    static THREAD_P MCIterator** allCols_;
    static THREAD_P Int32 numOfCols_;
    static THREAD_P Int32 numOfAllCols_;
    // number of null rows for this MC
    static THREAD_P Int32 nullCount_;
    // index of this MC row
    Int32 index_;

    // These operators must be defined to allow this type to be used with
    // existing templates.
    operator Int64() { fail("Int64()", __LINE__); return 0; };
    operator Int32() { fail("int()", __LINE__); return 0; };
    Int32 operator/(Int32 i) { fail("/", __LINE__); return 0; };
    Int32 operator%(Int32 i) { fail("%", __LINE__); return 0; };
    Int32 operator>=(Int32 i) { fail(">=", __LINE__); return 0; };
    Int32 operator<(Int32 i) { fail("<", __LINE__); return 0; };
    MCWrapper& operator-() { fail("-", __LINE__); return *this; };

  protected:
    // Give internal error if undefined operator invoked.
    void fail(const char* opName, Lng32 line);
};

//---------------------------------- END of MC IS classes --------------------------------------------------


// -----------------------------------------------------------------------
// Linked to HSColGroupStruct.
// The HSColGroupStruct now has a "NASet" of HSColumnStruct rather than a linked list
// to avoid duplicate permutations of the same set of columns.
// This implies that the operator == needs to be defined to allow the set insertion
// to work correctly.
// -----------------------------------------------------------------------
struct HSColumnStruct : public NABasicObject
  {
    NAString         *colname;        /* column name              */
    NAString         *externalColumnName;  /* column name to use in SQL (e.g. with delimiters) */
    Lng32              colnum;         /* column position in table */
    Lng32              position;       /* position in grouplist    */
    Lng32              datatype;
    Lng32              caseInsensitive;/* 1 if char col is not case sensitive, else 0 */
    Lng32              nullflag;       /* 1 if col value can be null, else 0 */
    CharInfo::CharSet  charset;
    CharInfo::Collation colCollation; /* column's collation enum value */
    Lng32              length;
    Lng32              precision;
    Lng32              scale;

    HSColumnStruct(const HSColumnStruct &src, NAMemory *h=STMTHEAP);

    HSColumnStruct()
      : colname(new(STMTHEAP) NAString(STMTHEAP)),
        externalColumnName(new(STMTHEAP) NAString(STMTHEAP)),
        colnum(-1), position(0), datatype(-1), nullflag(-1),
        charset(CharInfo::UnknownCharSet),
        length(-1), precision(-1), scale(-1),
        colCollation(CharInfo::DefaultCollation),
        caseInsensitive(-1)
    {}

    HSColumnStruct& operator=(const HSColumnStruct& rhs);

    NABoolean operator==(const HSColumnStruct&) const;

    void addTruncatedColumnReference(NAString & qry);

    ~HSColumnStruct();
  };

typedef NASet<HSColumnStruct>  HSColSet;           /* set of column structs   */

// This enumerates the various states a single-column group can be in with
// respect to internal sort.
// NOTE: Any changes or additions to this enum must be mirrored in the
//       SortStateName array defined in hs_globals.cpp.
enum SortState
  {
    UNPROCESSED,    // Hasn't been selected yet
    PENDING,        // Selected for batch currently being processed
    OVERRAN,        // Selected for batch currently being processed but
                    // there isn't enough memory (happens only with
                    // varchar compaction where we underestimated average
                    // varchar size)
    PROCESSED,      // Already processed
    DONT_TRY,       // Memory allocation failed, don't try this one again
    SKIP,           // SKIP for the time being
    NO_STATS        // no stats found during IUS processing
  };


//  Used by MC in-memory logic. Every MC has a  weight to group MCs
//  in group sets that can be processed together
// 
class MCWeight
{
public:
   MCWeight () : u(0), v(0), w(0) {};

   Int32 operator== (const MCWeight &rhs)
   {
     return ((u == rhs.u) && (v == rhs.v) && (w == rhs.w));
   }

   Int32 operator< (const MCWeight &rhs)
   {
     return ((u < rhs.u) ||
             (u==rhs.u) && (v > rhs.v) ||
             (u==rhs.u) && (v == rhs.v) && (w < rhs.w));
   }

   Int32 operator<= (const MCWeight &rhs)
   {
     return ((*this < rhs) || (*this == rhs));
   }

   void clear ()
   {
      u = v = w = 0;
   }

   NABoolean isNull()
   {
      return ((u == 0) && (v == 0) && (w == 0));
   }

   // number of other MCs this MC has common columns with
   Int32 u;
   // number of distinct columns that are used by this MC 
   // but not by other MCs this MC has common columns with
   Int32 v;
   // number of columns that are only used by this MC
   Int32 w;
};

// -----------------------------------------------------------------------
// Linked to HSGlobalsClass.
// -----------------------------------------------------------------------
struct HSColGroupStruct : public NABasicObject
  {
    HSColSet          colSet;                      /* set of column structs   */
    Lng32              colCount;                    /* #columns in group       */
    NAString         *clistr;                      /* general query statement */
    ULng32     oldHistid;                   /* old histogram_id        */
    ULng32     newHistid;                   /* new histogram_id        */
    NAString          oldHistidList;               /* a list of old hist ids, in case of duplicates */
    NAString         *colNames;                    /* list of columns in group*/
    HSHistogram      *groupHist;                   /* histogram for group     */
    HSColGroupStruct *next;
    HSColGroupStruct *prev;                        /* reverse list for SHOWSTATS */
    HSColGroupStruct *mcis_next;                   /* For MC IS to point to next neighbor*/
    char              readTime[TIMESTAMP_CHAR_LEN+1]; /* read time; carry over to new hist */
    double            coeffOfVar;                  /* coefficient of variation (skew of this hist) */
    double            oldAvgVarCharSize;           /* average varchar size from previous histograms */
    Int64             rowsRead;                    /* number of rows read for IS so far */ 
    Int64             sumSize;                     /* sum of varchar size for IS so far */
    double            avgVarCharSize;              /* average varchar size, -1 for other types */    
    char              reason;                      /* automation reason        */
    char              newReason;                   /* automation reason for updated hist */
    NABoolean         skewedValuesCollected;       /* Applies to only MC Groups */


    // These member items are used for internal sort of single-column groups.
    SortState        state;                        /* Internal sort status    */
    NABoolean        delayedRead;   
    size_t           memNeeded;                    /* memory required, in bytes */
    size_t           strMemAllocated;              /* memory allocated, in bytes, for char data;
                                                      if compacted, this is just the area used
                                                      for compacted data                         */
    void             *data;                        /* Storage for column values */
    void             *nextData;                    /* Ptr to next place to store a value */
    void             *strData;                     /* Storage for char cols; data/nextdata */
    void             *strNextData;                 /*   will be ptrs to this */
    NABoolean        strDataConsecutive;           /* True if strData is as originally read */
    void             *varcharFetchBuffer;          /* Direct fetch addr for varchar values that will be compacted */
    short            *nullIndics;                  /* Storage for null indicators */
    Int64            nullCount;                    /* Number of null values   */
    NABoolean        eligibleForVarCharCompaction; /* true if OK to use compaction on internal sort */
    Lng32            ISdatatype;                   /* converted type for sorting */
    Lng32            ISlength;                     /* len of converted type */
    Lng32            ISvcLenUsed;                  /* varchar only; if compacted, is avg length which is usually < ISlength */
    Lng32            ISprecision;                  /* prec of converted type */
    Lng32            ISscale;                      /* scale of converted type */
    NAString         ISSelectExpn;                 /* select list expn to retrieve col */
    Int64            prevRowCount;                 /* rowcount from existing histogram */
    Int64            prevUEC;                      /* uec from existing histogram */
    Int64            colSecs;                      /* Time to sort/group data for column */
    CountingBloomFilter* cbf;                      /* A bloom filter for IUS */
    NAString& cbfFileNameSuffix() { return *colSet[0].colname; }

    void* boundaryValues;                          /* List of bounary values for IUS */
    void* MFVValues;                               /* List of MFV values for IUS */

    AbstractFastStatsHist* fastStatsHist;

    // These member items are used for internal sort of multi-column groups.
    NABitVector*       mcis_nullIndBitMap;           /* used by MC */
    NABitVector*       mcis_colsUsedMap;             /* used by MC: which single cols used by this MC */
    NABitVector*       mcis_colsMissingMap;          /* used by MC: which single cols not used by this MC but */
    size_t           mcis_totalMCmemNeeded;        /* memory required, in bytes for MC structures overhead*/
    void             *mcis_data;                   /* copy of Storage for column values used by MC*/
    void             *mcis_nextData;               /* copy Ptr to next place to store a value used by MC*/
    Int32            mcis_rowsRead;                /* used for MC: total number of rows read for IS */
    Int32            mcs_usingme;                  /* used for MC: number of MCs using this single column */
    MCWeight         mcis_groupWeight;             /* used by MC: weight of the MC */
    NABoolean        mcis_groupHead;               /* used by MC: is this a group head */
    NABoolean        mcis_memFreed;                /* used by MC: is memory used by IS for this SC freed */
    NABoolean        mcis_readAsIs;                /* used for MC IS where a column is read to memory again */
                                                   /* are used by its neighbors. Used to compute group weight */

    NABoolean allKeysInsertedIntoCBF;
    Int32            backwardWarningCount;          // for UERR_UNEXPECTED_BACKWARDS_DATA warnings

    #ifdef _TEST_ALLOC_FAILURE
    // Stuff used to test memory allocation failures.
    #define MAX_FILTER_COUNT 10
    static Int32 allocCount;
    Lng32 filterTargets[MAX_FILTER_COUNT];
    void initFilter();
    NABoolean allocFilter(Lng32 count);
    #endif
    
    // @ZX Should we allow this to be called for non-varchar?
    NABoolean isCompacted()
    {
      if (!DFS2REC::isAnyVarChar(ISdatatype))
        return FALSE;
      // TODO: next line causes a compilation error... why?
      //HS_ASSERT(ISvcLenUsed > 0 && ISvcLenUsed <= ISlength);
      return ISlength != ISvcLenUsed;
    }

    void setISlength(Lng32 len, Lng32 maxVarCharLengthInBytes);

    // Size in bytes allocated for per varchar value in strData.
    size_t varcharContentSize()
    {
      return varcharContentSize(ISvcLenUsed);
    }

    // For a compacted varchar, size in bytes of a single value in fetch buffer
    // (prior to compaction).
    size_t inflatedVarcharContentSize()
    {
      return varcharContentSize(ISlength);
    }

    // Calculate size to allocate for strData.
    size_t strDataMemNeeded(Int64 rows);

    // Calculate tha average actual varchar size for the stats
    // collected on the current run.
    NABoolean computeAvgVarCharSize() const
    {
      if ( (colCount == 1)  AND
           (DFS2REC::isAnyVarChar(colSet[0].datatype)) )
        return TRUE;
      else
        return FALSE;
    }

    void print();                                  /* DEBUG: print all groups */

    HSColGroupStruct();
    ~HSColGroupStruct();
    NABoolean allocateISMemory(Int64 rows, NABoolean allocStrData = TRUE,
                               NABoolean recalcMemNeeded = FALSE);
    void freeISMemory(NABoolean freeStrData = TRUE, NABoolean freeMCData=TRUE);
    NAString generateTextForColumnCast();

    // Returned value is the number of bytes needed to represent a single varchar
    // value of the given length. The len parameter could be the declared length
    // of a varchar column, or if varchars are being compacted, the estimated
    // average actual length, or the actual length of a specific compacted varchar.
    // To this we add the size of the length field, and a byte if necessary for the
    // proper alignment of the Int16 length field.
    static inline size_t varcharContentSize(Lng32 len)
    {
      return len                          // declared or avg estimated varchar len
           + (len % 2)                    // possible alignment byte
           + VARCHAR_LEN_FIELD_IN_BYTES;  // size of len field
    }
  };


// This is the primary template for value iterators that assume the values of
// an in-memory column in sequence. This is used for all columns that are
// represented in memory by non-character types. Specializations are defined
// further down for fixed and varying character strings.
template <class T>
class IUSValueIterator
{
  public:
    IUSValueIterator(T* ptr)
      : vp(ptr)
    {}

    virtual ~IUSValueIterator()
    {}
    
    void init(HSColGroupStruct* group);
    
    void next()
    {
      vp++;
    }
    
    T* dataRepPtr() const
    {
      return vp;
    }
    
    T& val() const
    {
      return *vp;
    }
    
    size_t size() const
    {
      return sizeof(T);
    }
    
  private:
    T* vp;
};


// Specialization of iterator template for char types.
template <>
class IUSValueIterator <IUSFixedChar>
{
  public:
    IUSValueIterator(IUSFixedChar* ptr)
      : vp(ptr)
    {}
    
    virtual ~IUSValueIterator()
    {}
    
    void init(HSColGroupStruct* group)
    {
      vp->setContent((char*)group->strData);
    }
    
    void next()
    {
      vp->next();
    }

    char* dataRepPtr() const
    {
      return vp->getContent();
    }
    
    IUSFixedChar& val() const
    {
      return *vp;
    }
    
    size_t size() const
    {
      return vp->getLength();
    }
    
  private:
    IUSFixedChar* vp;
};


// Specialization of iterator template for varchar types.
template <>
class IUSValueIterator <IUSVarChar>
{
  public:
    IUSValueIterator(IUSVarChar* ptr)
      : vp(ptr)
    {}
    
    virtual ~IUSValueIterator()
    {}
    
    void init(HSColGroupStruct* group)
    {
      vp->setContent((char*)group->strData);
    }
    
    void next()
    {
      vp->next();
    }

    char* dataRepPtr() const
    {
      // Point past length field to actual string part.
      return vp->getContent() + VARCHAR_LEN_FIELD_IN_BYTES;
    }
    
    IUSVarChar& val() const
    {
      return *vp;
    }
    
    size_t size() const
    {
      // Actual length of this varchar value.
      return vp->getLength();
    }
    
  private:
    IUSVarChar* vp;
};


// Thrown in allocateISMemory to exit from the series of allocations if one fails.
class ISMemAllocException
{
  public:
    ISMemAllocException() {}
};

// -----------------------------------------------------------------------------
// CLASS: HSGlobalsClass
// -----------------------------------------------------------------------------
class HSGlobalsClass : public NABasicObject
  {
    // Following function requires access to groupListFromTable().
    friend Lng32 AddExistingColumns();
public:
    // parser errors
    enum { ERROR_NONE = 0, ERROR_SYNTAX, ERROR_SEMANTICS};

    // Set CQDs controlling min/max HBase cache size to minimize risk of
    // scanner timeout.
    NABoolean setHBaseCacheSize(double sampleRatio);

    // Set CQD HIVE_MAX_STRING_LENGTH_IN_BYTES if necessary
    NABoolean setHiveMaxStringLengthInBytes(void);

    // Reset any CQDs set above
    void resetCQDs(void);

    // Static fns for determining minimum table sizes for sampling, and for
    // using lowest sampling rate, under default sampling protocol.
    static Int64 getMinRowCountForSample();
    static Int64 getMinRowCountForLowSample();

    // Used by IUS for in-memory tables, and by internal Sort.
    static void getMemoryRequirements(HSColGroupStruct* group, Int64 rows);
    static void getMemoryRequirementsForOneGroup(HSColGroupStruct* group, Int64 rows);

    // used by internal sort for MC to compute MC memory requirements
    void getMCMemoryRequirements(HSColGroupStruct* group, Int64 rows);
    void getMemoryRequirementsForOneMCGroup(HSColGroupStruct* group, Int64 rows);

    static Int32 allocateMemoryForColumns(HSColGroupStruct* group, Int64 rows, HSColGroupStruct* mgr = NULL /* used for MC IS */);
    static Int32 allocateMemoryForIUSColumns(HSColGroupStruct* group, Int64 rows,
                                             HSColGroupStruct* delGroup, Int64 delRows,
                                             HSColGroupStruct* insGroup, Int64 insRows);

    // For internal sort or IUS, remove and count nulls for each column from the
    // rowset just read.
    static Lng32 processInternalSortNulls(Lng32 rowsRead, HSColGroupStruct* firstGroup);

    // Default name of Hive catalog, from cqd HIVE_CATALOG.
    static THREAD_P NAString* defaultHiveCatName;

    // See if catName is the name of a Hive catalog.
    static NABoolean isHiveCat(const NAString& catName)
    {
      return (((defaultHiveCatName != NULL) && (catName == (*defaultHiveCatName))) ||
              catName == HIVE_SYSTEM_CATALOG);
    }

    // Default name of Hbase catalog, from cqd SEABASE_CATALOG.
    static THREAD_P NAString* defaultHbaseCatName;

    // See if catName is the name of an HBase catalog.
    static NABoolean isHbaseCat(const NAString& catName)
    {
      return ((catName == TRAFODION_SYSCAT_LIT) || isNativeHbaseCat(catName));
    }

    static NABoolean isNativeHbaseCat(const NAString& catName)
    {
      return (((defaultHbaseCatName != NULL) && (catName == (*defaultHbaseCatName))) ||
              (catName == HBASE_SYSTEM_CATALOG));
    }

    static NABoolean isNativeCat(const NAString& catName)
    {
      return (isNativeHbaseCat(catName) || isHiveCat(catName));
    }

    static NABoolean isTrafodionCatalog(const NAString& catName)
    { 
      return (catName == TRAFODION_SYSCAT_LIT); 
    }

    static NABoolean isHBaseUMDHistogram(const NAString& tableName)
    { return (tableName == HBASE_HIST_NAME || 
              tableName == HBASE_HISTINT_NAME); }

    static void resetJitLogThresholdHash () { jitLogThresholdHash = NULL; }

    HSGlobalsClass(ComDiagsArea &diags);
    ~HSGlobalsClass();

    // Intialize stats schema on demand
    Lng32 InitializeStatsSchema();

    //Process USTAT options
    Lng32 Initialize();

    //Checks privileges
    NABoolean isAuthorized(NABoolean isShowStats);

    //Based on USTAT options used, it may not be necessary
    // to collect statistics. This method will tell you if
    // they are needed or not.
    inline NABoolean StatsNeeded() const {return statsNeeded_;}

    //Determines histograms for Single-Column groups
    Lng32 CollectStatistics();

    //Determines histograms for Single-Column groups using Hive backing sample
    // and fast-stats algorithm with CBFs.
    Lng32 CollectStatisticsWithFastStats();

    // Select the next set of columns to process with faststats.
    CollIndex selectFastStatsBatch(HSColGroupStruct** colGroups);

    // Process columns marked PENDING with faststats.
    Lng32 processFastStatsBatch(CollIndex numCols, HSColGroupStruct** colGroups);

    //Update histogram tables with newly generated statistics
    Lng32 FlushStatistics(NABoolean &statsWritten);

    //Drive the gathering and printing of generated statistics
    Lng32 GetStatistics(NAString& outStr, Space& space);

    //Reverse the column list to fix the order
    HSColGroupStruct* ReverseList(HSColGroupStruct* list);

    // Make adjustments to the interval count before creating histograms
    Lng32 getAdjustedIntervalCount(HSColGroupStruct *group,
                                  Lng32 intCount,
                                  Int64 rowCount,
                                  Lng32 rowsetSize,
                                  NABoolean &singleIntervalPerUec,
                                  Lng32 &gapIntCount,
                                  Lng32 &highFreqIntCount);

    //Add specified group to the singleGroup or multiGroup list as appropriate.
    void addGroup(HSColGroupStruct *group);

    // Remove a single group.
    void removeGroup(HSColGroupStruct* groupToRemove);

    // Remove the most recently added groups.
    NABoolean removeGroups(Lng32 numGroupsToRemove,
                           HSColGroupStruct* oldSingle,
                           HSColGroupStruct* oldMulti);

    //Locate single-column group that mathes colnum
    HSColGroupStruct* findGroup(const Lng32 colnum);
    HSColGroupStruct* findGroupAndPos(const Lng32 colnum, Int32 &pos);

    //Locate group that matches given group
    HSColGroupStruct* findGroup(const HSColGroupStruct *tableGroup);

    // check if all MCs have been computed and processed
    NABoolean allMCGroupsProcessed(NABoolean forIS=FALSE);

    //Return TRUE if 'entry' is a duplicate entry in 'list'.
    NABoolean findDuplicate(const HSColGroupStruct *entry,
                                  HSColGroupStruct *list);

    //Delete histograms in list from HISTOGRAMS and HISTOGRAM_INTERVALS tables.
    Lng32 removeHists(NAString &hists, char *uid, const char *operation);

    //Log the current contents of this class.
    void log(HSLogMan* LM);

    // Takes action necessary before throwing exception for an assertion failure.
    void preAssertionFailure(const char* condition, const char* fileName, Lng32 lineNum);

    // Derive a return code from the contents of the diagnostics area.
    Lng32 getRetcodeFromDiags();

    NABoolean canDoIUS() 
    { return okToPerformIUS() && wherePredicateSpecifiedForIUS(); };

    NABoolean okToPerformIUS();
    NABoolean useIUSForHistograms();
    NABoolean wherePredicateSpecifiedForIUS();
    NAString& getWherePredicateForIUS();
    Lng32 validateIUSWhereClause();
    NABoolean getPersistentSampleTableForIUS(NAString& tableName, 
                                             Int64 &requestedRows, 
                                             Int64 &sampleRows, 
                                             double &sampleRate,
                                             NABoolean forceToFetch = TRUE);
    Lng32 updatePersistentSampleTableForIUS(NAString& sampleTableName, double sampleRate,
                                            NAString& targetTableName);
    void generateIUSDeleteQuery(const NAString& smplTable, NAString& queryText, NABoolean transactional);
    void generateIUSSelectInsertQuery(const NAString& smplTable,
                                      const NAString& sourceTable,
                                      NAString& queryText);
    void getCBFFilePrefix(NAString& sampleTableName, NAString& filePrefix);
    void detectPersistentCBFsForIUS(NAString& sampleTableName, HSColGroupStruct *group);
    Lng32 UpdateIUSPersistentSampleTable(Int64 oldSampleSize, Int64 requestedSampleSize, Int64& newSampleSize);
    Lng32 readCBFsIntoMemForIUS(NAString& sampleTableName, HSColGroupStruct* group);
    Lng32 writeCBFstoDiskForIUS(NAString& sampleTableName, HSColGroupStruct* group);
    Lng32 deletePersistentCBFsForIUS(NAString& sampleTableName, HSColGroupStruct* group, SortState stateToDelete);

    void logDiagArea(const char* title);

    Lng32 begin_IUS_work();
    Lng32 end_IUS_work();

    // Populate the hash table used to determine when a ustat statement has run
    // too long and needs to have logging enabled.
    static void initJITLogData();

    // Get the JIT logging time threshold currently in effect.
    double getJitLogThreshold() const
      {
        return jitLogThreshold;
      }
    
    // Look up the source table being operated on and find its max elapsed time
    // before logging should be activated.
    void setJitLogThreshold()
      {
         double* thresholdPtr = jitLogThresholdHash->getFirstValue(user_table);
         jitLogThreshold = (thresholdPtr ? *thresholdPtr : 0);
      }

    // Get the overall start time for the current ustat statement (in seconds
    // since epoch).
    Int64 getStmtStartTime() const
      {
        return stmtStartTime;
      }

    // Set the overall start time for the current ustat statement (in seconds
    // since epoch). At certain points this will be compared to the current
    // time to see how long the statement has been executing.
    void setStmtStartTime(Int64 time)
      {
        stmtStartTime = time;
      }

    // Compare the elapsed time so far for the ustat statement, and activate
    // logging if it exceeds the threshold currently in effect. If no threshold
    // has been established for the source table, stmtStartTime will be 0 and
    // logging will not be activated regardless of how long we've been running.
    void checkTime(const char* checkPointName)
      {
        if (!jitLogOn && 
            stmtStartTime > 0 &&
            hs_getEpochTime() - stmtStartTime > jitLogThreshold)
          {
            startJitLogging(checkPointName, hs_getEpochTime() - stmtStartTime);
          }
      }

    // Dynamically turn on logging in response to a statement that has been running
    // far longer than expected.
    void startJitLogging(const char* checkPointName, Int64 elapsedSeconds);

    
    static void setPerformISForMC(NABoolean x) { performISForMC_ = x; }
    static NABoolean performISForMC() { return performISForMC_; }

                                              /*==============================*/
                                              /*     OBJECT INFORMATION       */
                                              /*==============================*/
    HSTableDef    *objDef;                         /* object definition       */
    NAString      *catSch;                         /* catalog+schema name     */
    NAString      *user_table;                     /* object name             */
    NABoolean     isHbaseTable;                    /* ustat on HBase table    */
    NABoolean     isHiveTable;                     /* ustat on Hive table     */
    NABoolean     hasOversizedColumns;             /* set to TRUE for tables  */
                                                   /* having gigantic columns */
    ComAnsiNameSpace nameSpace;                    /* object namespace    ++MV*/
    Int64          numPartitions;                  /* # of partns in object   */
    NAString      *hstogram_table;                 /* HISTOGRM table          */
    NAString      *hsintval_table;                 /* HISTINTS table          */
    NAString      *hsperssamp_table;               /* PERSISTENT_SAMPLES table */
    NAString      *hssample_table;                 /* SAMPLING table          */
    NABoolean      externalSampleTable;            /* ownership of sample tab */
    hs_table_type  tableType;                      /* GUARDIAN | ANSI format  */
    ComDiskFileFormat tableFormat;                 /* SQL/MP | SQL/MX table   */

                                              /*==============================*/
                                              /*    HISTOGRAM INFORMATION     */
                                              /*==============================*/
    NAString      *statstime;                      /* time of execution       */
    ULng32  statsTimeInt;                   /* time of execution       */
    Int64          actualRowCount;                 /* actual #rows            */
    Int64          sampleRowCount;                 /* sampled #rows           */
    Int64          rowChangeCount;                 /* rows IUD since last reset */
    HSColGroupStruct *dupGroup;                    /* list of duplicate hists */
    Int64          minRowCtPerPartition_;          /* minimal rows per partition */

                                              /*==============================*/
                                              /*  SYNTAX OPTION INFORMATION   */
                                              /*==============================*/
    Lng32           optFlags;                       /* syntax option flags     */
    Lng32        intCount;                       /* #intervals              */
    Int64          sampleValue1;                   /* sample option: value1   */
    Int64          sampleValue2;                   /* sample option: value2   */
    double         sampleTblPercent;               /* the sample % to use     */
    NABoolean      sampleOptionUsed;               /* SAMPLE specified        */
    NAString      *sampleOption;                   /* SAMPLE option used      */
    NABoolean      sampleTableUsed;                /* sample table created    */
    NABoolean      samplingUsed;                   /* sample (w/wo sample tbl)*/
    NABoolean      unpartitionedSample;            /* sample tbl not partitned*/
    NABoolean      isUpdatestatsStmt;              /* is update stats command */
    Lng32           groupCount;                     /* total #column groups    */
    Lng32           singleGroupCount;               /* #single-column groups   */
    HSColGroupStruct *singleGroup;                 /* single-column group list*/
    HSColGroupStruct *multiGroup;                  /* multi-column group list */

                                              /*==============================*/
                                              /*  ERROR HANDLING INFORMATION  */
                                              /*==============================*/
    Lng32           parserError;                    /* SYNTAX | SEMANTIC       */
    Lng32           errorCount;                     /* total #errors found     */
    NAString       errorFile;                      /* file in error           */
    Lng32           errorLine;                      /* file location of error  */
    ComDiagsArea  &diagsArea;                      /* diagnostic area         */
    
                                              /*==============================*/
                                              /*  AUTOMATION INFORMATION      */
                                              /*==============================*/
    static THREAD_P COM_VERSION    schemaVersion;           /* metadata version                                */
    static THREAD_P Lng32           autoInterval;            /* automation interval.  If 0, it is disabled.     */
    Int64                 sampleSeconds;           /* time to create sample table. 0 if no sampling   */
    Int64                 columnSeconds;           /* average time to read a column into memory       */
                                                   /* for internal sort                               */
    short                 samplePercentX100;       /* sampling percent to create sample table * 100   */
    NABoolean             allMissingStats;         /* TRUE if all hists to create are missing stats.  */

                                              /*==============================*/
                                              /*  OTHER INFORMATION           */
                                              /*==============================*/
    NABoolean             requestedByCompiler;     /* TRUE if ustats called by compiler. */

    double                sampleRateAsPercetageForIUS; /* sample rate in percentage 
                                                          for one instance of persistent 
                                                          sample table */

    NABoolean            sample_I_generated;

    Lng32          maxCharColumnLengthInBytes;   /* the value of USTAT_MAX_CHAR_COL_LENGTH_IN_BYTES */

    // Error recovery flags so we can reset CQDs that we set
    // during CollectStatistics() (We do this because the
    // HSHandleError macro commonly used makes it hard to
    // do the resets reliably in CollectStatistics itself. Sigh.)

    NABoolean hbaseCacheSizeCQDsSet_;
    NABoolean hiveMaxStringLengthCQDSet_;

private:
    //++ MV
    // special parser flags (see contr. and destr.)
    enum { dmALLOW_SPECIALTABLETYPE  = 0x1, dmALLOW_PHONYCHARACTERS = 0x2, dmINTERNAL_QUERY_FROM_EXEUTIL = 0x20000};
    ULng32               savedParserFlags;

    //Generated unique histogram IDs for all groups
    Lng32 MakeAllHistid();
	
    //Builds group list from HISTOGRAMS table
    Lng32 groupListFromTable(HSColGroupStruct*& groupList,
                             NABoolean skipEmpty=FALSE,
                             NABoolean exclusive=FALSE); // do we need exclusive locks on the accessed rows

    //Computes Multi-Column statistics, based on Single-Column statistics
    Lng32 ComputeMCStatistics(NABoolean usingIS=FALSE /* try using IS to compute MCs */);

    //Calculate final ROWCOUNT and UEC due to sampling
    Lng32 FixSamplingCounts(HSColGroupStruct *group);

    //Clear all histograms based on object_uid
    Lng32 ClearAllHistograms();

    //Clear selected histograms based on object_uid and hist_id
    Lng32 ClearSelectHistograms();

    //Delete all orphan histograms for SQL/MP tables.
    Lng32 DeleteOrphanHistograms();

    //Insert new statistics + Delete old statistics
    Lng32 WriteStatistics();

    //Gather and create output string for generated histograms
    Lng32 DisplayHistograms(NAString& displayData, Space& space,const ULng32 oldHistId, const char* colnames);

    //Internal sort functions.
    //
    // When performing internal sort, determines the amount of memory required
    // for each column that will be read into memory.
    Int64 getInternalSortMemoryRequirements(NABoolean performISForMC);

    // Get maximum amount of memory to use for internal sort.
    Int64 getMaxMemory();

    // re-order multi-column and single-column groups to maximize the number 
    // of multi-column group stats that can be done in memory 
    NABoolean orderMCGroupsNeeded();
    void orderMCGroups (HSColGroupStruct* s_group_back[]);
    
    // helper functions for orderMCGroups
    void computeMCGroupsWeight();
    void computeSingleUsedCols();
    void reorderMCGroupsByWeight();
    void formGroupSets();
    void reorderSingleGroupsByWeight (HSColGroupStruct* s_group_back[], Int32 colsOrder[], Int32 &headGroupCols);
    void freeMCISmemory(HSColGroupStruct* s_group_back[], Int32 colsOrder[], Int32 &headGroupCols);
    void reArrangeMCGroups();

    // Select a set of columns that will fit in available memory so they can
    // be sorted internally.
    Int32 selectSortBatch(Int64 rows, NABoolean ISonlyWhenBetter,
                        NABoolean trySampleInMemory);

    // Select a set of columns that can be IUS updated in memory in one batch.
    // 'curentRows' is the number of rows currently in the sample table,
    // 'futureRows' is the number of rows to be populated in sample table 
    // after IUS, 'ranOut' set to TRUE when no enough memory to perform
    // any IUS, and 'colsSelected' indicates # of columns selected for
    // IUS in this batch.
    Lng32 selectIUSBatch(Int64 currentRows, Int64 futureRows,NABoolean& ranOut, Int32& colsSelected);

    // Determine if all groups (both single and MC) can fit in memory for internal sort.
    // No space is actually allocated and no state is set for each group.
    NABoolean allGroupsFitInMemory(Int64 rows);

    // Determine the next batch of columns to be processed with internal sort
    // by calling selectSortBatch() and ensuring that adequate memory can be
    // allocated for those columns.
    Int32 getColsToProcess(Int64 rows,
                         NABoolean internalSortWhenBetter,
                         NABoolean trySampleTableBypass = FALSE);

    // If we decide to create and load a sample table, deallocate column memory
    // and reset PENDING group states back to UNPROCESSED before creating and
    // loading the sample table. We'll call getColsToProcess to reallocate it
    // again afterwards.
    void deallocatePendingMemory(void);

    // After an allocation failure, this is called to reduce the amount of
    // memory we estimate is available.
    static void memReduceAllowance();

    // When a memory allocation fails, return any memory already allocated for
    // the group for internal sort, and set any PENDING columns back to
    // UNPROCESSED state.  This function cannot fail.
    static void memRecover(HSColGroupStruct* group, NABoolean firstFailed, Int64 rows, 
                           HSColGroupStruct* mgroup);

    // Allocate memory for the columns selected for an internal sort batch.
    //Int32 allocateMemoryForColumns(Int64 rows);
    Int32 allocateMemoryForInternalSortColumns(Int64 rows);

    Lng32 prepareToReadColumnsIntoMem(HSCursor *cursor, Int64 rows);

    // Reads all values for selected columns into memory, where they can be
    // sorted and then grouped into intervals.
    Lng32 readColumnsIntoMem(HSCursor *cursor, Int64 maxRows);

    // Iterates through group list for single columns, and calls sorting
    // routine for each column marked as PENDING.
    Lng32 sortByColInMem();

    // Creates histograms for columns once they are sorted.
    Lng32 createStats(Int64 rowsAllocated);

    // Creates histograms for the columns specified in group.
    Lng32 createStatsForColumn(HSColGroupStruct* group, Int64 rowsAllocated);


    // Collect statistics by incrementally updating persistent sample table and
    // possibly histograms as well.
    Lng32 doIUS(NABoolean& done);

    // Collect stats by incrementally updating histograms where possible. Persistent
    // sample is also incrementally updated.
    Lng32 doFullIUS(Int64 currentSampleSize, Int64 futureSampleSize, NABoolean& done);

    // Causes persistent sample table to be incrementally updated, and other
    // preparatory tasks so RUS can be performed using persistent sample.
    Lng32 prepareToUsePersistentSample (Int64 currentSampleSize, Int64 futureSampleSize);

    // Incrementally update histograms for a selected batch of columns
    Lng32 CollectStatisticsForIUS(Int64 currentSampleSize, Int64 futureSampleSize);

    //
    // Prepare for IUS. This method implements the 1st algorithm which 
    // does not requre persistent CBFs. It performs the following:
    // 1. Check the existentce of the persistable table S
    // 2. Update the sample table with S-D and S-D+I
    // 3. Optionally trim the final sample table to the same size as before. 
    Lng32 computeSampleSizeForIUS(Int64& currentSampleSize, Int64& futureSampleSize);
    void setMemoryRequirementForIUS(HSColGroupStruct *group, Int64 futureSampleSize);

    Lng32 prepareForIUSAlgorithm1(Int64& rows /* # of rows in the sample table */);

    // Generate the incremental sample (aka sample set I)
    Lng32 generateSampleI(Int64 currentSampleSize, Int64 futureSampleSize);

    Lng32 moreColsForIUS();

    // Use In-memory tables to update histograms incrementally.
    Lng32 incrementHistograms();

    Lng32 initIUSIntervals(HSColGroupStruct* group,
                           HSColGroupStruct* delGroup,
                           HSColGroupStruct* insGroup,
                           UInt32 histID,
                           Int16 numIntervals);

    Int32 processIUSColumn(HSColGroupStruct* smplGroup,
                           HSColGroupStruct* delGroup,
                           HSColGroupStruct* insGroup);


    NABoolean statsNeeded_;    /* statistics are needed   */
    UstatContextID contID_;    /* context ID              */
    static THREAD_P float ISMemPercentage_;    /* % of available physical memory to use for internal sort */
    NABoolean currentRowCountIsEstimate_;          /* Row count est flag      */

    //HSInMemoryTable* iusSampleInMem;
    HSInMemoryTable* iusSampleDeletedInMem;
    HSInMemoryTable* iusSampleInsertedInMem;

    // used by IUS code for clean up purposes
    NABoolean sampleIExists_;

    // For IUS, once the persistent sample table has been successfully updated
    // in accordance with the IUS predicate, these ptrs will point to the requested
    // (expected) and actual number of rows in the sample table. end_IUS_work will
    // pass these ptrs to the function that updates the sample table's row in
    // SB_PERSISTENT_SAMPLES. If non-null, the values are used for the corresponding
    // columns in that table.
    Int64* PST_IUSrequestedSampleRows_;
    Int64* PST_IUSactualSampleRows_;

    template <class T>
    Int32 processIUSColumn(T* ptr,
                           const NAWchar* format,
                           HSColGroupStruct* smplGroup,
                           HSColGroupStruct* delGroup,
                           HSColGroupStruct* insGroup);

    // This function is used by convertBoundaryOrMFVValue() for types that can't
    // be handled by a simple call to na_swscanf().
    template <class T>
    T convertToISdatatype(T*,
                          const HSDataBuffer& valToConvert,
                          HSColGroupStruct* group);

    // Template for converting the value in an HSDataBuffer (used for interval
    // boundary and MFV values) to any non-char type. The converted value goes
    // in element 'index' of the array 'convertedValues'.
    template <class T>
    void convertBoundaryOrMFVValue(const HSDataBuffer& valToConvert,
                                   HSColGroupStruct* group,
                                   Int32 index,
                                   T* convertedValues,
                                   const NAWchar* format)
    {
      // Can just use na_swscanf() unless the column's in-memory type was mapped
      // from its original type, or is a fixed numeric with nonzero scale.
      Int32 actualDatatype = group->colSet[0].datatype;
      if (group->ISdatatype != actualDatatype ||
          (actualDatatype >= REC_MIN_BINARY_NUMERIC && actualDatatype <= REC_MAX_BINARY_NUMERIC
                                            && group->colSet[0].scale > 0))
        convertedValues[index] = convertToISdatatype((T*)NULL, valToConvert, group);
      else
        na_swscanf((const NAWchar*)valToConvert.data(), format, convertedValues+index);
    }

    // Template specialization for converting value in an HSDataBuffer to an
    // instance of IUSFixedChar.
    void convertBoundaryOrMFVValue(const HSDataBuffer& valToConvert,
                                   HSColGroupStruct* group,
                                   Int32 index,
                                   IUSFixedChar* convertedValues,
                                   const NAWchar* format)
    {
      convertedValues[index] = valToConvert;
    }

    // Template specialization for converting value in an HSDataBuffer to an
    // instance of IUSVarChar.
    void convertBoundaryOrMFVValue(const HSDataBuffer& valToConvert,
                                   HSColGroupStruct* group,
                                   Int32 index,
                                   IUSVarChar* convertedValues,
                                   const NAWchar* format)
    {
      convertedValues[index] = valToConvert;
    }

    double computeAvgCharLengthForIUS(HSColGroupStruct* group,
                                      HSColGroupStruct* delGroup,
                                      HSColGroupStruct* insGroup);

    Int32 estimateAndTestIUSStats(HSColGroupStruct* smplGroup,
                                  HSColGroupStruct* delGroup,
                                  HSColGroupStruct* insGroup,
                                  HSHistogram* hist,
                                  CountingBloomFilter* cbf,
                                  Lng32 numNonNullIntervals,
                                  double scaleFactor,
                                  Int32 nullCount,
                                  Int64* intvlRC);

    Lng32 mergeDatasetsForIUS();

    Lng32 mergeDatasetsForIUS(
                       HSColGroupStruct* smplGroup, Int64 smplrows,
                       HSColGroupStruct* delGroup, Int64 delrows,
                       HSColGroupStruct* insGroup, Int64 insrows);

    template <class T_IUS, class T_IS>
    Int32 mergeDatasetsForIUS(T_IUS* ptr, T_IS* dummyPtr,
                       HSColGroupStruct* smplGroup, Int64 smplrows,
                       HSColGroupStruct* delGroup, Int64 delrows,
                       HSColGroupStruct* insGroup, Int64 insrows);

    template <class T>
    class HSHiLowValues
      {
        public:

          NABoolean seenAtLeastOneValue_;  // initially FALSE
          // the next two are valid only if seenAtLeastOneValue_ is TRUE
          T hiValue_;  // highest value seen so far
          T lowValue_; // lowest value seen so far

          HSHiLowValues() : seenAtLeastOneValue_(FALSE) { };

          void findHiLowValues(T& val)
            {
              if (seenAtLeastOneValue_)
                {
                  if (val < lowValue_)
                    lowValue_ = val;
                  else if (val > hiValue_)
                    hiValue_ = val;
                }
              else
                {
                  seenAtLeastOneValue_ = TRUE;
                  lowValue_ = val;
                  hiValue_ = val;
                } 
            };
      };

    template <class T>
    Int16 findInterval(Int16 numInt, T* boundaries, T& val)
      {
        Int16 low = 1;
        Int16 high = numInt;
        Int16 current;
        //@ZX need to check special case of single interval
        while (high > low+1)
          {
            current = low + ((high - low) / 2);
            if (val <= boundaries[current])
              high = current;
            else
              low = current;
          }

        if (val <= boundaries[low])
          return low;
        else
          return high;
      }

    Int32 logCBF(const char*, CountingBloomFilter* cbf);

    // Hash table mapping table names to the elapsed time thresholds for
    // activating just-in-time logging. This is used to capture log info for
    // Ustat statements running long past their expected execution time.
    // The hash table is a static member so we can set it up once and reuse
    // it for any subsequent ustat stmt.
    static THREAD_P JitLogHashType* jitLogThresholdHash;
    double jitLogThreshold;
    Int64 stmtStartTime;
    NABoolean jitLogOn;

    // For IUS, was the SB_PERSISTENT_SAMPLES row for the source table updated?
    // The change is undone by the HSGlobalsClass dtor, so we need to account for
    // the possibility that an IUS statement failed prior to making the change.
    // Otherwise, a concurrent IUS operation could have its changes to the row
    // overwritten.
    NABoolean PSRowUpdated;

    static THREAD_P NABoolean performISForMC_;

  };  // class HSGlobalsClass

// -----------------------------------------------------------------------
// Column descriptor to store column info returned from CLI.
// -----------------------------------------------------------------------
struct HSColDesc : public NABasicObject
{
  Lng32 datatype;
  Lng32 length;
  Lng32 precision;
  Lng32 scale;
  Lng32 nullflag;

  Lng32 dataOffset;
  Lng32 indDataOffset;
  char *data;
  char *indData;

  Lng32 groupNum;
  NABoolean isSingleColGroup;

  HSColDesc()
    : data(NULL), indData(NULL),
      isSingleColGroup(FALSE)
  {}

  inline NABoolean isNull(const char *dataBuf) const
  {
    return (nullflag &&
           (dataBuf[indDataOffset] == (char)0xFF));
  }

  inline NABoolean isNull() const
  {
    return (nullflag &&
           (*indData == (char)0xFF));
  }

  // only if datatype == REC_BYTE_V_ASCII.
  inline Int32 varcharLen(const char *dataBuf) const
  {
    short len;
    memcpy((char *)&len, &dataBuf[dataOffset], VARCHAR_LEN_FIELD_IN_BYTES);
    return (Int32)len;
  }

  inline Int32 varcharLen() const
  {
    short len;
    memcpy((char *)&len, data, VARCHAR_LEN_FIELD_IN_BYTES);
    return (Int32)len;
  }

  inline void rebase(const Lng32 base)
  {
    dataOffset -= base;
    indDataOffset -= base;
  }
};

// Constants used by FrequencyCounts: the size of the
// hash table, a prime number, and the number of f_i
// values stored explicitly in a dense array. Can't use
// static const ints for these, because they are used as
// bounds in array declarations.
#define FC_NUM_HT_BUCKETS 389
#define FC_NUM_STORED_VALUES 1024

// 
// Class to maintain frequency counts (f_i) of a set of
// values, used for estimating UECs from a sample.  f_1 
// is the number of values that occur exactly one time in
// a sample, f_2 the number of values that occur exactly 2
// times, and so on.
// Note: Normally, this class would be a 'public NABasicObject'.
//       However, we need an array of these objects on the heap
//       and the following do not work when it is an NABasicObject:
//         - FrequencyCounts *arr = new FrequencyCounts[x];
//           delete [] arr;
//         - FrequencyCounts *arr = new (STMTHEAP) FrequencyCounts[x];
//           NADELETEARRAY(arr, x, FrequencyCounts, STMTHEAP);
//
//       With the form it is, we can use the standard C++ method
//       of alloc/dealloc (the MX STMTHEAP method does not work).
//
class FrequencyCounts
{
 public:
  FrequencyCounts();
  ~FrequencyCounts();

  // Copy assignment is used when an interval is copied while removing
  // undersized gap intervals.
  FrequencyCounts& operator=(const FrequencyCounts& rhs);

  // reset all the frequency counts to 0
  void reset();

  // increment f_i by value specified (default 1). 
  void increment(Int64 i, ULng32 val=1);

  // return f_i
  ULng32 operator[](Int64 i);

  // merge frequency counts into specified object (i.e., f)
  void mergeTo(FrequencyCounts &f);

 private:
  // Copy constructor is left undefined.
  FrequencyCounts(const FrequencyCounts& other);

  // for i in the range 1..(FC_NUM_STORED_VALUES-1), f_i values are 
  // stored in array fiArr_.  the value of f_i is fiArr_[i].
  // for i >= FC_NUM_STORED_VALUES, nonzero i and f_i values are 
  // stored in hash table bigfiHT_.
  // hash table entry
  struct entry 
  {
    ULng32 ix_;
    ULng32 value_;
    struct entry *next_;
  };

  // helper methods
  //
  void resetHT();
  void incrementHT(ULng32 ix, ULng32 val);
  ULng32 lookupHT(ULng32 ix);
  struct entry *newEntry(ULng32 ix, ULng32 value);
  struct entry *hashToBucket(ULng32 ix);

  // array of fi values and hash table
  ULng32 fiArr_[FC_NUM_STORED_VALUES];
  struct entry bigfiHT_[FC_NUM_HT_BUCKETS];
};

class HSInterval
  {
public:
    HSInterval();
    ~HSInterval();

    Int64 rowCount_;
    Int64 uecCount_;
    HSDataBuffer boundary_;
    Int64 MFVrowCount_; // stores Most Frequent Value frequency (rowcount)
    Int64 MFV2rowCount_; // second Most Frequent Value frequency (rowcount)
    HSDataBuffer mostFreqVal_; // stores Most Frequent Value
    double gapMagnitude_;  // leave as 0 for non-gap intervals
    NABoolean highFreq_;   // if TRUE, an interval for a high-frequency value
    double squareCntSum_;  // the summation of the square of all value counts
                           // squaredCntSum_ is used to calculate skew for 
                           // sampling UEC estimation and std dev of freq.
    Int64 origUec_; // to save original interval UEC, needed to compute stdev
    Int64 origRC_;  // to save original interval RC, needed to scale MFV properly
    Int64 origMFV_;  // to save original interval MFV, needed to scale MFV properly
  };

// The GapKeeper class tracks the n largest gaps as they are discovered.
// It maintains a sorted array of gap magnitudes, and provides a function
// to insert a new gap if it is among the largest.
class GapKeeper
{
  public:
    GapKeeper(Int32 gapsToKeep);
    ~GapKeeper();
    NABoolean insert(double gap);
    double smallest();
    Int32 qualifyingGaps(double minAcceptableGap);

  private:
    // Copy ctor and assignment not used.
    GapKeeper(const GapKeeper&);
    GapKeeper& operator=(const GapKeeper&);

    Int32 gapsToKeep_;
    double *gaps_;
};


class HSHistogram : public NABasicObject
  {
public:
    HSHistogram(Lng32 intcount, Int64 rowcount, Lng32 gapIntervals, Lng32 highFreqIntervals,
                NABoolean sampleUsed = FALSE,
                NABoolean singleIntervalPerUec = FALSE);
    ~HSHistogram();

    void deleteFiArray();
    Lng32 processIntervalValues(boundarySet<myVarChar>* boundaryRowSet,
                               HSColGroupStruct* group,
                               Int64 &rowsInSet,
                               double currentGapAvg);

    Lng32 updateMCInterval(const HSDataBuffer &lowval,
                          const HSDataBuffer &hival);
                       
    void addNullInterval(const Int64 nullCount, const Lng32 colCount);
    // The value returned by getNumIntervals does not include the 0th interval,
    // which is used only to store the minimum value.
    inline Lng32 getNumIntervals() const {return currentInt_;}
    inline NABoolean hasNullInterval() const {return hasNull_;}
    void getOrigTotalCounts(Int64 &rowCount, Int64 &uecCount);
    void getTotalCounts(Int64 &rowCount, Int64 &uecCount);
    Int64 getTotalUec();
    Int64 getTotalRowCount();
    Lng32 getLowValue(HSDataBuffer &lval, NABoolean addParen=TRUE);
    Lng32 getHighValue(HSDataBuffer &hval, NABoolean addParen=TRUE);
    Int64 getHighFreqThreshold()
      { return highFreqThreshold_; }

    inline Int64 getIntRowCount(const Lng32 intNum) const {return intArry_[intNum].rowCount_;}
    inline Int64 getIntUec(const Lng32 intNum) const {return intArry_[intNum].uecCount_;}
    inline double getIntSquareSum(const Lng32 intNum) const {return intArry_[intNum].squareCntSum_;}
    inline Int64 getIntOrigUec(const Lng32 intNum) const {return intArry_[intNum].origUec_;}
    inline Int64 getIntOrigRC(const Lng32 intNum) const {return intArry_[intNum].origRC_;}
    inline Int64 getIntMFVRowCount(const Lng32 intNum) const {return intArry_[intNum].MFVrowCount_;}
    inline Int64 getIntMFV2RowCount(const Lng32 intNum) const {return intArry_[intNum].MFV2rowCount_;}
    inline Int64 getIntOrigMFV(const Lng32 intNum) const {return intArry_[intNum].origMFV_;}

    void setIntRowCount(const Lng32 intNum, const Int64 value) { intArry_[intNum].rowCount_ = value; }
    void addIntRowCount(const Lng32 intNum, const Int64 value) { intArry_[intNum].rowCount_ += value; }
    void setIntOrigUec(const Lng32 intNum, const Int64 value) { intArry_[intNum].origUec_ = value; }
    void setIntOrigRC(const Lng32 intNum, const Int64 value) { intArry_[intNum].origRC_ = value; }
    void setIntMFVRowCount(const Lng32 intNum, const Int64 value) { intArry_[intNum].MFVrowCount_ = value; }
    void setIntMFV2RowCount(const Lng32 intNum, const Int64 value) { intArry_[intNum].MFV2rowCount_ = value; }

    void setIntOrigMFV(const Lng32 intNum, const Int64 value) { intArry_[intNum].origMFV_= value; }

    void setIntUec(const Lng32 intNum, const Int64 value) { intArry_[intNum].uecCount_ = value; }
    Lng32 getParenthesizedIntBoundary(Lng32 intNum, HSDataBuffer &intBoundary);
    const HSDataBuffer& getIntBoundary(Lng32 intNum) { return intArry_[intNum].boundary_; }
    const HSDataBuffer& getIntMFV(Lng32 intNum) { return intArry_[intNum].mostFreqVal_; }
    Lng32 getParenthesizedIntMFV(Lng32 intNum, HSDataBuffer &mostFreqVal);
    FrequencyCounts *fi(const ULng32 intNum) 
      { return fi_ ? &(fi_[intNum]) : 0; }
    void removeLesserGapIntervals(double trueGapAvg);
    double getGapMultiplier()
      { return gapMultiplier_; }
    GapKeeper gapKeeper_;

    // Used by IUS when reading existing histograms from metadata. currentInt_ is
    // the number of intervals actually used (intCount_ is the number available).
    void setCurrentInt(const Lng32 numInts) { currentInt_ = numInts; }
    void setHasNull(NABoolean val) { hasNull_ = val; }
    void setIntBoundary(const Lng32 intNum, const char* value, Int16 len)
      { intArry_[intNum].boundary_.copyFrom(value, len, TRUE); }
    void setIntBoundary(const Lng32 intNum, const HSDataBuffer & newBoundary)
      { intArry_[intNum].boundary_ = newBoundary; }
    void setIntMFVValue(const Lng32 intNum, const char* value, Int16 len)
      { intArry_[intNum].mostFreqVal_.copyFrom(value, len, TRUE); }

    void adjustMFVand2MFV(const Lng32 i, double newEstRow, double newEstUec);
    void setIntSquareSum(const Lng32 intNum, double sum) {intArry_[intNum].squareCntSum_ = sum;}

    void maintainEndIntervalForIUS(float avgRCPerInterval, Lng32 intNum);

    void setMaxStddev(double x) { maxStddev_ = x; };
    double getMaxStddev() { return maxStddev_ ; };

    void logIntervals(Lng32 curr = -1, Lng32 lookahead = -1);
    void logAll(const char* title);


private:
    // Copy ctor and assignment not used.
    HSHistogram(const HSHistogram&);
    HSHistogram& operator=(const HSHistogram&);

    Lng32 mergeInterval(const Lng32 intervalToMerge,
                       const Lng32 prevInterval,
                       const double gapThreshold);
    void mergeMFVs(const Lng32 to, const Lng32 from);

    Lng32            intCount_;       // # of intervals that can be used
    Lng32            maxAllowedInts_; // the total allocated intervals (allows for extras 
                                     // during gap/freq encoding).
    Lng32            currentInt_;     // current interval
    Int64           remRows_;        // remainder rows to spread accross intervals
    Int64           step_;           // MAX data points per interval
    Int64           originalStep_;   // unlike step_, not adjusted after each interval
    HSInterval     *intArry_;        // interval array
    NABoolean       hasNull_;        // NULL bounddary is used
    FrequencyCounts *fi_;            // frequency counts (per interval)
    double          gapMultiplier_;  // Gap avg. times this is "big gap" threshold
    Lng32            gapIntCount_;          // # gap intervals created; not all will be kept
    Lng32            targetGapIntervals_;   // keep this many gap intervals
    Lng32            highFreqIntervalsAllotted_; // # added for high freq values; don't include
                                                //    when calculating step size
    Lng32            highFreqIntervalsUsed_;// # of allotted high frequency intvls actually used
    Int64           highFreqThreshold_;    // row count for a single value beyond which
                                           //   a separate interval is formed
    NABoolean       singleIntervalPerUec_; // flag indicates if this histogram
                                           // will be a 'single interval per
                                           // uec' histogram

    double          maxStddev_;
public:
  // Have to define this function within class definition since it uses a
  // template (Microsoft compiler gives error C2660 when it is invoked if
  // defined in a separate file).
  /***********************************************/
  /* METHOD:  addIntervalData()                  */
  /* PURPOSE: Add the passed value and its row   */
  /*          count to the current interval, or  */
  /*          to a new one if the row count is   */
  /*          too big to fit in the current one. */
  /* PARAMS:  value      - the unique value.     */
  /*          group      - used to construct     */
  /*                       external format string*/
  /*          numRows    - the number of entries */
  /*                       equal to 'value'      */
  /*          bigGap     - if true, indicates a  */
  /*                       gap of sufficient     */
  /*                       size to create an     */
  /*                       interval for it.      */
  /*          gapMagnitude - Size of the gap that*/
  /*                       precedes this value.  */
  /*          final      - indicates that this is*/
  /*                       (or may be, if using  */
  /*                       of query sort/group,  */
  /*                       which reads a rowset  */
  /*                       at a time) the last   */
  /*                       unique value to be    */
  /*                       added.                */
  /* RETCODE:  0 - successful                    */
  /*          -1 - failure                       */
  /* ASSUMPTIONS: The data is SORTED(increasing) */
  /* NOTES:   bndry:   )[----](----]...(----]    */
  /*          int#     0   1     2  ...   n      */
  /***********************************************/
  template <class T>
  Lng32 addIntervalData(T& value,
                       const HSColGroupStruct *group,
                       const Int64 numRows,
                       NABoolean bigGap,
                       double gapMagnitude,
                       NABoolean final)
  {
    HSLogMan *LM = HSLogMan::Instance();
    Lng32 retcode = 0;
    HSDataBuffer result;
    static T lastValue, mostFreqVal;
    static Int64 MFVrows = 0, MFV2rows = 0;

    // Interval(0) is a special interval and we only need to format its
    // boundary, which serves as the minimum value of interval(1) (and hence
    // the whole histogram). Use the initial value to start off interval(1)
    // and return.
    //
    if (currentInt_ == 0)
      {
        setBufferValue(value, group, intArry_[0].boundary_);
        currentInt_++;
        intArry_[currentInt_].uecCount_ = 1;
        // Interval 1 can't be gap, but it may be a high frequency interval.
        if (numRows < step_ && numRows > highFreqThreshold_)
          {
            intArry_[currentInt_].highFreq_ = TRUE;
            highFreqIntervalsUsed_++;
            if (LM->LogNeeded())
              {
                sprintf(LM->msg,
                        "Interval 1 used as high frequency interval with " PF64 " rows",
                        numRows);
                LM->Log(LM->msg);
              }
          }
      }

    // Start a new interval if the current value's rowcount would overflow the
    // current interval, or if this value or the last was the single value included
    // in a gap interval. Otherwise, add the rowcount to the current interval.
    //
    else if (currentInt_ < intCount_ &&
          (intArry_[currentInt_].rowCount_ + numRows > step_ ||  // bucket overflow
           numRows > highFreqThreshold_ ||  // next intvl will be for high freq
           bigGap ||                        // next intvl will be for for gap
           intArry_[currentInt_].gapMagnitude_ > 0 || // current intvl is for gap
           intArry_[currentInt_].highFreq_))          // current intvl is for high freq
      {
        // Complete information for interval and start new one:
        // Save boundary and most frequent values.
        setBufferValue(lastValue, group, intArry_[currentInt_].boundary_);
        setBufferValue(mostFreqVal, group, intArry_[currentInt_].mostFreqVal_);
        intArry_[currentInt_].MFVrowCount_  = MFVrows;
        intArry_[currentInt_].MFV2rowCount_ = MFV2rows;
        MFVrows = MFV2rows = 0;  // Clear these for next interval;

        currentInt_++;
        intArry_[currentInt_].uecCount_ = 1;
        // If the current value is the high end of a big gap, set a nonzero gap
        // value for the next interval. This will cause that interval to be
        // completed with only that value when this function is called with the
        // next value. If the interval contains a single uec with a row count >=
        // the target bucket height, don't mark it as a gap because we want to
        // keep it a separate interval and not merge it with an adjacent interval
        // if it turns out not to be one of the biggest gaps.
        if (numRows < step_)
          {
            if (numRows > highFreqThreshold_)
              {
                intArry_[currentInt_].highFreq_ = TRUE;
                highFreqIntervalsUsed_++;
                if (LM->LogNeeded())
                  {
                    sprintf(LM->msg,
                            "Interval %d used as high frequency interval with " PF64 " rows",
                            currentInt_, numRows);
                    LM->Log(LM->msg);
                  }
              }
            else if (bigGap)
              {
                intArry_[currentInt_].gapMagnitude_ = gapMagnitude;
                gapIntCount_++;
              }
          }

        if (NOT singleIntervalPerUec_)
          {
            // Adjust the interval threshold (STEP_) by the remainder rows
            // and intervals. Update remRows by subtracting the row count of the
            // interval just completed. Subtract the number of unused intervals
            // that were designated for high frequency values and gaps before
            // dividing to find new step size. The gap count is imprecise because
            // some will be merged back.
            remRows_ = MAXOF(remRows_ - intArry_[currentInt_ - 1].rowCount_, 1);

            // If gaps are being processed, there may be a shortfall of
            // intervals due to undersized gap intervals, which will drive
            // the step size higher. Here we release reserve intervals if
            // necessary to try to keep the step size from exceeding its
            // original value by more than 10%.
            Int32 remainingIntervalsAvailable;

            // Increase # of intervals until current step less than
            // 110% of original (or we use up all available intervals).
            do {
                remainingIntervalsAvailable =
                   (MAXOF(1, 
                          intCount_ 
                            - (highFreqIntervalsAllotted_ - highFreqIntervalsUsed_)
                            - (MAXOF(0, (Lng32)((targetGapIntervals_ - gapIntCount_) * 1.5)))
                            - (currentInt_ - 1)));
                intCount_++; // This is the only place intCount_ is increased.
                step_ = remRows_ / remainingIntervalsAvailable;
              }
            while (intCount_ <= maxAllowedInts_ && step_ > 1.1 * originalStep_);

            intCount_--; // This was incremented one too many times.
          }
      }
    else
      intArry_[currentInt_].uecCount_++;

    double numRowsd                     = (double) numRows;
    intArry_[currentInt_].rowCount_     += numRows;
    intArry_[currentInt_].squareCntSum_ += numRowsd * numRowsd;
    if (fi_) 
      fi_[currentInt_].increment(numRows);

    // Update most frequent values.
    if (numRows > MFVrows)
    {
      MFV2rows = MFVrows;
      MFVrows = numRows;
      mostFreqVal = value;
    }
    else if (numRows > MFV2rows)
      MFV2rows = numRows;

    // If this is the last distinct value, set it as interval boundary value
    // instead of waiting for a value that forces start of a new interval.
    // If not doing internal sort, final=true may just mean end of a rowset,
    // but we have to set the boundary value and mostFreqVal in case it is the
    // last rowset. If not, the actual final value of the interval will 
    // overwrite it. This is why we save lastValue even if final is true.
    if (final)
    {
      setBufferValue(value, group, intArry_[currentInt_].boundary_);
      setBufferValue(mostFreqVal, group, intArry_[currentInt_].mostFreqVal_);
      intArry_[currentInt_].MFVrowCount_  = MFVrows;
      intArry_[currentInt_].MFV2rowCount_ = MFV2rows;
      MFVrows = MFV2rows = 0;  // Clear these for next interval/column;
    }
    lastValue = value;
    return retcode;
  }
};


class HSInMemoryTable : public NABasicObject
{
  public:
    HSInMemoryTable(NAString& tblName, NAString& condition,
                    Int64 maxRows, double sampleRate = 0)
      : tableName_(tblName),
        whereCondition_(condition),
        rows_(maxRows),  // replaced in populate() w/actual # rows read
        sampleRate_(sampleRate),
        columns_(NULL),
        isPopulated_(FALSE)
      {
        setUpColumns();
      }

    virtual ~HSInMemoryTable()
      {}

    HSColGroupStruct* getColumns() const { return columns_; }

    void setNumRows(Int64 x) { 
       rows_ = x; 
       HSGlobalsClass::getMemoryRequirements(columns_, rows_); 
    }

    Int64 getNumRows() const { return rows_; }

    // method for algorithm 2
    void generateSelectList(NAString& queryText);

    void generateInsertSelectDQuery(NAString& targetTbl, NAString& smplTable, 
                                    NAString& queryTex);

    void generateInsertSelectIQuery(NAString& targetTbl, NAString& sourceTable,
                                    NAString& queryText,
                                    NABoolean hasOversizedColumns, HSTableDef * objDef,
                                    Int64 currentSampleSize, Int64 futureSampleSize,
                                    Int64 sourceSetSize);

    void generateSelectDQuery(NAString& smplTable, NAString& queryTex);
    void generateSelectIQuery(NAString& smplTable, NAString& queryText);


    // method for algorithm 1
    void generateDeleteQuery(NAString& smplTable, NAString& queryText, NABoolean rollback);
    void generateInsertQuery(NAString& smplTable, NAString& sourceTable, 
                             NAString& queryText, NABoolean rollback);
   
    Lng32 populate(NAString& queryText);

    // The data is actually deallocated by calling freeISMemory() from
    // HSGlobalsClass::incrementHistograms() for each column as soon as the
    // column is successfully handled by IUS (the data is preserved for use
    // by RUS/IS if IUS can't be performed). This function just resets the
    // flag that would cause assertion failure when populate() is called, as
    // it must be to load data for the next batch of IUS columns.
    void depopulate() {
      isPopulated_ = FALSE;
    }

    void logState(const char* title);

  private:
    // Copy construction/assignment not defined.
    HSInMemoryTable(const HSInMemoryTable&);
    HSInMemoryTable& operator=(const HSInMemoryTable&);

    void setUpColumns();


    NAString tableName_;
    NAString whereCondition_;
    Int64 rows_;
    double sampleRate_;
    HSColGroupStruct* columns_;
    NABoolean isPopulated_;
};  // class HSInMemoryTable

#endif /* HSGLOBALS_H */
