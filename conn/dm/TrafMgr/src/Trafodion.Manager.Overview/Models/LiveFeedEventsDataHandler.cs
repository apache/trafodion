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
using System.Collections;
using System.Data;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.OverviewArea.Controls;
using Trafodion.Manager.UniversalWidget;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.OverviewArea.Models
{
    /// <summary>
    /// Class to handle the proper formatting of the diaply of the event data
    /// </summary>
    public class LiveFeedEventsDataHandler : TabularDataDisplayHandler
    {
        #region Fields

        private int limit = 1000;

        private Trafodion.Manager.OverviewArea.Controls.LiveFeedEventUserControl _theLiveFeedEventUserControl;
        private object _locker;
        //FilterManager _FilterManager = null;
        private ArrayList _Columns = new ArrayList();
        private bool _loadFromStore = false;
        private ConnectionDefinition _theConnectionDefinition;
        private EventDetails eventDetails;
        private string _theServerTimeZone = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: Columns - all defined columns
        /// </summary>
        public ArrayList Columns
        {
            get { return _Columns; }
            set { _Columns = value; }
        }

        /// <summary>
        /// Property: ServerTimeZone - the server time zone
        /// </summary>
        public string ServerTimeZone
        {
            get { return _theServerTimeZone; }
            set { _theServerTimeZone = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aLiveFeedEventUserControl"></param>
        public LiveFeedEventsDataHandler(LiveFeedEventUserControl aLiveFeedEventUserControl)
        {
            _theLiveFeedEventUserControl = aLiveFeedEventUserControl;
            //_FilterManager = _theTextEventUserControl.FilterManager;
            //string eventTimeColName = string.IsNullOrEmpty(_theLiveFeedEventUserControl.ConnectionDefn.ServerTimeZoneName) ? EventFilterModel.EVENT_TIME :
            //    string.Format("{0} ({1})", EventFilterModel.EVENT_TIME, _theLiveFeedEventUserControl.ConnectionDefn.ServerTimeZoneName);

            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_GENERATION_TIME_TS_COL_ID, "info_header_info_generation_time_ts_lct", EventFilterModel.EVENT_TIME, typeof(string), GetFormattedTimestamp));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_EVENT_ID_COL_ID, "event_header_event_id", EventFilterModel.EVENT_ID, typeof(int)));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_PROCESS_COL_ID, "info_header_info_process_id", EventFilterModel.PROCESS_ID, typeof(int)));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_SUBSYSTEM_COL_ID, "info_header_info_component_id", EventFilterModel.SUBSYSTEM, typeof(string), GetFormattedSubSystem));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_SEVERITY_COL_ID, "event_header_event_severity", EventFilterModel.SEVERITY, typeof(string), GetFormattedSeverity));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_ROLE_COL_ID, "qpid_header_process_name", EventFilterModel.ROLE_NAME, typeof(string), GetFormattedRoleName));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_MESSAGE_COL_ID, "text", EventFilterModel.MESSAGE, typeof(string)));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_NODE_COL_ID, "info_header_info_node_id", EventFilterModel.NODE_ID, typeof(long)));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_PROCESS_NAME_COL_ID, "info_header_info_process_name", EventFilterModel.PROCESS_NAME, typeof(string)));
        }

        #endregion Constructors

        #region Public methods
        
        /// <summary>
        /// To populate the UI
        /// </summary>
        /// <param name="aConfig"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aDataGrid"></param>
        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            lock (this)
            {
                populate(aConfig, aDataTable, aDataGrid);
            }
        }

        /// <summary>
        /// Helper method to get the column details given a column name
        /// </summary>
        /// <param name="aName"></param>
        /// <returns></returns>
        public ColumnDetails getColumnDetailsForName(string aName)
        {
            ColumnDetails details = null;
            int idx = _Columns.IndexOf(aName);
            if (idx >= 0)
            {
                details = _Columns[idx] as ColumnDetails;
            }
            return details;
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Populate the UI
        /// </summary>
        /// <param name="aConfig"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aDataGrid"></param>
        private void populate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            if (null == aDataTable)
            {
                return;
            }
            _theConnectionDefinition = aConfig.DataProviderConfig.ConnectionDefinition;
            LiveFeedEventsDataProvider dataProvider = (LiveFeedEventsDataProvider)aConfig.DataProviderConfig.GetDataProvider();
            int newEvents = dataProvider.NewlyArrivedEventCount;

            DataTable dataTable = new DataTable();
            foreach (ColumnDetails column in _Columns)
            {
                if (aDataTable.Columns.Contains(column.ColumnName))
                {
                    dataTable.Columns.Add(column.DisplayName, column.ColumnType);
                }
                else
                {
                    throw new Exception(string.Format("Event column [{0}] could not be found in the message.", column.ColumnName));
                }
            }

            if (aDataGrid.Cols.Count == 0)
            {
                // This is the first time we got events, so use the entire data table to initialize the data grid
                foreach (DataRow dr in aDataTable.Rows)
                {
                    DataRow row = dataTable.NewRow();
                    foreach (DataColumn dc in aDataTable.Columns)
                    {
                        ColumnDetails details = getColumnDetailsForName(dc.ColumnName);
                        if (details != null)
                        {
                            row[details.DisplayName] = details.GetFormattedValueImpl(dr[dc.ColumnName]);
                        }
                    }
                    dataTable.Rows.Add(row);
                }
                base.DoPopulate(aConfig, dataTable, aDataGrid);
            }
            else
            {
                // The data grid has been initialized, so just insert the new events at the top
                for (int i = 0; i < newEvents; i++)
                {
                    DataRow dr = aDataTable.Rows[i];
                    DataRow row = dataTable.NewRow();
                    foreach (ColumnDetails column in _Columns)
                    {
                        ColumnDetails details = getColumnDetailsForName(column.ColumnName);
                        if (details != null)
                        {
                            row[details.DisplayName] = details.GetFormattedValueImpl(dr[column.ColumnName]);
                        }
                    }
                    dataTable.Rows.Add(row);
                }

                // Now, insert all of the newly arrival to the data grid
                addRowToStore(aDataGrid, dataTable, dataProvider.CacheSize);
            }

            //Console.WriteLine("Datatable size = " + dataTable.Rows.Count);
            //base.DoPopulate(aConfig, dataTable, aDataGrid);

            string gridHeaderText;
            gridHeaderText = string.Format("{0} Events - Last Refreshed : {1}",
                                            aDataGrid.Rows.Count,
                                            DateTime.Now.ToString());
            aDataGrid.UpdateCountControlText(gridHeaderText);
            setFormattedFilterMessage();
            dataProvider.NewlyArrivedEventCount = 0;
        }

        /// <summary>
        /// Set formatted filter message
        /// </summary>
        private void setFormattedFilterMessage()
        {
            _theLiveFeedEventUserControl.ShowFilterMessage();
        }

        /// <summary>
        /// formats the severity
        /// </summary>
        /// <param name="severity"></param>
        /// <returns></returns>
        private string GetFormattedSeverity(object severity)
        {
            string ret = "";
            if (severity is int)
            {
                return _theLiveFeedEventUserControl.TheEventDetails.Severity((int)severity).ToString();
            }
            return ret;
        }

        /// <summary>
        /// returns a formatted sub-system
        /// </summary>
        /// <param name="subSystem"></param>
        /// <returns></returns>
        private string GetFormattedSubSystem(object subSystem)
        {
            string ret = "";
            if (subSystem is uint)
            {
                return _theLiveFeedEventUserControl.TheEventDetails.SubSystem(((int)(uint)subSystem)).ToString();
            }
            else if (subSystem is long)
            {
                return eventDetails.SubSystem(((int)(long)subSystem)).ToString();
            }
            return ret;
        }

        /// <summary>
        /// helper method for formating timestamps
        /// </summary>
        /// <param name="timestamp"></param>
        /// <returns></returns>
        private string GetFormattedTimestamp(object timestamp)
        {
            DateTime date_time = Utilities.GetFormattedTimeFromUnixTimestampLCT((long)timestamp);
            return Utilities.GetTrafodionSQLLongDateTime(date_time, false);
        }
        
        /// <summary>
        /// To format role names
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        private string GetFormattedRoleName(object obj)
        {
            return "";
        }

        /// <summary>
        /// Add row to the data store
        /// </summary>
        /// <param name="aRow"></param>
        /// <param name="aStore"></param>
        private void addRowToStore(Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid, DataTable aDataTable, int aCacheSize)
        {
            aDataGrid.BeginUpdate();
            int currentRowCount = aDataGrid.Rows.Count;
            int rowsToBeAdded = aDataTable.Rows.Count;
            if ((currentRowCount + rowsToBeAdded) > aCacheSize)
            {
                int deleteCount = (currentRowCount + rowsToBeAdded) - aCacheSize;
                //for (int i = 1; i <= deleteCount; i++)
                //{
                //    aDataGrid.Rows.RemoveAt(currentRowCount - i);
                //}
                aDataGrid.Rows.RemoveRange(aCacheSize - 1, deleteCount);
            }

            // Now, insert all of the newly arrival to the data grid
            aDataGrid.InsertRows(0, aDataTable);
            aDataGrid.EndUpdate();
        }
        #endregion Private methods
    }
}
