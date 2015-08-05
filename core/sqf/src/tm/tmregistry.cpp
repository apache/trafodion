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

#include <string.h>
#include "tmregistry.h"
#include "seabed/thread.h"

int32 tm_reg_get (MS_Mon_ConfigType   pv_type,
                  char                *pp_group,
                  char                *pp_key,
                  char                *pp_value)

{

    int32                lv_err = 0;
    MS_Mon_Reg_Get_Type  lv_info;

    lv_err = msg_mon_reg_get(pv_type,   // type
                           false,       // next
                           pp_group,    // group
                           pp_key,      // key
                           &lv_info);   // info

    if ((lv_err == 0) && (lv_info.num_returned == 1))
    {
        strcpy (pp_value, lv_info.list[0].value);
        return 0;
    }

    // will halt later TODO
    return -1;

}

int32 tm_reg_set (MS_Mon_ConfigType   lv_type,
                  char                *pp_group,
                  char                *pp_key,
                  char                *pp_value)
{

    int32 lv_err = 0;
    int32 lv_iter = 0;
    
    do{
        lv_err = msg_mon_reg_set(lv_type,    // type
                                 pp_group,   // group
                                 pp_key,     // key
                                 pp_value);  
        if(lv_err == 0)
            return lv_err;
        
        SB_Thread::Sthr::sleep(1000);
        lv_iter ++;
    } while((lv_err != 0) && (lv_iter < 10));

    return lv_err; 
}





