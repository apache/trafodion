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
#ifndef _NA_INTERNAL_ERROR_H_
#define _NA_INTERNAL_ERROR_H_
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         NAInternalError.h
 * Description:  Encapsulate the exception call back handler
 *               used in NAAssert and NAAbort functions
 *               
 * Created:      2/9/2003
 * Language:     C++
 *
 *****************************************************************************
 */
class ExceptionCallBack;

class NAInternalError {
private:
  static ExceptionCallBack *pExceptionCallBack_;
public:
  static ExceptionCallBack *getExceptionCallBack();
  static void registerExceptionCallBack(ExceptionCallBack *p);
  static void unRegisterExceptionCallBack();
  static void throwFatalException(const char *msg,
				  const char *fileName,
				  UInt32 lineNum);
  static void throwAssertException(const char *cond,
				   const char *fileName,
				   UInt32 lineNum);
};

#endif
