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
/* SCHEMA PRODUCED DATE - TIME : 7/07/2010 - 01:20:50 */
#pragma section zspi_ssn_zmxo
/* Constant ZSPI-SSN-ZMXO created on 07/07/2010 at 01:20 */
#define ZSPI_SSN_ZMXO 231
#pragma section zmxo_val_owner
/* Constant ZMXO-VAL-OWNER created on 07/07/2010 at 01:20 */
#define ZMXO_VAL_OWNER "TANDEM  "
#pragma section zmxo_val_number
/* Constant ZMXO-VAL-NUMBER created on 07/07/2010 at 01:20 */
#define ZMXO_VAL_NUMBER 231
#pragma section zmxo_val_version
/* Constant ZMXO-VAL-VERSION created on 07/07/2010 at 01:20 */
#define ZMXO_VAL_VERSION 19997U
#pragma section zmxo_val_ext_ssid
/* Constant ZMXO-VAL-EXT-SSID created on 07/07/2010 at 01:20 */
#define ZMXO_VAL_EXT_SSID "TANDEM.231.N29"
#pragma section zmxo_val_ssid
/* Definition ZMXO-VAL-SSID created on 07/07/2010 at 01:20 */
#pragma fieldalign shared2 __zmxo_val_ssid
typedef struct __zmxo_val_ssid
{
   union
   {
      char                            z_filler[8];
      /*value is "TANDEM  "*/
      zspi_ddl_char8_def              z_owner;
   } u_z_filler;
   zspi_ddl_int_def                z_number;
   /*value is 231*/
   zspi_ddl_uint_def               z_version;
   /*value is 19997*/
} zmxo_val_ssid_def;
#define zmxo_val_ssid_def_Size 12
#pragma section zmxo_tnm_eventnumber
/* Constant ZMXO-TNM-EVENTNUMBER created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_EVENTNUMBER 2010
#pragma section zmxo_tnm_component
/* Constant ZMXO-TNM-COMPONENT created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_COMPONENT 2011
#pragma section zmxo_tnm_objectref
/* Constant ZMXO-TNM-OBJECTREF created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_OBJECTREF 2012
#pragma section zmxo_tnm_datasource
/* Constant ZMXO-TNM-DATASOURCE created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DATASOURCE 2013
#pragma section zmxo_tnm_srvc_initialized
/* Constant ZMXO-TNM-SRVC-INITIALIZED created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SRVC_INITIALIZED 2014
#pragma section zmxo_tnm_srvc_started
/* Constant ZMXO-TNM-SRVC-STARTED created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SRVC_STARTED 2015
#pragma section zmxo_tnm_cfg_srvr_init
/* Constant ZMXO-TNM-CFG-SRVR-INIT created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_CFG_SRVR_INIT 2016
#pragma section zmxo_tnm_ds_started
/* Constant ZMXO-TNM-DS-STARTED created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DS_STARTED 2017
#pragma section zmxo_tnm_ds_stopping
/* Constant ZMXO-TNM-DS-STOPPING created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DS_STOPPING 2018
#pragma section zmxo_tnm_ds_stopped
/* Constant ZMXO-TNM-DS-STOPPED created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DS_STOPPED 2019
#pragma section zmxo_tnm_rg_over_limit
/* Constant ZMXO-TNM-RG-OVER-LIMIT created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_RG_OVER_LIMIT 2020
#pragma section zmxo_tnm_srvc_stopped
/* Constant ZMXO-TNM-SRVC-STOPPED created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SRVC_STOPPED 2021
#pragma section zmxo_tnm_reg_srvr_err
/* Constant ZMXO-TNM-REG-SRVR-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_REG_SRVR_ERR 2022
#pragma section zmxo_tnm_srvst_chng_err
/* Constant ZMXO-TNM-SRVST-CHNG-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SRVST_CHNG_ERR 2023
#pragma section zmxo_tnm_start_srvr_err
/* Constant ZMXO-TNM-START-SRVR-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_START_SRVR_ERR 2024
#pragma section zmxo_tnm_stop_srvr_err
/* Constant ZMXO-TNM-STOP-SRVR-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_STOP_SRVR_ERR 2025
#pragma section zmxo_tnm_dsst_chng_fail
/* Constant ZMXO-TNM-DSST-CHNG-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DSST_CHNG_FAIL 2026
#pragma section zmxo_tnm_port_notavail
/* Constant ZMXO-TNM-PORT-NOTAVAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_PORT_NOTAVAIL 2027
#pragma section zmxo_tnm_srvc_strt_fail
/* Constant ZMXO-TNM-SRVC-STRT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SRVC_STRT_FAIL 2028
#pragma section zmxo_tnm_srvc_strt_info
/* Constant ZMXO-TNM-SRVC-STRT-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SRVC_STRT_INFO 2029
#pragma section zmxo_tnm_ds_strt_fail
/* Constant ZMXO-TNM-DS-STRT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DS_STRT_FAIL 2030
#pragma section zmxo_tnm_ds_strt_info
/* Constant ZMXO-TNM-DS-STRT-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DS_STRT_INFO 2031
#pragma section zmxo_tnm_svc_stopped_info
/* Constant ZMXO-TNM-SVC-STOPPED-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SVC_STOPPED_INFO 2032
#pragma section zmxo_tnm_ds_stopped_info
/* Constant ZMXO-TNM-DS-STOPPED-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DS_STOPPED_INFO 2033
#pragma section zmxo_tnm_stsrv_cntxt_fail
/* Constant ZMXO-TNM-STSRV-CNTXT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_STSRV_CNTXT_FAIL 2034
#pragma section zmxo_tnm_srv_strt_indbg
/* Constant ZMXO-TNM-SRV-STRT-INDBG created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SRV_STRT_INDBG 2035
#pragma section zmxo_tnm_srv_st_chng_info
/* Constant ZMXO-TNM-SRV-ST-CHNG-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SRV_ST_CHNG_INFO 2036
#pragma section zmxo_tnm_ds_stopping
/* Constant ZMXO-TNM-DS-STOPPING created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DS_STOPPING 2037
#pragma section zmxo_tnm_ds_stop_abrupt
/* Constant ZMXO-TNM-DS-STOP-ABRUPT created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DS_STOP_ABRUPT 2038
#pragma section zmxo_tnm_dstop_abrpt_info
/* Constant ZMXO-TNM-DSTOP-ABRPT-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DSTOP_ABRPT_INFO 2039
#pragma section zmxo_tnm_ds_stopin_abrupt
/* Constant ZMXO-TNM-DS-STOPIN-ABRUPT created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DS_STOPIN_ABRUPT 2040
#pragma section zmxo_tnm_ds_stopin_discon
/* Constant ZMXO-TNM-DS-STOPIN-DISCON created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DS_STOPIN_DISCON 2041
#pragma section zmxo_tnm_svc_stop_abrupt
/* Constant ZMXO-TNM-SVC-STOP-ABRUPT created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SVC_STOP_ABRUPT 2042
#pragma section zmxo_tnm_svc_strt_warning
/* Constant ZMXO-TNM-SVC-STRT-WARNING created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SVC_STRT_WARNING 2043
#pragma section zmxo_tnm_sql_not_init
/* Constant ZMXO-TNM-SQL-NOT-INIT created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SQL_NOT_INIT 2044
#pragma section zmxo_tnm_sav_dsstat_fail
/* Constant ZMXO-TNM-SAV-DSSTAT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SAV_DSSTAT_FAIL 2045
#pragma section zmxo_tnm_sav_asstat_fail
/* Constant ZMXO-TNM-SAV-ASSTAT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SAV_ASSTAT_FAIL 2046
#pragma section zmxo_tnm_intrnlcntr_recal
/* Constant ZMXO-TNM-INTRNLCNTR-RECAL created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_INTRNLCNTR_RECAL 2047
#pragma section zmxo_tnm_collector_error
/* Constant ZMXO-TNM-COLLECTOR-ERROR created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_COLLECTOR_ERROR 2048
#pragma section zmxo_tnm_trace_info
/* Constant ZMXO-TNM-TRACE-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_TRACE_INFO 2049
#pragma section zmxo_tnm_res_stat_info
/* Constant ZMXO-TNM-RES-STAT-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_RES_STAT_INFO 2050
#pragma section zmxo_tnm_query_status_info
/* Constant ZMXO-TNM-QUERY-STATUS-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_QUERY_STATUS_INFO 2051
#pragma section zmxo_tnm_program_err
/* Constant ZMXO-TNM-PROGRAM-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_PROGRAM_ERR 2061
#pragma section zmxo_tnm_sql_err
/* Constant ZMXO-TNM-SQL-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SQL_ERR 2062
#pragma section zmxo_tnm_krypton_err
/* Constant ZMXO-TNM-KRYPTON-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_KRYPTON_ERR 2063
#pragma section zmxo_tnm_srvr_reg_err
/* Constant ZMXO-TNM-SRVR-REG-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SRVR_REG_ERR 2064
#pragma section zmxo_tnm_nsk_err
/* Constant ZMXO-TNM-NSK-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_NSK_ERR 2065
#pragma section zmxo_tnm_srvr_env
/* Constant ZMXO-TNM-SRVR-ENV created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SRVR_ENV 2066
#pragma section zmxo_tnm_malloc_err
/* Constant ZMXO-TNM-MALLOC-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_MALLOC_ERR 2067
#pragma section zmxo_tnm_sql_warning
/* Constant ZMXO-TNM-SQL-WARNING created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SQL_WARNING 2068
#pragma section zmxo_tnm_rg_stop
/* Constant ZMXO-TNM-RG-STOP created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_RG_STOP 2069
#pragma section zmxo_tnm_srv_moncal_fail
/* Constant ZMXO-TNM-SRV-MONCAL-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SRV_MONCAL_FAIL 2070
#pragma section zmxo_tnm_srv_itout_err
/* Constant ZMXO-TNM-SRV-ITOUT-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SRV_ITOUT_ERR 2071
#pragma section zmxo_tnm_updt_srv_st_fail
/* Constant ZMXO-TNM-UPDT-SRV-ST-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_UPDT_SRV_ST_FAIL 2072
#pragma section zmxo_tnm_tip_notconnect
/* Constant ZMXO-TNM-TIP-NOTCONNECT created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_TIP_NOTCONNECT 2073
#pragma section zmxo_tnm_tip_notconfig
/* Constant ZMXO-TNM-TIP-NOTCONFIG created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_TIP_NOTCONFIG 2074
#pragma section zmxo_tnm_tip_err
/* Constant ZMXO-TNM-TIP-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_TIP_ERR 2075
#pragma section zmxo_tnm_post_conct_err
/* Constant ZMXO-TNM-POST-CONCT-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_POST_CONCT_ERR 2076
#pragma section zmxo_tnm_odbcinit_started
/* Constant ZMXO-TNM-ODBCINIT-STARTED created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_ODBCINIT_STARTED 2077
#pragma section zmxo_tnm_definesetattr_err
/* Constant ZMXO-TNM-DEFINESETATTR-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DEFINESETATTR_ERR 2078
#pragma section zmxo_tnm_definesave_err
/* Constant ZMXO-TNM-DEFINESAVE-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DEFINESAVE_ERR 2079
#pragma section zmxo_tnm_insuf_privlgs
/* Constant ZMXO-TNM-INSUF-PRIVLGS created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_INSUF_PRIVLGS 2080
#pragma section zmxo_tnm_eventtype
/* Constant ZMXO-TNM-EVENTTYPE created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_EVENTTYPE 3000
#pragma section zmxo_tnm_rg_estcost
/* Constant ZMXO-TNM-RG-ESTCOST created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_RG_ESTCOST 3001
#pragma section zmxo_tnm_rg_limit
/* Constant ZMXO-TNM-RG-LIMIT created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_RG_LIMIT 3002
#pragma section zmxo_tnm_sql_text
/* Constant ZMXO-TNM-SQL-TEXT created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SQL_TEXT 3003
#pragma section zmxo_tnm_reason
/* Constant ZMXO-TNM-REASON created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_REASON 3004
#pragma section zmxo_tnm_nexttoken
/* Constant ZMXO-TNM-NEXTTOKEN created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_NEXTTOKEN 3005
#pragma section zmxo_tnm_srvobjref
/* Constant ZMXO-TNM-SRVOBJREF created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SRVOBJREF 3006
#pragma section zmxo_tnm_srvrstate
/* Constant ZMXO-TNM-SRVRSTATE created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SRVRSTATE 3007
#pragma section zmxo_tnm_procport
/* Constant ZMXO-TNM-PROCPORT created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_PROCPORT 3008
#pragma section zmxo_tnm_dsstate
/* Constant ZMXO-TNM-DSSTATE created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DSSTATE 3009
#pragma section zmxo_tnm_debugflag
/* Constant ZMXO-TNM-DEBUGFLAG created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DEBUGFLAG 3010
#pragma section zmxo_tnm_dsid
/* Constant ZMXO-TNM-DSID created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_DSID 3011
#pragma section zmxo_tnm_ceeparam
/* Constant ZMXO-TNM-CEEPARAM created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_CEEPARAM 3012
#pragma section zmxo_tnm_experience_level
/* Constant ZMXO-TNM-EXPERIENCE-LEVEL created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_EXPERIENCE_LEVEL 3013
#pragma section zmxo_tnm_severity
/* Constant ZMXO-TNM-SEVERITY created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_SEVERITY 3014
#pragma section zmxo_tnm_event_target
/* Constant ZMXO-TNM-EVENT-TARGET created on 07/07/2010 at 01:20 */
#define ZMXO_TNM_EVENT_TARGET 3015
#pragma section zmxo_evt_reg_srvr_err
/* Constant ZMXO-EVT-REG-SRVR-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_REG_SRVR_ERR 31000
#pragma section zmxo_evt_srvst_chng_err
/* Constant ZMXO-EVT-SRVST-CHNG-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SRVST_CHNG_ERR 31001
#pragma section zmxo_evt_start_srvr_err
/* Constant ZMXO-EVT-START-SRVR-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_START_SRVR_ERR 31002
#pragma section zmxo_evt_stop_srvr_err
/* Constant ZMXO-EVT-STOP-SRVR-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_STOP_SRVR_ERR 31003
#pragma section zmxo_evt_dsst_chng_fail
/* Constant ZMXO-EVT-DSST-CHNG-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_DSST_CHNG_FAIL 31005
#pragma section zmxo_evt_port_notavail
/* Constant ZMXO-EVT-PORT-NOTAVAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_PORT_NOTAVAIL 31006
#pragma section zmxo_evt_srvc_started
/* Constant ZMXO-EVT-SRVC-STARTED created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SRVC_STARTED 31007
#pragma section zmxo_evt_ds_started
/* Constant ZMXO-EVT-DS-STARTED created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_DS_STARTED 31008
#pragma section zmxo_evt_srvc_strt_fail
/* Constant ZMXO-EVT-SRVC-STRT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SRVC_STRT_FAIL 31009
#pragma section zmxo_evt_srvc_strt_info
/* Constant ZMXO-EVT-SRVC-STRT-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SRVC_STRT_INFO 31010
#pragma section zmxo_evt_ds_strt_fail
/* Constant ZMXO-EVT-DS-STRT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_DS_STRT_FAIL 31011
#pragma section zmxo_evt_ds_strt_info
/* Constant ZMXO-EVT-DS-STRT-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_DS_STRT_INFO 31012
#pragma section zmxo_evt_srvc_stopped
/* Constant ZMXO-EVT-SRVC-STOPPED created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SRVC_STOPPED 31013
#pragma section zmxo_evt_ds_stopped
/* Constant ZMXO-EVT-DS-STOPPED created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_DS_STOPPED 31014
#pragma section zmxo_evt_svc_stopped_info
/* Constant ZMXO-EVT-SVC-STOPPED-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SVC_STOPPED_INFO 31015
#pragma section zmxo_evt_ds_stopped_info
/* Constant ZMXO-EVT-DS-STOPPED-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_DS_STOPPED_INFO 31016
#pragma section zmxo_evt_stsrv_cntxt_fail
/* Constant ZMXO-EVT-STSRV-CNTXT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_STSRV_CNTXT_FAIL 31017
#pragma section zmxo_evt_srv_strt_indbg
/* Constant ZMXO-EVT-SRV-STRT-INDBG created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SRV_STRT_INDBG 31018
#pragma section zmxo_evt_srv_st_chng_info
/* Constant ZMXO-EVT-SRV-ST-CHNG-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SRV_ST_CHNG_INFO 31019
#pragma section zmxo_evt_ds_stopping
/* Constant ZMXO-EVT-DS-STOPPING created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_DS_STOPPING 31020
#pragma section zmxo_evt_ds_stop_abrupt
/* Constant ZMXO-EVT-DS-STOP-ABRUPT created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_DS_STOP_ABRUPT 31021
#pragma section zmxo_evt_dstop_abrpt_info
/* Constant ZMXO-EVT-DSTOP-ABRPT-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_DSTOP_ABRPT_INFO 31022
#pragma section zmxo_evt_ds_stopin_abrupt
/* Constant ZMXO-EVT-DS-STOPIN-ABRUPT created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_DS_STOPIN_ABRUPT 31023
#pragma section zmxo_evt_ds_stopin_discon
/* Constant ZMXO-EVT-DS-STOPIN-DISCON created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_DS_STOPIN_DISCON 31024
#pragma section zmxo_evt_svc_stop_abrupt
/* Constant ZMXO-EVT-SVC-STOP-ABRUPT created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SVC_STOP_ABRUPT 31025
#pragma section zmxo_evt_svc_strt_warning
/* Constant ZMXO-EVT-SVC-STRT-WARNING created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SVC_STRT_WARNING 31026
#pragma section zmxo_evt_sql_not_init
/* Constant ZMXO-EVT-SQL-NOT-INIT created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SQL_NOT_INIT 31028
#pragma section zmxo_evt_srvc_initialized
/* Constant ZMXO-EVT-SRVC-INITIALIZED created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SRVC_INITIALIZED 31029
#pragma section zmxo_evt_sav_dsstat_fail
/* Constant ZMXO-EVT-SAV-DSSTAT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SAV_DSSTAT_FAIL 31030
#pragma section zmxo_evt_sav_asstat_fail
/* Constant ZMXO-EVT-SAV-ASSTAT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SAV_ASSTAT_FAIL 31031
#pragma section zmxo_evt_intrnlcntr_recal
/* Constant ZMXO-EVT-INTRNLCNTR-RECAL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_INTRNLCNTR_RECAL 31032
#pragma section zmxo_evt_collector_error
/* Constant ZMXO-EVT-COLLECTOR-ERROR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_COLLECTOR_ERROR 31033
#pragma section zmxo_evt_trace_info
/* Constant ZMXO-EVT-TRACE-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_TRACE_INFO 31034
#pragma section zmxo_evt_res_stat_info
/* Constant ZMXO-EVT-RES-STAT-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_RES_STAT_INFO 31035
#pragma section zmxo_evt_query_status_info
/* Constant ZMXO-EVT-QUERY-STATUS-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_QUERY_STATUS_INFO 31036
#pragma section zmxo_evt_insuf_privlgs
/* Constant ZMXO-EVT-INSUF-PRIVLGS created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_INSUF_PRIVLGS 31260
#pragma section zmxo_evt_program_err
/* Constant ZMXO-EVT-PROGRAM-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_PROGRAM_ERR 30002
#pragma section zmxo_evt_sql_err
/* Constant ZMXO-EVT-SQL-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SQL_ERR 30004
#pragma section zmxo_evt_krypton_err
/* Constant ZMXO-EVT-KRYPTON-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_KRYPTON_ERR 30005
#pragma section zmxo_evt_srvr_reg_err
/* Constant ZMXO-EVT-SRVR-REG-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SRVR_REG_ERR 30006
#pragma section zmxo_evt_nsk_err
/* Constant ZMXO-EVT-NSK-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_NSK_ERR 30007
#pragma section zmxo_evt_srvr_env
/* Constant ZMXO-EVT-SRVR-ENV created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SRVR_ENV 30008
#pragma section zmxo_evt_malloc_err
/* Constant ZMXO-EVT-MALLOC-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_MALLOC_ERR 30009
#pragma section zmxo_evt_sql_warning
/* Constant ZMXO-EVT-SQL-WARNING created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SQL_WARNING 30010
#pragma section zmxo_evt_definesetattr_err
/* Constant ZMXO-EVT-DEFINESETATTR-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_DEFINESETATTR_ERR 30011
#pragma section zmxo_evt_definesave_err
/* Constant ZMXO-EVT-DEFINESAVE-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_DEFINESAVE_ERR 30012
#pragma section zmxo_evt_rg_over_limit
/* Constant ZMXO-EVT-RG-OVER-LIMIT created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_RG_OVER_LIMIT 30400
#pragma section zmxo_evt_rg_stop
/* Constant ZMXO-EVT-RG-STOP created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_RG_STOP 30401
#pragma section zmxo_evt_srv_moncal_fail
/* Constant ZMXO-EVT-SRV-MONCAL-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SRV_MONCAL_FAIL 30402
#pragma section zmxo_evt_srv_itout_err
/* Constant ZMXO-EVT-SRV-ITOUT-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_SRV_ITOUT_ERR 30403
#pragma section zmxo_evt_updt_srv_st_fail
/* Constant ZMXO-EVT-UPDT-SRV-ST-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_UPDT_SRV_ST_FAIL 30404
#pragma section zmxo_evt_tip_notconnect
/* Constant ZMXO-EVT-TIP-NOTCONNECT created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_TIP_NOTCONNECT 30420
#pragma section zmxo_evt_tip_notconfig
/* Constant ZMXO-EVT-TIP-NOTCONFIG created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_TIP_NOTCONFIG 30421
#pragma section zmxo_evt_tip_err
/* Constant ZMXO-EVT-TIP-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_TIP_ERR 30422
#pragma section zmxo_evt_post_conct_err
/* Constant ZMXO-EVT-POST-CONCT-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_POST_CONCT_ERR 30423
#pragma section zmxo_evt_odbcinit_started
/* Constant ZMXO-EVT-ODBCINIT-STARTED created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_ODBCINIT_STARTED 30800
#pragma section zmxo_evt_cfg_srvr_init
/* Constant ZMXO-EVT-CFG-SRVR-INIT created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CFG_SRVR_INIT 30950
#pragma section zmxo_evt_cs_reg_srvr_err
/* Constant ZMXO-EVT-CS-REG-SRVR-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_REG_SRVR_ERR 21000
#pragma section zmxo_evt_cs_srvst_chng_err
/* Constant ZMXO-EVT-CS-SRVST-CHNG-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SRVST_CHNG_ERR 21001
#pragma section zmxo_evt_cs_start_srvr_err
/* Constant ZMXO-EVT-CS-START-SRVR-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_START_SRVR_ERR 21002
#pragma section zmxo_evt_cs_stop_srvr_err
/* Constant ZMXO-EVT-CS-STOP-SRVR-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_STOP_SRVR_ERR 21003
#pragma section zmxo_evt_cs_dsst_chng_fail
/* Constant ZMXO-EVT-CS-DSST-CHNG-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_DSST_CHNG_FAIL 21005
#pragma section zmxo_evt_cs_port_notavail
/* Constant ZMXO-EVT-CS-PORT-NOTAVAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_PORT_NOTAVAIL 21006
#pragma section zmxo_evt_cs_srvc_started
/* Constant ZMXO-EVT-CS-SRVC-STARTED created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SRVC_STARTED 21007
#pragma section zmxo_evt_cs_ds_started
/* Constant ZMXO-EVT-CS-DS-STARTED created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_DS_STARTED 21008
#pragma section zmxo_evt_cs_srvc_strt_fail
/* Constant ZMXO-EVT-CS-SRVC-STRT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SRVC_STRT_FAIL 21009
#pragma section zmxo_evt_cs_srvc_strt_info
/* Constant ZMXO-EVT-CS-SRVC-STRT-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SRVC_STRT_INFO 21010
#pragma section zmxo_evt_cs_ds_strt_fail
/* Constant ZMXO-EVT-CS-DS-STRT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_DS_STRT_FAIL 21011
#pragma section zmxo_evt_cs_ds_strt_info
/* Constant ZMXO-EVT-CS-DS-STRT-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_DS_STRT_INFO 21012
#pragma section zmxo_evt_cs_srvc_stopped
/* Constant ZMXO-EVT-CS-SRVC-STOPPED created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SRVC_STOPPED 21013
#pragma section zmxo_evt_cs_ds_stopped
/* Constant ZMXO-EVT-CS-DS-STOPPED created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_DS_STOPPED 21014
#pragma section zmxo_evt_cs_svc_stopped_info
/* Constant ZMXO-EVT-CS-SVC-STOPPED-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SVC_STOPPED_INFO 21015
#pragma section zmxo_evt_cs_ds_stopped_info
/* Constant ZMXO-EVT-CS-DS-STOPPED-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_DS_STOPPED_INFO 21016
#pragma section zmxo_evt_cs_stsrv_cntxt_fail
/* Constant ZMXO-EVT-CS-STSRV-CNTXT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_STSRV_CNTXT_FAIL 21017
#pragma section zmxo_evt_cs_srv_strt_indbg
/* Constant ZMXO-EVT-CS-SRV-STRT-INDBG created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SRV_STRT_INDBG 21018
#pragma section zmxo_evt_cs_srv_st_chng_info
/* Constant ZMXO-EVT-CS-SRV-ST-CHNG-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SRV_ST_CHNG_INFO 21019
#pragma section zmxo_evt_cs_ds_stopping
/* Constant ZMXO-EVT-CS-DS-STOPPING created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_DS_STOPPING 21020
#pragma section zmxo_evt_cs_ds_stop_abrupt
/* Constant ZMXO-EVT-CS-DS-STOP-ABRUPT created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_DS_STOP_ABRUPT 21021
#pragma section zmxo_evt_cs_dstop_abrpt_info
/* Constant ZMXO-EVT-CS-DSTOP-ABRPT-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_DSTOP_ABRPT_INFO 21022
#pragma section zmxo_evt_cs_ds_stopin_abrupt
/* Constant ZMXO-EVT-CS-DS-STOPIN-ABRUPT created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_DS_STOPIN_ABRUPT 21023
#pragma section zmxo_evt_cs_ds_stopin_discon
/* Constant ZMXO-EVT-CS-DS-STOPIN-DISCON created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_DS_STOPIN_DISCON 21024
#pragma section zmxo_evt_cs_svc_stop_abrupt
/* Constant ZMXO-EVT-CS-SVC-STOP-ABRUPT created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SVC_STOP_ABRUPT 21025
#pragma section zmxo_evt_cs_svc_strt_warning
/* Constant ZMXO-EVT-CS-SVC-STRT-WARNING created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SVC_STRT_WARNING 21026
#pragma section zmxo_evt_cs_sql_not_init
/* Constant ZMXO-EVT-CS-SQL-NOT-INIT created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SQL_NOT_INIT 21028
#pragma section zmxo_evt_cs_srvc_initialized
/* Constant ZMXO-EVT-CS-SRVC-INITIALIZED created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SRVC_INITIALIZED 21029
#pragma section zmxo_evt_cs_sav_dsstat_fail
/* Constant ZMXO-EVT-CS-SAV-DSSTAT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SAV_DSSTAT_FAIL 21030
#pragma section zmxo_evt_cs_sav_asstat_fail
/* Constant ZMXO-EVT-CS-SAV-ASSTAT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SAV_ASSTAT_FAIL 21031
#pragma section zmxo_evt_cs_intrnlcntr_recal
/* Constant ZMXO-EVT-CS-INTRNLCNTR-RECAL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_INTRNLCNTR_RECAL 21032
#pragma section zmxo_evt_cs_collector_error
/* Constant ZMXO-EVT-CS-COLLECTOR-ERROR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_COLLECTOR_ERROR 21033
#pragma section zmxo_evt_cs_trace_info
/* Constant ZMXO-EVT-CS-TRACE-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_TRACE_INFO 21034
#pragma section zmxo_evt_cs_res_stat_info
/* Constant ZMXO-EVT-CS-RES-STAT-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_RES_STAT_INFO 21035
#pragma section zmxo_evt_cs_query_status_info
/* Constant ZMXO-EVT-CS-QUERY-STATUS-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_QUERY_STATUS_INFO 21036
#pragma section zmxo_evt_cs_insuf_privlgs
/* Constant ZMXO-EVT-CS-INSUF-PRIVLGS created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_INSUF_PRIVLGS 21260
#pragma section zmxo_evt_cs_program_err
/* Constant ZMXO-EVT-CS-PROGRAM-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_PROGRAM_ERR 20002
#pragma section zmxo_evt_cs_sql_err
/* Constant ZMXO-EVT-CS-SQL-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SQL_ERR 20004
#pragma section zmxo_evt_cs_krypton_err
/* Constant ZMXO-EVT-CS-KRYPTON-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_KRYPTON_ERR 20005
#pragma section zmxo_evt_cs_srvr_reg_err
/* Constant ZMXO-EVT-CS-SRVR-REG-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SRVR_REG_ERR 20006
#pragma section zmxo_evt_cs_nsk_err
/* Constant ZMXO-EVT-CS-NSK-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_NSK_ERR 20007
#pragma section zmxo_evt_cs_srvr_env
/* Constant ZMXO-EVT-CS-SRVR-ENV created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SRVR_ENV 20008
#pragma section zmxo_evt_cs_malloc_err
/* Constant ZMXO-EVT-CS-MALLOC-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_MALLOC_ERR 20009
#pragma section zmxo_evt_cs_sql_warning
/* Constant ZMXO-EVT-CS-SQL-WARNING created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SQL_WARNING 20010
#pragma section zmxo_evt_cs_definesetattr_err
/* Constant ZMXO-EVT-CS-DEFINESETATTR-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_DEFINESETATTR_ERR 20011
#pragma section zmxo_evt_cs_definesave_err
/* Constant ZMXO-EVT-CS-DEFINESAVE-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_DEFINESAVE_ERR 20012
#pragma section zmxo_evt_cs_rg_over_limit
/* Constant ZMXO-EVT-CS-RG-OVER-LIMIT created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_RG_OVER_LIMIT 20400
#pragma section zmxo_evt_cs_rg_stop
/* Constant ZMXO-EVT-CS-RG-STOP created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_RG_STOP 20401
#pragma section zmxo_evt_cs_srv_moncal_fail
/* Constant ZMXO-EVT-CS-SRV-MONCAL-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SRV_MONCAL_FAIL 20402
#pragma section zmxo_evt_cs_srv_itout_err
/* Constant ZMXO-EVT-CS-SRV-ITOUT-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_SRV_ITOUT_ERR 20403
#pragma section zmxo_evt_cs_updt_srv_st_fail
/* Constant ZMXO-EVT-CS-UPDT-SRV-ST-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_UPDT_SRV_ST_FAIL 20404
#pragma section zmxo_evt_cs_tip_notconnect
/* Constant ZMXO-EVT-CS-TIP-NOTCONNECT created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_TIP_NOTCONNECT 20420
#pragma section zmxo_evt_cs_tip_notconfig
/* Constant ZMXO-EVT-CS-TIP-NOTCONFIG created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_TIP_NOTCONFIG 20421
#pragma section zmxo_evt_cs_tip_err
/* Constant ZMXO-EVT-CS-TIP-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_TIP_ERR 20422
#pragma section zmxo_evt_cs_post_conct_err
/* Constant ZMXO-EVT-CS-POST-CONCT-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_POST_CONCT_ERR 20423
#pragma section zmxo_evt_cs_odbcinit_started
/* Constant ZMXO-EVT-CS-ODBCINIT-STARTED created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_ODBCINIT_STARTED 20800
#pragma section zmxo_evt_cs_cfg_srvr_init
/* Constant ZMXO-EVT-CS-CFG-SRVR-INIT created on 07/07/2010 at 01:20 */
#define ZMXO_EVT_CS_CFG_SRVR_INIT 20950
#pragma section zmxo_ddl_eventnumber_enum
/* Definition ZMXO-DDL-EVENTNUMBER-ENUM created on 07/07/2010 at 01:20 */
enum
{
   zmxo_enm_reg_srvr_err = 31000,
   zmxo_enm_srvst_chng_err = 31001,
   zmxo_enm_start_srvr_err = 31002,
   zmxo_enm_stop_srvr_err = 31003,
   zmxo_enm_dsst_chng_fail = 31005,
   zmxo_enm_port_notavail = 31006,
   zmxo_enm_srvc_strt_fail = 31009,
   zmxo_enm_srvc_strt_info = 31010,
   zmxo_enm_ds_strt_fail = 31011,
   zmxo_enm_ds_strt_info = 31012,
   zmxo_enm_srvc_started = 31007,
   zmxo_enm_srvc_initialized = 31029,
   zmxo_enm_cfg_srvr_init = 30950,
   zmxo_enm_ds_started = 31008,
   zmxo_enm_ds_stopped = 31014,
   zmxo_enm_svc_stopped_info = 31015,
   zmxo_enm_ds_stopped_info = 31016,
   zmxo_enm_stsrv_cntxt_fail = 31017,
   zmxo_enm_srv_strt_indbg = 31018,
   zmxo_enm_srv_st_chng_info = 31019,
   zmxo_enm_ds_stopping = 31020,
   zmxo_enm_ds_stop_abrupt = 31021,
   zmxo_enm_dstop_abrpt_info = 31022,
   zmxo_enm_ds_stopin_abrupt = 31023,
   zmxo_enm_ds_stopin_discon = 31024,
   zmxo_enm_svc_stop_abrupt = 31025,
   zmxo_enm_svc_strt_warning = 31026,
   zmxo_enm_sql_notinit = 31028,
   zmxo_enm_sav_dsstat_fail = 31030,
   zmxo_enm_sav_asstat_fail = 31031,
   zmxo_enm_intrnlcntr_recal = 31032,
   zmxo_enm_collector_error = 31033,
   zmxo_enm_trace_info = 31034,
   zmxo_enm_res_stat_info = 31035,
   zmxo_enm_query_status_info = 31036,
   zmxo_enm_rg_over_limit = 30400,
   zmxo_enm_srvc_stopped = 31013,
   zmxo_enm_program_err = 30002,
   zmxo_enm_sql_err = 30004,
   zmxo_enm_krypton_err = 30005,
   zmxo_enm_srvr_reg_err = 30006,
   zmxo_enm_nsk_err = 30007,
   zmxo_enm_srvr_env = 30008,
   zmxo_enm_malloc_err = 30009,
   zmxo_enm_sql_warning = 30010,
   zmxo_enm_definesetattr_err = 30011,
   zmxo_enm_definesave_err = 30012,
   zmxo_enm_rg_stop = 30401,
   zmxo_enm_srv_moncal_fail = 30402,
   zmxo_enm_srv_itout_err = 30403,
   zmxo_enm_updt_srv_st_fail = 30404,
   zmxo_enm_tip_notconnect = 30420,
   zmxo_enm_tip_notconfig = 30421,
   zmxo_enm_tip_err = 30422,
   zmxo_enm_post_conct_err = 30423,
   zmxo_enm_odbcinit_started = 30800,
   zmxo_enm_insuf_privlgs = 31260,
   zmxo_enm_cs_reg_srvr_err = 21000,
   zmxo_enm_cs_srvst_chng_err = 21001,
   zmxo_enm_cs_start_srvr_err = 21002,
   zmxo_enm_cs_stop_srvr_err = 21003,
   zmxo_enm_cs_dsst_chng_fail = 21005,
   zmxo_enm_cs_port_notavail = 21006,
   zmxo_enm_cs_srvc_strt_fail = 21009,
   zmxo_enm_cs_srvc_strt_info = 21010,
   zmxo_enm_cs_ds_strt_fail = 21011,
   zmxo_enm_cs_ds_strt_info = 21012,
   zmxo_enm_cs_srvc_started = 21007,
   zmxo_enm_cs_srvc_initialized = 21029,
   zmxo_enm_cs_cfg_srvr_init = 20950,
   zmxo_enm_cs_ds_started = 21008,
   zmxo_enm_cs_ds_stopped = 21014,
   zmxo_enm_cs_svc_stopped_info = 21015,
   zmxo_enm_cs_ds_stopped_info = 21016,
   zmxo_enm_cs_stsrv_cntxt_fail = 21017,
   zmxo_enm_cs_srv_strt_indbg = 21018,
   zmxo_enm_cs_srv_st_chng_info = 21019,
   zmxo_enm_cs_ds_stopping = 21020,
   zmxo_enm_cs_ds_stop_abrupt = 21021,
   zmxo_enm_cs_dstop_abrpt_info = 21022,
   zmxo_enm_cs_ds_stopin_abrupt = 21023,
   zmxo_enm_cs_ds_stopin_discon = 21024,
   zmxo_enm_cs_svc_stop_abrupt = 21025,
   zmxo_enm_cs_svc_strt_warning = 21026,
   zmxo_enm_cs_sql_notinit = 21028,
   zmxo_enm_cs_sav_dsstat_fail = 21030,
   zmxo_enm_cs_sav_asstat_fail = 21031,
   zmxo_enm_cs_intrnlcntr_recal = 21032,
   zmxo_enm_cs_collector_error = 21033,
   zmxo_enm_cs_trace_info = 21034,
   zmxo_enm_cs_res_stat_info = 21035,
   zmxo_enm_cs_query_status_info = 21036,
   zmxo_enm_cs_rg_over_limit = 20400,
   zmxo_enm_cs_srvc_stopped = 21013,
   zmxo_enm_cs_program_err = 20002,
   zmxo_enm_cs_sql_err = 20004,
   zmxo_enm_cs_krypton_err = 20005,
   zmxo_enm_cs_srvr_reg_err = 20006,
   zmxo_enm_cs_nsk_err = 20007,
   zmxo_enm_cs_srvr_env = 20008,
   zmxo_enm_cs_malloc_err = 20009,
   zmxo_enm_cs_sql_warning = 20010,
   zmxo_enm_cs_definesetattr_err = 20011,
   zmxo_enm_cs_definesave_err = 20012,
   zmxo_enm_cs_rg_stop = 20401,
   zmxo_enm_cs_srv_moncal_fail = 20402,
   zmxo_enm_cs_srv_itout_err = 20403,
   zmxo_enm_cs_updt_srv_st_fail = 20404,
   zmxo_enm_cs_tip_notconnect = 20420,
   zmxo_enm_cs_tip_notconfig = 20421,
   zmxo_enm_cs_tip_err = 20422,
   zmxo_enm_cs_post_conct_err = 20423,
   zmxo_enm_cs_odbcinit_started = 20800,
   zmxo_enm_cs_insuf_privlgs = 21260
};
typedef short                           zmxo_ddl_eventnumber_enum_def;
#pragma section zmxo_tkn_srvc_initialized
/* Token Code ZMXO-TKN-SRVC-INITIALIZED created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SRVC_INITIALIZED 33490910LU
#pragma section zmxo_tkn_odbcinit_started
/* Token Code ZMXO-TKN-ODBCINIT-STARTED created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_ODBCINIT_STARTED 33490973LU
#pragma section zmxo_tkn_srvc_started
/* Token Code ZMXO-TKN-SRVC-STARTED created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SRVC_STARTED 33490911LU
#pragma section zmxo_tkn_cfg_srvr_init
/* Token Code ZMXO-TKN-CFG-SRVR-INIT created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_CFG_SRVR_INIT 33490912LU
#pragma section zmxo_tkn_ds_started
/* Token Code ZMXO-TKN-DS-STARTED created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_DS_STARTED 33490913LU
#pragma section zmxo_tkn_ds_stopped
/* Token Code ZMXO-TKN-DS-STOPPED created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_DS_STOPPED 33490915LU
#pragma section zmxo_tkn_ds_stopping
/* Token Code ZMXO-TKN-DS-STOPPING created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_DS_STOPPING 33490933LU
#pragma section zmxo_tkn_component
/* Token Code ZMXO-TKN-COMPONENT created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_COMPONENT 33490907LU
#pragma section zmxo_tkn_objectref
/* Token Code ZMXO-TKN-OBJECTREF created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_OBJECTREF 33490908LU
#pragma section zmxo_tkn_datasource
/* Token Code ZMXO-TKN-DATASOURCE created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_DATASOURCE 33490909LU
#pragma section zmxo_tkn_program_err
/* Token Code ZMXO-TKN-PROGRAM-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_PROGRAM_ERR 33490957LU
#pragma section zmxo_tkn_sql_err
/* Token Code ZMXO-TKN-SQL-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SQL_ERR 33490958LU
#pragma section zmxo_tkn_krypton_err
/* Token Code ZMXO-TKN-KRYPTON-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_KRYPTON_ERR 33490959LU
#pragma section zmxo_tkn_srvr_reg_err
/* Token Code ZMXO-TKN-SRVR-REG-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SRVR_REG_ERR 33490960LU
#pragma section zmxo_tkn_nsk_err
/* Token Code ZMXO-TKN-NSK-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_NSK_ERR 33490961LU
#pragma section zmxo_tkn_srvr_env
/* Token Code ZMXO-TKN-SRVR-ENV created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SRVR_ENV 33490962LU
#pragma section zmxo_tkn_malloc_err
/* Token Code ZMXO-TKN-MALLOC-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_MALLOC_ERR 33490963LU
#pragma section zmxo_tkn_sql_warning
/* Token Code ZMXO-TKN-SQL-WARNING created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SQL_WARNING 33490964LU
#pragma section zmxo_tkn_definesetattr_err
/* Token Code ZMXO-TKN-DEFINESETATTR-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_DEFINESETATTR_ERR 33490974LU
#pragma section zmxo_tkn_definesave_err
/* Token Code ZMXO-TKN-DEFINESAVE-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_DEFINESAVE_ERR 33490975LU
#pragma section zmxo_tkn_rg_stop
/* Token Code ZMXO-TKN-RG-STOP created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_RG_STOP 33490965LU
#pragma section zmxo_tkn_srv_moncal_fail
/* Token Code ZMXO-TKN-SRV-MONCAL-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SRV_MONCAL_FAIL 33490966LU
#pragma section zmxo_tkn_srv_itout_err
/* Token Code ZMXO-TKN-SRV-ITOUT-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SRV_ITOUT_ERR 33490967LU
#pragma section zmxo_tkn_updt_srv_st_fail
/* Token Code ZMXO-TKN-UPDT-SRV-ST-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_UPDT_SRV_ST_FAIL 33490968LU
#pragma section zmxo_tkn_tip_notconnect
/* Token Code ZMXO-TKN-TIP-NOTCONNECT created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_TIP_NOTCONNECT 33490969LU
#pragma section zmxo_tkn_tip_notconfig
/* Token Code ZMXO-TKN-TIP-NOTCONFIG created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_TIP_NOTCONFIG 33490970LU
#pragma section zmxo_tkn_tip_err
/* Token Code ZMXO-TKN-TIP-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_TIP_ERR 33490971LU
#pragma section zmxo_tkn_post_conct_err
/* Token Code ZMXO-TKN-POST-CONCT-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_POST_CONCT_ERR 33490972LU
#pragma section zmxo_tkn_reg_srvr_err
/* Token Code ZMXO-TKN-REG-SRVR-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_REG_SRVR_ERR 33490918LU
#pragma section zmxo_tkn_srvst_chng_err
/* Token Code ZMXO-TKN-SRVST-CHNG-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SRVST_CHNG_ERR 33490919LU
#pragma section zmxo_tkn_start_srvr_err
/* Token Code ZMXO-TKN-START-SRVR-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_START_SRVR_ERR 33490920LU
#pragma section zmxo_tkn_stop_srvr_err
/* Token Code ZMXO-TKN-STOP-SRVR-ERR created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_STOP_SRVR_ERR 33490921LU
#pragma section zmxo_tkn_dsst_chng_fail
/* Token Code ZMXO-TKN-DSST-CHNG-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_DSST_CHNG_FAIL 33490922LU
#pragma section zmxo_tkn_port_notavail
/* Token Code ZMXO-TKN-PORT-NOTAVAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_PORT_NOTAVAIL 33490923LU
#pragma section zmxo_tkn_srvc_strt_fail
/* Token Code ZMXO-TKN-SRVC-STRT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SRVC_STRT_FAIL 33490924LU
#pragma section zmxo_tkn_srvc_strt_info
/* Token Code ZMXO-TKN-SRVC-STRT-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SRVC_STRT_INFO 33490925LU
#pragma section zmxo_tkn_ds_strt_fail
/* Token Code ZMXO-TKN-DS-STRT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_DS_STRT_FAIL 33490926LU
#pragma section zmxo_tkn_ds_strt_info
/* Token Code ZMXO-TKN-DS-STRT-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_DS_STRT_INFO 33490927LU
#pragma section zmxo_tkn_svc_stopped_info
/* Token Code ZMXO-TKN-SVC-STOPPED-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SVC_STOPPED_INFO 33490928LU
#pragma section zmxo_tkn_ds_stopped_info
/* Token Code ZMXO-TKN-DS-STOPPED-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_DS_STOPPED_INFO 33490929LU
#pragma section zmxo_tkn_stsrv_cntxt_fail
/* Token Code ZMXO-TKN-STSRV-CNTXT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_STSRV_CNTXT_FAIL 33490930LU
#pragma section zmxo_tkn_srv_strt_indbg
/* Token Code ZMXO-TKN-SRV-STRT-INDBG created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SRV_STRT_INDBG 33490931LU
#pragma section zmxo_tkn_srv_st_chng_info
/* Token Code ZMXO-TKN-SRV-ST-CHNG-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SRV_ST_CHNG_INFO 33490932LU
#pragma section zmxo_tkn_ds_stop_abrupt
/* Token Code ZMXO-TKN-DS-STOP-ABRUPT created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_DS_STOP_ABRUPT 33490934LU
#pragma section zmxo_tkn_ds_stopin_abrupt
/* Token Code ZMXO-TKN-DS-STOPIN-ABRUPT created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_DS_STOPIN_ABRUPT 33490936LU
#pragma section zmxo_tkn_ds_stopin_discon
/* Token Code ZMXO-TKN-DS-STOPIN-DISCON created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_DS_STOPIN_DISCON 33490937LU
#pragma section zmxo_tkn_svc_stop_abrupt
/* Token Code ZMXO-TKN-SVC-STOP-ABRUPT created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SVC_STOP_ABRUPT 33490938LU
#pragma section zmxo_tkn_svc_strt_warning
/* Token Code ZMXO-TKN-SVC-STRT-WARNING created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SVC_STRT_WARNING 33490939LU
#pragma section zmxo_tkn_sql_not_init
/* Token Code ZMXO-TKN-SQL-NOT-INIT created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SQL_NOT_INIT 33490940LU
#pragma section zmxo_tkn_sav_dsstat_fail
/* Token Code ZMXO-TKN-SAV-DSSTAT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SAV_DSSTAT_FAIL 33490941LU
#pragma section zmxo_tkn_sav_asstat_fail
/* Token Code ZMXO-TKN-SAV-ASSTAT-FAIL created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SAV_ASSTAT_FAIL 33490942LU
#pragma section zmxo_tkn_intrnlcntr_recal
/* Token Code ZMXO-TKN-INTRNLCNTR-RECAL created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_INTRNLCNTR_RECAL 33490943LU
#pragma section zmxo_tkn_collector_error
/* Token Code ZMXO-TKN-COLLECTOR-ERROR created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_COLLECTOR_ERROR 33490944LU
#pragma section zmxo_tkn_trace_info
/* Token Code ZMXO-TKN-TRACE-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_TRACE_INFO 33490945LU
#pragma section zmxo_tkn_res_stat_info
/* Token Code ZMXO-TKN-RES-STAT-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_RES_STAT_INFO 33490946LU
#pragma section zmxo_tkn_query_status_info
/* Token Code ZMXO-TKN-QUERY-STATUS-INFO created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_QUERY_STATUS_INFO 33490947LU
#pragma section zmxo_tkn_rg_estcost
/* Token Code ZMXO-TKN-RG-ESTCOST created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_RG_ESTCOST 33688505LU
#pragma section zmxo_tkn_rg_limit
/* Token Code ZMXO-TKN-RG-LIMIT created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_RG_LIMIT 33688506LU
#pragma section zmxo_tkn_sql_text
/* Token Code ZMXO-TKN-SQL-TEXT created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SQL_TEXT 33491899LU
#pragma section zmxo_tkn_reason
/* Token Code ZMXO-TKN-REASON created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_REASON 33491900LU
#pragma section zmxo_tkn_nexttoken
/* Token Code ZMXO-TKN-NEXTTOKEN created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_NEXTTOKEN 33491901LU
#pragma section zmxo_tkn_eventtype
/* Token Code ZMXO-TKN-EVENTTYPE created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_EVENTTYPE 33491896LU
#pragma section zmxo_tkn_srvobjref
/* Token Code ZMXO-TKN-SRVOBJREF created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SRVOBJREF 33491902LU
#pragma section zmxo_tkn_srvrstate
/* Token Code ZMXO-TKN-SRVRSTATE created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SRVRSTATE 33491903LU
#pragma section zmxo_tkn_procport
/* Token Code ZMXO-TKN-PROCPORT created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_PROCPORT 33688512LU
#pragma section zmxo_tkn_dsstate
/* Token Code ZMXO-TKN-DSSTATE created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_DSSTATE 33491905LU
#pragma section zmxo_tkn_dsid
/* Token Code ZMXO-TKN-DSID created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_DSID 33491907LU
#pragma section zmxo_tkn_debugflag
/* Token Code ZMXO-TKN-DEBUGFLAG created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_DEBUGFLAG 33491906LU
#pragma section zmxo_tkn_ceeparam
/* Token Code ZMXO-TKN-CEEPARAM created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_CEEPARAM 33491908LU
#pragma section zmxo_tkn_insuf_privlgs
/* Token Code ZMXO-TKN-INSUF-PRIVLGS created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_INSUF_PRIVLGS 33490976LU
#pragma section zmxo_tkn_experience_level
/* Token Code ZMXO-TKN-EXPERIENCE-LEVEL created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_EXPERIENCE_LEVEL 33491909LU
#pragma section zmxo_tkn_severity
/* Token Code ZMXO-TKN-SEVERITY created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_SEVERITY 33491910LU
#pragma section zmxo_tkn_event_target
/* Token Code ZMXO-TKN-EVENT-TARGET created on 07/07/2010 at 01:20 */
#define ZMXO_TKN_EVENT_TARGET 33491911LU
