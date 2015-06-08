//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.DatabaseArea.Controls;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.WorkloadArea.Model;
using System.Collections.Generic;
using TenTec.Windows.iGridLib;
using System.Drawing;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.Framework.Controls;
using System.Data;
using System.Data.Odbc;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class TransactionQueriesUserControl : WMSWorkloadCanvas
    {
        private const string WMS_COMMAND_GET_TRANSACTION_QUERIES = "STATUS TRANSACTION TRANSID {0}";
        private const string COLUMN_KEY_TRANS_QUERY_ID = "TRANS_QUERY_ID";
        private const string COLUMN_TEXT_TRANS_QUERY_ID = "Query ID";
        private const string COLUMN_KEY_TRANS_QUERY_STATE = "TRANS_QUERY_STATE";
        private const string COLUMN_TEXT_TRANS_QUERY_STATE = "Query State";
        private const string COLUMN_KEY_TRANS_QUERY_TEXT = "TRANS_QUERY_TEXT";
        private const string COLUMN_TEXT_TRANS_QUERY_TEXT = "Query Text";
        private const string TRANSACTION_QUERY_CONFIG_NAME = "WMS_TransactionQueryConfig";
        private const string TRANSACTION_QUERY_CONFIG_TITLE = "Get Transaction Queries";

        private long transactionId;
        private ConnectionDefinition connectionDefinition;
        private Size preferredSize;

        private TrafodionProgressUserControl progressUserControl;
        private TrafodionIGrid grid;

        public static readonly Size IdealWindowSize = new Size(1200, 600);

        #region Constructor

        public TransactionQueriesUserControl(long transactionId, ConnectionDefinition connectionDefinition)
        {
            this.transactionId = transactionId;
            this.connectionDefinition = connectionDefinition;

            InitializeComponent();
            Initialize();
        }

        #endregion

        #region Property

        private string CommandGetTransactionQueries
        {
            get { return string.Format(WMS_COMMAND_GET_TRANSACTION_QUERIES, this.transactionId); }
        }

        #endregion

        public void Initialize()
        {
            UniversalWidgetConfig universalWidgetConfig = GetConfig();

            //Create a UW using the configuration             
            GenericUniversalWidget universalWidget = new GenericUniversalWidget();
            ((TabularDataDisplayControl)universalWidget.DataDisplayControl).LineCountFormat = "Transaction Queries";
            universalWidget.DataProvider = new TransactionQueryDataProvider(connectionDefinition, CommandGetTransactionQueries, universalWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig);
            universalWidget.UniversalWidgetConfiguration = universalWidgetConfig;
            universalWidget.Dock = DockStyle.Fill;
            this.Controls.Add(universalWidget);

            universalWidget.DataDisplayControl.DataDisplayHandler = new TransactionQueryDataHandler();
            SetGrid(((TabularDataDisplayControl)universalWidget.DataDisplayControl).DataGrid);

            universalWidget.DataProvider.Start();
        }
        
        private UniversalWidgetConfig GetConfig()
        {
            UniversalWidgetConfig universalWidgetConfig = WidgetRegistry.GetConfigFromPersistence(TRANSACTION_QUERY_CONFIG_NAME);
            DatabaseDataProviderConfig dbConfig = null;
            if (universalWidgetConfig == null)
            {
                universalWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
                universalWidgetConfig.Name = TRANSACTION_QUERY_CONFIG_NAME;
                universalWidgetConfig.Title = TRANSACTION_QUERY_CONFIG_TITLE;
                dbConfig = (DatabaseDataProviderConfig)universalWidgetConfig.DataProviderConfig;
                dbConfig.CommandTimeout = WMSWorkloadCanvas.WORKLOAD_EXEC_TIMEOUT;
                dbConfig.OpenCommand = "WMSOPEN";
                dbConfig.CloseCommand = "WMSCLOSE";
                dbConfig.RefreshRate = WMSWorkloadCanvas.WORKLOAD_REFRESH_RATE;


                universalWidgetConfig.DataProviderConfig.ColumnMappings = new List<ColumnMapping>
                {
                    new ColumnMapping(){
                        InternalName = COLUMN_KEY_TRANS_QUERY_ID,
                        ExternalName = COLUMN_TEXT_TRANS_QUERY_ID,
                        ColumnWidth = 535,
                    },
                                
                    new ColumnMapping(){
                        InternalName = COLUMN_KEY_TRANS_QUERY_STATE,
                        ExternalName = COLUMN_TEXT_TRANS_QUERY_STATE,
                        ColumnWidth = 85
                    },
                                
                    new ColumnMapping(){
                        InternalName = COLUMN_KEY_TRANS_QUERY_TEXT,
                        ExternalName = COLUMN_TEXT_TRANS_QUERY_TEXT,
                        ColumnWidth = 575
                    }
                };
            }
            else
            {
                dbConfig = (DatabaseDataProviderConfig)universalWidgetConfig.DataProviderConfig;
            }

            dbConfig.SQLText = CommandGetTransactionQueries;
            dbConfig.TimerPaused = false;

            
            //Make this data provider's timer to continue after encountered an error. 
            universalWidgetConfig.DataProviderConfig.TimerContinuesOnError = true;

            //Set the connection definition if available
            universalWidgetConfig.DataProviderConfig.ConnectionDefinition = this.connectionDefinition;
            universalWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.Show;
            universalWidgetConfig.ShowHelpButton = true;
            universalWidgetConfig.ShowExportButtons = true;
            universalWidgetConfig.HelpTopic = HelpTopics.Transaction;

#warning:  below code should be removed if Topic is ready
            universalWidgetConfig.ShowHelpButton = false;

            return universalWidgetConfig;
        }

        private void SetGrid(TrafodionIGrid grid)
        {
            this.grid = grid;

            SetGridStyle(grid);
            SetGridAction(grid); 
        }

        private void SetGridStyle(TrafodionIGrid grid)
        {
            grid.Dock = DockStyle.Fill;
        }

        private void SetGridAction(TrafodionIGrid grid)
        { 
            grid.DoubleClickHandler = ShowQueryWorkloadDetail;
            grid.CellClick += new iGCellClickEventHandler(grid_CellClick);

            TrafodionIGridToolStripMenuItem showQueryDetailMenuItem = new TrafodionIGridToolStripMenuItem();
            showQueryDetailMenuItem.Text = "Workload Detail...";
            showQueryDetailMenuItem.Click += new EventHandler(showQueryDetailMenuItem_Click);
            grid.AddContextMenu(showQueryDetailMenuItem);    
        }

        private void ShowQueryWorkloadDetail(int gridRowIndex)
        {
            if ( gridRowIndex < 0 ) return;

            string queryId = this.grid.Cells[gridRowIndex, 0].Value as string;
            Connection connection = new Connection(this.connectionDefinition);
            if (connection != null)
            {
                OdbcConnection odbcCon = connection.OpenOdbcConnection;
                OdbcCommand command = new OdbcCommand();
                bool wmsOpened = false;
                try
                {
                    Cursor.Current = Cursors.WaitCursor;
                    command.Connection = odbcCon;
                    command.CommandTimeout = WORKLOAD_EXEC_TIMEOUT;
                    command.CommandText = "WMSOPEN";
                    command.ExecuteNonQuery();
                    wmsOpened = true;
                    string sql = "STATUS QUERY " + queryId + " MERGED";
                    DataTable dataTable = WMSOdbcAccess.getDataTableFromSQL(odbcCon, command, sql);

                    if (dataTable.Rows.Count > 0)
                    {
                        DataRow dr = dataTable.Rows[0];

                        string mxosrvrStartTime = "";
                        if (dataTable.Columns.Contains(WmsCommand.COL_QUERY_START_TIME))
                        {
                            mxosrvrStartTime = dataTable.Rows[0][WmsCommand.COL_QUERY_START_TIME] as string;
                        }
                        string title = string.Format(Properties.Resources.TitleQueryDetails, string.Format("{0}@{1}", queryId, mxosrvrStartTime));
                        WMSQueryDetailUserControl queryInfo = GetWatchedQueryWindow(this.connectionDefinition, queryId, mxosrvrStartTime);
                        if (queryInfo != null)
                        {
                            queryInfo.LoadData(dataTable);
                            string systemIdentifier = (this.connectionDefinition != null) ? this.connectionDefinition.Name + " : " : "";
                            string windowTitle = TrafodionForm.TitlePrefix + systemIdentifier + title;
                            if (WindowsManager.Exists(windowTitle))
                            {
                                WindowsManager.Restore(windowTitle);
                                WindowsManager.BringToFront(windowTitle);
                            }
                        }
                        else
                        {
                            WMSQueryDetailUserControl queryDetails = new WMSQueryDetailUserControl(this, this.connectionDefinition, queryId, mxosrvrStartTime, dataTable);
                            AddQueryToWatch(queryDetails);
                            Utilities.LaunchManagedWindow(string.Format(Properties.Resources.TitleQueryDetails, string.Format("{0}@{1}", queryId, mxosrvrStartTime)),
                                queryDetails, true, this.connectionDefinition, WMSQueryDetailUserControl.IdealWindowSize, false);
                        }

                    }
                    else
                    {
                        string title = string.Format(Properties.Resources.TitleQueryDetails, queryId);
                        MessageBox.Show("Query statistics not available for the selected query in WMS. It may be that the query has already completed.", title, MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    }
                }
                catch (OdbcException ex)
                {
                    string title = string.Format(Properties.Resources.TitleQueryDetails, queryId);
                    MessageBox.Show("Error: Query statistics not available for the selected query in WMS. It may be that the query has already completed.", title, MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
                finally
                {
                    if (wmsOpened)
                    {
                        command.CommandText = "WMSCLOSE";
                        command.ExecuteNonQuery();
                    }
                    if (connection != null)
                    {
                        connection.Close();
                    }
                    Cursor.Current = Cursors.Default;
                }
            }
        }

        void showQueryDetailMenuItem_Click(object sender, EventArgs e)
        {
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    ShowQueryWorkloadDetail(eventObj.Row);
                }
            }
        }

        private void grid_CellClick(object sender, iGCellClickEventArgs e)
        {
            if (((iGrid)sender).Cols[e.ColIndex].Key == COLUMN_KEY_TRANS_QUERY_ID)
            {
                ShowQueryWorkloadDetail(e.RowIndex);
            }
        }


        private class TransactionQueryDataHandler : TabularDataDisplayHandler
        {
            private const string COUNT_TEXT = "There are {0} queries";

            public TransactionQueryDataHandler()
            {
            }

            public override void DoPopulate(UniversalWidgetConfig config, System.Data.DataTable dataTable, TrafodionIGrid dataGrid)
            {
                base.DoPopulate(config, dataTable, dataGrid);
                SetHyperLinkColumn(dataGrid);
                dataGrid.UpdateCountControlText(COUNT_TEXT);
            }

            private void SetHyperLinkColumn(TrafodionIGrid grid)
            {
                TrafodionIGridHyperlinkCellManager hyperLinkCellManager = new TrafodionIGridHyperlinkCellManager();
                int indexQueryIdColumn = grid.Cols[COLUMN_KEY_TRANS_QUERY_ID].Index;
                hyperLinkCellManager.Attach(grid, indexQueryIdColumn);
            }
        }
    }
}
