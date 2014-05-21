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

#include "sp_process.h"
#include "sp_registry.h"
#include "sp_common.h"
#include "common/sp_errors.h"
#include "seabed/fserr.h"

#define TPT_DECL(name)       SB_Phandle_Type name
#define TPT_REF(name)        (&name)

// -------------------------------------------------------------------
// 
// SP_Process_List::SP_Process_List
// Constructor
//
// -------------------------------------------------------------------

SP_Process_List::SP_Process_List()
{
    iv_num_processes = 0;
    iv_num_backup_processes = 0;
    ip_list = NULL;
    iv_my_nid = -1;
}

// -------------------------------------------------------------------
// 
// SP_Process_List::~SP_Process_List
// Desstructor
//
// -------------------------------------------------------------------

SP_Process_List::~SP_Process_List()
{
    if (ip_list)
        delete []ip_list;
    ip_list = NULL;
}

// -------------------------------------------------------------------
// 
// SP_Process_List::populate_process_list
// Purpose : get names of processes we are responsible for
//
// -------------------------------------------------------------------

int SP_Process_List::populate_process_list(std::string &pp_list, std::string &pp_ip, int pv_nid)
{
    std::string lv_copyValue;
    std::string lv_copyValue2;
    std::string lv_delim = " ";
    int         lv_error;
    size_t      lv_pos;
    std::string lv_temp;

    SPTrace (2, ("SP_Process_List::populate_process_list ENTRY (nid %d)\n", pv_nid));

    iv_my_nid = pv_nid;
    lv_copyValue = pp_list;
    lv_pos = lv_copyValue.find_first_of(")");
    if (lv_pos == std::string::npos)
    {
       SPTrace (1, ("SP_Process_List::populate_process_list EXIT, bad string value\n"));
       return 1;
    }
    lv_copyValue[lv_pos] = ' ';
 
    lv_pos = lv_copyValue.find_first_of ("$");
    while (lv_pos != std::string::npos)
    {
       iv_num_processes++;
       lv_pos = lv_copyValue.find_first_of ("$", lv_pos+1);
    }

    SPTrace (3, ("SP_Process_List::populate_process_list number of child processes (%d)\n",
                 iv_num_processes));

    
    if (iv_num_processes == 0)
    {
       SPTrace (2, ("SP_Process_List::populate_process_list EXIT with error %d\n",SP_SUCCESS ));
       return SP_SUCCESS;
    }

    ip_list = new SP_Process[iv_num_processes];    
 
    for (int lv_inx = 0; lv_inx < iv_num_processes; lv_inx++)
    {
         lv_error = sp_get_process_name(lv_copyValue, lv_temp, lv_delim);
         if (lv_error)
         {
            SPTrace (1, ("SP_Process_List::populate_process_list EXIT (get_process_name) with error %d\n",
                    lv_error ));
            return lv_error;
         }
         ip_list[lv_inx].set_info(lv_temp, pp_ip, iv_my_nid);
    }
   
   
    SPTrace (2, ("SP_Process_List::populate_process_list EXIT with error %d\n",SP_SUCCESS ));
    return SP_SUCCESS;
}


// -------------------------------------------------------------------
// 
// SP_Process_List::populate_processes
// Purpose : call populate for each process configured
//
// -------------------------------------------------------------------
int SP_Process_List::populate_processes()
{
   int         lv_error = SP_SUCCESS;
   int         lv_inx = 0;
   std::string lv_name;

   SPTrace (2, ("SP_Process_List::populate_processes ENTRY\n"));

   // for each process that is configured we are responsible for,
   // populate the configuration information
   for (lv_inx = 0; lv_inx < iv_num_processes; lv_inx++)
   {
         lv_error = ip_list[lv_inx].populate();
         if (lv_error)
             break;
         if (ip_list[lv_inx].is_backup())
             ++iv_num_backup_processes;
   }
  
   SPTrace (2, ("SP_Process_List::populate_processes EXIT with error %d\n", lv_error));
   return lv_error;
}

bool SP_Process_List::kill_processes(int pv_proxy_nid)
{
   int lv_error;
   int lv_success = true;

   for (int lv_inx = 0; lv_inx < iv_num_processes; lv_inx++)
   {
       // Use the stop_process() method to terminate the process.
       // That's what the method is for. Don't call msg_mon_stop_process
       // because it is broken. This solves CR 6911.
       lv_error = ip_list[lv_inx].stop_process(pv_proxy_nid);
       SPTrace (1, ("SP_Process_List::kill_processes Stopping process %s resulted in error %d\n",
                    ip_list[lv_inx].get_name(), lv_error));
       if (lv_error)
       {
           if (lv_error != XZFIL_ERR_NOSUCHDEV)
           {
               lv_success = false;   
               LOG_AND_TRACE(1, ("\n Error stopping process %s with error %d\n",
                                 ip_list[lv_inx].get_name(),
                                 lv_error));
           } // XZFIL_ERR_NOSUCHDEV
       } // if (lv_error)
   } // endfor
   return lv_success;
}
