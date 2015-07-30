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
#ifndef EXECUTE_ID_TRIGGERS_H
#define EXECUTE_ID_TRIGGERS_H
//
// -- struct ExecuteId
//
// This structure uniquely identifies a specific trigger execution instance at
// run time. It is created by the executor root, and is recorded in the trigger
// temporary table.

typedef struct
{
        Lng32 cpuNum;                    // cpu number in the cluster
        Lng32 pid;                               // process id
        void *rootTcbAddress;   // root tcb address
} ExecuteId;

// This constant is used by the Catman when it creates the temp table
const Int32       SIZEOF_UNIQUE_EXECUTE_ID        = sizeof(ExecuteId);
#endif // EXECUTE_ID_TRIGGERS_H
