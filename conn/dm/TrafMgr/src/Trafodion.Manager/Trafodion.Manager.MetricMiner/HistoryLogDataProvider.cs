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
using System.Collections.Generic;
using System.Collections;
using System.Text;
using System.Data;
using System.Windows.Forms;
using System.Data.Odbc;
using System.ComponentModel;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.DatabaseArea.Queries;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.MetricMiner
{

    /// <summary>
    /// Class to provide data from a DB based on a SQL.
    /// The point list will be populated based on the PointConfig list provided
    /// </summary>
    public class HistoryLogDataProvider : GenericDataProvider
    {
        protected DataTable _theDataTable = null;
        public static string ElapsedTimeKey        = "elapsed_time";
        long executionTime = 0;
        long fetchTime = 0;


        public HistoryLogDataProvider(DataProviderConfig aconfig)
            : base(aconfig)
        {
        }

        
        /// <summary>
        /// Refreshes the data with the existing parameters
        /// </summary>
        public override void RefreshData()
        {
            base.RefreshData();
        }


        /// <summary>
        /// Returns the custom event args for the HistoryLogDataProvider
        /// </summary>
        /// <returns></returns>
        public override DataProviderEventArgs GetDataProviderEventArgs()
        {
            DataProviderEventArgs evtArgs = new DataProviderEventArgs();
            evtArgs.AddEventProperty(ElapsedTimeKey, TimeTakenToFetchData);
            return evtArgs;
        }

        /// <summary>
        /// Gets called when the user stops the data provider 
        /// </summary>
        /// <param name="worker"></param>
        public override void DoFetchCancel(Trafodion.Manager.Framework.WorkerThread worker)
        {
            //do nothing
        }

        /// <summary>
        /// Gets called when there is an error in the data provider 
        /// </summary>
        /// <param name="worker"></param>
        public override void DoFetchError(Trafodion.Manager.Framework.WorkerThread worker)
        {
            //do nothing
        }

        /// <summary>
        /// Check if the query needes any parameter and if so get it from the user
        /// </summary>
        /// <param name="predefinedParametersHash"></param>
        public override void DoPrefetchSetup(Hashtable predefinedParametersHash)
        {

        }

        public override void DoFetchProgress(Trafodion.Manager.Framework.WorkerThread worker, ProgressChangedEventArgs e)
        {
            //do nothing
        }


        /// <summary>
        /// This is where the actual fetch of the data will happen. 
        /// </summary>
        public override void DoFetchData(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e)
        {
            //Get the data from the server
            _theDataTable = new DataTable();
            Connection con = null;
            OdbcDataReader reader = null;
            string[] columns = { "Date", "Query Status", "Query", "Execution Status", "Connection Attributes" };
            Type[] columTypes = { typeof(DateTime), typeof(String), typeof(HistorylogElement), typeof(string), typeof(string) };
            try
            {
                long t1 = DateTime.Now.Ticks;
                
                List<HistorylogElement> logElements =  HistoryLogger.Instance.GetLogContents();
                logElements.Sort();
                executionTime = DateTime.Now.Ticks - t1;

                t1 = DateTime.Now.Ticks;

                _theDataTable = new DataTable();

                // Add columns to the result data table
                for (int colNum = 0; colNum < columns.Length; colNum++)
                {
                    _theDataTable.Columns.Add(columns[colNum], columTypes[colNum]);
                }


                foreach (HistorylogElement hle in logElements)
                {
                    //worker.ReportProgress();
                    object[] theCurrRow = new object[columns.Length];
                    theCurrRow[0] = hle.ExecutionTime;
                    theCurrRow[1] = hle.TheQueryStatus;
                    theCurrRow[2] = hle;
                    theCurrRow[3] = hle.ExecutionStats;
                    theCurrRow[4] = hle.ConnectionAttributes;


                    //Add rows to the result data table
                    _theDataTable.Rows.Add(theCurrRow);
                }
                fetchTime = DateTime.Now.Ticks - t1;
            }
            finally
            {
               
            }
        }

        /// <summary>
        /// Returns the data as a datatable after it has been fetched 
        /// </summary>
        /// <returns></returns>
        public override DataTable GetDataTable()
        {            
            return _theDataTable;
        }


    }
}
