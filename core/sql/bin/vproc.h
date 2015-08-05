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
/*-*-C++-*-
 *****************************************************************************
 *
 * File:         vproc.h
 * Created:      02/26/99
 * Language:     C++
 *
 *
 *
******************************************************************************
//
******************************************************************************
*/

#ifndef vproc_h
#define vproc_h

/* Following DEFINES should be modified for each release */
/* to build a new VersionProc. */

/* VPROC for NSF (NonStop Framework) */
#define PRODNUMNSF S0023N29
#define DATE1NSF 25FEB2011_ANI
#define NSF_CC_LABEL 20110228

/* VPROC for MXPAR (SQL Parser, also used by preprocessors) */
#define PRODNUMPAR S0200N29
#define DATE1PAR 25FEB2011_ANI
#define PAR_CC_LABEL 20110228

/* VPROC for MXCMP (SQL Compiler) */
#define PRODNUMCMP T1050N29
#define DATE1CMP 25FEB2011_ANI
#define CMP_CC_LABEL 20110228

/* VPROC for MXCUM (SQL Module Compiler) is identical to MXCMP's */
#define PRODNUMCUM T1050N29
#define DATE1CUM 25FEB2011_ANI
#define CUM_CC_LABEL 20110228

/* VPROC for MXEXE (Executor System Library) */
#define PRODNUMEXE T1051N29
#define DATE1EXE 25FEB2011_ANI
#define EXE_CC_LABEL 20110228

#define PRODNUMEXE_STRING "T1051N29"
#define DATE1EXE_STRING "25FEB2011_ANI"
#define EXE_CC_LABEL_STRING "20110228"

/* VPROC for MXESP (Executor Server Process) */
#define PRODNUMESP T1051N29
#define DATE1ESP 25FEB2011_ANI
#define ESP_CC_LABEL 20110228

#define PRODNUMESP_STRING "T1051N29"
#define DATE1ESP_STRING "25FEB2011_ANI"
#define ESP_CC_LABEL_STRING "20110228"

/* VPROC for MXCI (command interpreter) */
#define PRODNUMMXCI T1054N29
#define DATE1MXCI 25FEB2011_ANI
#define MXCI_CC_LABEL 20110228

/* VPROC for MXSQLC (SQL C Preprocessor) */
#define PRODNUMMXSQLC T1052N29
#define DATE1MXSQLC 25FEB2011_ANI
#define MXSQLC_CC_LABEL 20110228

/* VPROC for MXSQLCO (SQL Cobol Preprocessor)  */
#define PRODNUMMXSQLCO T1053N29
#define DATE1MXSQLCO 25FEB2011_ANI
#define MXSQLCO_CC_LABEL 20110228

/* VPROC for MXSQLCO (NT HOSTED SQL Cobol Preprocessor) */
#define	DATE_LABEL_STRING "20110228"
#define PRODNUMMXSQLCONT "T0610N29"
#define DATE1MXSQLCONT "25FEB2011_ANI"

/* VPROC for MXSQLCO (NT HOSTED SQL C Preprocessor) */
#define PRODNUMMXSQLCNT "T0607N29"
#define DATE1MXSQLCNT "05JUL2010_ALW"

/* VPROC for MXVQP (SQL/MX Visual Query Planner) */
#define PRODNUMMXVQP "T0536N29"
#define DATE1MXVQP "05JUL2010_ALW"

/* VPROC for EID (Executor in DP2) */
#define PRODNUMEID T1051N29
#define DATE1EID 25FEB2011_ANI
#define EID_CC_LABEL 20110228

/* VPROC for MXSRL (SQL/MX nonpriv SRL) */
#define PRODNUMMXSRL T1051N29
#define DATE1MXSRL 25FEB2011_ANI
#define MXSRL_CC_LABEL 20110228

/* VPROC for MXSRP (SQL/MX PRIV SRL) */
#define PRODNUMMXPSRL T1051N29
#define DATE1MXPSRL 25FEB2011_ANI
#define MXPSRL_CC_LABEL 20110228

/* VPROC for SQLCLIO */
#define PRODNUMSQLCLIO T1051N29
#define DATE1SQLCLIO 25FEB2011_ANI
#define SQLCLIO_CC_LABEL 20110228

/* VPROC for MXUDR (SPJ Server) */
#define PRODNUMMXUDR T1230N29
#define DATE1MXUDR 25FEB2011_ANI
#define UDR_CC_LABEL 20110228

/* VPROC for Language Manager (Langman) */
#define PRODNUMMXLANGMAN T1231N29
#define DATE1LANGMAN 25FEB2011_ANI
#define LANGMAN_CC_LABEL 20110228

/* VPROC for TDMMXO (MX in Open) */
#define PRODNUMTDMMXO T1051N29
#define DATE1TDMMXO 25FEB2011_ANI
#define TDMMXO_CC_LABEL 20110228

/* VPROC for MXUTP (UTP Server) */
#define PRODNUMMXUTP T1056N29
#define DATE1MXUTP 25FEB2011_ANI
#define MXUTP_CC_LABEL 20110228

/* VPROC for IMPORT (Import Utility) */
#define PRODNUMIMPORT T0208N29
#define DATE1IMPORT 25FEB2011_ANI
#define IMPORT_CC_LABEL 20110228

/* VPROC for NSP (NonStop Security Process) */
#define PRODNUMNSP T0799N29
#define DATE1NSP 25FEB2011_ANI
#define NSP_CC_LABEL 20110228

/* VPROC for SAFDLL (NonStop Security Framework) */
#define PRODNUMSAFDLL T1272N29
#define DATE1SAFDLL 25FEB2011_ANI
#define SAFDLL_CC_LABEL 20110228

/* VPROC for PAMPDLL (NonStop Security Framework) */
#define PRODNUMPAMPDLL T1272N29
#define DATE1PAMPDLL 25FEB2011_ANI
#define PAMPDLL_CC_LABEL 20110228
#define PRODNUMNSLDLL T1272N29
#define DATE1NSLDLL 25FEB2011_ANI
#define NSLDLL_CC_LABEL 20110228

/* VPROC for SUPERTCLL (NonStop Security Framework) */
#define PRODNUMSUPERTCL T0799N29
#define DATE1SUPERTCL 25FEB2011_ANI
#define SUPERTCL_CC_LABEL 20110228

/* VPROC for PAMDLL (NonStop Security Framework) */
#define PRODNUMPAMDLL T1272N29
#define DATE1PAMDLL 25FEB11_ANI
#define PAMDLL_CC_LABEL 20110228

/* VPROC for LDPEDLL (NonStop Security Framework) */
#define PRODNUMLDPEDLL T1272N29
#define DATE1LDPEDLL 25FEB2011_ANI_LD
#define LDPEDLL_CC_LABEL 20110228

/* VPROC for LDPSDLL (NonStop Security Framework) */
#define PRODNUMLDPSDLL T1272N29
#define DATE1LDPSDLL 25FEB2011_ANI
#define LDPSDLL_CC_LABEL 20110228

/* VPROC for LDAPGO (NonStop Security Framework) */
#define PRODNUMLDAPGO T1272N29
#define DATE1LDAPGO 25FEB2011_ANI
#define LDAPGO_CC_LABEL 20110228

/* VPROC for LDAPBACK (NonStop Security Framework) */
#define PRODNUMLDAPBACK T1272N29
#define DATE1LDAPBACK 25FEB2011_ANI
#define LDAPBACK_CC_LABEL 20110228

/* VPROC for LDCFSRV (NonStop Security Framework) */
#define PRODNUMLDCFSRV T1272N29
#define DATE1LDCFSRV 25FEB2011_ANI
#define LDCFSRV_CC_LABEL 20110228

/* VPROC for SECSERV (Password Security Server) */
#define PRODNUMSECSERV T0799N29
#define DATE1SECSERV 25FEB2011_ANI
#define SECSERV_CC_LABEL 20110228

/* VPROC for MXTOOL (MXTOOL Utility ) */
#define PRODNUMMXTOOL T1056N29
#define DATE1MXTOOL 25FEB2011_ANI
#define MXTOOL_CC_LABEL 20110228

/* VPROC for MXGNAMES (MXGNAMES Utility ) */
#define PRODNUMMXGNAMES T1056N29
#define DATE1MXGNAMES 25FEB2011_ANI
#define MXGNAMES_CC_LABEL 20110228

/* VPROC for BRMXAGENT (Backup/Restore Utility ) */
#define PRODNUMBRMXAGENT T1057N29
#define DATE1BRMXAGENT 25FEB2011_ANI
#define BRMXAGENT_CC_LABEL 20110228
#define PRODNUMMXEXPORTDDL T1056N29
#define DATE1MXEXPORTDDL 25FEB2011_ANI
#define MXEXPORTDDL_CC_LABEL 20110228
#define PRODNUMMXIMPORTDDL T1056N29
#define DATE1MXIMPORTDDL 25FEB2011_ANI
#define MXIMPORTDDL_CC_LABEL 20110228
#define PRODNUMMXPREPAREMAP T1056N29
#define DATE1MXPREPAREMAP 25FEB2011_ANI
#define MXPREPAREMAP_CC_LABEL 20110228

/* VPROC for CMP1420 (SQL Downrev Compiler) */
#define PRODNUMDOWNREVCMP T1270N29
#define DATE1DOWNREVCMP 25FEB2011_ANI
#define DOWNREVCMP_CC_LABEL 20110228
#define PRODNUMGENCOLLECT T1271N29
#define DATE1GENCOLLECT 25FEB2011_ANI
#define GENCOLLECT_CC_LABEL 20110228
#define PRODNUMMMCLEAN T1271N29
#define DATE1MMCLEAN 25FEB2011_ANI
#define MMCLEAN_CC_LABEL 20110228

/* VPROC for MXSTUBS (STUBS file for SQLMX ) */
#define PRODNUMMXSTUBS T0367N29
#define DATE1MXSTUBS 25FEB2011_ANI
#define MXSTUBS_CC_LABEL 20110228

/* VPROC for DDL Licensing */
#define PRODNUMDDLLIC T0394N29
#define DATE1DDLLIC 25FEB2011_ANI
#define DDLLIC_CC_LABEL 20110228

/* VPROC for EXE Licensing */
#define PRODNUMEXELIC T1051N29
#define DATE1EXELIC 25FEB11_ANI_L
#define EXELIC_CC_LABEL 20110228

/* VPROC for MXAUDSRV (Audit Fixup utility for SQLMX ) */
#define PRODNUMMXAUDSRV T0397N29
#define DATE1MXAUDSRV 25FEB2011_ANI
#define MXAUDSRV_CC_LABEL 20110228

/* VPROC for MXMAINTAIN (Automated table maintenance for SQLMX ) */
#define PRODNUMMXMAINTAIN T1051N29
#define DATE1MXMAINTAIN 25FEB2011_ANI
#define MXMAINTAIN_CC_LABEL 20110228

/* DEFINE for appending three strings; used in building name of VersionProc */
/* e.g. VPROC (One,Two,Three) becomes OneTwoThree */
#define VPROC(a,b,c) _VPROC(a,b,c)     /* to evaluate arguments here */
#define _VPROC(a,b,c) a##_##b##_##c
#define RELUDR MXUDR            /* Added:   5/21/2001 */

/* A DEFINE for a VPROC like string to be used for NT programs,TEST */
/* use the "strings" command on NT to see this, grep for some */
/* unique text (usually all or part of the string provided in parameter a)*/
#define NTVPROC(a,b,c) _NTVPROC(a,b,c)
#define _NTVPROC(a,b,c) static char NTVprocString ## a [] = \
 "NTVPROC=" #a "_" #b "_" #c

/* VPROC for MXSSCP (Executor Server Process) */
#define PRODNUMSSCP T1051N29
#define DATE1SSCP 25FEB2011_ANI
#define SSCP_CC_LABEL 20110228

/* VPROC for MXSSMP (Executor Server Process) */
#define PRODNUMSSMP T1051N29
#define DATE1SSMP 25FEB2011_ANI
#define SSMP_CC_LABEL 20110228
#define	DATE_LABEL_STRING "20110228"

/* VPROC for MXQMS (MVQR Query Matching Server) */
#define PRODNUMQMS T1050N29
#define DATE1QMS 25FEB2011_ANI
#define QMS_CC_LABEL 20110228

/* VPROC for MXQMM (MVQR Query Matching Monitor) */
#define PRODNUMQMM T1050N29
#define DATE1QMM 25FEB2011_ANI
#define QMM_CC_LABEL 20110228

/* VPROC for MXQMP (MVQR Query Matching Publisher) */
#define PRODNUMQMP T1050N29
#define DATE1QMP 25FEB2011_ANI
#define QMP_CC_LABEL 20110228

/* VPROC for MX BDR Server */
#define PRODNUMBDRSRV T1051N29
#define DATE1BDRSRV 25FEB2011_ANI
#define BDRSRV_CC_LABEL 20110228

/* VPROC for MX BDRSDR */
#define PRODNUMBDRDRC T1051N29
#define DATE1BDRDRC 25FEB2011_ANI
#define BDRDRC_CC_LABEL 20110228
#define	DATE_LABEL_STRING "20110228"

/* VPROC for MXQVP (Query Validation Process for QI) */
#define PRODNUMQVP T1050N29
#define DATE1QVP 25FEB2011_ANI
#define QVP_CC_LABEL 20110228

#endif
/* end of file */


