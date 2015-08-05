//------------------------------------------------------------------
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
// Process-Control module
//
#ifndef __SB_PCTLCOM_H_
#define __SB_PCTLCOM_H_

#include <stdio.h> // NULL

#include "cc.h" // needed by da.h

#include "int/diag.h"
#include "int/da.h"
#include "int/exp.h"
#include "int/types.h"

// from guardian dprcctlz ('X' prefix added)
enum {
    XPROC_OK          =   0,     // no error at all
    XPROC_FSERROR     =   1,     // file system error in error detail
    XPROC_PRM         =   2,     // parameter error
    XPROC_BNDS        =   3,     // bounds error
    XPROC_SINGLE      =   4,     // single named process
    XPROC_PRIMARY     =   5,     // caller's pair - Caller is primary
    XPROC_BACKUP      =   6,     // caller's pair - Caller is backup
    XPROC_UNNAMED     =   7,     // process is unamed
    XPROC_COMPLETE    =   8,     // search is complete
    XPROC_NONEXTANT   =   9,     // process does not exist
    XPROC_NODEDOWN    =  10,     // unable to communicate with node
    XPROC_IOP         =  11,     // target is IOP and option not specified
    XPROC_DOWNREV_IOP =  12,     // target is IOP on Cxx node
    XPROC_NOMINAL     =  13,     // process exists in name only

    XPROC_INVSTATE    = 100,     // invalid state (SQ only)
    XPROC_PRM_UNSUP   = 101      // param unsupported (SQ only)
};


//
// process functions
//
SB_Export short XPROCESS_GETPAIRINFO_(SB_Phandle_Type *SB_DA(processhandle, NULL),
                                      char            *SB_DA(pair, NULL),
                                      short            SB_DA(maxlen, 0),
                                      short           *SB_DA(pair_length, NULL),
                                      SB_Phandle_Type *SB_DA(primary_processhandle, NULL),
                                      SB_Phandle_Type *SB_DA(backup_processhandle, NULL),
                                      int             *SB_DA(search, NULL),
                                      SB_Phandle_Type *SB_DA(ancestor_phandle, NULL),
                                      char            *SB_DA(node, NULL),
                                      short            SB_DA(node_len, 0),
                                      short            SB_DA(options, 0),
                                      char            *SB_DA(ancestor_desc, NULL),
                                      short            SB_DA(ancestor_desc_maxlen, 0),
                                      short           *SB_DA(ancestor_desc_len, NULL),
                                      short           *SB_DA(error_detail, NULL))
SB_DIAG_UNUSED;

//
// process handle functions
//
SB_Export short XPROCESSHANDLE_DECOMPOSE_(SB_Phandle_Type *processhandle,
                                          int             *SB_DA(cpu,NULL),
                                          int             *SB_DA(pin,NULL),
                                          int             *SB_DA(nodenumber,NULL),
                                          char            *SB_DA(nodename,NULL),
                                          short            SB_DA(nodename_maxlen,
                                                                 XOMITSHORT),
                                          short           *SB_DA(nodename_length,
                                                                 NULL),
                                          char            *SB_DA(procname,NULL),
                                          short            SB_DA(procname_maxlen,
                                                                 XOMITSHORT),
                                          short           *SB_DA(procname_length,
                                                                 NULL),
                                          SB_Int64_Type   *SB_DA(sequence_number,
                                                                 NULL))
SB_DIAG_UNUSED;

#endif // !__SB_PCTLCOM_H_
