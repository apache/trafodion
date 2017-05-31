///////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////

#ifndef OPEN_H_
#define OPEN_H_

class CProcess;

class COpen
{
 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    int  Nid;                    // node id of opened process
    int  Pid;                    // process id of opened process
    Verifier_t Verifier;         // target process's verifier
    char Name[MAX_PROCESS_NAME]; // name of opened process
    
    COpen( CProcess *process );
    ~COpen( void );

    void DeLink( COpen **head, COpen **tail );
    COpen *GetNext( void );
    COpen *Link( COpen *entry );

protected:
private:
    COpen *Next;
    COpen *Prev;
};

#endif /*OPEN_H_*/
