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
#ifndef SECLIBAPI
#define SECLIBAPI __declspec(dllimport)
#endif

#include "rosetta/rosgen.h" /* rosetta utilities */
#include "fs/fsbuiltin.h"

#if (defined(psecure_h_security_ntuser_set_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_security_ntuser_set_
SECLIBAPI int_16 SECURITY_NTUSER_SET_(void);

#endif /* section security_ntuser_set_ */
#if (defined(psecure_h_textsidtousername) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_textsidtousername
/* The rosgen.h type unsigned_32 is equivalent to the windef.h type   */
/* DWORD.  We use the rosgen.h type to avoid introducing a dependency */
/* on any Windows NT headers, although in fact the return value is a  */
/* Windows NT status code, normally typed as DWORD.                   */
SECLIBAPI unsigned_32 TextSidToUsername(const char *textsid, char *username, size_t *userlen);

#endif /* section textsidtousername */
#if (defined(psecure_h_protection_check_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_protection_check_
SECLIBAPI _priv _resident _extensible int_16 PROTECTION_CHECK_
  (void    *phandle_,
   int_32  *msgid,
   void    *reqctrl_,
   int_16   reqctrlsize,
   void    *reqdata_ = NULL,
   int_16   reqdatasize = 0,
   int_32   linkertag = 0,
   int_16   linkopts = MSG_LINK_SECURE,
   int_16   nowaitflag = FALSE,
   int_16  *retrycpu = NULL,
   void    *smonbuffer_ = NULL);
#endif /* section protection_check_ */
#if (defined(psecure_h_safeguard_global_clearonpurge) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_safeguard_global_clearonpurge
_alias("SAFEGUARD^GLOBAL^CLEARONPURGE") _callable _resident
SECLIBAPI int_16 SAFEGUARD_GLOBAL_CLEARONPURGE();
#endif /* section safeguard_global_clearonpurge */
#if (defined(psecure_h_security_psb_get_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_security_psb_get_
SECLIBAPI _priv _resident _extensible int_16 SECURITY_PSB_GET_
  (int_16   item,          //  INPUT
   void    *value_,
   int_16   max_len,       //  OUTPUT : INPUT
   int_16  *value_len_arg = NULL, //  OUTPUT, OPTIONAL
   int_16   pin = OMITSHORT)           //  INPUT, OPTIONAL
                           // PIN IDENTIFIES TARGET OF SECURITY_PSB_GET_.
                           // DEFAULTS TO CURRENT PROCESS' PIN.
;
#endif /* section security_psb_get_ */
#if (defined(psecure_h_security_psb_set_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_security_psb_set_

SECLIBAPI _priv _resident _extensible int_16 SECURITY_PSB_SET_
  (int_16   item,      //  INPUT
   void    *value_,
   int_16   value_len) //  INPUT : INPUT
;
#endif /* section security_psb_set_ */
#if (defined(psecure_h_security_app_priv_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_security_app_priv_
SECLIBAPI _priv _resident _extensible int_16 SECURITY_APP_PRIV_
  (void   *msb_ptr_ = NULL,   // INPUT,  OPTIONAL
   int_16 *status_arg = NULL) // OUTPUT, OPTIONAL
;
#endif /* section security_app_priv_ */
#if (defined(psecure_h_security_g90vector_eval_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_security_g90vector_eval_

SECLIBAPI _priv _resident _extensible int_16 SECURITY_G90VECTOR_EVAL_
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
;
#endif /* section security_g90vector_eval_ */
#if (defined(psecure_h_security_objdesc_init_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_security_objdesc_init_

SECLIBAPI _resident _extensible int_16 SECURITY_OBJDESC_INIT_
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
;
#endif /* section security_objdesc_init_ */
#if (defined(psecure_h_security_g90file_owner_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_security_g90file_owner_

SECLIBAPI _priv _resident _extensible int_16 SECURITY_G90FILE_OWNER_
  (void   *msb_ptr_,    // INPUT
                        // MSB DESCRIBING THE SUBJECT
   void   *object_ptr_, // INPUT
                        // DISK FILE OBJECT DESCRIPTOR
   int_16 *status_arg = NULL,  // OUTPUT, OPTIONAL
                        // OPTIONAL STATUS RETURN
   int_16 *audit = NULL)       // OUTPUT, OPTIONAL
                        // TRUE_ => CLIENT AUDIT REQUIRED
;
#endif /* section security_g90file_owner_ */
#if (defined(psecure_h_security_msb_init_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_security_msb_init_

SECLIBAPI _priv _resident _extensible int_16 SECURITY_MSB_INIT_
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
;
#endif /* section security_msb_init_ */
#if (defined(psecure_h_security_msb_get_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_security_msb_get_

SECLIBAPI _resident _extensible int_16 SECURITY_MSB_GET_
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
;
#endif /* section security_msb_get_ */
#if (defined(psecure_h_security_msb_getlist_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_security_msb_getlist_

SECLIBAPI _resident _extensible int_16 SECURITY_MSB_GETLIST_
  (void    *msb_ptr_,      //  INPUT
                           // POINTER TO A MESSAGE SECURITY BLOCK (MSB)
   int_16  *item_array,    // INPUT
                           // ARRAY OF ITEM SELECTORS
   int_16   item_count,    // INPUT
                           // NUMBER OF ITEM SELECTORS IN ITEM_ARRAY
   void    *value_buffer_, // BUFFER FOR RETURNING WORD-ALIGNED VALUES
   int_16   value_maxlen,  // OUTPUT : INPUT
                           // MAX BYTE LENGTH OF VALUE_BUFFER
   int_16  *value_len = NULL,     // OUTPUT, OPTIONAL
                           // BYTE LENGTH OF VALUES RETURNED
   int_16  *error_item = NULL)    // OUTPUT, OPTIONAL
                           // INDEX OF THE ITEM BEING PROCESSED WHEN AN
                           //    ERROR WAS DETECTED (STARTS AT ZERO).
;
#endif /* section security_msb_getlist_ */
#if (defined(psecure_h_security_msb_size_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_security_msb_size_

SECLIBAPI _resident int_16 SECURITY_MSB_SIZE_();
#endif /* section security_msb_size_ */
#if (defined(psecure_h_tsn_app_priv_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_tsn_app_priv_
SECLIBAPI _extensible int_16 TSN_APP_PRIV_(
    void    *msb_ptr_ = NULL,
    int_16  *status_arg = NULL);

#endif /* section tsn_app_priv_ */
#if (defined(psecure_h_tsn_restore_priv_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_tsn_restore_priv_
SECLIBAPI _extensible int_16 TSN_RESTORE_PRIV_(
		void	*msb_ptr_ = NULL,
		int_16	*status_arg = NULL);

#endif /* section tsn_restore_priv_ */
#if (defined(psecure_h_tsn_protinfo_validate_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_tsn_protinfo_validate_
SECLIBAPI _extensible int_16 TSN_PROTINFO_VALIDATE_
  (void    *protinfo_buf_,
   int_32   protinfo_len);
#endif /* section tsn_protinfo_validate_ */
#if (defined(psecure_h_tsn_protinfo_init_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_tsn_protinfo_init_
SECLIBAPI _extensible int_16 TSN_PROTINFO_INIT_(void    *protinfo_buf_,
                                                 int_32  *protinfo_len,
                                                 int_16   type);
#endif /* section tsn_protinfo_init_ */
#if (defined(psecure_h_tsn_protinfo_equal_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_tsn_protinfo_equal_
SECLIBAPI _extensible int_16 TSN_PROTINFO_EQUAL_(void *protinfo_a_,
                                                  void *protinfo_b_);
#endif /* section tsn_protinfo_equal_ */
#if (defined(psecure_h_tsn_protinfo_alter_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_tsn_protinfo_alter_
SECLIBAPI _extensible int_16 TSN_PROTINFO_ALTER_(void *alter_buf_,
                                                  void *protinfo_buf_,
                                                  int_32
                                                       *protinfo_len);
#endif /* section tsn_protinfo_alter_ */
#if (defined(psecure_h_tsn_protinfo_eval_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_tsn_protinfo_eval_

SECLIBAPI _priv _resident _extensible int_16 TSN_PROTINFO_EVAL_
  (void   *msb_ptr_,          // MESSAGE SECURITY BLOCK
   void   *verb_info_ptr_,    // OPERATION (TSN^VERB^STRUCT)
   void   *obj_info_ptr_,     // OBJECT DESCRIPTOR
   int_16 *must_audit,        // CALLER MUST AUDIT RESULT
   void   *col_restrict_ptr_ = NULL) // COLUMN RESTRICTIONS
                               // (TSN^SQLCOL^SIZE^STRUCT)
;
#endif /* section tsn_protinfo_eval_ */
#if (defined(psecure_h_tsn_protinfo_getcolumns_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_tsn_protinfo_getcolumns_

SECLIBAPI _priv _resident _extensible int_16 TSN_PROTINFO_GETCOLUMNS_
  (void    *msb_ptr_,        // MESSAGE SECURITY BLOCK
   void    *permission_ptr_, // TSN^SQLPERM^STRUCT
   void    *protinfo_buf_,   // BLACK BOX
   int_16  *column_buf,      // BUFFER FOR RETURNED DATA
   int_16   column_len)      // ALLOCATED BYTE SIZE OF COLUMN^BUF
;
#endif /* section tsn_protinfo_getcolumns_ */
#if (defined(psecure_h_tsn_protinfo_checkcolumns_) || (!defined(psecure_h_including_section) && !defined(psecure_h_including_self)))
#undef psecure_h_tsn_protinfo_checkcolumns_

SECLIBAPI _extensible int_16 TSN_PROTINFO_CHECKCOLUMNS_
  (int_16  *granted_set, // GRANTED COLUMNS BITMASK
   int_16   req_column)  // COLUMN NUMBER TO TEST
;
#endif /* section tsn_protinfo_checkcolumns_ */
