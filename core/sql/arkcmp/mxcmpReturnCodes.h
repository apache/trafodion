/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#ifndef __MXCMPRETURNCODES_H
#define __MXCMPRETURNCODES_H

/* -*-C++-*-
 *****************************************************************************
 * File:         mxcmpReturnCodes.h
 * Description:  This holds the mxcmp exit codes.
 * Created:      10/23/2003
 * Language:     C++
 *****************************************************************************
 */

// mxcmp & mxCompileUserModule need these constants so that
// they can both return the same exit codes to ETK.

enum mxcmpExitCode
{ SUCCEED = 0, FAIL = 1, FAIL_RETRYABLE = -1, ERROR = 2, WARNING = 3 };

#endif /* __MXCMPRETURNCODES_H */
