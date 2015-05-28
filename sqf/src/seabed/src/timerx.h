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

#ifndef __SB_TIMERX_H_
#define __SB_TIMERX_H_

#include "seabed/ms.h" // BMS_SRE_TPOP

extern void  sb_timer_init();
extern bool  sb_timer_comp_q_empty();
extern void *sb_timer_comp_q_remove();
extern int   sb_timer_hi();
extern void  sb_timer_set_sre_tpop(BMS_SRE_TPOP *pp_sre, void *pp_tle);
extern int   sb_timer_size();
extern void  sb_timer_shutdown();

#endif // !__SB_TIMERX_H_
