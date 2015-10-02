/**
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**/


#ifndef _HISTRYH_
#define _HISTRYH_

/* T9050 - histry.h   frame trace related definitions */                         /*D40:DFH03126.2*/


#ifdef __cplusplus
   extern "C" {
#endif

#ifndef _KTDMTYP                                                                 /*D40:DFH0402.1*/
    #include <ktdmtyp.h>                                                         /*D40:DFH0402.5*/
#endif                                                                           /*D40:DFH0402.7*/
                                                                                 /*D40:DFH03126.3*/
enum { DEFAULT_PIN = -2 };  /* represents self */

enum { /* return values from HIST_INIT_, HIST_FORMAT_, HIST_GETPRIOR_ */
  HIST_OK              =   0,
  HIST_DONE            =   1,
  HIST_BAD_WIDTH       =  -2,
  HIST_BAD_VERSION     =  -3,
  HIST_BAD_OPTION      =  -4,
  HIST_BAD_CONTEXT     =  -5,
  HIST_NOT_IMPLEMENTED =  -6,
  HIST_INIT_ERROR      =  -7,
  HIST_ERROR           =  -8,
  HIST_BAD_WORKSPACE   =  -9,
  HIST_BAD_FORMAT_CALL = -10,
  HIST_MISSING_HOOK    = -11,                                                    /*G07:DFH1978.1*/
  HIST_TOO_SHORT       = -12,                                                    /*DLL:DFH2613/KIS.25.1*/
  HIST_BAD_PIN         = -13,                                                    /*DLL:DFH2613/KIS.25.2*/
  HIST_FAILURE         = -99                                                     /*DLL:DFH2613/KIS.25.3*/
  };

enum { /* values for HIST_INIT_ options parameter */
  /* Select one of the next group of options */
  HO_Init_Here      =      0, /* Derive context of this call to HIST_INIT_ */    /*D40:DFH03126.5*/
  HO_Init_uContext  =      1, /* Context param designates uContext */            /*D40:DFH03126.6*/
  HO_Init_JmpBuf    =      2, /* Context param designates JmpBuf */              /*D40:DFH03126.7*/
  HO_Init_31Regs    =      3, /* Context param -> special array of 31 regs */    /*D40:DFH03126.8*/
  HO_Init_Address   =      4, /* Context param is code address */                /*D40:DFH0463.1*/
  /* Select any combination of the following options */
  HO_NoSuppress     =   0x10, /* do not suppress transition frames */
  HO_ShowProtected  =   0x20, /* show any protected trap/signal context */
  HO_OneLine        =  0x100, /* produce single line of state display */         /*G07:DFH19783.1*/
  HO_SkipNative     =  0x400, /* Skip native RISC frames */                      /*DLL:DFH2613/KIS.25.4*/
  HO_JustCheck      = 0x1000  /* Validate parameters; return HIST_OK if OK */    /*DLL:DFH2613/KIS.25.5*/
  };

enum { /* Values for FormatSelect field */
  HF_Name         =      1, /* show name if available, else (maybe) code loc */  /*D40:DFH0366.1*/
  HF_Parent       =      2, /* show name of parent of subproc (if HF_name ) */
  HF_Offset       =      4, /* show offset with name */                          /*D40:DFH03126.9*/
  HF_CodeSpace    =      8, /* identify code space (incl. SRL name) */           /*D40:DFH03126.10*/
  HF_LocLineTNS   =   0x10, /* show P E L S when relevant */                     /*D40:DFH03126.11*/
  HF_LocLineRISC  =   0x20, /* show pc VFP FP sp when relevant */                /*D40:DFH03126.12*/
  HF_Context      =  0x100, /* show register values from full context */         /*D40:DFH0366.2*/
  HF_Registers    =  0x200, /* show all known RISC register values */
  HF_Context_TNS  =  0x400, /* show TNS register values from full context */     /*D40:DFH03124.11*/
  HF_Gaps         = 0x8000, /* show --- or ... at new context or ellipsis */     /*D40:DFH0366.3*/
  HF_base         = HF_Name + HF_Offset + HF_CodeSpace,                          /*D40:DFH03124.12*/
  HF_trace        = HF_base + HF_Gaps,                                           /*D40:DFH03124.13*/
  HF_withContext  = HF_trace + HF_Context + HF_Context_TNS,                      /*D40:DFH03124.15*/
  HF_full         = HF_withContext + HF_Registers                                /*D40:DFH03124.16*/
  };

#ifndef pcKind_DEFINED                                                           /*D40:DFH0463.2*/
#define pcKind_DEFINED                                                           /*D40:DFH0463.3*/
enum { /* code type (pcKind value) components and values */                      /*G07:DFH1978.3*/
  pcLib     =    1,
  pcSys     =    2,
  pcAcc     =    4,
  pcNat     =    8,
  pcSRL     =    9, /* pcNat + pcLib */                                          /*G07:DFH1978.4*/
  pcMilli   = 0x10,
  pcBpt     = 0x11,                                                              /*D40:DFH03125.1*/
  pcUnknown = 0x13,                                                              /*G07:DFH1978.5*/
  pcData    = 0x18, /* pcNat + differentiator: executable data */                /*G09:PCA289216.1*/
  pcDLL     = 0x19, /* pcNat + pcLib + differentiator */                         /*G07:DFH1978.6*/
  pcMask    = 0x3F                                                               /*G07:DFH1978.7*/
  };
#define is_pcTNS(k)       (((k) & (pcAcc | pcNat | pcMilli)) == 0)               /*D40:DFH03126.14*/
#define is_pcAccel(k)     (((k) & pcAcc) != 0)                                   /*D40:DFH03126.15*/
#define is_pcTNSorAcc(k)   ((k) < pcNat)                                         /*D40:DFH03126.16*/
#define is_pcNative(k)    (((k) & pcNat) != 0)                                   /*D40:DFH03126.17*/
#define is_pcUser(k)      (((k) & (pcLib | pcSys | pcMilli)) == 0)               /*D40:DFH03126.18*/
#define is_pcSRL(k)        ((k) == pcSRL)                                        /*G07:DFH1978.8*/
#define is_pcDLL(k)        ((k) == pcDLL)                                        /*G07:DFH1978.9*/
#endif /* pcKind_DEFINED */                                                      /*D40:DFH0463.4*/

enum { hW_ExtendedLen2 = 32,                                                     /*G07:DFH19783.3*/
       hW_PrivateLen0 = 8, hW_PrivateLen1 = 32, hW_PrivateLen2 = 560             /*G07:DFH19783.4*/
#if defined(_TNS_TARGET) && !defined(_DHISTH_)                                   /*G09:KIS27826.1*/
       /* 4088 - size( Native NSK_histWorkspace ) */          + 3448             /*G09:KIS27824.1*/
#endif                                                                           /*G07:DFH1978.14*/
     };                                                                          /*G07:DFH1978.15*/
#ifndef _DHISTH_                                                                 /*G07:DFH1978.16*/
  #define hW_extended1        uint8 extendedFiller1;                             /*G07:DFH19783.7*/
  #define hW_extended2        int32 extendedFiller2[hW_ExtendedLen2/4];          /*G07:DFH1978.18*/
#endif                                                                           /*G07:DFH1978.19*/
#ifndef _WHISTH_                                                                 /*G07:DFH1978.20*/
  #define hW_private0         int32 privateFiller0[hW_PrivateLen0/4];            /*G07:DFH19783.8*/
  #define hW_private1         int32 privateFiller1[hW_PrivateLen1/4];            /*G07:DFH1978.21*/
 #ifdef _DHISTH_                                                                 /*G07:DFH1978.22*/
  #define hW_private2         int32 info[hW_PrivateLen2/4]; /* original name */  /*G07:DFH1978.23*/
 #else                                                                           /*G07:DFH1978.24*/
  #define hW_private2         int32 privateFiller2[hW_PrivateLen2/4];            /*G07:DFH1978.25*/
 #endif                                                                          /*G07:DFH1978.26*/
#endif                                                                           /*G07:DFH1978.27*/


typedef struct NSK_histWorkspace {
  hW_private0                                                                    /*G07:DFH19783.9*/
  uint16       FormatSelect;   /* see HF_* literals */                           /*D40:DFH0336.2*/
  hW_extended1                                                                   /*G07:DFH19783.10*/
  uint8        pcKind;         /* pc is TNS/acc/native etc */                    /*G07:DFH1978.29*/
  uint32       pc;             /* pc for current frame (RISC format) */          /*D40:DFH03124.23*/
  hW_private1                                                                    /*G07:DFH1978.30*/
  hW_extended2                                                                   /*G07:DFH1978.31*/
  hW_private2                                                                    /*G07:DFH1978.32*/
} NSK_histWorkspace;
enum { hW_length = sizeof(NSK_histWorkspace) };

/* This "function" takes a HistVer literal and the workspace length;             /*G07:DFH1978.33*/
   it returns a value for the Version parameter to HIST_INIT_.  */               /*G07:DFH1978.34*/
#define HistVerFun(hv,l)  (((hv) & ~0xFFF) | (l))                                /*G07:DFH1978.35*/
                                                                                 /*G07:DFH1978.36*/
/* This macro is used internally to build HistVer literals.  */                  /*G07:DFH1978.37*/
#define HistVF(v,x,r)  (((v) << 24) | ((x) << 16) | ((r) << 12))                 /*G07:DFH1978.38*/
                                                                                 /*G07:DFH1978.39*/
#if defined(_WHISTH_) || !defined(_DHISTH_)                                      /*G07:DFH1978.40*/
enum { /* HIST_INIT_ version values for public interface */                      /*G07:DFH1978.41*/
  HistVersion1  = HistVerFun(HistVF(1, 0, 0), hW_length)                         /*G07:DFH1978.42*/
  };                                                                             /*G07:DFH1978.43*/
enum { HistVersion2 = HistVersion1 };                                            /*G07:DFH1978.44*/
#endif                                                                           /*G07:DFH1978.45*/
                                                                                 /*G07:DFH1978.46*/
enum { HIST_MinWidth = 75 }; /* minimum acceptable limit for HIST_FORMAT_ */     /*D40:DFH03126.24*/

extern int16  HIST_INIT_( NSK_histWorkspace *,     /* io: workspace */           /*G07:DFH1978.47*/
                    const uint32,                  /* i:  HistVersion */         /*G05:DFH15092.2*/
                    const uint16,                  /* i:  options: HO_... */     /*G05:DFH15092.3*/
                    const void * );                /* i:  context designator */  /*G05:DFH15092.4*/

extern int16  HIST_FORMAT_(NSK_histWorkspace *,    /* io: workspace */           /*G05:DFH15092.5*/
                           char *,                 /* o:  output text ptr */     /*G05:DFH15092.6*/
                    const  int16 );                /* i:  max output length */   /*G05:DFH15092.7*/

extern int16  HIST_GETPRIOR_(NSK_histWorkspace *); /* io: workspace */           /*G05:DFH15092.8*/

extern uint32 HIST_WORKLEN_(void);                                               /*G07:DFH1978.48*/

#ifdef __cplusplus                                                               /*D40:DFH03126.26*/
   }                                                                             /*D40:DFH03126.27*/
#endif                                                                           /*D40:DFH03126.28*/
#endif /* _HISTRYH_ */
