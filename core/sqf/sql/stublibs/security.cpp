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
#ifdef _WIN32

#include <windows.h>
#define DLLEXPORT __declspec(dllexport)
#define SECLIBAPI __declspec(dllexport)

#else


#include "sqtypes.h"
#define SECLIBAPI


#endif /* _WIN32 */

#include <string.h>
#define MYMIN(x,y) ((x) < (y) ? (x) : (y))
//#include <stdlib.h>

/* Begin: from inc\rosetta\rosgen.h */

typedef void *                                extaddr;

#define FEOK 0

typedef long NSK_int32;
#define gid_t NSK_int32

// Init_flag values used with Security_MSB_Init_:
enum {MSB_INIT_DEFAULT  = 0,
      MSB_CHECK_ONLY    = 1, // Check message security, but don't
                             // deliver message to target process
      MSB_ACCESS        = 2, // POSIX.1 access() support
      MSB_SQLSUBSYS     = 4
     };                      // Caller invokes SQL Subsystem privilege

short const          OMITSHORT         = -291;

typedef unsigned short _unsigned_16;
union security_options_template {
    _unsigned_16 initialize0;
    struct {
        _unsigned_16 _filler : 10;
        _unsigned_16 licensed_param : 1;
        _unsigned_16 licensed : 1;
        _unsigned_16 sql_executor_param : 1;
        _unsigned_16 sql_executor : 1;
        _unsigned_16 deny_grants : 1;
        _unsigned_16 checkonly : 1;
    } st;
};

class  NTSEC_USER
{
#define NTSEC_DATA_MAX_BYTES 64
private:
  // All data members.
  unsigned_32     Type;           // A constant so we know what's what.
  unsigned_32     Length;         // Actual size of the following SID or whatever.
  unsigned_32 Flags;              // Random useful information.
  char            Data[NTSEC_DATA_MAX_BYTES];     // The actual whatever.


public:
  // Constructors exported to callers.
  SECLIBAPI NTSEC_USER(){ Type = 1;}                         // Default constructor.
  SECLIBAPI NTSEC_USER(void *pSid){ }       // Constructor from a SID.
  SECLIBAPI NTSEC_USER(const char *Username){ Type = 1;}     // Constructor from a username.
  SECLIBAPI NTSEC_USER(const wchar_t *Username){ }  // UNICODE username.
  SECLIBAPI NTSEC_USER(const char *TextSid, int dummy);
  SECLIBAPI NTSEC_USER(const NTSEC_USER& User){ }// Copy constructor.

public:
  // Constructors for internal use only.
  // These can't be private, but aren't exported from the DLL either.
  NTSEC_USER(int predef_type){ }    // Constructor for predefined values.

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
  SECLIBAPI int operator< (const NTSEC_USER& rhs) const { return (1 == 1); }
  SECLIBAPI int operator<=(const NTSEC_USER& rhs) const { return (1 == 1); }
  SECLIBAPI int operator==(const NTSEC_USER& rhs) const;
  SECLIBAPI int operator==(NTSEC_USER& rhs) const { return (1 == 1); }
  SECLIBAPI int operator>=(const NTSEC_USER& rhs) const { return (1 == 1); }
  SECLIBAPI int operator> (const NTSEC_USER& rhs) const { return (1 == 1); }
  SECLIBAPI int operator!=(const NTSEC_USER& rhs) const;
public:
  // Member functions callable by other layers.
  // Functions with inline bodies don't need SECLIBAPI.

  // Let callers determine how much space they *really* need.
  unsigned_32 length() const { return 0; }

  // Let callers determine if this user is "real" or invalid.
  //?? Should return BOOL not int ?
  int valid() const 
  { return 1; //return Type == NTSEC_TYPE_NTSID; 
  }

  // Fetch username, in ASCII or UNICODE.  Caller supplies buffer.
  SECLIBAPI int_16 username(char *userbuf, size_t *userlen) const ;
  SECLIBAPI int_16 username(wchar_t *userbuf, size_t *userlen) const ;

  // Fetch user's SID in textual form.
  SECLIBAPI int_16 textsid(char *sidbuf, size_t *sidlen) const ;

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

SECLIBAPI NTSEC_USER::NTSEC_USER(const char *TextSid, int dummy){ 
    Length = MYMIN(strlen(TextSid),NTSEC_DATA_MAX_BYTES-1);
    strncpy(Data, 
	    TextSid,
	    Length);
  }

int NTSEC_USER::operator==(const NTSEC_USER& rhs) const 
{ return (rhs.Type == this->Type); }

// Fetch username, in ASCII or UNICODE.  Caller supplies buffer.
SECLIBAPI int_16 NTSEC_USER::username(char *userbuf, size_t *userlen) const 
{ *userlen = 0; *userbuf = 0; return 0; }

SECLIBAPI int_16 NTSEC_USER::username(wchar_t *userbuf, size_t *userlen) const 
{ *userlen = 0; *userbuf = 0; return 0; }

SECLIBAPI int_16 NTSEC_USER::textsid(char *sidbuf, size_t *sidlen) const 
{
    strncpy(sidbuf,
	    Data,
	    Length);
    *sidlen = Length;
    return 0;
}
  SECLIBAPI NTSEC_USER& NTSEC_USER::operator=(const NTSEC_USER& rhs) { return *this;}

SECLIBAPI int NTSEC_USER::operator!=(const NTSEC_USER& rhs) const { return (1 == 1); }

//------------------------------------------------------------------------------
// PROTECTION_CHECK_ contacts Safeguard for a security ruling.
// BENIGN STUB: no Safeguard, but on common paths.
//
SECLIBAPI int_16 PROTECTION_CHECK_(
        void                    *phandle_,
        int_32                  *msgid,
    void                        *reqctrl_,
    int_16                      reqctrlsize,
    void                        *reqdata_,
    int_16                      reqdatasize,
    int_32                      linkertag,
    int_16                      linkopts,
    int_16                      nowaitflag,
    int_16                      *retrycpu,
    void                        *smonbuffer_)
{
        return FEOK;
}

//------------------------------------------------------------------------------
// SAFEGUARD_GLOBAL_CLEARONPURGE returns the Safeguard configuration flag
// that controls whether deleted files should have their disk storage zeroed.
// BENIGN STUB: no Safeguard, but on common paths.
//
SECLIBAPI int_16 SAFEGUARD_GLOBAL_CLEARONPURGE()
{
        return FALSE;
}

SECLIBAPI    int_16 SECURITY_APP_PRIV_
  (void   *msb_ptr_ = NULL,   // INPUT,  OPTIONAL
   int_16 *status_arg = NULL) // OUTPUT, OPTIONAL
  { return FEOK; }

SECLIBAPI    int_16 SECURITY_G90VECTOR_EVAL_
  (void    *msb_ptr_,         //  INPUT
                              // MSB POINTER
   void    *object_ptr_,      //  INPUT
                              // OBJECT DESCRIPTOR POINTER
   int_16   requested_access, //  INPUT
                              // FILE ACCESS REQUESTED
                              // (SEE DSECURE ACCESS REQUEST LITERALS)
   int_16  *granted_access = NULL)   //  OUTPUT, OPTIONAL
                              // FILE ACCESS GRANTED TO SUBJECT
                              // (SEE DSECURE ACCESS GRANTED LITERALS)
                              // (NOTHING RETURNED FOR TMF ACCESS)
                              // (KLUDGED FOR OBISAYSYES CASE)
  { return FEOK; }

SECLIBAPI   int_16 SECURITY_OBJDESC_INIT_
  (void     *object_ptr_,  //  OUTPUT
                           // OBJECT DESCRIPTOR
   int_16    object_type,  //  INPUT
                           // OBJECT TYPE CODE
   NTSEC_USER     uid,          //  INPUT
                           // DISK FILE'S OWNER
   gid_t     gid,          //  INPUT
                           // DISK FILE'S GROUP
   int_32    modes = 0,        //  INPUT, OPTIONAL
                           // G90 PROTECTMODES OR POSIX.1 FILE MODES
   int_32    flags = 0,        //  INPUT, OPTIONAL
                           // MISC OBJECT ATTRIBUTES
   fixed_0   nopurgeuntil = 0, //  INPUT, OPTIONAL
                           // NO-PURGE-UNTIL TIME, IF SET
   extaddr   protinfo_ptr = NULL) //  INPUT, OPTIONAL
                           // PROTECTION INFO (BLACK BOX) FROM FILE LABEL
  { return FEOK; }

SECLIBAPI extern const NTSEC_USER SECURITY_INVALID_UID = NTSEC_USER(0)
;

SECLIBAPI    int_16 SECURITY_MSB_INIT_
  (void    *msgctrl_,        // INPUT/OUTPUT
                             // SECURE REQUEST MESSAGE CONTROL BUFFER
   int_16   maxlen,          // INPUT
                             // MAX BYTE LENGTH OF CONTROL BUFFER
   int_16  *length,          // INPUT/OUTPUT
                             // BYTE LENGTH OF CONTROL BUFFER.  INCREASED
                             // TO REFLECT THE ADDITION OF AN MSB BEFORE
                             // RETURNING TO OUR CALLER.  ROUNDED UP TO
                             // AN EVEN BYTE LENGTH.
   int_16   msb_selector,    // INPUT
                             // MSB TYPE SELECTOR FROM DSECURE(MSB_INIT)
   void    *target_phandle_, // INPUT
                             // PHANDLE OF THE TARGET PROCESS
   int_16   init_flags = MSB_INIT_DEFAULT)      // INPUT, OPTIONAL
                             // MISCELLANEOUS INITIALIZATION FLAGS
                             // FROM DSECURE(MSB_INIT):
                             //    MSB_ACCESS [FOR POSIX.1 ACCESS()]
                             //    MSB_CHECK_ONLY (FOR
                             //    PROCESS OPENS)
                             //    MSB_SQLSUBSYS [SQL EXECUTOR PRIVILEGE]
{ return FEOK; }

SECLIBAPI   int_16 SECURITY_MSB_GET_
  (void    *msb_ptr_,   //  INPUT
                        // POINTER TO A MESSAGE SECURITY BLOCK (MSB)
   int_16   item1,      // INPUT
                        // SELECTOR OF ITEM TO RETURN IN VALUE1
   void    *value1_,    // BUFFER IN WHICH TO RETURN SELECTED ITEM1
   int_16   max_len1,   // OUTPUT : INPUT
                        // MAX BYTE LENGTH OF VALUE1 BUFFER
   int_16  *value_len1, // OUTPUT
                        // BYTE LENGTH OF ITEM RETURNED IN VALUE1
   int_16   item2 = OMITSHORT,      // INPUT
                        // SELECTOR OF ITEM TO RETURN IN VALUE2
   void    *value2_ = NULL,    // BUFFER IN WHICH TO RETURN SELECTED ITEM2
   int_16   max_len2 = 0,   // OUTPUT : INPUT
                        // MAX BYTE LENGTH OF VALUE2 BUFFER
   int_16  *value_len2 = NULL) // OUTPUT
                        // BYTE LENGTH OF ITEM RETURNED IN VALUE2
{ 
  if (value1_) {
    security_options_template *pSecurityOptions = (security_options_template *) value1_;
    pSecurityOptions->st.licensed = 1;
  }
  
return FEOK; }

SECLIBAPI  int_16 SECURITY_MSB_SIZE_()
{ return FEOK; }

SECLIBAPI int_16 SECURITY_NTUSER_SET_(void)
  { return FEOK; }

SECLIBAPI    int_16 SECURITY_PSB_GET_
  (int_16   item,          //  INPUT
   void    *value_,
   int_16   max_len,       //  OUTPUT : INPUT
   int_16  *value_len_arg = NULL, //  OUTPUT, OPTIONAL
   int_16   pin = OMITSHORT)           //  INPUT, OPTIONAL
                           // PIN IDENTIFIES TARGET OF SECURITY_PSB_GET_.
                           // DEFAULTS TO CURRENT PROCESS' PIN.
{ 
  *((int *)value_) = 0;
  return FEOK; 
}


SECLIBAPI    int_16 SECURITY_PSB_SET_
  (int_16   item,      //  INPUT
   void    *value_,
   int_16   value_len) //  INPUT : INPUT
{ return FEOK; }

SECLIBAPI    int_16 TSN_PROTINFO_EVAL_
  (void   *msb_ptr_,          // MESSAGE SECURITY BLOCK
   void   *verb_info_ptr_,    // OPERATION (TSN^VERB^STRUCT)
   void   *obj_info_ptr_,     // OBJECT DESCRIPTOR
   int_16 *must_audit,        // CALLER MUST AUDIT RESULT
   void   *col_restrict_ptr_ = NULL) // COLUMN RESTRICTIONS
                               // (TSN^SQLCOL^SIZE^STRUCT)
{ return FEOK; }

SECLIBAPI    int_16 TSN_PROTINFO_GETCOLUMNS_
  (void    *msb_ptr_,        // MESSAGE SECURITY BLOCK
   void    *permission_ptr_, // TSN^SQLPERM^STRUCT
   void    *protinfo_buf_,   // BLACK BOX
   int_16  *column_buf,      // BUFFER FOR RETURNED DATA
   int_16   column_len)      // ALLOCATED BYTE SIZE OF COLUMN^BUF
{ return FEOK; }

SECLIBAPI  int_16 TSN_PROTINFO_CHECKCOLUMNS_
  (int_16  *granted_set, // GRANTED COLUMNS BITMASK
   int_16   req_column)  // COLUMN NUMBER TO TEST
{ return FEOK; }

NTSEC_USER _stub_ntsecuser0;
NTSEC_USER _stub_ntsecuser1("sq");
NTSEC_USER _stub_ntsecuser2("sq",1);
NTSEC_USER _stub_ntsecuser3(_stub_ntsecuser0);
NTSEC_USER _stub_ntsecuser4((void *)"sq2");
