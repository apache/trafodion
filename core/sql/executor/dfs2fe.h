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
#ifndef __DFS2FE__
#define __DFS2FE__

#include "Platform.h"                                     // NT_PORT SK 02/10/97

// NT Port - vs 01/22/97
#define dfs2fe_base		20000

enum ExeDfs2feEnum
  {FEFS2FIRSTERR             = dfs2fe_base +  1024        // THIS IS THE FIRST ERROR
                                           // NUMBER RESERVED FOR USE BY
                                           //  FS2.
                                           //------------------------------
   , FENOSUBSET              = dfs2fe_base +  1024        // THE SPECIFIED SUBSET IS NOT
                                           // DEFINED TO THE FILE SYSTEM
                                           //------------------------------
   , FECONSTRAINTVIOL        = dfs2fe_base +  1025        // THE SUPPLIED RECORD OR UPDATE
                                           // VALUE VIOLATES THE TABLE'S
                                           // INTEGRITY CONSTRAINT.
                                           //------------------------------
   , FEVIEWVIOLATION         = dfs2fe_base +  1026        // THE SELECTION EXPRESSION ON A
                                           // VIEW HAS BEEN VIOLATED.
                                           //------------------------------
   , FELABELBAD              = dfs2fe_base +  1027        // THE DISCPROCESS ENCOUNTERED A
                                           // BAD SQL LABEL OR TREE OF LABELS
                                           //------------------------------
   , FELABELWRONGTYPE        = dfs2fe_base +  1028        // THE DISCPROCESS ACCESSED A LABEL OF
                                           // AN UNEXPECTED TYPE DURING OPEN OR
                                           // LABEL DISPLAY.
                                           //------------------------------
   , FEOPENSHAREFAIL         = dfs2fe_base +  1029        // A REQUEST TO SHARE AN EXISTING OPEN
                                           // FAILED DUE TO NO MATCHING OPEN
                                           // FOUND.
                                           //------------------------------
   , FEBADLOCKLEN            = dfs2fe_base +  1030        // BAD LOCK KEY LENGTH
                                           //------------------------------
   , FEBADFIELD              = dfs2fe_base +  1031        // SOME OF THE SUPPLIED FIELDS ARE
                                           // BAD (DECIMAL OR VARCHAR) OR
                                           // THE SUPPLIED RECORD IS TOO LONG.
                                           // ALSO RETURNED BY THE DISCPROCESS WHEN
                                           // IT ENCOUNTERS A BAD FIELD IN A STORED
                                           // RECORD.
                                           //------------------------------
   , FEBADRECDESC            = dfs2fe_base +  1032        // THE RECORD DESCRIPTION IS
                                           // INCONSISTENT.
                                           //------------------------------
   , FEBADKEYDESC            = dfs2fe_base +  1033        // THE KEY FIELD DESCRIPTION IS
                                           // INCONSISTENT.
                                           //------------------------------
   , FEWRONGCATALOG          = dfs2fe_base +  1034        // THE REQUESTED OPERATION HAS
                                           // FAILED DUE TO AN INCONSISTENCY
                                           // IN SPECIFYING THE SQL CATALOG.
                                           // $$$ CAN THIS BE DROPPED? THE
                                           //  DISC-PROCESS DOES NOT USE IT
                                           //  ANYMORE
                                           //------------------------------
   , FEBADFLAGS              = dfs2fe_base +  1035        // THE FLAGS WORD OF THE DM BLOCK
                                           // IS INVALID, INTERNAL ERROR
                                           //------------------------------
   , FEBADFIELDLIST          = dfs2fe_base +  1036        // THE FIELD LIST PASSED
                                           // IS INVALID (INTERNAL ERROR).
                                           // OR THE DISCPROCESS DETECTED AN
                                           // INCONSISTENCY IN THE PROJECTION
                                           // SUPPLIED IN THE DEFINITION OF
                                           // A PROTECTION VIEW.
                                           //------------------------------
   , FEBADEXPR               = dfs2fe_base +  1037        // THE SELECTION EXPRESSION,
                                           // UPDATE EXPRESSION, OR
                                           // INTEGRITY CONSTRAINT PASSED
                                           // IS INVALID, INTERNAL ERROR
                                           //------------------------------
   , FEBADLOCKMODE           = dfs2fe_base +  1038        // THE LOCK MODE PASSED
                                           // IS INVALID, INTERNAL ERROR
                                           //------------------------------
   , FENOUPDATEINTENT        = dfs2fe_base +  1039        // UPDATE WAS CALLED BUT THE SUBSET
                                           // WAS NOT DEFINED WITH UPDATE INTENT
                                           //------------------------------
   , FENOCURRENT             = dfs2fe_base +  1040        // THERE IS NO CURRENT RECORD,
                                           // CURRENCY IS EITHER BEFORE THE FIRST
                                           // RECORD OF THE SET, AFTER THE LAST
                                           // OR IN BETWEEN TWO RECORDS.
                                           //------------------------------
   , FENOTRANDOMSUBSET       = dfs2fe_base +  1041        // DM^KEYPOSITION WAS CALLED ON A
                                           // SEQUENTIAL SUBSET, INTERNAL ERROR
                                           //------------------------------
   , FENODEFAULT             = dfs2fe_base +  1042        // THE OPERATION REQUIRED THAT A
                                           // DEFAULT VALUE BE FILLED IN FOR A
                                           // FIELD DEFINED AS NO DEFAULT.
                                           // ALSO RETURNED BY THE DISCPROCESS WHEN
                                           // IT ENCOUNTERS A RECORD FOR WHICH AN
                                           // ABSENT FIELD SPECIFIED NO DEFAULT.
                                           //------------------------------
   , FENOTUNIQUE             = dfs2fe_base +  1043        // THE ACCESS PATH SPECIFIED DOES
                                           // NOT REPRESENT A UNIQUE KEY OR
                                           // A PORTION OF THE KEY IS NOT
                                           // PROJECTED IN THE VIEW.
                                           //------------------------------
   , FESUBSETEXISTS          = dfs2fe_base +  1044        // THE OPERATION IS NOT ALLOWED
                                           // WHILE A SUBSET EXISTS.
                                           //------------------------------
   , FEWRONGFILETYPE         = dfs2fe_base +  1045        // WRONG FILE TYPE FOR THIS
                                           // OPERATION.
                                           //------------------------------
   , FEBADOPEN               = dfs2fe_base +  1046        // THE PARAMS PASSED TO OPEN ARE
                                           // ALLOWED FOR SQL OBJECTS
                                           // (E.G. SYNCDEPTH > 1, SBB,
                                           // BACKUP OPEN)
                                           //------------------------------
   , FEINDEXINVALID          = dfs2fe_base +  1047        // THE INDEX WHICH HAS BEEN OPENED
                                           // IS MARKED INVALID, THE CATALOG
                                           // MANAGER HAS NOT SUCESSFULLY
                                           // LOADED IT.
                                           //------------------------------
   , FE_BAD_DATALIST         = dfs2fe_base +  1048        // THE DESCRIPTION OF FIELDS TO BE
                                           // RETRIEVED VIA A DM BLOCK IS
                                           // INCORRECT, I.E. ONE OF DM.NUMBER,
                                           // DM.DATALISTP, CONTENT OF FIELD LIST,
                                           // IS INCONSISTENT
                                           //------------------------------
   , FE_BAD_SEL              = dfs2fe_base +  1049        // A SUPPLIED SELECTION EXPRESSION
                                           // IS INVALID.
                                           //------------------------------
   , FECONTINUE_AT           = dfs2fe_base +  1050        // A GET^FIRST REQUEST TO THE DISC
                                           // PROCESS IS RETURNING THE KEY/RECADDR
                                           // OF A RECORD STILL TO BE PROCESSED,
                                           // I.E. THE ENSUING GET^NEXT SHOULD
                                           // SPECIFY DM^AT^FLG = dfs2fe_base +  1 IN THE
                                           // SUPPLIED DMM^NEXT BUFFER.
                                           //------------------------------
   , FECONT_BAD_REC          = dfs2fe_base +  1051        // A VIRTUAL BLOCK GET REQUEST TO THE
                                           // DISC PROCESS IS RETURNING THE
                                           // KEY/RECADDR OF A RECORD WHICH IS
                                           // ABSENT FROM THE END OF THE
                                           // RETURNED VIRTUAL BLOCK DUE TO
                                           // RECORD STRUCTURE ERROR ENCOUNTERED IN
                                           // ATTEMPTING TO RETRIEVE IT.
                                           //------------------------------
   , FECONT_PARITY           = dfs2fe_base +  1052        // A RETRIEVAL REQUEST SENT TO THE
                                           // DISCPROCESS SHOULD BE RE-DRIVEN USING
                                           // "FECONTINUE" PROTOCOL.  THIS RETURN
                                           // CODE CONTAINS THE ADDITIONAL
                                           // INFORMATION THAT THE BLOCK OF THE
                                           // RETURNED LAST PROCESSED KEY
                                           // HAD A DATA PARITY ERROR.
                                           //------------------------------
   , FE_ECC_PARITY           = dfs2fe_base +  1053        // AN ECC ERROR OCCURRED OR A CHECKSUM
                                           // ERROR OCCURRED WHICH MAKES IMPOSSIBLE
                                           // TO PROCESS THE ACCESSED BLOCK.
                                           // NO DATA IS RETURNED.
                                           //------------------------------
   , FE_CRASHLABEL_EXEC      = dfs2fe_base +  1054        // ATTEMPTING TO PERFORM AN EXECUTOR
                                           // OPEN ON A NON-AUDITED TABLE OR P-VIEW
                                           // WHOSE CRASH-LABLE FLAG IS SET, WHICH
                                           // INDICATES THAT AN UNCOMMITTED DDL
                                           // OPERATION EXISTS.
                                           //------------------------------
   , FE_CRASHLABEL_UTIL      = dfs2fe_base +  1055        // ATTEMPTING TO PERFORM A UTILITY,
                                           // NON-TMF OPEN ON A TABLE OR P-VIEW
                                           // WHOSE CRASH-LABLE FLAG IS SET, NOT
                                           // SUPPLYING THE TRANSID OF THE
                                           // TRANSACTION WHICH LOCKED THE OBJECT,
                                           // WHICH INDICATES THAT AN UNCOMMITTED
                                           // DDL OPERATION FROM ANOTHER TRANSACTION
                                           // EXISTS.
                                           //------------------------------
   , FE_OPEN_RECOVERY        = dfs2fe_base +  1056        // ATTEMPTING TO PERFORM A STRUCTURED
                                           // OPEN ON A SQL TABLE WHICH IS PRESENTLY
                                           // OPENED FOR RECOVERY.
                                           //------------------------------
   , FE_LABEL_LOCKED         = dfs2fe_base +  1057        // ATTEMPTING TO OPEN A STRUCTURED
                                           // OPEN ON A SQL TABLE WHICH IS PRESENTLY
                                           // LOCKED BY A TRANSID DIFFERENT FROM THE
                                           // CURRENT ONE.
                                           //------------------------------
   , FE_CLEARONPURGE         = dfs2fe_base +  1058        // ATTEMPTING TO OPEN A STRUCTURED
                                           // OPEN ON A SQL TABLE FOR WHICH A
                                           // DROP WITH CLEAR-ON-PURGE IS IN
                                           // PROGRESS.
                                           //------------------------------
   , FE_FILE_MISSING         = dfs2fe_base +  1059        // THE DISC-PROCESS WAS REQUESTED TO
                                           // PROCESS A PROTECTION VIEW WHOSE BASE
                                           // TABLE DOES NOT EXIST, OR IS
                                           // INCONSISTENT.
                                           //------------------------------
   , FE_BAD_SBB              = dfs2fe_base +  1060        // SBB FLAGS ARE ILLEGAL OR INCONSISTENT
                                           // WITH THE DISC PROCESS REQUEST USED.
                                           //------------------------------
   , FEWRONGCBID             = dfs2fe_base +  1061        // SCB^ID OR ICB^ID NO LONGER IDENTIFIES
                                           // A VALID DISC PROCESS SCB OR ICB.
                                           //------------------------------
   , FEWRONGSCBID            = dfs2fe_base +  FEWRONGCBID // SCB^ID NO LONGER IDENTIFIES A
                                           // VALID DISC PROCESS SCB.
                                           //------------------------------
   , FEWRONGICBID            = dfs2fe_base +  FEWRONGCBID // ICB^ID NO LONGER IDENTIFIES A
                                           // VALID DISC PROCESS ICB.
                                           //------------------------------
   , FE_SCB_RESYNC           = dfs2fe_base +  1062        // SCB NEEDS "RE-SYNCING" FOLLOWING A
                                           // DISC PROCESS TAKEOVER.  FILE SYSTEM
                                           // SHOULD RE-SUBMIT "FIRST" (I.E. SUBSET
                                           // DEFINING) REQUEST WITH SCB-ID INSTEAD
                                           // OF OCB-ID (SETTING DM^SCB^RESYNC^FLG).
                                           //------------------------------
   , FEMISSINGLOCK           = dfs2fe_base +  1063        // DISCPROCESS FAILED TO FIND AN
                                           // EXPECTED LOCK.
                                           //------------------------------
   , FEINVDROP               = dfs2fe_base +  1064        // DISCPROCESS REFUSED TO DROP
                                           // A TABLE BECAUSE P-VIEWS
                                           // FOR THE TABLE STILL EXIST.
                                           //------------------------------
   , FE_LOCK_PROTOCOL        = dfs2fe_base +  1065        // INVALID LOCK PROTOCOL
                                           //------------------------------
   , FEFSINTERNALERROR_1     = dfs2fe_base +  1066        // INTERNAL ERROR IN OPEN
                                           //------------------------------
   , FEFSINTERNALERROR_2     = dfs2fe_base +  1067        // INTERNAL ERROR IN OPEN / DP
                                           // PROTOCOL
                                           //------------------------------
   , FEBADCATNAME            = dfs2fe_base +  1068        // A BAD CATALOG NAME WAS
                                           // DETECTED.
                                           //------------------------------
   , FEPARMSINCONSISTENT     = dfs2fe_base +  1069        // INCONSISTENCY WAS DETECTED
                                           // BETWEEN SOME INPUT PARAMS.
                                           //------------------------------
   , FENOTSQLLICENSED        = dfs2fe_base +  1070        // THE PROCESS DOESN'T HAVE THE
                                           // SQL LICENSE BIT SET.
                                           //------------------------------
   , FELENGTHMISMATCH        = dfs2fe_base +  1071        // LENGTH FIELDS IN A STRUCTURE
                                           // WERE NOT CONSISTENT WITH
                                           // EACH OTHER.
                                           //------------------------------
   , FEINVALIDVALUE          = dfs2fe_base +  1072        // A VALUE WAS SUPPLIED WHICH IS
                                           // OUT OF RANGE.
                                           //------------------------------
   , FE_BAD_IN_DATALIST      = dfs2fe_base +  1073        // THE DESCRIPTION OF FIELDS TO BE
                                           // UPDATED VIA A DM BLOCK IS
                                           // INCORRECT, I.E. ONE OF DM.IN^NUMBER,
                                           // DM.IN^DATALISTP, CONTENT OF FIELD
                                           // LIST IS INCONSISTENT
                                           //------------------------------
   , FEFS2INTERNALERROR      = dfs2fe_base +  1074        // AN INTERNAL ERROR IN AN FS2
                                           // PROCEDURE.
                                           //------------------------------
   , FE_CURRENCY_UNKNOWN     = dfs2fe_base +  1075        // A DM^GET WAS ATTEMPTED FOLLOWING
                                           // A DM^GET OPERATION WHICH FAILED,
                                           // BUT WHICH MAY HAVE CHANGED CURRENCY.
                                           //------------------------------
   , FETIMESTAMP_MISMATCH    = dfs2fe_base +  1076        // ONE OF THE TIMESTAMP'S PASSED
                                           // TO OPEN DOES NOT MATCH
                                           // CATALOG^OPTIME^F IN A LABEL OR
                                           // A NAME PASSED FOR A TIMESTAMP
                                           // CHECK IS UNKNOWN. RECOMPILATION
                                           // SHOULD OCCUR. THE TABLE REMAINS
                                           // OPEN.
                                           //------------------------------
   , FETIMESTAMP_FAILURE     = dfs2fe_base +  1077        // THE CATALOG^OPTIME^F TIMESTAMP
                                           // FOR A PARTITION DOES NOT MATCH
                                           // THE OTHER PARTITIONS. THIS IS
                                           // A SERIOUS CONSISTENCY FAILURE.
                                           // THE TABLE IS CLOSED.
                                           //------------------------------
   , FEBADRECORD             = dfs2fe_base +  1078        // THE DISCPROCESS ENCOUNTERED A STORED
                                           // RECORD WHICH WAS BAD FOR A REASON
                                           // OTHER THAN THOSE DESCRIBED BY
                                           // FEBADFIELD OR FENODEFAULT.
                                           //------------------------------
   , FEBADKEYCOMP            = dfs2fe_base +  1079        // THE REQUESTED KEY COMPRESSION OPTION
                                           //  IS INCONSISTENT WITH THE DATA TYPE,
                                           //  OFFSET IN RECORD, DESCENDING FLAG,
                                           //  OF SOME OF THE KEY FIELDS
                                           //------------------------------
   , FETRANSIDMISMATCH       = dfs2fe_base +  1080        // THE TRANSID IN THE DM BLOCK DOES NOT
                                           // MATCH THE CURRENT TRANSID WHEN:
                                           //    AUDITED WITH LOCK PROTOCOL OR
                                           //    NONAUDITED TEMPORARY FILE
                                           // THE USER MOST LIKELY SWITCHED
                                           // TRANSIDS BETWEEN CURSOR OPEN AND
                                           // FETCH TIME.
                                           //------------------------------
   , FETRANSIDNOTALLOWED     = dfs2fe_base +  1081        // A TRANSID WAS PASSED IN THE DM
                                           // BLOCK BUT IT SHOULD NOT HAVE BEEN.
                                           // PROBABLE SQLEXECUTOR PROBLEM.
                                           //------------------------------
   , FEBUFFERTOOSMALL        = dfs2fe_base +  1082        // A BUFFER PASSED IS NOT LARGE ENOUGH
                                           // TO CONTAIN THE NECESSARY DATA.
                                           // THE BUFFER SIZE NEEDED IS BASED
                                           // UPON THE FIELD LIST (PROJECTION).
                                           //------------------------------
   , FEBADSTRUCTURE          = dfs2fe_base +  1083        // AN INPUT STRUCTURE HAS AN INVALID
                                           // FORMAT, AS INDICATED BY AN
                                           // INCORRECT EYE^CATCHER FIELD VALUE.
                                           //------------------------------
   , FENOINSERT              = dfs2fe_base +  1084        // ATTEMPT TO INSERT INTO A VIEW WHICH
                                           // DOES NOT SUPPORT INSERT.
                                           //------------------------------
   , FEWRONGVERSION          = dfs2fe_base +  1085        // A SUPPLIED DATA STRUCTURE IS A
                                           // VERSION THAT IS NOT SUPPORTED
                                           // BY THIS RELEASE.
                                           //------------------------------
   , FELOCKEXIST             = dfs2fe_base +  1086        // ATTEMPT TO UNLOCK A TABLE FOR WHICH
                                           // SUBSETS WITH TESTLOCK OR KEEPLOCK
                                           // EXIST.
                                           //------------------------------
   , FEDP2INTERNALERROR      = dfs2fe_base +  1087        // THE FILE SYSTEM DETECTED AN
                                           // INTERNAL DISC PROCESS ERROR.
                                           //------------------------------
   , FENOTCREATED            = dfs2fe_base +  1088        // THE PURGE OPERATION DESCRIBED
                                           //  BY AN AUDIT RECORD HAS NOT
                                           //  BEEN BACKED OUT, AND FUTURE
                                           //  OPEN MAY RETURN FENOTFOUND.
                                           // RETURNED ONLY TO BACKOUT AND
                                           //  RECOVERY PROCESSES.
                                           //------------------------------
   , FEKEYMISMATCH           = dfs2fe_base +  1089        // THE ALTERNATE KEY FROM AN
                                           // INDEX RECORD AND BASE TABLE
                                           // RECORD DO NOT MATCH.
                                           //------------------------------
   , FEMSGOVERFLOW           = dfs2fe_base +  1090        // THE REQUESTED OPERATION CANNOT
                                           // BE COMPLETED DUE TO CURRENT
                                           // LIMITATIONS ON MESSAGE SIZES.
                                           //------------------------------
   , FEEXPIRETIME            = dfs2fe_base +  1091        // DROP REFUSED BECAUSE OF
                                           //  VIOLATION OF EXPIRATION TIME
                                           //------------------------------
   , FELABTOOLONG            = dfs2fe_base +  1092        // THE REQUEST REFUSED BECAUSE IT
                                           // WOULD MAKE A LABEL TOO LONG.
                                           //------------------------------
   , FERELRECTOOLONG         = dfs2fe_base +  1093        // FOR RELATIVE FILES ONLY.
                                           // THE MAX PACKED RECORD LENGTH
                                           // IS TOO LONG FOR THE RECORD LENGTH
                                           //------------------------------
   , FEBADNUMFIELDS          = dfs2fe_base +  1094        // FOR KEY-SEQUENCED FILES ONLY.
                                           // INVALID NUMBER OF KEY-FIELDS.
                                           //------------------------------
   , FEBADBLOCKSIZE          = dfs2fe_base +  1095        // REQUESTED BLOCKSIZE IS INVALID.
                                           //------------------------------
   , FERECLENTOOLONG         = dfs2fe_base +  1096        // THE RECORD LENGTH IS TOO LONG
                                           // FOR THE BLOCKSIZE.
                                           //------------------------------
   , FEOFFLINE               = dfs2fe_base +  1097        // ATTEMPTED TO ACCESS AN "OFFLINE"
                                           // OBJECT (OR "DEFINITION INCONSISTENT"
                                           // OBJECT).
                                           //-------------------------------
   , FEFILETOOLARGE          = dfs2fe_base +  1098        // SUPPLIED MAX NUMBER OF EXTENTS
                                           //  IS TOO LARGE (FILE LENGTH WOULD BE
                                           //  > 2**31 - 1), OR THE SUM OF THE
                                           //  PARTITION SIZES WOULD EXCEED
                                           //  THE LARGEST ALLOWABLE SYSKEY.
                                           //-------------------------------
   , FEPARTSDONTMATCH        = dfs2fe_base +  1099        // SOME INTEGRITY CHECK MADE BY OPEN
                                           // COMPARING THE PARTITION DEFINITIONS
                                           // FAILED. PROBABLE CATALOG MANAGER BUG.
                                           //-------------------------------
   , FETRUNC                 = dfs2fe_base +  1100        //TRUNCATION OCCURED (AND WAS ALLOWED)
                                           //-------------------------------
   , FETRUNCNOTALLOWED       = dfs2fe_base +  1101        //TRUNCATION NEEDED BUT WAS PROHIBITED
                                           //-------------------------------
   , FEOVFL                  = dfs2fe_base +  1102        //OVERFLOW OCCURRED
                                           //-------------------------------
   , FEUNDERFLOW             = dfs2fe_base +  1103        //UNDERFLOW OCCURRED
                                           //-------------------------------
   , FEBADDECDATA            = dfs2fe_base +  1104        //DECIMAL DATA WITH NON-DIGITS IN IT
                                           //-------------------------------
   , FESIGNERROR             = dfs2fe_base +  1105        //ASSIGNING NEGATIVE VALUE TO UNSIGNED
                                           // DATA
                                           //-------------------------------
   , FENEGATIVEUNSIGNED      = dfs2fe_base +  1106        //UNSIGNED NUMERIC WITH NEGATIVE
                                           // VALUE WAS ENCOUNTERED
                                           //-------------------------------
   , FEDIVBYZERO             = dfs2fe_base +  1107        //DIVISION BY ZERO WILL OCCUR
                                           //-------------------------------
   , FEUNIMPLEMENTEDDATATYPE               //DATATYPE WILL BE IMPLEMENTED
                             = dfs2fe_base +  1108        // IN A FUTURE VERSION/RELEASE
                                           //-------------------------------
   , FEILLEGALDATATYPE       = dfs2fe_base +  1109        //ILLEGAL DATATYPE VALUE I.E.
                                           // NOT ONE OF THE REC^TYPE^*
                                           //-------------------------------
   , FETYPEINCOMPATIBILITY                 //OPERATION BETWEEN INCOMPATIBLE TYPES
                             = dfs2fe_base +  1110        // WAS REQUESTED
                                           //-------------------------------
   , FEILLEGALDIVIDE         = dfs2fe_base +  1111        // DIVISION REQUESTED IN OTHER-THAN
                                           // BIN64^SIGNED OR FLOAT DATATYPES.
                                           // COMPILER'S TYPE PROPAGATION SHOULD
                                           // CONVERT OPERANDS TO BIN64-SIGNED
                                           // OR FLOAT DATATYPES FOR DIVISION
                                           //-------------------------------
   , FEUNSIGNEDARITH         = dfs2fe_base +  1112        // ARITHMETIC OPERATION WAS REQUESTED
                                           // IN AN UNEXPECTED UNSIGNED DATATYPE
                                           //-------------------------------
   , FEILLEGALOPERATOR       = dfs2fe_base +  1113        // ILLEGAL OPERATOR VALUE $$$
                                           //-------------------------------
   , FEILLEGALPATTERN        = dfs2fe_base +  1114        // ILLEGAL LIKE PATTERN
                                           //-------------------------------
   , FESYSKEYTOOLARGE        = dfs2fe_base +  1115        // A SYSKEY VALUE WAS SPECIFIED THAT
                                           // EXCEEDS THE CURRENT DEFINED SIZE
                                           // OF THE TABLE.
                                           //-------------------------------
   , FEFS2TRANSABORT         = dfs2fe_base +  1116        // THE TRANSACTION WAS ABORTED BY THE
                                           // FILE SYSTEM.
                                           //-------------------------------
   , FEINDEXNOTFOUND         = dfs2fe_base +  1117        // THE SPECIFIED INDEX MAP ENTRY DOES
                                           // NOT EXIST.
                                           //-------------------------------
   , FEIXVIEWNOTFOUND        = dfs2fe_base +  1118        // THE SPECIFIED INDEX VIEW ENTRY DOES
                                           // NOT EXIST.
                                           //-------------------------------
   , FEPARTNOTFOUND          = dfs2fe_base +  1119        // THE SPECIFIED PARTITION ENTRY DOES
                                           // NOT EXIST.
                                           //-------------------------------
   , FEFIELDSNOTININDEX      = dfs2fe_base +  1120        // INDEX ONLY RETRIEVAL REQUESTED BUT
                                           // REQUESTED FIELD NOT IN INDEX
                                           //-------------------------------
   , FEINVALIDPROTECTION     = dfs2fe_base +  1121        // REMOTE USER SPECIFIED LOCAL-ONLY
                                           // AUTHORITY (A,G,O,-) IN SECURITY
                                           // VECTOR FOR THE OBJECT BEING CREATED
                                           // OR ALTERED; OR THE SUPPLIED SECURITY
                                           // DOES NOT GRANT READ AUTHORITY TO
                                           // ALL USERS BEING GRANTED WRITE
                                           // AUTHORITY.
                                           //-------------------------------
   , FEDUPKEYSPEC            = dfs2fe_base +  1122        // DUPLICATE KEY-SPECIFIER SUPPLIED
                                           // IN INDEX MAP ENTRY/ARRAY.
                                           //-------------------------------
   , FEBADTREE_RECOVERY      = dfs2fe_base +  1123        // AN INCOMPLETE TREE OF LABELS WAS
                                           // ENCOUNTERED BY ROLLBACK/ROLLFORWARD
                                           // WHEN TRYING TO OPEN A SQL TABLE.
                                           // RETURNED ONLY INTERNALLY TO
                                           // ROLLBACK/ROLLFORWARD
                                           // AND NEVER TO A SQL END USER.
                                           //--------------------------------
   , FELOCKED_AT             = dfs2fe_base +  1124        // A GET^FIRST REQUEST TO THE DISC
                                           // PROCESS IS RETURNING THE KEY/RECADDR
                                           // OF A RECORD STILL TO BE PROCESSED
                                           // (I.E. THE ENSUING GET^NEXT SHOULD
                                           // SPECIFY DM^AT^FLG = dfs2fe_base +  1 IN THE
                                           // SUPPLIED DMM^NEXT BUFFER) DUE TO
                                           // A LOCKED RECORD BEING ENCOUNTERED
                                           // WHEN USING THE BOUNCELOCK PROTOCOL.
                                           // INTERNAL ERROR WHICH SHOULD NEVER
                                           // BE RETURNED TO A SQL END USER
                                           //--------------------------------
   , FELABELCHANGED          = dfs2fe_base +  1125        // A DISPLAY TABLE TO THE DISC PROCESS
                                           // SUPPLIED A [OCBNUM, VSN] OR A
                                           // [0, VSN], WHICH WAS FOUND NOT TO
                                           // MATCH A CURRENT OPEN OR FILE VSN.
                                           // INTERNAL ERROR WHICH SHOULD NEVER
                                           // BE RETURNED TO A SQL END USER
                                           //--------------------------------
   , FEBAD_IXVIEWARRAY       = dfs2fe_base +  1126        // AN ILLEGAL IXVIEWARRAY WAS SUPPLIED
                                           // TO THE DISC PROCESS IN A TABLE ALTER.
                                           // INTERNAL ERROR WHICH SHOULD NEVER
                                           // BE RETURNED TO A SQL END USER
                                           //--------------------------------
   , FECATVIOL               = dfs2fe_base +  1127        // ATTEMPT TO UPDATE A CATALOG TABLE
                                           // FROM A NON LICENSED PROCESS.
                                           //--------------------------------
   , FETRUNCSCALE            = dfs2fe_base +  1128        // SCALE TRUNCATION OCCURRED ON NUMERIC
                                           // DATA ASSIGNMENT
                                           //--------------------------------
   , FETRUNCSCALENOTALLOWED  = dfs2fe_base +  1129        // SCALE TRUNCATION NEEDED BUT NOT
                                           // ALLOWED
                                           //--------------------------------
   , FENOSQL                 = dfs2fe_base +  1130        // AN ATTEMPT WAS MADE TO USE SQL
                                           // ON A NON-SQL SYSTEM.
                                           //--------------------------------
   , FECANCELLED             = dfs2fe_base +  1131        // THE CURRENT REQUEST TO THE DISK
                                           // PROCESS WAS FOUND ON THE LOCK
                                           // QUEUE AND WAS CANCELLED.
                                           //--------------------------------
   , FESQLNOTCOMPLETE        = dfs2fe_base +  1132        // THIS ERROR IS RETURNED FOR NOWAIT
                                           // SQL REQUESTS THAT HAVE NOT
                                           // COMPLETED.  THIS ERROR IS NOT
                                           // RETURNED TO THE SQL USER.
                                           //--------------------------------
   , FESCBOVERFLOW           = dfs2fe_base +  1133        // THE SUBSET CONTROL BLOCK EXCEEDS
                                           // 32K BYTES. THE SELECTION EXPR OR
                                           // THE UPDATE EXPR MAY BE TOO BIG.
                                           //--------------------------------
   , FE_ENSCRIBECOMPLETE     = dfs2fe_base +  1134        // THIS ERROR IS RETURNED BY DM^WAIT
                                           // TO INDICATE THAT AN ENSCRIBE
                                           // NOWAIT REQUEST HAS COMPLETED.
                                           // THIS ERROR IS NOT RETURNED TO THE
                                           // SQL USER.
                                           //--------------------------------
   , FENEEDNEWOPEN           = dfs2fe_base +  1135        // THIS ERROR INDICATES THAT THE
                                           // SQL EXECUTOR SHOULD OBTAIN A
                                           // NEW OPEN.
                                           // THIS ERROR IS NOT RETURNED TO THE
                                           // SQL USER.
                                           //--------------------------------
   , FESBBCONFLICT           = dfs2fe_base +  1136        // THIS OPERATION MAY CONFLICT WITH A
                                           // CONCURRENT VSBB FOR WRITES OPERATION.
                                           //--------------------------------
   , FEPARTDUPKEY            = dfs2fe_base +  1137        // A SUPPLIED KEY FOR A NEW PARTITION
                                           // APPEARED IN THE EXISTED PARTITIONS.
                                           //--------------------------------
   , FEPARTEXISTS            = dfs2fe_base +  1138        // A SUPPLIED NEW PARTITION
                                           // APPEARED IN THE EXISTED PARTITIONS.
                                           //-----------------------------------
   , FEINVALIDDATE           = dfs2fe_base +  1139        // THE EXPRESSION HAS RESULTED IN AN
                                           // INVALID DATE.
                                           //-----------------------------------
   , FENULLTONONULL          = dfs2fe_base +  1140        // A NULL VALUE IS BEING ASSIGNED TO
                                           // A NON-NULLABLE TARGET.
                                           //------------------------------------
   , FEBADDATEINTSYNTAX      = dfs2fe_base +  1141        // THE SYNTAX FOR THE INPUT DATETIME OR
                                           // INTERVAL VALUE IS NOT CORRECT. THIS
                                           // ERROR IS NOT RETURNED TO THE USER.
                                           //------------------------------------
   , FEBADBUFFERADDR         = dfs2fe_base +  1142        // THE SUPPLIED INPUT OR OUTPUT BUFFER
                                           // ADDRESS IS NOT CONSISTENT WITH THE
                                           // OFFSET OF THE FIRST REQUESTED OR
                                           // SUPPLIED COLUMN.
                                           //------------------------------------
   , FEVSBBFLUSHFAIL         = dfs2fe_base +  1143        // FLUSH OF VSBB BUFFERS FAILED DURING
                                           // REPLY. TRANSACTION IS ABORTED FOR
                                           // AUDITED TABLES. FOR NON-AUDITED
                                           // TABLES DATA LOSS HAS OCCURRED.
                                           //------------------------------------
   , FEVSBBWRITEEXISTED      = dfs2fe_base +  1144        // A NON-AUDITED TABLE HAS AN
                                           // OUTSTANDING INSERT/UPDATE VSBB BUFFER
                                           // WHICH SHOULD BE FLUSHED BEFORE
                                           // ATTEMPTING UNLOCK THE TABLE.
                                           //------------------------------------
   , FESKIPERROR             = dfs2fe_base +  1145        // THIS ERROR IS ONLY RETURNED BY DM^GET
                                           // IF THE DM^SKIP^UNAVAIL^PART^FLAG IS
                                           // SET TO TRUE AT DM^START TIME AND THE
                                           // NEXT PARTITION IS NOT AVAILABLE.
                                           // NEED TO CALL DM^GET AGAIN TO SKIP
                                           // THE UNAVAILABLE PARTITION AND
                                           // RETRIEVE THE NEXT AVAILABLE RECORD.
                                           //------------------------------------
   , FEUNKNOWNOPENREPLY      = dfs2fe_base +  1146        // IOP DID NOT RETURN THE EXPECTED
                                           // REPLY TO AN OPEN REQUEST.
                                           //------------------------------------

   , FECOLLATIONNOTFOUND     = dfs2fe_base +  1147        // THE SPECIFIED COLLATION ENTRY DOES
                                           // NOT EXIST.
                                           //------------------------------------
   , FECOLLATIONEXISTS       = dfs2fe_base +  1148        // THE SPECIFIED COLLATION ENTRY
                                           // ALREADY EXISTS IN THE COLLATION
                                           // ARRAY.
                                           //------------------------------------
   , FECPRLWRONGVERSION      = dfs2fe_base +  1149        // THE VERSION OF CHARACTER PROCESSING
                                           // RULES OBJECT IS NOT SUPPORTED.
                                           //------------------------------------
   , FECPRLWONTFIT           = dfs2fe_base +  1150        // THE ENCODED STRING WONT FIT IN
                                           // THE SUPPLIED BUFFER.
                                           //------------------------------------
   , FECPRLUNKNOWN           = dfs2fe_base +  1151        // RECEIVED AN UNKNOWN ERROR DURING
                                           // ENCODING A COLLATION FIELD.
                                           //------------------------------------
   , FECOLLATIONTOOMANY      = dfs2fe_base +  1152        // THE REQUEST EXCEEDS THE LIMIT ON
                                           // NUMBER OF COLLATIONS ALLOWED ON
                                           // A TABLE.
                                           //------------------------------------
   , FEBADCOLLARRAY          = dfs2fe_base +  1153        // A BAD FIELD IN COLLATION ARRAY
                                           // IS DETECTED.
                                           //------------------------------------
   , FECATNAMEDOESNTMATCH    = dfs2fe_base +  1154        // A SUPPLIED CATALOG NAME DOES NOT
                                           // MATCH THE ONE IN THE LABEL.
                                           //------------------------------------
   , FEPROTECTWRONGVERSION   = dfs2fe_base +  1155        // A SUPPLIED VERSION OF PROTECTION
                                           // IS NOT SUPPORTED IN THIS RELEASE.
                                           //------------------------------------
   , FEBADPROTECTIONINFO     = dfs2fe_base +  1156        // AN INCORRECT DESCRIPTION OF
                                           // PROTECTION IS DETECTED.
                                           //------------------------------------
   , FEUNKNOWNLABELREPLY     = dfs2fe_base +  1157        // IOP DID NOT RETURN THE EXPECTED
                                           // REPLY TO A LABEL REQUEST.
                                           //------------------------------------
   , FEOVFL_AT               = dfs2fe_base +  1158        // OVERFLOW OCCURS WHEN DP2 TRIES TO
                                           // COMPUTE THE AGGREGATE VALUE FOR THE
                                           // CURRENT GROUP.
                                           //------------------------------------
   , FEBADPARTKEY            = dfs2fe_base +  1159        // THE SUPPLIED KEY FOR A NEW
                                           // PARTITION COLLATES PRIOR TO THE
                                           // ROOT PARTITION.
                                           //------------------------------------
   , FEBADCOLLENTRY          = dfs2fe_base +  1160        // A BAD FIELD IN COLLATION ENTRY
                                           // IS DETECTED.
                                           //------------------------------------
   , FEUNKNOWNCHARSET        = dfs2fe_base +  1161        // A CHARACTER SET UNKNOWN TO TANDEM
                                           // IS DETECTED.
                                           //------------------------------------
   , FEUNKNOWNMBCSERROR      = dfs2fe_base +  1162        // AN UNKNOWN ERROR IS RECEIVED WHEN
                                           // CALLING MBCS PROCEDURES, E.G., WHEN
                                           // UPSHIFTING A MBCS COLUMN.
                                           //------------------------------------
   , FERESERVEDNAME          = dfs2fe_base +  1163        // ATTEMTING TO CREATE A TABLE/FILE
                                           // WITH A ZZIVNNNN NAME.
                                           //------------------------------------
   , FECHARSETINCOMPATIBILITY              //OPERATION BETWEEN INCOMPATIBLE
                             = dfs2fe_base +  1164        // CHARACTER SETS WAS REQUESTED
                                           //-------------------------------
   , FENOAGGREXPR            = dfs2fe_base +  1165        // NO AGGREGATE EXPRESSION WAS SUPPLIED
                                           // WITH AGGREGATE REQUEST.
                                           //------------------------------------
   , FEBADAGGRCOMPEXPR       = dfs2fe_base +  1166        // BAD AGGREGATE COMPOSITE EXPRESSION.
                                           //------------------------------------
   , FEWRONGLOCALVERSION     = dfs2fe_base +  1167        // HLSQLV OF THE SQL OBJECT IS HIGHER
                                           // THAN THE SQL SYSTEM VERSION OF THE
                                           // NODE WHERE THE SQL COMMAND IS
                                           // IS ISSUED.
                                           //------------------------------------
   , FEWRONGREMOTEVERSION    = dfs2fe_base +  1168        // HLSQLV OF THE SQL OBJECT IS HIGHER
                                           // THAN THE SQL SYSTEM VERSION OF THE
                                           // NODE WHERE THE PARTITION OF THE
                                           // SQL OBJECT RESIDES, AND THE ACCESS
                                           // TO THAT PARTITION IS NEEDED IN THE
                                           // EXECUTION OF A SQL STATEMENT.
                                           //------------------------------------
   , FEINVALIDVERSION        = dfs2fe_base +  1169        // 1) THE HLSQLV IS ZERO (CREATED AT
                                           // R1/R2) BUT ITS CORRESPONDING DPV
                                           // IS NEITHER 0 (R1) NOR 1 (R2). OR
                                           // 2) THE SQL SYSTEM VERSION IS INVALID,
                                           // I.E., NOT IN ONE OF KNOWN SQL RELEASES
                                           //------------------------------------
   , FEBADEXPRVERSION        = dfs2fe_base +  1170        // HLSQLV OF THE SQL EXPRESSION IS
                                           // HIGHER THAN THE SQL SYSTEM VERSION
                                           // OF NODE WHERE THE EXPRESSION IS BEING
                                           // EVALUATED.
                                           //------------------------------------
                                           //------------------------------------
   , FEMSGCOUNTOVERFLOW      = dfs2fe_base +  1171        // # OF MESSAGE BYTES > %17777767777D
                                           // MAYBE LOOPING BETWEEN DP2 AND FS2
                                           //------------------------------------
   , FEFS2WRONGREQID         = dfs2fe_base +  1172        // RESERVED FOR FS2 INTERNAL USE
                                           // IN PROC KERNEL^TIMED^SENDBUF.
                                           // SUPPOSED NOT TO BE USED OR SEEN
                                           // BY ANY OTHER PROCEDURE.
                                           //------------------------------------
   // * * * DON'T FORGET TO CHANGE AFS2FE TO MATCH THIS FILE * * *
   , FERCLABELINCONSISTENT   = dfs2fe_base +  1173        // THE SLAB RC DOES NOT MATCH THE TABLE
                                           // OR INDEX LABELS.
                                           //------------------------------------
   , FEWRONGOP		     = dfs2fe_base + 1177
   , FERESENDDATA	     = dfs2fe_base + 1178
   , FEMOREDATA		  = dfs2fe_base + 1179
   , FESEQUENCE        = dfs2fe_base + 1180
   , FESIDETREE        = dfs2fe_base + 1181
   , FENOSQLMX         = dfs2fe_base + 1183
   , FECPUSWITCHED     = dfs2fe_base + 1184
   , FESQLMXINTERNALERR= dfs2fe_base + 1185
   , FENOEIDSPACE      = dfs2fe_base + 1187 // out of memory in EID
   , FENOSELFREFERENCE       = dfs2fe_base + 1191
   , FELOCKSELF              = dfs2fe_base + 1192
   , FEFSALLOCATIONFAILURE   = dfs2fe_base + 1193
   , FEREOPEN                = dfs2fe_base + 1194
   , FEBROKENACCESSPATH      = dfs2fe_base + 1195
}
  // Please keep this file in sync with sql/executor/dfs2fe.h
  // * * * DON'T FORGET TO CHANGE AFS2FE TO MATCH THIS FILE * * *
  /*, FE<ERROR>           = dfs2fe_base +  <NNNN> */      // <COMMENT>
                                           //
  ;                                        // TERMINATING SEMI-COLON FOR THE
                                           //   PRECEDING LITERAL LIST


#endif
