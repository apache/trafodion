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
using System.Collections.Generic;
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.LiveFeedFramework.Controls;

namespace Trafodion.Manager.LiveFeedMonitoringArea.Controls
{
    /// <summary>
    /// To class to display all of the connectors in a system instance
    /// </summary>
    public partial class DisplayConnectorsUserControl : UserControl
    {
        #region Fields

        private TrafodionIGrid _theIGrid = null;
        private ConnectionDefinition _theConnectionDefinition = null;
        private DataTable _theDataTable = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: DataTable - the data table keeping all of the current connector info
        /// </summary>
        public DataTable DataTable
        {
            get { return _theDataTable; }
            set { _theDataTable = value; }
        }

        #endregion Properties 

        #region Constructor
        
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public DisplayConnectorsUserControl(ConnectionDefinition aConnectionDefinition)
        {
            _theConnectionDefinition = aConnectionDefinition;
            InitializeComponent();
            ShowWidget();
        }

        #endregion Constructor 

        #region Public methods

        /// <summary>
        /// To reload the grid
        /// </summary>
        public void ReloadData()
        {
            LoadData(_theDataTable);
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// To load the grid with new data
        /// </summary>
        /// <param name="aTable"></param>
        private void LoadData(DataTable aTable)
        {
            _theIGrid.FillWithData(aTable);
            _theIGrid.ResizeGridColumns(aTable);
        }

        /// <summary>
        /// To create the widget 
        /// </summary>
        private void ShowWidget()
        {
            _theDataTable = new DataTable();

            _theIGrid = new TrafodionIGrid();
            _theIGrid.Dock = DockStyle.Fill;
            _theLowerPanel.Controls.Add(_theIGrid);

            _theDataTable.Columns.Add("Connector", typeof(string));
            _theDataTable.Columns.Add("Queue Name", typeof(string));
            _theDataTable.Columns.Add("Total Packages Received", typeof(long));

            List<LiveFeedConnection> connections = LiveFeedConnectionRegistry.Instance.GetActiveLiveFeedConnections(_theConnectionDefinition);
            if (connections != null)
            {
                foreach (LiveFeedConnection conn in connections)
                {
                    _theDataTable.Rows.Add(new object[] { conn.ConnectionDefn.Name, conn.LiveFeedQueues[0].Name, conn.TotalReceivedCount });
                }
            }

            _theIGrid.FillWithData(_theDataTable);
            _theIGrid.ResizeGridColumns(_theDataTable);
        }

        /// <summary>
        /// Config button handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theConfigButton_Click(object sender, EventArgs e)
        {
            //LiveFeedBrokerConfigDialog dialog = new LiveFeedBrokerConfigDialog(_theConnectionDefinition);
            //dialog.ShowDialog();
        }

        #endregion Private methods
    }
}
