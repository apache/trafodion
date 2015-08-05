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

//
// Implement map for md
//

#ifndef __SB_MAPMD_H_
#define __SB_MAPMD_H_

#include "imap.h"

#include "queue.h"

#include "msi.h"  // needs queue.h

//
// MD-map
//
class SB_Md_Map : public SB_Imap {
public:
    SB_Md_Map(int pv_qid, const char *pp_name);
    virtual ~SB_Md_Map();

    bool                  empty();
    virtual void         *get(int pv_id);
    virtual SB_Imap_Enum *keys();
    virtual void          printself(bool pv_traverse);
    virtual void          put(MS_Md_Type *pp_md);
    virtual void         *remove(int pv_id);
    virtual void          removeall();
    int                   size();

private:
    char ia_md_map_name[40];
    int  iv_qid;
};

//
// Thread-safe MD-map
//
class SB_Ts_Md_Map : public SB_Ts_Imap {
private:
    SB_Ecid_Type iv_aecid_md_map; // should be first instance

public:
    SB_Ts_Md_Map(int pv_qid, const char *pp_name);
    virtual ~SB_Ts_Md_Map();

    bool                  empty();
    virtual void         *get(int pv_id);
    virtual SB_Imap_Enum *keys();
    virtual void          printself(bool pv_traverse);
    virtual void          put(MS_Md_Type *pp_md);
    virtual void         *remove(int pv_id);
    virtual void         *remove_lock(int pv_id, bool pv_lock);
    virtual void          removeall();
    int                   size();

private:
    char ia_md_map_name[40];
    int  iv_qid;
};

#ifdef USE_SB_INLINE
#include "mapmd.inl"
#endif

#endif // !__SB_MAPMD_H_
