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
#ifndef NOWAITOP_H
#define NOWAITOP_H

/* -*-C++-*-
******************************************************************************
*
* File:         NoWaitOp.h
* Description:  Definition of NowaitOp class.
*               
* Created:      3/26/2002
* Language:     C++
*
*
*
******************************************************************************
*/

//#include "Statement.h"
//#include "Descriptor.h"

class Statement;
class Descriptor;
class NoWaitOp;

#ifdef EX_GOD_H    // compile the following only if ex_god.h also included
#ifdef CLI_STDH_H  // compile the following only if CliDefs.h also included

class NoWaitOp : public NABasicObject
{
  public:

    enum opType { FETCH, EXECUTE, PREPARE, FETCH_CLOSE };

    NoWaitOp(Statement * stmt, 
             Descriptor * inputDesc, Descriptor * outputDesc,
             Lng32 tag, opType op, NABoolean initiated);

    ~NoWaitOp(void);

    RETCODE awaitIox(Lng32 *tag);

    inline Statement * getStatement(void)
      { return stmt_; };
    inline Lng32 * getTagAddr(void)
      { return &tag_; };

    static inline Lng32 getTagSize()
      { return sizeof(Lng32); }

  private:

    Statement * stmt_;        // Statement object of no-wait op
    Descriptor * inputDesc_;  // input Descriptor for no-wait op
    Descriptor * outputDesc_; // output Descriptor for no-wait op
    Lng32 tag_;                // tag to be returned on operation completion
    opType op_;               // type of operation (e.g. Fetch, Execute, ...)
    NABoolean initiated_;     // true if operation has been started in
                              // the Executor

} ;


#endif // CLI_STDH_H
#endif // EX_GOD_H

#endif /* NOWAITOP_H */

