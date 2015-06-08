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
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.LiveFeedMonitoringArea.Controls
{
    /// <summary>
    /// The user control to display all subscriptions
    /// </summary>
    public partial class DisplayPublicationsUserControl : UserControl
    {
        #region Fields

        private iGrid _theIGrid = null;
        private ConnectionDefinition _theConnectionDefinition = null;
        private DataTable _theDataTable = null;
        private string[] _theSelectedPublications = null;
        private TenTec.Windows.iGridLib.iGCellStyle fCellStyleEllipsisCol;
        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: DataTable - of the display data grid 
        /// </summary>
        public DataTable DataTable
        {
            get { return _theDataTable; }
            set { _theDataTable = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public DisplayPublicationsUserControl(ConnectionDefinition aConnectionDefinition)
        {
            _theConnectionDefinition = aConnectionDefinition;
            InitializeComponent();
            ShowWidget();
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// To reload all data
        /// </summary>
        public void ReloadData()
        {
            LoadData(_theDataTable);
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// To reload data grid with the data table 
        /// </summary>
        /// <param name="aTable"></param>
        private void LoadData(DataTable aTable)
        {
            for (int i = 0; i < aTable.Rows.Count; i++)
            {
                _theIGrid.Rows[i].Cells[1].Value = aTable.Rows[i][1];
            }
        }

        /// <summary>
        /// To show the widget
        /// </summary>
        private void ShowWidget()
        {
            _theDataTable = new DataTable();

            this.fCellStyleEllipsisCol = new TenTec.Windows.iGridLib.iGCellStyle(true);
            // 
            // fCellStyleEllipsisCol
            // 
            this.fCellStyleEllipsisCol.DropDownControl = null;
            this.fCellStyleEllipsisCol.EmptyStringAs = TenTec.Windows.iGridLib.iGEmptyStringAs.Null;
            this.fCellStyleEllipsisCol.Flags = ((TenTec.Windows.iGridLib.iGCellFlags)((TenTec.Windows.iGridLib.iGCellFlags.DisplayText | TenTec.Windows.iGridLib.iGCellFlags.DisplayImage)));
            this.fCellStyleEllipsisCol.ImageList = null;
            this.fCellStyleEllipsisCol.SingleClickEdit = TenTec.Windows.iGridLib.iGBool.NotSet;
            this.fCellStyleEllipsisCol.TypeFlags = TenTec.Windows.iGridLib.iGCellTypeFlags.HasEllipsisButton;
            this.fCellStyleEllipsisCol.ValueType = null;

            _theIGrid = new iGrid();

            _theIGrid.Dock = DockStyle.Fill;
            _theLowerPanel.Controls.Add(_theIGrid);
            _theIGrid.Cols.Add("Publications", 250);
            _theIGrid.Cols.Add("Total subscriptions", 200);
            iGCol col = _theIGrid.Cols.Add("To Subscribe", 200);
            col.CellStyle = this.fCellStyleEllipsisCol;

            _theIGrid.EllipsisButtonGlyph = _thePictureBox.Image;
            _theIGrid.EllipsisButtonClick += new iGEllipsisButtonClickEventHandler(_theIGrid_EllipsisButtonClick);

            // Now, define the data table.
            _theDataTable.Columns.Add("Publications", typeof(string));
            _theDataTable.Columns.Add("Total subscriptions", typeof(long));

            List<LiveFeedConnection> connections = LiveFeedConnectionRegistry.Instance.GetActiveLiveFeedConnections(_theConnectionDefinition);

            foreach (string pubs in LiveFeedRoutingKeyMapper.AllPublicationNames)
            {
                int totalSubscriptions = 0;

                if (connections != null)
                {
                    foreach (LiveFeedConnection conn in connections)
                    {
                        if (conn.Subscriptions.ContainsKey(pubs))
                        {
                            totalSubscriptions += conn.Subscriptions[pubs];
                        }
                    }
                }

                _theDataTable.Rows.Add(new object[] { pubs, totalSubscriptions });
            }

            foreach (DataRow dr in _theDataTable.Rows)
            {
                iGRow row = _theIGrid.Rows.Add();
                row.Cells[0].Value = dr[0];
                row.Cells[1].Value = dr[1];
            }
        }

        /// <summary>
        /// Event handler for ellipsis button clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void _theIGrid_EllipsisButtonClick(object sender, iGEllipsisButtonClickEventArgs e)
        {
            string sub = _theIGrid.Rows[e.RowIndex].Cells[0].Value as string;

            if (string.IsNullOrEmpty(sub))
            {
                MessageBox.Show("No publication is selected!");
            }
            else
            {
                SubscribeToDataTableCanvas canvas = new SubscribeToDataTableCanvas(_theConnectionDefinition, "Publications: " + sub, sub);
                Trafodion.Manager.Framework.Controls.WindowsManager.PutInWindow(new System.Drawing.Size(900, 500), canvas, "Show Publication", _theConnectionDefinition);
            }
        }

        /// <summary>
        /// event handler for subscribe button clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theSubscribeButton_Click(object sender, EventArgs e)
        {
            TrafodionFilterSelection fs = new TrafodionFilterSelection("Select Publications", LiveFeedRoutingKeyMapper.AllPublicationNames, _theSelectedPublications);
            fs.AvailableListTitle = "Available Publications";
            fs.SelectedListTitle = "Selected Publications";
            DialogResult result = fs.ShowDialog();
            if (result == DialogResult.OK)
            {
                string[] selected = fs.SelectedList;
                _theSelectedPublications = selected;
            }

            if (_theSelectedPublications == null)
            {
                MessageBox.Show("No publication is selected!");
            }
            else
            {
                string title = "";
                foreach (string pub in _theSelectedPublications)
                {
                    title = string.IsNullOrEmpty(title) ? pub : ", " + pub;
                }

                SubscribeToProtoBufCanvas canvas = new SubscribeToProtoBufCanvas(_theConnectionDefinition, "Publications: " + title, _theSelectedPublications);
                Trafodion.Manager.Framework.Controls.WindowsManager.PutInWindow(new System.Drawing.Size(900, 500), canvas, "Show Publication", _theConnectionDefinition);
            }
        }

        #endregion Private methods
    }
}
