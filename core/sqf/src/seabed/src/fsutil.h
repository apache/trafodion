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

#ifndef __SB_FSUTIL_H_
#define __SB_FSUTIL_H_

#include "fsi.h"

extern FS_Io_Type *FS_util_io_tag_alloc(FS_Fd_Type       *pp_fd,
                                        int               pv_io_type,
                                        SB_Tag_Type       pv_tag_user,
                                        SB_Transid_Type   pv_transid,
                                        SB_Transseq_Type  pv_startid,
                                        char             *pp_buffer);
extern FS_Io_Type *FS_util_io_tag_alloc_nowait_open(FS_Fd_Type *pp_fd,
                                                    int         pv_msgid);
extern void        FS_util_io_tag_free(FS_Fd_Type *pp_fd,
                                       FS_Io_Type *pp_io);
extern void        FS_util_io_tag_free_nowait_open(FS_Fd_Type *pp_fd);
extern FS_Io_Type *FS_util_io_tag_get_by_tag(FS_Fd_Type  *pp_fd,
                                             SB_Tag_Type  pv_tag_user);
extern int         FS_util_ru_tag_alloc(FS_Fd_Type *pp_fd, bool pv_read);
extern void        FS_util_ru_tag_free(FS_Fd_Type *pp_fd, int pv_tag);
extern FS_Ru_Type *FS_util_ru_tag_get_by_tag(FS_Fd_Type  *pp_fd,
                                             SB_Tag_Type  pv_tag_user);

#endif // !__SB_FSUTIL_H_
