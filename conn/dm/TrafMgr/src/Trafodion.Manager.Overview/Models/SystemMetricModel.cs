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
using System.Windows.Forms.DataVisualization.Charting;
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.Framework;
using linuxcounters;
using Trafodion.Manager.Framework.Connections;
using System.Linq;
using System.Drawing;
using System.Collections.Generic;

namespace Trafodion.Manager.OverviewArea.Models
{
    public enum TseMetric
    {
        requests,
        service_time,
        ase_service_time,
        request_io_wait_time,
        ready_list_count
    }

    public partial class SystemMetricModel
    {
        #region Fields

        // Persistence key used to store system metric related persistence
        public const string EventFilterPersistenceKey = "SystemMetricPersistenceKey";
        private const string PERSISTENCE_KEY_SELECTED_TSE_METRIC = "SelectedTseMetric";
        private const string TRACE_SUB_AREA_NAME = "SystemMetricModel";
        private static readonly string DEFAULT_TSE_METRIC = TseMetric.service_time.ToString();        

        public static string SelectedTseMetric
        {
            get
            {
                string selectedTseMetric = (string)Persistence.Get(PERSISTENCE_KEY_SELECTED_TSE_METRIC);
                if (selectedTseMetric != null && selectedTseMetric.Trim().Length > 0)
                {
                    return selectedTseMetric;
                }
                else
                {
                    Persistence.Put(PERSISTENCE_KEY_SELECTED_TSE_METRIC, DEFAULT_TSE_METRIC);
                    return DEFAULT_TSE_METRIC;
                }
            }
            set
            {
                Persistence.Put(PERSISTENCE_KEY_SELECTED_TSE_METRIC, value);
            }
        }
        

        /// <summary>
        /// Definition of system metric types
        /// </summary>
        public enum SystemMetrics 
        { 
            Core = 0, 
            Memory, 
            Swap, 
            File_System, 
            Load_Avg, 
            Disk, 
            Network_Rcv, 
            Network_Txn, 
            Virtual_Memory,
            Tse
        };
        
        /// <summary>
        /// Definition of system metric types
        /// </summary>
        public static SystemMetrics[] OrderedSystemMetrics = new SystemMetrics[]
                        {                
                            SystemMetrics.Core,
                            SystemMetrics.Memory,
                            SystemMetrics.Swap,
                            SystemMetrics.File_System,
                            SystemMetrics.Load_Avg,
                            SystemMetrics.Network_Rcv,
                            SystemMetrics.Network_Txn,
                            SystemMetrics.Disk,
                            SystemMetrics.Tse,
                            SystemMetrics.Virtual_Memory
                        };
        
        /// <summary>
        /// System metric table's schema definition
        /// </summary>
        public enum SystemMetricTableColumns
        {
            id = 0, 
            metric,
            displayName, 
            title,
            xValueMember, 
            xValueType,
            yValueMember,
            yValueType,
            yValueMin, 
            yValueMax, 
            colorTableKey
        }

        /// <summary>
        /// Different system metric displays supported in M6
        /// </summary>
        public enum SystemMetricDisplays
        {
            OverallSummary = 0, 
            CoreMetricDetails,
            MemoryMetricDetails,
            SwapMetricDetails,
            FileSystemMetricDetails,
            LoadAvgMetricDetails,
            DiskMetricDetails,
            NetworkMetricDetails,
            VMMetricDetails,
            TseMetricDetails
        }

        /// <summary>
        /// Load Avg system metric's sub metrics
        /// </summary>
        public enum LoadAvgSubMetrics
        {
            Load_Avg_1_Min = 0,
            Load_Avg_5_Min,
            Load_Avg_15_Min
        }

        /// <summary>
        /// Network system metric's sub metrics
        /// </summary>
        public enum NetworkSubMetrics
        {
            Network_Rcv_Packets = 0,
            Network_Rcv_Drops,
            Network_Rcv_Errs,
            Network_Txn_Packets,
            Network_Txn_Drops,
            Network_Txn_Errs
        }

        /// <summary>
        /// Virtual Memory system metric's sub metrics
        /// </summary>
        public enum VMSubMetrics
        {
            VM_Swap_In = 0, 
            VM_Swap_Out,
            VM_Minor_Page_Faults,
            VM_Major_Page_Faults,
            VM_Context_Switches
        }

        // An enumeration of the current score for the subject as follows:
        //    green(0), yellow(1), red (2)
        //
        public enum HealthSubjectScore
        {
            Green = 0, 
            Yellow, 
            Red
        }

        /// <summary>
        /// Supported Health/States layers in M6
        /// </summary>
        public enum HealthLayer
        {
            Access = 0, 
            Database, 
            Foundation, 
            OS,
            Server,
            Storage
        }

        public static string[] HealthLayerDisplayNames = new string[]
        {
            "Access", 
            "Database", 
            "Foundation", 
            "OS", 
            "Server", 
            "Storage"
        };

        /// <summary>
        /// The subject areas of Access Layer
        /// </summary>
        public enum AccessSubjectAreas
        {
            BDR = 0, 
            NDCS,
            NVT, 
            WMS
        }

        /// <summary>
        /// [Note] Make sure the order is the same as enum
        /// </summary>
        public static string[] AccessSubjectAreaDisplayNames = new string[] 
        {
            "Access", "Bulk Data Replicator (BDR)",
            "Connectivity Services (HPDCS))", 
            "Transporter (NVT)",
            "Workload Management Services (WMS)"
        };

        /// <summary>
        /// The subject areas of database layer
        /// </summary>
        public enum DatabaseSubjectAreas
        {
            CanaryQuery = 0
        }

        /// <summary>
        /// [Note] Make sure the order is the same as enum
        /// </summary>
        public static string[] DatabaseSubjectAreaDisplayNames = new string[] 
        {
            "Internal canary query"
        };

        /// <summary>
        /// The subjuect areas of foundation layer
        /// </summary>
        public enum FoundationSubjectAreas
        {
            DTM = 0,
            Monitor,
            SeaPilot,
            StorageEngine
        }

        /// <summary>
        /// [Note] Make sure the order is the same as enum
        /// </summary>
        public static string[] FoundationSubjectAreaDisplayNames = new string[]
        {
            "Distributed Transaction Manager (DTM)", 
            "Monitor",
            "SeaPilot", 
            "Storage Engines"
        };

        /// <summary>
        /// The subject areas of the OS layer
        /// </summary>
        public enum OsSubjectAreas
        {
            LDAP = 0,
            NetworkServices,
            NTP,
            SNMP
        }

        /// <summary>
        /// [Note] Make sure the order is the same as enum
        /// </summary>
        public static string[] OsSubjectAreaDisplayNames = new string[]
        {
            "Light-Weight Directory Protocol (LDAP)",
            "Networking services",
            "Network Time Protocol (NTP)", 
            "Simple Network Management Protocol (SNMP)"
        };

        /// <summary>
        /// The subject areas of the Server Layer
        /// </summary>
        public enum ServerSubjectAreas
        {
            Enclosures = 0,
            Nodes,
            Cores,
            Memory,
            Power,
            Fans
        }

        /// <summary>
        /// [Note] Make sure the order is the same as enum
        /// </summary>
        public static string[] ServerSubjectAreaDisplayNames = new string[]
        {
            "Enclosures", 
            "Nodes", 
            "Cores", 
            "Memory", 
            "Power", 
            "Fans"
        };

        /// <summary>
        /// The subject areas of the Storage layer
        /// </summary>
        public enum StorageSubjectAreas
        {
            FileSystems = 0,
            MSAControllers,
            MSADisks,
            SSDs,
            CompressionCards
        }

        /// <summary>
        /// [Note] Make sure the order is the same as enum
        /// </summary>
        public static string[] StorageSubjectAreaDisplayNames = new string[]
        {
            "File systems", 
            "MSA controllers",
            "MSA disks", 
            "Solid state disks",
            "Compression cards"
        };

        /// <summary>
        /// The schema definition of Health Layer data table
        /// </summary>
        public enum HealthLayerStateTableColumns
        {
            layer = 0,
            subjectArea, 
            score
        }

        public enum CoreMetricDataTableColumns
        {
            node_id = 0,
            total_nodes,
            gen_time_ts_lct,
            core_id,
            node_and_core_id,
            avg_core_total,
            max_core,
            min_core,
            avg_core,
            zero
        }

        public enum MemoryMetricDataTableColumns
        {
            node_id = 0,
            total_nodes,
            gen_time_ts_lct,
            node_name,
            memory_used, 
            swap_used,
            max_memory_used,
            min_memory_used,
            avg_memory_used,
            max_swap_used,
            min_swap_used,
            avg_swap_used,
            zero
        }

        public enum DiskMetricDataTableColumns
        {
            node_id = 0, 
            total_nodes,
            gen_time_ts_lct,
            node_and_device_name, 
            reads_and_writes,
            max_reads_and_writes,
            min_reads_and_writes,
            avg_reads_and_writes, 
            zero
        }

        public enum TseMetricDataTableColumns
        {
            node_id = 0,
            total_nodes,
            gen_time_ts_lct,
            node_and_device_name,

            service_time,
            max_service_time,
            min_service_time,

            requests,
            max_requests,
            min_requests,

            ase_service_time,
            max_ase_service_time,
            min_ase_service_time,

            request_io_wait_time,
            max_request_io_wait_time,
            min_request_io_wait_time,

            ready_list_count,
            max_ready_list_count,
            min_ready_list_count,  

            zero
        }

        public enum FileSysMetricDataTableColumns
        {
            node_id = 0,
            total_nodes,
            gen_time_ts_lct,
            node_and_fsname,
            percent_consumed,
            max_percent_consumed,
            min_percent_consumed,
            avg_percent_consumed,
            zero
        }

        public enum LoadAvgMetricDataTableColumns
        {
            node_id = 0, 
            total_nodes,
            gen_time_ts_lct,
            node_name,
            one_min_avg,
            max_one_min_avg,
            min_one_min_avg,
            avg_one_min_avg, 
            five_min_avg, 
            fifteen_min_avg,
            zero
        }

        public enum NetworkMetricDataTableColumns
        {
            node_id = 0, 
            total_nodes,
            gen_time_ts_lct,
            node_and_netid, 
            rcv_packets, 
            rcv_drops,
            rcv_errs,
            max_rcv_packets,
            min_rcv_packets,
            avg_rcv_packets, 
            txn_packets, 
            txn_drops,
            txn_errs,
            max_txn_packets,
            min_txn_packets,
            avg_txn_packets, 
            zero
        }

        public enum VirtualMemoryMetricDataTableColumns
        {
            node_id = 0, 
            total_nodes,
            gen_time_ts_lct,
            node_name, 
            context_switches,
            max_context_switches,
            min_context_switches,
            avg_context_switches, 
            swap_in, 
            swap_out, 
            minor_page_faults,
            major_page_faults,
            zero
        }

        // The following definitions are used as the column names of metric data table or metric details data tables.
        public const string SystemMetricNodeID = "NodeID";
        public const string SystemMetricNodeName = "NodeName";
        public const string SystemMetricCoreID = "CoreID";
        public const string SystemMetricNodeAndCoreID = "Node&CoreID";
        public const string SystemMetricNodeAndNetID = "Node&NetID";
        public const string SystemMetricNodeAndDevID = "Node&DevID";
        public const string SystemMetricNodeAndIFID = "Node&IFID";
        public const string SystemMetricNodeAndFSName = "Node&FSName";
        public const string SystemMetricZero = "Zero";
        public const string SystemMetricGenTimeTsLCT = "GenTimeTsLCT";

        public static readonly string SystemMetricCoreBusy = SystemMetricModel.SystemMetrics.Core.ToString();
        public const string SystemMetricCoreBusyMin = "BusyMin";
        public const string SystemMetricCoreBusyMax = "BusyMax";
        public const string SystemMetricCoreBusyTitle = "%Node Busy";
        public const string SystemMetricCoreBusyMinToolTip = "Min %Node Busy";
        public const string SystemMetricCoreBusyMaxToolTip = "Max %Node Busy";
        public const string SystemMetricToolTipMergedCoreBusyAvgMinMax = " Node: {0}\nAvg %Node Busy: {1}\nMin %Node Busy: {2}\nMax %Node Busy: {3}\n{4}";

        public static readonly string SystemMetricDiskIO = SystemMetricModel.SystemMetrics.Disk.ToString();
        public const string SystemMetricDiskIOMin = "DiskMin";
        public const string SystemMetricDiskIOMax = "DiskMax";
        public const string SystemMetricDiskIOTitle = "Disk IOs";
        public const string SystemMetricDiskIOMinToolTip = "Min Disk IOs";
        public const string SystemMetricDiskIOMaxToolTip = "Max Disk IOs";
        public const string SystemMetricToolTipMergedDiskIOAvgMinMax = " Node: {0}\nAvg Disk IOs: {1}\nMin Disk IOs: {2}\nMax Disk IOs: {3}\n{4}";
        
        public static readonly string SystemMetricTse = SystemMetricModel.SystemMetrics.Tse.ToString();
        public const string SystemMetricTseMin = "TseMin";
        public const string SystemMetricTseMax = "TseMax";
        public const string SystemMetricTseTitle = "TSE Skew";
        public const string SystemMetricTseMetricTitle = "TSE {0} Skew";
        public const string SystemMetricTseMinToolTip = "Min TSE";
        public const string SystemMetricTseMaxToolTip = "Max TSE";
        public const string SystemMetricToolTipMergedTseAvgMinMax = " Node: {0}\nAvg {5}: {1}\nMin {5}: {2}\nMax {5}: {3}\n{4}";

        public const string SystemMetricToolTipFormat = " Node: #VALX\n{0}: #VALY";

        public const string SystemMetricMemoryUsed = "Memory Used";
        public const string SystemMetricSwapUsed = "Swap Used";

        public const string SystemMetricContextSwitches = "Context Switches";
        public const string SystemMetricSwapIn = "Swap In";
        public const string SystemMetricSwapOut = "Swap Out";
        public const string SystemMetricMinorPageFaults = "Minor Page Faults";
        public const string SystemMetricMajorPageFaults = "Major Page Faults";

        public const string SystemMetricOneMinAvg = "1 Min Avg";
        public const string SystemMetricFiveMinAvg = "5 Min Avg";
        public const string SystemMetricFifteenMinAvg = "15 Min Avg";

        public const string SystemMetricNetID = "NetID";
        public const string SystemMetricRcvPackets = "Rcv Packets";
        public const string SystemMetricTxnPackets = "Txn Packets";
        public const string SystemMetricRcvDrops = "Rcv Drops";
        public const string SystemMetricRcvErrs = "Rcv Errs";
        public const string SystemMetricTxnDrops = "Txn Drops";
        public const string SystemMetricTxnErrs = "Txn Errs";

        public const string SystemMetricPercentConsumed = "percent_consumed";

        public static readonly Color MinColor = Color.Aquamarine;
        public static readonly Color MaxColor = Color.SkyBlue;
              
        // The static metric tables            
        private static DataTable OverallSummaryMetrics = new DataTable();
        private static DataTable CoreMetricDetails = new DataTable();
        private static DataTable MemoryMetricDetails = new DataTable();
        private static DataTable FileSystemMetricDetails = new DataTable();
        private static DataTable LoadAvgMetricDetails = new DataTable();
        private static DataTable DiskMetricDetails = new DataTable();
        private static DataTable TseMetricDetails = new DataTable();
        private static DataTable NetworkMetricDetails = new DataTable();
        private static DataTable VMMetricDetails = new DataTable();
        public static readonly Dictionary<string, string> TseMetrics  = new Dictionary<string, string>();

        private const int maxNodes = 32;
        private const int CoresPerNode = 8;
        private static Random random = new Random();

        #endregion Fields

        #region Properties

        /// <summary>
        /// The Core metric detailed table
        /// </summary>
        public static DataTable CoreMetricDetailsTable
        {
            get { return CoreMetricDetails; }
        }

        /// <summary>
        /// The File system metric detailed table
        /// </summary>
        public static DataTable FileSystemMetricDetailsTable
        {
            get { return FileSystemMetricDetails; }
        }

        /// <summary>
        /// The Memory metric detailed table
        /// </summary>
        public static DataTable MemoryMetricDetailsTable
        {
            get { return MemoryMetricDetails; }
        }

        /// <summary>
        /// The Disk metric detailed table
        /// </summary>
        public static DataTable DiskMetricDetailsTable
        {
            get { return DiskMetricDetails; }
        }

        /// <summary>
        /// The TSE metric detailed table
        /// </summary>
        public static DataTable TseMetricDetailsTable
        {
            get { return TseMetricDetails; }
        }

        /// <summary>
        /// The Load Avg metric detailed table
        /// </summary>
        public static DataTable LoadAvgMetricDetailsTable
        {
            get { return LoadAvgMetricDetails; }
        }

        /// <summary>
        /// The Network metric detailed table
        /// </summary>
        public static DataTable NetworkMetricDetailsTable
        {
            get { return NetworkMetricDetails; }
        }

        /// <summary>
        /// The Virtual memory metric detailed table
        /// </summary>
        public static DataTable VMMetricDetailsTable
        {
            get { return VMMetricDetails; }
        }

        #endregion Properties

        #region Constructor
        /// <summary>
        /// Static constructor
        /// </summary>
        static SystemMetricModel()
        {
            // Construct all of the metric tables first
            ConstructOverallSummaryMetricsTable();
            ConstructCoreMetricDetailsTable();
            ConstructMemoryMetricDetailsTable();
            ConstructFileSystemMetricDetailsTable();
            ConstructLoadAvgMetricDetailsTable();
            ConstructNetworkMetricDetailsTable();
            ConstructVMMetricDetailsTable();
            ConstructDiskMetricDetailsTable();
            ConstructTseMetricDetailsTable();
        }

        #endregion Constructor  

        #region Public methods

        //public static DataTable GetOverallSummaryDataTable()
        //{
        //    DataTable table = new DataTable();
        //    //InitializeOverallSummaryDataTable(table);
        //    //GenerateOverallSummaryDataTable(table);
        //    return table;
        //}

        #endregion Public methods

        #region Private methods

        public static string GetChartNodeIdSeriesName()
        {
            return string.Format("Series_{0}", "NodeID");
        }

        public static string GetChartAreaName(SystemMetricModel.SystemMetrics metric)
        {
            return GetChartAreaName(metric.ToString());
        }

        public static string GetChartAreaName(string metric)
        {
            return string.Format("{0}", metric);
        }

                /// <summary>
        /// Construct the metric data table's column definitions.
        /// </summary>
        /// <param name="dt"></param>
        private static void AddSystemMetricTableColumns(DataTable dt)
        {
            dt.Columns.Add(SystemMetricTableColumns.id.ToString(), typeof(int));
            dt.Columns.Add(SystemMetricTableColumns.metric.ToString(), typeof(SystemMetrics));
            dt.Columns.Add(SystemMetricTableColumns.displayName.ToString(), typeof(string));
            dt.Columns.Add(SystemMetricTableColumns.title.ToString(), typeof(string));
            dt.Columns.Add(SystemMetricTableColumns.xValueMember.ToString(), typeof(string));
            dt.Columns.Add(SystemMetricTableColumns.xValueType.ToString(), typeof(ChartValueType));
            dt.Columns.Add(SystemMetricTableColumns.yValueMember.ToString(), typeof(string));
            dt.Columns.Add(SystemMetricTableColumns.yValueType.ToString(), typeof(ChartValueType));
            dt.Columns.Add(SystemMetricTableColumns.yValueMin.ToString(), typeof(double));
            dt.Columns.Add(SystemMetricTableColumns.yValueMax.ToString(), typeof(double));
            dt.Columns.Add(SystemMetricTableColumns.colorTableKey.ToString(), typeof(SystemMetrics));

            DataColumn[] key = new DataColumn[1];
            key[0] = dt.Columns[SystemMetricTableColumns.metric.ToString()];
            dt.PrimaryKey = key;
        }

        /// <summary>
        /// Construct the Overall System metrics datatable to be used by the OverallSummary display
        /// NOTE: This has to follow the order of the ENUM definition. 
        /// NOTE: Remember to update metric version. (esp. the display name got changed)
        /// </summary>
        private static void ConstructOverallSummaryMetricsTable()
        {
            AddSystemMetricTableColumns(OverallSummaryMetrics);

            OverallSummaryMetrics.Rows.Add(new object[] { (int)SystemMetrics.Core, SystemMetrics.Core, "Core", SystemMetricModel.SystemMetricCoreBusyTitle, SystemMetricNodeID, ChartValueType.String, SystemMetricCoreBusy, ChartValueType.Double, 0.0, 100.0, SystemMetrics.Core });
            OverallSummaryMetrics.Rows.Add(new object[] { (int)SystemMetrics.Memory, SystemMetrics.Memory, "Memory", "%Mem Used", SystemMetricNodeID, ChartValueType.String, SystemMetricMemoryUsed, ChartValueType.Double, 0.0, 100.0, SystemMetrics.Memory });
            OverallSummaryMetrics.Rows.Add(new object[] { (int)SystemMetrics.Swap, SystemMetrics.Swap, "Swap", "%Swap Used", SystemMetricNodeID, ChartValueType.String, SystemMetricSwapUsed, ChartValueType.Double, 0.0, 100.0, SystemMetrics.Swap });
            OverallSummaryMetrics.Rows.Add(new object[] { (int)SystemMetrics.File_System, SystemMetrics.File_System, "File System", "%FileSys Used", SystemMetricNodeID, ChartValueType.String,  SystemMetricPercentConsumed, ChartValueType.Double, 0.0, 100.0, SystemMetrics.File_System });
            OverallSummaryMetrics.Rows.Add(new object[] { (int)SystemMetrics.Load_Avg, SystemMetrics.Load_Avg, "Load Avg", "1 min Load Avg", SystemMetricNodeID, ChartValueType.String,  SystemMetricOneMinAvg, ChartValueType.Double, 0.0, -1, SystemMetrics.Load_Avg });
            OverallSummaryMetrics.Rows.Add(new object[] { (int)SystemMetrics.Network_Rcv, SystemMetrics.Network_Rcv, "Network Rcv", "Network Rcvs", SystemMetricNodeID, ChartValueType.String,  SystemMetricRcvPackets, ChartValueType.Double, 0.0, -1, SystemMetrics.Network_Rcv });
            OverallSummaryMetrics.Rows.Add(new object[] { (int)SystemMetrics.Network_Txn, SystemMetrics.Network_Txn, "Network Txn", "Network Xmits", SystemMetricNodeID, ChartValueType.String,  SystemMetricTxnPackets, ChartValueType.Double, 0.0, -1, SystemMetrics.Network_Txn });
            OverallSummaryMetrics.Rows.Add(new object[] { (int)SystemMetrics.Disk, SystemMetrics.Disk, "Disk", SystemMetricDiskIOTitle, SystemMetricNodeID, ChartValueType.String, SystemMetricDiskIO, ChartValueType.Double, 0.0, -1, SystemMetrics.Disk });
            OverallSummaryMetrics.Rows.Add(new object[] { (int)SystemMetrics.Tse, SystemMetrics.Tse, "Tse", SystemMetricTseTitle, SystemMetricNodeID, ChartValueType.String, SystemMetricTse, ChartValueType.Double, 0, -1, SystemMetrics.Tse });
            OverallSummaryMetrics.Rows.Add(new object[] { (int)SystemMetrics.Virtual_Memory, SystemMetrics.Virtual_Memory, "VM", "Context Switches", SystemMetricNodeID, ChartValueType.String,  SystemMetricContextSwitches, ChartValueType.Double, 0.0, -1, SystemMetrics.Virtual_Memory });
        }

        /// <summary>
        /// Construct the Core metric datatable to be used by the CoreMetricDetails display
        /// NOTE: This has to follow the order of the ENUM definition. 
        /// NOTE: Remember to update metric version. (esp. the display name got changed)
        /// </summary>
        private static void ConstructCoreMetricDetailsTable()
        {
            AddSystemMetricTableColumns(CoreMetricDetails);

            CoreMetricDetails.Rows.Add(
                new object[] { 
                    (int)SystemMetrics.Core, 
                    SystemMetrics.Core, 
                    "Core", 
                    "%Core Busy", 
                    CoreMetricDataTableColumns.node_and_core_id.ToString(), 
                    ChartValueType.String, 
                    CoreMetricDataTableColumns.avg_core_total.ToString(), 
                    ChartValueType.Double, 
                    0.0, 
                    100.0, 
                    SystemMetrics.Core });
        }

        /// <summary>
        /// Construct the Memory metric datatable to be used by the MemoryMetricDetails display
        /// NOTE: This has to follow the order of the ENUM definition. 
        /// NOTE: Remember to update metric version. (esp. the display name got changed)
        /// </summary>
        private static void ConstructMemoryMetricDetailsTable()
        {
            AddSystemMetricTableColumns(MemoryMetricDetails);

            MemoryMetricDetails.Rows.Add(
                new object[] { 
                    (int)SystemMetrics.Memory, 
                    SystemMetrics.Memory, 
                    "Memory", 
                    "%Memory Used", 
                    MemoryMetricDataTableColumns.node_name.ToString(), 
                    ChartValueType.String, 
                    MemoryMetricDataTableColumns.memory_used.ToString(), 
                    ChartValueType.Double, 
                    0.0, 
                    100.0, 
                    SystemMetrics.Memory });

            MemoryMetricDetails.Rows.Add(
                new object[] { 
                    (int)SystemMetrics.Swap, 
                    SystemMetrics.Swap, 
                    "Swap", 
                    "%Swap Used", 
                    MemoryMetricDataTableColumns.node_name.ToString(), 
                    ChartValueType.String, 
                    MemoryMetricDataTableColumns.swap_used.ToString(), 
                    ChartValueType.Double, 
                    0.0, 
                    100.0, 
                    SystemMetrics.Swap });
        }

        /// <summary>
        /// Construct the File System metric datatable to be used by the DiskMetricDetails display
        /// </summary>
        private static void ConstructFileSystemMetricDetailsTable()
        {
            AddSystemMetricTableColumns(FileSystemMetricDetails);
            FileSystemMetricDetails.Rows.Add(
                new object[] { 
                    (int)SystemMetrics.File_System, 
                    SystemMetrics.File_System, 
                    "FileSystem", 
                    "%FileSys Used", 
                    FileSysMetricDataTableColumns.node_and_fsname.ToString(), 
                    ChartValueType.String, 
                    FileSysMetricDataTableColumns.percent_consumed.ToString(), 
                    ChartValueType.Double, 
                    0.0, 
                    100.0, 
                    SystemMetrics.File_System });
        }

        /// <summary>
        /// Construct the Disk metric datatable to be used by the DiskMetricDetails display
        /// </summary>
        private static void ConstructDiskMetricDetailsTable()
        {
            AddSystemMetricTableColumns(DiskMetricDetails);
            DiskMetricDetails.Rows.Add(
                new object[] { 
                    (int)SystemMetrics.Disk, 
                    SystemMetrics.Disk, 
                    "Disk", 
                    "Disk IOs", 
                    DiskMetricDataTableColumns.node_and_device_name.ToString(), 
                    ChartValueType.String, 
                    DiskMetricDataTableColumns.reads_and_writes.ToString(), 
                    ChartValueType.Double, 
                    0.0, 
                    -1, 
                    SystemMetrics.Disk });
        }

        /// <summary>
        /// Construct the TSE metric datatable to be used by the TseMetricDetails display
        /// </summary>
        private static void ConstructTseMetricDetailsTable()
        {
            AddSystemMetricTableColumns(TseMetricDetails);
            TseMetricDetails.Rows.Add(
                new object[] { 
                    (int)SystemMetrics.Tse, 
                    SystemMetrics.Tse, 
                    "Tse", 
                    "TSE", 
                    TseMetricDataTableColumns.node_and_device_name.ToString(), 
                    ChartValueType.String, 
                    TseMetricDataTableColumns.service_time.ToString(), 
                    ChartValueType.Int64, 
                    0, 
                    -1, 
                    SystemMetrics.Tse });

            TseMetrics.Add(TseMetric.service_time.ToString(), "Service Time");
            TseMetrics.Add(TseMetric.request_io_wait_time.ToString(), "Request IO Wait Time");
            TseMetrics.Add(TseMetric.requests.ToString(), "Request Count");
            TseMetrics.Add(TseMetric.ready_list_count.ToString(), "Ready List Count");
            TseMetrics.Add(TseMetric.ase_service_time.ToString(), "ASE Service Time");
        }

        private static void ConstructLoadAvgMetricDetailsTable()
        {
            LoadAvgMetricDetails.Columns.Add(SystemMetricTableColumns.id.ToString(), typeof(int));
            LoadAvgMetricDetails.Columns.Add(SystemMetricTableColumns.metric.ToString(), typeof(LoadAvgSubMetrics));
            LoadAvgMetricDetails.Columns.Add(SystemMetricTableColumns.displayName.ToString(), typeof(string));
            LoadAvgMetricDetails.Columns.Add(SystemMetricTableColumns.title.ToString(), typeof(string));
            LoadAvgMetricDetails.Columns.Add(SystemMetricTableColumns.xValueMember.ToString(), typeof(string));
            LoadAvgMetricDetails.Columns.Add(SystemMetricTableColumns.xValueType.ToString(), typeof(ChartValueType));
            LoadAvgMetricDetails.Columns.Add(SystemMetricTableColumns.yValueMember.ToString(), typeof(string));
            LoadAvgMetricDetails.Columns.Add(SystemMetricTableColumns.yValueType.ToString(), typeof(ChartValueType));
            LoadAvgMetricDetails.Columns.Add(SystemMetricTableColumns.yValueMin.ToString(), typeof(double));
            LoadAvgMetricDetails.Columns.Add(SystemMetricTableColumns.yValueMax.ToString(), typeof(double));
            LoadAvgMetricDetails.Columns.Add(SystemMetricTableColumns.colorTableKey.ToString(), typeof(SystemMetrics));

            LoadAvgMetricDetails.Rows.Add(
                new object[] { 
                    (int)LoadAvgSubMetrics.Load_Avg_1_Min, 
                    LoadAvgSubMetrics.Load_Avg_1_Min, 
                    LoadAvgSubMetrics.Load_Avg_1_Min.ToString(), 
                    "1 Min Load Avg", 
                    LoadAvgMetricDataTableColumns.node_name, 
                    ChartValueType.String, 
                    LoadAvgMetricDataTableColumns.one_min_avg.ToString(), 
                    ChartValueType.Double, 
                    0.0, 
                    -1, 
                    SystemMetrics.Load_Avg });

            LoadAvgMetricDetails.Rows.Add(
                new object[] { 
                    (int)LoadAvgSubMetrics.Load_Avg_5_Min, 
                    LoadAvgSubMetrics.Load_Avg_5_Min, 
                    LoadAvgSubMetrics.Load_Avg_5_Min.ToString(), 
                    "5 Min Load Avg", 
                    LoadAvgMetricDataTableColumns.node_name, 
                    ChartValueType.String, 
                    LoadAvgMetricDataTableColumns.five_min_avg.ToString(), 
                    ChartValueType.Double, 
                    0.0, 
                    -1, 
                    SystemMetrics.Load_Avg });

            LoadAvgMetricDetails.Rows.Add(
                new object[] { 
                    (int)LoadAvgSubMetrics.Load_Avg_15_Min, 
                    LoadAvgSubMetrics.Load_Avg_15_Min, 
                    LoadAvgSubMetrics.Load_Avg_15_Min.ToString(), 
                    "15 Min Load Avg", 
                    LoadAvgMetricDataTableColumns.node_name, 
                    ChartValueType.String, 
                    LoadAvgMetricDataTableColumns.fifteen_min_avg.ToString(),  
                    ChartValueType.Double, 
                    0.0, 
                    -1, 
                    SystemMetrics.Load_Avg });
        }

        private static void ConstructNetworkMetricDetailsTable()
        {
            NetworkMetricDetails.Columns.Add(SystemMetricTableColumns.id.ToString(), typeof(int));
            NetworkMetricDetails.Columns.Add(SystemMetricTableColumns.metric.ToString(), typeof(NetworkSubMetrics));
            NetworkMetricDetails.Columns.Add(SystemMetricTableColumns.displayName.ToString(), typeof(string));
            NetworkMetricDetails.Columns.Add(SystemMetricTableColumns.title.ToString(), typeof(string));
            NetworkMetricDetails.Columns.Add(SystemMetricTableColumns.xValueMember.ToString(), typeof(string));
            NetworkMetricDetails.Columns.Add(SystemMetricTableColumns.xValueType.ToString(), typeof(ChartValueType));
            NetworkMetricDetails.Columns.Add(SystemMetricTableColumns.yValueMember.ToString(), typeof(string));
            NetworkMetricDetails.Columns.Add(SystemMetricTableColumns.yValueType.ToString(), typeof(ChartValueType));
            NetworkMetricDetails.Columns.Add(SystemMetricTableColumns.yValueMin.ToString(), typeof(double));
            NetworkMetricDetails.Columns.Add(SystemMetricTableColumns.yValueMax.ToString(), typeof(double));
            NetworkMetricDetails.Columns.Add(SystemMetricTableColumns.colorTableKey.ToString(), typeof(SystemMetrics));

            NetworkMetricDetails.Rows.Add(
                new object[] { 
                    (int)NetworkSubMetrics.Network_Rcv_Packets, 
                    NetworkSubMetrics.Network_Rcv_Packets, 
                    NetworkSubMetrics.Network_Rcv_Packets.ToString(), 
                    "Rcv Packets", 
                    NetworkMetricDataTableColumns.node_and_netid.ToString(), 
                    ChartValueType.String, 
                    NetworkMetricDataTableColumns.rcv_packets.ToString(), 
                    ChartValueType.Double, 
                    0.0, 
                    -1, 
                    SystemMetrics.Network_Rcv });

            NetworkMetricDetails.Rows.Add(
                new object[] { 
                    (int)NetworkSubMetrics.Network_Rcv_Drops, 
                    NetworkSubMetrics.Network_Rcv_Drops, 
                    NetworkSubMetrics.Network_Rcv_Drops.ToString(), 
                    "Rcv Drops", 
                    NetworkMetricDataTableColumns.node_and_netid.ToString(), 
                    ChartValueType.String, 
                    NetworkMetricDataTableColumns.rcv_drops.ToString(), 
                    ChartValueType.Double, 
                    0.0, 
                    1000, 
                    SystemMetrics.Network_Rcv });

            NetworkMetricDetails.Rows.Add(
                new object[] { 
                    (int)NetworkSubMetrics.Network_Rcv_Errs, 
                    NetworkSubMetrics.Network_Rcv_Errs, 
                    NetworkSubMetrics.Network_Rcv_Errs.ToString(), 
                    "Rcv Errs", 
                    NetworkMetricDataTableColumns.node_and_netid.ToString(), 
                    ChartValueType.String, 
                    NetworkMetricDataTableColumns.rcv_errs.ToString(),  
                    ChartValueType.Double, 
                    0.0, 
                    1000, 
                    SystemMetrics.Network_Rcv });

            NetworkMetricDetails.Rows.Add(
                new object[] { 
                    (int)NetworkSubMetrics.Network_Txn_Packets, 
                    NetworkSubMetrics.Network_Txn_Packets, 
                    NetworkSubMetrics.Network_Txn_Packets.ToString(), 
                    "Xmit Packets", 
                    NetworkMetricDataTableColumns.node_and_netid.ToString(), 
                    ChartValueType.String, 
                    NetworkMetricDataTableColumns.txn_packets.ToString(), 
                    ChartValueType.Double, 
                    0.0, 
                    -1, 
                    SystemMetrics.Network_Txn });

            NetworkMetricDetails.Rows.Add(
                new object[] { 
                    (int)NetworkSubMetrics.Network_Txn_Drops, 
                    NetworkSubMetrics.Network_Txn_Drops, 
                    NetworkSubMetrics.Network_Txn_Drops.ToString(), 
                    "Xmit Drops", 
                    NetworkMetricDataTableColumns.node_and_netid.ToString(), 
                    ChartValueType.String, 
                    NetworkMetricDataTableColumns.txn_drops.ToString(),  
                    ChartValueType.Double, 
                    0.0, 
                    1000, 
                    SystemMetrics.Network_Txn });

            NetworkMetricDetails.Rows.Add(
                new object[] { 
                    (int)NetworkSubMetrics.Network_Txn_Errs, 
                    NetworkSubMetrics.Network_Txn_Errs, 
                    NetworkSubMetrics.Network_Txn_Errs.ToString(), 
                    "Xmit Errs", 
                    NetworkMetricDataTableColumns.node_and_netid.ToString(),  
                    ChartValueType.String, 
                    NetworkMetricDataTableColumns.txn_errs.ToString(),  
                    ChartValueType.Double, 
                    0.0, 
                    1000, 
                    SystemMetrics.Network_Txn });
        }

        private static void ConstructVMMetricDetailsTable()
        {
            VMMetricDetails.Columns.Add(SystemMetricTableColumns.id.ToString(), typeof(int));
            VMMetricDetails.Columns.Add(SystemMetricTableColumns.metric.ToString(), typeof(VMSubMetrics));
            VMMetricDetails.Columns.Add(SystemMetricTableColumns.displayName.ToString(), typeof(string));
            VMMetricDetails.Columns.Add(SystemMetricTableColumns.title.ToString(), typeof(string));
            VMMetricDetails.Columns.Add(SystemMetricTableColumns.xValueMember.ToString(), typeof(string));
            VMMetricDetails.Columns.Add(SystemMetricTableColumns.xValueType.ToString(), typeof(ChartValueType));
            VMMetricDetails.Columns.Add(SystemMetricTableColumns.yValueMember.ToString(), typeof(string));
            VMMetricDetails.Columns.Add(SystemMetricTableColumns.yValueType.ToString(), typeof(ChartValueType));
            VMMetricDetails.Columns.Add(SystemMetricTableColumns.yValueMin.ToString(), typeof(double));
            VMMetricDetails.Columns.Add(SystemMetricTableColumns.yValueMax.ToString(), typeof(double));
            VMMetricDetails.Columns.Add(SystemMetricTableColumns.colorTableKey.ToString(), typeof(SystemMetrics));

            VMMetricDetails.Rows.Add(
                new object[] { 
                    (int)VMSubMetrics.VM_Swap_In, 
                    VMSubMetrics.VM_Swap_In, 
                    VMSubMetrics.VM_Swap_In.ToString(), 
                    "Swap In", 
                    VirtualMemoryMetricDataTableColumns.node_name.ToString(), 
                    ChartValueType.String, 
                    VirtualMemoryMetricDataTableColumns.swap_in.ToString(), 
                    ChartValueType.Double, 
                    0.0, 
                    -1, 
                    SystemMetrics.Virtual_Memory });

            VMMetricDetails.Rows.Add(
                new object[] { 
                    (int)VMSubMetrics.VM_Swap_Out, 
                    VMSubMetrics.VM_Swap_Out, 
                    VMSubMetrics.VM_Swap_Out.ToString(), 
                    "Swap Out", 
                    VirtualMemoryMetricDataTableColumns.node_name.ToString(), 
                    ChartValueType.String, 
                    VirtualMemoryMetricDataTableColumns.swap_out.ToString(), 
                    ChartValueType.Double, 
                    0.0, 
                    -1, 
                    SystemMetrics.Virtual_Memory });

            VMMetricDetails.Rows.Add(
                new object[] { 
                    (int)VMSubMetrics.VM_Minor_Page_Faults, 
                    VMSubMetrics.VM_Minor_Page_Faults, 
                    VMSubMetrics.VM_Minor_Page_Faults.ToString(), 
                    "Minor Page Faults", 
                    VirtualMemoryMetricDataTableColumns.node_name.ToString(), 
                    ChartValueType.String, 
                    VirtualMemoryMetricDataTableColumns.minor_page_faults.ToString(), 
                    ChartValueType.Double, 
                    0.0, 
                    -1, 
                    SystemMetrics.Virtual_Memory });

            VMMetricDetails.Rows.Add(
                new object[] { 
                    (int)VMSubMetrics.VM_Major_Page_Faults, 
                    VMSubMetrics.VM_Major_Page_Faults, 
                    VMSubMetrics.VM_Major_Page_Faults.ToString(), 
                    "Major Page Faults", 
                    VirtualMemoryMetricDataTableColumns.node_name.ToString(),  
                    ChartValueType.String, 
                    VirtualMemoryMetricDataTableColumns.major_page_faults.ToString(), 
                    ChartValueType.Double, 
                    0.0, 
                    -1, 
                    SystemMetrics.Virtual_Memory });

            VMMetricDetails.Rows.Add(
                new object[] { 
                    (int)VMSubMetrics.VM_Context_Switches, 
                    VMSubMetrics.VM_Context_Switches, 
                    VMSubMetrics.VM_Context_Switches.ToString(), 
                    "Context Switches", 
                    VirtualMemoryMetricDataTableColumns.node_name.ToString(),  
                    ChartValueType.String, 
                    VirtualMemoryMetricDataTableColumns.context_switches.ToString(), 
                    ChartValueType.Double, 
                    0.0, 
                    -1, 
                    SystemMetrics.Virtual_Memory });
        }

      
        /// <summary>
        /// Return the title of the given system metric
        /// </summary>
        /// <param name="m"></param>
        /// <returns></returns>
        public static string GetOverallSummaryTitle(SystemMetrics m)
        {
            DataRow dr = OverallSummaryMetrics.Rows.Find(m);
            return (string)dr[SystemMetricTableColumns.title.ToString()];
        }

        /// <summary>
        /// Return the XValueMember of the given system metric
        /// </summary>
        /// <param name="m"></param>
        /// <returns></returns>
        public static string GetOverallSummaryXValueMember(SystemMetrics m)
        {
            DataRow dr = OverallSummaryMetrics.Rows.Find(m);
            return (string)dr[SystemMetricTableColumns.xValueMember.ToString()];
        }

        /// <summary>
        /// Return the XValueType of the given system metric
        /// </summary>
        /// <param name="m"></param>
        /// <returns></returns>
        public static ChartValueType GetOverallSummaryXValueType(SystemMetrics m)
        {
            DataRow dr = OverallSummaryMetrics.Rows.Find(m);
            return (ChartValueType)dr[SystemMetricTableColumns.xValueType.ToString()];
        }

        /// <summary>
        /// Return the YValueMember of the given system metric
        /// </summary>
        /// <param name="m"></param>
        /// <returns></returns>
        public static string GetOverallSummaryYValueMember(SystemMetrics m)
        {
            DataRow dr = OverallSummaryMetrics.Rows.Find(m);
            return (string)dr[SystemMetricTableColumns.yValueMember.ToString()];
        }

        /// <summary>
        /// Return the YValueType of the given system metric
        /// </summary>
        /// <param name="m"></param>
        /// <returns></returns>
        public static ChartValueType GetOverallSummaryYValueType(SystemMetrics m)
        {
            DataRow dr = OverallSummaryMetrics.Rows.Find(m);
            return (ChartValueType)dr[SystemMetricTableColumns.yValueType.ToString()];
        }

        /// <summary>
        /// Return the YValyeMin of the given system metric
        /// </summary>
        /// <param name="m"></param>
        /// <returns></returns>
        public static double GetOverallSummaryYValueMin(SystemMetrics m)
        {
            DataRow dr = OverallSummaryMetrics.Rows.Find(m);
            return (double)dr[SystemMetricTableColumns.yValueMin.ToString()];
        }

        /// <summary>
        /// Return the YValueMax of the given system metric
        /// </summary>
        /// <param name="m"></param>
        /// <returns></returns>
        public static double GetOverallSummaryYValueMax(SystemMetrics m)
        {
            DataRow dr = OverallSummaryMetrics.Rows.Find(m);
            return (double)dr[SystemMetricTableColumns.yValueMax.ToString()];
        }

        public static string GetTseMetricText()
        {
            return TseMetrics[SelectedTseMetric];
        }

        public static string GetTseMetricTitle()
        {
            string metricText = TseMetrics[SelectedTseMetric];
            return string.Format(SystemMetricTseMetricTitle, metricText);
        }

        public static void GetTseMetricValue(DataRow dr, out double avgTse, out double minTse, out double maxTse)
        {
            avgTse = 0;
            minTse = 0;
            maxTse = 0;

            TseMetric selectedTseMetric = (TseMetric)Enum.Parse(typeof(TseMetric), SelectedTseMetric);
            switch (selectedTseMetric)
            {
                case TseMetric.requests:
                    maxTse = (double)dr[TseMetricDataTableColumns.max_requests.ToString()];
                    minTse = (double)dr[TseMetricDataTableColumns.min_requests.ToString()];
                    avgTse = (double)dr[TseMetricDataTableColumns.requests.ToString()];
                    break;

                case TseMetric.service_time:
                    maxTse = (double)dr[TseMetricDataTableColumns.max_service_time.ToString()];
                    minTse = (double)dr[TseMetricDataTableColumns.min_service_time.ToString()];
                    avgTse = (double)dr[TseMetricDataTableColumns.service_time.ToString()];
                    break;

                case TseMetric.ase_service_time:
                    maxTse = (double)dr[TseMetricDataTableColumns.max_ase_service_time.ToString()];
                    minTse = (double)dr[TseMetricDataTableColumns.min_ase_service_time.ToString()];
                    avgTse = (double)dr[TseMetricDataTableColumns.ase_service_time.ToString()];
                    break;

                case TseMetric.request_io_wait_time:
                    maxTse = (double)dr[TseMetricDataTableColumns.max_request_io_wait_time.ToString()];
                    minTse = (double)dr[TseMetricDataTableColumns.min_request_io_wait_time.ToString()];
                    avgTse = (double)dr[TseMetricDataTableColumns.request_io_wait_time.ToString()];
                    break;

                case TseMetric.ready_list_count:
                    maxTse = (double)dr[TseMetricDataTableColumns.max_ready_list_count.ToString()];
                    minTse = (double)dr[TseMetricDataTableColumns.min_ready_list_count.ToString()];
                    avgTse = (double)dr[TseMetricDataTableColumns.ready_list_count.ToString()];
                    break;
            }
        }

        public static string GetTseMetricColumnName()
        {
            string columnName = string.Empty;

            TseMetric selectedTseMetric = (TseMetric)Enum.Parse(typeof(TseMetric), SelectedTseMetric);
            switch (selectedTseMetric)
            {
                case TseMetric.requests:
                    columnName = TseMetricDataTableColumns.requests.ToString();
                    break;

                case TseMetric.service_time:
                    columnName = TseMetricDataTableColumns.service_time.ToString();
                    break;

                case TseMetric.ase_service_time:
                    columnName = TseMetricDataTableColumns.ase_service_time.ToString();
                    break;

                case TseMetric.request_io_wait_time:
                    columnName = TseMetricDataTableColumns.request_io_wait_time.ToString();
                    break;

                case TseMetric.ready_list_count:
                    columnName = TseMetricDataTableColumns.ready_list_count.ToString();
                    break;
            }

            return columnName;
        }
        
        public static DataTable TransformSystemMetricProtoBuf(ConnectionDefinition aConnectionDefinition, string key, object aStats, int aNodeID, out DateTime aCurrentTS, int diskIoNodeCount)
        {
            if (key == LiveFeedRoutingKeyMapper.PUBS_performance_tse_metrics_asm)
            {
                DataTable table = TransformTseMetricProtobufToSummaryDataTable(aStats, aNodeID, diskIoNodeCount);
                aCurrentTS = (table.Rows.Count > 0) ? (DateTime)table.Rows[0][SystemMetricModel.VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString()] : DateTime.Now;
                return table;
            }
            else
            {
                return TransformSystemMetricProtoBuf(aConnectionDefinition, key, aStats, aNodeID, out aCurrentTS);
            }
        }

        /// <summary>
        /// To transform a protobuf into a performance metric data table
        /// [TBD] It would be even better if we could move this completely to LiveFeedStatsTransformer
        ///       so that we could streamline the transformation. Now, it's kinda of two steps.
        /// </summary>
        /// <param name="key"></param>
        /// <param name="aStats"></param>
        /// <returns></returns>
        public static DataTable TransformSystemMetricProtoBuf(ConnectionDefinition aConnectionDefinition, string key, object aStats, int aNodeID, out DateTime aCurrentTS)
        {
            DataTable t = null;
            DataTable table = null;
            DateTime currentTS = DateTime.Now;

            switch (key)
            {
                case LiveFeedRoutingKeyMapper.PUBS_performance_core_metrics_asm:                    
                    if (aConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
                    {
                        table = TransformCoreMetricProtobufToSummaryDataTable(aStats, aNodeID);
                    }
                    else 
                    {
                        table = TransformNodeMetricProtobufToSummaryDataTable(aStats, aNodeID);
                    }                    
                    currentTS = (table.Rows.Count > 0) ? (DateTime)table.Rows[0][SystemMetricModel.VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString()] : DateTime.Now; break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_disk_metrics_asm:
                    if (aConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ151)
                    {
                        table = TransformDiskMetricProtobufToSummaryDataTableAvgMax(aStats, aNodeID);
                    }
                    else 
                    {
                        table = TransformDiskMetricProtobufToSummaryDataTable(aStats, aNodeID);
                    }                    
                    currentTS = (table.Rows.Count > 0) ? (DateTime)table.Rows[0][SystemMetricModel.VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString()] : DateTime.Now;                    break;                

                case LiveFeedRoutingKeyMapper.PUBS_performance_memory_metrics_asm:
                    table = TransformMemoryMetricProtobufToSummaryDataTable(aStats, aNodeID);
                    currentTS = (table.Rows.Count > 0) ? (DateTime)table.Rows[0][SystemMetricModel.VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString()] : DateTime.Now;                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_loadavg_metrics_asm:
                    table = TransformLoadAvgMetricProtobufToSummaryDataTable(aStats, aNodeID);
                    currentTS = (table.Rows.Count > 0) ? (DateTime)table.Rows[0][SystemMetricModel.VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString()] : DateTime.Now;                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_virtualmem_metrics_asm:
                    if (aConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ130)
                    {
                        table = TransformVirtualMemMetricProtobufToSummaryDataTable_M7(aStats, aNodeID);
                    }
                    else 
                    {
                        table = TransformVirtualMemMetricProtobufToSummaryDataTable(aStats, aNodeID);
                    }                    
                    currentTS = (table.Rows.Count > 0) ? (DateTime)table.Rows[0][SystemMetricModel.VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString()] : DateTime.Now;
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_network_metrics_asm:
                    table = TransformNetworkMetricProtobufToSummaryDataTable(aStats, aNodeID);
                    currentTS = (table.Rows.Count > 0) ? (DateTime)table.Rows[0][SystemMetricModel.VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString()] : DateTime.Now;                    break;

                case LiveFeedRoutingKeyMapper.PUBS_performance_filesys_metrics_asm:
                    table = TransformFileSysMetricProtobufToSummaryDataTable(aStats, aNodeID);
                    currentTS = (table.Rows.Count > 0) ? (DateTime)table.Rows[0][SystemMetricModel.VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString()] : DateTime.Now;                    break;
            }

            aCurrentTS = currentTS;
            return table;
        }

        public static DataTable TransformNodeMetricProtobufToSummaryDataTable(object aStats, int aNodeID)
        {
            DataTable table = new DataTable();
            table.TableName = "CoreMetricSummaryDataTable";
            table.Columns.Add(CoreMetricDataTableColumns.node_id.ToString(), typeof(int));
            table.Columns.Add(CoreMetricDataTableColumns.total_nodes.ToString(), typeof(int));
            table.Columns.Add(CoreMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));            
            table.Columns.Add(CoreMetricDataTableColumns.avg_core_total.ToString(), typeof(double));
            table.Columns.Add(CoreMetricDataTableColumns.zero.ToString(), typeof(int));

            linuxcounters.core_metrics_shortbusy_assembled metrics = aStats as linuxcounters.core_metrics_shortbusy_assembled;
            if (metrics != null)
            {
                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    linuxcounters.core_metrics_shortbusy_assembled.aggregate_core_buffer buffer = new linuxcounters.core_metrics_shortbusy_assembled.aggregate_core_buffer();
                    System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();
                    string traceMessage = string.Format("Received publcation [{0}]: aggregate = {1} {2}",
                                                  LiveFeedRoutingKeyMapper.RKEY_performance_core_metrics_asm,
                                                  metrics.aggregate.Count,
                                                  Environment.NewLine);

                    for (int i = 0; i < metrics.aggregate.Count; i++)
                    {
                        traceMessage += string.Format(" aggregate_count[{0}]:", i);
                        // add count properties first
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
                    if (aNodeID != -1 && aNodeID != i)
                    {
                        continue;
                    }

                    try
                    {
                        DataRow dr = table.NewRow();
                        dr[CoreMetricDataTableColumns.node_id.ToString()] = metrics.aggregate[i].node_id;
                        dr[CoreMetricDataTableColumns.total_nodes.ToString()] = metrics.aggregate.Count;                        
                        dr[CoreMetricDataTableColumns.avg_core_total.ToString()] = metrics.aggregate[i].avg_core_total;
                        dr[CoreMetricDataTableColumns.gen_time_ts_lct.ToString()] = Utilities.GetFormattedTimeFromUnixTimestampLCT(metrics.header.info_generation_time_ts_lct);
                        dr[CoreMetricDataTableColumns.zero.ToString()] = 0;
                        table.Rows.Add(dr);
                    }
                    catch (Exception ex)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                           String.Format("Received core metric publication: get value exception: {0}", ex.Message));
                    }
                }
            }
            return table;
        }

        public static DataTable TransformCoreMetricProtobufToSummaryDataTable(object aStats, int aNodeID)
        {
            DataTable table = new DataTable();
            table.TableName = "CoreMetricSummaryDataTable";
            table.Columns.Add(CoreMetricDataTableColumns.node_id.ToString(), typeof(int));
            table.Columns.Add(CoreMetricDataTableColumns.total_nodes.ToString(), typeof(int));
            table.Columns.Add(CoreMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
            table.Columns.Add(CoreMetricDataTableColumns.core_id.ToString(), typeof(int));
            table.Columns.Add(CoreMetricDataTableColumns.node_and_core_id.ToString(), typeof(string));
            table.Columns.Add(CoreMetricDataTableColumns.avg_core_total.ToString(), typeof(double));
            table.Columns.Add(CoreMetricDataTableColumns.zero.ToString(), typeof(int));

            linuxcounters.core_metrics_shortbusy_assembled metrics = aStats as linuxcounters.core_metrics_shortbusy_assembled;
            if (metrics != null)
            {
                int core_idx = 0;
                int total_aggregates = metrics.aggregate.Count;

                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    linuxcounters.core_metrics_shortbusy_assembled.aggregate_core_count core_count = new linuxcounters.core_metrics_shortbusy_assembled.aggregate_core_count();
                    System.Reflection.PropertyInfo[] count_properties = core_count.GetType().GetProperties();
                    string traceMessage = string.Format("Received publcation [{0}]: aggregate_count = {1}, aggregate = {2} {3}",
                                                  LiveFeedRoutingKeyMapper.RKEY_performance_core_metrics_asm,
                                                  metrics.aggregate_count.Count,
                                                  metrics.aggregate.Count,
                                                  Environment.NewLine);

                    for (int i = 0; i < metrics.aggregate_count.Count; i++)
                    {
                        traceMessage += string.Format(" aggregate_count[{0}]:", i);
                        // add count properties first
                        foreach (System.Reflection.PropertyInfo c_property in count_properties)
                        {
                            try
                            {
                                traceMessage += string.Format(" {0} = {1},",
                                                              c_property.Name,
                                                              c_property.GetValue(metrics.aggregate_count[i], null));
                            }
                            catch (Exception ex)
                            {
                                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                   String.Format("Received core metric publication: property [{0}] get value exception: {1}",
                                                                 c_property.Name, ex.Message));
                            }
                        }
                        traceMessage += Environment.NewLine;
                    }

                    Logger.OutputToLog(TraceOptions.TraceOption.LIVEFEEDDATA, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, traceMessage);
                }

                // fill up one node at a time.
                for (int i = 0; i < metrics.aggregate_count.Count; i++)
                {
                    int numcores = metrics.aggregate_count[i].numcores;
                    if (numcores > 0)
                    {
                        if (aNodeID != -1 && aNodeID != i)
                        {
                            core_idx += numcores;
                            continue;
                        }

                        for (int j = 0; j < numcores; j++)
                        {
                            if ((core_idx + j) < total_aggregates)
                            {
                                try
                                {
                                    DataRow dr = table.NewRow();
                                    dr[CoreMetricDataTableColumns.node_id.ToString()] = metrics.aggregate[core_idx + j].node_id;
                                    dr[CoreMetricDataTableColumns.core_id.ToString()] = metrics.aggregate[core_idx + j].processor;
                                    dr[CoreMetricDataTableColumns.node_and_core_id.ToString()] = string.Format("{0}:{1}", metrics.aggregate[core_idx + j].node_id, metrics.aggregate[core_idx + j].processor);
                                    dr[CoreMetricDataTableColumns.avg_core_total.ToString()] = metrics.aggregate[core_idx + j].avg_core_total;
                                    dr[CoreMetricDataTableColumns.total_nodes.ToString()] = metrics.aggregate_count.Count;
                                    dr[CoreMetricDataTableColumns.gen_time_ts_lct.ToString()] = Utilities.GetFormattedTimeFromUnixTimestampLCT(metrics.header.info_generation_time_ts_lct);
                                    dr[CoreMetricDataTableColumns.zero.ToString()] = 0;
                                    table.Rows.Add(dr);
                                }
                                catch (Exception ex)
                                {
                                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                       String.Format("Received core metric publication: get value exception: {0}", ex.Message));
                                }
                            }
                            else
                            {
                                // [TBD] should emit an exception, but now just return
                                return table;
                            }
                        }
                        core_idx += numcores;
                    }
                    else
                    {
                        if (aNodeID != -1 && aNodeID != i)
                        {
                            if (numcores == -1)
                            {
                                core_idx += 1;
                            }
                            continue;
                        }

                        DataRow dr = table.NewRow();

                        dr[CoreMetricDataTableColumns.node_id.ToString()] = i;
                        dr[CoreMetricDataTableColumns.core_id.ToString()] = numcores;
                        dr[CoreMetricDataTableColumns.node_and_core_id.ToString()] = string.Format("{0}:{1}", i, "na");
                        dr[CoreMetricDataTableColumns.avg_core_total.ToString()] = numcores;
                        dr[CoreMetricDataTableColumns.total_nodes.ToString()] = metrics.aggregate_count.Count;
                        dr[CoreMetricDataTableColumns.gen_time_ts_lct.ToString()] = Utilities.GetFormattedTimeFromUnixTimestampLCT(metrics.header.info_generation_time_ts_lct);
                        dr[CoreMetricDataTableColumns.zero.ToString()] = 0;

                        table.Rows.Add(dr);
                        if (numcores == -1)
                        {
                            core_idx += 1;
                        }
                    }
                }
            }

            return table;
        }


        public static DataTable TransformMemoryMetricProtobufToSummaryDataTable(object aStats, int aNodeID)
        {
            DataTable table = new DataTable();
            table.TableName = "MemoryMetricSummaryDataTable";
            table.Columns.Add(MemoryMetricDataTableColumns.node_id.ToString(), typeof(int));
            table.Columns.Add(MemoryMetricDataTableColumns.total_nodes.ToString(), typeof(int));
            table.Columns.Add(MemoryMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
            table.Columns.Add(MemoryMetricDataTableColumns.node_name.ToString(), typeof(string));
            table.Columns.Add(MemoryMetricDataTableColumns.memory_used.ToString(), typeof(double));
            table.Columns.Add(MemoryMetricDataTableColumns.swap_used.ToString(), typeof(double));
            table.Columns.Add(MemoryMetricDataTableColumns.zero.ToString(), typeof(int));

            linuxcounters.memory_metrics_shortbusy_assembled metrics = aStats as linuxcounters.memory_metrics_shortbusy_assembled;
            if (metrics != null)
            {
                int core_idx = 0;

                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    linuxcounters.memory_metrics_shortbusy_assembled.aggregate_buffer buffer = new linuxcounters.memory_metrics_shortbusy_assembled.aggregate_buffer();
                    System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();

                    string traceMessage = string.Format("Received publcation [{0}]: aggregate = {1} {2}",
                                                  LiveFeedRoutingKeyMapper.RKEY_performance_memory_metrics_asm,
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
                    if (aNodeID != -1 && aNodeID != i)
                    {
                        continue;
                    }

                    try
                    {
                        DataRow dr = table.NewRow();
                        dr[MemoryMetricDataTableColumns.node_id.ToString()] = metrics.aggregate[i].node_id;
                        dr[MemoryMetricDataTableColumns.total_nodes.ToString()] = metrics.aggregate.Count;
                        if (metrics.aggregate[i].node_id == -1)
                        {
                            dr[MemoryMetricDataTableColumns.node_name.ToString()] = "na";
                        }
                        else
                        {
                            dr[MemoryMetricDataTableColumns.node_name.ToString()] = string.Format("{0}", metrics.aggregate[i].node_id);
                        }
                        dr[MemoryMetricDataTableColumns.memory_used.ToString()] = metrics.aggregate[i].memory_percent_used;
                        dr[MemoryMetricDataTableColumns.swap_used.ToString()] = metrics.aggregate[i].swap_percent_used;
                        dr[MemoryMetricDataTableColumns.gen_time_ts_lct.ToString()] = Utilities.GetFormattedTimeFromUnixTimestampLCT(metrics.header.info_generation_time_ts_lct);
                        dr[MemoryMetricDataTableColumns.zero.ToString()] = 0;
                        table.Rows.Add(dr);
                    }
                    catch (Exception ex)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                           String.Format("Received memory metric publication: get value exception: {0}", ex.Message));
                    }
                }
            }

            return table;
        }

        public static DataTable TransformDiskMetricProtobufToSummaryDataTable(object aStats, int aNodeID)
        {
            DataTable table = new DataTable();
            table.TableName = "DiskMetricSummaryDataTable";
            table.Columns.Add(DiskMetricDataTableColumns.node_id.ToString(), typeof(int));
            table.Columns.Add(DiskMetricDataTableColumns.total_nodes.ToString(), typeof(int));
            table.Columns.Add(DiskMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));            
            table.Columns.Add(DiskMetricDataTableColumns.reads_and_writes.ToString(), typeof(double));
            table.Columns.Add(DiskMetricDataTableColumns.zero.ToString(), typeof(int));

            linuxcounters.disk_metrics_shortbusy_assembled metrics = aStats as linuxcounters.disk_metrics_shortbusy_assembled;

            if (metrics != null)
            {
                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {//reads_plus_writes
                    linuxcounters.disk_metrics_shortbusy_assembled.aggregate_diskios_buffer buffer = new linuxcounters.disk_metrics_shortbusy_assembled.aggregate_diskios_buffer();
                    System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();
                    string traceMessage = string.Format("Received publcation [{0}]: aggregate = {1} {2}",
                                                  LiveFeedRoutingKeyMapper.RKEY_performance_disk_metrics_asm,
                                                  metrics.aggregate.Count,
                                                  Environment.NewLine);

                    for (int i = 0; i < metrics.aggregate.Count; i++)
                    {
                        traceMessage += string.Format(" aggregate_count[{0}]:", i);
                        // add count properties first
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
                                                   String.Format("Received disk metric publication: property [{0}] get value exception: {1}",
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
                    if (aNodeID != -1 && aNodeID != i)
                    {
                        continue;
                    }

                    try
                    {
                        DataRow dr = table.NewRow();
                        dr[DiskMetricDataTableColumns.node_id.ToString()] = metrics.aggregate[i].node_id;
                        dr[DiskMetricDataTableColumns.total_nodes.ToString()] = metrics.aggregate.Count;
                        dr[DiskMetricDataTableColumns.reads_and_writes.ToString()] = metrics.aggregate[i].reads_plus_writes;
                        dr[CoreMetricDataTableColumns.gen_time_ts_lct.ToString()] = Utilities.GetFormattedTimeFromUnixTimestampLCT(metrics.header.info_generation_time_ts_lct);
                        dr[CoreMetricDataTableColumns.zero.ToString()] = 0;
                        table.Rows.Add(dr);
                    }
                    catch (Exception ex)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                           String.Format("Received disk metric publication: get value exception: {0}", ex.Message));
                    }
                }
            }

            return table;
        }

        public static DataTable TransformTseMetricProtobufToSummaryDataTable(object stats, int selectedNodeId, int diskIoNodeCount)
        {
            DataTable table = new DataTable();
            table.TableName = "TseMetricSummaryDataTable";
            table.Columns.Add(TseMetricDataTableColumns.node_id.ToString(), typeof(int));
            table.Columns.Add(TseMetricDataTableColumns.total_nodes.ToString(), typeof(int));
            table.Columns.Add(TseMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
            table.Columns.Add(TseMetricDataTableColumns.node_and_device_name.ToString(), typeof(string));
            table.Columns.Add(TseMetricDataTableColumns.service_time.ToString(), typeof(double));
            table.Columns.Add(TseMetricDataTableColumns.requests.ToString(), typeof(double));
            table.Columns.Add(TseMetricDataTableColumns.ase_service_time.ToString(), typeof(double));
            table.Columns.Add(TseMetricDataTableColumns.request_io_wait_time.ToString(), typeof(double));
            table.Columns.Add(TseMetricDataTableColumns.ready_list_count.ToString(), typeof(double));
            table.Columns.Add(TseMetricDataTableColumns.zero.ToString(), typeof(int));

            if (diskIoNodeCount == 0) return table;

            se.perf_stats_delta_assembled metrics = stats as se.perf_stats_delta_assembled;
            if (metrics != null)
            {
                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    se.perf_stats_delta_assembled.device_delta_data device = new se.perf_stats_delta_assembled.device_delta_data();
                    System.Reflection.PropertyInfo[] deviceProperties = device.GetType().GetProperties();

                    string traceMessage = string.Format("Received publcation [{0}]: device_count_aggregate = {1}, data = {2} {3}",
                                                  LiveFeedRoutingKeyMapper.RKEY_performance_tse_metrics_asm,
                                                  metrics.device_count_aggregate.Count,
                                                  metrics.data.Count,
                                                  Environment.NewLine);

                    for (int i = 0; i < metrics.data.Count; i++)
                    {
                        traceMessage += string.Format(" data[{0}]:", i);
                        // add count properties first
                        foreach (System.Reflection.PropertyInfo c_property in deviceProperties)
                        {
                            try
                            {
                                traceMessage += string.Format(" {0} = {1},",
                                                              c_property.Name,
                                                              c_property.GetValue(metrics.data[i], null));
                            }
                            catch (Exception ex)
                            {
                                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                   String.Format("Received disk metric publication: property [{0}] get value exception: {1}",
                                                                 c_property.Name, ex.Message));
                            }
                        }
                        traceMessage += Environment.NewLine;
                    }

                    Logger.OutputToLog(TraceOptions.TraceOption.LIVEFEEDDATA, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, traceMessage);
                }

                int totalDeviceCount = metrics.data.Count;
                for (int completeDeviceIndex = 0; completeDeviceIndex < totalDeviceCount; completeDeviceIndex++)
                {
                    try
                    {
                        int deviceNodeId = metrics.data[completeDeviceIndex].node_id;
                        if (deviceNodeId >= 0)
                        {
                            if (selectedNodeId >= 0 && selectedNodeId != deviceNodeId)
                            {
                                continue;
                            }

                            string nodeAndDevice = string.Format("{0}:{1}", metrics.data[completeDeviceIndex].node_id, metrics.data[completeDeviceIndex].volume_name);

                            // Clear possible duplicated ones
                            DataRow[] duplicatedDevices = table.Select(string.Format("{0} = '{1}'", TseMetricDataTableColumns.node_and_device_name, nodeAndDevice));
                            if (duplicatedDevices != null && duplicatedDevices.Length > 0)
                            {
                                foreach (DataRow duplicatedDevice in duplicatedDevices)
                                {
                                    table.Rows.Remove(duplicatedDevice);
                                }
                            }

                            DataRow dr = table.NewRow();
                            dr[TseMetricDataTableColumns.node_id.ToString()] = deviceNodeId;
                            dr[TseMetricDataTableColumns.node_and_device_name.ToString()] = nodeAndDevice;
                            dr[TseMetricDataTableColumns.service_time.ToString()] = metrics.data[completeDeviceIndex].service_time;
                            dr[TseMetricDataTableColumns.requests.ToString()] = metrics.data[completeDeviceIndex].requests;
                            dr[TseMetricDataTableColumns.ase_service_time.ToString()] = metrics.data[completeDeviceIndex].ase_service_time;
                            dr[TseMetricDataTableColumns.request_io_wait_time.ToString()] = metrics.data[completeDeviceIndex].request_io_wait_time;
                            dr[TseMetricDataTableColumns.ready_list_count.ToString()] = metrics.data[completeDeviceIndex].ready_list_count;
                            dr[TseMetricDataTableColumns.total_nodes.ToString()] = diskIoNodeCount;
                            dr[TseMetricDataTableColumns.gen_time_ts_lct.ToString()] = Utilities.GetFormattedTimeFromUnixTimestampLCT(metrics.header.info_generation_time_ts_lct);
                            dr[TseMetricDataTableColumns.zero.ToString()] = 0;
                            table.Rows.Add(dr);
                        }
                    }
                    catch (Exception ex)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                            String.Format("Received disk metric publication: get value exception: {0}", ex.Message));
                    }
                }

                int nodeCount = diskIoNodeCount;
                for (int i = 0; i < nodeCount; i++)
                {
                    if (selectedNodeId != -1 && selectedNodeId != i)
                    {
                        continue;
                    }

                    DataRow[] nodeDevices = table.Select(string.Format("{0} = {1}", TseMetricDataTableColumns.node_id.ToString(), i));
                    if (nodeDevices == null || nodeDevices.Length == 0)
                    {
                        DataRow dr = table.NewRow();
                        dr[TseMetricDataTableColumns.node_id.ToString()] = i;
                        dr[TseMetricDataTableColumns.node_and_device_name.ToString()] = string.Format("{0}:{1}", i, "na");
                        dr[TseMetricDataTableColumns.service_time.ToString()] = 0;
                        dr[TseMetricDataTableColumns.requests.ToString()] = 0;
                        dr[TseMetricDataTableColumns.ase_service_time.ToString()] = 0;
                        dr[TseMetricDataTableColumns.request_io_wait_time.ToString()] = 0;
                        dr[TseMetricDataTableColumns.ready_list_count.ToString()] = 0;
                        dr[TseMetricDataTableColumns.total_nodes.ToString()] = diskIoNodeCount;
                        dr[TseMetricDataTableColumns.gen_time_ts_lct.ToString()] = Utilities.GetFormattedTimeFromUnixTimestampLCT(metrics.header.info_generation_time_ts_lct);
                        dr[TseMetricDataTableColumns.zero.ToString()] = 0;
                        table.Rows.Add(dr);
                    }
                }
 
            }

            return table;
        }

        public static DataTable TransformDiskMetricProtobufToSummaryDataTableAvgMax(object aStats, int aNodeID)
        {
            DataTable table = new DataTable();
            table.TableName = "DiskMetricSummaryDataTable";
            table.Columns.Add(DiskMetricDataTableColumns.node_id.ToString(), typeof(int));
            table.Columns.Add(DiskMetricDataTableColumns.total_nodes.ToString(), typeof(int));
            table.Columns.Add(DiskMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
            table.Columns.Add(DiskMetricDataTableColumns.node_and_device_name.ToString(), typeof(string));
            table.Columns.Add(DiskMetricDataTableColumns.reads_and_writes.ToString(), typeof(double));
            table.Columns.Add(DiskMetricDataTableColumns.zero.ToString(), typeof(int));

            linuxcounters.disk_metrics_shortbusy_assembled metrics = aStats as linuxcounters.disk_metrics_shortbusy_assembled;
            if (metrics != null)
            {
                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    linuxcounters.disk_metrics_shortbusy_assembled.aggregate_device_count device_count = new linuxcounters.disk_metrics_shortbusy_assembled.aggregate_device_count();
                    System.Reflection.PropertyInfo[] count_properties = device_count.GetType().GetProperties();

                    string traceMessage = string.Format("Received publcation [{0}]: device_count_aggregate = {1}, aggregate = {2} {3}",
                                                  LiveFeedRoutingKeyMapper.RKEY_performance_disk_metrics_asm,
                                                  metrics.device_count_aggregate.Count,
                                                  metrics.aggregate.Count,
                                                  Environment.NewLine);

                    for (int i = 0; i < metrics.device_count_aggregate.Count; i++)
                    {
                        traceMessage += string.Format(" device_count_aggregate[{0}]:", i);
                        // add count properties first
                        foreach (System.Reflection.PropertyInfo c_property in count_properties)
                        {
                            try
                            {
                                traceMessage += string.Format(" {0} = {1},",
                                                              c_property.Name,
                                                              c_property.GetValue(metrics.device_count_aggregate[i], null));
                            }
                            catch (Exception ex)
                            {
                                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                   String.Format("Received disk metric publication: property [{0}] get value exception: {1}",
                                                                 c_property.Name, ex.Message));
                            }
                        }
                        traceMessage += Environment.NewLine;
                    }

                    Logger.OutputToLog(TraceOptions.TraceOption.LIVEFEEDDATA, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, traceMessage);
                }

                int core_idx = 0;
                int total_aggregates = metrics.aggregate.Count;

                // fill up one node at a time.
                for (int i = 0; i < metrics.device_count_aggregate.Count; i++)
                {
                    int num = metrics.device_count_aggregate[i].num_storage_devices;
                    if (num > 0)
                    {
                        if (aNodeID != -1 && aNodeID != i)
                        {
                            core_idx += num;
                            continue;
                        }

                        for (int j = 0; j < num; j++)
                        {
                            if ((core_idx + j) < total_aggregates)
                            {
                                try
                                {
                                    DataRow dr = table.NewRow();
                                    dr[DiskMetricDataTableColumns.node_id.ToString()] = metrics.aggregate[core_idx + j].node_id;
                                    dr[DiskMetricDataTableColumns.node_and_device_name.ToString()] = string.Format("{0}:{1}", metrics.aggregate[core_idx + j].node_id, metrics.aggregate[core_idx + j].interface_name);
                                    dr[DiskMetricDataTableColumns.reads_and_writes.ToString()] = metrics.aggregate[core_idx + j].reads_plus_writes;
                                    dr[DiskMetricDataTableColumns.total_nodes.ToString()] = metrics.device_count_aggregate.Count;
                                    dr[DiskMetricDataTableColumns.gen_time_ts_lct.ToString()] = Utilities.GetFormattedTimeFromUnixTimestampLCT(metrics.header.info_generation_time_ts_lct);
                                    dr[DiskMetricDataTableColumns.zero.ToString()] = 0;
                                    table.Rows.Add(dr);
                                }
                                catch (Exception ex)
                                {
                                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                        String.Format("Received disk metric publication: get value exception: {0}", ex.Message));
                                }
                            }
                            else
                            {
                                // [TBD] should emit an exception, but now just return
                                return table;
                            }
                        }
                        core_idx += num;
                    }
                    else
                    {
                        if (aNodeID != -1 && aNodeID != i)
                        {
                            if (num == -1)
                            {
                                core_idx += 1;
                            }
                            continue;
                        }

                        DataRow dr = table.NewRow();
                        dr[DiskMetricDataTableColumns.node_id.ToString()] = i;
                        dr[DiskMetricDataTableColumns.node_and_device_name.ToString()] = string.Format("{0}:{1}", i, "na");
                        dr[DiskMetricDataTableColumns.reads_and_writes.ToString()] = num;
                        dr[DiskMetricDataTableColumns.total_nodes.ToString()] = metrics.device_count_aggregate.Count;
                        dr[DiskMetricDataTableColumns.gen_time_ts_lct.ToString()] = Utilities.GetFormattedTimeFromUnixTimestampLCT(metrics.header.info_generation_time_ts_lct);
                        dr[DiskMetricDataTableColumns.zero.ToString()] = 0;
                        table.Rows.Add(dr);

                        if (num == -1)
                        {
                            core_idx += 1;
                        }
                    }
                }
            }

            return table;
        }

        public static DataTable TransformFileSysMetricProtobufToSummaryDataTable(object aStats, int aNodeID)
        {
            DataTable table = new DataTable();
            table.TableName = "FileSysMetricSummaryDataTable";
            table.Columns.Add(FileSysMetricDataTableColumns.node_id.ToString(), typeof(int));
            table.Columns.Add(FileSysMetricDataTableColumns.total_nodes.ToString(), typeof(int));
            table.Columns.Add(FileSysMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
            table.Columns.Add(FileSysMetricDataTableColumns.node_and_fsname.ToString(), typeof(string));
            table.Columns.Add(FileSysMetricDataTableColumns.percent_consumed.ToString(), typeof(double));
            table.Columns.Add(FileSysMetricDataTableColumns.zero.ToString(), typeof(int));

            linuxcounters.filesystem_metrics_shortbusy_assembled metrics = aStats as linuxcounters.filesystem_metrics_shortbusy_assembled;
            if (metrics != null)
            {
                int core_idx = 0;
                int total_aggregates = metrics.aggregate.Count;

                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    linuxcounters.filesystem_metrics_shortbusy_assembled.aggregate_filesystem_count filesys_count = new linuxcounters.filesystem_metrics_shortbusy_assembled.aggregate_filesystem_count();
                    System.Reflection.PropertyInfo[] count_properties = filesys_count.GetType().GetProperties();
                    string traceMessage = string.Format("Received publcation [{0}]: aggregate_count = {1}, aggregate = {2} {3}",
                                                  LiveFeedRoutingKeyMapper.RKEY_performance_filesys_metrics_asm,
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

                    Logger.OutputToLog(TraceOptions.TraceOption.LIVEFEEDDATA, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, traceMessage);
                }

                // fill up one node at a time.
                for (int i = 0; i < metrics.filesystem_count_aggr.Count; i++)
                {
                    int num = metrics.filesystem_count_aggr[i].num_file_systems;
                    if (num > 0)
                    {
                        if (aNodeID != -1 && aNodeID != i)
                        {
                            core_idx += num;
                            continue;
                        }

                        for (int j = 0; j < num; j++)
                        {
                            if ((core_idx + j) < total_aggregates)
                            {
                                try
                                {
                                    DataRow dr = table.NewRow();
                                    dr[FileSysMetricDataTableColumns.node_id.ToString()] = metrics.aggregate[core_idx + j].node_id;
                                    dr[FileSysMetricDataTableColumns.node_and_fsname.ToString()] = string.Format("{0}:{1}", metrics.aggregate[core_idx + j].node_id, metrics.aggregate[core_idx + j].mnt_fsname);
                                    dr[FileSysMetricDataTableColumns.percent_consumed.ToString()] = metrics.aggregate[core_idx + j].percent_consumed;
                                    dr[FileSysMetricDataTableColumns.total_nodes.ToString()] = metrics.filesystem_count_aggr.Count;
                                    dr[FileSysMetricDataTableColumns.gen_time_ts_lct.ToString()] = Utilities.GetFormattedTimeFromUnixTimestampLCT(metrics.header.info_generation_time_ts_lct);
                                    dr[FileSysMetricDataTableColumns.zero.ToString()] = 0;
                                    table.Rows.Add(dr);
                                }
                                catch (Exception ex)
                                {
                                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                        String.Format("Received file system metric publication: get value exception: {0}", ex.Message));
                                }
                            }
                            else
                            {
                                // [TBD] should emit an exception, but now just return
                                return table;
                            }
                        }
                        core_idx += num;
                    }
                    else
                    {
                        if (aNodeID != -1 && aNodeID != i)
                        {
                            if (num == -1)
                            {
                                core_idx += 1;
                            }
                            continue;
                        }

                        DataRow dr = table.NewRow();
                        dr[FileSysMetricDataTableColumns.node_id.ToString()] = i;
                        dr[FileSysMetricDataTableColumns.node_and_fsname.ToString()] = string.Format("{0}:{1}", i, "na");
                        dr[FileSysMetricDataTableColumns.percent_consumed.ToString()] = num;
                        dr[FileSysMetricDataTableColumns.total_nodes.ToString()] = metrics.filesystem_count_aggr.Count;
                        dr[FileSysMetricDataTableColumns.gen_time_ts_lct.ToString()] = Utilities.GetFormattedTimeFromUnixTimestampLCT(metrics.header.info_generation_time_ts_lct);
                        dr[FileSysMetricDataTableColumns.zero.ToString()] = 0;
                        table.Rows.Add(dr);

                        if (num == -1)
                        {
                            // advance to the next only for -1.
                            core_idx += 1;
                        }
                    }
                }
            }

            return table;
        }

        public static DataTable TransformLoadAvgMetricProtobufToSummaryDataTable(object aStats, int aNodeID)
        {
            DataTable table = new DataTable();
            table.TableName = "LoadAvgMetricSummaryDataTable";
            table.Columns.Add(LoadAvgMetricDataTableColumns.node_id.ToString(), typeof(int));
            table.Columns.Add(LoadAvgMetricDataTableColumns.total_nodes.ToString(), typeof(int));
            table.Columns.Add(LoadAvgMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
            table.Columns.Add(LoadAvgMetricDataTableColumns.node_name.ToString(), typeof(string));
            table.Columns.Add(LoadAvgMetricDataTableColumns.one_min_avg.ToString(), typeof(double));
            table.Columns.Add(LoadAvgMetricDataTableColumns.five_min_avg.ToString(), typeof(double));
            table.Columns.Add(LoadAvgMetricDataTableColumns.fifteen_min_avg.ToString(), typeof(double));
            table.Columns.Add(LoadAvgMetricDataTableColumns.zero.ToString(), typeof(int));

            linuxcounters.loadavg_metrics_shortbusy_assembled metrics = aStats as linuxcounters.loadavg_metrics_shortbusy_assembled;
            if (metrics != null)
            {
                int core_idx = 0;

                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    linuxcounters.loadavg_metrics_shortbusy_assembled.aggregate_buffer buffer = new linuxcounters.loadavg_metrics_shortbusy_assembled.aggregate_buffer();
                    System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();
                    string traceMessage = string.Format("Received publcation [{0}]: aggregate = {1} {2}",
                                                  LiveFeedRoutingKeyMapper.RKEY_performance_loadavg_metrics_asm,
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
                    if (aNodeID != -1 && aNodeID != i)
                    {
                        continue;
                    }

                    try
                    {
                        DataRow dr = table.NewRow();
                        dr[LoadAvgMetricDataTableColumns.node_id.ToString()] = metrics.aggregate[i].node_id;
                        dr[LoadAvgMetricDataTableColumns.total_nodes.ToString()] = metrics.aggregate.Count;
                        if (metrics.aggregate[i].node_id == -1) 
                        {
                            dr[LoadAvgMetricDataTableColumns.node_name.ToString()] = "na";
                        }
                        else
                        {
                            dr[LoadAvgMetricDataTableColumns.node_name.ToString()] = string.Format("{0}", metrics.aggregate[i].node_id);
                        }
                        dr[LoadAvgMetricDataTableColumns.one_min_avg.ToString()] = metrics.aggregate[i].one_min_avg / 100.00;
                        dr[LoadAvgMetricDataTableColumns.five_min_avg.ToString()] = metrics.aggregate[i].five_min_avg / 100.00;
                        dr[LoadAvgMetricDataTableColumns.fifteen_min_avg.ToString()] = metrics.aggregate[i].fifteen_min_avg / 100.00;
                        dr[LoadAvgMetricDataTableColumns.gen_time_ts_lct.ToString()] = Utilities.GetFormattedTimeFromUnixTimestampLCT(metrics.header.info_generation_time_ts_lct);
                        dr[LoadAvgMetricDataTableColumns.zero.ToString()] = 0;
                        table.Rows.Add(dr);
                    }
                    catch (Exception ex)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                           String.Format("Received loadavg metric publication: get value exception: {0}", ex.Message));
                    }
                }
            }

            return table;
        }

        public static DataTable TransformNetworkMetricProtobufToSummaryDataTable(object aStats, int aNodeID)
        {
            DataTable table = new DataTable();
            table.TableName = "NetworkMetricSummaryDataTable";
            table.Columns.Add(NetworkMetricDataTableColumns.node_id.ToString(), typeof(int));
            table.Columns.Add(NetworkMetricDataTableColumns.total_nodes.ToString(), typeof(int));
            table.Columns.Add(NetworkMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
            table.Columns.Add(NetworkMetricDataTableColumns.node_and_netid.ToString(), typeof(string));
            table.Columns.Add(NetworkMetricDataTableColumns.rcv_packets.ToString(), typeof(double));
            table.Columns.Add(NetworkMetricDataTableColumns.rcv_drops.ToString(), typeof(double));
            table.Columns.Add(NetworkMetricDataTableColumns.rcv_errs.ToString(), typeof(double));
            table.Columns.Add(NetworkMetricDataTableColumns.txn_packets.ToString(), typeof(double));
            table.Columns.Add(NetworkMetricDataTableColumns.txn_drops.ToString(), typeof(double));
            table.Columns.Add(NetworkMetricDataTableColumns.txn_errs.ToString(), typeof(double));
            table.Columns.Add(NetworkMetricDataTableColumns.zero.ToString(), typeof(int));

            linuxcounters.network_metrics_shortbusy_assembled metrics = aStats as linuxcounters.network_metrics_shortbusy_assembled;
            if (metrics != null)
            {
                int core_idx = 0;
                int total_aggregates = metrics.aggregate.Count;

                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    linuxcounters.network_metrics_shortbusy_assembled.aggregate_network_count network_count = new linuxcounters.network_metrics_shortbusy_assembled.aggregate_network_count();
                    System.Reflection.PropertyInfo[] count_properties = network_count.GetType().GetProperties();
                    string traceMessage = string.Format("Received publcation [{0}]: aggregate_count = {1}, aggregate = {2} {3}",
                                                  LiveFeedRoutingKeyMapper.RKEY_performance_network_metrics_asm,
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

                    Logger.OutputToLog(TraceOptions.TraceOption.LIVEFEEDDATA, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, traceMessage);
                }

                // fill up one node at a time.
                for (int i = 0; i < metrics.network_count_aggr.Count; i++)
                {
                    int num = metrics.network_count_aggr[i].num_network_interfaces;
                    if (num > 0)
                    {
                        if (aNodeID != -1 && aNodeID != i)
                        {
                            core_idx += num;
                            continue;
                        }

                        for (int j = 0; j < num; j++)
                        {
                            if ((core_idx + j) < total_aggregates)
                            {
                                try
                                {
                                    DataRow dr = table.NewRow();
                                    dr[NetworkMetricDataTableColumns.node_id.ToString()] = metrics.aggregate[core_idx + j].node_id;
                                    dr[NetworkMetricDataTableColumns.node_and_netid.ToString()] = string.Format("{0}:{1}", metrics.aggregate[core_idx + j].node_id, metrics.aggregate[core_idx + j].interfacename);
                                    dr[NetworkMetricDataTableColumns.rcv_packets.ToString()] = metrics.aggregate[core_idx + j].rcv_packets;
                                    dr[NetworkMetricDataTableColumns.rcv_drops.ToString()] = metrics.aggregate[core_idx + j].rcv_drops;
                                    dr[NetworkMetricDataTableColumns.rcv_errs.ToString()] = metrics.aggregate[core_idx + j].rcv_errs;
                                    dr[NetworkMetricDataTableColumns.txn_packets.ToString()] = metrics.aggregate[core_idx + j].txn_packets;
                                    dr[NetworkMetricDataTableColumns.txn_drops.ToString()] = metrics.aggregate[core_idx + j].txn_drops;
                                    dr[NetworkMetricDataTableColumns.txn_errs.ToString()] = metrics.aggregate[core_idx + j].txn_errs;
                                    dr[NetworkMetricDataTableColumns.total_nodes.ToString()] = metrics.network_count_aggr.Count;
                                    dr[NetworkMetricDataTableColumns.gen_time_ts_lct.ToString()] = Utilities.GetFormattedTimeFromUnixTimestampLCT(metrics.header.info_generation_time_ts_lct);
                                    dr[NetworkMetricDataTableColumns.zero.ToString()] = 0;
                                    table.Rows.Add(dr);
                                }
                                catch (Exception ex)
                                {
                                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                        String.Format("Received network metric publication: get value exception: {0}", ex.Message));
                                }
                            }
                            else
                            {
                                // [TBD] should emit an exception, but now just return
                                return table;
                            }
                        }
                        core_idx += num;
                    }
                    else
                    {
                        if (aNodeID != -1 && aNodeID != i)
                        {
                            if (num == -1)
                            {
                                core_idx += 1;
                            }
                            continue;
                        }

                        DataRow dr = table.NewRow();
                        dr[NetworkMetricDataTableColumns.node_id.ToString()] = i;
                        dr[NetworkMetricDataTableColumns.node_and_netid.ToString()] = string.Format("{0}:{1}", i, "na");
                        dr[NetworkMetricDataTableColumns.rcv_packets.ToString()] = num;
                        dr[NetworkMetricDataTableColumns.rcv_drops.ToString()] = num;
                        dr[NetworkMetricDataTableColumns.rcv_errs.ToString()] = num;
                        dr[NetworkMetricDataTableColumns.txn_packets.ToString()] = num;
                        dr[NetworkMetricDataTableColumns.txn_drops.ToString()] = num;
                        dr[NetworkMetricDataTableColumns.txn_errs.ToString()] = num;
                        dr[NetworkMetricDataTableColumns.total_nodes.ToString()] = metrics.network_count_aggr.Count;
                        dr[NetworkMetricDataTableColumns.gen_time_ts_lct.ToString()] = Utilities.GetFormattedTimeFromUnixTimestampLCT(metrics.header.info_generation_time_ts_lct);
                        dr[NetworkMetricDataTableColumns.zero.ToString()] = 0;
                        table.Rows.Add(dr);

                        if (num == -1)
                        {
                            core_idx += 1;
                        }
                    }
                }
            }

            return table;
        }

        public static DataTable TransformVirtualMemMetricProtobufToSummaryDataTable(object aStats, int aNodeID)
        {
            DataTable table = new DataTable();
            table.TableName = "VirtualMemMetricSummaryDataTable";
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.node_id.ToString(), typeof(int));
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.total_nodes.ToString(), typeof(int));
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.node_name.ToString(), typeof(string));
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.context_switches.ToString(), typeof(double));
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.minor_page_faults.ToString(), typeof(double));
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.major_page_faults.ToString(), typeof(double));
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.zero.ToString(), typeof(int));

            linuxcounters.virtualmem_metrics_shortbusy_assembled metrics = aStats as linuxcounters.virtualmem_metrics_shortbusy_assembled;
            if (metrics != null)
            {
                int core_idx = 0;

                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    linuxcounters.virtualmem_metrics_shortbusy_assembled.aggregate_buffer buffer = new linuxcounters.virtualmem_metrics_shortbusy_assembled.aggregate_buffer();
                    System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();
                    string traceMessage = string.Format("Received publcation [{0}]: aggregate = {1} {2}",
                                                  LiveFeedRoutingKeyMapper.RKEY_performance_virtualmem_metrics_asm,
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
                    if (aNodeID != -1 && aNodeID != i)
                    {
                        continue;
                    }

                    try
                    {
                        DataRow dr = table.NewRow();
                        dr[VirtualMemoryMetricDataTableColumns.node_id.ToString()] = metrics.aggregate[i].node_id;
                        dr[VirtualMemoryMetricDataTableColumns.total_nodes.ToString()] = metrics.aggregate.Count;
                        if (metrics.aggregate[i].node_id == -1)
                        {
                            dr[VirtualMemoryMetricDataTableColumns.node_name.ToString()] = "na";
                        }
                        else
                        {
                            dr[VirtualMemoryMetricDataTableColumns.node_name.ToString()] = string.Format("{0}", metrics.aggregate[i].node_id);
                        }
                        dr[VirtualMemoryMetricDataTableColumns.context_switches.ToString()] = metrics.aggregate[i].contextswitches;                        
                        dr[VirtualMemoryMetricDataTableColumns.minor_page_faults.ToString()] = metrics.aggregate[i].minor_page_faults;
                        dr[VirtualMemoryMetricDataTableColumns.major_page_faults.ToString()] = metrics.aggregate[i].major_page_faults;
                        dr[VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString()] = Utilities.GetFormattedTimeFromUnixTimestampLCT(metrics.header.info_generation_time_ts_lct);
                        dr[VirtualMemoryMetricDataTableColumns.zero.ToString()] = 0;
                        table.Rows.Add(dr);
                    }
                    catch (Exception ex)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                           String.Format("Received virtual memory metric publication: get value exception: {0}", ex.Message));
                    }
                }
            }

            return table;
        }

        public static DataTable TransformVirtualMemMetricProtobufToSummaryDataTable_M7(object aStats, int aNodeID)
        {
            DataTable table = new DataTable();
            table.TableName = "VirtualMemMetricSummaryDataTable";
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.node_id.ToString(), typeof(int));
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.total_nodes.ToString(), typeof(int));
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.node_name.ToString(), typeof(string));
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.context_switches.ToString(), typeof(double));
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.swap_in.ToString(), typeof(double));
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.swap_out.ToString(), typeof(double));
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.minor_page_faults.ToString(), typeof(double));
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.major_page_faults.ToString(), typeof(double));
            table.Columns.Add(VirtualMemoryMetricDataTableColumns.zero.ToString(), typeof(int));

            linuxcounters.virtualmem_metrics_shortbusy_assembled metrics = aStats as linuxcounters.virtualmem_metrics_shortbusy_assembled;
            if (metrics != null)
            {
                int core_idx = 0;

                if (Logger.IsTraceActive(TraceOptions.TraceOption.LIVEFEEDDATA))
                {
                    linuxcounters.virtualmem_metrics_shortbusy_assembled.aggregate_buffer buffer = new linuxcounters.virtualmem_metrics_shortbusy_assembled.aggregate_buffer();
                    System.Reflection.PropertyInfo[] buffer_properties = buffer.GetType().GetProperties();
                    string traceMessage = string.Format("Received publcation [{0}]: aggregate = {1} {2}",
                                                  LiveFeedRoutingKeyMapper.RKEY_performance_virtualmem_metrics_asm,
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
                    if (aNodeID != -1 && aNodeID != i)
                    {
                        continue;
                    }

                    try
                    {
                        DataRow dr = table.NewRow();
                        dr[VirtualMemoryMetricDataTableColumns.node_id.ToString()] = metrics.aggregate[i].node_id;
                        dr[VirtualMemoryMetricDataTableColumns.total_nodes.ToString()] = metrics.aggregate.Count;
                        if (metrics.aggregate[i].node_id == -1)
                        {
                            dr[VirtualMemoryMetricDataTableColumns.node_name.ToString()] = "na";
                        }
                        else
                        {
                            dr[VirtualMemoryMetricDataTableColumns.node_name.ToString()] = string.Format("{0}", metrics.aggregate[i].node_id);
                        }
                        dr[VirtualMemoryMetricDataTableColumns.context_switches.ToString()] = metrics.aggregate[i].contextswitches;
                        dr[VirtualMemoryMetricDataTableColumns.swap_in.ToString()] = metrics.aggregate[i].swap_in;
                        dr[VirtualMemoryMetricDataTableColumns.swap_out.ToString()] = metrics.aggregate[i].swap_out;
                        dr[VirtualMemoryMetricDataTableColumns.minor_page_faults.ToString()] = metrics.aggregate[i].minor_page_faults;
                        dr[VirtualMemoryMetricDataTableColumns.major_page_faults.ToString()] = metrics.aggregate[i].major_page_faults;
                        dr[VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString()] = Utilities.GetFormattedTimeFromUnixTimestampLCT(metrics.header.info_generation_time_ts_lct);
                        dr[VirtualMemoryMetricDataTableColumns.zero.ToString()] = 0;
                        table.Rows.Add(dr);
                    }
                    catch (Exception ex)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                           String.Format("Received virtual memory metric publication: get value exception: {0}", ex.Message));
                    }
                }
            }

            return table;
        }

        #endregion Private methods
    }
}
