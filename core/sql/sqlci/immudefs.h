#ifndef IMMUDEFS_H
#define IMMUDEFS_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         immudefs.h
 * RCS:          $Id: immudefs.h,v 1.2 1997/04/23 00:31:32  Exp $
 * Description:  
 *   C form of IMMUDEFS.             
 *               
 * Created:      2/20/96
 * Modified:     $ $Date: 1997/04/23 00:31:32 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
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
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
// Change history:
// 
// $Log: immudefs.h,v $
// Revision 1.2  1997/04/23 00:31:32
// Merge of MDAM/Costing changes into SDK thread
//
// Revision 1.1.1.1.2.1  1997/04/11 23:25:16
// Checking in partially resolved conflicts from merge with MDAM/Costing
// thread. Final fixes, if needed, will follow later.
//
// Revision 1.4.4.1  1997/04/10 18:33:40
// *** empty log message ***
//
// Revision 1.1.1.1  1997/03/28 01:39:42
// These are the source files from SourceSafe.
//
// 
// Revision 1.3  1996/05/31 21:35:57
// no change
//
// Revision 1.2  1996/04/05 20:19:10
// Included the standard banner with RCS strings.
// 

#include "Platform.h"

// Required only for sunobj builds
#define _tal
#define _far
#define _extensible


enum {
   IMMUerr_successful           =   0, /* Processing was successful. */
   IMMUerr_message_truncated    =  -1, /* Message was too large for buffer. */
   IMMUerr_invalid_key          =  -2, /* An invalid key was passed. */
   IMMUerr_missing_param        =  -3, /* Required parameter missing. */
   IMMUerr_not_KS_file          =  -4, /* Not key-sequenced file. */
   IMMUerr_incompatible_version =  -5, /* Incompatible Record version format. */
   IMMUerr_wrong_message_file   =  -6, /* Message file not for Product Number. */
   IMMUerr_invalid_message_file =  -7, /* Invalid data detected in file. */
   IMMUerr_file_not_open        =  -8  /* Tried to close unopened file. */
     } ;

enum {
   len_RecordType = 2,                 /* Length of Record Type field */
   len_ProductNum = 8,                 /* Length of product number field */
   len_HelpTopic = 31,                 /* Length of help field */
   len_Keyword = 31                    /* Length of keyword field */
     } ;

struct IMMUkey_def {
    /* Key fields */
    char IMMUkey_product_number[len_ProductNum];
                                       /* Product Number and Version */

    char IMMUkey_record_type[len_RecordType];
                                       /* Record Type */
                                       /*   VR: Version Record */
                                       /*   ER: Error Record */
                                       /*   HL: Help Record */
                                       /*   KY: Keyword Record */

    short  IMMUkey_key_field1;         /* If VR then doesn't matter */
                                       /* If ER then Error Number */
                                       /* If HL then doesn't matter */
                                       /* If KY then doesn't matter */

    short  IMMUkey_key_field2;         /* If VR then doesn't matter */
                                       /* If ER then Line Number */
                                       /* If HL then Line Number */
                                       /* If KY then Alias Number */

    char   IMMUkey_key_field3[len_HelpTopic];
                                       /* If VR then doesn't matter */
                                       /* If ER then doesn't matter */
                                       /* If HL then Help Topic */
                                       /* If KY then Known Keyword */
    char   _filler;
  } ;

struct IMMU_filename {
    short    length;
    char *   file_name;
  } ;

/*                                               */
/* New interface procedures for D00 release.     */
/*                                               */

_tal _extensible short   IMMU_OPEN_(
  char  _far *Fname,                      /* Message File Name.          IN */
  short       Flen,                       /* Message File Length         IN */
  short _far *Fnum,                       /* Message File Number.        OUT */
  char  _far *ProdNum,                    /* Tandem Product Number.      IN */
  short _far *SpecialSymbol,              /* Special Symbol.             OU */
  short _far *Language       /* Opt   */  /* Language.                   OU */
  );

_tal _extensible short   IMMU_READ_FORMAT_(
  short        Fnum,                      /* Message File Number.       IN  */
  struct IMMUkey_def _far *Key,           /* Access Key.                IN  */
  char   _far *Msg,                       /* Message.                   IN  */
  short        MaxLen,                    /* Maximum Length to Return.  IN  */
  short  _far *Len,          /* Opt   */  /* Actual Length Returned.    I/O */
  short        SpecialSymbol,/* Opt   */  /* Special Symbol.            IN  */
  short  _far *MoreLines,    /* Opt   */  /* Indicates more lines for msg. O*/
  char   _far *Parm1,        /* Opt   */  /* Substitutable Parmeter.     IN */
  char   _far *Parm2,        /* Opt   */  /* Substitutable Parmeter.     IN */
  char   _far *Parm3,        /* Opt   */  /* Substitutable Parmeter.     IN */
  char   _far *Parm4,        /* Opt   */  /* Substitutable Parmeter.     IN */
  char   _far *Parm5         /* Opt   */  /* Substitutable Parmeter.     IN */
  );

_tal _extensible short   IMMU_FORMAT_(
  short        SpecialSymbol,             /* Special Symbol              IN */
  char   _far *Msg,                       /* Message.                    I/O*/
  short        MaxLen,                    /* Maximum Length to Return.   IN */
  short  _far *Len,          /* Opt   */  /* Actual Length Returned.     I/O*/
  char   _far *Parm1,                     /* Substitutable Parameter.    IN */
  char   _far *Parm2,        /* Opt   */  /* Substitutable Parameter.    IN */
  char   _far *Parm3,        /* Opt   */  /* Substitutable Parameter.    IN */
  char   _far *Parm4,        /* Opt   */  /* Substitutable Parameter.    IN */
  char   _far *Parm5         /* Opt   */  /* Substitutable Parameter.    IN */
  );

_tal _extensible short   IMMU_MESSAGE_FILE_ERROR_(
  short  _far *Fname,                     /* Name of Message File.       IN */
  short        Flen,                      /* Length of Message File.     IN */
  short        Error,                     /* Error Which Occurred.       IN */
  char   _far *Msg,                       /* Message.                    OUT*/
  short        MaxLen,                    /* Maximum Length to Return.   IN */
  short  _far *Len,             /* Opt */ /* Actual Length Returned.     OUT*/
  struct IMMUkey_def _far  *Key /* Opt */ /* Access Key.                 IN */
  );


_tal _extensible short   IMMU_CLOSE_(
  short  Fnum                            /* Message File Number.        IN */
  );

#endif /* IMMUDEFS_H */
