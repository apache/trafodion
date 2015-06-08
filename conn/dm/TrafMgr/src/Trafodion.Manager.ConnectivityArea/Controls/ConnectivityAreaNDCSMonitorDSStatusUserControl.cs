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
using System.Windows.Forms;
using Trafodion.Manager.ConnectivityArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// 
    /// </summary> 
    public partial class ConnectivityAreaNDCSMonitorDSStatusUserControl : UserControl, ICloneToWindow
    {
        #region Fields

        NDCSService _theNDCSService = null;
        NDCSSystem _theNDCSSystem = null;
        private Trafodion.Manager.Framework.Navigation.NavigationTreeView appropriateTreeView = null;
        private ConnectivityObjectsIGrid<NDCSService> ConnectivityNDCSServiceDataGrid = new ConnectivityObjectsIGrid<NDCSService>();
        TrafodionIGridHyperlinkCellManager _hyperLinkCellManager = new TrafodionIGridHyperlinkCellManager();

        // default is no column has underline link.
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
        private List <NDCSDataSourceTracing> _UpdateTracing_DataSource = new List<NDCSDataSourceTracing>();


        #endregion Fields

        #region Properties

        /// <summary>
        /// Gets the underlying system
        /// </summary>
        public NDCSSystem NdcsSystem
        {
            get { return _theNDCSSystem; }
        }

        /// <summary>
        /// 
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
            // What is the current selected node in the tree?  NDCS Services folder or a NDCS Services

            Control theUserControl = null;

            if (this.NdcsService != null)
            {
                theUserControl = new ConnectivityAreaNDCSMonitorDSStatusUserControl(NdcsSystem, appropriateTreeView, NdcsService);

            }
            else
            {
                theUserControl = new ConnectivityAreaNDCSMonitorDSStatusUserControl(NdcsSystem, appropriateTreeView);
            }
            
            ((ConnectivityAreaNDCSMonitorDSStatusUserControl)theUserControl).Populate();
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
                            Properties.Resources.NDCSDSStatus + " | " +
                            this.TheServiceName;
                }
                else
                {
                    return Properties.Resources.TabPageTitle_Monitoring + " | " +
                            Properties.Resources.NDCSDSStatus + " | ALL";
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
        public ConnectivityAreaNDCSMonitorDSStatusUserControl(NDCSSystem aNdcsSystem, Trafodion.Manager.Framework.Navigation.NavigationTreeView treeView)
        {
            InitializeComponent();
            _theNDCSSystem = aNdcsSystem;
            _theNDCSSystem.ModelChangedEvent += _theNDCSSystem_ModelChangedEvent;
            this.appropriateTreeView = treeView;
            SetInitialValues(null);
        }

        void _theNDCSSystem_ModelChangedEvent(object sender, NDCSObject.NDCSModelEventArgs e)
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
        /// <param name="aNDCSService"></param>
        public ConnectivityAreaNDCSMonitorDSStatusUserControl(NDCSSystem aNdcsSystem, Trafodion.Manager.Framework.Navigation.NavigationTreeView treeView, NDCSService aNDCSService)
        {
            InitializeComponent();

            _theNDCSService = aNDCSService;
            _theNDCSSystem = aNdcsSystem;
            _theNDCSSystem.ModelChangedEvent += _theNDCSSystem_ModelChangedEvent;
            this.appropriateTreeView = treeView;
            SetInitialValues(aNDCSService);

        }

        private void MyDispose()
        {
            if (_theNDCSSystem != null)
            {
                _theNDCSSystem.ModelChangedEvent -= _theNDCSSystem_ModelChangedEvent;
            }
        }

        private void _startButton_Click(object sender, EventArgs e)
        {
            NDCSDataSource theNdcsDatasource;
            Cursor.Current = Cursors.WaitCursor;
            try
            {
                foreach (TenTec.Windows.iGridLib.iGRow theRow in ConnectivityNDCSServiceDataGrid.SelectedRows)
                {
                    theNdcsDatasource = this.NdcsSystem.FindNDCSDataSource(theRow.Cells[Properties.Resources.DSName].Value.ToString());
                    theNdcsDatasource.Start(theRow.Cells[Properties.Resources.ServiceName].Value.ToString());
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
            string theWindowTitle;
            int theConnectedCount = 0;

            try
            {
                if (ConnectivityNDCSServiceDataGrid.SelectedRows.Count == 1)
                {
                    TenTec.Windows.iGridLib.iGRow theRow = ConnectivityNDCSServiceDataGrid.SelectedRows[0];

                    theNdcsServiceName = theRow.Cells[Properties.Resources.ServiceName].Value.ToString();
                    theNdcsDatasourceName = theRow.Cells[Properties.Resources.DSName].Value.ToString();
                    theWindowTitle = "Stopping Data Source - " + theNdcsServiceName + " " + theNdcsDatasourceName;
                    theConnectedCount = (int)theRow.Cells[Properties.Resources.CurrentServerConnected].Value;

                }
                else
                {
                    foreach (TenTec.Windows.iGridLib.iGRow theRow in ConnectivityNDCSServiceDataGrid.SelectedRows)
                    {
                        theConnectedCount += (int)theRow.Cells[Properties.Resources.CurrentServerConnected].Value;
                    }

                    theWindowTitle = "Stopping Data Sources";
                }

                theStopDatasourceDialog = new StopDatasourceDialog(NdcsSystem, theWindowTitle, theConnectedCount);
                if (theStopDatasourceDialog.ShowDialog(this) == DialogResult.OK)
                {
                    bool isStopConnectedDS = false;
                    string dsName = string.Empty;
                    foreach (TenTec.Windows.iGridLib.iGRow theRow in ConnectivityNDCSServiceDataGrid.SelectedRows)
                    {
                        dsName = theRow.Cells[Properties.Resources.DSName].Value.ToString();
                        if (dsName.Equals(
                            ConnectionDefn.ConnectedDataSource))
                        {
                            isStopConnectedDS = true;
                            break;
                        }
                    }

                    if (isStopConnectedDS)
                    {
                        MessageBox.Show(string.Format(Properties.Resources.StopConnectedDSError, dsName),
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
                        foreach (TenTec.Windows.iGridLib.iGRow theRow in ConnectivityNDCSServiceDataGrid.SelectedRows)
                        {
                            theNdcsServiceName = theRow.Cells[Properties.Resources.ServiceName].Value.ToString();
                            theNdcsDatasource = this.NdcsSystem.FindNDCSDataSource(theRow.Cells[Properties.Resources.DSName].Value.ToString());
                            theNdcsDatasource.Stop(theNdcsServiceName, theStopMode, theStopReason);
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

        private void _refreshButton_Click(object sender, EventArgs e)
        {
            RefreshDataGrid();
        }

        private void RefreshDataGrid()
        {
            Cursor.Current = Cursors.WaitCursor;
            try
            {
                //ConnectivityNDCSServiceDataGrid.StaySorted = true;

                //Save sort order
                ArrayList sortObject = Trafodion.Manager.Framework.Utilities.GetSortObject(ConnectivityNDCSServiceDataGrid);

                ConnectivityNDCSServiceDataGrid.Rows.Clear();
                Populate();
                this._startButton.Enabled = false;
                this._stopButton.Enabled = false;
                _UpdateTracing_DataSource.Clear();
                this._applyButton.Enabled = !(_UpdateTracing_DataSource.Count == 0);


                //re-apply sort
                Trafodion.Manager.Framework.Utilities.ApplySort(this.ConnectivityNDCSServiceDataGrid, sortObject);

            }
            finally
            {
                Cursor.Current = Cursors.Default;
            }
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
                ArrayList failedTracingUpdate = new ArrayList();
                ArrayList caughtExceptions = new ArrayList();
                foreach (NDCSDataSourceTracing ds_item in _UpdateTracing_DataSource)
                {
                    try
                    {
                        ds_item.TheDataSource.Trace(ds_item.TheServiceName, ds_item.EanableTracing);
                    }
                    catch (Exception traceException)
                    {
                        failedTracingUpdate.Add(ds_item);
                        caughtExceptions.Add(traceException);
                    }
                }

                if (failedTracingUpdate.Count > 0)
                {
                    string errorMessage = "\r\n";
                    foreach (NDCSDataSourceTracing dsTrace in failedTracingUpdate)
                    {
                        errorMessage += dsTrace.TheDataSource + " on " + dsTrace.TheServiceName  + "\r\n";
                    }
                                       
                    string exceptionMessage = "";
                    foreach (Exception exceptionToDisplay in caughtExceptions)
                    {
                        exceptionMessage += exceptionToDisplay.Message + "\r\n\r\n";
                    }

                    TrafodionMultipleErrorDialog errorDialog = new TrafodionMultipleErrorDialog("Trafodion Database Manager - Tracing Error", "Failed to update tracing for the following data sources:", errorMessage, exceptionMessage);
                    errorDialog.Show();
                    //MessageBox.Show(Utilities.GetForegroundControl(), errorMessage, Properties.Resources.SystemError, MessageBoxButtons.OK);
                }
            }
            catch (Exception ex) 
            {
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.SystemError, MessageBoxButtons.OK);
            }
            finally
            {
                Cursor.Current = Cursors.Default;
                RefreshDataGrid();
                _UpdateTracing_DataSource.Clear();
                this._applyButton.Enabled = (_UpdateTracing_DataSource.Count != 0);
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
            this._applyButton.Enabled = (_UpdateTracing_DataSource.Count != 0);
        }
        
        
        internal void Populate()
        {
            DataTable dataTable = null;

            try
            {
                if (NdcsService != null)
                {
                    dataTable = NdcsService.GetDataSourceStatus();
                }
                else
                {
                    dataTable = _theNDCSSystem.GetAllDataSourceStatus();
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
            //Define the path to each object in the navigation tree
            for (int i = 0; i < ConnectivityNDCSServiceDataGrid.Rows.Count; i++)
            {
                ConnectivityNDCSServiceDataGrid.Rows[i].Tag = Trafodion.Manager.Properties.Resources.MySystemsText + "\\" +
                    NdcsSystem.Name + "\\" + Properties.Resources.DataSources + "\\" +
                    ConnectivityNDCSServiceDataGrid.Rows[i].Cells[Properties.Resources.DSName].Value.ToString();
            }

            ConnectivityNDCSServiceDataGrid.ObjectLinkColumnNumber = 0;
            ConnectivityNDCSServiceDataGrid.Dock = DockStyle.Fill;
            ConnectivityNDCSServiceDataGrid.TreeView = null;

            string _timestamp = Utilities.CurrentFormattedDateTime;
            string _strformat;

            if (_theNDCSService == null)
            {
                _strformat = string.Format(Properties.Resources.RefreshTimestampCaption, _timestamp) +
                    string.Format(Properties.Resources.DatasourceStatusForSystem, _theNDCSSystem.NDCSServices.Count, _theNDCSSystem.NDCSDataSources.Count);

                ConnectivityNDCSServiceDataGrid.UpdateCountControlText(_strformat);
            }
            else
            {
                _strformat = string.Format(Properties.Resources.RefreshTimestampCaption, _timestamp) +
                    string.Format(Properties.Resources.DatasourceStatusForService, TheServiceName, "{0}");

                ConnectivityNDCSServiceDataGrid.UpdateCountControlText(_strformat);
            }

            ConnectivityNDCSServiceDataGrid.EndUpdate();
            if (_theNDCSSystem.UserHasInfoPrivilege)
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
        private void SetInitialValues(NDCSService aNDCSService)
        {
            // handle hyper link for iGrid cells
            HyperlinkColIndex = 1;  // for both NDCS Services folder and single NDCS Services

            this._tablePanel.Controls.Clear();
            this._tablePanel.Controls.Add(ConnectivityNDCSServiceDataGrid);
            ConnectivityNDCSServiceDataGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            ConnectivityNDCSServiceDataGrid.SelectionChanged += new EventHandler(ConnectivityNDCSServiceDataGrid_SelectionChanged);
            ConnectivityNDCSServiceDataGrid.CellClick += new TenTec.Windows.iGridLib.iGCellClickEventHandler(ConnectivityNDCSServiceDataGrid_CellClick);

            // make the initial top status panel here
            this.ConnectivityNDCSServiceDataGrid.AddCountControlToParent("   " , DockStyle.Top);


            Control theDataGridSaveControl = this.ConnectivityNDCSServiceDataGrid.GetButtonControl();
            theDataGridSaveControl.Dock = DockStyle.Fill;
            this._bottomRightPanel.Controls.Add(theDataGridSaveControl);

            this._bottomLeftPanel.SuspendLayout();

            this._startButton.Location = new System.Drawing.Point(3, 4);
            this._stopButton.Location = new System.Drawing.Point(60, 4);
            this._refreshButton.Location = new System.Drawing.Point(117, 4);
            this._applyButton.Location = new System.Drawing.Point(182, 4);
            this._bottomLeftPanel.Controls.Add(this._startButton);
            this._bottomLeftPanel.Controls.Add(this._stopButton);
            this._bottomLeftPanel.Controls.Add(this._refreshButton);
            this._bottomLeftPanel.Controls.Add(this._applyButton);
                
            // disable these buttons by default
            this._startButton.Enabled = false;
            this._stopButton.Enabled = false;
            this._applyButton.Enabled = false;

            this._bottomLeftPanel.ResumeLayout(false);

        }


        //This is for creating a link to other areas of TrafodionManager.
        void ConnectivityNDCSServiceDataGrid_CellClick(object sender, TenTec.Windows.iGridLib.iGCellClickEventArgs e)
        {
            if (_theNDCSSystem.UserHasInfoPrivilege)
            {
                NDCSDataSourceTracing aDataSourceTracingItem;
                string theNdcsServiceName;
                string theNdcsDataSourceName;

                NDCSDataSource theDataSource;

                // Handle hyperlink click here
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

                    // user is clicked on a hyper link.  
                    return;
                }


                if (ConnectionDefn.ComponentPrivilegeExists("HPDCS", "ADMIN_ALTER") && e.ColIndex == this.ConnectivityNDCSServiceDataGrid.Cols[Properties.Resources.DSTracing].Index)
                {
                    try
                    {
                        theDataSource = (NDCSDataSource)this.ConnectivityNDCSServiceDataGrid.Cols[Properties.Resources.DSName].Cells[e.RowIndex].Value;
                        theNdcsServiceName = this.ConnectivityNDCSServiceDataGrid.Cols[Properties.Resources.ServiceName].Cells[e.RowIndex].Value.ToString();

                        //GetCheckboxValue
                        bool currentCheckState = (bool)this.ConnectivityNDCSServiceDataGrid.Cols[Properties.Resources.DSTracing].Cells[e.RowIndex].Value;

                        // toggle the check box
                        this.ConnectivityNDCSServiceDataGrid.Cols[Properties.Resources.DSTracing].Cells[e.RowIndex].Value = !currentCheckState;

                        aDataSourceTracingItem = new NDCSDataSourceTracing(theNdcsServiceName, theDataSource, !currentCheckState);

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
                    catch (Exception ex) { }
                }
            }
        }


        void ConnectivityNDCSServiceDataGrid_SelectionChanged(object sender, EventArgs e)
        {
            bool startEnabled = false;
            bool stopEnabled = false;

            if (this.ConnectivityNDCSServiceDataGrid.SelectedRows.Count > 0)
            {
                foreach (TenTec.Windows.iGridLib.iGRow theRow in ConnectivityNDCSServiceDataGrid.SelectedRows)
                {

                    if (theRow.Cells[Properties.Resources.DSState].Value.ToString() == "Started")
                    { stopEnabled = true; }
                    else
                    {startEnabled = true;}

                    if (stopEnabled && startEnabled)
                        break;
                }

                this._startButton.Enabled = startEnabled;
                this._stopButton.Enabled = stopEnabled;
                this._startButton.Enabled = startEnabled && ConnectionDefn.ComponentPrivilegeExists("HPDCS", "ADMIN_START");
                this._stopButton.Enabled = stopEnabled && ConnectionDefn.ComponentPrivilegeExists("HPDCS", "ADMIN_STOP");
            }

        }


    }
}
