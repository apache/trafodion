#region Copyright info
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
//
#endregion Copyright info

using System.Collections;

namespace Trafodion.Manager.LiveFeedFramework.Models
{
    /// <summary>
    /// This is static Routing Key, Publication Key mapper.  The assumtion is that routing key and publication key 
    /// is one-to-one mapping.  There are two hash maps so that you could find the publication key using the routing key and vise versa. 
    /// [TBD] Should this be a dynamic mapper so that all of the routing keys can be extracted out
    ///       of publication folder 
    /// </summary>
    public class LiveFeedRoutingKeyMapper
    {
        #region Known Publications 

        /// <summary>
        /// These keys are internally known to TrafodionManager. 
        /// </summary>
        //public const string PUBS_all = "all";
        public const string PUBS_common_text_event = "common.text_event";

        public const string PUBS_performance_core_metrics_asm = "performance.core_metrics";
        public const string PUBS_performance_disk_metrics_asm = "performance.disk_metrics";
        public const string PUBS_performance_tse_metrics_asm = "performance.tse_metrics";
        public const string PUBS_performance_filesys_metrics_asm = "performance.filesys_metrics";
        public const string PUBS_performance_loadavg_metrics_asm = "performance.loadavg_metrics";
        public const string PUBS_performance_memory_metrics_asm = "performance.memory_metrics";
        public const string PUBS_performance_network_metrics_asm = "performance.network_metrics";
        public const string PUBS_performance_virtualmem_metrics_asm = "performance.virtualmem_metrics";

        public const string PUBS_health_state_access_layer = "health.accesslayer_check";
        public const string PUBS_health_state_database_layer = "health.databaselayer_check";
        public const string PUBS_health_state_foundation_layer = "health.foundationlayer_check";
        public const string PUBS_health_state_os_layer = "health.oslayer_check";
        public const string PUBS_health_state_server_layer = "health.serverlayer_check";
        public const string PUBS_health_state_storage_layer = "health.storagelayer_check";

        public const string PUBS_problem_management_problem_open = "problem_management.problem_open";
        public const string PUBS_problem_management_problem_close = "problem_management.problem_close";

        #endregion Known Publications

        /// <summary>
        /// These keys are publically known to all LiveFeed parties.
        /// </summary>
        #region Known Routing Keys

        public const string RKEY_common_text_event = "event.common.instance.public.gpb.text_event";

        public const string RKEY_performance_core_metrics_asm = "performance_stat.linuxcounters.instance.public.gpb.core_metrics_shortbusy_assembled";
        public const string RKEY_performance_disk_metrics_asm = "performance_stat.linuxcounters.instance.public.gpb.disk_metrics_shortbusy_assembled";
        public const string RKEY_performance_tse_metrics_asm = "performance_stat.se.instance.public.gpb.perf_stats_delta_assembled";
        public const string RKEY_performance_filesys_metrics_asm = "performance_stat.linuxcounters.instance.public.gpb.filesystem_metrics_shortbusy_assembled";
        public const string RKEY_performance_loadavg_metrics_asm = "performance_stat.linuxcounters.instance.public.gpb.loadavg_metrics_shortbusy_assembled";
        public const string RKEY_performance_memory_metrics_asm = "performance_stat.linuxcounters.instance.public.gpb.memory_metrics_shortbusy_assembled";
        public const string RKEY_performance_network_metrics_asm = "performance_stat.linuxcounters.instance.public.gpb.network_metrics_shortbusy_assembled";
        public const string RKEY_performance_virtualmem_metrics_asm = "performance_stat.linuxcounters.instance.public.gpb.virtualmem_metrics_shortbusy_assembled";

        public const string RKEY_health_state_access_layer = "health_state.accesslayer.instance.public.gpb.level_1_check";
        public const string RKEY_health_state_database_layer = "health_state.databaselayer.instance.public.gpb.level_1_check";
        public const string RKEY_health_state_foundation_layer = "health_state.foundationlayer.instance.public.gpb.level_1_check";
        public const string RKEY_health_state_os_layer = "health_state.oslayer.instance.public.gpb.level_1_check";
        public const string RKEY_health_state_server_layer = "health_state.serverlayer.instance.public.gpb.level_1_check";
        public const string RKEY_health_state_storage_layer = "health_state.storagelayer.instance.public.gpb.level_1_check";


        public const string RKEY_problem_management_problem_open = "health_state.problem_management.instance.public.gpb.problem_open";
        public const string RKEY_problem_management_problem_close = "health_state.problem_management.instance.public.gpb.problem_close";

        #endregion Known Routing Keys

        #region fields

        private static readonly Hashtable PubsToRoutingKeyMap = new Hashtable();
        private static readonly Hashtable RoutingKeyToPubsMap = new Hashtable();

        public static string[] AllPublicationNames = new string[] {
                LiveFeedRoutingKeyMapper.PUBS_common_text_event,
                LiveFeedRoutingKeyMapper.PUBS_performance_core_metrics_asm,
                LiveFeedRoutingKeyMapper.PUBS_performance_disk_metrics_asm,
                //LiveFeedRoutingKeyMapper.PUBS_performance_tse_metrics_asm,
                LiveFeedRoutingKeyMapper.PUBS_performance_filesys_metrics_asm,
                LiveFeedRoutingKeyMapper.PUBS_performance_loadavg_metrics_asm,
                LiveFeedRoutingKeyMapper.PUBS_performance_memory_metrics_asm,
                LiveFeedRoutingKeyMapper.PUBS_performance_network_metrics_asm,
                LiveFeedRoutingKeyMapper.PUBS_performance_virtualmem_metrics_asm,
                LiveFeedRoutingKeyMapper.PUBS_health_state_access_layer,
                LiveFeedRoutingKeyMapper.PUBS_health_state_database_layer,
                LiveFeedRoutingKeyMapper.PUBS_health_state_foundation_layer,
                LiveFeedRoutingKeyMapper.PUBS_health_state_os_layer,
                LiveFeedRoutingKeyMapper.PUBS_health_state_server_layer,
                LiveFeedRoutingKeyMapper.PUBS_health_state_storage_layer,
                LiveFeedRoutingKeyMapper.PUBS_problem_management_problem_open,
                LiveFeedRoutingKeyMapper.PUBS_problem_management_problem_close
            };

        #endregion fields

        #region Constructor 

        // Every publication should have an entry here. 
        static LiveFeedRoutingKeyMapper()
        {
            PubsToRoutingKeyMap.Add(PUBS_common_text_event, RKEY_common_text_event);

            PubsToRoutingKeyMap.Add(PUBS_performance_core_metrics_asm, RKEY_performance_core_metrics_asm);
            PubsToRoutingKeyMap.Add(PUBS_performance_disk_metrics_asm, RKEY_performance_disk_metrics_asm);
            PubsToRoutingKeyMap.Add(PUBS_performance_tse_metrics_asm, RKEY_performance_tse_metrics_asm);
            PubsToRoutingKeyMap.Add(PUBS_performance_filesys_metrics_asm, RKEY_performance_filesys_metrics_asm);
            PubsToRoutingKeyMap.Add(PUBS_performance_loadavg_metrics_asm, RKEY_performance_loadavg_metrics_asm);
            PubsToRoutingKeyMap.Add(PUBS_performance_memory_metrics_asm, RKEY_performance_memory_metrics_asm);
            PubsToRoutingKeyMap.Add(PUBS_performance_network_metrics_asm, RKEY_performance_network_metrics_asm);
            PubsToRoutingKeyMap.Add(PUBS_performance_virtualmem_metrics_asm, RKEY_performance_virtualmem_metrics_asm);

            PubsToRoutingKeyMap.Add(PUBS_health_state_access_layer, RKEY_health_state_access_layer);
            PubsToRoutingKeyMap.Add(PUBS_health_state_database_layer, RKEY_health_state_database_layer);
            PubsToRoutingKeyMap.Add(PUBS_health_state_foundation_layer, RKEY_health_state_foundation_layer);
            PubsToRoutingKeyMap.Add(PUBS_health_state_os_layer, RKEY_health_state_os_layer);
            PubsToRoutingKeyMap.Add(PUBS_health_state_server_layer, RKEY_health_state_server_layer);
            PubsToRoutingKeyMap.Add(PUBS_health_state_storage_layer, RKEY_health_state_storage_layer);

            PubsToRoutingKeyMap.Add(PUBS_problem_management_problem_open, RKEY_problem_management_problem_open);
            PubsToRoutingKeyMap.Add(PUBS_problem_management_problem_close, RKEY_problem_management_problem_close);

            RoutingKeyToPubsMap.Add(RKEY_common_text_event, PUBS_common_text_event);

            RoutingKeyToPubsMap.Add(RKEY_performance_core_metrics_asm, PUBS_performance_core_metrics_asm);
            RoutingKeyToPubsMap.Add(RKEY_performance_disk_metrics_asm, PUBS_performance_disk_metrics_asm);
            RoutingKeyToPubsMap.Add(RKEY_performance_tse_metrics_asm, PUBS_performance_tse_metrics_asm);
            RoutingKeyToPubsMap.Add(RKEY_performance_filesys_metrics_asm, PUBS_performance_filesys_metrics_asm);
            RoutingKeyToPubsMap.Add(RKEY_performance_loadavg_metrics_asm, PUBS_performance_loadavg_metrics_asm);
            RoutingKeyToPubsMap.Add(RKEY_performance_memory_metrics_asm, PUBS_performance_memory_metrics_asm);
            RoutingKeyToPubsMap.Add(RKEY_performance_network_metrics_asm, PUBS_performance_network_metrics_asm);
            RoutingKeyToPubsMap.Add(RKEY_performance_virtualmem_metrics_asm, PUBS_performance_virtualmem_metrics_asm);

            RoutingKeyToPubsMap.Add(RKEY_health_state_access_layer, PUBS_health_state_access_layer);
            RoutingKeyToPubsMap.Add(RKEY_health_state_database_layer, PUBS_health_state_database_layer);
            RoutingKeyToPubsMap.Add(RKEY_health_state_foundation_layer, PUBS_health_state_foundation_layer);
            RoutingKeyToPubsMap.Add(RKEY_health_state_os_layer, PUBS_health_state_os_layer);
            RoutingKeyToPubsMap.Add(RKEY_health_state_server_layer, PUBS_health_state_server_layer);
            RoutingKeyToPubsMap.Add(RKEY_health_state_storage_layer, PUBS_health_state_storage_layer);

            RoutingKeyToPubsMap.Add(RKEY_problem_management_problem_open, PUBS_problem_management_problem_open);
            RoutingKeyToPubsMap.Add(RKEY_problem_management_problem_close, PUBS_problem_management_problem_close);
        }

        #endregion Constructor

        #region Public methods

        /// <summary>
        /// To get the LiveFeed Routing key with the publication key.
        /// </summary>
        /// <param name="aPublication"></param>
        /// <returns></returns>
        public static string GetRoutingKey(string aPublication)
        {
            if (PubsToRoutingKeyMap.ContainsKey(aPublication))
            {
                return (string)PubsToRoutingKeyMap[aPublication];
            }
            else
            {
                return "";
            }
        }

        /// <summary>
        /// To get the publication key with a given routing key.
        /// </summary>
        /// <param name="aRoutingKey"></param>
        /// <returns></returns>
        public static string GetPublication(string aRoutingKey)
        {
            if (RoutingKeyToPubsMap.ContainsKey(aRoutingKey))
            {
                return (string)RoutingKeyToPubsMap[aRoutingKey];
            }
            else
            {
                return "";
            }
        }

        #endregion Public methods
    }
}
