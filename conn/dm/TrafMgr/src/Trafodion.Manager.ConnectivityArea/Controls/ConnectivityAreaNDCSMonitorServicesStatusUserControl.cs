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
using System.Collections.Generic;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.ConnectivityArea.Model;
using System.Data;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// 
    /// </summary>
    public partial class ConnectivityAreaNDCSMonitorServicesStatusUserControl : UserControl, ICloneToWindow
    {
        #region Fields

        private NDCSService _theNDCSService = null;
        private NDCSSystem _NdcsSystem = null;
        private Trafodion.Manager.Framework.Navigation.NavigationTreeView appropriateTreeView = null;
        private ConnectivityObjectsIGrid<NDCSService> ConnectivityNDCSServiceDataGrid = new ConnectivityObjectsIGrid<NDCSService>();
        TrafodionIGridHyperlinkCellManager _hyperLinkCellManager = new TrafodionIGridHyperlinkCellManager();
        iGCellClickEventHandler _cellClickHandler = null;

        // default is no column has underline link.
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
        /// Gets/Sets the underlying NDCS Service object
        /// </summary>
        public NDCSService NdcsService
        {
            get { return _theNDCSService; }
            set { _theNDCSService = value; }
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
            // What is the current selected node in the tree?  NDCS Services folder or a NDCS Services
            Control theUserControl = null;

            if (this.NdcsService != null)
            {
                theUserControl = new ConnectivityAreaNDCSMonitorServicesStatusUserControl(NdcsSystem, appropriateTreeView, _theNDCSService);
            }
            else
            {
                theUserControl = new ConnectivityAreaNDCSMonitorServicesStatusUserControl(NdcsSystem, appropriateTreeView);
            }

            ((ConnectivityAreaNDCSMonitorServicesStatusUserControl)theUserControl).Populate();

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
                            Properties.Resources.NDCSServicesStatus + " | " +
                            this.TheServiceName;
                }
                else
                {
                    return Properties.Resources.TabPageTitle_Monitoring + " | " +
                            Properties.Resources.NDCSServicesStatus + " | ALL";
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
        public ConnectivityAreaNDCSMonitorServicesStatusUserControl(NDCSSystem aNdcsSystem, Trafodion.Manager.Framework.Navigation.NavigationTreeView treeView)
        {
            InitializeComponent();
            NdcsSystem = aNdcsSystem;
            this.appropriateTreeView = treeView;
            SetInitialValues(null);
            HyperlinkColIndex = 0;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="aNdcsSystem"></param>
        /// <param name="aNDCSService"></param>
        public ConnectivityAreaNDCSMonitorServicesStatusUserControl(NDCSSystem aNdcsSystem,  Trafodion.Manager.Framework.Navigation.NavigationTreeView treeView, NDCSService aNDCSService)
        {
            InitializeComponent();
            NdcsSystem = aNdcsSystem;
            NdcsService = aNDCSService;
            this.appropriateTreeView = treeView;
            SetInitialValues(aNDCSService);
            HyperlinkColIndex = -1;
        }

        /// <summary>
        /// To avoid showing the Refresh button on the system node level
        /// </summary>
        /// <param name="bHide"></param>
        public void HideRefreshButton(bool bHide)
        {
            this._bottomLeftPanel.SuspendLayout();
            if (bHide)
            {
                this._refreshButton.Hide();
                this._bottomLeftPanel.Controls.Remove(this._refreshButton);
            }
            else
            {
                this._bottomLeftPanel.Controls.Add(this._refreshButton);
                this._refreshButton.Show();
            }
            this._bottomLeftPanel.ResumeLayout(false);
        }


        private void _refreshButton_Click(object sender, EventArgs e)
        {
            RefreshDataGrid();
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

                // Fix issue 5114. After refeshing the grid's data, its filter should also be re-applied.
                ConnectivityNDCSServiceDataGrid.ApplyFilter();

                //re-apply sort
                Trafodion.Manager.Framework.Utilities.ApplySort(this.ConnectivityNDCSServiceDataGrid, sortObject);

            }
            finally
            {
                Cursor.Current = Cursors.Default;
            }


        }


        /// <summary>
        /// Call to this control to refresh the contain ...
        /// </summary>
        public override void Refresh()
        {
            if (this.Parent != null)
            {
                this.Parent.Refresh();
            }

            Populate();
        }

        
        internal void Populate()
        {
            DataTable dataTable = null;
            Cursor.Current = Cursors.WaitCursor;
            try
            {
                if (NdcsService != null)
                {
                    dataTable = NdcsService.GetStatus();
                }
                else
                {
                    dataTable = NdcsSystem.GetAllServiceStatus();
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
            finally
            {
                Cursor.Current = Cursors.Default;
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
                    NdcsSystem.Name + "\\" + Properties.Resources.TabPageLabel_Services + "\\" +
                    ConnectivityNDCSServiceDataGrid.Rows[i].Cells[Properties.Resources.ServiceName].Value.ToString();
            }

            string _strformat;
            string _timestamp = Utilities.CurrentFormattedDateTime;

            if (_theNDCSService == null)
            {
                _strformat = string.Format(Properties.Resources.NDCSServiceCountInSystem, ConnectivityNDCSServiceDataGrid.Rows.Count / 2);
                ConnectivityNDCSServiceDataGrid.UpdateCountControlText(_strformat);
            }
            else
            {
                _strformat = string.Format(Properties.Resources.NDCSServiceStatus, TheServiceName);
                ConnectivityNDCSServiceDataGrid.UpdateCountControlText(_strformat);
            }

            ConnectivityNDCSServiceDataGrid.EndUpdate();
            _hyperLinkCellManager.Attach(ConnectivityNDCSServiceDataGrid, HyperlinkColIndex); 
            // It's better to call this method after you attached iGHyperlinkCellManager to the grid
            // because the manager can change the formatting of the links dynamically
            ConnectivityNDCSServiceDataGrid.ResizeGridColumns(dataTable, 7, 30);

        }




        /// <summary>
        /// Stores the initial settings
        /// </summary>
        private void SetInitialValues(NDCSService aNDCSService)
        {
            _cellClickHandler = new iGCellClickEventHandler(MethodsGrid_CellClick);
            ConnectivityNDCSServiceDataGrid.CellClick += _cellClickHandler;

            this._tablePanel.Controls.Clear();
            this._tablePanel.Controls.Add(ConnectivityNDCSServiceDataGrid);

            // make the initial top status panel here
            this.ConnectivityNDCSServiceDataGrid.AddCountControlToParent("   ", DockStyle.Top);

            Control theDataGridSaveControl = this.ConnectivityNDCSServiceDataGrid.GetButtonControl();
            theDataGridSaveControl.Dock = DockStyle.Fill;
            this._bottomRightPanel.Controls.Add(theDataGridSaveControl);

            this._bottomLeftPanel.SuspendLayout();
            this._refreshButton.Location = new System.Drawing.Point(3, 4);
            this._bottomLeftPanel.Controls.Add(this._refreshButton);
            this._bottomLeftPanel.ResumeLayout(false);
        }

        private void MethodsGrid_CellClick(object sender, iGCellClickEventArgs e)
        {
            iGCell CurrentCell = ConnectivityNDCSServiceDataGrid.Cells[e.RowIndex, e.ColIndex];

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
