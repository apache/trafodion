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
//
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         immudefs.C
 * RCS:          $Id: immudefs.cpp,v 1.2 1997/04/23 00:31:30  Exp $
 * Description:  
 *   Simulated functions for IMMU calls.            
 *               
 * Created:      2/23/96
 * Modified:     $ $Date: 1997/04/23 00:31:30 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
// Change history:
// 
// $Log: immudefs.cpp,v $
// Revision 1.2  1997/04/23 00:31:30
// Merge of MDAM/Costing changes into SDK thread
//
// Revision 1.1.1.1.2.1  1997/04/11 23:25:14
// Checking in partially resolved conflicts from merge with MDAM/Costing
// thread. Final fixes, if needed, will follow later.
//
// Revision 1.4.4.1  1997/04/10 18:33:38
// *** empty log message ***
//
// Revision 1.1.1.1  1997/03/28 01:39:42
// These are the source files from SourceSafe.
//
// 
// 6     1/22/97 11:04p 
// Merged UNIX and NT versions.
// 
// 4     1/14/97 4:55a 
// Merged UNIX and  NT versions.
// Revision 1.4  1996/10/11 23:32:33
// Modified the functions for error generation to use ComDiags.
//
// Revision 1.3  1996/05/31 21:16:10
// no change
//
// Revision 1.2  1996/04/05 20:08:03
// Included the standard banner with RCS strings.
// 

#include <string.h>

#include "immudefs.h"
#include "Platform.h"

// All the following functions are required for 
// sun objects only. For OSS objects, these functions
// are available in the system library.


_tal _extensible short   IMMU_OPEN_(
  char  _far *Fname,                      /* Message File Name.          IN */
  short       Flen,                       /* Message File Length         IN */
  short _far *Fnum,                       /* Message File Number.        OUT */
  char  _far *ProdNum,                    /* Tandem Product Number.      IN */
  short _far *SpecialSymbol,              /* Special Symbol.             OU */
  short _far *Language                    /* Optional                   OU */
  )
{
  return 0 ;
}

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
  )
{
  

  strcpy (Msg, "\nSorry, the HELP messages will be available only when we move to OSS" ) ;
  *Len = 68 ;
  *MoreLines = 0 ;

  return 0 ;
}

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
  )
{
  return 0 ;
}

_tal _extensible short   IMMU_MESSAGE_FILE_ERROR_(
  short  _far *Fname,                     /* Name of Message File.       IN */
  short        Flen,                      /* Length of Message File.     IN */
  short        Error,                     /* Error Which Occurred.       IN */
  char   _far *Msg,                       /* Message.                    OUT*/
  short        MaxLen,                    /* Maximum Length to Return.   IN */
  short  _far *Len,             /* Opt */ /* Actual Length Returned.     OUT*/
  struct IMMUkey_def _far  *Key /* Opt */ /* Access Key.                 IN */
  )
{
  return 0 ;
}

_tal _extensible short   IMMU_CLOSE_(
  short  Fnum                            /* Message File Number.        IN */
  )
{
  return 0 ;
}

