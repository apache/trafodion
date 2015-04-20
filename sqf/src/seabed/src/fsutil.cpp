//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2015 Hewlett-Packard Development Company, L.P.
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

#include <stdio.h>
#include <stdlib.h>

#include "seabed/int/opts.h"

#include "seabed/fs.h"

#include "fsi.h"
#include "fsutil.h"
#include "trans.h"


//
// Purpose: i/o tag alloc
//
FS_Io_Type *FS_util_io_tag_alloc(FS_Fd_Type        *pp_fd,
                                 int                pv_io_type,
                                 SB_Tag_Type        pv_tag_user,
                                 SB_Transid_Type    pv_transid,
                                 SB_Transseq_Type   pv_startid,
                                 char              *pp_buffer) {
    FS_Io_Type *lp_io;
    int         lv_tag;

    if (pp_fd->ip_io_tag_mgr->size() <= pp_fd->iv_nowait_depth) {
        lv_tag = pp_fd->ip_io_tag_mgr->alloc();
        lp_io = &pp_fd->ip_io[lv_tag];
        lp_io->ip_fd = pp_fd;
        lp_io->iv_io_type = pv_io_type;
        lp_io->iv_msgid = 0;
        lp_io->iv_tag_user = pv_tag_user;
        TRANSID_COPY(lp_io->iv_transid, pv_transid);
        TRANSSEQ_COPY(lp_io->iv_startid, pv_startid);
        lp_io->ip_buffer = pp_buffer;

        // link in
        lp_io->ip_io_next = NULL;
        lp_io->ip_io_prev = pp_fd->ip_io_new;
        if (pp_fd->ip_io_new == NULL)
            pp_fd->ip_io_old = lp_io;
        else
            pp_fd->ip_io_new->ip_io_next = lp_io;
        pp_fd->ip_io_new = lp_io;

        lp_io->iv_inuse = true;
    } else
        lp_io = NULL;
    return lp_io;
}

//
// Purpose: i/o tag alloc for nowait-open
//
FS_Io_Type *FS_util_io_tag_alloc_nowait_open(FS_Fd_Type *pp_fd, int pv_msgid) {
    FS_Io_Type *lp_io;

    lp_io = &pp_fd->iv_nowait_open_io;
    lp_io->iv_inuse = true;
    lp_io->ip_fd = pp_fd;
    lp_io->iv_msgid = 0;
    lp_io->iv_msgid_open = pv_msgid;
    lp_io->iv_tag_user = -30;

    return lp_io;
}

//
// Purpose: i/o tag free
//
void FS_util_io_tag_free(FS_Fd_Type *pp_fd, FS_Io_Type *pp_io) {
    FS_Fd_Type *lp_fd;

    // unlink
    if (pp_io->ip_io_prev == NULL) {
        pp_fd->ip_io_old = pp_io->ip_io_next;
        if (pp_io->ip_io_next == NULL)
            pp_fd->ip_io_new = NULL;
        else
            pp_io->ip_io_next->ip_io_prev = NULL;
    } else {
        pp_io->ip_io_prev->ip_io_next = pp_io->ip_io_next;
        if (pp_io->ip_io_next == NULL)
            pp_fd->ip_io_new = pp_io->ip_io_prev;
        else
            pp_io->ip_io_next->ip_io_prev = pp_io->ip_io_prev;
    }

    lp_fd = static_cast<FS_Fd_Type *>(pp_io->ip_fd);
    pp_io->iv_inuse = false;
    lp_fd->ip_io_tag_mgr->free_slot(pp_io->iv_tag_io);
}

void FS_util_io_tag_free_nowait_open(FS_Fd_Type *pp_fd) {
    FS_Io_Type *lp_io;

    lp_io = &pp_fd->iv_nowait_open_io;
    lp_io->iv_inuse = false;
}

//
// Purpose: i/o tag get i/o by tag
//
FS_Io_Type *FS_util_io_tag_get_by_tag(FS_Fd_Type  *pp_fd,
                                      SB_Tag_Type  pv_tag_user) {
    int lv_tag;

    if (pv_tag_user == XOMITTAG) {
        return pp_fd->ip_io_old;
    } else {
        for (lv_tag = 0; lv_tag <= pp_fd->iv_nowait_depth; lv_tag++) {
            if (pp_fd->ip_io[lv_tag].iv_inuse &&
                (pp_fd->ip_io[lv_tag].iv_tag_user == pv_tag_user)) {
                return &pp_fd->ip_io[lv_tag];
            }
        }
    }
    return NULL;
}

//
// Purpose: readupdate tag alloc
//
int FS_util_ru_tag_alloc(FS_Fd_Type *pp_fd, bool pv_read) {
    FS_Ru_Type *lp_ru;
    int         lv_tag;

    if (pv_read || (pp_fd->ip_ru_tag_mgr->size() < pp_fd->iv_recv_depth)) {
        lv_tag = pp_fd->ip_ru_tag_mgr->alloc();
        lp_ru = &pp_fd->ip_ru[lv_tag];
        lp_ru->iv_inuse = true;
        lp_ru->ip_buffer = NULL;
        lp_ru->iv_msgid = -1;
        lp_ru->iv_read_count = 0;
        lp_ru->iv_count_written = 0;
        lp_ru->iv_io_type = -1;
        lp_ru->iv_read = false;
        lp_ru->iv_tag = -1;
        TRANSID_SET_NULL(lp_ru->iv_transid);

        // link in
        lp_ru->ip_ru_next = NULL;
        lp_ru->ip_ru_prev = pp_fd->ip_ru_new;
        if (pp_fd->ip_ru_new == NULL)
            pp_fd->ip_ru_old = lp_ru;
        else
            pp_fd->ip_ru_new->ip_ru_next = lp_ru;
        pp_fd->ip_ru_new = lp_ru;
    } else
        lv_tag = -1;
    return lv_tag;
}

//
// Purpose: readupdate tag free
//
void FS_util_ru_tag_free(FS_Fd_Type *pp_fd, int pv_tag) {
    FS_Ru_Type *lp_ru;

    lp_ru = &pp_fd->ip_ru[pv_tag];

    // unlink
    if (lp_ru->ip_ru_prev == NULL) {
        pp_fd->ip_ru_old = lp_ru->ip_ru_next;
        if (lp_ru->ip_ru_next == NULL)
            pp_fd->ip_ru_new = NULL;
        else
            lp_ru->ip_ru_next->ip_ru_prev = NULL;
    } else {
        lp_ru->ip_ru_prev->ip_ru_next = lp_ru->ip_ru_next;
        if (lp_ru->ip_ru_next == NULL)
            pp_fd->ip_ru_new = lp_ru->ip_ru_prev;
        else
            lp_ru->ip_ru_next->ip_ru_prev = lp_ru->ip_ru_prev;
    }

    lp_ru->iv_inuse = false;
    pp_fd->ip_ru_tag_mgr->free_slot(pv_tag);
}

//
// Purpose: readupdate tag get readupdate by tag
//
FS_Ru_Type *FS_util_ru_tag_get_by_tag(FS_Fd_Type  *pp_fd,
                                      SB_Tag_Type  pv_tag_user) {
    int lv_tag;

    if (pv_tag_user == XOMITTAG) {
        return pp_fd->ip_ru_old;
    } else {
        for (lv_tag = 0; lv_tag <= pp_fd->iv_recv_depth; lv_tag++) {
            if (pp_fd->ip_ru[lv_tag].iv_inuse &&
                (pp_fd->ip_ru[lv_tag].iv_tag == pv_tag_user)) {
                return &pp_fd->ip_ru[lv_tag];
            }
        }
    }
    return NULL;
}

