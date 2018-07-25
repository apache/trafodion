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
#ifndef COMANSINAMEPART_H
#define COMANSINAMEPART_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComAnsiNamePart.h
 * Description:  An object of this class represents one part (catalog,
 *               schema, object or column) of a fully qualified ANSI
 *               name.  An ANSI name part can be either a ANSI SQL
 *               regular identifier or delimited identifier.
 *
 * Created:      7/20/95
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "NAWinNT.h"		// for wchar (tcr)
#include <iosfwd>
using namespace std;
#include <string.h>
#include "ComOperators.h"
#include "ComSmallDefs.h"
#include "ComSizeDefs.h"
#include "NABoolean.h"
#include "NAString.h"

// -----------------------------------------------------------------------
// External declarations
// -----------------------------------------------------------------------
class ComRoutineActionNamePart;

// -----------------------------------------------------------------------
// Computed internal-format column name
// -----------------------------------------------------------------------

#define DIVISION_COLUMN_NAME_PREFIX "DIVISION_"
#define DIVISION_COLUMN_NAME_PREFIX_LEN_IN_BYTES 9
#define DIVISION_COLUMN_SEQ_NUM_MAX_LEN_IN_BYTES 10
#define DIVISION_COLUMN_NAME_MAX_LEN_IN_BYTES 35
#define MAX_FUNNY_NAME_LENGTH 16 
// 35 =    9 // DIVISION_COLUMN_NAME_PREFIX_LEN_IN_BYTES
//      + 10 // DIVISION_COLUMN_SEQ_NUM_MAX_LEN_IN_BYTES
//      + 16 // MAX_FUNNY_NAME_LENGTH - funny-suffix-max-len-in-bytes defined in w:/sqlshare/CatSQLShare.h

// // InternalIdentifierHasDivColNamePrefix() returns TRUE if the internal
// // identifier has the DIVISION_ name prefix; otherwise, returns FALSE.
// NABoolean InternalIdentifierHasDivColNamePrefix (const char * internalColName); // in

// -----------------------------------------------------------------------
// Functions to convert ANSI SQL names from the current for-internal-
// processing character set (i.e., the Unicode UTF-8 character set) to
// the Unicode UCS-2/UTF-16 encoding format and vice versa.
//
// Similar APIs with char* and NAWchar* parameters in place of
// NAString and NAWString parameters are declared and defined
// in the header and source files w:/common/ComDistribution.h and
// .cpp for used by low-level code like the one in the source
// file w:/comexe/LateBindInfo.cpp which cannot use the process heap.
// -----------------------------------------------------------------------
// returned error code described in w:/common/csconvert.h
Int32 ComAnsiNameToUTF8 ( const NAWString &inAnsiNameInUCS2 // in  - valid ANSI SQL name in UCS2/UTF16
                        , NAString & outBuf4AnsiNameInUTF8  // out - out buffer in UTF8
                        );

// returned error code described in w:/common/csconvert.h
Int32 ComAnsiNameToUCS2 ( const NAString & inAnsiNameInUTF8 // in  - input C string in UTF8
                        , NAWString & outAnsiNameInUCS2     // out - out buffer containing UCS2/UTF16 chars
                        );

// -----------------------------------------------------------------------
// definition of class ComAnsiNamePart
// -----------------------------------------------------------------------

class ComAnsiNamePart : public NABasicObject
{

  //
  // global friend functions
  //

  friend ostream& operator<< (ostream &out, const ComAnsiNamePart &name);

  public:

    //
    // enumerations
    //

    // Please use the enumerated constants defined in w:/common/ComSizeDefs.h
    // instead of the following definitions for new code.
    enum { MAX_IDENTIFIER_INT_LEN = ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES,
           MAX_IDENTIFIER_EXT_LEN = ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES,
           // nobody use this enum ... MAX_DELIMIDENT_LEN     = MAX_IDENTIFIER_EXT_LEN,
           MAX_ANSI_NAME_EXT_LEN  = ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES,
           MAX_ANSI_NAME_INT_LEN  = 3 * MAX_IDENTIFIER_INT_LEN + 2, // what is this ???
           MAX_OLD_ANSI_IDENTIFIER_LEN = 258,  // See comexe/LateBindInfo.h and BDR
           MAX_OLD_ANSI_NAME_EXT_LEN = 776 };  // used in BDR

      // ANSI 5.2 SR 8 + 9:
      // The first is the max number of internal characters,
      // the second the max of external chars in a delimited identifier
      // (the most pathological example, an ident which internally is
      // 128 dquotes -- externally is 128 pairs of dquotes (i.e. quoted quotes)
      // all delimited fore and aft with a dquote.

    enum formatEnum { EXTERNAL_FORMAT ,
                      INTERNAL_FORMAT };

        // EXTERNAL_FORMAT   the input identifier is in external
        //                   format, ANSI format, the format used by the user
        //
        // INTERNAL_FORMAT   the input identifier is in internal
        //                   format, the format stored in the
        //                   schema metadata tables

    //
    // constructors
    //

        // Default constructor.  This method creates an empty object.

    ComAnsiNamePart (CollHeap * h=0,
                     unsigned short toInternalIdentifierFlags = NASTRING_ALLOW_NSK_GUARDIAN_NAME_FORMAT);

    // copy ctor
    ComAnsiNamePart (const ComAnsiNamePart & orig, CollHeap * h=0) ;

    ComAnsiNamePart (const NAString &name, 
                     formatEnum format = EXTERNAL_FORMAT,
                     CollHeap * h=0,
                     unsigned short toInternalIdentifierFlags = NASTRING_ALLOW_NSK_GUARDIAN_NAME_FORMAT);

        // toInternalIdentifierFlags is the sames as pv_flags in ToInternalIdentifier() declared in NAString.h

    ComAnsiNamePart (const char *name, size_t nameLen,
                     formatEnum format = EXTERNAL_FORMAT,
                     CollHeap * h=0,
                     unsigned short toInternalIdentifierFlags = NASTRING_ALLOW_NSK_GUARDIAN_NAME_FORMAT);

        // Checks the input ANSI SQL name component which is in the
        // specified format.
	// In the first ctor, the input name component cannot contain 
	// any null character;
	// in the second,
        // the name component is exactly  nameLen  bytes long, and
	// may contain null characters.
        //
        // If the input name component is legal, construct the object
        // from the input name component; otherwise, construct an
        // empty object.

    ComAnsiNamePart ( const NAString &externalNameParts 
                     ,size_t &count
                     ,CollHeap * h=0
                     ,NABoolean createDropalias = FALSE 
                     ,NABoolean acceptCircumflex = FALSE
                     ,unsigned short toInternalIdentifierFlags = NASTRING_ALLOW_NSK_GUARDIAN_NAME_FORMAT
                    );

    ComAnsiNamePart (const char *externalNameParts, 
                     size_t externalNPLen,
                     size_t &count,
                     CollHeap * h=0,
                     unsigned short toInternalIdentifierFlags = NASTRING_ALLOW_NSK_GUARDIAN_NAME_FORMAT);

        // Scans (parses) the specified input string
        // for an external-format ANSI SQL name part.
        //
        // The first character in the input string must be the first
        // character of the name part.  Note that the input string
        // can include other input besides the name part.
        //
        // The method copies the name part if it is found, and
        // returns the length of the scanned name part via the
        // parameter  count.  The method also computes the corre-
        // sponding internal-format name part and stores the
        // computed internal name.
        //
        // If the name part is not found or is illegal (for example,
        // the name part is too long), the method returns the number
        // of bytes examined when the identifier is determined
        // to be invalid, and constructs an empty object.

    //
    // virtual destructor
    //
    virtual ~ComAnsiNamePart ();

    //
    // virtual cast functions
    //
    virtual ComRoutineActionNamePart * castToComRoutineActionNamePart();
    virtual const ComRoutineActionNamePart * castToComRoutineActionNamePart() const;

    //
    // type conversion operator
    //
    operator const char * () const	{ return (const char *)externalName_; }

        // Returns a pointer to the external-format ANSI SQL name component.

    //
    // assignment operators
    //

    ComAnsiNamePart& operator= (const ComAnsiNamePart& );
    ComAnsiNamePart& operator= (const NAString &externalName);

        // The specified external name on the right-hand side
        // must be a valid ANSI SQL name component, in external
        // format.  If not, this object (on the left-hand side)
        // will be cleared.

    //
    // logical operator
    //
    NABoolean operator== (const ComAnsiNamePart &rhs) const;

    inline Int32      compareTo  (const ComAnsiNamePart &rhs) const;

    //
    // accessors
    //

    // Use these two methods only if input strings have been parsed.
    void setExternalName (const NAString &name) { externalName_ = name; }
    void setInternalName (const NAString &name) { internalName_ = name; }

    const NAString &getData () const		{ return externalName_; }

    const NAString &getExternalName () const	{ return externalName_; }
  
    // Returns the ANSI SQL name component, in external format.
  
    const NAString &getInternalName () const	{ return internalName_; }

        // Returns the ANSI SQL name component, in internal format,
        // without padded trailing white spaces.

    NABoolean isEmpty () const		    { return externalName_.isNull(); }

    NABoolean isValid () const		    { return NOT isEmpty(); }

        // The method returns TRUE if the object contains a legal
        // ANSI SQL name component.  The method returns FALSE if
        // the object is empty (does not contain an ANSI SQL name
        // component).  An object is empty if it is created by
        // the default constructor or the input ANSI SQL name
        // component is illegal.

    NABoolean isDelimitedIdentifier () const;

        // The method returns TRUE if the ANSI SQL name component
        // (identifier) is a delimited identifier.  The method
        // returns FALSE if the identifier is invalid or is not a
        // delimited identifier.

    //
    // mutators
    //

    void clear()			{ externalName_ = internalName_ = ""; }

        // Makes this object an empty object.


  private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

    NABoolean scanAnsiNamePart ( const NAString &externalNameParts
			       , size_t &count
                               , NABoolean createDropAlias = FALSE
                               , NABoolean acceptCircumflex = FALSE
                               , unsigned short toInternalIdentifierFlags = NASTRING_ALLOW_NSK_GUARDIAN_NAME_FORMAT
                               );

        // This method scans (parses) for one external-format ANSI 
	// SQL name part in the (possibly multi-part) input string.
	// That is, given "xx.yy.zz", the name is set to XX and the
	// count returns as 2.
	//
        // The method returns TRUE if the name part is found, and
        // returns the length of the scanned name part, and sets
        // the corresponding internal-format name; otherwise, the
        // method returns FALSE.
        //
        // The syntax of ANSI SQL name part is defined on page 78
        // of the ANSI X3H2-93-004 (also known as SQL-92) standard.
        // The ANSI SQL name component is referred to as
        // <actual identifier> in the standard.
        //
        // externalNameParts     IN
        //
        //   This parameter contains the input string containing an
        //   external-format ANSI SQL actual identifier.
        //
	// count                 IN OUT
	//
	//   This param's IN value must be either 0 (scan entire NAString)
	//   or 1 (scan till bad character) is described in more detail
	//   in the .cpp file.
	//
	//   The length, in bytes, of the scanned name component is
	//   returned via this parameter.  If an error occurs, this
	//   parameter contains the number of characters examined
	//   when the identifier is determined to be invalid.
	//   Any separating dot is not included in this count.
	//
        // NABoolean createDropAlias  = FALSE              IN
        //
        // NABoolean acceptCircumflex = FALSE              IN
        //
        // unsigned short toInternalIdentifierFlags = NASTRING_ALLOW_NSK_GUARDIAN_NAME_FORMAT
        //                                                 IN
        //
        //   Same as the pv_flags parameter in ToInternalIdentifier() routine
        //
        // data member     this->externalName_             OUT
        // data member     this->internalName_             OUT
        //
        //   The method puts the internal-format name computed from
        //   the scanned externalNameParts into data member internalName_.
        //   If an error occurs, internalName_ contains an empty string.

    NABoolean copyExternalName (const NAString &externalName);
    NABoolean copyInternalName (const NAString &internalName);

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

    NAString  externalName_;
    NAString  internalName_;

    CollHeap * heap_;

  public:
    unsigned short toInternalIdentifierFlags_;

};  // class ComAnsiNamePart

// -----------------------------------------------------------------------
// definitions of inline methods
// -----------------------------------------------------------------------

Int32 ComAnsiNamePart::compareTo (const ComAnsiNamePart &rhs) const
{
  if (this EQU &rhs) return 0;
  else return internalName_.compareTo (rhs.internalName_);
}

// -----------------------------------------------------------------------
// declaration of function ComDeriveRandomInternalName
// -----------------------------------------------------------------------

ComBoolean ComDeriveRandomInternalName( Lng32 nameCharSet,
                                        const ComString &inputNameInInternalFormat,
                                        ComString &generatedNameInInternalFormat,
                                        NAHeap *h = 0 );

ComBoolean ComIsRandomInternalName(const ComString &inputName);

#endif // COMANSINAMEPART_H
