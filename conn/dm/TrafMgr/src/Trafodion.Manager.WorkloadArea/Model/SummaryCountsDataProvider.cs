//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.WorkloadArea.Model
{
    /// <summary>
    /// This custom data provider is to used for the Live view display, where a sequence of operations are done
    /// This is achieved by overriding the DoFetchData method
    /// </summary>
    public class SummaryCountsDataProvider : DatabaseDataProvider
    {
        #region member variables

        ConnectionDefinition _connectionDefinition;
        private DataTable _servicesDataTable;
        private WmsStatsStruct _wmsStats;

        public readonly object SyncRootForServiceDataTable = new object();

        #endregion member variables

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _connectionDefinition; }
            set { _connectionDefinition = value; }
        }

        public DataTable ServicesDataTable
        {
            get 
            {
                lock (this.SyncRootForServiceDataTable)
                {
                    return _servicesDataTable;
                }
            }
            private set
            {
                lock (this.SyncRootForServiceDataTable)
                {
                    _servicesDataTable = value;
                }
            }
        }

        public WmsStatsStruct WmsStats
        {
            get { return _wmsStats; }
        }

        public SummaryCountsDataProvider(ConnectionDefinition aConnectionDefinition, DatabaseDataProviderConfig dbConfig)
            :base(dbConfig)
        {
            _connectionDefinition = aConnectionDefinition;
        }

        /// <summary>
        /// The DoFetchData method is overridden to run a sequence of operations
        /// First the base class DoFetchData method is called to get the list of running queries. 
        /// Then the Wms system model is refreshed and a status wms command is run.
        /// Finally the list of wms services is refreshed
        /// All this data is required for the live view. 
        /// </summary>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        public override void DoFetchData(Trafodion.Manager.Framework.WorkerThread worker, System.ComponentModel.DoWorkEventArgs e)
        {
            FetchWmsStatsAndServices();
        }

        /// <summary>
        /// Fetch the WMS service status and the WMS platform metrics
        /// </summary>
        private void FetchWmsStatsAndServices()
        {
            if (_connectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STATUS.ToString()))
            {
                double maxCpuBusy = 100.0;
                double maxMemUsage = 85.0;
                double cpuBusy = 0.0;
                double memUsage = 0.0;
                double maxSDDUsage = 0.0;
                double ssdUsage = 0.0;
                double maxExecQueries=0.0;
                double curExec = 0.0;
                double maxAvgESPs = 0.0;
                double avgESPNumber = 0.0;

                DataTable statusWMS_dt = WmsCommand.executeCommand("STATUS WMS", _connectionDefinition);
                if (0 >= statusWMS_dt.Rows.Count)
                    return;

                maxCpuBusy = Math.Round(Double.Parse(statusWMS_dt.Rows[0]["MAX_CPU_BUSY"].ToString()), 2);
                maxMemUsage = Math.Round(Double.Parse(statusWMS_dt.Rows[0]["MAX_MEM_USAGE"].ToString()), 2);

                cpuBusy = Math.Round(Double.Parse(statusWMS_dt.Rows[0]["CPU_BUSY"].ToString()), 2);
                memUsage = Math.Round(Double.Parse(statusWMS_dt.Rows[0]["MEM_USAGE"].ToString()), 2);

                maxSDDUsage = Math.Round(Double.Parse(statusWMS_dt.Rows[0]["MAX_SSD_USAGE"].ToString()),2 );
                ssdUsage = Math.Round(Double.Parse(statusWMS_dt.Rows[0]["SSD_USAGE"].ToString()), 2);
                

                if (_connectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ130) 
                {
                    maxExecQueries = Double.Parse(statusWMS_dt.Rows[0]["MAX_EXEC_QUERIES"].ToString());
                    curExec = Double.Parse(statusWMS_dt.Rows[0]["CUR_EXEC"].ToString());
                    maxAvgESPs = Double.Parse(statusWMS_dt.Rows[0]["MAX_AVG_ESPS"].ToString());
                    avgESPNumber = Double.Parse(statusWMS_dt.Rows[0]["AVG_ESP_NUMBER"].ToString());
                }                

                _wmsStats = new WmsStatsStruct(maxCpuBusy, maxMemUsage, cpuBusy, memUsage, maxSDDUsage, ssdUsage, maxExecQueries, 
                                                curExec, maxAvgESPs, avgESPNumber);
                if (_connectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ130)
                    ServicesDataTable = WmsCommand.executeCommand("STATUS SERVICE ALL STATS", _connectionDefinition);                
                else
                    ServicesDataTable = WmsCommand.executeCommand("STATUS SERVICE ALL", _connectionDefinition);
                
            }

        }

    }

    /// <summary>
    /// Struct to hold the WMS platform metrics
    /// </summary>
    public struct WmsStatsStruct
    {
        public double MaxCpuBusy;
        public double MaxMemUsage;
        public double CpuBusy;
        public double MemUsage;
        public double MaxSSDUsage;
        public double SSDUsage;
        public double MaxExecQueries;
        public double CurExec;
        public double MaxAvgESPs;
        public double AvgESPNumber;
       
        
         public WmsStatsStruct(double maxCpuBusy, double maxMemUsage, double cpuBusy,
            double memUsage, double maxOverflowUsage, double overflowUsage, double maxExecQueries, double curExec, double maxAvgESPs, double avgESPNumber)
        {
             MaxCpuBusy = maxCpuBusy;
             MaxMemUsage = maxMemUsage;
             CpuBusy = cpuBusy;
             MemUsage = memUsage;
             MaxSSDUsage = maxOverflowUsage;
             SSDUsage = overflowUsage;
             MaxExecQueries = maxExecQueries;
             CurExec = curExec;
             MaxAvgESPs = maxAvgESPs;
             AvgESPNumber = avgESPNumber;
        }
    }
}
