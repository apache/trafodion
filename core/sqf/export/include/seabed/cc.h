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

#ifndef __SB_CC_H_
#define __SB_CC_H_

typedef int _bcc_status;
typedef int bfat_16;
typedef int _xcc_status;
typedef int xfat_16;

#define _bstatus_lt(x) ((x) < 0)
#define _bstatus_gt(x) ((x) > 0)
#define _bstatus_eq(x) ((x) == 0)
#define _bstatus_le(x) ((x) <= 0)
#define _bstatus_ge(x) ((x) >= 0)
#define _bstatus_ne(x) ((x) != 0)

#define _xstatus_lt(x) ((x) < 0)
#define _xstatus_gt(x) ((x) > 0)
#define _xstatus_eq(x) ((x) == 0)
#define _xstatus_le(x) ((x) <= 0)
#define _xstatus_ge(x) ((x) >= 0)
#define _xstatus_ne(x) ((x) != 0)

#endif // !__SB_CC_H_
