/********************************************************************  
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
********************************************************************/

/*******************************************************************************
MODULE CSInterface.h    : Define prototypes and stuff for MXCI interface


DESCRIPTION
This defines things needed in the CSInterface.cpp module that others will need too.
This is the only header used by MXCI from this library, so keep it minimal so it
does not change often (or all of MXCI must be compiled too).

MACL = MXCS Admin Commands Library

CS is added to all enums and structure names so they do not conflict with
RWInterface.h ones in MXCI builds.  The values are the same.
********************************************************************************/

#ifndef CSINTERFACE_H
#define CSINTERFACE_H

// PRAGMA FOR NSK COMPILES

// ENUM AND DEFINES
#define CSMTEXT   500       // Space to allocate in each error struct for text

enum CSRetStatus {          // All functions return these values
    CSERROR = -1,
    CSSUCCESS,                          // SUCCESS = 0 already in zmxocfg.h
    CSDONE,
    CSGET_OUTPUT_ROW,
    CSGET_ERROR_INFO
};

// STRUCTURES AND TYPEDEFS
typedef struct cserrorvalue {           // error report structure
    Int32 errorCode_;                     // the error number from the msg file
    char *charparam1_;                  // substitution string 1 ptr if needed
    char *charparam2_;                  // substitution string 2 ptr if needed
    char *charparam3_;                  // substitution string 3 ptr if needed
    Lng32 intparam1_;                    // substitution long int 1 if needed
    Lng32 intparam2_;                    // substitution long int 2 if needed
    Lng32 intparam3_;                    // substitution long int 3 if needed
    char textbuf_[CSMTEXT];             // space for the strings
} CSErrorValue;

// EXTERNAL FUNCTION PROTOTYPES
#ifdef __cplusplus
extern "C" {                            // stop the mangler from touching names
#endif
Lng32 CS_MXCI_sendQuery(void *CSEnv, char *cmd, Lng32 len);
Lng32 CS_MXCI_getErrorInfo(void *CSEnv, CSErrorValue* &err);
Lng32 CS_MXCI_getReportLine(void *CSEnv, char* &line, Lng32 &len);
Lng32 CS_MXCI_Constructor(void* &CSEnv);
Lng32 CS_MXCI_Destructor(void *CSEnv);
Lng32 CS_MXCI_handleBreak(void *CSEnv);
void CS_MXCI_StopContext(void *CSEnv);
#ifdef __cplusplus 
} 
#endif

#endif
// END MODULE
