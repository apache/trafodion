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
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.OverviewArea.Models
{
    //Class to handle the proper formatting of the diaply of the event data
    public class TextEventsDataHandler : TabularDataDisplayHandler
    {
        #region Fields

        int limit = 1000;
        private Trafodion.Manager.OverviewArea.Controls.TextEventUserControl _theTextEventUserControl;
        private object _locker;
        //FilterManager _FilterManager = null;
        private ArrayList _Columns = new ArrayList();
        private bool _loadFromStore = false;
        private string _theServerTimeZone = "";

        #endregion Fields


        #region Properties

        public bool LoadFromStore
        {
            get { return _loadFromStore; }
            set { _loadFromStore = value; }
        }

        public ArrayList Columns
        {
            get { return _Columns; }
            set { _Columns = value; }
        }

        /// <summary>
        /// Property: ServerTimeZone - the server's time zone
        /// </summary>
        public string ServerTimeZone
        {
            get { return _theServerTimeZone; }
            set { _theServerTimeZone = value; }
        }

        #endregion Properties


        #region Constructors

        public TextEventsDataHandler(Trafodion.Manager.OverviewArea.Controls.TextEventUserControl aTextEventUserControl)
        {
            _theTextEventUserControl = aTextEventUserControl;
            //_FilterManager = _theTextEventUserControl.FilterManager;
            //string eventTimeColName = string.IsNullOrEmpty(_theTextEventUserControl.ConnectionDefinition.ServerTimeZoneName) ? EventFilterModel.EVENT_TIME :
            //                string.Format("{0} ({1})", EventFilterModel.EVENT_TIME, _theTextEventUserControl.ConnectionDefinition.ServerTimeZoneName);

            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_EVENT_ID_COL_ID, "event_id", EventFilterModel.EVENT_ID, typeof(int)));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_PROCESS_COL_ID, "process_id", EventFilterModel.PROCESS_ID, typeof(int)));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_SUBSYSTEM_COL_ID, "component_id", EventFilterModel.SUBSYSTEM, typeof(string), GetFormattedSubSystem));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_SEVERITY_COL_ID, "SEVERITY", EventFilterModel.SEVERITY, typeof(string), GetFormattedSeverity));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_GENERATION_TIME_TS_COL_ID, "GEN_TS_LCT", EventFilterModel.EVENT_TIME, typeof(string), GetFormattedTimestamp));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_ROLE_COL_ID, "role_name", EventFilterModel.ROLE_NAME, typeof(string)));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_MESSAGE_COL_ID, "text", EventFilterModel.MESSAGE, typeof(string)));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_NODE_COL_ID, "node_id", EventFilterModel.NODE_ID, typeof(long)));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_PROCESS_NAME_COL_ID, "PROCESS_NAME", EventFilterModel.PROCESS_NAME, typeof(string)));
            _Columns.Add(new ColumnDetails(EventFilterModel.EVENT_TOKENIZED_EVENT_TABLE_COL_ID, "TOKENIZED_EVENT_TABLE", EventFilterModel.TOKENIZED_EVENT_TABLE, typeof(string)));
        }

        #endregion Constructors


        #region Public methods
        //Populates the UI
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

        //private void DeleteRowsIfNeeded(System.Data.DataTable aDataTable, Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        //{
        //    int currentRowCount = aDataGrid.Rows.Count;
        //    int rowsToBeAdded = (aDataTable != null) ? aDataTable.Rows.Count : 0;
        //    if ((currentRowCount + rowsToBeAdded) > limit)
        //    {
        //        int deleteCount = (currentRowCount + rowsToBeAdded) - limit;
        //        aDataGrid.Rows.RemoveRange((currentRowCount - deleteCount), deleteCount);
        //    }
        //}

        //private void DeleteRowsIfNeeded(System.Data.DataTable aDataStore)
        //{
        //    int currentRowCount = aDataStore.Rows.Count;
        //    int rowsToBeAdded = 1;
        //    if ((currentRowCount + rowsToBeAdded) > limit)
        //    {
        //        int deleteCount = (currentRowCount + rowsToBeAdded) - limit;
        //        aDataStore.Rows.RemoveAt((currentRowCount - deleteCount));
        //    }
        //}

        private void populate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            if ((null == aDataTable) || (null == aConfig) || (null == aDataGrid))
            {
                return;
            }
            DataTable dataTable = new DataTable();
            foreach (DataColumn dc in aDataTable.Columns)
            {
                ColumnDetails details = getColumnDetailsForName(dc.ColumnName);
                if (details != null)
                {
                    dataTable.Columns.Add(details.DisplayName, details.ColumnType);
                }
                else
                {
                    dataTable.Columns.Add(dc.ColumnName, dc.DataType);
                }
            }

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
                    else
                    {
                        row[dc.ColumnName] = dr[dc.ColumnName];
                    }
                }
                dataTable.Rows.Add(row);
            }

#if DEBUG
            Console.WriteLine("Datatable size = " + dataTable.Rows.Count);
#endif

            base.DoPopulate(aConfig, dataTable, aDataGrid);

            //if (aDataGrid.Rows.Count == 0)
            //{
            //    base.DoPopulate(aConfig, dataTable, aDataGrid);
            //}
            //else
            //{
            //    if (dataTable.Rows.Count > 0)
            //    {
            //        DeleteRowsIfNeeded(dataTable, aDataGrid);
            //        aDataGrid.InsertRows(0, dataTable);
            //    }
            //}

            string gridHeaderText;
            gridHeaderText = string.Format("{0} Events - Last Refreshed : {1}",
                                            aDataGrid.Rows.Count,
                                            DateTime.Now.ToString());
            aDataGrid.UpdateCountControlText(gridHeaderText);
            setFormattedFilterMessage();
            //Commented Codes that will resize column width during a refresh
            //if (dataTable.Rows.Count > 0)
            //{
            //    aDataGrid.ResizeGridColumns(dataTable);
            //}
        }

        //private void addRowToStore(DataRow aRow, DataTable aStore)
        //{
        //    if (!LoadFromStore)
        //    {
        //        DeleteRowsIfNeeded(aStore);
        //        aStore.Rows.InsertAt(aRow, 0);
        //    }
        //}

        private void setFormattedFilterMessage()
        {
            _theTextEventUserControl.ShowFilterMessage();
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
                return _theTextEventUserControl.TheEventDetails.Severity((int)severity).ToString();
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
                return _theTextEventUserControl.TheEventDetails.SubSystem(((int)(uint)subSystem)).ToString();
            }
            else if (subSystem is long)
            {
                return _theTextEventUserControl.TheEventDetails.SubSystem(((int)(long)subSystem)).ToString();
            }
            return ret;
        }

        private string GetFormattedTimestamp(object timestamp)
        {
            return Trafodion.Manager.Framework.Utilities.GetTrafodionSQLLongDateTime((DateTime)timestamp, false);
            //return Trafodion.Manager.Framework.Utilities.GetTrafodionSQLLongDateTimeWithTimezone((DateTime)timestamp, false, false, ServerTimeZone);
        }

        #endregion Private methods
    }
}
