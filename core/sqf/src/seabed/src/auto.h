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
// Implement auto vars
//

#ifndef __SB_AUTO_H_
#define __SB_AUTO_H_

#include <errno.h>

#include "monclio.h"

#include "monmsgtype.h"
#include "mserr.h"
#include "utracex.h"

typedef SharedMsgDef Mon_Shared_Msg_Type;

// Could use templates,
// but for now keep simple

class Mon_Msg_Auto {
public:
    // constructor - allocate Mon_Msg_Type
    inline Mon_Msg_Auto() {
        alloc_msg();
    }

    inline Mon_Msg_Auto(Mon_Msg_Type *pp_msg) {
        iv_err = 0;
        iv_rel = true;
        ip_v = pp_msg;
    }

    // destructor - deallocate Mon_Msg_Type
    inline ~Mon_Msg_Auto() {
        if (ip_v != NULL) {
            if (iv_rel) {
                if (gp_local_mon_io->release_msg(ip_v)) {
                    iv_err = ms_err_errno_to_mpierr("Mon_Msg_Auto::~Mon_Msg_Auto-release");
                    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_LOCIO_RELEASE_MSG,
                                       errno,
                                       iv_err);
                }
            } else
                delete ip_v;
        }
    }

    // pointer ref - return Mon_Msg_Type *
    inline Mon_Msg_Type *operator->() { return ip_v; }

    // pointer ref - return Mon_Msg_Type *
    inline Mon_Msg_Type *operator&() { return ip_v; }

    inline void alloc_msg() {
        iv_err = 0;
        if (gp_local_mon_io != NULL) {
            if (gp_local_mon_io->acquire_msg(&ip_v)) {
                iv_err = ms_err_errno_to_mpierr("Mon_Msg_Auto::alloc_msg-acquire_msg");
                SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_LOCIO_ACQUIRE_MSG,
                                   errno,
                                   iv_err);
                iv_rel = false;
                ip_v = new Mon_Msg_Type;
            } else
                iv_rel = true;
        } else {
            iv_rel = false;
            ip_v = new Mon_Msg_Type;
        }
    }

    inline void del_msg() {
        if (ip_v != NULL) {
            if (iv_rel) {
                if (gp_local_mon_io->release_msg(ip_v)) {
                    iv_err = ms_err_errno_to_mpierr("Mon_Msg_Auto::del_msg-release_msg");
                    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_LOCIO_RELEASE_MSG,
                                       errno,
                                       iv_err);
                }
            } else
                delete ip_v;
            ip_v = NULL;
        }
    }

    inline void forget_msg() {
        ip_v = NULL;
    }

    inline int get_error() {
        return iv_err;
    }

    inline void set_msg(Mon_Msg_Type *pp_msg) {
        ip_v = pp_msg;
    }

private:
    // hold Mon_Msg_Type
    Mon_Msg_Type *ip_v;
    int           iv_err; // hold error
    bool          iv_rel; // release?
};

class Mon_Shared_Msg_Auto {
public:
    // constructor - allocate Mon_Shared_Msg_Type
    inline Mon_Shared_Msg_Auto() {
        alloc_msg();
    }

    inline Mon_Shared_Msg_Auto(Mon_Shared_Msg_Type *pp_msg) {
        iv_err = 0;
        iv_rel = true;
        ip_v = pp_msg;
    }

    // destructor - deallocate Mon_Shared_Msg_Type
    inline ~Mon_Shared_Msg_Auto() {
        if (ip_v != NULL) {
            if (iv_rel) {
                if (gp_local_mon_io->release_msg(&ip_v->msg)) {
                    iv_err = ms_err_errno_to_mpierr("Mon_Msg_Auto::~Mon_Msg_Auto-release");
                    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_LOCIO_RELEASE_MSG,
                                       errno,
                                       iv_err);
                }
            } else
                delete ip_v;
        }
    }

    // pointer ref - return Mon_Msg_Type *
    inline Mon_Shared_Msg_Type *operator->() { return ip_v; }

    // pointer ref - return Mon_Msg_Type *
    inline Mon_Shared_Msg_Type *operator&() { return ip_v; }

    inline void alloc_msg() {
        Mon_Msg_Type *lp_msg;

        iv_err = 0;
        if (gp_local_mon_io != NULL) {
            if (gp_local_mon_io->acquire_msg(&lp_msg)) {
                iv_rel = false;
                ip_v = new Mon_Shared_Msg_Type;
                iv_err = ms_err_errno_to_mpierr("Mon_Msg_Auto::alloc_msg-acquire_msg");
                SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_LOCIO_ACQUIRE_MSG,
                                   errno,
                                   iv_err);
            } else {
                set_msg(lp_msg);
                iv_rel = true;
            }
        } else {
            iv_rel = false;
            ip_v = new Mon_Shared_Msg_Type;
        }
    }

    inline void del_msg() {
        if (ip_v != NULL) {
            if (iv_rel) {
                if (gp_local_mon_io->release_msg(&ip_v->msg)) {
                    iv_err = ms_err_errno_to_mpierr("Mon_Msg_Auto::del_msg-release_msg");
                    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_LOCIO_RELEASE_MSG,
                                       errno,
                                       iv_err);
                }
            } else
                delete ip_v;
            ip_v = NULL;
        }
    }

    inline void forget_msg() {
        ip_v = NULL;
    }

    inline int get_error() {
        return iv_err;
    }

    inline void set_msg(Mon_Msg_Type *pp_msg) {
        ip_v = reinterpret_cast<Mon_Shared_Msg_Type *>(pp_msg);
    }

private:
    // hold Mon_Shared_Msg_Type
    Mon_Shared_Msg_Type *ip_v;
    int                  iv_err; // hold error
    bool                 iv_rel; // release?
};

#endif // !__SB_AUTO_H_
