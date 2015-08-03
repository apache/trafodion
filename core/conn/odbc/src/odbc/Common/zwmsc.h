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
/* SCHEMA PRODUCED DATE - TIME : 7/07/2010 - 01:22:23 */
#pragma section zspi_ssn_zwms
/* Constant ZSPI-SSN-ZWMS created on 07/07/2010 at 01:22 */
#define ZSPI_SSN_ZWMS 165
#pragma section zwms_val_owner
/* Constant ZWMS-VAL-OWNER created on 07/07/2010 at 01:22 */
#define ZWMS_VAL_OWNER "TANDEM  "
#pragma section zwms_val_number
/* Constant ZWMS-VAL-NUMBER created on 07/07/2010 at 01:22 */
#define ZWMS_VAL_NUMBER 165
#pragma section zwms_val_version
/* Constant ZWMS-VAL-VERSION created on 07/07/2010 at 01:22 */
#define ZWMS_VAL_VERSION 19997U
#pragma section zwms_val_ext_ssid
/* Constant ZWMS-VAL-EXT-SSID created on 07/07/2010 at 01:22 */
#define ZWMS_VAL_EXT_SSID "TANDEM.165.N29"
#pragma section zwms_val_ssid
/* Definition ZWMS-VAL-SSID created on 07/07/2010 at 01:22 */
#pragma fieldalign shared2 __zwms_val_ssid
typedef struct __zwms_val_ssid
{
   union
   {
      char                            z_filler[8];
      /*value is "TANDEM  "*/
      zspi_ddl_char8_def              z_owner;
   } u_z_filler;
   zspi_ddl_int_def                z_number;
   /*value is 165*/
   zspi_ddl_uint_def               z_version;
   /*value is 19997*/
} zwms_val_ssid_def;
#define zwms_val_ssid_def_Size 12
#pragma section zwms_tnm_eventnumber
/* Constant ZWMS-TNM-EVENTNUMBER created on 07/07/2010 at 01:22 */
#define ZWMS_TNM_EVENTNUMBER 2010
#pragma section zwms_tnm_component
/* Constant ZWMS-TNM-COMPONENT created on 07/07/2010 at 01:22 */
#define ZWMS_TNM_COMPONENT 2011
#pragma section zwms_tnm_objectref
/* Constant ZWMS-TNM-OBJECTREF created on 07/07/2010 at 01:22 */
#define ZWMS_TNM_OBJECTREF 2012
#pragma section zwms_tnm_eventtype
/* Constant ZWMS-TNM-EVENTTYPE created on 07/07/2010 at 01:22 */
#define ZWMS_TNM_EVENTTYPE 3000
#pragma section zwms_tnm_experience_level
/* Constant ZWMS-TNM-EXPERIENCE-LEVEL created on 07/07/2010 at 01:22 */
#define ZWMS_TNM_EXPERIENCE_LEVEL 3013
#pragma section zwms_tnm_severity
/* Constant ZWMS-TNM-SEVERITY created on 07/07/2010 at 01:22 */
#define ZWMS_TNM_SEVERITY 3014
#pragma section zwms_tnm_event_target
/* Constant ZWMS-TNM-EVENT-TARGET created on 07/07/2010 at 01:22 */
#define ZWMS_TNM_EVENT_TARGET 3015
#pragma section zwms_evt_cs_stop_srvr_err
/* Constant ZWMS-EVT-CS-STOP-SRVR-ERR created on 07/07/2010 at 01:22 */
#define ZWMS_EVT_CS_STOP_SRVR_ERR 21003
#pragma section zwms_evt_cs_stsrv_cntxt_fail
/* Constant ZWMS-EVT-CS-STSRV-CNTXT-FAIL created on 07/07/2010 at 01:22 */
#define ZWMS_EVT_CS_STSRV_CNTXT_FAIL 21017
#pragma section zwms_evt_cs_trace_info
/* Constant ZWMS-EVT-CS-TRACE-INFO created on 07/07/2010 at 01:22 */
#define ZWMS_EVT_CS_TRACE_INFO 21034
#pragma section zwms_evt_cs_krypton_err
/* Constant ZWMS-EVT-CS-KRYPTON-ERR created on 07/07/2010 at 01:22 */
#define ZWMS_EVT_CS_KRYPTON_ERR 20005
#pragma section zwms_evt_cs_nsk_err
/* Constant ZWMS-EVT-CS-NSK-ERR created on 07/07/2010 at 01:22 */
#define ZWMS_EVT_CS_NSK_ERR 20007
#pragma section zwms_evt_cs_srvr_env
/* Constant ZWMS-EVT-CS-SRVR-ENV created on 07/07/2010 at 01:22 */
#define ZWMS_EVT_CS_SRVR_ENV 20008
#pragma section zwms_evt_cs_malloc_err
/* Constant ZWMS-EVT-CS-MALLOC-ERR created on 07/07/2010 at 01:22 */
#define ZWMS_EVT_CS_MALLOC_ERR 20009
#pragma section zwms_ddl_eventnumber_enum
/* Definition ZWMS-DDL-EVENTNUMBER-ENUM created on 07/07/2010 at 01:22 */
enum
{
   zwms_enm_cs_stop_srvr_err = 21003,
   zwms_enm_cs_stsrv_cntxt_fail = 21017,
   zwms_enm_cs_trace_info = 21034,
   zwms_enm_cs_krypton_err = 20005,
   zwms_enm_cs_nsk_err = 20007,
   zwms_enm_cs_srvr_env = 20008,
   zwms_enm_cs_malloc_err = 20009
};
typedef short                           zwms_ddl_eventnumber_enum_def;
#pragma section zwms_tkn_component
/* Token Code ZWMS-TKN-COMPONENT created on 07/07/2010 at 01:22 */
#define ZWMS_TKN_COMPONENT 33490907LU
#pragma section zwms_tkn_objectref
/* Token Code ZWMS-TKN-OBJECTREF created on 07/07/2010 at 01:22 */
#define ZWMS_TKN_OBJECTREF 33490908LU
#pragma section zwms_tkn_eventtype
/* Token Code ZWMS-TKN-EVENTTYPE created on 07/07/2010 at 01:22 */
#define ZWMS_TKN_EVENTTYPE 33491896LU
#pragma section zwms_tkn_experience_level
/* Token Code ZWMS-TKN-EXPERIENCE-LEVEL created on 07/07/2010 at 01:22 */
#define ZWMS_TKN_EXPERIENCE_LEVEL 33491909LU
#pragma section zwms_tkn_severity
/* Token Code ZWMS-TKN-SEVERITY created on 07/07/2010 at 01:22 */
#define ZWMS_TKN_SEVERITY 33491910LU
#pragma section zwms_tkn_event_target
/* Token Code ZWMS-TKN-EVENT-TARGET created on 07/07/2010 at 01:22 */
#define ZWMS_TKN_EVENT_TARGET 33491911LU
#pragma section zwms_evt_stop_srvr_err_det_01
/* Constant ZWMS-EVT-STOP-SRVR-ERR-DET-01 created on 07/07/2010 at 01:22 */
#define ZWMS_EVT_STOP_SRVR_ERR_DET_01 210030001L
#pragma section zwms_evt_cntxt_fail_det_01
/* Constant ZWMS-EVT-CNTXT-FAIL-DET-01 created on 07/07/2010 at 01:22 */
#define ZWMS_EVT_CNTXT_FAIL_DET_01 210170001L
#pragma section zwms_evt_trace_info_det_01
/* Constant ZWMS-EVT-TRACE-INFO-DET-01 created on 07/07/2010 at 01:22 */
#define ZWMS_EVT_TRACE_INFO_DET_01 210340001L
#pragma section zwms_evt_krypton_err_det_01
/* Constant ZWMS-EVT-KRYPTON-ERR-DET-01 created on 07/07/2010 at 01:22 */
#define ZWMS_EVT_KRYPTON_ERR_DET_01 200050001L
#pragma section zwms_evt_nsk_err_det_01
/* Constant ZWMS-EVT-NSK-ERR-DET-01 created on 07/07/2010 at 01:22 */
#define ZWMS_EVT_NSK_ERR_DET_01 200070001L
#pragma section zwms_evt_srvr_env_det_01
/* Constant ZWMS-EVT-SRVR-ENV-DET-01 created on 07/07/2010 at 01:22 */
#define ZWMS_EVT_SRVR_ENV_DET_01 200080001L
#pragma section zwms_evt_malloc_err_det_01
/* Constant ZWMS-EVT-MALLOC-ERR-DET-01 created on 07/07/2010 at 01:22 */
#define ZWMS_EVT_MALLOC_ERR_DET_01 200090001L
