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
using System.Collections.Generic;

namespace Trafodion.Manager.OverviewArea.Models
{
    public partial class SystemMetricModel
    {
        #region Fields
        /// <summary>
        /// Definition of system metric types
        /// </summary>
        public enum DynamicMetrics
        {
            Core = 0,
            Memory,
            Swap,
            File_System,
            Load_Avg,
            Disk,
            Network_Rcv,
            Network_Txn,
            Virtual_Memory
        };

        public static Dictionary<SystemMetrics, string> ChartNames = new Dictionary<SystemMetrics, string>()
        {
            {SystemMetrics.Core, SystemMetrics.Core.ToString()},
            {SystemMetrics.Memory, "MemorySwap"},
            {SystemMetrics.Swap, "MemorySwap"},
            {SystemMetrics.File_System, SystemMetrics.File_System.ToString()},
            {SystemMetrics.Load_Avg, SystemMetrics.Load_Avg.ToString()},
            {SystemMetrics.Disk, SystemMetrics.Disk.ToString()},
            {SystemMetrics.Tse, SystemMetrics.Tse.ToString()},
            {SystemMetrics.Network_Rcv, "Network"},
            {SystemMetrics.Network_Txn, "Network"},
            {SystemMetrics.Virtual_Memory, SystemMetrics.Virtual_Memory.ToString()}
        };

        //15 minutes
        public static int lengthMinutes = 15;
        public static int totalNodes = 0;
        //use this max count to determine whether history datatabe need to do clearing 15min * 60seconds
        public static int MaximumDataCount = 900;

        public static List<List<SystemMetrics>> shareChartMetrics
        {
            get
            {
                return new List<List<SystemMetrics>>() 
                { 
                    new List<SystemMetrics>(){SystemMetrics.Memory, SystemMetrics.Swap}, 
                    new List<SystemMetrics>(){SystemMetrics.Network_Rcv, SystemMetrics.Network_Txn}                
                };
            }
        }

        #endregion Fields


        ////DynamicChart, Temporary Put it here... Maybe we will create a DynamicModel later
        //public static DataTable CoreMetricDynamicTable = new DataTable();
        //public static DataTable MemoryMetricDynamicTable = new DataTable();
        //public static DataTable FileSystemMetricDynamicTable = new DataTable();
        //public static DataTable LoadAvgMetricDynamicTable = new DataTable();
        //public static DataTable DiskMetricDynamicTable = new DataTable();
        //public static DataTable NetworkMetricDynamicTable = new DataTable();
        //public static DataTable VMMetricDynamicTable = new DataTable();

        

        //private static void ConstructDynamicTables()
        //{
        //    ConstructCoreMetricDynamicTable();
        //    ConstructMemoryMetricDynamicTable();
        //    ConstructDiskMetricDynamicTable();
        //    ConstructorFileSystemMetricDynamicTable();
        //    ConstructLoadAvgMetricDynamicTable();
        //    ConstructNetworkMetricDynamicTable();
        //    ConstructVMMetricDynamicTable();
        //}

        //private static void ConstructCoreMetricDynamicTable()
        //{
        //    if (CoreMetricDynamicTable.Columns.Count == 0)
        //    {
        //        //node_id, gen_time_ts_lct, core_id, avg_core_total,                 
        //        CoreMetricDynamicTable.Columns.Add(CoreMetricDataTableColumns.node_id.ToString(), typeof(int));
        //        CoreMetricDynamicTable.Columns.Add(CoreMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
        //        CoreMetricDynamicTable.Columns.Add(CoreMetricDataTableColumns.max_core.ToString(), typeof(double));
        //        CoreMetricDynamicTable.Columns.Add(CoreMetricDataTableColumns.min_core.ToString(), typeof(double));
        //        CoreMetricDynamicTable.Columns.Add(CoreMetricDataTableColumns.avg_core.ToString(), typeof(double));
        //    }
        //}

        //private static void ConstructMemoryMetricDynamicTable()
        //{
        //    if (MemoryMetricDynamicTable.Columns.Count == 0)
        //    {
        //        //node_id, gen_time_ts_lct, memory_used, swap_used                
        //        MemoryMetricDynamicTable.Columns.Add(MemoryMetricDataTableColumns.node_id.ToString(), typeof(int));
        //        MemoryMetricDynamicTable.Columns.Add(MemoryMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
        //        MemoryMetricDynamicTable.Columns.Add(MemoryMetricDataTableColumns.max_memory_used.ToString(), typeof(double));
        //        MemoryMetricDynamicTable.Columns.Add(MemoryMetricDataTableColumns.min_memory_used.ToString(), typeof(double));
        //        MemoryMetricDynamicTable.Columns.Add(MemoryMetricDataTableColumns.avg_memory_used.ToString(), typeof(double));
        //        MemoryMetricDynamicTable.Columns.Add(MemoryMetricDataTableColumns.max_swap_used.ToString(), typeof(double));
        //        MemoryMetricDynamicTable.Columns.Add(MemoryMetricDataTableColumns.min_swap_used.ToString(), typeof(double));
        //        MemoryMetricDynamicTable.Columns.Add(MemoryMetricDataTableColumns.avg_swap_used.ToString(), typeof(double));
        //    }
        //}

        //private static void ConstructDiskMetricDynamicTable()
        //{
        //    if (DiskMetricDynamicTable.Columns.Count == 0)
        //    {
        //        //node_id, gen_time_ts_lct, node_and_device_name,reads_and_writes
        //        DiskMetricDynamicTable.Columns.Add(DiskMetricDataTableColumns.node_id.ToString(), typeof(int));
        //        //DiskMetricDynamicTable.Columns.Add(DiskMetricDataTableColumns.total_nodes.ToString(), typeof(int));
        //        DiskMetricDynamicTable.Columns.Add(DiskMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
        //        //DiskMetricDynamicTable.Columns.Add(DiskMetricDataTableColumns.node_and_device_name.ToString(), typeof(string));
        //        DiskMetricDynamicTable.Columns.Add(DiskMetricDataTableColumns.max_reads_and_writes.ToString(), typeof(double));
        //        DiskMetricDynamicTable.Columns.Add(DiskMetricDataTableColumns.min_reads_and_writes.ToString(), typeof(double));
        //        DiskMetricDynamicTable.Columns.Add(DiskMetricDataTableColumns.avg_reads_and_writes.ToString(), typeof(double));
        //    }
        //}

        //private static void ConstructorFileSystemMetricDynamicTable()
        //{
        //    if (FileSystemMetricDynamicTable.Columns.Count == 0)
        //    {
        //        //node_id, gen_time_ts_lct, node_and_fsname, percent_consumed
        //        FileSystemMetricDynamicTable.Columns.Add(FileSysMetricDataTableColumns.node_id.ToString(), typeof(int));
        //        FileSystemMetricDynamicTable.Columns.Add(FileSysMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
        //        //FileSystemMetricDynamicTable.Columns.Add(FileSysMetricDataTableColumns.node_and_fsname.ToString(), typeof(string));
        //        FileSystemMetricDynamicTable.Columns.Add(FileSysMetricDataTableColumns.max_percent_consumed.ToString(), typeof(double));
        //        FileSystemMetricDynamicTable.Columns.Add(FileSysMetricDataTableColumns.min_percent_consumed.ToString(), typeof(double));
        //        FileSystemMetricDynamicTable.Columns.Add(FileSysMetricDataTableColumns.avg_percent_consumed.ToString(), typeof(double));
        //    }
        //}

        //private static void ConstructLoadAvgMetricDynamicTable()
        //{
        //    if (LoadAvgMetricDynamicTable.Columns.Count == 0)
        //    {
        //        //node_id, gen_time_ts_lct, one_min_avg
        //        LoadAvgMetricDynamicTable.Columns.Add(LoadAvgMetricDataTableColumns.node_id.ToString(), typeof(int));
        //        LoadAvgMetricDynamicTable.Columns.Add(LoadAvgMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
        //        //LoadAvgMetricDynamicTable.Columns.Add(LoadAvgMetricDataTableColumns.node_name.ToString(), typeof(string));
        //        LoadAvgMetricDynamicTable.Columns.Add(LoadAvgMetricDataTableColumns.max_one_min_avg.ToString(), typeof(double));
        //        LoadAvgMetricDynamicTable.Columns.Add(LoadAvgMetricDataTableColumns.min_one_min_avg.ToString(), typeof(double));
        //        LoadAvgMetricDynamicTable.Columns.Add(LoadAvgMetricDataTableColumns.avg_one_min_avg.ToString(), typeof(double));
        //    }
        //}

        //private static void ConstructNetworkMetricDynamicTable()
        //{
        //    if (NetworkMetricDynamicTable.Columns.Count == 0)
        //    {
        //        //node_id, gen_time_ts_lct, node_and_netid, rcv_packets, rcv_errs, txn_packets, txn_drops, txn_errs
        //        NetworkMetricDynamicTable.Columns.Add(NetworkMetricDataTableColumns.node_id.ToString(), typeof(int));
        //        NetworkMetricDynamicTable.Columns.Add(NetworkMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
        //        //NetworkMetricDynamicTable.Columns.Add(NetworkMetricDataTableColumns.node_and_netid.ToString(), typeof(string));
        //        NetworkMetricDynamicTable.Columns.Add(NetworkMetricDataTableColumns.max_rcv_packets.ToString(), typeof(double));
        //        NetworkMetricDynamicTable.Columns.Add(NetworkMetricDataTableColumns.min_rcv_packets.ToString(), typeof(double));
        //        NetworkMetricDynamicTable.Columns.Add(NetworkMetricDataTableColumns.avg_rcv_packets.ToString(), typeof(double));
        //        //NetworkMetricDynamicTable.Columns.Add(NetworkMetricDataTableColumns.rcv_drops.ToString(), typeof(double));
        //        //NetworkMetricDynamicTable.Columns.Add(NetworkMetricDataTableColumns.rcv_errs.ToString(), typeof(double));
        //        NetworkMetricDynamicTable.Columns.Add(NetworkMetricDataTableColumns.max_txn_packets.ToString(), typeof(double));
        //        NetworkMetricDynamicTable.Columns.Add(NetworkMetricDataTableColumns.min_txn_packets.ToString(), typeof(double));
        //        NetworkMetricDynamicTable.Columns.Add(NetworkMetricDataTableColumns.avg_txn_packets.ToString(), typeof(double));
        //        //NetworkMetricDynamicTable.Columns.Add(NetworkMetricDataTableColumns.txn_drops.ToString(), typeof(double));
        //        //NetworkMetricDynamicTable.Columns.Add(NetworkMetricDataTableColumns.txn_errs.ToString(), typeof(double));
        //    }
        //}

        //private static void ConstructVMMetricDynamicTable()
        //{
        //    if (VMMetricDynamicTable.Columns.Count == 0)
        //    {
        //        //node_id, gen_time_ts_lct, context_switches, swap_in, swap_out, minor_page_faults, major_page_faults
        //        VMMetricDynamicTable.Columns.Add(VirtualMemoryMetricDataTableColumns.node_id.ToString(), typeof(int));
        //        VMMetricDynamicTable.Columns.Add(VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
        //        VMMetricDynamicTable.Columns.Add(VirtualMemoryMetricDataTableColumns.max_context_switches.ToString(), typeof(double));
        //        VMMetricDynamicTable.Columns.Add(VirtualMemoryMetricDataTableColumns.min_context_switches.ToString(), typeof(double));
        //        VMMetricDynamicTable.Columns.Add(VirtualMemoryMetricDataTableColumns.avg_context_switches.ToString(), typeof(double));
        //        //VMMetricDynamicTable.Columns.Add(VirtualMemoryMetricDataTableColumns.swap_in.ToString(), typeof(double));
        //        //VMMetricDynamicTable.Columns.Add(VirtualMemoryMetricDataTableColumns.swap_out.ToString(), typeof(double));
        //        //VMMetricDynamicTable.Columns.Add(VirtualMemoryMetricDataTableColumns.minor_page_faults.ToString(), typeof(double));
        //        //VMMetricDynamicTable.Columns.Add(VirtualMemoryMetricDataTableColumns.major_page_faults.ToString(), typeof(double));
        //    }
        //}



    }
}
