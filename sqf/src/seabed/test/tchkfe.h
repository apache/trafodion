//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef __TCHKFE_H_
#define __TCHKFE_H_

//
// cc checking
//
#define TEST_CHK_BCCEQ(bcc) assert(_bstatus_eq(bcc))
#define TEST_CHK_BCCNE(bcc) assert(_bstatus_ne(bcc))
#define TEST_CHK_BCCIGNORE(bcc) bcc = bcc
#define TEST_CHK_CCEQ(cc) assert(_xstatus_eq(cc))
#define TEST_CHK_CCNE(cc) assert(_xstatus_ne(cc))
#define TEST_CHK_CCIGNORE(cc) cc = cc

//
// ferr checking
//
#define TEST_CHK_FEOK(ferr) assert(ferr == XZFIL_ERR_OK)
#define TEST_CHK_FEIGNORE(ferr) ferr = ferr

//
// wait checking
//
#define TEST_CHK_WAITIGNORE(status) status = status

#endif // !__TCHKFE_H_
