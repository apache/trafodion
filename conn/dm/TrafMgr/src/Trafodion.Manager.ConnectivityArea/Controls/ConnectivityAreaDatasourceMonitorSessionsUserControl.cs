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
using System.Collections;
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.ConnectivityArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;
using TenTec.Windows.iGridLib;


namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// 
    /// </summary>
    public partial class ConnectivityAreaDatasourceMonitorSessionsUserControl : UserControl, ICloneToWindow
    {

        #region Fields

        NDCSDataSource _NdcsDataSource;
        private Trafodion.Manager.Framework.Navigation.NavigationTreeView appropriateTreeView = null;
        ConnectivityObjectsIGrid<NDCSSystem> ConnectivityiGrid = new ConnectivityObjectsIGrid<NDCSSystem>();
        TrafodionIGridHyperlinkCellManager _hyperLinkCellManager = new TrafodionIGridHyperlinkCellManager();
        iGCellClickEventHandler _cellClickHandler = null;

        // always the first column, NDCS Service Name, has underline link.
        int HyperlinkColIndex = -1;
        
        #endregion Fields

        #region Properties

        /// <summary>
        /// Gets the underlying system
        /// </summary>
        public NDCSDataSource NDCSDataSource
        {
            get { return _NdcsDataSource; }
        }


        #endregion Properties

        #region ICloneToWindow Members

        /// <summary>
        /// Creates a new instance
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            Control theUserControl = null;
            theUserControl = new ConnectivityAreaDatasourceMonitorSessionsUserControl(appropriateTreeView, this._NdcsDataSource);
            ((ConnectivityAreaDatasourceMonitorSessionsUserControl)theUserControl).Populate();

            return theUserControl;

        }

        /// <summary>
        /// Get the window title of the cloned window
        /// </summary>
        public string WindowTitle
        {
            get
            {
                return Properties.Resources.TabPageTitle_Monitoring + " | " +
                    Properties.Resources.TabPageLabel_NDCSServerStatus + " | " +
                    "Data Source Name - " + this._NdcsDataSource.Name;

            }
        }

        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return this._NdcsDataSource.NDCSSystem.ConnectionDefinition; }
        }

        #endregion

        /// <summary>
        /// 
        /// </summary>
        /// <param name="aDataSource"></param>
        public ConnectivityAreaDatasourceMonitorSessionsUserControl(Trafodion.Manager.Framework.Navigation.NavigationTreeView treeView,
            NDCSDataSource aDataSource)
        {
            InitializeComponent();
            _NdcsDataSource = aDataSource;
            this.appropriateTreeView = treeView;
            SetInitialValues();
        }

        /// <summary>
        /// 
        /// </summary>
        internal void Populate()
        {
            BuildIGrid();            
            
        }

        /// <summary>
        /// 
        /// </summary>
        public void BuildIGrid()
        {
            try
            {
                DataTable dataTable = _NdcsDataSource.GetAllSessions();

                ConnectivityiGrid.BeginUpdate();
                ConnectivityiGrid.FillWithData(dataTable);
                ConnectivityiGrid.ResizeGridColumns(dataTable);
                ConnectivityiGrid.Dock = DockStyle.Fill;

                if (this.appropriateTreeView != null)
                {
                    ConnectivityiGrid.TreeView = this.appropriateTreeView;
                }
                else
                {
                    ConnectivityiGrid.TreeView = null;
                }

                for (int i = 0; i < ConnectivityiGrid.Rows.Count; i++)
                {
                    ConnectivityiGrid.Rows[i].Tag = Trafodion.Manager.Properties.Resources.MySystemsText + "\\" +
                        this._NdcsDataSource.NDCSSystem.Name + "\\" + Properties.Resources.TabPageLabel_Services + "\\" +
                        ConnectivityiGrid.Rows[i].Cells[Properties.Resources.ServiceName].Value.ToString();
                }
                ConnectivityiGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;

                string timestamp = Utilities.CurrentFormattedDateTime;
                string _refreshTimeStampString;

                _refreshTimeStampString = string.Format(Properties.Resources.RefreshTimestampCaption, timestamp)
                   + string.Format(Properties.Resources.ServerStatusForDatasource, _NdcsDataSource.Name, "{0}");

                ConnectivityiGrid.UpdateCountControlText(_refreshTimeStampString);

                ConnectivityiGrid.EndUpdate();
                _hyperLinkCellManager.Attach(ConnectivityiGrid, HyperlinkColIndex);
                // It's better to call this method after you attached iGHyperlinkCellManager to the grid
                // because the manager can change the formatting of the links dynamically
                ConnectivityiGrid.ResizeGridColumns(dataTable, 7, 30);

            }
            catch (System.Data.Odbc.OdbcException oe)
            {
                // Got an ODBC erorr. Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), oe.Message, Properties.Resources.ODBCError, MessageBoxButtons.OK);
            }
            catch (Exception ex)
            {
                // Got some other exception.  Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.SystemError, MessageBoxButtons.OK);
            }
        }

        /// <summary>
        /// Stores the initial settings
        /// </summary>
        private void SetInitialValues()
        {
            // handles hyper link in iGrid cells
            _cellClickHandler = new iGCellClickEventHandler(MethodsGrid_CellClick);
            ConnectivityiGrid.CellClick += _cellClickHandler;
            HyperlinkColIndex = 1;

            Control theDataGridSaveControl = ConnectivityiGrid.GetButtonControl();
            theDataGridSaveControl.Dock = DockStyle.Fill;
            _bottomRightPanel.Controls.Add(theDataGridSaveControl);
            ConnectivityiGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            ConnectivityiGrid.SelectionChanged += new EventHandler(ConnectivityiGrid_SelectionChanged);

            TrafodionPanel1.Controls.Add(ConnectivityiGrid);

            this._bottomLeftPanel.SuspendLayout();

            this._stopButton.Location = new System.Drawing.Point(3, 4);
            this._refreshButton.Location = new System.Drawing.Point(60, 4);
            this._bottomLeftPanel.Controls.Add(this._stopButton);
            this._bottomLeftPanel.Controls.Add(this._refreshButton);
            this._stopButton.Enabled = false;

            ToolTip forRightPaneButton = new ToolTip();
            forRightPaneButton.SetToolTip(this._refreshButton, Properties.Resources.Tooltip_Btn_Datasource_Refresh);

            forRightPaneButton.AutomaticDelay = 1500;

            this._bottomLeftPanel.ResumeLayout(false);

            // Add the CountControl To the Grid, update it after the grid is filled
            ConnectivityiGrid.AddCountControlToParent(" ", DockStyle.Top);
        }
                

        private void _stopButton_Click(object sender, EventArgs e)
        {
            StopNDCSServerDialog theStopNDCSServerDialog;
            int theConnectedCount = 0;

            try
            {
                theStopNDCSServerDialog = new StopNDCSServerDialog(theConnectedCount);
                if (theStopNDCSServerDialog.ShowDialog(this) == DialogResult.OK)
                {
                    Cursor.Current = Cursors.WaitCursor;
                    try
                    {
                        NDCSSession theSessionToStop;

                        foreach (TenTec.Windows.iGridLib.iGRow theRow in ConnectivityiGrid.SelectedRows)
                        {
                            theSessionToStop = (NDCSSession)theRow.Cells[Properties.Resources.ProcessName].Value;
                            theSessionToStop.Stop(NDCSObject.StopMode.STOP_DISCONNECT);
                        }
                    }
                    finally
                    {
                        Cursor.Current = Cursors.Default;
                        RefreshDataGrid();
                    }
                }

                theStopNDCSServerDialog.Dispose();
            }
            catch (System.Data.Odbc.OdbcException oe)
            {
                // Got an ODBC erorr. Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), oe.Message, Properties.Resources.ODBCError, MessageBoxButtons.OK);
            }
            catch (Exception ex)
            {
                // Got some other exception.  Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.SystemError, MessageBoxButtons.OK);
            }

        }

        private void RefreshDataGrid()
        {
            Cursor.Current = Cursors.WaitCursor;
            try
            {
                ArrayList sortObject = Trafodion.Manager.Framework.Utilities.GetSortObject(ConnectivityiGrid);

                ConnectivityiGrid.Rows.Clear();
                BuildIGrid();

                //re-apply sort
                Trafodion.Manager.Framework.Utilities.ApplySort(this.ConnectivityiGrid, sortObject);
            }
            finally
            {
                Cursor.Current = Cursors.Default;
                this._stopButton.Enabled = false;
            }

        }


        private void _refreshButton_Click(object sender, EventArgs e)
        {
            RefreshDataGrid();
            
        }


        void ConnectivityiGrid_SelectionChanged(object sender, EventArgs e)
        {
            if (this.ConnectivityiGrid.SelectedRows.Count > 0)
            {
                this._stopButton.Enabled = true;
                this._stopButton.Enabled = ConnectionDefn.ComponentPrivilegeExists("HPDCS", "ADMIN_STOP");
            }
        }

        private void MethodsGrid_CellClick(object sender, iGCellClickEventArgs e)
        {
            iGCell CurrentCell = ConnectivityiGrid.Cells[e.RowIndex, e.ColIndex];

            // Handle hyperline here
            if (e.ColIndex == HyperlinkColIndex)
            {
                try
                {
                    if (this.appropriateTreeView != null)
                    {
                        TreeNode clickedNode = appropriateTreeView.FindByFullPath(this.ConnectivityiGrid.Rows[e.RowIndex].Tag.ToString());
                        appropriateTreeView.Select(clickedNode);
                        appropriateTreeView.FireSelected((NavigationTreeNode)clickedNode);
                    }
                }
                catch (Exception ex) { }

            }
        }

    }
}
