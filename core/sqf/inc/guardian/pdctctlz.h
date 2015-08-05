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
// PREPROC: start of section: 
#if (defined(pdctctlz_h_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_
//

#include "rosetta/rosgen.h" /* rosetta utilities */
#undef ARG_PRESENT_OMIT
#define ARG_PRESENT_OMIT
#include "fs/fsbuiltin.h"		// for OMIT parameter
#include "nsk/nskcommon.h"


//
#endif
// PREPROC: end of section: 
//
// #pragma section DCT_ENLARGENRL_
//
// PREPROC: start of section: dct_enlargenrl_
#if (defined(pdctctlz_h_dct_enlargenrl_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_enlargenrl_
//
 _priv _resident int_16 DCT_ENLARGENRL_();
//
#endif
// PREPROC: end of section: dct_enlargenrl_
//
// #pragma section DCT_ENLARGEPPL_
//
// PREPROC: start of section: dct_enlargeppl_
#if (defined(pdctctlz_h_dct_enlargeppl_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_enlargeppl_
//

 _priv _resident int_16 DCT_ENLARGEPPL_();
//
#endif
// PREPROC: end of section: dct_enlargeppl_
//
// #pragma section DCT_LOCATE_NAME_
//
// PREPROC: start of section: dct_locate_name_
#if (defined(pdctctlz_h_dct_locate_name_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_locate_name_
//

 _priv _resident _variable int_16 DCT_LOCATE_NAME_
  (unsigned_char *name,
   int_16         namelen,
   int_32        *addr,
   int_16        *bucket,
   int_32        *prev_addr,
   int_16         nrl_index);
//
#endif
// PREPROC: end of section: dct_locate_name_
//
// #pragma section DCT_INITIALIZE_
//
// PREPROC: start of section: dct_initialize_
#if (defined(pdctctlz_h_dct_initialize_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_initialize_
//

 _priv _resident void DCT_INITIALIZE_();
//
#endif
// PREPROC: end of section: dct_initialize_
//
// #pragma section DCT_RESET_
//
// PREPROC: start of section: dct_reset_
#if (defined(pdctctlz_h_dct_reset_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_reset_
//

 _priv _resident void DCT_RESET_();
//
#endif
// PREPROC: end of section: dct_reset_
//
// #pragma section DCT_GLUP_ADDNAME_
//
// PREPROC: start of section: dct_glup_addname_
#if (defined(pdctctlz_h_dct_glup_addname_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_glup_addname_
//

 _priv _resident void DCT_GLUP_ADDNAME_
  (int_16 *dct_entry);
//
#endif
// PREPROC: end of section: dct_glup_addname_
//
// #pragma section DCT_GLUP_UPDATE_
//
// PREPROC: start of section: dct_glup_update_
#if (defined(pdctctlz_h_dct_glup_update_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_glup_update_
//

 _priv _resident void DCT_GLUP_UPDATE_(int_16 *dct_entry);
//
#endif
// PREPROC: end of section: dct_glup_update_
//
// #pragma section DCT_GLUP_DELNAME_
//
// PREPROC: start of section: dct_glup_delname_
#if (defined(pdctctlz_h_dct_glup_delname_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_glup_delname_
//

 _priv _resident void DCT_GLUP_DELNAME_
  (int_16 *dct_entry);
//
#endif
// PREPROC: end of section: dct_glup_delname_
//
// #pragma section DCT_GLUP_RENAME_
//
// PREPROC: start of section: dct_glup_rename_
#if (defined(pdctctlz_h_dct_glup_rename_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_glup_rename_
//

 _priv _resident void DCT_GLUP_RENAME_(int_16 *dct_entry);
//
#endif
// PREPROC: end of section: dct_glup_rename_
//
// #pragma section DCT_GLUP_ADJUSTPIDS_
//
// PREPROC: start of section: dct_glup_adjustpids_
#if (defined(pdctctlz_h_dct_glup_adjustpids_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_glup_adjustpids_
//

 _priv _resident void DCT_GLUP_ADJUSTPIDS_
  (int_16 *dct_entry);
//
#endif
// PREPROC: end of section: dct_glup_adjustpids_
//
// #pragma section DCT_CHECK_VIA_NRL_INDEX_
//
// PREPROC: start of section: dct_check_via_nrl_index_
#if (defined(pdctctlz_h_dct_check_via_nrl_index_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_check_via_nrl_index_
//

 _priv _resident int_16 DCT_CHECK_VIA_NRL_INDEX_
  (int_16  nrl_index);
//
#endif
// PREPROC: end of section: dct_check_via_nrl_index_
//
// #pragma section DCT_CHECK_VIA_PPL_INDEX_
//
// PREPROC: start of section: dct_check_via_ppl_index_
#if (defined(pdctctlz_h_dct_check_via_ppl_index_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_check_via_ppl_index_
//

 _priv _resident int_16 DCT_CHECK_VIA_PPL_INDEX_
  (int_16  ppl_index);
//
#endif
// PREPROC: end of section: dct_check_via_ppl_index_
//
// #pragma section DCT_GLOBALUPDATE_
//
// PREPROC: start of section: dct_globalupdate_
#if (defined(pdctctlz_h_dct_globalupdate_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_globalupdate_
//

 _priv _resident void DCT_GLOBALUPDATE_
  (int_16  *dctent,
   int_16   length);
//
#endif
// PREPROC: end of section: dct_globalupdate_
//
// #pragma section DCT_GLOBALUPDATE_NAMEID_
//
// PREPROC: start of section: dct_globalupdate_nameid_
#if (defined(pdctctlz_h_dct_globalupdate_nameid_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_globalupdate_nameid_
//

 _priv _resident void DCT_GLOBALUPDATE_NAMEID_
  (int_16  *nameid,
   int_16   length);
//
#endif
// PREPROC: end of section: dct_globalupdate_nameid_
//
// #pragma section DCT_DCTENT_TO_NRD_
//
// PREPROC: start of section: dct_dctent_to_nrd_
#if (defined(pdctctlz_h_dct_dctent_to_nrd_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_dctent_to_nrd_
//

 _priv _resident _variable void DCT_DCTENT_TO_NRD_
  (int_16  *dct_entry,
   int_16  *nrd,
   int_16   maxnrdlen,
   int_16   nrl_index);
//
#endif
// PREPROC: end of section: dct_dctent_to_nrd_
//
// #pragma section DCT_COPY_DCT_VIA_NRL_INDEX_
//
// PREPROC: start of section: dct_copy_dct_via_nrl_index_
#if (defined(pdctctlz_h_dct_copy_dct_via_nrl_index_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_copy_dct_via_nrl_index_
//

 _priv _resident int_16 DCT_COPY_DCT_VIA_NRL_INDEX_
  (int_16   nrl_index,
   int_16  *dctent);
//
#endif
// PREPROC: end of section: dct_copy_dct_via_nrl_index_
//
// #pragma section DCT_COPY_DCT_VIA_NAME_
//
// PREPROC: start of section: dct_copy_dct_via_name_
#if (defined(pdctctlz_h_dct_copy_dct_via_name_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_copy_dct_via_name_
//

 _priv _resident int_16 DCT_COPY_DCT_VIA_NAME_
  (unsigned_char *name,
   int_16         namelen,
   int_16        *dctent);
//
#endif
// PREPROC: end of section: dct_copy_dct_via_name_
//
// #pragma section DCT_COPY_DCT_VIA_PP_INDEX_
//
// PREPROC: start of section: dct_copy_dct_via_pp_index_
#if (defined(pdctctlz_h_dct_copy_dct_via_pp_index_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_copy_dct_via_pp_index_
//

 _priv _resident int_16 DCT_COPY_DCT_VIA_PP_INDEX_
  (int_16   ppi,
   int_16  *dctent);
//
#endif
// PREPROC: end of section: dct_copy_dct_via_pp_index_
//
// #pragma section DCT_ADD_PROCESS_
//
// PREPROC: start of section: dct_add_process_
#if (defined(pdctctlz_h_dct_add_process_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_add_process_
//

 _priv _extensible int_16 DCT_ADD_PROCESS_
  (unsigned_char *name,
   int_16         namelen,
   int_16         devsubtype,
   int_16         misc,
   int_16         cpu,
   int_16         pin,
   int_16        *ancestor_phandle,
   int_16        *descriptor,
   int_16         maxnrdlen,
   int_16         options,
   unsigned_char *created_name,
   int_16         created_namemaxlen,
   int_16        *created_namelen,
   unsigned_char *ancestor_name,
   int_16         ancestor_namelen);
//
#endif
// PREPROC: end of section: dct_add_process_
//
// #pragma section DCT_RELEASE_PROCESS_
//
// PREPROC: start of section: dct_release_process_
#if (defined(pdctctlz_h_dct_release_process_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_release_process_
//

 _priv _resident void DCT_RELEASE_PROCESS_
  (int_32  index);
//
#endif
// PREPROC: end of section: dct_release_process_
//
// #pragma section DCT_RELEASE_DEVICE_
//
// PREPROC: start of section: dct_release_device_
#if (defined(pdctctlz_h_dct_release_device_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_release_device_
//

 _priv int_16 DCT_RELEASE_DEVICE_(int_16   index,
                                            fixed_0  verifier);
//
#endif
// PREPROC: end of section: dct_release_device_
//
// #pragma section DCT_DELETE_PROCESS_
//
// PREPROC: start of section: dct_delete_process_
#if (defined(pdctctlz_h_dct_delete_process_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_delete_process_
//

 _priv int_16 DCT_DELETE_PROCESS_(int_16  *phandle,
                                            int_16   pair_stop,
                                            int_32  *dct_index,
                                            int_32   oss_pid,
                                            int_16   flag_word);
//
#endif
// PREPROC: end of section: dct_delete_process_
//
// #pragma section DCT_GET_BY_NAME_
//
// PREPROC: start of section: dct_get_by_name_
#if (defined(pdctctlz_h_dct_get_by_name_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_get_by_name_
//
DllImport int_16 DCT_GET_BY_NAME_
  (unsigned_char *name,
   int_16         namelen,
   int_16        *descriptor,
   int_16         maxnrdlen,
   int_16        *primary_phandle,
   int_16        *backup_phandle,
   unsigned_char *ancestor_name,
   int_16         ancestor_maxnamelen,
   int_16        *ancestor_namelen);
//
#endif
// PREPROC: end of section: dct_get_by_name_
//
// #pragma section DCT_GET_BY_LDEV_
//
// PREPROC: start of section: dct_get_by_ldev_
#if (defined(pdctctlz_h_dct_get_by_ldev_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_get_by_ldev_
//
DllImport int_16 DCT_GET_BY_LDEV_
  (int_32         ldev,
   unsigned_char *name,
   int_16         maxnamelen,
   int_16        *namelen,
   int_16        *descriptor,
   int_16         maxnrdlen,
   int_16        *primary_phandle,
   int_16        *backup_phandle,
   unsigned_char *ancestor_name,
   int_16         ancestor_maxnamelen,
   int_16        *ancestor_namelen);
//
#endif
// PREPROC: end of section: dct_get_by_ldev_
//
// #pragma section DCT_GET_BY_CPU_PIN_
//
// PREPROC: start of section: dct_get_by_cpu_pin_
#if (defined(pdctctlz_h_dct_get_by_cpu_pin_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_get_by_cpu_pin_
//

 _priv _extensible _resident int_16 DCT_GET_BY_CPU_PIN_
  (int_16         cpu,
   int_16         pin,
   unsigned_char *name,
   int_16         maxnamelen,
   int_16        *namelen,
   int_16        *descriptor,
   int_16         maxnrdlen,
   int_16        *primary_phandle,
   int_16        *backup_phandle,
   unsigned_char *ancestor_name,
   int_16         ancestor_maxnamelen,
   int_16        *ancestor_namelen);
//
#endif
// PREPROC: end of section: dct_get_by_cpu_pin_
//
// #pragma section DCT_GETNEXT_ENTRY_
//
// PREPROC: start of section: dct_getnext_entry_
#if (defined(pdctctlz_h_dct_getnext_entry_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_getnext_entry_
//

DllImport
int_16 DCT_GETNEXT_ENTRY_
  (int_32        *index,
   int_16         devtype,
   int_16         devsubtype,
   unsigned_char *pattern,
   int_16         pmax,
   unsigned_char *name,
   int_16         maxnamelen,
   int_16        *namelen,
   int_16        *descriptor,
   int_16         maxnrdlen,
   fat_16         options,
   int_16        *primary_phandle,
   int_16        *backup_phandle,
   unsigned_char *ancestor_name,
   int_16         ancestor_maxnamelen,
   int_16        *ancestor_namelen);
//
#endif
// PREPROC: end of section: dct_getnext_entry_
//
// #pragma section DCT_GETNEXT_IOP_NAME_
//
// PREPROC: start of section: dct_getnext_iop_name_
#if (defined(pdctctlz_h_dct_getnext_iop_name_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_getnext_iop_name_
//

 _callable _extensible _resident int_16 DCT_GETNEXT_IOP_NAME_
  (unsigned_char *prevname,
   int_16         prevnamelen,
   unsigned_char *nextname,
   int_16         maxnamelen,
   int_16        *namelen,
   int_16        *descriptor,
   int_16         maxnrdlen);
//
#endif
// PREPROC: end of section: dct_getnext_iop_name_
//
// #pragma section DCT_UPDATE_XIOANCESTOR_
//
// PREPROC: start of section: dct_update_xioancestor_
#if (defined(pdctctlz_h_dct_update_xioancestor_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_update_xioancestor_
//

 _priv _resident int_16 DCT_UPDATE_XIOANCESTOR_
  (int_32         pp_index,
   int_16        *anc_phandle,
   unsigned_char *anc_name,
   int_16         namelen,
   int_16         anc_is_iop);
//
#endif
// PREPROC: end of section: dct_update_xioancestor_
//
// #pragma section DCT_UPDATE_
//
// PREPROC: start of section: dct_update_
#if (defined(pdctctlz_h_dct_update_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_update_
//
DllImport int_16 DCT_UPDATE_
  (unsigned_char *name,
   int_16         namelen,
   fixed_0        verifier,
   int_16         devsubtype,
   int_16         recsize,
   int_16         oreqid,
   int_16         misc,
   int_16         cpu,
   int_16         pin,
   int_32         ldev,
   int_16        *ancestor_phandle,
   unsigned_char *ancestor_name,
   int_16         ancestor_namelen);
//
DllImport int_16 DCT_UPDATE1_
  (unsigned_char *name,
   int_16         namelen,
   fixed_0        verifier,
   int_16         devsubtype,
   int_16         recsize,
   int_16         oreqid,
   int_16         misc,
   int_16         cpu,
   int_16         pin,
   int_32         ldev,
   int_16        *ancestor_phandle,
   unsigned_char *ancestor_name,
   int_16         ancestor_namelen);
//
#endif
// PREPROC: end of section: dct_update_
//
// #pragma section DCT_GET_BY_LOCAL_PHANDLE_
//
// PREPROC: start of section: dct_get_by_local_phandle_
#if (defined(pdctctlz_h_dct_get_by_local_phandle_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_get_by_local_phandle_
//

 _priv _extensible _resident int_16 DCT_GET_BY_LOCAL_PHANDLE_
  (int_16        *phandle,
   unsigned_char *name,
   int_16         maxnamelen,
   int_16        *namelen,
   int_16        *descriptor,
   int_16         maxnrdlen,
   int_16        *primary_phandle,
   int_16        *backup_phandle,
   unsigned_char *ancestor_name,
   int_16         ancestor_maxnamelen,
   int_16        *ancestor_namelen);
//
#endif
// PREPROC: end of section: dct_get_by_local_phandle_
//
// #pragma section DCT_RENAME_
//
// PREPROC: start of section: dct_rename_
#if (defined(pdctctlz_h_dct_rename_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_rename_
//

 _priv _resident _extensible int_16 DCT_RENAME_
  (unsigned_char *oldname,
   int_16         oldnamelen,
   unsigned_char *newname,
   int_16         newnamelen,
   int_32         ldev,
   fixed_0        verifier);
//
#endif
// PREPROC: end of section: dct_rename_
//
// #pragma section DCT_SET_BACKUP_
//
// PREPROC: start of section: dct_set_backup_
#if (defined(pdctctlz_h_dct_set_backup_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_set_backup_
//

 _priv _resident int_16 DCT_SET_BACKUP_
  (int_16 *phandle);
//
#endif
// PREPROC: end of section: dct_set_backup_
//
// #pragma section DCT_SET_ME_PRIMARY_
//
// PREPROC: start of section: dct_set_me_primary_
#if (defined(pdctctlz_h_dct_set_me_primary_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_set_me_primary_
//
 _priv _resident _extensible int_16 DCT_SET_ME_PRIMARY_();
//
#endif
// PREPROC: end of section: dct_set_me_primary_
//
// #pragma section DCT_GET_BROTHER_PHANDLE_
//
// PREPROC: start of section: dct_get_brother_phandle_
#if (defined(pdctctlz_h_dct_get_brother_phandle_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_get_brother_phandle_
//

 _priv _resident _extensible int_16 DCT_GET_BROTHER_PHANDLE_
  (int_16 *phandle);
//
#endif
// PREPROC: end of section: dct_get_brother_phandle_
//
// #pragma section DCT_FORMAT_LOOKUP_BY_NAME_
//
// PREPROC: start of section: dct_format_lookup_by_name_
#if (defined(pdctctlz_h_dct_format_lookup_by_name_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_format_lookup_by_name_
//

DllImport
int_16 DCT_FORMAT_LOOKUP_BY_NAME_
  (unsigned_char *name,
   int_16         namelen,
   fat_32         nodeid,
   int_16        *rqst_ctrl,
   int_16         rqst_ctrl_maxlength,
   int_16        *rqst_ctrl_reqlength,
   int_16        *rqst_data,
   int_16         rqst_data_maxlength,
   int_16        *rqst_data_reqlength,
   int_16        *reply_ctrl_reqlength,
   int_16        *reply_data_reqlength,
   int_16        *server);
//
#endif
// PREPROC: end of section: dct_format_lookup_by_name_
//
// #pragma section DCT_FORMAT_LOOKUP_BY_LDEV_
//
// PREPROC: start of section: dct_format_lookup_by_ldev_
#if (defined(pdctctlz_h_dct_format_lookup_by_ldev_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_format_lookup_by_ldev_
//

 _priv _extensible _resident int_16 DCT_FORMAT_LOOKUP_BY_LDEV_
  (int_32   ldev,
   int_32   nodeid,
   int_16  *rqst_ctrl,
   int_16   rqst_ctrl_maxlength,
   int_16  *rqst_ctrl_reqlength,
   int_16  *rqst_data,
   int_16   rqst_data_maxlength,
   int_16  *rqst_data_reqlength,
   int_16  *reply_ctrl_reqlength,
   int_16  *reply_data_reqlength,
   int_16  *server);
//
#endif
// PREPROC: end of section: dct_format_lookup_by_ldev_
//
// #pragma section DCT_FORMAT_LOOKUP_BY_PHANDLE_
//
// PREPROC: start of section: dct_format_lookup_by_phandle_
#if (defined(pdctctlz_h_dct_format_lookup_by_phandle_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_format_lookup_by_phandle_
//

 _priv _extensible _resident
  int_16 DCT_FORMAT_LOOKUP_BY_PHANDLE_(int_16  *phandle,
                                       int_16  *rqst_ctrl,
                                       int_16   rqst_ctrl_maxlength,
                                       int_16  *rqst_ctrl_reqlength,
                                       int_16  *rqst_data,
                                       int_16   rqst_data_maxlength,
                                       int_16  *rqst_data_reqlength,
                                       int_16  *reply_ctrl_reqlength,
                                       int_16  *reply_data_reqlength,
                                       int_16  *server);
//
#endif
// PREPROC: end of section: dct_format_lookup_by_phandle_
//
// #pragma section DCT_FORMAT_FIND_NEXT_
//
// PREPROC: start of section: dct_format_find_next_
#if (defined(pdctctlz_h_dct_format_find_next_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_format_find_next_
//

DllImport
int_16 DCT_FORMAT_FIND_NEXT_
  (int_32         index,
   fat_32         nodeid,
   int_16         devtype,
   int_16         devsubtype,
   unsigned_char *pattern,
   int_16         pmax,
   int_16        *rqst_ctrl,
   int_16         rqst_ctrl_maxlength,
   int_16        *rqst_ctrl_reqlength,
   int_16        *rqst_data,
   int_16         rqst_data_maxlength,
   int_16        *rqst_data_reqlength,
   int_16        *reply_ctrl_reqlength,
   int_16        *reply_data_reqlength,
   int_16        *server,
   fat_16         options);
//
#endif
// PREPROC: end of section: dct_format_find_next_
//
// #pragma section DCT_INTERPRET_LOOKUP_
//
// PREPROC: start of section: dct_interpret_lookup_
#if (defined(pdctctlz_h_dct_interpret_lookup_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_interpret_lookup_
//

DllImport
int_16 DCT_INTERPRET_LOOKUP_
  (int_16        *reply_ctrl_buffer,
   int_16        *reply_data_buffer,
   int_16        *primary_phandle,
   unsigned_char *name,
   int_16         maxnamelen,
   int_16        *namelen,
   int_16        *descriptor,
   int_16         maxnrdlen,
   fat_16         options,
   int_16        *backup_phandle,
   unsigned_char *ancestor_name,
   int_16         ancestor_maxnamelen,
   int_16        *ancestor_namelen);
//
#endif
// PREPROC: end of section: dct_interpret_lookup_
//
// #pragma section DCT_WAITED_FIND_BY_LDEV_
//
// PREPROC: start of section: dct_waited_find_by_ldev_
#if (defined(pdctctlz_h_dct_waited_find_by_ldev_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_waited_find_by_ldev_
//
DllImport int_16 DCT_WAITED_FIND_BY_LDEV_
  (int_32         ldev,
   fat_32         nodeid,
   int_16        *primary_phandle,
   unsigned_char *name,
   int_16         maxnamelen,
   int_16        *namelen,
   int_16        *descriptor,
   int_16         maxnrdlen,
   int_16        *backup_phandle,
   unsigned_char *ancestor_name,
   int_16         ancestor_maxnamelen,
   int_16        *ancestor_namelen);
//
#endif
// PREPROC: end of section: dct_waited_find_by_ldev_
//
// #pragma section DCT_WAITED_FIND_BY_PHANDLE_
//
// PREPROC: start of section: dct_waited_find_by_phandle_
#if (defined(pdctctlz_h_dct_waited_find_by_phandle_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_waited_find_by_phandle_
//
DllImport
int_16 DCT_WAITED_FIND_BY_PHANDLE_(int_16        *phandle,
                                   unsigned_char *name,
                                   int_16         maxnamelen,
                                   int_16        *namelen,
                                   int_16        *descriptor,
                                   int_16         maxnrdlen,
                                   int_16        *primary_phandle,
                                   int_16        *backup_phandle,
                                   unsigned_char *ancestor_name,
                                   int_16         ancestor_maxnamelen,
                                   int_16        *ancestor_namelen);
//
#endif
// PREPROC: end of section: dct_waited_find_by_phandle_
//
// #pragma section DCT_WAITED_FIND_BY_NAME_
//
// PREPROC: start of section: dct_waited_find_by_name_
#if (defined(pdctctlz_h_dct_waited_find_by_name_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_waited_find_by_name_
//
DllImport int_16 DCT_WAITED_FIND_BY_NAME_
  (unsigned_char *name,
   int_16         namelen,
   fat_32         nodeid,
   int_16        *primary_phandle,
   int_16        *descriptor,
   int_16         maxnrdlen,
   unsigned_char *outname,
   int_16         outname_maxnamelen,
   int_16        *outname_namelen,
   int_16        *backup_phandle,
   unsigned_char *ancestor_name,
   int_16         ancestor_maxnamelen,
   int_16        *ancestor_namelen);
//
#endif
// PREPROC: end of section: dct_waited_find_by_name_
//
// #pragma section DCT_WAITED_FIND_NEXT_
//
// PREPROC: start of section: dct_waited_find_next_
#if (defined(pdctctlz_h_dct_waited_find_next_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_waited_find_next_
//

 _extensible _callable int_16 DCT_WAITED_FIND_NEXT_
  (int_32        *index,
   int_32         nodeid,
   int_16         devtype,
   int_16         devsubtype,
   unsigned_char *pattern,
   int_16         pmax,
   int_16        *primary_phandle,
   unsigned_char *name,
   int_16         maxnamelen,
   int_16        *namelen,
   int_16        *descriptor,
   int_16         maxnrdlen,
   int_16         options,
   int_16        *backup_phandle,
   unsigned_char *ancestor_name,
   int_16         ancestor_maxnamelen,
   int_16        *ancestor_namelen);
//
#endif
// PREPROC: end of section: dct_waited_find_next_
//
// #pragma section DCT_ADD_DEVICE_
//
// PREPROC: start of section: dct_add_device_
#if (defined(pdctctlz_h_dct_add_device_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_add_device_
//

 _extensible _callable int_16 DCT_ADD_DEVICE_
  (unsigned_char *newname,
   int_16         namelen,
   int_16         devtype,
   int_16         devsubtype,
   int_16         recsize,
   int_16         misc,
   int_16         cpu1,
   int_16         cpu2,
   int_16        *ancestor,
   unsigned_char *multiname,
   int_16         mlen,
   int_16        *descriptor,
   int_16         maxnrdlen,
   int_16         object_type);
//
#endif
// PREPROC: end of section: dct_add_device_
//
// #pragma section DCT_DELETE_DEVICE_
//
// PREPROC: start of section: dct_delete_device_
#if (defined(pdctctlz_h_dct_delete_device_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_delete_device_
//

 _extensible _callable int_16 DCT_DELETE_DEVICE_
  (unsigned_char *name,
   int_16         namelen,
   fixed_0        verifier,
   int_16         override,
   int_16         object_type);
//
#endif
// PREPROC: end of section: dct_delete_device_
//
// #pragma section DCT_VERIFY_
//
// PREPROC: start of section: dct_verify_
#if (defined(pdctctlz_h_dct_verify_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_verify_
//

 _priv _resident int_16 DCT_VERIFY_(int_16  index);
//
#endif
// PREPROC: end of section: dct_verify_
//
// #pragma section DCT_VERIFY_GLOBAL_
//
// PREPROC: start of section: dct_verify_global_
#if (defined(pdctctlz_h_dct_verify_global_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_verify_global_
//

 _priv _resident int_16 DCT_VERIFY_GLOBAL_
  (int_16 *dctent);
//
#endif
// PREPROC: end of section: dct_verify_global_
//
// #pragma section CREATEPROCESSNAME
//
// PREPROC: start of section: createprocessname
#if (defined(pdctctlz_h_createprocessname) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_createprocessname
//

 _callable _cc_status CREATEPROCESSNAME
  (int_16 *name);
//
#endif
// PREPROC: end of section: createprocessname
//
// #pragma section GETPPDENTRY
//
// PREPROC: start of section: getppdentry
#if (defined(pdctctlz_h_getppdentry) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_getppdentry
//

 _callable _cc_status GETPPDENTRY(int_16   index,
                                            int_16   sysid,
                                            int_16  *ppdentry);
//
#endif
// PREPROC: end of section: getppdentry
//
// #pragma section LOOKUPPROCESSNAME
//
// PREPROC: start of section: lookupprocessname
#if (defined(pdctctlz_h_lookupprocessname) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_lookupprocessname
//

 _callable _cc_status LOOKUPPROCESSNAME
  (int_16 *name_);
//
#endif
// PREPROC: end of section: lookupprocessname
//
// #pragma section GETDEVNAME
//
// PREPROC: start of section: getdevname
#if (defined(pdctctlz_h_getdevname) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_getdevname
//

 _extensible_n(3) _callable int_16 GETDEVNAME
  (int_16  *ldev,
   int_16  *name,
   int_16   sysid,
   int_16   type,
   int_16   stype);
//
#endif
// PREPROC: end of section: getdevname
//
// #pragma section CREATEREMOTENAME
//
// PREPROC: start of section: createremotename
#if (defined(pdctctlz_h_createremotename) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_createremotename
//

 _callable _cc_status CREATEREMOTENAME(int_16  *name,
                                                 int_16   sysid);
//
#endif
// PREPROC: end of section: createremotename
//
// #pragma section DCT_VERIFIERS_AT_COLDLOAD_
//
// PREPROC: start of section: dct_verifiers_at_coldload_
#if (defined(pdctctlz_h_dct_verifiers_at_coldload_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_verifiers_at_coldload_
//

 _priv _resident void DCT_VERIFIERS_AT_COLDLOAD_();
//
#endif
// PREPROC: end of section: dct_verifiers_at_coldload_
//
// #pragma section DCT_VERIFIERS_AT_RELOAD_
//
// PREPROC: start of section: dct_verifiers_at_reload_
#if (defined(pdctctlz_h_dct_verifiers_at_reload_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_verifiers_at_reload_
//

 _priv _resident void DCT_VERIFIERS_AT_RELOAD_();
//
#endif
// PREPROC: end of section: dct_verifiers_at_reload_
//
// #pragma section DCT_GLUP_BUMP_VERIFIER_
//
// PREPROC: start of section: dct_glup_bump_verifier_
#if (defined(pdctctlz_h_dct_glup_bump_verifier_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_glup_bump_verifier_
//

 _priv _resident void DCT_GLUP_BUMP_VERIFIER_
  (fixed_0 *new_value,
   int_16   length);
//
#endif
// PREPROC: end of section: dct_glup_bump_verifier_
//
// #pragma section DCT_GLUP_ASSIGN_NEW_VERIFIER_
//
// PREPROC: start of section: dct_glup_assign_new_verifier_
#if (defined(pdctctlz_h_dct_glup_assign_new_verifier_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_glup_assign_new_verifier_
//

 _priv _resident void DCT_GLUP_ASSIGN_NEW_VERIFIER_
  (int_16  *ppl_index,
   int_16   length);
//
#endif
// PREPROC: end of section: dct_glup_assign_new_verifier_
//
// #pragma section DCT_GET_UNAMED_PROCESS_VERIFIER
//
// PREPROC: start of section: dct_get_unamed_process_verifier
#if (defined(pdctctlz_h_dct_get_unamed_process_verifier) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_get_unamed_process_verifier
//

 _priv _resident fixed_0 DCT_GET_UNAMED_PROCESS_VERIFIER();
//
#endif
// PREPROC: end of section: dct_get_unamed_process_verifier
//
// #pragma section DCT_VERIFIERS_FOR_RELOAD_
//
// PREPROC: start of section: dct_verifiers_for_reload_
#if (defined(pdctctlz_h_dct_verifiers_for_reload_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_verifiers_for_reload_
//

 _priv _resident void DCT_VERIFIERS_FOR_RELOAD_
  (int_16  cpu);
//
#endif
// PREPROC: end of section: dct_verifiers_for_reload_
//
// #pragma section DCT_CALCULATE_NEW_VERIFIER_
//
// PREPROC: start of section: dct_calculate_new_verifier_
#if (defined(pdctctlz_h_dct_calculate_new_verifier_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_calculate_new_verifier_
//

 _priv _resident fixed_0 DCT_CALCULATE_NEW_VERIFIER_
  (fixed_0  bump_count);
//
#endif
// PREPROC: end of section: dct_calculate_new_verifier_
//
// #pragma section DCT_NRD_AND_NAME_TO_PPD_
//
// PREPROC: start of section: dct_nrd_and_name_to_ppd_
#if (defined(pdctctlz_h_dct_nrd_and_name_to_ppd_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_nrd_and_name_to_ppd_
//

 _resident _extensible int_16 DCT_NRD_AND_NAME_TO_PPD_
  (int_16        *nrd,
   unsigned_char *name,
   int_16         namelen,
   int_16        *ppd,
   unsigned_char *ancestor_name,
   int_16         ancestor_namelen);
//
#endif
// PREPROC: end of section: dct_nrd_and_name_to_ppd_
//
// #pragma section DCT_GET_PROCESS_BY_INDEX_
//
// PREPROC: start of section: dct_get_process_by_index_
#if (defined(pdctctlz_h_dct_get_process_by_index_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_get_process_by_index_
//

 _priv _extensible _resident int_16 DCT_GET_PROCESS_BY_INDEX_
  (int_32         index,
   unsigned_char *name,
   int_16         maxnamelen,
   int_16        *namelen,
   int_16        *descriptor,
   int_16         maxnrdlen,
   int_16        *primary_phandle,
   int_16        *backup_phandle,
   unsigned_char *ancestor_name,
   int_16         ancestor_maxnamelen,
   int_16        *ancestor_namelen);
//
#endif
// PREPROC: end of section: dct_get_process_by_index_
//
// #pragma section DCT_DELETE_FOX_TABLEENTRY_CXX_
//
// PREPROC: start of section: dct_delete_fox_tableentry_cxx_
#if (defined(pdctctlz_h_dct_delete_fox_tableentry_cxx_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_delete_fox_tableentry_cxx_
//

 _priv void DCT_DELETE_FOX_TABLEENTRY_CXX_
  (int_16  index);
//
#endif
// PREPROC: end of section: dct_delete_fox_tableentry_cxx_
//
// #pragma section DCT_CONTROL_4CHAR_NAMES_
//
// PREPROC: start of section: dct_control_4char_names_
#if (defined(pdctctlz_h_dct_control_4char_names_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_control_4char_names_
//

 _priv _resident void DCT_CONTROL_4CHAR_NAMES_
  (int_16  flag_word);
//
#endif
// PREPROC: end of section: dct_control_4char_names_
//
// #pragma section DCT_GENERATE_4CHARNAME_
//
// PREPROC: start of section: dct_generate_4charname_
#if (defined(pdctctlz_h_dct_generate_4charname_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_generate_4charname_
//

 _priv int_16 DCT_GENERATE_4CHARNAME_
  (unsigned_char *name,
   int_16         maxnamelen,
   int_16        *namelen,
   int_16        *dct_4char_id);
//
#endif
// PREPROC: end of section: dct_generate_4charname_
//
// #pragma section DCT_GENERATE_5CHARNAME_
//
// PREPROC: start of section: dct_generate_5charname_
#if (defined(pdctctlz_h_dct_generate_5charname_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_generate_5charname_
//

 _priv int_16 DCT_GENERATE_5CHARNAME_
  (unsigned_char *name,
   int_16         maxnamelen,
   int_16        *namelen,
   int_16        *dct_5char_id);
//
#endif
// PREPROC: end of section: dct_generate_5charname_
//
// #pragma section PROCESSNAME_CREATE_
//
// PREPROC: start of section: processname_create_
#if (defined(pdctctlz_h_processname_create_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_processname_create_
//
#ifdef __cplusplus
 extern "C"
#endif
 DllImport
 int_16 PROCESSNAME_CREATE_
  (unsigned_char *name,
   int_16         maxnamelen,
   int_16        *namelen,
   int_16         name_type,
   unsigned_char *nodename,
   int_16         nodenamelen,
   int_16         options);
//
#endif
// PREPROC: end of section: processname_create_
//
// #pragma section DCT_CONVERT_LDEV_TO_ASCII_
//
// PREPROC: start of section: dct_convert_ldev_to_ascii_
#if (defined(pdctctlz_h_dct_convert_ldev_to_ascii_) || (!defined(pdctctlz_h_including_section) && !defined(pdctctlz_h_including_self)))
#undef pdctctlz_h_dct_convert_ldev_to_ascii_
//

 DllImport
 int_16 DCT_CONVERT_LDEV_TO_ASCII_
  (int_32         ldev,
   unsigned_char *name,
   int_16         maxnamelen,
   int_16        *namelen); //~ source file above = $QUINCE.GRZDV.SDCTCTL

#endif
// PREPROC: end of section: dct_convert_ldev_to_ascii_
//
//
#if (!defined(pdctctlz_h_including_self))
#undef pdctctlz_h_including_section
#endif
// end of file
