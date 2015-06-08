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

using System;
using System.Data;
//using lighthouse;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.LiveFeedFramework.Models
{
    /// <summary>
    /// The class to transform a received LiveFeed stats from protobuf to a DataTable.
    /// </summary>
    public class LiveFeedStatsTransformer
    {
        #region Fields

        /// <summary>
        /// Table schema for Health & States Layer
        /// </summary>
        public enum HealthStatesLayerDataTableColumns
        {
            layer_name = 0,
            layer_current_score,
            layer_previous_score,
            layer_score_change_ts_lct,
            layer_score_change_ts_utc
        }

        /// <summary>
        /// Table schema for Health and States Subjects
        /// </summary>
        public enum HealthStatesSubjectDataTableColumns
        {
            layer_name = 0,
            subject_name, 
            subject_current_score,
            subject_previous_score,
            subject_score_change_ts_lct,
            subject_score_change_ts_utc
        }

        // Define trace sub area. 
        private const string TRACE_SUB_AREA_NAME = "LiveFeedStatsTransformer";

        #endregion Fields

        #region Public methods

        /// <summary>
        /// To Deserialize a Stats received in the Protobuf reader
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aReader"></param>
        /// <returns></returns>
        public static object Deserialize(string aPublication, System.IO.BinaryReader aReader, LiveFeedConnection aLiveFeedConnection)
        {
            object retn_packet = null;
            common.qpid_header header = null;

            switch (aPublication)
            {
                case LiveFeedRoutingKeyMapper.PUBS_common_text_event:
                    common.text_event text_event = ProtoBuf.Serializer.Deserialize<common.text_event>(aReader.BaseStream);
                    header = text_event.header.header.header;
                    retn_packet = text_event;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_core_metrics_asm:
                    linuxcounters.core_metrics_shortbusy_assembled core_metrics = ProtoBuf.Serializer.Deserialize<linuxcounters.core_metrics_shortbusy_assembled>(aReader.BaseStream);
                    header = core_metrics.header.header;
                    retn_packet = core_metrics;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_disk_metrics_asm:
                    linuxcounters.disk_metrics_shortbusy_assembled disk_metrics = ProtoBuf.Serializer.Deserialize<linuxcounters.disk_metrics_shortbusy_assembled>(aReader.BaseStream);
                    header = disk_metrics.header.header;
                    retn_packet = disk_metrics;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_tse_metrics_asm:
                    se.perf_stats_delta_assembled tse_metrics = ProtoBuf.Serializer.Deserialize<se.perf_stats_delta_assembled>(aReader.BaseStream);
                    header = tse_metrics.header.header;
                    retn_packet = tse_metrics;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_filesys_metrics_asm:
                    linuxcounters.filesystem_metrics_shortbusy_assembled filesys_metrics = ProtoBuf.Serializer.Deserialize<linuxcounters.filesystem_metrics_shortbusy_assembled>(aReader.BaseStream);
                    header = filesys_metrics.header.header;                    
                    retn_packet = filesys_metrics;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_loadavg_metrics_asm:
                    linuxcounters.loadavg_metrics_shortbusy_assembled loadavg_metrics = ProtoBuf.Serializer.Deserialize<linuxcounters.loadavg_metrics_shortbusy_assembled>(aReader.BaseStream);
                    header = loadavg_metrics.header.header;  
                    retn_packet = loadavg_metrics;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_memory_metrics_asm:
                    linuxcounters.memory_metrics_shortbusy_assembled memory_metrics = ProtoBuf.Serializer.Deserialize<linuxcounters.memory_metrics_shortbusy_assembled>(aReader.BaseStream);
                    header = memory_metrics.header.header;  
                    retn_packet = memory_metrics;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_network_metrics_asm:
                    linuxcounters.network_metrics_shortbusy_assembled network_metrics = ProtoBuf.Serializer.Deserialize<linuxcounters.network_metrics_shortbusy_assembled>(aReader.BaseStream);
                    header = network_metrics.header.header;  
                    retn_packet = network_metrics;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_virtualmem_metrics_asm:
                    linuxcounters.virtualmem_metrics_shortbusy_assembled virtualmem_metrics = ProtoBuf.Serializer.Deserialize<linuxcounters.virtualmem_metrics_shortbusy_assembled>(aReader.BaseStream);
                    header = virtualmem_metrics.header.header;  
                    retn_packet = virtualmem_metrics;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_access_layer:
                    accesslayer.level_1_check access_check = ProtoBuf.Serializer.Deserialize<accesslayer.level_1_check>(aReader.BaseStream);
                    header = access_check.checkinfo.header.header.header;  
                    retn_packet = access_check;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_database_layer:
                    databaselayer.level_1_check database_check = ProtoBuf.Serializer.Deserialize<databaselayer.level_1_check>(aReader.BaseStream);
                    header = database_check.checkinfo.header.header.header;  
                    retn_packet = database_check;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_foundation_layer:
                    foundationlayer.level_1_check foundation_check = ProtoBuf.Serializer.Deserialize<foundationlayer.level_1_check>(aReader.BaseStream);
                    header = foundation_check.checkinfo.header.header.header;  
                    retn_packet = foundation_check;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_os_layer:
                    oslayer.level_1_check os_check = ProtoBuf.Serializer.Deserialize<oslayer.level_1_check>(aReader.BaseStream);
                    header = os_check.checkinfo.header.header.header;  
                    retn_packet = os_check;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_server_layer:
                    serverlayer.level_1_check server_check = ProtoBuf.Serializer.Deserialize<serverlayer.level_1_check>(aReader.BaseStream);
                    header = server_check.checkinfo.header.header.header;  
                    retn_packet = server_check;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_storage_layer:
                    storagelayer.level_1_check storage_check = ProtoBuf.Serializer.Deserialize<storagelayer.level_1_check>(aReader.BaseStream);
                    header = storage_check.checkinfo.header.header.header;  
                    retn_packet = storage_check;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_problem_management_problem_open:
                    problem_management.problem_open problem_open=ProtoBuf.Serializer.Deserialize<problem_management.problem_open>(aReader.BaseStream);
                    header = problem_open.header.header;
                    retn_packet = problem_open;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_problem_management_problem_close:
                    problem_management.problem_close problem_close = ProtoBuf.Serializer.Deserialize<problem_management.problem_close>(aReader.BaseStream);
                    header = problem_close.header.header;
                    retn_packet = problem_close;
                    break;
            }

            if (retn_packet != null)
            {
                RetrievePlatformReleaseVersion(aLiveFeedConnection, header);
            }

            // An unknow publication; this should never happen
            return retn_packet;
        }

        /// <summary>
        /// To transform a de-serialized stats (in the form of a protobuf class) to a DataTable
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="stats"></param>
        /// <returns></returns>
        public static DataTable TransformStatsToDataTable(string aPublication, object aStats)
        {
            DataTable table = CreateDataTableSchema(aPublication, aStats);
            FillData(aPublication, table, aStats);
            return table;
        }

        /// <summary>
        /// To transform a de-serialized stats to an existing DataTable.
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="stats"></param>
        /// <param name="dt"></param>
        public static void TransformStatsToDataTable(string aPublication, object aStats, DataTable aDataTable)
        {
            FillData(aPublication, aDataTable, aStats);
        }

        /// <summary>
        /// Return the health states layer data table
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aStats"></param>
        /// <returns></returns>
        public static DataTable TransformHealthStatesProtobufToLayerDataTable(string aPublication, object aStats)
        {
            TraceHealthLayerCheckData(aPublication, aStats);

            DataTable table = new DataTable();
            table.TableName = aPublication;
            table.Columns.Add(HealthStatesLayerDataTableColumns.layer_name.ToString(), typeof(string));
            table.Columns.Add(HealthStatesLayerDataTableColumns.layer_current_score.ToString(), typeof(int));
            table.Columns.Add(HealthStatesLayerDataTableColumns.layer_previous_score.ToString(), typeof(int));
            table.Columns.Add(HealthStatesLayerDataTableColumns.layer_score_change_ts_lct.ToString(), typeof(long));
            table.Columns.Add(HealthStatesLayerDataTableColumns.layer_score_change_ts_utc.ToString(), typeof(long));

            health.layer_check health_layer_check = GetHealthLayerCheck(aPublication, aStats);

            DataRow dr = table.NewRow();
            // Forget about header, go directly to data
            dr[HealthStatesLayerDataTableColumns.layer_name.ToString()] = health_layer_check.layer_name;
            dr[HealthStatesLayerDataTableColumns.layer_current_score.ToString()] = health_layer_check.layer_current_score;
            dr[HealthStatesLayerDataTableColumns.layer_previous_score.ToString()] = health_layer_check.layer_previous_score;
            dr[HealthStatesLayerDataTableColumns.layer_score_change_ts_lct.ToString()] = health_layer_check.layer_score_change_ts_lct;
            dr[HealthStatesLayerDataTableColumns.layer_score_change_ts_utc.ToString()] = health_layer_check.layer_score_change_ts_utc;
            table.Rows.Add(dr);

            return table;
        }

        /// <summary>
        /// Return the health states layer data table
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aStats"></param>
        /// <returns></returns>
        public static DataTable TransformHealthStatesProtobufToSubjectDataTable(string aPublication, object aStats, out DateTime aCurrentTS)
        {
            TraceHealthLayerCheckData(aPublication, aStats);

            DataTable table = new DataTable();
            table.TableName = aPublication;
            table.Columns.Add(HealthStatesSubjectDataTableColumns.layer_name.ToString(), typeof(string));
            table.Columns.Add(HealthStatesSubjectDataTableColumns.subject_name.ToString(), typeof(string));
            table.Columns.Add(HealthStatesSubjectDataTableColumns.subject_current_score.ToString(), typeof(int));
            table.Columns.Add(HealthStatesSubjectDataTableColumns.subject_previous_score.ToString(), typeof(int));
            table.Columns.Add(HealthStatesSubjectDataTableColumns.subject_score_change_ts_lct.ToString(), typeof(long));
            table.Columns.Add(HealthStatesSubjectDataTableColumns.subject_score_change_ts_utc.ToString(), typeof(long));

            health.layer_check health_layer_check = GetHealthLayerCheck(aPublication, aStats);
            health.layer_check.health_layer_data_struct data_struct = new health.layer_check.health_layer_data_struct();
            for (int i = 0; i < health_layer_check.data.Count; i++)
            {
                DataRow dr = table.NewRow();
                // Forget about header, go directly to data
                dr[HealthStatesSubjectDataTableColumns.layer_name.ToString()] = health_layer_check.layer_name;
                dr[HealthStatesSubjectDataTableColumns.subject_name.ToString()] = health_layer_check.data[i].subject_name;
                dr[HealthStatesSubjectDataTableColumns.subject_current_score.ToString()] = health_layer_check.data[i].subject_current_score;
                dr[HealthStatesSubjectDataTableColumns.subject_previous_score.ToString()] = health_layer_check.data[i].subject_previous_score;
                dr[HealthStatesSubjectDataTableColumns.subject_score_change_ts_lct.ToString()] = health_layer_check.data[i].subject_score_change_ts_lct;
                //dr[HealthStatesSubjectDataTableColumns.subject_score_change_ts_utc.ToString()] = health_layer_check.data[i].subject_score_change_ts_utc;
                table.Rows.Add(dr);
            }

            aCurrentTS = Utilities.GetFormattedTimeFromUnixTimestampLCT((long)health_layer_check.header.header.info_generation_time_ts_lct);
            return table;
        }

        #endregion Public methods

        #region Private methods


        private static void RetrievePlatformReleaseVersion(LiveFeedConnection aLiveFeedConnection, common.qpid_header header)
        {
            if (aLiveFeedConnection != null &&
                aLiveFeedConnection.ConnectionDefn != null && 
                string.IsNullOrEmpty(aLiveFeedConnection.ConnectionDefn.PlatformReleaseVersion))
            {
                string qpid_system_version = header.system_version;
                int id = qpid_system_version.IndexOf("Release");
                if (id >= 0)
                {
                    aLiveFeedConnection.ConnectionDefn.PlatformReleaseVersion = qpid_system_version.Substring(id + 8, 5);
                    aLiveFeedConnection.ConnectionDefn.PlatformReleaseVersionString = qpid_system_version.Substring(id);
                }
                else
                {
                    aLiveFeedConnection.ConnectionDefn.PlatformReleaseVersion = 
                    aLiveFeedConnection.ConnectionDefn.PlatformReleaseVersionString = Trafodion.Manager.Framework.Connections.ConnectionDefinition.DefaultPlatformVersion;
                }
            }
        }

        /// <summary>
        /// Using reflection to inspect a stats' properties.  Then, create a DataTable schema with these 
        /// properties (i.e. the DataTable's column definitions).
        /// [Note]: Examine proto files to see if there are anymore headers to be examined/added in this method.
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="anObjectToInspect"></param>
        /// <returns></returns>
        private static DataTable CreateDataTableSchema(string aPublication, object anObjectToInspect)
        {
            DataTable table = new DataTable();
            table.TableName = aPublication;

            switch (aPublication)
            {
                case LiveFeedRoutingKeyMapper.PUBS_common_text_event:
                    AddEventHeader(table);
                    AddEventColumns(table);
                    return table;

                case LiveFeedRoutingKeyMapper.PUBS_performance_core_metrics_asm:
                    AddInfoHeader(table);
                    AddCoreMetricsColumns(table);
                    return table;

                case LiveFeedRoutingKeyMapper.PUBS_performance_disk_metrics_asm:
                    AddInfoHeader(table);
                    AddDiskMetricsColumns(table);
                    return table;

                case LiveFeedRoutingKeyMapper.PUBS_performance_tse_metrics_asm:
                    AddInfoHeader(table);
                    AddTseMetricsColumns(table);
                    return table;

                case LiveFeedRoutingKeyMapper.PUBS_performance_filesys_metrics_asm:
                    AddInfoHeader(table);
                    AddFileSysMetricsColumns(table);
                    return table;

                case LiveFeedRoutingKeyMapper.PUBS_performance_loadavg_metrics_asm:
                    AddInfoHeader(table);
                    AddLoadAvgMetricsColumns(table);
                    return table;

                case LiveFeedRoutingKeyMapper.PUBS_performance_memory_metrics_asm:
                    AddInfoHeader(table);
                    AddMemoryMetricsColumns(table);
                    return table;

                case LiveFeedRoutingKeyMapper.PUBS_performance_network_metrics_asm:
                    AddInfoHeader(table);
                    AddNetworkMetricsColumns(table);
                    return table;

                case LiveFeedRoutingKeyMapper.PUBS_performance_virtualmem_metrics_asm:
                    AddInfoHeader(table);
                    AddVirtualmemMetricsColumns(table);
                    return table;

                case LiveFeedRoutingKeyMapper.PUBS_problem_management_problem_open:
                case LiveFeedRoutingKeyMapper.PUBS_problem_management_problem_close:
                    AddInfoHeader(table);
                    AddProblemColumns(table);
                    return table;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_access_layer:
                case LiveFeedRoutingKeyMapper.PUBS_health_state_database_layer:
                case LiveFeedRoutingKeyMapper.PUBS_health_state_foundation_layer:
                case LiveFeedRoutingKeyMapper.PUBS_health_state_os_layer:
                case LiveFeedRoutingKeyMapper.PUBS_health_state_server_layer:
                case LiveFeedRoutingKeyMapper.PUBS_health_state_storage_layer:
                    AddHealthLayerCheckColumns(table);
                    return table;
            }

            return table;
        }

        /// <summary>
        /// Add event header's fields to the given DataTable's column definitions.
        /// [Note]: Examine proto files to see if the event header has any other headers (structs).
        /// </summary>
        /// <param name="aDataTable"></param>
        static private void AddEventHeader(DataTable aDataTable)
        {
            //Inspect a event header
            common.event_header event_header = new common.event_header();
            Type objectType = event_header.GetType();
            System.Reflection.PropertyInfo[] properties = objectType.GetProperties();

            //You can then use the PropertyInfo array to loop through each property of the type.
            foreach (System.Reflection.PropertyInfo property in properties)
            {
                if (property.PropertyType.FullName.Equals("common.info_header", StringComparison.OrdinalIgnoreCase))
                {
                    //Event header contains a info header
                    AddInfoHeader(aDataTable);
                }
                else if (property.PropertyType.FullName.StartsWith("System.", StringComparison.OrdinalIgnoreCase))
                {
                    //Basic types, add it to the column defnitions.
                    aDataTable.Columns.Add("event_header_" + property.Name, property.PropertyType);
                }
            }
        }

        /// <summary>
        /// Add info header's fields to the given DataTable's column definitions.
        /// [Note]: Examine proto files to see if the info header has anymore headers or structs.
        /// </summary>
        /// <param name="aDataTable"></param>
        static private void AddInfoHeader(DataTable aDataTable)
        {
            //Inspect a info header
            common.info_header info_header = new common.info_header();
            Type objectType = info_header.GetType();
            System.Reflection.PropertyInfo[] properties = objectType.GetProperties();

            //You can then use the PropertyInfo array to loop through each property of the type.
            foreach (System.Reflection.PropertyInfo property in properties)
            {
                if (property.PropertyType.FullName.Equals("common.qpid_header", StringComparison.OrdinalIgnoreCase))
                {
                    //Info header contains a Qpid header first
                    AddQpidHeader(aDataTable);
                }
                else if (property.PropertyType.FullName.StartsWith("System.", StringComparison.OrdinalIgnoreCase))
                {
                    //Basic types, add it to the column definitions.
                    aDataTable.Columns.Add("info_header_" + property.Name, property.PropertyType);
                }
            }
        }

        /// <summary>
        /// Add Qpid header's fields to the given DataTable's column definitions.
        /// [Note]: Examine proto files to see if the Qpid header contains any headers or structs.
        /// </summary>
        /// <param name="aDataTable"></param>
        static private void AddQpidHeader(DataTable aDataTable)
        {
            //Inspect Qpid header
            common.qpid_header qpid_header = new common.qpid_header();
            Type objectType = qpid_header.GetType();
            System.Reflection.PropertyInfo[] properties = objectType.GetProperties();

            //You can then use the PropertyInfo array to loop through each property of the type.
            foreach (System.Reflection.PropertyInfo property in properties)
            {
                aDataTable.Columns.Add("qpid_header_" + property.Name, property.PropertyType);
            }
        }

        /// <summary>
        /// Add Health layer_check datatable column definitions. 
        /// </summary>
        /// <param name="aDataTable"></param>
        static private void AddHealthLayerCheckColumns(DataTable aDataTable)
        {
            health.layer_check layer_check = new health.layer_check();
            Type objectType = layer_check.GetType();
            System.Reflection.PropertyInfo[] properties = objectType.GetProperties();

            foreach (System.Reflection.PropertyInfo property in properties)
            {
                if (property.PropertyType.FullName.Equals("common.health_header", StringComparison.OrdinalIgnoreCase))
                {
                    //health header contains a info header
                    AddHealthHeader(aDataTable);
                }
                else if (property.PropertyType.FullName.StartsWith("System.", StringComparison.OrdinalIgnoreCase) && 
                         !property.PropertyType.FullName.StartsWith("System.Collections", StringComparison.OrdinalIgnoreCase))
                {
                    //Basic types, add it to the column defnitions.
                    aDataTable.Columns.Add(property.Name, property.PropertyType);
                }
                else if (property.PropertyType.FullName.StartsWith("System.Collections", StringComparison.OrdinalIgnoreCase))
                {
                    //This must be health.layer_check.health_layer_data_struct
                    AddHealthLayerDataStruct(aDataTable);
                }
            }
        }

        /// <summary>
        /// Add Health header. 
        /// </summary>
        /// <param name="aDataTable"></param>
        static private void AddHealthHeader(DataTable aDataTable)
        {
            //Inspect Health header
            common.health_header health_header = new common.health_header();
            Type objectType = health_header.GetType();
            System.Reflection.PropertyInfo[] properties = objectType.GetProperties();

            //Now, use the PropertyInfo array to walk through each property of the type.
            foreach (System.Reflection.PropertyInfo property in properties)
            {
                if (property.PropertyType.FullName.Equals("common.info_header", StringComparison.OrdinalIgnoreCase))
                {
                    //health header contains a info header
                    AddInfoHeader(aDataTable);
                }
                else if (property.PropertyType.FullName.StartsWith("System.", StringComparison.OrdinalIgnoreCase))
                {
                    //Basic types, add it to the column defnitions.
                    aDataTable.Columns.Add("health_header_" + property.Name, property.PropertyType);
                }
            }
        }

        /// <summary>
        /// Add health layer's sub data struct for subjects
        /// </summary>
        /// <param name="aDataTable"></param>
        static private void AddHealthLayerDataStruct(DataTable aDataTable)
        {
            health.layer_check.health_layer_data_struct layer_data = new health.layer_check.health_layer_data_struct();
            Type objectType = layer_data.GetType();
            System.Reflection.PropertyInfo[] properties = objectType.GetProperties();

            foreach (System.Reflection.PropertyInfo property in properties)
            {
                aDataTable.Columns.Add("data_struct_" + property.Name, property.PropertyType);
            }
        }

        #region common.text_event

        /// <summary>
        /// Add event buffer fields to the given DataTable's column definitions.
        /// [Note]: Examine proto files to see if the event buffer contains any other new headers
        /// </summary>
        /// <param name="aDataTable"></param>
        static private void AddEventColumns(DataTable aDataTable)
        {
            common.text_event text_event = new common.text_event();
            System.Reflection.PropertyInfo[] properties = text_event.GetType().GetProperties();
            foreach (System.Reflection.PropertyInfo property in properties)
            {
                // not to add header again
                if (property.PropertyType.FullName.StartsWith("System.", StringComparison.OrdinalIgnoreCase))
                {
                    //Basic types, add it as a column
                    aDataTable.Columns.Add(property.Name, property.PropertyType);
                }
            }
        }

        /// <summary>
        /// copy data values from protobuf to cells in data table.
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aStats"></param>
        static private void FillEventData(string aPublication, DataTable aDataTable, object aStats)
        {
            common.text_event text_event = aStats as common.text_event;
            if (text_event != null)
            {
                DataRow dr = aDataTable.NewRow();
                FillEventHeaderData(aPublication, dr, text_event.header);

                System.Reflection.PropertyInfo[] properties = text_event.GetType().GetProperties();
                foreach (System.Reflection.PropertyInfo property in properties)
                {
                    // not to add header again
                    if (property.PropertyType.FullName.StartsWith("System.", StringComparison.OrdinalIgnoreCase))
                    {
                        //Basic types, add it as a column
                        dr[property.Name] = property.GetValue(text_event, null);
                    }
                }
                aDataTable.Rows.Add(dr);
            }
        }

        #endregion common.text_event

        #region performance.core_metrics

        /// <summary>
        /// Add core_metrics schemas
        /// </summary>
        /// <param name="aDataTable"></param>
        static private void AddCoreMetricsColumns(DataTable aDataTable)
        {
            linuxcounters.core_metrics_shortbusy_assembled.aggregate_core_buffer buffer = new linuxcounters.core_metrics_shortbusy_assembled.aggregate_core_buffer();
            System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();

            //You can then use the PropertyInfo array to loop through each property of the type.
            foreach (System.Reflection.PropertyInfo property in buffer_properties)
            {
                aDataTable.Columns.Add("core_metrics_" + property.Name, property.PropertyType);
            }

        }

        /// <summary>
        /// copy data values from protobuf to cells in data table.
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aStats"></param>
        static private void FillCoreMetricsData(string aPublication, DataTable aDataTable, object aStats)
        {
            linuxcounters.core_metrics_shortbusy_assembled metrics = aStats as linuxcounters.core_metrics_shortbusy_assembled;
            if (metrics != null)
            {
                linuxcounters.core_metrics_shortbusy_assembled.aggregate_core_buffer buffer = new linuxcounters.core_metrics_shortbusy_assembled.aggregate_core_buffer();
                System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();
                int core_idx = 0;

                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    string traceMessage = string.Format("Received publcation [{0}]: aggregate = {1} {2}",
                                                  LiveFeedRoutingKeyMapper.GetRoutingKey(aPublication),
                                                  metrics.aggregate.Count,
                                                  Environment.NewLine);

                    for (int i = 0; i < metrics.aggregate.Count; i++)
                    {
                        traceMessage += string.Format(" aggregate[{0}]:", i);
                        foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                        {
                            try
                            {
                                traceMessage += string.Format(" {0} = {1},",
                                                              b_property.Name,
                                                              b_property.GetValue(metrics.aggregate[i], null));
                            }
                            catch (Exception ex)
                            {
                                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                   String.Format("Received core metric publication: property [{0}] get value exception: {1}",
                                                                 b_property.Name, ex.Message));
                            }
                        }
                        traceMessage += Environment.NewLine;
                    }

                    Logger.OutputToLog(TraceOptions.TraceOption.LIVEFEEDDATA, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, traceMessage);
                }

                // fill up one node at a time.
                for (int i = 0; i < metrics.aggregate.Count; i++)
                {
                    DataRow dr = aDataTable.NewRow();
                    FillInfoHeaderData(aPublication, dr, metrics.header);

                    // add the aggr fields now
                    foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                    {
                        try
                        {
                            if (b_property.Name.Equals("node_id"))
                            {
                                dr["core_metrics_" + b_property.Name] = i;
                            }
                            else
                            {
                                dr["core_metrics_" + b_property.Name] = b_property.GetValue(metrics.aggregate[i], null);
                            }
                        }
                        catch (Exception ex)
                        {
                            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                               String.Format("Received core metric publication: property [{0}] get value exception: {1}",
                                                             b_property.Name, ex.Message));
                        }
                    }
                    aDataTable.Rows.Add(dr);
                }
            }
        }

        #endregion performance.core_metrics

        #region performance.disk_metrics

        /// <summary>
        /// Add disk metrics schemas
        /// </summary>
        /// <param name="aDataTable"></param>
        static private void AddDiskMetricsColumns(DataTable aDataTable)
        {            
            linuxcounters.disk_metrics_shortbusy_assembled.aggregate_diskios_buffer buffer = new linuxcounters.disk_metrics_shortbusy_assembled.aggregate_diskios_buffer();
            System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();

            //You can then use the PropertyInfo array to loop through each property of the type.
            foreach (System.Reflection.PropertyInfo property in buffer_properties)
            {
                aDataTable.Columns.Add("disk_metrics_" + property.Name, property.PropertyType);
            }
        }

        /// <summary>
        /// copy data values from protobuf to cells in data table.
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aStats"></param>
        static private void FillDiskMetricsData(string aPublication, DataTable aDataTable, object aStats)
        {
            linuxcounters.disk_metrics_shortbusy_assembled metrics = aStats as linuxcounters.disk_metrics_shortbusy_assembled;
            if (metrics != null)
            {
                linuxcounters.disk_metrics_shortbusy_assembled.aggregate_diskios_buffer buffer = new linuxcounters.disk_metrics_shortbusy_assembled.aggregate_diskios_buffer();
                System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();
                int core_idx = 0;

                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    string traceMessage = string.Format("Received publcation [{0}]: aggregate = {1} {2}",
                                                  LiveFeedRoutingKeyMapper.GetRoutingKey(aPublication),
                                                  metrics.aggregate.Count,
                                                  Environment.NewLine);

                    for (int i = 0; i < metrics.aggregate.Count; i++)
                    {
                        traceMessage += string.Format(" aggregate[{0}]:", i);
                        foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                        {
                            try
                            {
                                traceMessage += string.Format(" {0} = {1},",
                                                              b_property.Name,
                                                              b_property.GetValue(metrics.aggregate[i], null));
                            }
                            catch (Exception ex)
                            {
                                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                   String.Format("Received core metric publication: property [{0}] get value exception: {1}",
                                                                 b_property.Name, ex.Message));
                            }
                        }
                        traceMessage += Environment.NewLine;
                    }

                    Logger.OutputToLog(TraceOptions.TraceOption.LIVEFEEDDATA, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, traceMessage);
                }

                // fill up one node at a time.
                for (int i = 0; i < metrics.aggregate.Count; i++)
                {
                    DataRow dr = aDataTable.NewRow();
                    FillInfoHeaderData(aPublication, dr, metrics.header);

                    // add the aggr fields now
                    foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                    {
                        try
                        {
                            if (b_property.Name.Equals("node_id"))
                            {
                                dr["disk_metrics_" + b_property.Name] = i;
                            }
                            else
                            {
                                dr["disk_metrics_" + b_property.Name] = b_property.GetValue(metrics.aggregate[i], null);
                            }
                        }
                        catch (Exception ex)
                        {
                            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                               String.Format("Received disk metric publication: property [{0}] get value exception: {1}",
                                                             b_property.Name, ex.Message));
                        }
                    }
                    aDataTable.Rows.Add(dr);
                }
            }            
        }
        
        #endregion performance.disk_metrics

        #region tse metric


        /// <summary>
        /// Add TSE metrics schemas
        /// </summary>
        /// <param name="aDataTable"></param>
        static private void AddTseMetricsColumns(DataTable aDataTable)
        {
            se.perf_stats_delta_assembled.device_delta_data buffer = new se.perf_stats_delta_assembled.device_delta_data();
            System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();

            //You can then use the PropertyInfo array to loop through each property of the type.
            foreach (System.Reflection.PropertyInfo property in buffer_properties)
            {
                aDataTable.Columns.Add("tse_metrics_" + property.Name, property.PropertyType);
            }
        }
        
        /// <summary>
        /// copy data values from protobuf to cells in data table.
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aStats"></param>
        static private void FillTseMetricsData(string aPublication, DataTable aDataTable, object aStats)
        {
            se.perf_stats_delta_assembled metrics = aStats as se.perf_stats_delta_assembled;
            if (metrics != null)
            {
                se.perf_stats_delta_assembled.device_delta_data buffer = new se.perf_stats_delta_assembled.device_delta_data();
                System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();
                int core_idx = 0;

                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    string traceMessage = string.Format("Received publcation [{0}]: data = {1} {2}",
                                                  LiveFeedRoutingKeyMapper.GetRoutingKey(aPublication),
                                                  metrics.data.Count,
                                                  Environment.NewLine);

                    for (int i = 0; i < metrics.data.Count; i++)
                    {
                        traceMessage += string.Format(" aggregate[{0}]:", i);
                        foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                        {
                            try
                            {
                                traceMessage += string.Format(" {0} = {1},",
                                                              b_property.Name,
                                                              b_property.GetValue(metrics.data[i], null));
                            }
                            catch (Exception ex)
                            {
                                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                   String.Format("Received TSE metric publication: property [{0}] get value exception: {1}",
                                                                 b_property.Name, ex.Message));
                            }
                        }
                        traceMessage += Environment.NewLine;
                    }

                    Logger.OutputToLog(TraceOptions.TraceOption.LIVEFEEDDATA, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, traceMessage);
                }

                // fill up one node at a time.
                for (int i = 0; i < metrics.data.Count; i++)
                {
                    DataRow dr = aDataTable.NewRow();
                    FillInfoHeaderData(aPublication, dr, metrics.header);

                    // add the aggr fields now
                    foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                    {
                        try
                        {
                            if (b_property.Name.Equals("node_id"))
                            {
                                dr["disk_metrics_" + b_property.Name] = i;
                            }
                            else
                            {
                                dr["disk_metrics_" + b_property.Name] = b_property.GetValue(metrics.data[i], null);
                            }
                        }
                        catch (Exception ex)
                        {
                            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                               String.Format("Received disk metric publication: property [{0}] get value exception: {1}",
                                                             b_property.Name, ex.Message));
                        }
                    }
                    aDataTable.Rows.Add(dr);
                }
            }
        }

        #endregion

        #region performance.filesys_metrics

        /// <summary>
        /// Add filesys metrics schemas
        /// </summary>
        /// <param name="aDataTable"></param>
        static private void AddFileSysMetricsColumns(DataTable aDataTable)
        {
            //Create columns
            linuxcounters.filesystem_metrics_shortbusy_assembled.aggregate_filesystem_count filesys_count = new linuxcounters.filesystem_metrics_shortbusy_assembled.aggregate_filesystem_count();
            linuxcounters.filesystem_metrics_shortbusy_assembled.aggregate_filesystem_use_buffer filesys_buffer = new linuxcounters.filesystem_metrics_shortbusy_assembled.aggregate_filesystem_use_buffer();
            System.Reflection.PropertyInfo[] count_properties = filesys_count.GetType().GetProperties();
            System.Reflection.PropertyInfo[] buffer_properties = filesys_buffer.GetType().GetProperties();

            //You can then use the PropertyInfo array to loop through each property of the type.
            foreach (System.Reflection.PropertyInfo property in count_properties)
            {
                aDataTable.Columns.Add("filesys_metrics_count_" + property.Name, property.PropertyType);
            }

            foreach (System.Reflection.PropertyInfo property in buffer_properties)
            {
                aDataTable.Columns.Add("filesys_metrics_" + property.Name, property.PropertyType);
            }
        }

        /// <summary>
        /// copy data values from protobuf to cells in data table.
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aStats"></param>
        static private void FillFileSysMetricsData(string aPublication, DataTable aDataTable, object aStats)
        {
            linuxcounters.filesystem_metrics_shortbusy_assembled metrics = aStats as linuxcounters.filesystem_metrics_shortbusy_assembled;
            if (metrics != null)
            {
                linuxcounters.filesystem_metrics_shortbusy_assembled.aggregate_filesystem_count filesys_count = new linuxcounters.filesystem_metrics_shortbusy_assembled.aggregate_filesystem_count();
                linuxcounters.filesystem_metrics_shortbusy_assembled.aggregate_filesystem_use_buffer filesys_buffer = new linuxcounters.filesystem_metrics_shortbusy_assembled.aggregate_filesystem_use_buffer();
                System.Reflection.PropertyInfo[] count_properties = filesys_count.GetType().GetProperties();
                System.Reflection.PropertyInfo[] buffer_properties = filesys_buffer.GetType().GetProperties();

                int core_idx = 0;
                int total_aggregates = metrics.aggregate.Count;

                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    string traceMessage = string.Format("Received publcation [{0}]: aggregate_count = {1}, aggregate = {2} {3}",
                                                  LiveFeedRoutingKeyMapper.GetRoutingKey(aPublication),
                                                  metrics.filesystem_count_aggr.Count,
                                                  metrics.aggregate.Count,
                                                  Environment.NewLine);

                    for (int i = 0; i < metrics.filesystem_count_aggr.Count; i++)
                    {
                        traceMessage += string.Format(" filesystem_count_aggr[{0}]:", i);
                        // add count properties first
                        foreach (System.Reflection.PropertyInfo c_property in count_properties)
                        {
                            try
                            {
                                traceMessage += string.Format(" {0} = {1},",
                                                              c_property.Name,
                                                              c_property.GetValue(metrics.filesystem_count_aggr[i], null));
                            }
                            catch (Exception ex)
                            {
                                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                   String.Format("Received filesystem metric publication: property [{0}] get value exception: {1}",
                                                                 c_property.Name, ex.Message));
                            }
                        }
                        traceMessage += Environment.NewLine;
                    }

                    //for (int i = 0; i < metrics.aggregate.Count; i++)
                    //{
                    //    traceMessage += string.Format(" aggregate[{0}]:", i);
                    //    foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                    //    {
                    //        try
                    //        {
                    //            traceMessage += string.Format(" {0} = {1},",
                    //                                          b_property.Name,
                    //                                          b_property.GetValue(metrics.aggregate[i], null));
                    //        }
                    //        catch (Exception ex)
                    //        {
                    //            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                    //                               String.Format("Received filesystem metric publication: property [{0}] get value exception: {1}",
                    //                                             b_property.Name, ex.Message));
                    //        }
                    //    }
                    //    traceMessage += Environment.NewLine;
                    //}

                    Logger.OutputToLog(TraceOptions.TraceOption.LIVEFEEDDATA, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, traceMessage);
                }

                // fill up one node at a time.
                for (int i = 0; i < metrics.filesystem_count_aggr.Count; i++)
                {
                    int num = metrics.filesystem_count_aggr[i].num_file_systems;
                    if (num > 0)
                    {
                        for (int j = 0; j < num; j++)
                        {
                            if ((core_idx + j) < total_aggregates)
                            {
                                DataRow dr = aDataTable.NewRow();
                                FillInfoHeaderData(aPublication, dr, metrics.header);

                                // add count properties first
                                foreach (System.Reflection.PropertyInfo c_property in count_properties)
                                {
                                    try
                                    {
                                        dr["filesys_metrics_count_" + c_property.Name] = c_property.GetValue(metrics.filesystem_count_aggr[i], null);
                                    }
                                    catch (Exception ex)
                                    {
                                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                           String.Format("Received filesystem metric publication: property [{0}] get value exception: {1}",
                                                                         c_property.Name, ex.Message));
                                    }
                                }

                                // add the aggr fields now
                                foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                                {
                                    try
                                    {
                                        dr["filesys_metrics_" + b_property.Name] = b_property.GetValue(metrics.aggregate[core_idx + j], null);
                                    }
                                    catch (Exception ex)
                                    {
                                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                           String.Format("Received filesystem metric publication: property [{0}] get value exception: {1}",
                                                                         b_property.Name, ex.Message));
                                    }
                                }
                                aDataTable.Rows.Add(dr);
                            }
                            else
                            {
                                // [TBD] should emit an exception, but now just return
                                return;
                            }
                        }
                        core_idx += num;
                    }
                    else
                    {
                        DataRow dr = aDataTable.NewRow();
                        FillInfoHeaderData(aPublication, dr, metrics.header);

                        // add count properties first
                        foreach (System.Reflection.PropertyInfo c_property in count_properties)
                        {
                            dr["filesys_metrics_count_" + c_property.Name] = 1;
                        }

                        // add the aggr fields now
                        foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                        {
                            if (b_property.Name.Equals("node_id"))
                            {
                                dr["filesys_metrics_" + b_property.Name] = i;
                            }
                            else
                            {
                                dr["filesys_metrics_" + b_property.Name] = num;
                            }
                        }
                        aDataTable.Rows.Add(dr);
                        if (num == -1)
                        {
                            // advance to the next only for -1.
                            core_idx += 1;
                        }
                    }
                }

                //[TBD]
                // At the end, the core_idx should be the same as aggregate count
                //if (core_idx != metrics.aggregate.Count)
                //{
                //    throw new Exception(string.Format("performance.filesys_metrics buffer counts mismatched: aggregate total={0}, buffer count={1}", core_idx, metrics.aggregate.Count));
                //}
            }
        }

        #endregion performance.disk_metrics

        #region performance.loadavg_metrics

        /// <summary>
        /// Add Load Avg metrics schemas
        /// </summary>
        /// <param name="aDataTable"></param>
        static private void AddLoadAvgMetricsColumns(DataTable aDataTable)
        {
            //Create columns
            linuxcounters.loadavg_metrics_shortbusy_assembled.aggregate_buffer buffer = new linuxcounters.loadavg_metrics_shortbusy_assembled.aggregate_buffer();
            System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();

            //You can then use the PropertyInfo array to loop through each property of the type.
            foreach (System.Reflection.PropertyInfo property in buffer_properties)
            {
                aDataTable.Columns.Add("loadavg_metrics_" + property.Name, property.PropertyType);
            }
        }

        /// <summary>
        /// copy data values from protobuf to cells in data table.
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aStats"></param>
        static private void FillLoadAvgMetricsData(string aPublication, DataTable aDataTable, object aStats)
        {
            linuxcounters.loadavg_metrics_shortbusy_assembled metrics = aStats as linuxcounters.loadavg_metrics_shortbusy_assembled;
            if (metrics != null)
            {
                linuxcounters.loadavg_metrics_shortbusy_assembled.aggregate_buffer buffer = new linuxcounters.loadavg_metrics_shortbusy_assembled.aggregate_buffer();
                System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();
                int core_idx = 0;

                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    string traceMessage = string.Format("Received publcation [{0}]: aggregate = {1} {2}",
                                                  LiveFeedRoutingKeyMapper.GetRoutingKey(aPublication),
                                                  metrics.aggregate.Count,
                                                  Environment.NewLine);

                    for (int i = 0; i < metrics.aggregate.Count; i++)
                    {
                        traceMessage += string.Format(" aggregate[{0}]:", i);
                        foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                        {
                            try
                            {
                                traceMessage += string.Format(" {0} = {1},",
                                                              b_property.Name,
                                                              b_property.GetValue(metrics.aggregate[i], null));
                            }
                            catch (Exception ex)
                            {
                                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                   String.Format("Received loadavg metric publication: property [{0}] get value exception: {1}",
                                                                 b_property.Name, ex.Message));
                            }
                        }
                        traceMessage += Environment.NewLine;
                    }

                    Logger.OutputToLog(TraceOptions.TraceOption.LIVEFEEDDATA, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, traceMessage);
                }

                // fill up one node at a time.
                for (int i = 0; i < metrics.aggregate.Count; i++)
                {
                    DataRow dr = aDataTable.NewRow();
                    FillInfoHeaderData(aPublication, dr, metrics.header);

                    // add the aggr fields now
                    foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                    {
                        try
                        {
                            if (b_property.Name.Equals("node_id"))
                            {
                                dr["loadavg_metrics_" + b_property.Name] = i;
                            }
                            else
                            {
                                dr["loadavg_metrics_" + b_property.Name] = b_property.GetValue(metrics.aggregate[i], null);
                            }
                        }
                        catch (Exception ex)
                        {
                            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                               String.Format("Received loadavg metric publication: property [{0}] get value exception: {1}",
                                                             b_property.Name, ex.Message));
                        }
                    }
                    aDataTable.Rows.Add(dr);
                }
            }
        }

        #endregion performance.loadavg_metrics

        #region performance.memory_metrics

        /// <summary>
        /// Add memory metrics schemas
        /// </summary>
        /// <param name="aDataTable"></param>
        static private void AddMemoryMetricsColumns(DataTable aDataTable)
        {
            //Create columns
            linuxcounters.memory_metrics_shortbusy_assembled.aggregate_buffer buffer = new linuxcounters.memory_metrics_shortbusy_assembled.aggregate_buffer();
            System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();

            //You can then use the PropertyInfo array to loop through each property of the type.
            foreach (System.Reflection.PropertyInfo property in buffer_properties)
            {
                aDataTable.Columns.Add("memory_metrics_" + property.Name, property.PropertyType);
            }
        }

        /// <summary>
        /// copy data values from protobuf to cells in data table.
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aStats"></param>
        static private void FillMemoryMetricsData(string aPublication, DataTable aDataTable, object aStats)
        {
            linuxcounters.memory_metrics_shortbusy_assembled metrics = aStats as linuxcounters.memory_metrics_shortbusy_assembled;
            if (metrics != null)
            {
                linuxcounters.memory_metrics_shortbusy_assembled.aggregate_buffer buffer = new linuxcounters.memory_metrics_shortbusy_assembled.aggregate_buffer();
                System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();

                int core_idx = 0;

                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    string traceMessage = string.Format("Received publcation [{0}]: aggregate = {1} {2}",
                                                  LiveFeedRoutingKeyMapper.GetRoutingKey(aPublication),
                                                  metrics.aggregate.Count,
                                                  Environment.NewLine);

                    for (int i = 0; i < metrics.aggregate.Count; i++)
                    {
                        traceMessage += string.Format(" aggregate[{0}]:", i);
                        foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                        {
                            try
                            {
                                traceMessage += string.Format(" {0} = {1},",
                                                              b_property.Name,
                                                              b_property.GetValue(metrics.aggregate[i], null));
                            }
                            catch (Exception ex)
                            {
                                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                   String.Format("Received memory metric publication: property [{0}] get value exception: {1}",
                                                                 b_property.Name, ex.Message));
                            }
                        }
                        traceMessage += Environment.NewLine;
                    }

                    Logger.OutputToLog(TraceOptions.TraceOption.LIVEFEEDDATA, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, traceMessage);
                }

                // fill up one node at a time.
                for (int i = 0; i < metrics.aggregate.Count; i++)
                {
                    DataRow dr = aDataTable.NewRow();
                    FillInfoHeaderData(aPublication, dr, metrics.header);

                    // add the aggr fields now
                    foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                    {
                        try
                        {
                            if (b_property.Name.Equals("node_id"))
                            {
                                dr["memory_metrics_" + b_property.Name] = i;
                            }
                            else
                            {
                                dr["memory_metrics_" + b_property.Name] = b_property.GetValue(metrics.aggregate[i], null);
                            }
                        }
                        catch (Exception ex)
                        {
                            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                               String.Format("Received memory metric publication: property [{0}] get value exception: {1}",
                                                             b_property.Name, ex.Message));
                        }
                    }
                    aDataTable.Rows.Add(dr);
                }
            }
        }

        #endregion performance.memory_metrics

        #region performance.network_metrics

        /// <summary>
        /// Add network_metrics schemas
        /// </summary>
        /// <param name="aDataTable"></param>
        static private void AddNetworkMetricsColumns(DataTable aDataTable)
        {
            //Create columns
            linuxcounters.network_metrics_shortbusy_assembled.aggregate_network_count network_count = new linuxcounters.network_metrics_shortbusy_assembled.aggregate_network_count();
            linuxcounters.network_metrics_shortbusy_assembled.aggregate_network_buffer network_buffer = new linuxcounters.network_metrics_shortbusy_assembled.aggregate_network_buffer();
            System.Reflection.PropertyInfo[] count_properties = network_count.GetType().GetProperties();
            System.Reflection.PropertyInfo[] buffer_properties = network_buffer.GetType().GetProperties();

            //You can then use the PropertyInfo array to loop through each property of the type.
            foreach (System.Reflection.PropertyInfo property in count_properties)
            {
                aDataTable.Columns.Add("network_metrics_count_" + property.Name, property.PropertyType);
            }

            foreach (System.Reflection.PropertyInfo property in buffer_properties)
            {
                aDataTable.Columns.Add("network_metrics_" + property.Name, property.PropertyType);
            }
        }




        /// <summary>
        /// copy data values from protobuf to cells in data table.
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aStats"></param>
        static private void FillNetworkMetricsData(string aPublication, DataTable aDataTable, object aStats)
        {
            linuxcounters.network_metrics_shortbusy_assembled metrics = aStats as linuxcounters.network_metrics_shortbusy_assembled;
            if (metrics != null)
            {
                linuxcounters.network_metrics_shortbusy_assembled.aggregate_network_count network_count = new linuxcounters.network_metrics_shortbusy_assembled.aggregate_network_count();
                linuxcounters.network_metrics_shortbusy_assembled.aggregate_network_buffer network_buffer = new linuxcounters.network_metrics_shortbusy_assembled.aggregate_network_buffer();
                System.Reflection.PropertyInfo[] count_properties = network_count.GetType().GetProperties();
                System.Reflection.PropertyInfo[] buffer_properties = network_buffer.GetType().GetProperties();

                int core_idx = 0;
                int total_aggregates = metrics.aggregate.Count;

                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    string traceMessage = string.Format("Received publcation [{0}]: aggregate_count = {1}, aggregate = {2} {3}",
                                                  LiveFeedRoutingKeyMapper.GetRoutingKey(aPublication),
                                                  metrics.network_count_aggr.Count,
                                                  metrics.aggregate.Count,
                                                  Environment.NewLine);

                    for (int i = 0; i < metrics.network_count_aggr.Count; i++)
                    {
                        traceMessage += string.Format(" network_count_aggr[{0}]:", i);
                        // add count properties first
                        foreach (System.Reflection.PropertyInfo c_property in count_properties)
                        {
                            try
                            {
                                traceMessage += string.Format(" {0} = {1},",
                                                              c_property.Name,
                                                              c_property.GetValue(metrics.network_count_aggr[i], null));
                            }
                            catch (Exception ex)
                            {
                                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                   String.Format("Received network metric publication: property [{0}] get value exception: {1}",
                                                                 c_property.Name, ex.Message));
                            }
                        }
                        traceMessage += Environment.NewLine;
                    }

                    //for (int i = 0; i < metrics.aggregate.Count; i++)
                    //{
                    //    traceMessage += string.Format(" aggregate[{0}]:", i);
                    //    foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                    //    {
                    //        try
                    //        {
                    //            traceMessage += string.Format(" {0} = {1},",
                    //                                          b_property.Name,
                    //                                          b_property.GetValue(metrics.aggregate[i], null));
                    //        }
                    //        catch (Exception ex)
                    //        {
                    //            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                    //                               String.Format("Received network metric publication: property [{0}] get value exception: {1}",
                    //                                             b_property.Name, ex.Message));
                    //        }
                    //    }
                    //    traceMessage += Environment.NewLine;
                    //}

                    Logger.OutputToLog(TraceOptions.TraceOption.LIVEFEEDDATA, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, traceMessage);
                }

                // fill up one node at a time.
                for (int i = 0; i < metrics.network_count_aggr.Count; i++)
                {
                    int num = metrics.network_count_aggr[i].num_network_interfaces;
                    if (num > 0)
                    {
                        for (int j = 0; j < num; j++)
                        {
                            if ((core_idx + j) < total_aggregates)
                            {
                                DataRow dr = aDataTable.NewRow();
                                FillInfoHeaderData(aPublication, dr, metrics.header);

                                // add count properties first
                                foreach (System.Reflection.PropertyInfo c_property in count_properties)
                                {
                                    try
                                    {
                                        dr["network_metrics_count_" + c_property.Name] = c_property.GetValue(metrics.network_count_aggr[i], null);
                                    }
                                    catch (Exception ex)
                                    {
                                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                           String.Format("Received network metric publication: property [{0}] get value exception: {1}",
                                                                         c_property.Name, ex.Message));
                                    }
                                }

                                // add the aggr fields now
                                foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                                {
                                    try
                                    {
                                        dr["network_metrics_" + b_property.Name] = b_property.GetValue(metrics.aggregate[core_idx + j], null);
                                    }
                                    catch (Exception ex)
                                    {
                                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                           String.Format("Received network metric publication: property [{0}] get value exception: {1}",
                                                                         b_property.Name, ex.Message));
                                    }
                                }
                                aDataTable.Rows.Add(dr);
                            }
                            else
                            {
                                // [TBD] should emit an exception, but now just return
                                return;
                            }
                        }
                        core_idx += num;
                    }
                    else
                    {
                        DataRow dr = aDataTable.NewRow();
                        FillInfoHeaderData(aPublication, dr, metrics.header);

                        // add count properties first
                        foreach (System.Reflection.PropertyInfo c_property in count_properties)
                        {
                            dr["network_metrics_count_" + c_property.Name] = 1;
                        }

                        // add the aggr fields now
                        foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                        {
                            if (b_property.Name.Equals("node_id"))
                            {
                                dr["network_metrics_" + b_property.Name] = i;
                            }
                            else
                            {
                                dr["network_metrics_" + b_property.Name] = num;
                            }
                        }
                        aDataTable.Rows.Add(dr);
                        if (num == -1)
                        {
                            core_idx += 1;
                        }
                    }
                }

                //[TBD]
                // At the end, the core_idx should be the same as aggregate count
                //if (core_idx != metrics.aggregate.Count)
                //{
                //    throw new Exception(string.Format("performance.network_metrics buffer counts mismatched: aggregate total={0}, buffer count={1}", core_idx, metrics.aggregate.Count));
                //}
            }
        }

        #endregion performance.network_metrics

        #region performance.virtualmem_metrics

        /// <summary>
        /// Add vm metrics schemas
        /// </summary>
        /// <param name="aDataTable"></param>
        static private void AddVirtualmemMetricsColumns(DataTable aDataTable)
        {
            //Create columns
            linuxcounters.virtualmem_metrics_shortbusy_assembled.aggregate_buffer buffer = new linuxcounters.virtualmem_metrics_shortbusy_assembled.aggregate_buffer();
            System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();

            //You can then use the PropertyInfo array to loop through each property of the type.
            foreach (System.Reflection.PropertyInfo property in buffer_properties)
            {
                aDataTable.Columns.Add("virtualmem_metrics_" + property.Name, property.PropertyType);
            }
        }

        /// <summary>
        /// copy data values from protobuf to cells in data table.
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aStats"></param>
        static private void FillVirtualmemMetricsData(string aPublication, DataTable aDataTable, object aStats)
        {
            linuxcounters.virtualmem_metrics_shortbusy_assembled metrics = aStats as linuxcounters.virtualmem_metrics_shortbusy_assembled;
            if (metrics != null)
            {
                linuxcounters.virtualmem_metrics_shortbusy_assembled.aggregate_buffer buffer = new linuxcounters.virtualmem_metrics_shortbusy_assembled.aggregate_buffer();
                System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();

                int core_idx = 0;

                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    string traceMessage = string.Format("Received publcation [{0}]: aggregate = {1} {2}",
                                                  LiveFeedRoutingKeyMapper.GetRoutingKey(aPublication),
                                                  metrics.aggregate.Count,
                                                  Environment.NewLine);

                    for (int i = 0; i < metrics.aggregate.Count; i++)
                    {
                        traceMessage += string.Format(" aggregate[{0}]:", i);
                        foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                        {
                            try
                            {
                                traceMessage += string.Format(" {0} = {1},",
                                                              b_property.Name,
                                                              b_property.GetValue(metrics.aggregate[i], null));
                            }
                            catch (Exception ex)
                            {
                                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                   String.Format("Received virtual memory metric publication: property [{0}] get value exception: {1}",
                                                                 b_property.Name, ex.Message));
                            }
                        }
                        traceMessage += Environment.NewLine;
                    }

                    Logger.OutputToLog(TraceOptions.TraceOption.LIVEFEEDDATA, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, traceMessage);
                }

                // fill up one node at a time.
                for (int i = 0; i < metrics.aggregate.Count; i++)
                {
                    DataRow dr = aDataTable.NewRow();
                    FillInfoHeaderData(aPublication, dr, metrics.header);

                    // add the aggr fields now
                    foreach (System.Reflection.PropertyInfo b_property in buffer_properties)
                    {
                        try
                        {
                            if (b_property.Name.Equals("node_id"))
                            {
                                dr["virtualmem_metrics_" + b_property.Name] = i;
                            }
                            else
                            {
                                dr["virtualmem_metrics_" + b_property.Name] = b_property.GetValue(metrics.aggregate[i], null);
                            }
                        }
                        catch (Exception ex)
                        {
                            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                               String.Format("Received virtual memory metric publication: property [{0}] get value exception: {1}",
                                                             b_property.Name, ex.Message));
                        }
                    }
                    aDataTable.Rows.Add(dr);
                }
            }
        }

        #endregion performance.virtualmem_metrics


        #region problem_management
        /// <summary>
        /// Add problem schemas
        /// </summary>
        /// <param name="aDataTable"></param>
        static private void AddProblemColumns(DataTable aDataTable) 
        {
            //Create columns
            problem_management.problem_open problem = new problem_management.problem_open();
            System.Reflection.PropertyInfo[] problem_properties = problem.GetType().GetProperties();

            //You can then use the PropertyInfo array to loop through each property of the type.
            foreach (System.Reflection.PropertyInfo property in problem_properties)
            {
                aDataTable.Columns.Add(property.Name, property.PropertyType);
            }
        }

        /// <summary>
        /// copy data values from protobuf to cells in data table.
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aStats"></param>
        static void FillProblemOpenData(string aPublication, DataTable aDataTable, object aStats) 
        {           
                    //problem_management.problem_open problem = aStats as problem_management.problem_open;
                    

            problem_management.problem_open problem = aStats as problem_management.problem_open;
            if (problem != null) 
            {
                DataRow dr = aDataTable.NewRow();
                FillInfoHeaderData(aPublication, dr, problem.header);
                System.Reflection.PropertyInfo[] properties = problem.GetType().GetProperties();
                foreach (System.Reflection.PropertyInfo property in properties) 
                {
                    if (property.PropertyType.FullName.StartsWith("System.", StringComparison.OrdinalIgnoreCase))
                        dr[property.Name] = property.GetValue(problem, null);
                }
                aDataTable.Rows.Add(dr);
            }
        }

        static void FillProblemCloseData(string aPublication, DataTable aDataTable, object aStats)
        {
            problem_management.problem_close problem = aStats as problem_management.problem_close;
            if (problem != null)
            {
                DataRow dr = aDataTable.NewRow();
                FillInfoHeaderData(aPublication, dr, problem.header);
                System.Reflection.PropertyInfo[] properties = problem.GetType().GetProperties();
                foreach (System.Reflection.PropertyInfo property in properties)
                {
                    if (property.PropertyType.FullName.StartsWith("System.", StringComparison.OrdinalIgnoreCase))
                        dr[property.Name] = property.GetValue(problem, null);
                }
                aDataTable.Rows.Add(dr);
            }
        }

        #endregion problem_management

        /// <summary>
        /// Use the given Event header to fill the event_header columns in the given DataRow. (all of the fields
        /// start with "event_header_".
        /// [Note]: Examine proto files to see if the event header has any other headers (structs).
        /// </summary>
        /// <param name="publication"></param>
        /// <param name="dr"></param>
        /// <param name="event_header"></param>
        static private void FillEventHeaderData(string publication, DataRow dr, common.event_header event_header)
        {
            //Fill the info header first.
            FillInfoHeaderData(publication, dr, event_header.header);

            //Fill all other fields of the event Header; all of the fields with basic type.
            Type objectType = event_header.GetType();
            System.Reflection.PropertyInfo[] properties = objectType.GetProperties();

            //You can then use the PropertyInfo array to loop through each property of the type.
            foreach (System.Reflection.PropertyInfo property in properties)
            {
                if (property.PropertyType.FullName.StartsWith("System.", StringComparison.OrdinalIgnoreCase))
                {
                    dr["event_header_" + property.Name] = property.GetValue(event_header, null);
                }
            }
        }

        /// <summary>
        /// Use the given info header to fill the info_header columns in the given DataRow.
        /// [Note]: Examine proto files to see if the info header has anymore headers or structs.
        /// </summary>
        /// <param name="publication"></param>
        /// <param name="dr"></param>
        /// <param name="info_header"></param>
        static private void FillInfoHeaderData(string publication, DataRow dr, common.info_header info_header)
        {
            //Fill the Qpid header first.
            FillQpidHeaderData(publication, dr, info_header.header);

            //Fill all other fields of the Info Header.
            Type objectType = info_header.GetType();
            System.Reflection.PropertyInfo[] properties = objectType.GetProperties();

            //You can then use the PropertyInfo array to loop through each property of the type.
            foreach (System.Reflection.PropertyInfo property in properties)
            {
                if (property.PropertyType.FullName.StartsWith("System.", StringComparison.OrdinalIgnoreCase))
                {
                    dr["info_header_" + property.Name] = property.GetValue(info_header, null);
                }
            }
        }

        /// <summary>
        /// Use the given Qpid header to fill the qpid_header columns in the given DataRow.
        /// [Note]: Examine proto files to see if the qpid header has anymore headers or structs.
        /// </summary>
        /// <param name="publication"></param>
        /// <param name="dr"></param>
        /// <param name="qpid_header"></param>
        static private void FillQpidHeaderData(string publication, DataRow dr, common.qpid_header qpid_header)
        {
            //Fill all of the header's fields
            Type objectType = qpid_header.GetType();
            System.Reflection.PropertyInfo[] properties = objectType.GetProperties();

            //You can then use the PropertyInfo array to loop through each property of the type.
            foreach (System.Reflection.PropertyInfo property in properties)
            {
                dr["qpid_header_" + property.Name] = property.GetValue(qpid_header, null);
            }
        }

        static private void FillHealthHeaderData(string publication, DataRow dr, common.health_header health_header)
        {
            //Fill info header first
            FillInfoHeaderData(publication, dr, health_header.header);

            //Fill all of the header's fields
            Type objectType = health_header.GetType();
            System.Reflection.PropertyInfo[] properties = objectType.GetProperties();

            //You can then use the PropertyInfo array to loop through each property of the type.
            foreach (System.Reflection.PropertyInfo property in properties)
            {
                if (property.PropertyType.FullName.StartsWith("System.", StringComparison.OrdinalIgnoreCase))
                {
                    dr["health_header_" + property.Name] = property.GetValue(health_header, null);
                }
            }
        }

        static private health.layer_check GetHealthLayerCheck(string publication, object aStats)
        {
            health.layer_check health_layer_check = null;

            switch (publication)
            {
                case LiveFeedRoutingKeyMapper.PUBS_health_state_access_layer:
                    accesslayer.level_1_check access_check = aStats as accesslayer.level_1_check;
                    if (access_check != null)
                    {
                        health_layer_check = access_check.checkinfo;
                    }
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_database_layer:
                    databaselayer.level_1_check database_check = aStats as databaselayer.level_1_check;
                    if (database_check != null)
                    {
                        health_layer_check = database_check.checkinfo;
                    }
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_foundation_layer:
                    foundationlayer.level_1_check foundation_check = aStats as foundationlayer.level_1_check;
                    if (foundation_check != null)
                    {
                        health_layer_check = foundation_check.checkinfo;
                    }
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_os_layer:
                    oslayer.level_1_check os_check = aStats as oslayer.level_1_check;
                    if (os_check != null)
                    {
                        health_layer_check = os_check.checkinfo;
                    }
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_server_layer:
                    serverlayer.level_1_check server_check = aStats as serverlayer.level_1_check;
                    if (server_check != null)
                    {
                        health_layer_check = server_check.checkinfo;
                    }
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_storage_layer:
                    storagelayer.level_1_check storage_check = aStats as storagelayer.level_1_check;
                    if (storage_check != null)
                    {
                        health_layer_check = storage_check.checkinfo;
                    }
                    break;

                default:
                    break;
            }

            return health_layer_check;
        }

        static private void FillHealthLayerCheckData(string publication, DataTable aDataTable, object aStats)
        {
            health.layer_check health_layer_check = GetHealthLayerCheck(publication, aStats);

            if (health_layer_check == null)
                return;

            for (int i = 0; i < health_layer_check.data.Count; i++)
            {
                DataRow dr = aDataTable.NewRow();
                // Fill header first
                FillHealthHeaderData(publication, dr, health_layer_check.header);

                dr["layer_name"] = health_layer_check.layer_name;
                dr["layer_current_score"] = health_layer_check.layer_current_score;
                dr["layer_previous_score"] = health_layer_check.layer_previous_score;
                dr["layer_score_change_ts_lct"] = health_layer_check.layer_score_change_ts_lct;
                dr["layer_score_change_ts_utc"] = health_layer_check.layer_score_change_ts_utc;

                health.layer_check.health_layer_data_struct data_struct = new health.layer_check.health_layer_data_struct();
                System.Reflection.PropertyInfo[] properties = data_struct.GetType().GetProperties();

                foreach (System.Reflection.PropertyInfo property in properties)
                {
                    try
                    {
                        dr["data_struct_" + property.Name] = property.GetValue(health_layer_check.data[i], null);
                    }
                    catch (Exception ex)
                    {
                        //[TBD] should emit at least a trace logging.
                    }
                }

                aDataTable.Rows.Add(dr);
            }
        }

        /// <summary>
        /// Use the given stats to fill a row to the given DataTable.
        /// [Note]: Examine proto files to see if any of the published stats has been changed.
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aStats"></param>
        static private void FillData(string aPublication, DataTable aDataTable, object aStats)
        {
            switch (aPublication)
            {
                case LiveFeedRoutingKeyMapper.PUBS_common_text_event:
                    FillEventData(aPublication, aDataTable, aStats);
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_core_metrics_asm:
                    FillCoreMetricsData(aPublication, aDataTable, aStats);
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_disk_metrics_asm:
                    FillDiskMetricsData(aPublication, aDataTable, aStats);
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_tse_metrics_asm:
                    FillTseMetricsData(aPublication, aDataTable, aStats);
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_filesys_metrics_asm:
                    FillFileSysMetricsData(aPublication, aDataTable, aStats);
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_loadavg_metrics_asm:
                    FillLoadAvgMetricsData(aPublication, aDataTable, aStats);
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_memory_metrics_asm:
                    FillMemoryMetricsData(aPublication, aDataTable, aStats);
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_network_metrics_asm:
                    FillNetworkMetricsData(aPublication, aDataTable, aStats);
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_virtualmem_metrics_asm:
                    FillVirtualmemMetricsData(aPublication, aDataTable, aStats);
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_problem_management_problem_open:
                    FillProblemOpenData(aPublication, aDataTable, aStats);
                    break;
                case LiveFeedRoutingKeyMapper.PUBS_problem_management_problem_close:
                    FillProblemCloseData(aPublication, aDataTable, aStats);
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_access_layer:
                case LiveFeedRoutingKeyMapper.PUBS_health_state_database_layer:
                case LiveFeedRoutingKeyMapper.PUBS_health_state_foundation_layer:
                case LiveFeedRoutingKeyMapper.PUBS_health_state_os_layer:
                case LiveFeedRoutingKeyMapper.PUBS_health_state_server_layer:
                case LiveFeedRoutingKeyMapper.PUBS_health_state_storage_layer:
                    FillHealthLayerCheckData(aPublication, aDataTable, aStats);
                    break;
            }
        }

        /// <summary>
        /// Trace the health layer data
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aStats"></param>
        static private void TraceHealthLayerCheckData(string aPublication, object aStats)
        {
            if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
            {
                health.layer_check health_layer_check = GetHealthLayerCheck(aPublication, aStats);
                string trace = "";

                if (health_layer_check == null)
                    return;

                // skip header first
                trace += String.Format("Received Health/State publication: layer_name = {0}; layer_current_score = {1}; layer_previous_score = {2}; layer_score_change_ts_lc = {3}; {4}",
                                       health_layer_check.layer_name,
                                       health_layer_check.layer_current_score,
                                       health_layer_check.layer_previous_score,
                                       health_layer_check.layer_score_change_ts_lct,
                                       Environment.NewLine);

                for (int i = 0; i < health_layer_check.data.Count; i++)
                {
                    trace += String.Format(" health_layer_check.data[{0}]: ", i);

                    health.layer_check.health_layer_data_struct data_struct = new health.layer_check.health_layer_data_struct();
                    System.Reflection.PropertyInfo[] properties = data_struct.GetType().GetProperties();

                    foreach (System.Reflection.PropertyInfo property in properties)
                    {
                        try
                        {
                            trace += String.Format(" {0} = {1};",
                                                   property.Name,
                                                   property.GetValue(health_layer_check.data[i], null));
                        }
                        catch (Exception ex)
                        {
                            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                               String.Format("Received Health/State publication: property [{0}] get value exception: {1}",
                                                             property.Name, ex.Message));
                        }
                    }

                    trace += Environment.NewLine;
                }

                Logger.OutputToLog(TraceOptions.TraceOption.LIVEFEEDDATA, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, trace);
            }
        }

        #endregion Private methods
    }
}
