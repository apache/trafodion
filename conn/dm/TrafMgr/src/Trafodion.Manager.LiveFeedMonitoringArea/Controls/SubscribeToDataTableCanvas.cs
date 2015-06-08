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
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.LiveFeedFramework.Controls;
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;

namespace Trafodion.Manager.LiveFeedMonitoringArea.Controls
{
    /// <summary>
    /// The example of subscribing to a data table format of the live feed data provider
    /// Note: can only subscribe to one publication
    /// </summary>
    public partial class SubscribeToDataTableCanvas : UserControl
    {
        #region Fields

        private ConnectionDefinition _theConnectionDefinition = null;
        private UniversalWidgetConfig _config1 = null;
        private LiveFeedUniversalWidget _widget1 = null;
        private LiveFeedConnection.LiveFeedConnectionStateChanged _LiveFeedConnectionStateChangeHandler = null;
        private LiveFeedConnection.OnDataArrivalHandler _LiveFeedConnectionDataArrivalHandler = null;

        private string _theTitle = null;
        private string _thePublication = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: ConnectionDefn - the connection definition
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }

        /// <summary>
        /// Property: WindowTitle - the window title
        /// </summary>
        public string WindowTitle
        {
            get { return _theTitle; }
        }

        /// <summary>
        /// Property: Subscription - the publciation which this data provider is using
        /// </summary>
        public string Subscription
        {
            get { return _thePublication; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aTitle"></param>
        /// <param name="aPublications"></param>
        public SubscribeToDataTableCanvas(ConnectionDefinition aConnectionDefinition, string aTitle, string aPublication)
        {
            _theTitle = aTitle;
            _thePublication = aPublication;
            _theConnectionDefinition = aConnectionDefinition;
            InitializeComponent();
            ShowWidgets();
        }

        #endregion Constructors

        #region Private methods

        /// <summary>
        /// To dispose everything here
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                Shutdown();
            }
        }

        /// <summary>
        /// Create all of the widgets
        /// </summary>
        private void ShowWidgets()
        {
            GridLayoutManager gridLayoutManager = new GridLayoutManager(1, 1);
            gridLayoutManager.CellSpacing = 4;
            this._theCanvas.LayoutManager = gridLayoutManager;

            //Create a UW for XML raw data using the configuration
            _config1 = new UniversalWidgetConfig();
            LiveFeedDataProviderConfig qpidConfig = new LiveFeedDataProviderConfig();
            qpidConfig.ConnectionDefinition = this.ConnectionDefn;
            qpidConfig.Configure(_thePublication);

            _config1.DataProviderConfig = qpidConfig;
            _config1.Name = _theTitle;
            _config1.Title = _theTitle;
            _config1.ShowTimerSetupButton = false;
            _config1.ShowRefreshButton = false;

            _widget1 = new LiveFeedUniversalWidget();
            ((TabularDataDisplayControl)_widget1.DataDisplayControl).LineCountFormat = Properties.Resources.InitialEntryCount;
            ((TabularDataDisplayControl)_widget1.DataDisplayControl).ShowExportButtons = false;
            _widget1.UniversalWidgetConfiguration = _config1;

            _widget1.DataDisplayControl.DataDisplayHandler = new PublicationDataHandler(this);

            //Add the widget to the canvas
            GridConstraint gridConstraint = new GridConstraint(0, 0, 1, 1);
            WidgetContainer container1 = _theCanvas.AddWidget(_widget1, _config1.Name, _config1.Title, gridConstraint, -1);
            container1.AllowMaximize = false;
            container1.Resizable = false;
            container1.AutoSize = true;

            // Now, start the data provider. (all of the three widgets are sharing the same data provider)
            _widget1.StartDataProvider();
        }

        /// <summary>
        /// To clean everything up in this Canvas.  
        /// </summary>
        private void Shutdown()
        {
            _widget1.DataProvider.Stop();
        }

        #endregion Private methods
    }

    #region Class PublicationDataHandler

    /// <summary>
    /// The class for handling received publication data
    /// </summary>
    public class PublicationDataHandler : TabularDataDisplayHandler
    {
        #region Fields

        private SubscribeToDataTableCanvas _thePubsCanvas;
        private int limit = 1000;

        #endregion Fields

        #region Constructors

        /// <summary>
        /// The constructor
        /// </summary>
        /// <param name="aPubsCanvas"></param>
        public PublicationDataHandler(SubscribeToDataTableCanvas aPubsCanvas)
        {
            _thePubsCanvas = aPubsCanvas;
        }

        #endregion Constructor

        #region Public methods

        /// <summary>
        /// For applying filtering
        /// </summary>
        /// <param name="aConfig"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aDataGrid"></param>
        public void PopulateFromCache(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            lock (this)
            {
                aDataGrid.Rows.Clear();
                populate(aConfig, aDataTable, aDataGrid);
            }
        }

        /// <summary>
        /// Do populate
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

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// To adjust grid
        /// </summary>
        /// <param name="aDataTable"></param>
        /// <param name="aDataGrid"></param>
        private void DeleteRowsIfNeeded(System.Data.DataTable aDataTable, Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            int currentRowCount = aDataGrid.Rows.Count;
            int rowsToBeAdded = (aDataTable != null) ? aDataTable.Rows.Count : 0;
            if ((currentRowCount + rowsToBeAdded) > limit)
            {
                int deleteCount = (currentRowCount + rowsToBeAdded) - limit;
                aDataGrid.Rows.RemoveRange((currentRowCount - deleteCount), deleteCount);
            }
        }

        /// <summary>
        /// the real populate routine
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

            long recorded_ts = 0;
            DataTable dataTable = new DataTable();

            foreach (DataColumn dc in aDataTable.Columns)
            {
                if (dc.ColumnName.Contains("time_ts_utc") ||
                    dc.ColumnName.Contains("time_ts_lct") ||
                    dc.ColumnName.Equals("RECORDED_TS", StringComparison.OrdinalIgnoreCase))
                {
                    dataTable.Columns.Add(dc.ColumnName, typeof(DateTime));
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
                    if (dc.ColumnName.Contains("time_ts_utc"))
                    {
                        row[dc.ColumnName] = GetFormattedTimeFromUnixTimestampUTC((long)dr[dc.ColumnName] / 1000);
                    }
                    else if (dc.ColumnName.Contains("time_ts_lct"))
                    {
                        row[dc.ColumnName] = GetFormattedTimeFromUnixTimestampLCT((long)dr[dc.ColumnName] / 1000);
                    }
                    else if (dc.ColumnName.Equals("RECORDED_TS", StringComparison.OrdinalIgnoreCase))
                    {
                        if (recorded_ts == 0)
                        {
                            recorded_ts = (long)dr[dc.ColumnName] * 1000;
                        }
                        row[dc.ColumnName] = GetFormattedTimeFromUnixTimestampUTC((long)dr[dc.ColumnName] * 1000);

                    }
                    else
                    {
                        row[dc.ColumnName] = dr[dc.ColumnName];
                    }
                }
                dataTable.Rows.Add(row);
            }

            if (aDataGrid.Rows.Count == 0)
            {
                base.DoPopulate(aConfig, dataTable, aDataGrid);
            }
            else
            {
                //if (dataTable.Rows.Count > 0)
                //{
                //    DeleteRowsIfNeeded(dataTable, aDataGrid);
                //    aDataGrid.InsertRows(0, dataTable);
                //}
                base.DoPopulate(aConfig, dataTable, aDataGrid);
            }

            string gridHeaderText;
            if (aDataGrid.Rows.Count == 0)
            {
                gridHeaderText = string.Format(Properties.Resources.TotalEntryCount,
                                                "There is 0 entry",
                                                Trafodion.Manager.Framework.Utilities.GetFormattedDateTime(DateTime.Now));
            }
            else if (aDataGrid.Rows.Count == 1)
            {
                gridHeaderText = string.Format(Properties.Resources.TotalEntryCount,
                                                "There is 1 entry",
                                                (recorded_ts == 0) ? 
                                                Trafodion.Manager.Framework.Utilities.GetFormattedDateTime(DateTime.Now) :
                                                Trafodion.Manager.Framework.Utilities.GetFormattedTimeFromUnixTimestamp(recorded_ts));
            }
            else
            {
                gridHeaderText = string.Format(Properties.Resources.TotalEntryCount,
                                                String.Format("There are {0} entries, {1} in last publication ",
                                                aDataGrid.Rows.Count, dataTable.Rows.Count),
                                                (recorded_ts == 0) ?
                                                Trafodion.Manager.Framework.Utilities.GetFormattedDateTime(DateTime.Now) :
                                                Trafodion.Manager.Framework.Utilities.GetFormattedTimeFromUnixTimestamp(recorded_ts));
            }
            aDataGrid.ResizeGridColumns(dataTable);
            aDataGrid.UpdateCountControlText(gridHeaderText);
        }

        #endregion Private methods

        /// <summary>
        /// Formats the datetime to a human readable form with a input from Unix ls
        /// </summary>
        /// <param name="aTime"></param>
        /// <returns>DateTime Short format used by HP Database Manager</returns>
        static public DateTime GetFormattedTimeFromUnixTimestampUTC(long aTime)
        {
            long secondsSinceEpoch = aTime / 1000;
            DateTime theDateTime = new DateTime((secondsSinceEpoch * 10000000) + 621355968000000000, DateTimeKind.Utc);
            return theDateTime;
        }

        /// <summary>
        /// To format datatime for LCT timestamps
        /// </summary>
        /// <param name="aTime"></param>
        /// <returns></returns>
        static public DateTime GetFormattedTimeFromUnixTimestampLCT(long aTime)
        {
            long secondsSinceEpoch = aTime / 1000;
            DateTime theDateTime = new DateTime((secondsSinceEpoch * 10000000) + 621355968000000000, DateTimeKind.Local);
            return theDateTime;
        }
    }

    #endregion Class PublicationDataHandler
}
