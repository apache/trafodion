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

#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include "sp_process.h"
#include "sp_registry.h"
#include "sp_common.h"
#include "common/sp_errors.h"
#include "seabed/fserr.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/exception/error_info.hpp>
#include <boost/exception/diagnostic_information.hpp> 

#define TPT_DECL(name)       SB_Phandle_Type name
#define TPT_REF(name)        (&name)

const int MAX_PROCESS_TYPE_LENGTH = 4;

extern const char *ms_getenv_str(const char *pp_key);

// ------------------------------------------------------------------------------
//
// sp_convert_to_yes_or_no
// Purpose : convert a character to a word representation
//
// ------------------------------------------------------------------------------
int sp_convert_to_yes_or_no (std::string &pr_value)
{
    if (pr_value == "Y")
       return SP_YES;
    return SP_NO;
}

// ------------------------------------------------------------------------------
//
// sp_replace_node_num
// Purpose : replace the logical node number in a process.
//           Example turn $ECB0 into $ECB1
//
// -------------------------------------------------------------------------------
std::string sp_replace_node_num (int pv_old_nid, int pv_new_nid, std::string &pv_str)
{	
	try
	{
		std::string result;
		result = boost::replace_all_copy(pv_str, boost::lexical_cast<std::string>(pv_old_nid), boost::lexical_cast<std::string>(pv_new_nid));
		return result;
	}
	catch(boost::exception& e)
	{
		LOG_AND_TRACE(1,("sp_replace_node_num: replace node error: %s", boost::diagnostic_information(e).c_str()));
	}
	catch(...)
	{	
		LOG_AND_TRACE(1,("sp_replace_node_num: replace node error."));
	}
	return "";
}

// ------------------------------------------------------------------------------
//
// sp_return_type
// Purpose : return the type
//
// ------------------------------------------------------------------------------
int sp_return_type(std::string &pr_value)
{
    if (pr_value == "BROKER")
        return SP_BROKER;
    else if (pr_value == "METRICS")
        return SP_METRICS;
    else if (pr_value == "TPA")
        return SP_TPA;
    else if (pr_value == "TPA_SCRIPT")
        return SP_TPA_SCRIPTS;
    else if (pr_value == "NSA")
        return SP_NSA;
    else if (pr_value == "PM")
        return SP_PM;
    else if (pr_value == "TP")
        return SP_TP;
    else if (pr_value == "UAA")
        return SP_UAA;
    else if (pr_value == "UNA")
        return SP_UNA;
    else if (pr_value == "SNMP")
        return SP_SNMP;
    else if (pr_value == "PTPA")
        return SP_PTPA;
    else if (pr_value == "HARNESS")
        return SP_HARNESS;
    else if (pr_value == "LCSH")
        return SP_LCSH;
    else if (pr_value =="EBCM")
        return SP_EBCM;
    else if (pr_value =="SMAD")
        return SP_SMAD;
    else if (pr_value =="GENERIC_EXE")
        return SP_GENERIC_EXE;
    else if (pr_value =="GENERIC_SCRIPT")
        return SP_GENERIC_SCRIPT;

    return SP_UNC;
}

//--------------------------------------------------------------------
//
// SP_Process::SP_Process
// Constructor
//
// -------------------------------------------------------------------
SP_Process::SP_Process()
{
   iv_started = false;
   iv_nid = iv_pid = -1;
   iv_retries = 0;

   const char *lp_temp = ms_getenv_str ("MY_SQROOT");
   if (lp_temp)
       strcpy(iv_sq_root, lp_temp);
   else
   {
      char la_buf[SP_MAX_ERROR_TEXT];
      sprintf(la_buf, "\nProxy exiting : could not find $MY_SQROOT\n");
      LOG_AND_TRACE(1,("%s", la_buf));
      exit(1);
   }

    //name is initialized to empty string, node is not initialized
    for (int lv_inx = 0; lv_inx < SP_NUM_DEST_BROKERS; lv_inx++)
        process_params.sp_params.destbrokers[lv_inx].node = -1;

    process_params.sp_params.sourcebroker.node = -1;
}

//--------------------------------------------------------------------
//
// SP_Process::~SP_Process
// Destructor
//
// -------------------------------------------------------------------
SP_Process::~SP_Process(){}

// ------------------------------------------------------------------
//
// SP_Process::route_helper
// Purpose : take care of destination broker routing.  Will always do
//           local routing and sometimes do remote routing
//
// ------------------------------------------------------------------
int SP_Process::route_helper(int pv_nid, std::string &pv_myIp, int pv_remote_route)
{
    int lv_error = SP_SUCCESS;

    SPTrace (2, ("SP_Process::route_helper ENTRY.\n"));

    // Setup routing between this broker and its destination brokers
    for (int lv_inx_j = 0; lv_inx_j < SP_NUM_DEST_BROKERS; lv_inx_j++)
    {
        if (process_params.sp_params.destbrokers[lv_inx_j].name.empty())
          continue;

       // local dest broker
       if (pv_nid == process_params.sp_params.destbrokers[lv_inx_j].node)
       {
            lv_error = sp_route_broker(pv_myIp,
                                   connection_info.port,
                                   process_params.sp_params.subtype, pv_myIp,
                                   process_params.sp_params.destbrokers[lv_inx_j].port,
                                   process_params.sp_params.destbrokers[lv_inx_j].subtype );
        }
        // the broker is not on our node. we might need to reroute remotely,
        // depending on the pv_remote_route variable
        else if (pv_remote_route)
        {
           // if a remote dest broker is down, find it's backup and update information
            if(!gv_nodes.is_up(process_params.sp_params.destbrokers[lv_inx_j].node))
            {
               LOG_AND_TRACE (3, ("SP_Process::route_helper dest broker node down\n"));
               lv_error = set_backup_broker(lv_inx_j);
               if (lv_error)
                   break;

             }
             // get most up to date ip, regardless of a current failure, we never know
             // if physical hardware has moved around.  always get the most current ip
             // when routing off node
             std::string lv_ip;
             lv_error = sp_get_ip_from_registry
                         (process_params.sp_params.destbrokers[lv_inx_j].node, lv_ip);
             if (lv_error)
                break;

             lv_error = sp_route_broker(pv_myIp,
                                connection_info.port,
                                process_params.sp_params.subtype, lv_ip,
                                process_params.sp_params.destbrokers[lv_inx_j].port,
                                process_params.sp_params.destbrokers[lv_inx_j].subtype );

              // this dest broker could be restarting as well.  Allow an error and allow the route
              // to occur on demand.  The only way we'll get down here is if a node went
              // or proxies are falling over.  Routing happens on demand for all remote routes.
              // we do our best to do it proactively, but it we can't, wait for the
              // PROCESS_UP from the broker
             if (lv_error)
             {
                   SPTrace (3, ("SP_Process::route_helper routing skipped due to error "
                                " %d for %s\n", lv_error,
                                process_params.sp_params.destbrokers[lv_inx_j].name.c_str()));
                   lv_error = SP_SUCCESS;
              }
           }
           else
           {
                           SPTrace (3, ("SP_Process::route_helper routing skipped, broker obj %s.\n",
                                           process_params.obj.c_str()));
           }
    }
    SPTrace (2, ("SP_Process::route_helper EXIT with error %d.\n", lv_error));
    return lv_error;
}
//--------------------------------------------------------------------
//
// SP_Process::reroute_remote_node
// Purpose - after a death, recreate the route to a broker on a
//           remote node
//
// -------------------------------------------------------------------
int SP_Process::reroute_remote_node(std::string &pp_processName)
{
    int lv_error = SP_SUCCESS;

    SPTrace (2, ("SP_Process::reroute_remote_node ENTRY looking for routes from %s to %s.\n",
                iv_name.c_str(), pp_processName.c_str()));

    for (int lv_inx = 0; lv_inx < SP_NUM_DEST_BROKERS; lv_inx++)
    {
         if (process_params.sp_params.destbrokers[lv_inx].name.empty())
           continue;

         // if one of our brokers had a route to this process, route to it
         if (process_params.sp_params.destbrokers[lv_inx].name == pp_processName)
         {
                  LOG_AND_TRACE(3, ("SP_Process::reroute_remote_node match! source %s dest %s\n",
                              process_params.sp_params.destbrokers[lv_inx].name.c_str(), iv_name.c_str()));

              // get most up to date ip
              std::string lv_ip;
              lv_error = sp_get_ip_from_registry (process_params.sp_params.destbrokers[lv_inx].node,
                                                  lv_ip);
              if (lv_error)
                 break;

              std::string lv_local_ip;
              lv_error = sp_get_ip_from_registry (iv_nid, lv_local_ip);
              if (lv_error)
                 break;

              lv_error = sp_route_broker (lv_local_ip, connection_info.port,
                     process_params.sp_params.subtype, lv_ip,
                     process_params.sp_params.destbrokers[lv_inx].port,
                     process_params.sp_params.destbrokers[lv_inx].subtype );
              if (lv_error)
                 break;
         }
    }

    SPTrace (2, ("SP_Process::reroute_remote_node EXIT with error %d.\n", lv_error));
    return lv_error;
}

//--------------------------------------------------------------------
//
// SP_Process::set_source_broker
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_source_broker()
{
      std::string lp_delim = " ";
      int         lv_error = SP_SUCCESS;
    //  std::string lv_name = process_params.sp_params.sourcebroker.name;
      std::string lp_value;

      process_params.sp_params.sourcebroker.name = process_params.sp_params.sourcebroker.primary_name;
      // now get NODE, IP and PORT
   //   lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
   //          (char *) lv_name.c_str(), (char *) CONNECTIONINFO, lp_value);

      // now get NODE, IP and PORT
      lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
             (char *) process_params.sp_params.sourcebroker.name.c_str(), (char *) CONNECTIONINFO, lp_value);

      if (lv_error)
      {
          LOG_AND_TRACE (1, ("SP_Process::set_source_broker EXIT with error %d (CONNECTIONINFO) for primary_name (%s)\n",
                        lv_error,  process_params.sp_params.sourcebroker.name.c_str()));
          return lv_error;
      }

      std::string lv_copyValue2 = lp_value;
      int lv_node;

      if (!sp_find_and_set_key_int (lv_copyValue2, "NODE", lp_delim,
                                     lv_node))
      {
          LOG_AND_TRACE (1, ("SP_Process::set_source_broker EXIT with error %d (NODE)\n", SP_NOT_FOUND));
          return SP_NOT_FOUND;
      }

      process_params.sp_params.sourcebroker.node = lv_node;
      process_params.sp_params.sourcebroker.primary_node = lv_node;

      // remote source broker
      if (lv_node != iv_nid)
      {
          int lv_backup = -1;
          SPTrace (3, ("SP_Process::set_source_broker lv_node(%d) iv_nid(%d), name (%s).\n",
                       lv_node, iv_nid, get_name()));

          // check if node up
          if (gv_nodes.is_up(lv_node))
          {
              std::string lv_ip;
              lv_error = sp_get_ip_from_registry (lv_node, lv_ip);
              if (!lv_error)
              {
                 process_params.sp_params.sourcebroker.ip = lv_ip;
              }
             else
             {
                 LOG_AND_TRACE (1, ("SP_Process::set_source_broker EXIT with error %d\n", SP_NOT_FOUND));
                 return SP_NOT_FOUND;
              }
          }
          else
          {
             SPTrace (3, ("SP_Process::set_source_broker retrieving backup for %s.\n",
                            process_params.sp_params.sourcebroker.name.c_str()));
             std::string lv_value;
             // get backup node,
             lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
                         (char *) process_params.sp_params.sourcebroker.name.c_str(),
                         (char *) PROCESSINFO, lv_value);

             if (lv_error)
             {
                  LOG_AND_TRACE (1, ("SP_Process::set_source_broker EXIT retrieving PROCESSINFO "
                           "for backup for %s on node %d failed with error %d.\n",
                            process_params.sp_params.sourcebroker.name.c_str(),
                            process_params.sp_params.sourcebroker.node, lv_error));
                  return lv_error;
             }
             if (!sp_find_and_set_key_int (lv_value, "BACKUP", lp_delim, lv_backup))
             {
                  LOG_AND_TRACE (1, ("SP_Process::set_source_broker EXIT retrieving BACKUP "
                               "for backup for %s on node %d failed with error %d.\n",
                               process_params.sp_params.sourcebroker.name.c_str(),
                               process_params.sp_params.sourcebroker.node, SP_BACKUP_ERROR));
                   return SP_BACKUP_ERROR;
             }
             // if the backup node is down, we cannot survive a double failure
             if (!gv_nodes.is_up(lv_backup))
             {
                  LOG_AND_TRACE (1, ("SP_Process::set_source_broker EXIT double failure for %s on node %d.\n",
                               process_params.sp_params.sourcebroker.name.c_str(),
                               process_params.sp_params.sourcebroker.node));
                  return SP_BACKUP_ERROR;
              }

              // now get the new IP associated with the backup node
              std::string lv_ip;
              lv_error = sp_get_ip_from_registry (lv_backup, lv_ip);
              if (!lv_error)
              {
                  process_params.sp_params.sourcebroker.ip = lv_ip;
                  // change process name to reflect new node number - REWORK for REINTEGRATION
                  std::string lv_temp = sp_replace_node_num (lv_node, lv_backup,
                                        process_params.sp_params.sourcebroker.name);
                  if (lv_temp.empty())
                  {
                      LOG_AND_TRACE (1, ("SP_Process::set_source_broker EXIT unable to replace backup name\n"));
                      return SP_BACKUP_ERROR;
                  }
                  process_params.sp_params.sourcebroker.name = lv_temp;
                  process_params.sp_params.sourcebroker.node = lv_backup;
              }
              else
              {
                  LOG_AND_TRACE (1, ("SP_Process::set_source_broker EXIT with error %d(IP 1, lv_node %d)\n",
                               SP_NOT_FOUND, lv_node));
                  return SP_NOT_FOUND;
              }
          }
      }
      else if (!sp_find_and_set_key_str (lv_copyValue2, "IP", lp_delim,
                                process_params.sp_params.sourcebroker.ip))
      {
           LOG_AND_TRACE (1, ("SP_Process::set_source_broker EXIT with error %d(IP 2)\n", SP_NOT_FOUND));
           return SP_NOT_FOUND;
      }

      if (!sp_find_and_set_key_int (lv_copyValue2, "PORT", lp_delim,
                                     process_params.sp_params.sourcebroker.port))
      {
          LOG_AND_TRACE (1, ("SP_Process::set_source_broker EXIT with error %d (PORT)\n", SP_NOT_FOUND));
          return SP_NOT_FOUND;
      }
      return SP_SUCCESS;
}

// -----------------------------------------------------------------
//
// SP_Process::set_backup_broker
// Purpose : If a broker is down, this is a helper method to setting
//           the information for the backup
//
// ------------------------------------------------------------------
int SP_Process::set_backup_broker(int pv_inx)
{
     int         lv_backup = -1;
     int         lv_error = SP_SUCCESS;
     std::string lv_temp;
     std::string lv_value;
     std::string lp_delim = " ";

     SPTrace (3, ("SP_Process::set_backup_broker retrieving backup for %s on node %d.\n",
                         process_params.sp_params.destbrokers[pv_inx].name.c_str(),
                         process_params.sp_params.destbrokers[pv_inx].node));
     // get backup node,
     lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
                         (char *) process_params.sp_params.destbrokers[pv_inx].name.c_str(),
                         (char *) PROCESSINFO, lv_value);
     if (lv_error)
     {
          LOG_AND_TRACE (1, ("SP_Process::set_backup_broker EXIT retrieving backup "
                       "for %s on node %d failed with error %d.\n",
                       process_params.sp_params.destbrokers[pv_inx].name.c_str(),
                       process_params.sp_params.destbrokers[pv_inx].node, lv_error));
          return lv_error;
      }

      // if this dest broker doesn't have a backup, then it better be a node broker or
      // (for now) an EFB.  If it is - then just change it to go to the local node.
      if (!sp_find_and_set_key_int (lv_value, "BACKUP", lp_delim, lv_backup))
      {
            lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
                       (char *) process_params.sp_params.destbrokers[pv_inx].name.c_str(),
                       (char *) PROCESSPARAMS, lv_value);
            if (lv_error)
            {
                LOG_AND_TRACE (1, ("SP_Process::set_backup_broker EXIT retrieving PROCESSPARAMS "
                             "for backup for %s on node %d failed with error %d.\n",
                             process_params.sp_params.destbrokers[pv_inx].name.c_str(),
                             process_params.sp_params.destbrokers[pv_inx].node, lv_error));

                return lv_error;
            }

            if (!sp_find_and_set_key_str (lv_value, "SUBTYPE", lp_delim, lv_temp))
            {
                 LOG_AND_TRACE (1, ("SP_Process::set_backup_broker EXIT retrieving "
                              "SUBTYPE for backup for %s on node %d failed with error %d.\n",
                              process_params.sp_params.destbrokers[pv_inx].name.c_str(),
                              process_params.sp_params.destbrokers[pv_inx].node, lv_error));

                 return lv_error;
            }

            // if node or EFB, use local node id
            if ((lv_temp == "NODE") || (lv_temp == "EXTERN"))
            {
                  // replace the node id with this node
                  std::string lv_temp = sp_replace_node_num (
                             process_params.sp_params.destbrokers[pv_inx].node,
                             iv_nid, process_params.sp_params.destbrokers[pv_inx].name);
                  if (lv_temp.empty())
                  {
                        LOG_AND_TRACE (1, ("SP_Process::set_backup_broker EXIT did not find node id in %s\n",
                        process_params.sp_params.destbrokers[pv_inx].name.c_str()));
                        return SP_BACKUP_ERROR;
                   }

                   SPTrace(2, ("SP_Process::set_backup_broker LOCAL change from %d / %s to %d / %s.\n",
                                   process_params.sp_params.destbrokers[pv_inx].node,
                                   process_params.sp_params.destbrokers[pv_inx].name.c_str(),
                                   iv_nid,
                                   lv_temp.c_str()));
                   process_params.sp_params.destbrokers[pv_inx].node = iv_nid;
                   process_params.sp_params.destbrokers[pv_inx].name = lv_temp;
                   SPTrace (2, ("SP_Process::set_backup_broker EXIT using local for %s.\n",
                                process_params.sp_params.destbrokers[pv_inx].name.c_str()));
                   return SP_SUCCESS;
            }
            else
            {
                   LOG_AND_TRACE (1, ("SP_Process::set_backup_broker EXIT - Error retrieving full "
                                "backup info for backup for %s on node %d.\n",
                                process_params.sp_params.destbrokers[pv_inx].name.c_str(),
                                process_params.sp_params.destbrokers[pv_inx].node));
                  return SP_BACKUP_ERROR;
            }
     } // endif node has no backup

     // There is a backup.
     // if the backup is up, then switch the process name and node number -
     // (REWORK for REINTEGRATION)
     if (gv_nodes.is_up(lv_backup))
     {
          std::string lv_temp = sp_replace_node_num (
                                process_params.sp_params.destbrokers[pv_inx].node,
                                lv_backup, process_params.sp_params.destbrokers[pv_inx].name);
          if (lv_temp.empty())
          {
                  LOG_AND_TRACE (1, ("SP_Process::set_backup_broker EXIT (2) did not find node id in %s\n",
                  process_params.sp_params.destbrokers[pv_inx].name.c_str()));
                  return SP_BACKUP_ERROR;
          }

          SPTrace(2, ("SP_Process::set_backup_broker BACKUP change from node %d name %s to %d / %s.\n",
                   process_params.sp_params.destbrokers[pv_inx].node,
                   process_params.sp_params.destbrokers[pv_inx].name.c_str(),
                   lv_backup,
                   lv_temp.c_str()));
          process_params.sp_params.destbrokers[pv_inx].node = lv_backup;
          process_params.sp_params.destbrokers[pv_inx].name = lv_temp;
     }
     else
     {
         LOG_AND_TRACE (1, ("SP_Process::set_backup_broker EXIT double failure for %s.\n",
         process_params.sp_params.destbrokers[pv_inx].name.c_str()));
         return SP_BACKUP_ERROR;
     }

     SPTrace (2, ("SP_Process::set_backup_broker EXIT with error %d (backup node %d).\n",
                  lv_error, lv_backup));
     return lv_error;
}
//--------------------------------------------------------------------
//
// SP_Process::set_dest_brokers
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_dest_brokers (std::string &pp_dest_brokers)
{

    std::string lv_copyValue = pp_dest_brokers;
    std::string lv_copyValue2;
    int         lv_error = SP_SUCCESS;
    size_t      lv_pos;
    std::string lv_temp, lp_value, lp_key;
    std::string lp_delim = " ";

    SPTrace (2, ("SP_Process::set_dest_brokers ENTRY\n"));

    SPTrace (3, ("SP_Process::set_dest_brokers processing destination broker string %s\n",
                  pp_dest_brokers.c_str()));

    lv_pos = lv_copyValue.find_first_of(")");
    if (lv_pos == std::string::npos)
       return 1;

    lv_copyValue[lv_pos] = ' ';

    for (int lv_inx = 0; lv_inx < SP_NUM_DEST_BROKERS; lv_inx++)
    {
         lv_error = sp_get_process_name(lv_copyValue, lv_temp, lp_delim);
         if (lv_error)
         {
             if (lv_inx == 0)
             {
                 // No valid dest broker
                 SPTrace (1, ("SP_Process::set_dest_brokers EXIT 1 with error %d\n", lv_error));
                 return lv_error;
             }
             else
                 // No more (valid) brokers, but at least one was found
                 break;
         }
         process_params.sp_params.destbrokers[lv_inx].name = lv_temp;
         process_params.sp_params.destbrokers[lv_inx].primary_name = lv_temp;

         // now get IP and PORT and NODE
          lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
                  (char *) lv_temp.c_str(), (char *) CONNECTIONINFO, lp_value);
          if (lv_error)
          {
             SPTrace (1, ("SP_Process::set_dest_brokers EXIT 2 with error %d\n", lv_error));
             return lv_error;
          }

          lv_copyValue2 = lp_value;

          if (!sp_find_and_set_key_int (lv_copyValue2, "NODE", lp_delim,
                                       process_params.sp_params.destbrokers[lv_inx].node))
          {
              SPTrace (1, ("SP_Process::set_dest_brokers EXIT 3 with error %d\n", SP_NOT_FOUND));
              return SP_NOT_FOUND;
          }
          // check if node down, if so - update the backup information
          else if (!gv_nodes.is_up(process_params.sp_params.destbrokers[lv_inx].node))
          {
             lv_error = set_backup_broker(lv_inx);
             if (lv_error)
             {
                 SPTrace (1, ("SP_Process::set_dest_brokers EXIT 4 with error %d (set_backup_broker)\n",
                              lv_error));
                 return lv_error;

             }
          }
           process_params.sp_params.destbrokers[lv_inx].primary_node =
                          process_params.sp_params.destbrokers[lv_inx].node;

         if (!sp_find_and_set_key_str (lv_copyValue2, "IP", lp_delim,
                                       process_params.sp_params.destbrokers[lv_inx].ip))
          {
              SPTrace (1, ("SP_Process::set_dest_brokers EXIT 5 with error %d\n", SP_NOT_FOUND));
              return SP_NOT_FOUND;
          }

          if (!sp_find_and_set_key_int (lv_copyValue2, "PORT",  lp_delim,
                                       process_params.sp_params.destbrokers[lv_inx].port))
          {
              SPTrace (1, ("SP_Process::set_dest_brokers EXIT 6 with error %d\n", SP_NOT_FOUND));
              return SP_NOT_FOUND;
          }

          // get the process params
          lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
                  (char *) lv_temp.c_str(), (char *) PROCESSPARAMS, lp_value);
          if (lv_error)
          {
             SPTrace (1, ("SP_Process::set_dest_brokers EXIT 7 with error %d\n", lv_error));
             return lv_error;
          }

          lv_copyValue2 = lp_value;
          if (!sp_find_and_set_key_str (lv_copyValue2, "SUBTYPE", lp_delim,
                            process_params.sp_params.destbrokers[lv_inx].subtype))
          {
             SPTrace (1, ("SP_Process::set_dest_brokers EXIT 8 with error %d\n", SP_NOT_FOUND));
             return SP_NOT_FOUND;
          }
    }

    SPTrace (2, ("SP_Process::set_dest_brokers EXIT 9 with error %d\n", SP_SUCCESS));
    return SP_SUCCESS;
}

//--------------------------------------------------------------------
//
// SP_Process::set_process_info
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_process_info (std::string &pp_process_info)
{
   std::string lp_key, lp_value;
   std::string lp_delim = " ";
   std::string lp_tempCopy = pp_process_info;
   int         lv_error = SP_SUCCESS;

    SPTrace (2, ("SP_Process::set_process_info ENTRY\n"));
    SPTrace (3, ("SP_Process::set_process_info processing process info string %s\n",
                 pp_process_info.c_str()));

   // expected values
   if (!sp_find_and_set_key_int (lp_tempCopy, "NODE", lp_delim, process_info.node))
   {
      SPTrace (1, ("SP_Process::set_process_info EXIT returning error %d\n",SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }
   else if (process_info.node != iv_nid)
      iv_backup = true;  // this is a backup process
   else
      iv_backup = false;

   process_info.primary_node = process_info.node;

   SPTrace (1, ("SP_Process::set_process_info is backup %d\n", iv_backup));

   if (!sp_find_and_set_key_str (lp_tempCopy, "MODE", lp_delim, process_info.mode))
   {
      SPTrace (1, ("SP_Process::set_process_info EXIT returning error %d (MODE)\n",SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }

   lv_error = sp_find_key (lp_tempCopy, "START", lp_value, lp_delim);
   if (lv_error != SP_SUCCESS)
   {
      SPTrace (1, ("SP_Process::set_process_info EXIT returning error %d\n",SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }

   if (lp_value == "U")
       process_info.startup = SP_START_UP;
   else
       process_info.startup = SP_START_DOWN;

   lv_error = sp_find_key (lp_tempCopy, "TRACE", lp_value, lp_delim);
   if (lv_error != SP_SUCCESS)
   {
      SPTrace (1, ("SP_Process::set_process_info EXIT returning error %d\n",SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }
   process_info.trace = sp_convert_to_yes_or_no(lp_value);

   lv_error = sp_find_key (lp_tempCopy, "METRICS", lp_value, lp_delim);
   if (lv_error != SP_SUCCESS)
   {
      SPTrace (1, ("SP_Process::set_process_info EXIT returning error %d\n",SP_NOT_FOUND));
      return SP_NOT_FOUND;
    }
    process_info.metrics = sp_convert_to_yes_or_no(lp_value);

   SPTrace (2, ("SP_Process::set_process_info EXIT returning error %d\n",SP_SUCCESS));
   return SP_SUCCESS;
}

//--------------------------------------------------------------------
//
// SP_Process::set_broker_process_params
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_broker_process_params ()
{
   std::string lp_key, lp_value;
   std::string lp_delim = " ";
   std::string lp_tempCopy;
   int         lv_index = process_params.arg_count;
   int         lv_error = SP_SUCCESS;

   SPTrace (2, ("SP_Process::set_broker_process_params ENTRY\n"));

   lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
              (char *) get_name(), (char *) BROKERARGS, lp_value);

   if (lv_error)
   {
      SPTrace (1, ("SP_Process::set_broker_process_params EXIT returning %d (BROKERARGS)\n", SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }

   SPTrace (3, ("SP_Process::set_broker_process_params processing broker process params string %s\n",
                 lp_value.c_str()));

   lp_tempCopy = lp_value;

   if (!sp_find_and_set_key_str (lp_tempCopy, "PORT", lp_delim,
                                 lp_value))
   {
      SPTrace (1, ("SP_Process::set_broker_process_params EXIT returning %d\n", SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }
   sprintf (process_params.sp_prog_args[lv_index++], "--port=%s", lp_value.c_str());


   if (!sp_find_and_set_key_str (lp_tempCopy, "WORKERTHREADS", lp_delim,
                                 lp_value))
   {
      SPTrace (1, ("SP_Process::set_broker_process_params EXIT returning %d\n", SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }
   sprintf (process_params.sp_prog_args[lv_index++], "--worker-threads=%s", lp_value.c_str());

   if (!sp_find_and_set_key_str (lp_tempCopy, "DATADIR", lp_delim,
                                 lp_value))
   {
      SPTrace (1, ("SP_Process::set_broker_process_params EXIT returning %d\n", SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }
   sprintf (process_params.sp_prog_args[lv_index++], "--%s", lp_value.c_str());

   if (!sp_find_and_set_key_str (lp_tempCopy, "AUTH", lp_delim,
                                 lp_value))
   {
      SPTrace (1, ("SP_Process::set_broker_process_params EXIT returning %d\n", SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }
   sprintf (process_params.sp_prog_args[lv_index++], "--auth=%s", lp_value.c_str());

   if (!sp_find_and_set_key_str (lp_tempCopy, "DAEMON", lp_delim,
                                 lp_value))
   {
      SPTrace (1, ("SP_Process::set_broker_process_params EXIT returning %d\n", SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }
   sprintf (process_params.sp_prog_args[lv_index++], "--%s", lp_value.c_str());

   process_params.arg_count = lv_index;

   SPTrace (2, ("SP_Process::set_broker_process_params EXIT returning %d\n", SP_SUCCESS));
   return SP_SUCCESS;
}

//--------------------------------------------------------------------
//
// SP_Process::set_tp_process_params
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_tp_process_params ()
{
   std::string lp_value;
   int         lv_error = SP_SUCCESS;

   SPTrace (2, ("SP_Process::set_tp_process_params ENTRY\n"));

   lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
              (char *) get_name(), (char *) TPARGS, lp_value);

   SPTrace (3, ("SP_Process::set_tp_process_params TPARGS : %s\n", lp_value.c_str()));

   if (lv_error)
   {
      SPTrace (1, ("SP_Process::set_tp_process_params EXIT returning %d (TPARGS)\n", SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }

   set_publish_info(SP_NODE_BROKER_INDEX);
   set_optional_process_params (lp_value);

   SPTrace (2, ("SP_Process::set_tp_process_params EXIT with error %d\n", lv_error));

   return lv_error;

}

//--------------------------------------------------------------------
//
// SP_Process::set_lcsh_process_params
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_lcsh_process_params ()
{
   std::string lp_value;
   int         lv_error = SP_SUCCESS;

   SPTrace (2, ("SP_Process::set_lcsh_process_params ENTRY\n"));

   lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
              (char *) get_name(), (char *) LCSHARGS, lp_value);

   if (lv_error)
   {
      SPTrace (1, ("SP_Process::set_lcsh_process_params EXIT returning %d (LCSHARGS)\n", SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }

   SPTrace (3, ("SP_Process::set_lcsh_process_params LCSHARGS : %s\n", lp_value.c_str()));

   set_publish_info(SP_NODE_BROKER_INDEX);
   set_optional_process_params (lp_value);

   SPTrace (2, ("SP_Process::set_lcsh_process_params EXIT with error %d\n", lv_error));

   return lv_error;

}

//--------------------------------------------------------------------
//
// SP_Process::set_pub_only_process_params
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_pub_only_process_params ()
{
   int lv_error = SP_SUCCESS;

   SPTrace (2, ("SP_Process::set_pub_only_process_params ENTRY\n"));

   set_publish_info(SP_NODE_BROKER_INDEX);

   SPTrace (2, ("SP_Process::set_pub_only_process_params EXIT with error %d\n", lv_error));

   return lv_error;

}


//--------------------------------------------------------------------
//
// SP_Process::set_sub_only_process_params
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_sub_only_process_params ()
{
   int lv_error = SP_SUCCESS;

   SPTrace (2, ("SP_Process::set_sub_only_process_params ENTRY\n"));

   set_subscribe_info();

   SPTrace (2, ("SP_Process::set_sub_only_process_params EXIT with error %d\n", lv_error));

   return lv_error;

}

//--------------------------------------------------------------------
//
// SP_Process::set_nsa_process_params
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_nsa_process_params ()
{
   int lv_error = SP_SUCCESS;
   int lv_index = process_params.arg_count;

   SPTrace (2, ("SP_Process::set_nsa_process_params ENTRY\n"));

   // this process doesn't follow the general publish/sucscribe broker args model, so
   // this has to be here.
   sprintf(process_params.sp_prog_args[lv_index++], "--node-broker-port=%d",
           process_params.sp_params.destbrokers[SP_NODE_BROKER_INDEX].port);
   sprintf(process_params.sp_prog_args[lv_index++], "--node-broker-ip=%s",
           process_params.sp_params.destbrokers[SP_NODE_BROKER_INDEX].ip.c_str());

   process_params.arg_count = lv_index;

   SPTrace (2, ("SP_Process::set_nsa_process_params EXIT with error %d\n", lv_error));

   return lv_error;

}

//--------------------------------------------------------------------
//
// SP_Process::set_pm_process_params
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_pm_process_params ()
{
   std::string lp_value;
   int         lv_error = SP_SUCCESS;

   SPTrace (2, ("SP_Process::set_pm_process_params ENTRY\n"));

   lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
              (char *) get_name(), (char *) PMARGS, lp_value);

   if (lv_error)
   {
      SPTrace (1, ("SP_Process::set_pm_process_params EXIT returning %d (PMARGS)\n", SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }

   SPTrace (3, ("SP_Process::set_pm_process_params PMARGS : %s\n", lp_value.c_str()));

   set_publish_info(SP_NODE_BROKER_INDEX);
   set_subscribe_info();
   set_optional_process_params (lp_value);

   SPTrace (2, ("SP_Process::set_pm_process_params EXIT with error %d\n", lv_error));

   return lv_error;

}
//--------------------------------------------------------------------
//
// SP_Process::set_smad_process_params
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_smad_process_params ()
{
   SPTrace (2, ("SP_Process::set_smad_process_params ENTRY\n"));

   set_publish_info(SP_NODE_BROKER_INDEX);
   set_subscribe_info();

   SPTrace (2, ("SP_Process::set_smad_process_params EXIT with error %d\n", SP_SUCCESS));
   return SP_SUCCESS;
}

//--------------------------------------------------------------------
//
// SP_Process::set_snmp_process_params
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_snmp_process_params ()
{
   std::string lp_value;
   int lv_error = SP_SUCCESS;

   SPTrace (2, ("SP_Process::set_snmp_process_params ENTRY\n"));

   lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
              (char *) get_name(), (char *) SNMPARGS, lp_value);

   if (lv_error)
   {
      SPTrace (1, ("SP_Process::set_snmp_process_params EXIT returning %d (SNMPARGS)\n", SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }

   SPTrace (3, ("SP_Process::set_snmp_process_params SNMPARGS : %s\n", lp_value.c_str()));

   set_publish_info(SP_NODE_BROKER_INDEX);
   set_subscribe_info();
   set_optional_process_params (lp_value);

   SPTrace (2, ("SP_Process::set_snmp_process_params EXIT with error %d\n", SP_SUCCESS));
   return SP_SUCCESS;
}

//--------------------------------------------------------------------
//
// SP_Process::set_una_process_params
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_una_process_params()
{
   std::string lp_value;
   int         lv_error = SP_SUCCESS;
   int         lv_index = 0;

   SPTrace (2, ("SP_Process::set_una_process_params ENTRY\n"));

   lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
              (char *) get_name(), (char *) UNAARGS, lp_value);

   if (lv_error)
   {
      SPTrace (1, ("SP_Process::set_una_process_params EXIT returning %d (UNAARGS)\n", SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }

   SPTrace (3, ("SP_Process::set_una_process_params UNAARGS : %s\n", lp_value.c_str()));

   set_publish_info(SP_NODE_BROKER_INDEX);

   lv_index = process_params.arg_count;
   sprintf(process_params.sp_prog_args[lv_index++], "--subscribe-broker=%s:%d",
           process_params.sp_params.sourcebroker.ip.c_str(),
           process_params.sp_params.sourcebroker.port);
   sprintf(process_params.sp_prog_args[lv_index++], "--subscribe-broker=%s:%d",
           process_params.sp_params.sourcebroker.ip.c_str(),
           process_params.sp_params.sourcebroker.port);
   sprintf(process_params.sp_prog_args[lv_index++], "--subscribe-broker=%s:%d",
           process_params.sp_params.sourcebroker.ip.c_str(),
           process_params.sp_params.sourcebroker.port);
   process_params.arg_count = lv_index;

   set_optional_process_params (lp_value);

   SPTrace (2, ("SP_Process::set_una_process_params EXIT with error %d\n", lv_error));
   return SP_SUCCESS;
}
//--------------------------------------------------------------------
//
// SP_Process::set_unc_process_params
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_unc_process_params ()
{
   std::string lp_key, lp_value;
   std::string lp_delim = " ";
   std::string lp_tempCopy;
   int lv_error = SP_SUCCESS;

   SPTrace (2, ("SP_Process::set_unc_process_params ENTRY\n"));

   lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
              (char *) get_name(), (char *) UNCARGS, lp_value);

   if (lv_error)
   {
      SPTrace (1, ("SP_Process::set_unc_process_params EXIT returning %d (UNCARGS)\n", SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }

   SPTrace (3, ("SP_Process::set_unc_process_params UNCARGS : %s\n", lp_value.c_str()));

   lp_tempCopy = lp_value;
   set_publish_info(SP_NODE_BROKER_INDEX);
   set_subscribe_info();
   set_optional_process_params (lp_tempCopy);

   SPTrace (2, ("SP_Process::set_unc_process_params EXIT with error %d\n", SP_SUCCESS));
   return SP_SUCCESS;
}


//--------------------------------------------------------------------
//
// SP_Process::set_ptpa_process_params
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_ptpa_process_params ()
{
   std::string lp_key, lp_value;
   std::string lp_delim = " ";
   std::string lp_tempCopy;
   int lv_error = SP_SUCCESS;

   SPTrace (2, ("SP_Process::set_ptpa_process_params ENTRY\n"));

   lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
              (char *) get_name(), (char *) PTPAARGS, lp_value);

   if (lv_error)
   {
      SPTrace (1, ("SP_Process::set_ptpa_process_params EXIT returning %d (PTPAARGS)\n", SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }

   SPTrace (3, ("SP_Process::set_ptpa_process_params PTPAARGS : %s\n", lp_value.c_str()));
   
   lp_tempCopy = lp_value;
   set_publish_info(SP_NODE_BROKER_INDEX);
   set_subscribe_info();
   set_optional_process_params (lp_tempCopy);

   SPTrace (2, ("SP_Process::set_ptpa_process_params EXIT with error %d\n", SP_SUCCESS));
   return SP_SUCCESS;
}


//--------------------------------------------------------------------
//
// SP_Process::set_optional_process_params
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
void SP_Process::set_optional_process_params (std::string &pp_process_params)
{
   std::string lp_key, lp_value;
   std::string lp_delim = " ";
   std::string lp_tempCopy = pp_process_params;
   int lv_index = process_params.arg_count;

   if (sp_find_and_set_key_str (lp_tempCopy, "BADMESSAGES", lp_delim, lp_value))
   {
       SPTrace (3, ("SP_Process::set_optional_process_params BADMESSAGES found\n"));
        sprintf(process_params.sp_prog_args[lv_index++], "--bad-messages=%s",
                general_info.bad_messages.c_str());
   }

   if (sp_find_and_set_key_str (lp_tempCopy, "XMLFILE", lp_delim,
                                lp_value))
   {
      SPTrace (3, ("SP_Process::set_optional_process_params XMLFILE found\n"));
      sprintf(process_params.sp_prog_args[lv_index++], "--config-file=%s/%s", iv_sq_root,lp_value.c_str());
   }


   if (sp_find_and_set_key_str (lp_tempCopy, "PROTOSOURCE", lp_delim, lp_value))
   {
       SPTrace (3, ("SP_Process::set_optional_process_params PROTOSOURCE found\n"));
       sprintf(process_params.sp_prog_args[lv_index++], "--proto-src=%s", general_info.proto_src.c_str());
   }

   if (sp_find_and_set_key_str (lp_tempCopy, "TEXTCATALOG", lp_delim, lp_value))
   {
       SPTrace (3, ("SP_Process::set_optional_process_params TEXTCATALOG found\n"));
       sprintf(process_params.sp_prog_args[lv_index++], "--text-catalog=%s",
                general_info.text_catalog.c_str());

   }

   if (sp_find_and_set_key_str (lp_tempCopy, "OVERFLOW", lp_delim, lp_value))
   {
       SPTrace (3, ("SP_Process::set_optional_process_params OVERFLOW found\n"));
       sprintf(process_params.sp_prog_args[lv_index++], "--overflow=%s", general_info.overflow.c_str());
   }

   if (sp_find_and_set_key_str (lp_tempCopy, "PMCONTEXT", lp_delim, lp_value))
   {
       SPTrace (1, ("SP_Process::set_optional_process_params PMCONTEXT found\n"));
       sprintf(process_params.sp_prog_args[lv_index++], "--context=%s", general_info.pm_context.c_str());
   }

   if (sp_find_and_set_key_str (lp_tempCopy, "CONFIGDIR", lp_delim, lp_value))
   {
       SPTrace (1, ("SP_Process::set_optional_process_params CONFIGDIR found\n"));
       sprintf(process_params.sp_prog_args[lv_index++], "--config-dir=%s", general_info.config_dir.c_str());
   }

   process_params.arg_count = lv_index;

   SPTrace (2, ("SP_Process::set_optional_process_params EXIT\n"));
}
//--------------------------------------------------------------------
//
// SP_Process::set_gen_process_params
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_gen_process_params (std::string &pp_process_params)
{
   std::string lp_key, lp_value;
   std::string lp_delim = " ";
   std::string lp_tempCopy = pp_process_params;
   int lv_error = SP_SUCCESS;

   SPTrace (2, ("SP_Process::set_gen_process_params ENTRY\n"));
   SPTrace (3, ("SP_Process::set_gen_process_params processing process params string %s\n",
                 pp_process_params.c_str()));

  // optional
   if (!sp_find_and_set_key_str (lp_tempCopy, "SUBTYPE", lp_delim,
                                       process_params.sp_params.subtype))
   {
      SPTrace (1, ("SP_Process::set_gen_process_params SUBTYPE not found\n"));
      lv_error = SP_SUCCESS;;
   }

  // optional
   if (!sp_find_and_set_key_str(lp_tempCopy, "SOURCEBROKER", lp_delim,
                                process_params.sp_params.sourcebroker.name))
   {
      SPTrace (1, ("SP_Process::set_gen_process_params %s not found\n", "SOURCEBROKER"));
      lv_error = SP_SUCCESS;
   }
   else
   {
       process_params.sp_params.sourcebroker.primary_name = process_params.sp_params.sourcebroker.name;
       lv_error = set_source_broker();
   }

   if (!sp_find_and_set_key_int (lp_tempCopy, "ARGS", lp_delim,
                                process_params.arg_count))
   {
      SPTrace (1, ("SP_Process::set_gen_process_params EXIT with error %d (ARGS)\n", SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }

   process_params.org_arg_count = process_params.arg_count;

   for (int lv_inx = 1; lv_inx <=  process_params.arg_count; lv_inx++)
   {
      char temp[10];
      sprintf(temp, "ARGS%d", lv_inx);
      lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
             (char *) iv_name.c_str(), (char *) temp, lp_value);
      if (lv_error)
      {
           SPTrace (1, ("SP_Process::set_gen_process_params EXIT with error %d(%s)\n", lv_error, temp));
           return lv_error;
      }
      else
      {
         sprintf (process_params.sp_prog_args[lv_inx-1], "%s", lp_value.c_str());
       }
   }

   // zero out the rest
   for (int lv_inx = process_params.arg_count; lv_inx < SP_MAX_PROG_ARGS; lv_inx++)
       sprintf (process_params.sp_prog_args[lv_inx], "%d", 0);

   lp_delim = ")";
   lv_error = sp_find_key (lp_tempCopy, "DESTBROKERS", lp_value, lp_delim);
   if (lv_error != SP_SUCCESS)
   {
      if (lv_error != SP_NOT_FOUND)
      {
          SPTrace (1, ("SP_Process::set_gen_process_params EXIT returning %d (DESTBROKERS)\n", lv_error));
          return lv_error;
      }
   }
   else
   {
       lp_value += ")";

       lv_error = set_dest_brokers (lp_value);
       if (lv_error)
       {
           SPTrace (1, ("SP_Process::set_gen_process_params EXIT returning %d (set_dest_brokers)\n",
                        SP_NOT_FOUND));
           return SP_NOT_FOUND;
       }
   }

   SPTrace (2, ("SP_Process::set_gen_process_params EXIT with error %d\n", SP_SUCCESS));
   return SP_SUCCESS;
}

//--------------------------------------------------------------------
//
// SP_Process::set_process_params
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_process_params (std::string &pp_process_params)
{
   std::string lp_key, lp_value;
   std::string lp_delim = " ";
   std::string lp_tempCopy = pp_process_params;
   int         lv_error = SP_SUCCESS;

   SPTrace (2, ("SP_Process::set_process_params ENTRY\n"));
   SPTrace (3, ("SP_Process::set_process_params processing process params string %s\n",
                pp_process_params.c_str()));

   lv_error = sp_find_key (lp_tempCopy, "TYPE", lp_value, lp_delim);
   if (lv_error != SP_SUCCESS)
   {
      SPTrace (1, ("SP_Process::set_process_params EXIT with error %d\n",SP_NOT_FOUND ));
      return SP_NOT_FOUND;
   }

   process_params.type = sp_return_type (lp_value);

   if (!sp_find_and_set_key_str (lp_tempCopy, "OBJ", lp_delim, process_params.obj))
   {
      SPTrace (1, ("SP_Process::set_process_params EXIT with error %d\n",SP_NOT_FOUND ));
      return SP_NOT_FOUND;
   }

   lv_error = set_gen_process_params(lp_tempCopy);
   if (lv_error)
   {
      SPTrace (1, ("SP_Process::set_process_params EXIT with error %d\n",lv_error ));
      return SP_NOT_FOUND;
   }

    lv_error = set_specific_params();
    SPTrace (2, ("SP_Process::set_process_params EXIT with error %d\n",lv_error ));
    return lv_error;
}

//--------------------------------------------------------------------
//
// SP_Process::set_specific_params
// Purpose - helper method - process params configuration
//
// -------------------------------------------------------------------
int SP_Process::set_specific_params()
{
  int lv_error = SP_SUCCESS;

  process_params.arg_count = process_params.org_arg_count;
  SPTrace (3, ("SP_Process::set_specific_params ENTRY resetting arg count to %d\n",
               process_params.arg_count ));

  // Take care of our specific (non generic) processes.  These are ones that we have
  // internal knowledge of
  switch (process_params.type)
   {
   case SP_BROKER:
      lv_error = set_broker_process_params();
      break;
   case SP_UNC:
      lv_error = set_unc_process_params();
      break;
   case SP_PM:
      lv_error = set_pm_process_params();
      break;
   case SP_LCSH:
      lv_error = set_lcsh_process_params();
      break;
   case SP_SNMP:
      lv_error = set_snmp_process_params();
      break;
   case SP_SMAD:
      lv_error = set_smad_process_params();
      break;
   case SP_NSA:
      lv_error = set_nsa_process_params();
      break;
   case SP_TPA:
      lv_error = set_pub_only_process_params();
      break;
   case SP_UAA:
      lv_error = set_sub_only_process_params();
      break;
   case SP_TP:
      lv_error = set_tp_process_params();
      break;
   case SP_UNA:
      lv_error = set_una_process_params();
      break;
   case SP_PTPA:
      lv_error = set_ptpa_process_params();
      break;
   default:
     break;
   }

   SPTrace (3, ("SP_Process::set_specific_params EXIT with error %d\n", lv_error));
   return lv_error;
}
//--------------------------------------------------------------------
//
// SP_Process::set_connection_info
// Purpose - helper method - configuration
//
// -------------------------------------------------------------------
int SP_Process::set_connection_info (std::string &pp_connection_info)
{
   std::string lp_key, lp_value;
   std::string lp_delim = " ";
   std::string lp_tempCopy = pp_connection_info;
   int         lv_error = SP_SUCCESS;

   SPTrace (2, ("SP_Process::set_connection_info ENTRY\n"));
   SPTrace (3, ("SP_Process::set_connection_info processing connection info string %s\n",
                pp_connection_info.c_str()));

   // required
    if (!sp_find_and_set_key_int (lp_tempCopy, "NODE", lp_delim, connection_info.node))
    {
        SPTrace (1, ("SP_Process::set_connection_info EXIT with error %d\n", SP_NOT_FOUND));
        return SP_NOT_FOUND;
    }

   connection_info.original_node = connection_info.node;
   SPTrace (3, ("SP_Process::set_connection_info setting node %d (%d)\n", connection_info.node,
                  connection_info.original_node));
   // optional
   lv_error = sp_find_key (lp_tempCopy, "IP", lp_value, lp_delim);
   if ((lv_error != SP_SUCCESS) && (lv_error != SP_NOT_FOUND))
   {
      SPTrace (1, ("SP_Process::set_connection_info EXIT with error %d\n", SP_NOT_FOUND));
      return SP_NOT_FOUND;
   }

   if (lv_error != SP_NOT_FOUND)
       connection_info.ip = lp_value;

   lv_error = sp_find_key (lp_tempCopy, "PORT", lp_value, lp_delim);
   if ((lv_error != SP_SUCCESS) && (lv_error != SP_NOT_FOUND))
   {
       SPTrace (1, ("SP_Process::set_connection_info EXIT with error %d\n", SP_NOT_FOUND));
       return SP_NOT_FOUND;
   }

   if (lv_error != SP_NOT_FOUND)
       connection_info.port = atoi(lp_value.c_str());

   SPTrace (2, ("SP_Process::set_connection_info EXIT with error %d\n", SP_SUCCESS));
   return SP_SUCCESS;
}

//---------------------------------------------------------------------------
//
// parseStartupString - NOT USED, but keeping since this might be useful later
// Purpose - break up a string into command line parameters, similar
// to how the shell would do it
//
// ---------------------------------------------------------------------------
int parseStartupString(char * pp_startupString, char * pa_argv[], int pv_maxSubstrings)
{
    int lv_stringCount = 0;
    char * lp_next = pp_startupString;

    while ((*lp_next) && (lv_stringCount < pv_maxSubstrings))
    {
        pa_argv[lv_stringCount] = lp_next;
        lv_stringCount++;
        char * lp_candidateBreak = lp_next+1;

        // search for the next break between command line parameters

        while ((*lp_candidateBreak) &&
               ((*lp_candidateBreak != ' ') ||
                (strncmp(lp_candidateBreak," --",3) != 0)))
        {
            lp_candidateBreak++;
        }

        if (*lp_candidateBreak)  // if we found one
        {
            // slam a null in there, breaking it off from previous parameter
            *lp_candidateBreak = '\0';
            lp_next = lp_candidateBreak + 1;  // point to "--"
        }
        else  // didn't find one; we are at end of string
        {
            lp_next = lp_candidateBreak;  // set to end of string
        }
    }

    return lv_stringCount;
}

//---------------------------------------------------------------------------
//
// stop_process
// Purpose - stop a process
//
// ---------------------------------------------------------------------------
int SP_Process::stop_process(int pv_proxy_nid)
{
    int lv_error = SP_SUCCESS;

    SPTrace (2, ("spProxy::stop_process ENTRY for %s\n", get_name()));

    // Ignore non-positive pids. No process to kill.
    if (iv_pid <= 0)
        return lv_error;

    // Stop UNC processes with kill() instead of msg_mon_stop_process.
    // We first tried using SIGTERM but this did not guarentee the UNC
    // process will be killed. In fact, none of the backup UNC processes
    // were successfully killed. Only SIGKILL guarantee this.

    if ((pv_proxy_nid == iv_nid) && (type() == SP_UNC))
    {
        lv_error = kill(iv_pid, SIGKILL );
        if (lv_error < 0)
            LOG_AND_TRACE (3, ("spProxy::stop_process kill %s (pid %d) failed with ERRNO %d\n", get_name(), iv_pid, errno));
        else
        {
            LOG_AND_TRACE (3, ("spProxy::stop_process kill %s (pid %d) succeeded using SIGKILL\n", get_name(), iv_pid));
            iv_pid = 0;
        }
    }
    else
    {
        // non-UNC process on this node AND ALL processes on other nodes
        // (including UNC processes on other nodes) are terminated with
        // msg_mon_stop_process. There is no other way for proxy to stop
        // a process on another node, so the reader might think that this
        // is a problem for stopping UNC processes on other nodes. But proxy
        // is designed to only stop processes running on its own node. But to
        // be safe, check for and report an error in the proxy log file
        // if an attempt is made to stop a UNC process on another node.
      
        // is it a UNC process on another node?
        if (type() == SP_UNC)
        {
	  LOG_AND_TRACE(0, ("spProxy::stop_process ERROR: stopping %s process pid %d on node (%d) is NOT advised. Doing it anyway.\n", get_name(), iv_pid, iv_nid));
        }

        lv_error = msg_mon_stop_process ((char*)get_name(), iv_nid, iv_pid);

        if (lv_error)  // it failed
        {
          // ignore nonexistent process errors. we don't care that it
          // doesn't exist. that's the point; we want it to not exist.
          if (lv_error != XZFIL_ERR_NOSUCHDEV)
            LOG_AND_TRACE (3, ("spProxy::stop_process msg_mon_stop_process %s failed with %d\n", get_name(), lv_error));
        }
        else
        {
            LOG_AND_TRACE (3, ("spProxy::stop_process msg_mon_stop_process %s succeeded\n", get_name()));
            iv_pid = 0;
        }
    }

    SPTrace (2, ("spProxy::stop_process EXIT with error %d\n", lv_error));
    return lv_error;
}

//--------------------------------------------------------------------
//
// SP_Process::start_process
// Purpose - start up a seapilot process. This is public, callable by anyone.
// it hides the details of what type of process is starting.
//
// -------------------------------------------------------------------
int SP_Process::start_process(int pv_myNid, int pv_node_port, std::string& pv_node_ip)
{
        int lv_error;

        if (is_on_demand_process())
          lv_error = start_on_demand_process(pv_myNid, pv_node_port, pv_node_ip);
        else
          lv_error = start_normal_process(pv_myNid, pv_node_port, pv_node_ip);

        return lv_error;
}

//---------------------------------------------------------------------------
//
// start_on_demand_process
// Purpose - start an on demand process (ex, UNC, PM, SNMP, UNA, UAA)
//
// ---------------------------------------------------------------------------
int SP_Process::start_on_demand_process(int pv_myNid, int pv_node_port, std::string& pv_node_ip)
{
    int lv_error = SP_SUCCESS;

    SPTrace (2, ("spProxy::start_on_demand_process ENTRY for %s\n", get_name()));

    lv_error = start_gen_process(pv_myNid,
                                 pv_node_port,
                                 pv_node_ip,
                                 true /* reset retry limit*/);

    SPTrace (2, ("spProxy::start_on_demand_process EXIT for %s with error %d\n",
                 get_name(), lv_error));
    return lv_error;
}

//--------------------------------------------------------------------
//
// SP_Process::start_process
// Purpose - start up a seapilot process
//
// -------------------------------------------------------------------
int SP_Process::start_normal_process(int pv_myNid, int pv_node_port, std::string& pv_node_ip)
{
    int lv_error = SP_SUCCESS;

    SPTrace (2, ("SP_Process::start_normal_process ENTRY\n"));

    if (iv_retries++ > SP_MAX_PROCESS_RETRIES)
    {
        LOG_AND_TRACE (1, ("SP_Process::start_normal_process EXIT with error %d.  MAX RETRIES hit.\n", lv_error));
        return lv_error;
    }

    switch(process_params.type) {
    case SP_UNC:
    case SP_PM:
    case SP_UAA:
    case SP_UNA:
    case SP_SNMP:
    case SP_PTPA:
    case SP_EBCM:
    case SP_BROKER:
       lv_error = start_gen_process(pv_myNid, pv_node_port, pv_node_ip, true);
       break;
    default:
       lv_error = start_gen_process(pv_myNid, pv_node_port, pv_node_ip, false);
       break;
    }

    SPTrace (2, ("SP_Process::start_normal_process EXIT with error %d\n", lv_error));

    return lv_error;
}

// -----------------------------------------------------------------------------
//
// SP_Process::set_general_info
// Purpose : read in the standard strings for highly used tokens.
//
// --------------------------------------------------------------------------

int SP_Process::set_general_info()
{
    std::string la_buf;
    int         lv_error = SP_SUCCESS;
    lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
                  (char *) CLUSTER_GROUP, (char *) "SP_PROTOSOURCE", la_buf);
    if (lv_error)
    {
         SPTrace (1, ("SP_Process::set_general_info for SP_PROTOSOURCE EXIT with error %d\n", lv_error));
         return SP_NOT_FOUND;
    }

    general_info.proto_src = la_buf;

    lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
                  (char *) CLUSTER_GROUP, (char *) "SP_TEXTCATALOG", la_buf);
    if (lv_error)
    {
         SPTrace (1, ("SP_Process::set_general_info for SP_TEXTCATALOG EXIT with error %d\n", lv_error));
         return SP_NOT_FOUND;
    }

    general_info.text_catalog = la_buf;

    lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
                  (char *) CLUSTER_GROUP, (char *) "SP_BADMESSAGES", la_buf);
    if (lv_error)
    {
         SPTrace (1, ("SP_Process::set_general_info for SP_BADMESSAGES EXIT with error %d\n", lv_error));
         return SP_NOT_FOUND;
    }

    general_info.bad_messages = la_buf;

    lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
                  (char *) CLUSTER_GROUP, (char *) "SP_OVERFLOW", la_buf);
    if (lv_error)
    {
         SPTrace (1, ("SP_Process::set_general_info for SP_OVERFLOW EXIT with error %d\n", lv_error));
         return SP_NOT_FOUND;
    }

    general_info.overflow = la_buf;


    lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
                  (char *) CLUSTER_GROUP, (char *) "SP_PMCONTEXT", la_buf);
    if (lv_error)
    {
         SPTrace (1, ("SP_Process::set_general_info for SP_PMCONTEXT EXIT with error %d\n", lv_error));
         return SP_NOT_FOUND;
    }

    general_info.pm_context = la_buf;

    lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
                  (char *) CLUSTER_GROUP, (char *) "SP_CONFIGDIR", la_buf);
    if (lv_error)
    {
         SPTrace (1, ("SP_Process::set_general_info for SP_CONFIGDIR EXIT with error %d\n", lv_error));
         return SP_NOT_FOUND;
    }

    general_info.config_dir = la_buf;

    SPTrace (2, ("SP_Process::set_general_info EXIT with error %d\n", lv_error));
    return lv_error;
}


//--------------------------------------------------------------------
//
// SP_Process::populate
// Purpose - populate registry/configuration information into data
//           structures.
//
// -------------------------------------------------------------------
int SP_Process::populate ()
{
   int         lv_error = SP_SUCCESS;
   std::string lv_value;

  SPTrace (2, ("SP_Process::populate ENTRY (%s)\n", get_name()));

  lv_error = set_general_info ();

   if (lv_error)
   {
      SPTrace (1, ("SP_Process::populate (set_general_info) EXIT with error %d\n", lv_error));
      return lv_error;
   }

  lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
              (char *) get_name(), (char *) PROCESSINFO, lv_value);
   if (!lv_error)
      lv_error = set_process_info (lv_value);

   if (lv_error)
   {
      SPTrace (1, ("SP_Process::populate (set_process_info) EXIT with error %d for %s with %s\n",
                   lv_error, get_name(), PROCESSINFO));
      return lv_error;
   }

   lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
              (char *) get_name(), (char *) PROCESSPARAMS, lv_value);
   if (!lv_error)
      lv_error = set_process_params (lv_value);

   if (lv_error)
   {
      SPTrace (1, ("SP_Process::populate (set_process_params) EXIT with error %d\n", lv_error));
      return lv_error;
   }

   lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
              (char *) get_name(), (char *) CONNECTIONINFO, lv_value);
   if (!lv_error)
       lv_error = set_connection_info (lv_value);

   if (lv_error)
   {
      SPTrace (1, ("SP_Process::populate (set_connection_info) EXIT with error %d\n", lv_error));
      return lv_error;
   }

   // can not have 127.0.0.1 for local use, so we don't need registry
   if (process_params.sp_params.subtype == "NODE")
      connection_info.ip = iv_ip_addr;

   // if this is a backup process, we need to change name and node.  All else has
   // dynamically been changed
 /*
   if (iv_backup)
   {
       std::string lv_temp = sp_replace_node_num (process_info.node, iv_nid, iv_name);
       if (lv_temp.empty())
       {
            SPTrace (2, ("SP_Process::populate EXIT unable to set backup name.  Error %d\n",
                         SP_BACKUP_ERROR));
            return SP_BACKUP_ERROR;
       }
       iv_name = lv_temp;
   }*/

   SPTrace (2, ("SP_Process::populate EXIT with error %d\n", SP_SUCCESS));
   return SP_SUCCESS;
}

//--------------------------------------------------------------------
//
// SP_Process::start_helper
// set up publish argument
//
// -------------------------------------------------------------------
int SP_Process::start_helper(char *pp_prog, char *pp_name, char *pp_ret_name, int pv_argc,
                             char **pp_argv, SB_Phandle_Type  *pp_phandle, int pv_open,
                             int  *pp_oid, int pp_ptype, int pv_nid)
{

    int         lv_error = SP_SUCCESS;
    int         lv_nid(pv_nid);        // for naming consistency with lv_pid
    int         lv_pid;
    int         lv_retryCount = 3;
    int         i;
    char        *p;
    std::string failed_op;

    SPTrace (1, ("SP_Process::start_helper %s ENTRY\n", pp_name));

    // Dump the argv array so we can see how we composed the broker command line
    for(i=0, p=pp_argv[0]; i<pv_argc; i++, p=pp_argv[i])
        LOG_AND_TRACE (3, ("SP_Process::start_helper argv[%d] = %s\n", i, p));

    // If we're restarting, there may be a broker left over from
    // the previous incarnation that hasn't finished dying. If so,
    // we'll get an error (FSERR) from the monitor that indicates
    // "duplicate process name" when we try to start the broker.
    // This is a retryable error. Try up to 3 more times before
    // giving up.
    //
    // Main loop:
    // 1. Try to start the process.
    // 2a Start worked: register for the death notice.
    //    3a Register worked: all done
    //    3b Register failed: kill process (try again)
    // 2b Start failed:
    //    4 did the failure occur because of a duplicate process name?
    //      4a yes. Register for death notice.
    //         5a Register worked: kill process. Exit.
    //         5b Register failed: kill process (try again)
    //    - 4b some other error besides dup proc name
    //         6 try again
    //
    // Error conditions: if an error occurs, it is immediately reported using
    // LOG_AND_TRACE. The main loop continues around (up to 3 times) in case
    // the error is transient and retryable.
    //
    // Success is when the process is started and the death notice is registered
    // (steps 1, 2a and 3a are successful). Success can also be achieved if we
    // encounter a duplicate process (2b, 4) and are able to be notified when
    // it dies (4a, 5a).

    do
    {
        lv_error = msg_mon_start_process(pp_prog,            // prog
                                         (char *) pp_name,   // name
                                         pp_ret_name,        // ret name
                                         pv_argc,
                                         pp_argv,
                                         pp_phandle,
                                         pv_open,            // open
                                         pp_oid,             // oid
                                         pp_ptype,           // type
                                         0,                  // priority
                                         0,                  // debug
                                         0,                  // backup
                                         &lv_nid,            // nid
                                         &lv_pid,            // pid
                                         NULL,               // infile
                                         NULL);              // outfile

        // If creation worked, register for the death notice. We want to know
        // if/when it dies so we can restart it.
        if (!lv_error)
        {
            // process creation worked. regardless of what happens from here on,
            // record the fact that it exists.
            iv_nid = lv_nid;
            iv_pid = lv_pid;
            iv_started = true;

            lv_error = msg_mon_register_death_notification(lv_nid, lv_pid);

            // if registration failed, we aren't going to be told when the
            // process dies. Therefore we won't ever try to start it later. We have
            // to kill the process now and retry all over again, because once this
            // routine ends we're not coming back.
            if (lv_error)
            {
                failed_op="msg_mon_register_death_notification";
                // registration failed. kill the process and try again.
                // There's no point in checking the return status of the following
                // call; we can't do anything to recover if it fails except to carry
                // on and hope we eventually succeed.
                //
                // The argument to stop_process is the nid of PROXY, not of the
                // process that was just started. Proxy only starts processes running
                // on its own node, so the value being passed is ok.
                stop_process(lv_nid);
            }
            else
            {
                // Process created and registration for death notice worked.
                break;   // leave the do...while loop! success!
            } // register for death notice worked
        }  // process creation worked
        else
        {
            failed_op="msg_mon_start_process";
            // XZFIL_ERR_FSERR is returned if the start failed because a process with
            // the same name already exists (is running). If that happens, make sure we will
            // get a death notice for it (it may be left over fron a previous run of SeaQuest
            // and never died). Once we are able to get the death notice, kill the process.
            // Once the death notice arrives, we'll try to start the process again, and at
            // that point it should work.
            if (lv_error == XZFIL_ERR_FSERR)
            {
                failed_op="duplicate process name";

                lv_error = msg_mon_get_process_info((char *) pp_name, &lv_nid, &lv_pid);
                if (lv_error)
                    failed_op="msg_mon_get_process_info";
                else
                {
                    lv_error = msg_mon_register_death_notification(lv_nid, lv_pid);

                    if (lv_error)
                        failed_op += " and msg_mon_register_death_notification";
                    else
                    {
                        // The argument to stop_process is the nid of PROXY, not of the
                        // process that was just started. Proxy only starts processes running
                        // on its own node, so the value passed here is OK.
                        lv_error = stop_process(lv_nid);

                        if (lv_error)
                            failed_op += " and stop_process";
                        else
                        {
                            // We failed to start the process (duplicate process)
                            // but we did succeed in registering for the death notice and
                            // killing the existing process. At this point, we have to leave
                            // this routine so that we can process the death notice when
                            // it arrives.
                            break; 
                        }  // stop_process worked
                    }  // register_death_notification worked
                }  // get_process_info worked
            }  // proc creation failed due to duplicate process name
        }  // process creation failed

        // If we reach here, that means one or more operations failed
        // and we have to retry the process creation until we reach 
        // the retry limit.
        lv_retryCount--;
        LOG_AND_TRACE(1, ("SP_Process::start_helper %s failed with error %d from %s. %d retries left.\n",
                          pp_name, lv_error, failed_op.c_str(), lv_retryCount));
    } while (lv_retryCount>0);

    return lv_error;
}

//--------------------------------------------------------------------
//
// SP_Process::start_gen_process
// Purpose - start up the process as a seaquest process
//
// -------------------------------------------------------------------
int SP_Process::start_gen_process(int pv_myNid, int pv_node_port, std::string& pv_node_ip,
                                  bool pv_reset_retries)
{
    char        la_arg_count[24];
    char        la_exe[MS_MON_MAX_PROCESS_PATH+1];
    char        la_process_type[MAX_PROCESS_TYPE_LENGTH];
    char        la_prog[MS_MON_MAX_PROCESS_PATH+1];
    char        la_ret_name[MS_MON_MAX_PROCESS_NAME+1];
    char        la_server_name[MS_MON_MAX_PROCESS_NAME+1];
    int         lv_argc = 0;
    char        *lv_argv[SP_MAX_PROG_ARGS + 3]; // + 3 is for type and process name and # of program args
    int         lv_error;
    TPT_DECL    (lv_server_phandle);

    LOG_AND_TRACE (2, ("SP_Process::start_gen_process ENTRY %s (%s, mode %s)\n",
                  get_name(),process_params.obj.c_str(), process_info.mode.c_str()));

    sprintf(la_server_name, "%s", get_name());
    if (process_info.mode == "GEN")
       strcpy(la_exe, WRAPPER_EXE);
    else
       strcpy (la_exe, process_params.obj.c_str());

    if (!sp_find_exec (la_exe, la_prog))
    {
        LOG_AND_TRACE (1, ("SP_Process::start_gen_process exe (%s) not found.\n", la_exe));
        return SP_NOT_FOUND;
    }

   sprintf(la_process_type, "%d", process_params.type);
   sprintf(la_arg_count, "%d", process_params.arg_count);

    // The proxy has 3 standard first arguments
    // 0 : What exe should we launch, wrapper or actual exe
    // 1 : Type of process
    // 2 : How many args to follow
    if (process_info.mode == "GEN")
       lv_argv[0]= (char *)WRAPPER_EXE;
    else
       lv_argv[0]=la_prog;


   lv_argv[1] = la_process_type;
   lv_argv[2] = la_arg_count;

   for (int lv_inx = 0; lv_inx < SP_MAX_PROG_ARGS; lv_inx++)
       lv_argv[lv_inx+3] = process_params.sp_prog_args[lv_inx];

   lv_argc = process_params.arg_count + 3;

   lv_error = start_helper(la_prog, (char*) la_server_name, la_ret_name,lv_argc,
                             lv_argv, TPT_REF(lv_server_phandle), 0,
                             NULL, MS_ProcessType_Generic, pv_myNid);

   if ((pv_reset_retries) && (lv_error == SP_SUCCESS))
      iv_retries = 0;

   SPTrace (2, ("SP_Process::start_gen_process EXIT with error %d\n", lv_error));
   return lv_error;
}

//--------------------------------------------------------------------
//
// SP_Process::set_publish_info
// set up publish argument
//
// -------------------------------------------------------------------
int SP_Process::set_publish_info(int pv_broker_index)
{
   int lv_index = process_params.arg_count;
   sprintf(process_params.sp_prog_args[lv_index++], "--publish-broker-port=%d",
           process_params.sp_params.destbrokers[pv_broker_index].port);
   sprintf(process_params.sp_prog_args[lv_index++], "--publish-broker-ip=%s",
           process_params.sp_params.destbrokers[pv_broker_index].ip.c_str());

   process_params.arg_count = lv_index;
   return SP_SUCCESS;
}

//--------------------------------------------------------------------
//
// SP_Process::set_subscribe_info
// set up subscriber argument
//
// -------------------------------------------------------------------
int SP_Process::set_subscribe_info()
{
   SPTrace (3, ("SP_Process::set_subscribe_info ip(%s), port (%d) ENTRY\n",
                process_params.sp_params.sourcebroker.ip.c_str(),
                process_params.sp_params.sourcebroker.port));
   char tempbuf[1024];
   std::string key, value;
   sprintf(tempbuf, "%d",  process_params.sp_params.sourcebroker.port);
   value = tempbuf;
   key = "--subscribe-broker-port=";
   add_process_param(key, value );
   key = "--subscribe-broker-ip=";
   add_process_param(key, process_params.sp_params.sourcebroker.ip);
   return SP_SUCCESS;
}

//-------------------------------------------------------------------
//
// SP_Process::add_process_param
// if the parameter is already set, dont override it
//
//-------------------------------------------------------------------
void SP_Process::add_process_param(std::string& pp_process_param_key, std::string& pp_process_param_value)
{
    bool found = false;
    for(int found_index=0; found_index<process_params.arg_count; found_index++)
    {
        if(pp_process_param_key.compare(0,
                    pp_process_param_key.size(),
                    process_params.sp_prog_args[found_index],
                    pp_process_param_key.size()
                    )==0)
        {
            found = true;
            break;
        }
    }
    if(!found)
    {
        sprintf(process_params.sp_prog_args[process_params.arg_count++],
                "%s%s",
                pp_process_param_key.c_str(),
                pp_process_param_value.c_str()
                );
    }
}


//--------------------------------------------------------------------
//
// SP_Process::reset_brokers
// reset our source and destination brokers
// this is called when a node goes down without a spare node
//
// -------------------------------------------------------------------
int SP_Process::reset_brokers(int pv_nid)
{
        int lv_error;

        lv_error = reset_dest_brokers(pv_nid);
        if (lv_error)
                return lv_error;

        lv_error = reset_source_brokers(pv_nid);
        return lv_error;
}

//--------------------------------------------------------------------
//
// SP_Process::reset_dest_brokers
// this is called when a node goes down without a spare node
//
// -------------------------------------------------------------------
int SP_Process::reset_dest_brokers(int pv_nid)
{
      std::string lp_delim = " ";
      int         lv_error = SP_SUCCESS;
      std::string lv_value;

      SPTrace (2, ("SP_Process::reset_dest_brokers ENTRY (%s)\n", iv_name.c_str()));

     for (int lv_inx = 0; lv_inx < SP_NUM_DEST_BROKERS; lv_inx++)
     {
        if (process_params.sp_params.destbrokers[lv_inx].name.empty())
            continue;

        SPTrace (3, ("SP_Process::reset_dest_brokers (%s) dest broker node %d (down node %d)\n",
                       process_params.sp_params.destbrokers[lv_inx].name.c_str(),
                       process_params.sp_params.destbrokers[lv_inx].node, pv_nid));

       if (process_params.sp_params.destbrokers[lv_inx].node != pv_nid)
            continue;

      lv_error = set_backup_broker(lv_inx);
      if (lv_error)
      {
           SPTrace (1, ("SP_Process::set_dest_brokers EXIT with error %d (set_backup_broker)\n",
                        lv_error));
           return lv_error;
       }


      // now get the new IP associated with the backup node
 /*      std::string lv_ip;
      lv_error = sp_get_ip_from_registry (lv_backup, lv_ip);


    if (!lv_error)
      {
          SPTrace (3, ("SP_Process::reset_dest_brokers retrieved IP of %s for backup %d\n", lv_ip.c_str(), lv_backup));
          process_params.sp_params.destbrokers[lv_inx].ip = lv_ip;
          // change process name to reflect new node number
          std::string lv_temp = sp_replace_node_num (process_params.sp_params.destbrokers[lv_inx].node,
                                lv_backup, process_params.sp_params.destbrokers[lv_inx].name);
          if (lv_temp.empty())
          {
              SPTrace (1, ("SP_Process::reset_dest_brokers EXIT unable to replace backup name\n"));
              return SP_BACKUP_ERROR;
          }
          process_params.sp_params.destbrokers[lv_inx].name = lv_temp;
          process_params.sp_params.destbrokers[lv_inx].node = lv_backup;
       }
       else
       {
           SPTrace (1, ("SP_Process::reset_dest_brokers EXIT with error %d\n",
                        SP_NOT_FOUND));
           return SP_NOT_FOUND;
       }
  */
      std::string lv_ip;
      lv_error = sp_get_ip_from_registry (process_params.sp_params.destbrokers[lv_inx].node, lv_ip);

      SPTrace (3, ("SP_Process::reset_dest_brokers received ip of %s with node %d\n",
                   lv_ip.c_str() ,process_params.sp_params.destbrokers[lv_inx].node ));

      if (!lv_error)
      {
          process_params.sp_params.destbrokers[lv_inx].ip = lv_ip;
          SPTrace (3, ("SP_Process::reset_dest_brokers reset params\n"));

          // continue resetting our args, just in case something changed
          lv_error = set_specific_params();
      }
      if (lv_error)
      {
           SPTrace (1, ("SP_Process::set_dest_brokers EXIT with error %d (end)\n",
                        lv_error));
           return lv_error;
       }
     }
      SPTrace (1, ("SP_Process::reset_dest_brokers EXIT with error %d\n", SP_SUCCESS));

      return SP_SUCCESS;
}

//--------------------------------------------------------------------
//
// SP_Process::reset_brokers_up
// reset our source and destination brokers if a node comes back (up)
//
// -------------------------------------------------------------------
int SP_Process::reset_brokers_up(int pv_nid)
{
        int lv_error;

        lv_error = reset_dest_brokers_up(pv_nid);
        if (lv_error)
                return lv_error;

        lv_error = reset_source_brokers_up(pv_nid);
        return lv_error;
}

//--------------------------------------------------------------------
//
// SP_Process::reset_dest_brokers_up
// reset our dest brokers if a node comes back
//
// -------------------------------------------------------------------
int SP_Process::reset_dest_brokers_up(int pv_nid)
{
      std::string lp_delim = " ";
      int         lv_error = SP_SUCCESS;
      std::string lv_value;

      SPTrace (2, ("SP_Process::reset_dest_brokers_up ENTRY (%s)\n", iv_name.c_str()));

     for (int lv_inx = 0; lv_inx < SP_NUM_DEST_BROKERS; lv_inx++)
     {
        if (process_params.sp_params.destbrokers[lv_inx].name.empty())
            continue;

        SPTrace (3, ("SP_Process::reset_dest_brokers_up (%s) dest broker node %d (up node %d)\n",
                       process_params.sp_params.destbrokers[lv_inx].name.c_str(),
                       process_params.sp_params.destbrokers[lv_inx].node, pv_nid));

        if (process_params.sp_params.destbrokers[lv_inx].primary_node != pv_nid)
        {
            SPTrace (3, ("SP_Process::reset_dest_brokers_up (%s) - skipping\n",
                        process_params.sp_params.destbrokers[lv_inx].name.c_str()));
            continue;
        }

        SPTrace (3, ("SP_Process::reset_dest_brokers_up resetting from node %s num %d to node %s num %d\n",
                        process_params.sp_params.destbrokers[lv_inx].name.c_str(),
                        process_params.sp_params.destbrokers[lv_inx].node,
                        process_params.sp_params.destbrokers[lv_inx].primary_name.c_str(),
                        process_params.sp_params.destbrokers[lv_inx].primary_node));

        process_params.sp_params.destbrokers[lv_inx].name =
                         process_params.sp_params.destbrokers[lv_inx].primary_name;
        process_params.sp_params.destbrokers[lv_inx].node =
                         process_params.sp_params.destbrokers[lv_inx].primary_node;
      } // endfor

      lv_error = update_dest_broker_ip();
      // continue resetting our args, just in case something changed
      if (!lv_error)
          lv_error = set_specific_params();

      SPTrace (2, ("SP_Process::reset_dest_brokers_up EXIT with error %d\n", lv_error));

      return lv_error;
}

//--------------------------------------------------------------------
//
// SP_Process::update_dest_broker_ip - REWORK for REINTEGRATION
// Purpos : reset our dest brokers ip
//
// -------------------------------------------------------------------
int SP_Process::update_dest_broker_ip()
{
    int         lv_error = SP_SUCCESS;
    std::string lv_ip;

    SPTrace (2, ("SP_Process::update_dest_broker_ip ENTRY\n"));

    for (int lv_inx = 0; lv_inx < SP_NUM_DEST_BROKERS; lv_inx++)
    {
        if (process_params.sp_params.destbrokers[lv_inx].name.empty())
            continue;

        lv_error = sp_get_ip_from_registry (process_params.sp_params.destbrokers[lv_inx].node, lv_ip);
        if (!lv_error)
        {
              SPTrace (2, ("SP_Process::update_dest_broker_ip retrieved IP of %s for node %d\n",
                           lv_ip.c_str(), process_params.sp_params.destbrokers[lv_inx].node));
              process_params.sp_params.destbrokers[lv_inx].ip = lv_ip;
         }
         else
             break;
      }

      if (!lv_error)
      {
          SPTrace (3, ("SP_Process::update_dest_broker_ip reset params\n"));

          // continue resetting our args, just in case something changed
          lv_error = set_specific_params();
      }
      SPTrace (2, ("SP_Process::update_dest_broker_ip EXIT with error %d\n", lv_error));
      return lv_error;
}
//--------------------------------------------------------------------
//
// SP_Process::update_source_broker_ip - REWORK for REINTEGRATION
// Purpos : reset our source brokers ip
//
// -------------------------------------------------------------------
int SP_Process::update_source_broker_ip()
{
     SPTrace (2, ("SP_Process::update_source_broker_ip ENTRY\n"));
     std::string lv_ip;
     int lv_error;

     if (process_params.sp_params.sourcebroker.name.empty())
     {
          SPTrace (2, ("SP_Process::update_source_broker_ip EXIT, no-op\n"));
          return SP_SUCCESS;
     }

     lv_error = sp_get_ip_from_registry (process_params.sp_params.sourcebroker.node, lv_ip);
     if (!lv_error)
     {
         SPTrace (3, ("SP_Process::update_source_broker_ip retrieved IP of %s for node %d\n",
                           lv_ip.c_str(), process_params.sp_params.sourcebroker.node));

         process_params.sp_params.sourcebroker.ip = lv_ip;

         SPTrace (3, ("SP_Process::update_source_broker_ip reset params\n"));
         lv_error = set_specific_params();
      }

     SPTrace (2, ("SP_Process::update_source_broker_ip EXIT with error%d\n", lv_error));

     return lv_error;
}
//--------------------------------------------------------------------
//
// SP_Process::reset_source_brokers - REWORK for REINTEGRATION
// Purpos : reset our source brokers if a node goes down without a spare node
//
// -------------------------------------------------------------------
int SP_Process::reset_source_brokers(int pv_nid)
{
      std::string lp_delim = " ";
      int         lv_backup;
      int         lv_error = SP_SUCCESS;
      std::string lv_value;

      SPTrace (2, ("SP_Process::reset_source_brokers ENTRY (%s)\n", iv_name.c_str()));

      if (process_params.sp_params.sourcebroker.name.empty())
      {
            SPTrace (2, ("SP_Process::reset_source_brokers EXIT, no-op\n"));
            return SP_SUCCESS;
      }

      SPTrace (3, ("SP_Process::reset_source_brokers (%s) source broker node %d (down node %d)\n",
                   process_params.sp_params.sourcebroker.name.c_str(),
                   process_params.sp_params.sourcebroker.node, pv_nid));

      if (process_params.sp_params.sourcebroker.node != pv_nid)
      {
            SPTrace (2, ("SP_Process::reset_source_brokers EXIT, no-op\n"));
            return SP_SUCCESS;
      }

      std::string lv_name = process_params.sp_params.sourcebroker.name;

      SPTrace (3, ("SP_Process::reset_source_brokers retrieving backup for %s.\n",
                       process_params.sp_params.sourcebroker.name.c_str()));

     // get backup node,
     lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
                 (char *) process_params.sp_params.sourcebroker.name.c_str(),
                 (char *) PROCESSINFO, lv_value);

     if (lv_error)
     {
         SPTrace (1, ("SP_Process::reset_source_brokers EXIT retrieving PROCESSINFO "
                      "for backup for %s on node %d failed with error %d.\n",
                      process_params.sp_params.sourcebroker.name.c_str(),
                      process_params.sp_params.sourcebroker.node, lv_error));
         return lv_error;
     }
     if (!sp_find_and_set_key_int (lv_value, "BACKUP", lp_delim, lv_backup))
     {
         SPTrace (1, ("SP_Process::reset_source_broker EXIT retrieving BACKUP "
                      "for backup for %s on node %d failed with error %d.\n",
                       process_params.sp_params.sourcebroker.name.c_str(),
                       process_params.sp_params.sourcebroker.node, SP_BACKUP_ERROR));
                   return SP_BACKUP_ERROR;
      }

       // if the backup node is down, we cannot survive a double failure
       if (!gv_nodes.is_up(lv_backup))
       {
            LOG_AND_TRACE (1, ("SP_Process::reset_source_broker EXIT double failure for%s on node %d.\n",
                         process_params.sp_params.sourcebroker.name.c_str(),
                         process_params.sp_params.sourcebroker.node));
            return SP_BACKUP_ERROR;
       }

      // now get the new IP associated with the backup node
      std::string lv_ip;
      lv_error = sp_get_ip_from_registry (lv_backup, lv_ip);
      if (!lv_error)
      {
          SPTrace (3, ("SP_Process::reset_source_broker retrieved IP of %s for backup node %d\n",
                       lv_ip.c_str(), lv_backup));
          process_params.sp_params.sourcebroker.ip = lv_ip;

          // change process name to reflect new node number
          std::string lv_temp = sp_replace_node_num (process_params.sp_params.sourcebroker.node, lv_backup,
                                process_params.sp_params.sourcebroker.name);
          if (lv_temp.empty())
          {
              SPTrace (1, ("SP_Process::reset_source_broker EXIT unable to replace backup name\n"));
              return SP_BACKUP_ERROR;
          }
          process_params.sp_params.sourcebroker.name = lv_temp;
          process_params.sp_params.sourcebroker.node = lv_backup;
       }
       else
       {
           SPTrace (1, ("SP_Process::reset_source_broker EXIT with error %d\n",
                        SP_NOT_FOUND));
           return SP_NOT_FOUND;
       }

      SPTrace (3, ("SP_Process::reset_source_broker reset params\n"));

      lv_error = set_specific_params();
      SPTrace (1, ("SP_Process::reset_source_broker EXIT\n"));

      return SP_SUCCESS;

}

//--------------------------------------------------------------------
//
// SP_Process::reset_source_brokers_up
// Purpos : reset our source brokers if a primary node comes back
//
// -------------------------------------------------------------------
int SP_Process::reset_source_brokers_up(int pv_nid)
{
      std::string lp_delim = " ";
      int         lv_error = SP_SUCCESS;
      std::string lv_value;

      SPTrace (2, ("SP_Process::reset_source_brokers_up ENTRY (%s)\n", iv_name.c_str()));

      if (process_params.sp_params.sourcebroker.name.empty())
      {
            SPTrace (2, ("SP_Process::reset_source_brokers_up EXIT, no-op\n"));
            return SP_SUCCESS;
      }

      SPTrace (3, ("SP_Process::reset_source_brokers_up (%s) source broker node %d (up node %d)\n",
                   process_params.sp_params.sourcebroker.name.c_str(),
                   process_params.sp_params.sourcebroker.node, pv_nid));

      if (process_params.sp_params.sourcebroker.primary_node != pv_nid)
      {
            SPTrace (2, ("SP_Process::reset_source_brokers_up EXIT, no-op\n"));
            return SP_SUCCESS;
      }

      // update info and ip
      lv_error = set_source_broker();
      if (!lv_error)
          lv_error = set_specific_params();
      SPTrace (1, ("SP_Process::reset_source_brokers_up EXIT\n"));

      return SP_SUCCESS;

}

// ----------------------------------------------------------------------------
//
// SP_Process::reset_node - REWORK for REINTEGRATION
// Purpose : reset node information, includes name
// ----------------------------------------------------------------------------
void SP_Process::reset_node (bool pv_backup)
{
    SPTrace (2, ("SP_Process::reset_node node ENTRY (%d, %d), backup (%d, %d)\n",
                 connection_info.node, iv_nid, iv_backup, pv_backup));

    std::string lv_temp;
    int         lv_node;

    if (pv_backup == false)
    {
       lv_node = iv_nid;
       lv_temp = sp_replace_node_num (process_info.node, iv_nid, iv_name);
    }
    else
    {
        lv_node = process_info.primary_node;
        lv_temp = sp_replace_node_num (iv_nid, process_info.primary_node,iv_name);
     }

    if (lv_temp.empty())
    {
        SPTrace (2, ("SP_Process::reset_node unable to set backup name.\n"));
    }
    else
    {
        iv_name = lv_temp;
    }

    connection_info.node = lv_node;
    process_info.node = lv_node;
    iv_backup = pv_backup;
    SPTrace (2, ("SP_Process::reset_node EXIT.\n"));
}
