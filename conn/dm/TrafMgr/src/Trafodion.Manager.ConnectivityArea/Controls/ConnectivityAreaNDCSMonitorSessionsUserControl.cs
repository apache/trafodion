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
 
    public partial class ConnectivityAreaNDCSMonitorSessionsUserControl : UserControl, ICloneToWindow
    {
        #region Fields

        private NDCSSystem _NdcsSystem;
        private Trafodion.Manager.Framework.Navigation.NavigationTreeView appropriateTreeView = null;
        private ConnectivityObjectsIGrid<NDCSService> ConnectivityNDCSServiceDataGrid = new ConnectivityObjectsIGrid<NDCSService>();
        private NDCSService _theNDCSService = null;
        TrafodionIGridHyperlinkCellManager _hyperLinkCellManager = new TrafodionIGridHyperlinkCellManager();
        iGCellClickEventHandler _cellClickHandler = null;

        // default is the column "Data Source Name" has underline link.
        int HyperlinkColIndex = -1;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Gets the underlying system
        /// </summary>
        public NDCSSystem NdcsSystem
        {
            get { return _NdcsSystem; }
            set { _NdcsSystem = value; }
        }

        /// <summary>
        /// Gets the underlying NDCS Service object
        /// </summary>
        public NDCSService NdcsService
        {
            get { return _theNDCSService; }
        }

        /// <summary>
        /// Gets the underlying NDCS Service Name
        /// </summary>
        public string TheServiceName
        {
            get { return _theNDCSService.Name; }
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

            if (this.NdcsService != null)
            {
                theUserControl = new ConnectivityAreaNDCSMonitorSessionsUserControl(NdcsSystem, this.appropriateTreeView, NdcsService);

            }
            else
            {
                theUserControl = new ConnectivityAreaNDCSMonitorSessionsUserControl(NdcsSystem, this.appropriateTreeView);
            }

            ((ConnectivityAreaNDCSMonitorSessionsUserControl)theUserControl).Populate();
            return theUserControl;

        }

        /// <summary>
        /// Get the window title of the cloned window
        /// </summary>
        public string WindowTitle
        {
            get
            {
                if (this.NdcsService != null)
                {
                    return Properties.Resources.TabPageTitle_Monitoring + " | " +
                            Properties.Resources.TabPageLabel_NDCSServerStatus + " | " +
                            this.TheServiceName;
                }
                else
                {
                    return Properties.Resources.TabPageTitle_Monitoring + " | " +
                            Properties.Resources.TabPageLabel_NDCSServerStatus + " | ALL";
                }
            }
        }

        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return NdcsSystem.ConnectionDefinition; }
        }

        #endregion

        
        /// <summary>
        /// 
        /// </summary>
        /// <param name="aNdcsSystem"></param>
        public ConnectivityAreaNDCSMonitorSessionsUserControl(NDCSSystem aNdcsSystem, Trafodion.Manager.Framework.Navigation.NavigationTreeView treeView)
        {
            InitializeComponent();
            NdcsSystem = aNdcsSystem;
            this.appropriateTreeView = treeView;
            SetInitialValues(null);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="aNdcsSystem"></param>
        /// <param name="aNDCSService"></param>
        public ConnectivityAreaNDCSMonitorSessionsUserControl(NDCSSystem aNdcsSystem, Trafodion.Manager.Framework.Navigation.NavigationTreeView treeView, NDCSService aNDCSService)
        {
            InitializeComponent();
            NdcsSystem = aNdcsSystem;
            _theNDCSService = aNDCSService;
            this.appropriateTreeView = treeView;
            SetInitialValues(aNDCSService);
        }

        private void _stopButton_Click(object sender, EventArgs e)
        {
            string theNdcsServiceName;
            string theNdcsDatasourceName;

            NDCSDataSource theNdcsDatasource;
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

                        foreach (TenTec.Windows.iGridLib.iGRow theRow in ConnectivityNDCSServiceDataGrid.SelectedRows)
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
                //Save sort order
                ArrayList sortObject = Trafodion.Manager.Framework.Utilities.GetSortObject(ConnectivityNDCSServiceDataGrid);

                ConnectivityNDCSServiceDataGrid.Rows.Clear();
                Populate();

                //re-apply sort
                Trafodion.Manager.Framework.Utilities.ApplySort(this.ConnectivityNDCSServiceDataGrid, sortObject);
            }
            finally
            {
                Cursor.Current = Cursors.Default;
                this._stopButton.Enabled = false;
            }
        }

        private void _refreshButton_Click(object sender, EventArgs e)
        {
            NdcsSystem.RefreshServices();
            RefreshDataGrid();
        }

        /// <summary>
        /// 
        /// </summary>
        internal void Populate()
        {
            DataTable dataTable = null;
            try
            {
                if (NdcsService == null)
                {
                    dataTable = new DataTable();
                    NDCSSessionHelp helper = new NDCSSessionHelp();
                    foreach (NDCSService service in NdcsSystem.NDCSServices)
                    {
                        dataTable.Merge(helper.GetSessionsForService(service));
                    }
                }
                else
                {
                    dataTable = new NDCSSessionHelp().GetSessionsForService(NdcsService);
                }
            }
            catch (System.Data.Odbc.OdbcException oe)
            {
                // Got an ODBC erorr. Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), oe.Message, Properties.Resources.ODBCError, MessageBoxButtons.OK);
                return;
            }
            catch (Exception ex)
            {
                // Got some other exception.  Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.SystemError, MessageBoxButtons.OK);
                return;
            }

            ConnectivityNDCSServiceDataGrid.BeginUpdate();

            ConnectivityNDCSServiceDataGrid.FillWithData(dataTable);
            ConnectivityNDCSServiceDataGrid.ResizeGridColumns(dataTable);

            ConnectivityNDCSServiceDataGrid.ObjectLinkColumnNumber = 0;
            ConnectivityNDCSServiceDataGrid.Dock = DockStyle.Fill;
            if (this.appropriateTreeView != null)
            {
                ConnectivityNDCSServiceDataGrid.TreeView = this.appropriateTreeView;
            }
            else
            {
                ConnectivityNDCSServiceDataGrid.TreeView = null;
            }

            for (int i = 0; i < ConnectivityNDCSServiceDataGrid.Rows.Count; i++)
            {
                ConnectivityNDCSServiceDataGrid.Rows[i].Tag = Trafodion.Manager.Properties.Resources.MySystemsText + "\\" +
                    NdcsSystem.Name + "\\" + Properties.Resources.DataSources + "\\" +
                    ConnectivityNDCSServiceDataGrid.Rows[i].Cells[Properties.Resources.DSName].Value.ToString();
            }

            string _strformat;
            string _timestamp = Utilities.CurrentFormattedDateTime;

            if (_theNDCSService == null)
            {
                _strformat = string.Format(Properties.Resources.RefreshTimestampCaption, _timestamp) + 
                    Properties.Resources.NDCSServiceProcessCount;
                ConnectivityNDCSServiceDataGrid.UpdateCountControlText(_strformat);
            }
            else
            {
                _strformat = string.Format(Properties.Resources.RefreshTimestampCaption, _timestamp) + 
                    string.Format(Properties.Resources.ServerStatusForService, TheServiceName , "{0}");
                ConnectivityNDCSServiceDataGrid.UpdateCountControlText(_strformat);
            }

            this._stopButton.Enabled = false;

            ConnectivityNDCSServiceDataGrid.EndUpdate();
            if (_NdcsSystem.UserHasInfoPrivilege)
            {
                _hyperLinkCellManager.Attach(ConnectivityNDCSServiceDataGrid, HyperlinkColIndex);
                // It's better to call this method after you attached iGHyperlinkCellManager to the grid
                // because the manager can change the formatting of the links dynamically
            }
            ConnectivityNDCSServiceDataGrid.ResizeGridColumns(dataTable, 7, 30);

        }

        /// <summary>
        /// Stores the initial settings
        /// </summary>
        private void SetInitialValues(NDCSService aNdcsService)
        {
            _cellClickHandler = new iGCellClickEventHandler(MethodsGrid_CellClick);
            ConnectivityNDCSServiceDataGrid.CellClick += _cellClickHandler;
            HyperlinkColIndex = 2;

            
            Control theDataGridSaveControl = ConnectivityNDCSServiceDataGrid.GetButtonControl();
            theDataGridSaveControl.Dock = DockStyle.Fill;
            this._bottomRightPanel.Controls.Add(theDataGridSaveControl);

            this._bottomLeftPanel.SuspendLayout();

            this._stopButton.Location = new System.Drawing.Point(3, 4);
            this._refreshButton.Location = new System.Drawing.Point(60, 4);
            this._bottomLeftPanel.Controls.Add(this._stopButton);
            this._bottomLeftPanel.Controls.Add(this._refreshButton);

            this._stopButton.Enabled = false;

            this._bottomLeftPanel.ResumeLayout(false);

            this._tablePanel.Controls.Clear();
            this._tablePanel.Controls.Add(ConnectivityNDCSServiceDataGrid);
            ConnectivityNDCSServiceDataGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            ConnectivityNDCSServiceDataGrid.SelectionChanged += new EventHandler(ConnectivityNDCSServiceDataGrid_SelectionChanged);

            // add a Count Control panel here, update the text after the grid filled with data
            ConnectivityNDCSServiceDataGrid.AddCountControlToParent("---", DockStyle.Top);

        }

        void ConnectivityNDCSServiceDataGrid_SelectionChanged(object sender, EventArgs e)
        {
            if (this.ConnectivityNDCSServiceDataGrid.SelectedRows.Count > 0)
            {
                this._stopButton.Enabled = ConnectionDefn.ComponentPrivilegeExists("HPDCS", "ADMIN_STOP");
            }
        }

        private void MethodsGrid_CellClick(object sender, iGCellClickEventArgs e)
        {
            if (_NdcsSystem.UserHasInfoPrivilege)
            {
                // Handle hyperline here
                if (e.ColIndex == HyperlinkColIndex)
                {
                    try
                    {
                        if (this.appropriateTreeView != null)
                        {
                            TreeNode clickedNode = appropriateTreeView.FindByFullPath(this.ConnectivityNDCSServiceDataGrid.Rows[e.RowIndex].Tag.ToString());
                            appropriateTreeView.Select(clickedNode);
                            appropriateTreeView.FireSelected((NavigationTreeNode)clickedNode);
                        }
                    }
                    catch (Exception ex) { }

                }
            }
        }

    }
}
