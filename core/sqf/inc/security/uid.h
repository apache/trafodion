// UID.h -- Representing users in ServerWare NT security.
//
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
//
//

#ifndef _uid_h_
#define _uid_h_

#include <stddef.h>
#include "rosetta/rosgen.h"

// NTSEC_USER constructors/operators actually live in the Security DLL.
#ifndef SECLIBAPI
#define SECLIBAPI __declspec(dllimport)
#endif

//-----------------------------------------------------------------------------
// End users of ServerWare NT have some form of internal representation that
// could change over time.  To conceal that change from code outside of the
// security implementation, we encapsulate the representation in a class and
// manipulate it with associated member functions.
//
// We also need to hand around user identities in some "exportable" form,
// i.e. in some form we can stick into a message and have it be interpretable
// by another process, or that we can stick into a buffer that gets written
// to disk (e.g. the disk label, TSN Black Box, SQL/ARK metadata).
//
// After some experimentation, the simplest answer is to use the "exportable"
// form also as the internal form.  If the class has no virtual functions,
// it ought not to take up any more space than a struct with the equivalent
// data members.
//
// NSK defines the type name UID_T (in DSYSTYP), corresponding to the POSIX
// scalar type uid_t (defined in sys/types.h).  This name has too strong a
// connotation of the POSIX identifier, so we do NOT use it for our class name.
// Instead, we'll use NTSEC_USER as our new class name.

//-----------------------------------------------------------------------------
// NTSEC_USER is the ServerWare/NT Security representation of a user identity.
//
// C++ claims that all data members specified between two access keywords
// (i.e. public:, private:, protected:) will be stored in successively
// higher memory locations, with possibly alignment filler.  Microsoft
// claims this is true for all data members in the class, implying that
// the language definition doesn't guarantee ordering across access keywords.
// Just to be safe, PUT ALL DATA MEMBERS IN THE SAME PLACE in this class.

// NTSEC_USER Type values.
// We use text to make them simpler to identify in memory.
//
// Our pre-defined NTSEC_USER values.  These are not real users.
enum {
#ifdef __linux__
	NTSEC_TYPE_PREDEF	= ((('P' << 8) | ('R' << 8)) | ('E' << 8)) | ' ',
	NTSEC_TYPE_NTSID	= ((('S' << 8) | ('I' << 8)) | ('D' << 8)) | ' '
#else
	NTSEC_TYPE_PREDEF	= 'PRE ',	// Predefined values, not real users.
	NTSEC_TYPE_NTSID	= 'SID ',	// NT users represented by an NT SID.
#endif
};

// NTSEC_USER Flag values.
//
// Things to remember about this user.
enum {
	NTSEC_FLAG_NTADMIN	 = 0x01,	// Belongs to local NT Administrators group.
	NTSEC_FLAG_NTBACKUP  = 0x02,	// Had SE_BACKUP_NAME privilege enabled.
	NTSEC_FLAG_NTRESTORE = 0x04 	// Had SE_RESTORE_NAME privilege enabled.
};

// NTSEC_USER Data area size.
//
// We happen to include space for the maximum size of a SID.  NT does
// not define a constant for this.  Even if it did we couldn't use it;
// since NTSEC_USER can be one component of a structure on disk, its size can't
// change over time.  So, we establish a constant for the space we reserve
// for the SID (or whatever other representation we may change to later).
//
// A SID is 8 bytes plus up to 15 DWORDs, as of Windows NT 3.51.
// Therefore that's how much space we'll allocate, forever.
#define NTSEC_DATA_MAX_BYTES	(8+15*4)

// Textual SID maximum length.
//
// We allow extracting a "printable SID" from the NTSEC_USER value.
// See the implementation of the "textsid()" member function for 
// the derivation of this value.  This includes the terminating null.
#define SECURITY_TEXTSID_MAX_BYTES	(21+15*11)

// dg64 - this struct is embedded in 32-bit generated files, so need 32-bits
class NTSEC_USER
{
private:
	// All data members.
#ifdef NA_64BIT
	xunsigned_32	Type;		// A constant so we know what's what.
#else
	unsigned_32	Type;		// A constant so we know what's what.
#endif
#ifdef NA_64BIT
	xunsigned_32	Length;		// Actual size of the following SID or whatever.
#else
	unsigned_32	Length;		// Actual size of the following SID or whatever.
#endif
#ifdef NA_64BIT
	xunsigned_32 Flags;		// Random useful information.
#else
	unsigned_32 Flags;		// Random useful information.
#endif
	char		Data[NTSEC_DATA_MAX_BYTES];	// The actual whatever.

public:
	// Constructors exported to callers.
	SECLIBAPI NTSEC_USER();				// Default constructor.
	SECLIBAPI NTSEC_USER(void *pSid);	// Constructor from a SID.
										// (?? should use PSID)
	SECLIBAPI NTSEC_USER(const char *Username);	// Constructor from a username.
	SECLIBAPI NTSEC_USER(const wchar_t *Username);	// UNICODE username.
	SECLIBAPI NTSEC_USER(const char *TextSid, int dummy); // Textual SID.
	SECLIBAPI NTSEC_USER(const NTSEC_USER& User);// Copy constructor.

public:
	// Constructors for internal use only.
	// These can't be private, but aren't exported from the DLL either.
	NTSEC_USER(int predef_type);	// Constructor for predefined values.

	// No destructor needed.

public:
	// Binary operators.
	// In each of these, "this" is the left hand side, 
	// the parameter is the right hand side.

	// Assignment operator.  Allows rhs to be const; returns a reference
	// so as to behave like standard assignment operators.
	SECLIBAPI NTSEC_USER& operator=(const NTSEC_USER& rhs);

	// Comparison operators.
	// The "const" keywords allow either side to be const.
	// Return TRUE/FALSE (?? should use return type BOOL not int).
	SECLIBAPI int operator< (const NTSEC_USER& rhs) const;
	SECLIBAPI int operator<=(const NTSEC_USER& rhs) const;
	SECLIBAPI int operator==(const NTSEC_USER& rhs) const;
	SECLIBAPI int operator>=(const NTSEC_USER& rhs) const;
	SECLIBAPI int operator> (const NTSEC_USER& rhs) const;
	SECLIBAPI int operator!=(const NTSEC_USER& rhs) const;

public:
	// Member functions callable by other layers.
	// Functions with inline bodies don't need SECLIBAPI.

	// Let callers determine how much space they *really* need.
	unsigned_32 length() const 
	{ return (sizeof(NTSEC_USER) - NTSEC_DATA_MAX_BYTES + Length); }

	// Let callers determine if this user is "real" or invalid.
	//?? Should return BOOL not int ?
	int valid() const { return Type == NTSEC_TYPE_NTSID; }

	// Fetch username, in ASCII or UNICODE.  Caller supplies buffer.
	SECLIBAPI int_16 username(char *userbuf, size_t *userlen) const;
	SECLIBAPI int_16 username(wchar_t *userbuf, size_t *userlen) const;

	// Fetch user's SID in textual form.
	SECLIBAPI int_16 textsid(char *sidbuf, size_t *sidlen) const;
	// Fetch user's SID in binary form.
	SECLIBAPI int_16 binarysid(void *sidbuf, size_t *sidlen) const;

public:
	// Member functions not exported to other layers; therefore,
	// they are not SECLIBAPI and are not implemented in-line.
	// They are still "public" so other parts of Security can use them.

	// "Flags" access functions.
	unsigned_32 get_flags(void) const;
	void set_flags(unsigned_32 flags);

private:
	// Member functions used only within the class (so not SECLIBAPI).

	// Generic comparison function, used by comparison operators.
	int compare(const NTSEC_USER& rhs) const;
};


#endif // _uid_h_
