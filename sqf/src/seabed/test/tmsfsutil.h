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

#ifndef __TMSFSUTIL_H_
#define __TMSFSUTIL_H_

typedef void (*Util_DH_Type)(const char *, const char *); // debug hook

extern void        msfs_util_event_send(int event_id, bool wait);
extern void        msfs_util_event_wait(int event_id);
extern void        msfs_util_event_wait2(int *event_id);
extern const char *msfs_util_get_sysmsg_str(int sysmsg);
extern int         msfs_util_init(int            *argc,
                                  char         ***argv,
                                  Util_DH_Type    dh);
extern int         msfs_util_init_attach(int            *argc,
                                         char         ***argv,
                                         Util_DH_Type    dh,
                                         bool            forkexec,
                                         char           *name);
extern int         msfs_util_init_fs(int            *argc,
                                     char         ***argv,
                                     Util_DH_Type    dh);
extern int         msfs_util_init_role(bool             client,
                                       int             *argc,
                                       char          ***argv,
                                       Util_DH_Type     dh);
extern int         msfs_util_init_role_fs(bool            client,
                                          int            *argc,
                                          char         ***argv,
                                          Util_DH_Type    dh);
extern int         msfs_util_wait_process_count(int  type,
                                                int  count,
                                                int *retcount,
                                                bool verbose);


#endif // !__TMSFSUTIL_H_
