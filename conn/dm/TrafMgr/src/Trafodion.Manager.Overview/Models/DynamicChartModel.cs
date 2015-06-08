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
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.OverviewArea.Models
{
    public class DynamicChartModel
    {
        #region Fields
        private static Dictionary<ConnectionDefinition, DynamicChartModel> _activeSystemModels = 
            new Dictionary<ConnectionDefinition, DynamicChartModel>(new MyConnectionDefinitionComparer());
        private ConnectionDefinition _theConnectionDefinition = null;
        private Connection _theCurrentConnection = null;
        #endregion Fields

        public ConnectionDefinition ConnectionDefinition
        {
            get
            {
                return _theConnectionDefinition;
            }
            set
            {
                _theConnectionDefinition = value;
            }
        }

        public Connection CurrentConnection
        {
            get
            {
                return _theCurrentConnection;
            }
            set
            {
                _theCurrentConnection = value;
            }
        }



         private DynamicChartModel(ConnectionDefinition aConnectionDefinition)
        {
            ConnectionDefinition = aConnectionDefinition;
            //Add this instance to the static dictionary, so in future if need a reference to this system, 
            //we can look up using the connection definition
            if (!_activeSystemModels.ContainsKey(aConnectionDefinition))
                _activeSystemModels.Add(aConnectionDefinition, this);

            //Subscribe to connection definition's events, so that you can maintain the static dictionary
            ConnectionDefinition.Changed += ConnectionDefinition_Changed;

        }

         ~DynamicChartModel()
        {
            ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
        }

         public static DynamicChartModel FindSystemModel(ConnectionDefinition connectionDefinition)
        {
            DynamicChartModel systemModel = null;
            _activeSystemModels.TryGetValue(connectionDefinition, out systemModel);
            if (systemModel == null)
            {
                systemModel = new DynamicChartModel(connectionDefinition);
            }
            return systemModel;
        }


        /// <summary>
        /// If the connection definition has changed/removed, the static dictionary is updated accordingly
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aReason"></param>
        void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if (aReason != ConnectionDefinition.Reason.Tested)
            {
                _activeSystemModels.Remove(aConnectionDefinition);
            }
        }

        private DataTable _theCoreMetricDynamicTable = new DataTable();
        private DataTable _theMemoryMetricDynamicTable = new DataTable();
        private DataTable _theFileSystemMetricDynamicTable = new DataTable();
        private DataTable _theLoadAvgMetricDynamicTable = new DataTable();
        private DataTable _theDiskMetricDynamicTable = new DataTable();
        private DataTable _theTseMetricDynamicTable = new DataTable();
        private DataTable _theNetworkMetricDynamicTable = new DataTable();
        private DataTable _theVMMetricDynamicTable = new DataTable();

        public DataTable CoreMetricDynamicTable
        {
            get
            {
                ConstructCoreMetricDynamicTable();
                return _theCoreMetricDynamicTable;
            }
        }

        public DataTable MemoryMetricDynamicTable
        {
            get{
                ConstructMemoryMetricDynamicTable();
                return _theMemoryMetricDynamicTable;
            }
        }

        public DataTable FileSystemMetricDynamicTable
        {
            get
            {
                ConstructorFileSystemMetricDynamicTable();
                return _theFileSystemMetricDynamicTable;
            }
        }

        public DataTable LoadAvgMetricDynamicTable
        {
            get
            {
                ConstructLoadAvgMetricDynamicTable();
                return _theLoadAvgMetricDynamicTable;
            }
        }

        public DataTable DiskMetricDynamicTable
        {
            get
            {
                ConstructDiskMetricDynamicTable();
                return _theDiskMetricDynamicTable;
            }
        }

        public DataTable TseMetricDynamicTable
        {
            get
            {
                ConstructTseMetricDynamicTable();
                return _theTseMetricDynamicTable;
            }
        }


        public DataTable NetworkMetricDynamicTable
        {
            get
            {
                ConstructNetworkMetricDynamicTable();
                return _theNetworkMetricDynamicTable;
            }
        }

        public DataTable VMMetricDynamicTable
        {
            get
            {
                ConstructVMMetricDynamicTable();
                return _theVMMetricDynamicTable;
            }
        }

        private void ConstructCoreMetricDynamicTable()
        {
            if (_theCoreMetricDynamicTable.Columns.Count == 0)
            {
                //node_id, gen_time_ts_lct, core_id, avg_core_total,                 
                _theCoreMetricDynamicTable.Columns.Add(SystemMetricModel.CoreMetricDataTableColumns.node_id.ToString(), typeof(int));
                _theCoreMetricDynamicTable.Columns.Add(SystemMetricModel.CoreMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
                _theCoreMetricDynamicTable.Columns.Add(SystemMetricModel.CoreMetricDataTableColumns.max_core.ToString(), typeof(double));
                _theCoreMetricDynamicTable.Columns.Add(SystemMetricModel.CoreMetricDataTableColumns.min_core.ToString(), typeof(double));
                _theCoreMetricDynamicTable.Columns.Add(SystemMetricModel.CoreMetricDataTableColumns.avg_core.ToString(), typeof(double));
            }
        }

        private void ConstructMemoryMetricDynamicTable()
        {
            if (_theMemoryMetricDynamicTable.Columns.Count == 0)
            {
                //node_id, gen_time_ts_lct, memory_used, swap_used                
                _theMemoryMetricDynamicTable.Columns.Add(SystemMetricModel.MemoryMetricDataTableColumns.node_id.ToString(), typeof(int));
                _theMemoryMetricDynamicTable.Columns.Add(SystemMetricModel.MemoryMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
                _theMemoryMetricDynamicTable.Columns.Add(SystemMetricModel.MemoryMetricDataTableColumns.max_memory_used.ToString(), typeof(double));
                _theMemoryMetricDynamicTable.Columns.Add(SystemMetricModel.MemoryMetricDataTableColumns.min_memory_used.ToString(), typeof(double));
                _theMemoryMetricDynamicTable.Columns.Add(SystemMetricModel.MemoryMetricDataTableColumns.avg_memory_used.ToString(), typeof(double));
                _theMemoryMetricDynamicTable.Columns.Add(SystemMetricModel.MemoryMetricDataTableColumns.max_swap_used.ToString(), typeof(double));
                _theMemoryMetricDynamicTable.Columns.Add(SystemMetricModel.MemoryMetricDataTableColumns.min_swap_used.ToString(), typeof(double));
                _theMemoryMetricDynamicTable.Columns.Add(SystemMetricModel.MemoryMetricDataTableColumns.avg_swap_used.ToString(), typeof(double));
            }
        }

        private void ConstructDiskMetricDynamicTable()
        {
            if (_theDiskMetricDynamicTable.Columns.Count == 0)
            {
                //node_id, gen_time_ts_lct, node_and_device_name,reads_and_writes
                _theDiskMetricDynamicTable.Columns.Add(SystemMetricModel.DiskMetricDataTableColumns.node_id.ToString(), typeof(int));
                _theDiskMetricDynamicTable.Columns.Add(SystemMetricModel.DiskMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
                _theDiskMetricDynamicTable.Columns.Add(SystemMetricModel.DiskMetricDataTableColumns.max_reads_and_writes.ToString(), typeof(double));
                _theDiskMetricDynamicTable.Columns.Add(SystemMetricModel.DiskMetricDataTableColumns.min_reads_and_writes.ToString(), typeof(double));
                _theDiskMetricDynamicTable.Columns.Add(SystemMetricModel.DiskMetricDataTableColumns.avg_reads_and_writes.ToString(), typeof(double));
            }
        }

        private void ConstructTseMetricDynamicTable()
        {
            if (_theTseMetricDynamicTable.Columns.Count == 0)
            {
                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.node_id.ToString(), typeof(int));
                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));

                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.max_service_time.ToString(), typeof(double));
                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.min_service_time.ToString(), typeof(double));
                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.service_time.ToString(), typeof(double));

                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.max_requests.ToString(), typeof(double));
                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.min_requests.ToString(), typeof(double));
                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.requests.ToString(), typeof(double));

                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.max_ase_service_time.ToString(), typeof(double));
                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.min_ase_service_time.ToString(), typeof(double));
                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.ase_service_time.ToString(), typeof(double));

                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.max_request_io_wait_time.ToString(), typeof(double));
                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.min_request_io_wait_time.ToString(), typeof(double));
                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.request_io_wait_time.ToString(), typeof(double));

                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.max_ready_list_count.ToString(), typeof(double));
                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.min_ready_list_count.ToString(), typeof(double));
                _theTseMetricDynamicTable.Columns.Add(SystemMetricModel.TseMetricDataTableColumns.ready_list_count.ToString(), typeof(double));
            }
        }

        private void ConstructorFileSystemMetricDynamicTable()
        {
            if (_theFileSystemMetricDynamicTable.Columns.Count == 0)
            {
                //node_id, gen_time_ts_lct, node_and_fsname, percent_consumed
                _theFileSystemMetricDynamicTable.Columns.Add(SystemMetricModel.FileSysMetricDataTableColumns.node_id.ToString(), typeof(int));
                _theFileSystemMetricDynamicTable.Columns.Add(SystemMetricModel.FileSysMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
                _theFileSystemMetricDynamicTable.Columns.Add(SystemMetricModel.FileSysMetricDataTableColumns.max_percent_consumed.ToString(), typeof(double));
                _theFileSystemMetricDynamicTable.Columns.Add(SystemMetricModel.FileSysMetricDataTableColumns.min_percent_consumed.ToString(), typeof(double));
                _theFileSystemMetricDynamicTable.Columns.Add(SystemMetricModel.FileSysMetricDataTableColumns.avg_percent_consumed.ToString(), typeof(double));
            }
        }

        private void ConstructLoadAvgMetricDynamicTable()
        {
            if (_theLoadAvgMetricDynamicTable.Columns.Count == 0)
            {
                //node_id, gen_time_ts_lct, one_min_avg
                _theLoadAvgMetricDynamicTable.Columns.Add(SystemMetricModel.LoadAvgMetricDataTableColumns.node_id.ToString(), typeof(int));
                _theLoadAvgMetricDynamicTable.Columns.Add(SystemMetricModel.LoadAvgMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
                _theLoadAvgMetricDynamicTable.Columns.Add(SystemMetricModel.LoadAvgMetricDataTableColumns.max_one_min_avg.ToString(), typeof(double));
                _theLoadAvgMetricDynamicTable.Columns.Add(SystemMetricModel.LoadAvgMetricDataTableColumns.min_one_min_avg.ToString(), typeof(double));
                _theLoadAvgMetricDynamicTable.Columns.Add(SystemMetricModel.LoadAvgMetricDataTableColumns.avg_one_min_avg.ToString(), typeof(double));
            }
        }

        private void ConstructNetworkMetricDynamicTable()
        {
            if (_theNetworkMetricDynamicTable.Columns.Count == 0)
            {
                //node_id, gen_time_ts_lct, node_and_netid, rcv_packets, rcv_errs, txn_packets, txn_drops, txn_errs
                _theNetworkMetricDynamicTable.Columns.Add(SystemMetricModel.NetworkMetricDataTableColumns.node_id.ToString(), typeof(int));
                _theNetworkMetricDynamicTable.Columns.Add(SystemMetricModel.NetworkMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
                _theNetworkMetricDynamicTable.Columns.Add(SystemMetricModel.NetworkMetricDataTableColumns.max_rcv_packets.ToString(), typeof(double));
                _theNetworkMetricDynamicTable.Columns.Add(SystemMetricModel.NetworkMetricDataTableColumns.min_rcv_packets.ToString(), typeof(double));
                _theNetworkMetricDynamicTable.Columns.Add(SystemMetricModel.NetworkMetricDataTableColumns.avg_rcv_packets.ToString(), typeof(double));
                _theNetworkMetricDynamicTable.Columns.Add(SystemMetricModel.NetworkMetricDataTableColumns.max_txn_packets.ToString(), typeof(double));
                _theNetworkMetricDynamicTable.Columns.Add(SystemMetricModel.NetworkMetricDataTableColumns.min_txn_packets.ToString(), typeof(double));
                _theNetworkMetricDynamicTable.Columns.Add(SystemMetricModel.NetworkMetricDataTableColumns.avg_txn_packets.ToString(), typeof(double));
            }
        }

        private void ConstructVMMetricDynamicTable()
        {
            if (_theVMMetricDynamicTable.Columns.Count == 0)
            {
                //node_id, gen_time_ts_lct, context_switches, swap_in, swap_out, minor_page_faults, major_page_faults
                _theVMMetricDynamicTable.Columns.Add(SystemMetricModel.VirtualMemoryMetricDataTableColumns.node_id.ToString(), typeof(int));
                _theVMMetricDynamicTable.Columns.Add(SystemMetricModel.VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString(), typeof(DateTime));
                _theVMMetricDynamicTable.Columns.Add(SystemMetricModel.VirtualMemoryMetricDataTableColumns.max_context_switches.ToString(), typeof(double));
                _theVMMetricDynamicTable.Columns.Add(SystemMetricModel.VirtualMemoryMetricDataTableColumns.min_context_switches.ToString(), typeof(double));
                _theVMMetricDynamicTable.Columns.Add(SystemMetricModel.VirtualMemoryMetricDataTableColumns.avg_context_switches.ToString(), typeof(double));
            }
        }



    }

}
