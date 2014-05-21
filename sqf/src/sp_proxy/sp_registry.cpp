// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
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

#include <string.h>
#include "sp_common.h"
#include "sp_registry.h"

int sp_reg_get (MS_Mon_ConfigType   pv_type,
                  char                *pp_group,
                  char                *pp_key,
                  std::string         &pp_value)

{

    int                  lv_err = 0;
    MS_Mon_Reg_Get_Type  lv_info;

    lv_err = msg_mon_reg_get(pv_type,   // type
                           false,       // next
                           pp_group,    // group
                           pp_key,      // key
                           &lv_info);   // info

    // It is safe to reference lv_info.num_returned because the call worked.
    if ((lv_err == 0) && (lv_info.num_returned == 1))
    {
        pp_value = lv_info.list[0].value;
        return 0;
    }
    else
    {
        if (lv_err)
        {
            LOG_AND_TRACE(1,("sp_reg_get: msg_mon_reg_get %s::%s failed with %d\n", 
                             pp_group, pp_key, lv_err));
        }
        else
        {
	    // It is safe to reference lv_info.num_returned because the call worked.
	    // We got more than one item, which is more than expected.
            LOG_AND_TRACE(1,("sp_reg_get: %s::%s returned %d items, status %d\n",
                             pp_group, pp_key, lv_info.num_returned, lv_err));
            for (int i=0; i<lv_info.num_returned; i++)
                LOG_AND_TRACE(2,("sp_reg_get:     item %d: <<%s>>\n", i, lv_info.list[i].value));
        }
    }
    return -1;

}

int sp_reg_set (MS_Mon_ConfigType const  lv_type,
                char const * const       pp_group,
                char const * const       pp_key,
                char const * const       pp_value)

{
    int lv_err;
    
    lv_err =  msg_mon_reg_set(static_cast<MS_Mon_ConfigType>(lv_type),   // type
                         const_cast<char *>(pp_group),             // group
                          const_cast<char *>(pp_key),               // key
                          const_cast<char *>(pp_value));
    
    if (lv_err)
        LOG_AND_TRACE(1,("sp_reg_set: msg_mon_reg_get failed with %d\n", lv_err));

    return lv_err;
}





