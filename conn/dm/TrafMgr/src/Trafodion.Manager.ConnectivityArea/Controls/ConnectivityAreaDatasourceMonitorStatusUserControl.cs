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
using System.Data;
using System.Linq;
using System.Windows.Forms;
using Trafodion.Manager.ConnectivityArea.Model;
using Trafodion.Manager.ConnectivityArea.Properties;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// 
    /// </summary>
    public partial class ConnectivityAreaDatasourceMonitorStatusUserControl : UserControl, ICloneToWindow
    {
        #region Fields

        NDCSSystem _NdcsSystem;
        NDCSDataSource _NdcsDataSource;
        private Trafodion.Manager.Framework.Navigation.NavigationTreeView appropriateTreeNode = null;
        private ConnectivityObjectsIGrid<NDCSSystem> ConnectivityiGrid = new ConnectivityObjectsIGrid<NDCSSystem>();
        TrafodionIGridHyperlinkCellManager _hyperLinkCellManager = new TrafodionIGridHyperlinkCellManager();

        // the default is NDCS Service column has underline link.
        int HyperlinkColIndex = -1;

        
        /// <summary>
        /// A struct that holds the column value and count for each value
        /// </summary>
        private struct NDCSDataSourceTracing
        {
            string _theServiceName;
            NDCSDataSource _theDataSource;
            bool _enableTracing;

            public NDCSDataSourceTracing(string theName, NDCSDataSource theObj, bool tracingFlag)
            {
                _theServiceName = theName;
                _theDataSource = theObj;
                _enableTracing = tracingFlag;
            }

            public string TheServiceName
            {
                get { return _theServiceName; }
            }

            public NDCSDataSource TheDataSource
            {
                get { return _theDataSource; }
            }

            public bool EanableTracing
            {
                set { _enableTracing = value; }
                get { return _enableTracing; }
            }


            public bool Exists(List<NDCSDataSourceTracing> theList)
            {
                foreach (NDCSDataSourceTracing ds_item in theList)
                {
                    if (TheServiceName.Equals(ds_item.TheServiceName) && TheDataSource.Name.Equals(ds_item.TheDataSource.Name))
                    {
                        return true;
                    }
                }

                return false;

            }
        }

        // the list keeps tracking user clicks on the Tracing checkbox.  It will be used for Apply function
        // tracking tracing flag changes
        private List<NDCSDataSourceTracing> _UpdateTracing_DataSource = new List<NDCSDataSourceTracing>();


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
        /// 
        /// </summary>
        public bool ValuesChanged
        {
            get
            {
                return (_UpdateTracing_DataSource.Count > 0);
            }
        }

        #endregion Properties

        #region ICloneToWindow Members

        /// <summary>
        /// Creates a new instance
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            return new ConnectivityAreaDatasourceMonitorStatusUserControl(NdcsSystem, appropriateTreeNode, _NdcsDataSource);
        }

        /// <summary>
        /// Get the window title of the cloned window
        /// </summary>
        public string WindowTitle
        {
            get 
            {
                return Properties.Resources.TabPageTitle_Monitoring + " | " +
                    Properties.Resources.NDCSDSStatus + " | " +
                    "Data Source Name - " + this._NdcsDataSource.Name;
                
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
        
        //Include the treeView for linking to other areas of the treeview
        /// <summary>
        /// 
        /// </summary>
        /// <param name="aNdcsSystem"></param>
        /// <param name="treeView"></param>
        /// <param name="aDataSource"></param>
        public ConnectivityAreaDatasourceMonitorStatusUserControl(NDCSSystem aNdcsSystem, Trafodion.Manager.Framework.Navigation.NavigationTreeView treeView, NDCSDataSource aDataSource)
        {
            NdcsSystem = aNdcsSystem;
            NdcsSystem.ModelChangedEvent += NdcsSystem_ModelChangedEvent;
            _NdcsDataSource = aDataSource;
            this.appropriateTreeNode = treeView;
            InitializeComponent();
            SetInitialValues();
            // populate now, since this the first page, do not delay the populate
            BuildIGrid(aNdcsSystem);
        }

        void MyDispose()
        {
            if (_NdcsSystem != null)
            {
                _NdcsSystem.ModelChangedEvent -= NdcsSystem_ModelChangedEvent;
            }
        }

        void NdcsSystem_ModelChangedEvent(object sender, NDCSObject.NDCSModelEventArgs e)
        {
            if (e.Action == NDCSObject.NDCS_MODEL_ACTION.DELETE_DATASOURCE)
            {
                _UpdateTracing_DataSource.RemoveAll(t => t.TheDataSource == e.NDCSObject);
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="aNdcsSystem"></param>
        public void BuildIGrid(NDCSSystem aNdcsSystem)
        {
            Cursor.Current = Cursors.WaitCursor;
            try
            {
                ConnectivityiGrid.BeginUpdate();

                DataTable dataTable = _NdcsDataSource.GetAllStatus();
                ConnectivityiGrid.FillWithData(dataTable);
                ConnectivityiGrid.ResizeGridColumns(dataTable);
                HyperlinkColIndex = 1;

                //Define the path to each object in the navigation tree
                for (int i = 0; i < ConnectivityiGrid.Rows.Count; i++)
                {
                    ConnectivityiGrid.Rows[i].Tag = Trafodion.Manager.Properties.Resources.MySystemsText + "\\" + 
                        NdcsSystem.Name + "\\" + Properties.Resources.TabPageLabel_Services + "\\" + 
                        ConnectivityiGrid.Rows[i].Cells[Properties.Resources.ServiceName].Value.ToString();
                }
                
                //ConnectivityiGrid.ObjectLinkColumnNumber = 0;
                ConnectivityiGrid.Dock = DockStyle.Fill;

                string timestamp = Utilities.CurrentFormattedDateTime;
                string _refreshTimeStampString;

                _refreshTimeStampString = string.Format(Properties.Resources.RefreshTimestampCaption, timestamp) +
                    string.Format(Properties.Resources.DatasourceStatus, _NdcsDataSource.Name, "{0}");

                ConnectivityiGrid.UpdateCountControlText(_refreshTimeStampString);

                ConnectivityiGrid.EndUpdate();
                _hyperLinkCellManager.Attach(ConnectivityiGrid, HyperlinkColIndex); // NDCS Service column
                // It's better to call this method after you attached iGHyperlinkCellManager to the grid
                // because the manager can change the formatting of the links dynamically
                ConnectivityiGrid.ResizeGridColumns(dataTable, 7, 30);
            }            
            catch (Exception ex)
            {
                //    Got some other exception.  Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.Error, MessageBoxButtons.OK);
            }    
            finally
            {
                Cursor.Current = Cursors.Default;
            }

        }

               

        private void _startButton_Click(object sender, EventArgs e)
        {
            NDCSDataSource theNdcsDatasource;
            Cursor.Current = Cursors.WaitCursor;
            try
            {
                foreach (TenTec.Windows.iGridLib.iGRow theRow in ConnectivityiGrid.SelectedRows)
                {
                    string serviceName = theRow.Cells[Properties.Resources.ServiceName].Value as string;
                    theNdcsDatasource = this.NdcsSystem.FindNDCSDataSource(theRow.Cells[Properties.Resources.DSName].Value.ToString());
                    theNdcsDatasource.Start(theRow.Cells[Properties.Resources.ServiceName].Value.ToString());
                    _UpdateTracing_DataSource.RemoveAll(t => t.TheServiceName.Equals(serviceName) && t.TheDataSource == theNdcsDatasource);
                }
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
            finally
            {
                Cursor.Current = Cursors.Default;
                RefreshDataGrid();
            }
        }

        private void _stopButton_Click(object sender, EventArgs e)
        {
            string theNdcsServiceName;
            string theNdcsDatasourceName;

            NDCSDataSource theNdcsDatasource;
            StopDatasourceDialog theStopDatasourceDialog;

            string theStopReason;
            NDCSObject.StopMode theStopMode;
            string theWindowTitle, theDatasourceName;
            int theConnectedCount = 0;

            try
            {
                if (ConnectivityiGrid.SelectedRows.Count == 1)
                {
                    TenTec.Windows.iGridLib.iGRow theRow = ConnectivityiGrid.SelectedRows[0];

                    theNdcsServiceName = theRow.Cells[Properties.Resources.ServiceName].Value.ToString();
                    theNdcsDatasourceName = theRow.Cells[Properties.Resources.DSName].Value.ToString();
                    theWindowTitle = "Stopping Data Source";
                    theDatasourceName = theNdcsServiceName + " " + theNdcsDatasourceName;
                    theConnectedCount = (int)theRow.Cells[Properties.Resources.CurrentServerConnected].Value;
                    theStopDatasourceDialog = new StopDatasourceDialog(NdcsSystem, theWindowTitle, theDatasourceName, theConnectedCount);

                }
                else
                {
                    foreach (TenTec.Windows.iGridLib.iGRow theRow in ConnectivityiGrid.SelectedRows)
                    {
                        theConnectedCount += (int)theRow.Cells[Properties.Resources.CurrentServerConnected].Value;
                    }

                    theWindowTitle = "Stopping Data Sources";
                    theStopDatasourceDialog = new StopDatasourceDialog(NdcsSystem, theWindowTitle, theConnectedCount);
                }

                if (theStopDatasourceDialog.ShowDialog(this) == DialogResult.OK)
                {
                    bool isStopConnectedDS = false;
                    string dsName=string.Empty;
                    foreach (TenTec.Windows.iGridLib.iGRow theRow in ConnectivityiGrid.SelectedRows)
                    {
                        dsName=theRow.Cells[Properties.Resources.DSName].Value.ToString();
                        if (dsName.Equals(ConnectionDefn.ConnectedDataSource))
                        {
                            isStopConnectedDS = true;
                            break;
                        }
                    }

                    if (isStopConnectedDS)
                    {
                        MessageBox.Show(string.Format(Properties.Resources.StopConnectedDSError,dsName),
                            Trafodion.Manager.Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
                        return;
                    }

                    theStopReason = theStopDatasourceDialog.TheReason;
                    if (theStopDatasourceDialog.StopMode == 0)
                    {
                        theStopMode = NDCSObject.StopMode.STOP_IMMEDIATE;
                    }
                    else
                    {
                        theStopMode = NDCSObject.StopMode.STOP_DISCONNECT;
                    }

                    Cursor.Current = Cursors.WaitCursor;
                    try
                    {
                        foreach (TenTec.Windows.iGridLib.iGRow theRow in ConnectivityiGrid.SelectedRows)
                        {
                            theNdcsServiceName = theRow.Cells[Properties.Resources.ServiceName].Value.ToString();
                            theNdcsDatasource = this.NdcsSystem.FindNDCSDataSource(theRow.Cells[Properties.Resources.DSName].Value.ToString());

                            theNdcsDatasource.Stop(theNdcsServiceName, theStopMode, theStopReason);
                            _UpdateTracing_DataSource.RemoveAll(t => t.TheServiceName.Equals(theNdcsServiceName) && t.TheDataSource == theNdcsDatasource);

                        }
                    }
                    finally
                    {
                        Cursor.Current = Cursors.Default;
                        RefreshDataGrid();
                    }
                }

                theStopDatasourceDialog.Dispose();
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
                this.BuildIGrid(NdcsSystem);

                //re-apply sort
                Trafodion.Manager.Framework.Utilities.ApplySort(this.ConnectivityiGrid, sortObject);
            }
            finally
            {
                // disable these buttons by default
                this._startButton.Enabled = false;
                this._stopButton.Enabled = false;
                this._applyButton.Enabled = false;
                Cursor.Current = Cursors.Default;

            }

        }

        private void _refreshButton_Click(object sender, EventArgs e)
        {
            _UpdateTracing_DataSource.Clear();
            RefreshDataGrid();
        }


        private void _applyButton_Click(object sender, EventArgs e)
        {
            ApplyTracingChanges();
        }


        public void ApplyTracingChanges()
        {
            Cursor.Current = Cursors.WaitCursor;
            try
            {
                foreach (NDCSDataSourceTracing ds_item in _UpdateTracing_DataSource)
                {
                    ds_item.TheDataSource.Trace(ds_item.TheServiceName, ds_item.EanableTracing);
                }
            }
            catch (Exception ex) 
            {
                // Got some other exception.  Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.SystemError, MessageBoxButtons.OK);
            }
            finally
            {
                Cursor.Current = Cursors.Default;
                RefreshDataGrid();
                _UpdateTracing_DataSource.Clear();
                this._applyButton.Enabled = (_UpdateTracing_DataSource.Count != 0) && ConnectionDefn.ComponentPrivilegeExists("HPDCS", "ADMIN_ALTER");
            }

        }



        /// <summary>
        /// When user move away from the current selected node, a prompt will let user to choose to ignore any tracing changes
        /// </summary>
        /// <returns></returns>
        public void CancelTracingChanges()
        {
            RefreshDataGrid();
            _UpdateTracing_DataSource.Clear();
            this._applyButton.Enabled = (_UpdateTracing_DataSource.Count != 0) && ConnectionDefn.ComponentPrivilegeExists("HPDCS", "ADMIN_ALTER");
        }

        
        
        //This is for creating a link to other areas of TrafodionManager.
        void ConnectivityiGrid_CellClick(object sender, TenTec.Windows.iGridLib.iGCellClickEventArgs e)
        {
            NDCSDataSourceTracing aDataSourceTracingItem;
            string theNdcsServiceName;

            NDCSDataSource theDataSource;

            // Handle hyperlink click here
            if (e.ColIndex == HyperlinkColIndex)
            {
                try
                {
                    if (this.appropriateTreeNode != null)
                    {
                        TreeNode clickedNode = appropriateTreeNode.FindByFullPath(this.ConnectivityiGrid.Rows[e.RowIndex].Tag.ToString());
                        appropriateTreeNode.Select(clickedNode);
                        appropriateTreeNode.FireSelected((NavigationTreeNode)clickedNode);
                    }
                }
                catch (Exception ex) 
                { 
                    // do nothing 
                }

                return;

            }


            if (ConnectionDefn.ComponentPrivilegeExists("HPDCS", "ADMIN_ALTER") && e.ColIndex == this.ConnectivityiGrid.Cols[Properties.Resources.DSTracing].Index)
            {
                try
                {
                    theDataSource = (NDCSDataSource)this.ConnectivityiGrid.Cols[Properties.Resources.DSName].Cells[e.RowIndex].Value;
                    theNdcsServiceName = this.ConnectivityiGrid.Cols[Properties.Resources.ServiceName].Cells[e.RowIndex].Value.ToString();

                    // toggle the check box
                    this.ConnectivityiGrid.Cols[Properties.Resources.DSTracing].Cells[e.RowIndex].Value = 
                        !((bool)this.ConnectivityiGrid.Cols[Properties.Resources.DSTracing].Cells[e.RowIndex].Value);

                    aDataSourceTracingItem = new NDCSDataSourceTracing(theNdcsServiceName, theDataSource,
                        (bool)this.ConnectivityiGrid.Cols[Properties.Resources.DSTracing].Cells[e.RowIndex].Value);

                    if (!aDataSourceTracingItem.Exists(_UpdateTracing_DataSource))
                    {
                        // add this one to the list
                        _UpdateTracing_DataSource.Add(aDataSourceTracingItem);
                    }
                    else
                    {
                        // check and remove if theDataSource is already added into the list
                        // toggle back to the same when its added, so the List function can "remove" it.
                        aDataSourceTracingItem.EanableTracing = !aDataSourceTracingItem.EanableTracing;
                        _UpdateTracing_DataSource.Remove(aDataSourceTracingItem);
                    }


                    // is there any datasouce chaged its tracing flag?
                    if (_UpdateTracing_DataSource.Count > 0)
                    {
                        this._applyButton.Enabled = true;
                    }
                    else
                    {
                        this._applyButton.Enabled = false;
                    }
                }
                catch(Exception ex) { }
            }

        }

        void ConnectivityiGrid_SelectionChanged(object sender, EventArgs e)
        {
            bool startEnabled = false;
            bool stopEnabled = false;

            if (this.ConnectivityiGrid.SelectedRows.Count > 0)
            {
                foreach (TenTec.Windows.iGridLib.iGRow theRow in ConnectivityiGrid.SelectedRows)
                {
                    if (theRow.Cells[Properties.Resources.DSState].Value.ToString() == "Started")
                    { stopEnabled = true; }
                    else
                    { startEnabled = true; }

                    if (stopEnabled && startEnabled)
                        break;
                }

                this._startButton.Enabled = ConnectionDefn.ComponentPrivilegeExists("HPDCS", "ADMIN_START") && startEnabled;
                this._stopButton.Enabled = ConnectionDefn.ComponentPrivilegeExists("HPDCS", "ADMIN_STOP") && stopEnabled;
            }

        }

        /// <summary>
        /// Stores the initial settings
        /// </summary>
        private void SetInitialValues()
        {
            if (ConnectivityiGrid != null)
            {
                ConnectivityiGrid.CellClick += new TenTec.Windows.iGridLib.iGCellClickEventHandler(ConnectivityiGrid_CellClick);
                ConnectivityiGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
                ConnectivityiGrid.SelectionChanged += new EventHandler(ConnectivityiGrid_SelectionChanged);

                TrafodionPanel1.Controls.Add(ConnectivityiGrid);

                Control theDataGridSaveControl = ConnectivityiGrid.GetButtonControl();
                theDataGridSaveControl.Dock = DockStyle.Fill;
                _bottomRightPanel.Controls.Add(theDataGridSaveControl);
            }

            //this._bottomLeftPanel.ResumeLayout();

            ToolTip forRightPaneButton = new ToolTip();
            this._startButton.Location = new System.Drawing.Point(3, 4);
            this._stopButton.Location = new System.Drawing.Point(60, 4);
            this._refreshButton.Location = new System.Drawing.Point(117, 4);
            this._applyButton.Location = new System.Drawing.Point(182, 4);
            this._applyButton.Enabled = false;
            this._bottomLeftPanel.Controls.Add(this._startButton);
            this._bottomLeftPanel.Controls.Add(this._stopButton);
            this._bottomLeftPanel.Controls.Add(this._refreshButton);
            this._bottomLeftPanel.Controls.Add(this._applyButton);

            // disable these buttons by default
            this._startButton.Enabled = false;
            this._stopButton.Enabled = false;
            this._applyButton.Enabled = false;

            forRightPaneButton.SetToolTip(this._applyButton, Resources.Tooltip_Btn_Datasource_Apply);

            forRightPaneButton.SetToolTip(this._refreshButton, Resources.Tooltip_Btn_Datasource_Refresh);
            forRightPaneButton.AutomaticDelay = 1500;

            //this._bottomLeftPanel.ResumeLayout(false);

            // Add the CountControl To the Grid, update it after the grid is filled
            ConnectivityiGrid.AddCountControlToParent("---", DockStyle.Top);

        }


    }
}
