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

#ifndef __SB_BUF_H_
#define __SB_BUF_H_

#include <stdio.h> // BUFSIZ

enum { MAX_BUF_LINE  = 512 };
enum { MAX_BUF_WHERE = 100 };

typedef char SB_Buf_Line[MAX_BUF_LINE];
typedef char SB_Buf_Where[MAX_BUF_WHERE];

class SB_Buf_Lline {
public:
    // constructor - allocate
    inline SB_Buf_Lline() {}
    // destructor - deallocate
    inline ~SB_Buf_Lline() {}

    // return size
    inline int size() { return static_cast<int>(sizeof(ia_v)); }

    // pointer ref - return ia_v *
    inline char *operator->() { return ia_v; }

    // pointer ref - return ia_v *
    inline char *operator&() { return ia_v; }

    // hold ia_v
    char ia_v[BUFSIZ];
};

#endif // !__SB_BUF_H_
