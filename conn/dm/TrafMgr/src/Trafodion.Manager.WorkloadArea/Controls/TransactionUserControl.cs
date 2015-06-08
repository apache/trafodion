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

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class TransactionUserControl : UserControl
    {
        private const int NON_NODE = -1;
        private const string WMS_COMMAND_GET_ALL_TRANSACTIONS = "STATUS TRANSACTION";
        private const string WMS_COMMAND_GET_NODE_TRANSACTIONS = "STATUS TRANSACTION NODE {0}";
        private const string TRANSACTION_CONFIG_NAME = "WMS_TransactionConfig";
        private const string TRANSACTION_CONFIG_TITLE = "Get Transactions";

        private int nodeId = NON_NODE;
        private ConnectionDefinition connectionDefinition = null;

        private TrafodionIGrid grid;

        public static readonly Size IdealWindowSize = new Size(1020, 600);

        #region Constructor

        public TransactionUserControl(ConnectionDefinition connectionDefinition)
            : this(NON_NODE, connectionDefinition)
        {
        }

        public TransactionUserControl(int nodeId, ConnectionDefinition connectionDefinition)
        {
            InitializeComponent();

            this.nodeId = nodeId;
            this.connectionDefinition = connectionDefinition;

            Initialize();
        }

        #endregion


        #region Property

        private string CommandGetTransaction
        {
            get
            {
                return this.nodeId == NON_NODE ? WMS_COMMAND_GET_ALL_TRANSACTIONS : string.Format(WMS_COMMAND_GET_NODE_TRANSACTIONS, this.nodeId);
            }
        }

        #endregion

        public void Initialize()
        {
            UniversalWidgetConfig universalWidgetConfig = GetConfig();

            //Create a UW using the configuration             
            GenericUniversalWidget universalWidget = new GenericUniversalWidget();
            ((TabularDataDisplayControl)universalWidget.DataDisplayControl).LineCountFormat = "Transactions";
            universalWidget.DataProvider = new DatabaseDataProvider(universalWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig);
            universalWidget.UniversalWidgetConfiguration = universalWidgetConfig;
            universalWidget.Dock = DockStyle.Fill;
            this.Controls.Add(universalWidget);


#warning : WMS has remove the Trans ID drill down, so remove it according, but maybe it will be back in the future.
            ////Add popup menu items to the table
            //TabularDataDisplayControl dataDisplayControl = universalWidget.DataDisplayControl as TabularDataDisplayControl;
            //TrafodionIGridToolStripMenuItem getQueryIdMenuItem = new TrafodionIGridToolStripMenuItem();
            //getQueryIdMenuItem.Text = "Show Queries...";
            //getQueryIdMenuItem.Click += new EventHandler(getQueryIdMenuItem_Click);
            //dataDisplayControl.AddMenuItem(getQueryIdMenuItem);
            
            universalWidget.DataDisplayControl.DataDisplayHandler = new TransactionDataHandler();
            grid = ((TabularDataDisplayControl)universalWidget.DataDisplayControl).DataGrid;
            SetGrid(grid);

            universalWidget.DataProvider.Start();
        }

        private UniversalWidgetConfig GetConfig()
        {
            UniversalWidgetConfig universalWidgetConfig = WidgetRegistry.GetConfigFromPersistence(TRANSACTION_CONFIG_NAME);
            DatabaseDataProviderConfig dbConfig = null;
            if (universalWidgetConfig == null)
            {
                universalWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
                universalWidgetConfig.Name = TRANSACTION_CONFIG_NAME;
                universalWidgetConfig.Title = TRANSACTION_CONFIG_TITLE;
                dbConfig = (DatabaseDataProviderConfig)universalWidgetConfig.DataProviderConfig;
                dbConfig.CommandTimeout = WMSWorkloadCanvas.WORKLOAD_EXEC_TIMEOUT;
                dbConfig.OpenCommand = "WMSOPEN";
                dbConfig.CloseCommand = "WMSCLOSE";
                dbConfig.RefreshRate = WMSWorkloadCanvas.WORKLOAD_REFRESH_RATE;

                List<string> defaultVisibleColumns = new List<string>();
                defaultVisibleColumns.Add("TRANS_ID");
                defaultVisibleColumns.Add("TRANS_START_TIME");
                defaultVisibleColumns.Add("TRANS_DURATION");
                defaultVisibleColumns.Add("TRANS_ROLLBACK_DURATION");
                defaultVisibleColumns.Add("TRANS_NODE");
                defaultVisibleColumns.Add("TRANS_SEQNO");
                defaultVisibleColumns.Add("TRANS_PROCESS_NAME");
                defaultVisibleColumns.Add("TRANS_PROG_NAME");
                defaultVisibleColumns.Add("TRANS_STATE");
                universalWidgetConfig.DataProviderConfig.DefaultVisibleColumnNames = defaultVisibleColumns;

                List<ColumnMapping> columnMappings = new List<ColumnMapping>();
                columnMappings.Add(new ColumnMapping("TRANS_ID", "ID", 70));
                columnMappings.Add(new ColumnMapping("TRANS_START_TIME", "Start Time", 155));
                columnMappings.Add(new ColumnMapping("TRANS_DURATION", "Duration Minutes", 120));
                columnMappings.Add(new ColumnMapping("TRANS_ROLLBACK_DURATION", "Rollback Duration Minutes", 170));
                columnMappings.Add(new ColumnMapping("TRANS_NODE", "Node", 40));
                columnMappings.Add(new ColumnMapping("TRANS_SEQNO", "Sequence Number", 120));
                columnMappings.Add(new ColumnMapping("TRANS_PROCESS_NAME", "Process Name", 120));
                columnMappings.Add(new ColumnMapping("TRANS_PROG_NAME", "Program Name", 120));
                columnMappings.Add(new ColumnMapping("TRANS_STATE", "State", 100));
                universalWidgetConfig.DataProviderConfig.ColumnMappings = columnMappings;
            }
            else
            {
                dbConfig = (DatabaseDataProviderConfig)universalWidgetConfig.DataProviderConfig;
            }

            dbConfig.SQLText = CommandGetTransaction;
            dbConfig.TimerPaused = false;

            //Make this data provider's timer to continue after encountered an error. 
            universalWidgetConfig.DataProviderConfig.TimerContinuesOnError = true;

            //Set the connection definition if available
            universalWidgetConfig.DataProviderConfig.ConnectionDefinition = this.connectionDefinition;
            universalWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.Show;
            universalWidgetConfig.ShowExportButtons = true;
            universalWidgetConfig.ShowHelpButton = true;
            universalWidgetConfig.HelpTopic = HelpTopics.Transaction;

            return universalWidgetConfig;
        }

        private void SetGrid(TrafodionIGrid grid)
        {
#warning : WMS has remove the Trans ID drill down, so remove it according, but maybe it will be back in the future.
            //grid.DoubleClickHandler = grid_DoubleClick;

            List<string> alwaysHiddenColumnNames = new List<string>();
            grid.AlwaysHiddenColumnNames.AddRange(alwaysHiddenColumnNames);
        }
        
        void getQueryIdMenuItem_Click(object sender, EventArgs events)
        {
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    grid_DoubleClick(eventObj.Row);
                }
            }
        }

        void grid_DoubleClick(int row)
        {
            if (row < 0) return;

            long transactionId = long.Parse(grid.Rows[row].Cells["TRANS_ID"].Value.ToString());

            TransactionQueriesUserControl parentChildQueriesUserControl = new TransactionQueriesUserControl(transactionId, this.connectionDefinition);
            string windowTitle = string.Format(Properties.Resources.TitleQueriessOfSpecifiedNode, transactionId);
            Utilities.LaunchManagedWindow(windowTitle, parentChildQueriesUserControl, this.connectionDefinition, TransactionQueriesUserControl.IdealWindowSize, true);
        }

        private class TransactionDataHandler : TabularDataDisplayHandler
        {
            private const string COUNT_TEXT = "There are {0} transactions";

            public TransactionDataHandler()
            {
            }

            public override void DoPopulate(UniversalWidgetConfig config, System.Data.DataTable dataTable, TrafodionIGrid dataGrid)
            {
                base.DoPopulate(config, dataTable, dataGrid);
                dataGrid.UpdateCountControlText(COUNT_TEXT);
            }
        }
    }
}
