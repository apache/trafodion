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
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.ConnectivityArea.Model;
using System.Data;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// 
    /// </summary>
    public partial class ConnectivityAreaDatasourceConfigGeneralUserControl : UserControl, ICloneToWindow
    {
        #region Fields
        NDCSDataSource _NdcsDatasource;
        private bool valuesChanged = false;
        private ConnectivityObjectsIGrid<Trafodion.Manager.ConnectivityArea.Model.NDCSObject> ConnectivityiGrid;
        private bool _isCreate = false;
        private ConnectivityAreaDatasourceConfigurationUserControl _linkToParent;
        public ConnectivityAreaDatasourceConfigurationUserControl LinkToParent
        {
            set
            {
                _linkToParent = value;
            }
        }

        #endregion Fields

        #region Properties
        
        /// <summary>
        /// Gets the underlying system
        /// </summary>
        public NDCSDataSource NdcsDatasource
        {
            get { return _NdcsDatasource; }
            set { _NdcsDatasource = value; }
        }

        public bool ValuesChanged
        {
            get { return valuesChanged; }
            set { valuesChanged = value; }
        }

        #endregion Properties
        public ConnectivityAreaDatasourceConfigGeneralUserControl(NDCSDataSource aNdcsDatasource, bool aIsCreate)
        {
            _isCreate = aIsCreate;
            this._NdcsDatasource = aNdcsDatasource;

            InitializeComponent();

            // Resource Mgmt is not supported for SQ
            this.splitContainer1.Panel2Collapsed = true;
            this.TrafodionGroupBox10.Visible = false;
            this.TrafodionGroupBox11.Visible = false;

            SetupIGrid();
            SetupControls();
            PopulateControls();
            addEventHandlers();
        }

        /// <summary>
        /// Stores the initial settings
        /// </summary>
        private void SetupControls()
        {
            valuesChanged = false;
            //this.dsName_TrafodionTextBox.Visible = this._isCreate;
            //this.dsName_TrafodionLabel.Visible = (!this._isCreate);

            //Establish Maximum values for spin-controls
            this.serverMax_TrafodionNumericUpDown.Maximum = System.Int16.MaxValue;
            this.serverInitial_TrafodionNumericUpDown.Maximum = System.Int16.MaxValue;
            this.serverAvail_TrafodionNumericUpDown.Maximum = System.Int16.MaxValue;
            this.processPriorityInitial_TrafodionNumericUpDown.Maximum = NDCSDataSource.MAX_PROCESS_PRIORITY;
            this.serverIdleTimeoutCustom_TrafodionNumericUpDown.Maximum = NDCSDataSource.MAX_TIMEOUT_VALUE;
            this.connectionIdleCustom_TrafodionNumericUpDown.Maximum = NDCSDataSource.MAX_TIMEOUT_VALUE;

            //Establish Minimuim values for spin-controls
            this.serverMax_TrafodionNumericUpDown.Minimum = NDCSDataSource.MIN_SERVER_COUNT;
            this.serverInitial_TrafodionNumericUpDown.Minimum = NDCSDataSource.MIN_SERVER_COUNT;
            this.serverAvail_TrafodionNumericUpDown.Minimum = NDCSDataSource.MIN_SERVER_COUNT;
            this.processPriorityInitial_TrafodionNumericUpDown.Minimum = NDCSDataSource.MIN_PROCESS_PRIORITY;
            this.serverIdleTimeoutCustom_TrafodionNumericUpDown.Minimum = NDCSDataSource.MIN_TIMEOUT_VALUE;
            this.connectionIdleCustom_TrafodionNumericUpDown.Minimum = NDCSDataSource.MIN_TIMEOUT_VALUE;

            //Add Tooltips to NumericUpDown controls

            string numericUpDown_Tooltip = String.Format(Properties.Resources.NumericUpDown_Tooltip, 
                NDCSDataSource.MIN_SERVER_COUNT,NDCSDataSource.MAX_SERVER_COUNT);
            this.configInfo_TrafodionToolTip.SetToolTip(serverMax_TrafodionNumericUpDown,numericUpDown_Tooltip);
            this.configInfo_TrafodionToolTip.SetToolTip(serverInitial_TrafodionNumericUpDown,numericUpDown_Tooltip);
            this.configInfo_TrafodionToolTip.SetToolTip(serverAvail_TrafodionNumericUpDown,numericUpDown_Tooltip);
            this.configInfo_TrafodionToolTip.SetToolTip(processPriorityInitial_TrafodionNumericUpDown,
                 String.Format(Properties.Resources.NumericUpDown_Tooltip, 
                NDCSDataSource.MIN_PROCESS_PRIORITY,NDCSDataSource.MAX_PROCESS_PRIORITY));
            this.configInfo_TrafodionToolTip.SetToolTip(serverIdleTimeoutCustom_TrafodionNumericUpDown, 
                String.Format(Properties.Resources.NumericUpDown_Tooltip,
                NDCSDataSource.MIN_TIMEOUT_VALUE, NDCSDataSource.MAX_TIMEOUT_VALUE));
            this.configInfo_TrafodionToolTip.SetToolTip(connectionIdleCustom_TrafodionNumericUpDown,
                String.Format(Properties.Resources.NumericUpDown_Tooltip,
                NDCSDataSource.MIN_TIMEOUT_VALUE, NDCSDataSource.MAX_TIMEOUT_VALUE));
            
            //Add Tooltips to Label controls
            this.configInfo_TrafodionToolTip.SetToolTip(this.serverAvail_TrafodionLabel, Properties.Resources.ServerAvail_TrafodionLabel);
            this.configInfo_TrafodionToolTip.SetToolTip(this.serverMax_TrafodionLabel, Properties.Resources.ServerMax_TrafodionLabel);
            this.configInfo_TrafodionToolTip.SetToolTip(this.serverStartAhead_TrafodionLabel, Properties.Resources.ServerStartAhead_TrafodionLabel);

            //Add Tooltips to RadioButton controls
            this.configInfo_TrafodionToolTip.SetToolTip(this.serverIdleCustom_TrafodionRadioButton, Properties.Resources.ServerIdleCustom_TrafodionRadioButton);
            this.configInfo_TrafodionToolTip.SetToolTip(this.serverIdleNone_TrafodionRadioButton, Properties.Resources.ServerIdleNone_TrafodionRadioButton);
            this.configInfo_TrafodionToolTip.SetToolTip(this.serverIdleSysDefault_TrafodionRadioButton, Properties.Resources.ServerIdleSysDefault_TrafodionRadioButton);
            this.configInfo_TrafodionToolTip.SetToolTip(this.connectionIdleCustom_TrafodionRadioButton, Properties.Resources.ConnectionIdleCustom_TrafodionRadioButton);
            this.configInfo_TrafodionToolTip.SetToolTip(this.connectionIdleNone_TrafodionRadioButton, Properties.Resources.ConnectionIdleNone_TrafodionRadioButton);
            this.configInfo_TrafodionToolTip.SetToolTip(this.connectionIdleSysDefault_TrafodionRadioButton, Properties.Resources.ConnectionIdleSysDefault_TrafodionRadioButton);

            this.configInfo_TrafodionToolTip.SetToolTip(this.startModeManual_TrafodionRadioButton, Properties.Resources.StartModeManual_TrafodionRadioButton);
            this.configInfo_TrafodionToolTip.SetToolTip(this.startModeAutomatic_TrafodionRadioButton, Properties.Resources.StartModeAutomatic_TrafodionRadioButton);

            this.configInfo_TrafodionToolTip.SetToolTip(this.processPriorityDefault_TrafodionRadioButton, Properties.Resources.ProcessPriorityDefault_TrafodionRadioButton);
            this.configInfo_TrafodionToolTip.SetToolTip(this.processPriorityInitial_TrafodionRadioButton, Properties.Resources.ProcessPriorityInitial_TrafodionRadioButton);
            this.configInfo_TrafodionToolTip.SetToolTip(this.nodeUtilizationOrder_TrafodionTextBox, Properties.Resources.nodeUtilizationOrder_TrafodionTextBox);
        }

        void nodeUtilizationOrder_TrafodionTextBox_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (!char.IsNumber(e.KeyChar) && (Keys)e.KeyChar != Keys.Back && e.KeyChar != ',' && e.KeyChar != '-')
            {
                e.Handled = true;
            } 
        }

        #region Populate Controls
       
        public void PopulateControls()
        {
            try
            {
                NDCSDataSource DStoPopulate = null;//new NDCSDataSource();
                    DStoPopulate = this._NdcsDatasource;

                //** Populate the "Number of Servers" Controlls ***
                this.serverMax_TrafodionNumericUpDown.Text = DStoPopulate.MaxServerCount.ToString();
                this.serverAvail_TrafodionNumericUpDown.Text = DStoPopulate.AvailableServerCount.ToString();
                this.serverInitial_TrafodionNumericUpDown.Text = DStoPopulate.InitialServerCount.ToString();

                //Populate the name
                if (this._isCreate)
                { 
                    this.dsName_TrafodionTextBox.Text = this._NdcsDatasource.Name;
                    this.dsName_TrafodionTextBox.ReadOnly = false;
                    this.configInfo_TrafodionToolTip.SetToolTip(this.dsName_TrafodionLabel, dsName_TrafodionLabel.Text);
                }
                else
                {
                    this.dsName_TrafodionTextBox.Text = this._NdcsDatasource.Name;
                    this.dsName_TrafodionTextBox.ReadOnly = true;
                    this.configInfo_TrafodionToolTip.SetToolTip(this.dsName_TrafodionLabel, dsName_TrafodionLabel.Text);
                }


                PopulateServerIdleTimeout();
                PopulateConnectionIdleTimeout();
                PopulateProcessPriority();
                PopulateResourceStats();
                PopulateNodeUtilization();
                PopulateResources();
                PopulateStartMode();

            } catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }


        private void PopulateNodeUtilization()
        {

            if (null == this._NdcsDatasource.CPUList || this._NdcsDatasource.CPUList.Length == 0)
            {
                this.nodeUtilizationDefault_TrafodionRadioButton.Checked = true;
                return;
            } else {
                this.nodeUtilizationOrder_TrafodionRadioButton.Checked = true;
                this.nodeUtilizationOrder_TrafodionTextBox.Text = this._NdcsDatasource.CPUList;
            }
        }

        private void PopulateStartMode()
        {
            if (this._NdcsDatasource.StartAutomatically)
                startModeAutomatic_TrafodionRadioButton.Checked = true;
            else
                startModeManual_TrafodionRadioButton.Checked = true;
        }

        private void PopulateServerIdleTimeout()
        {
            //** Populate the "Server Idle Timeout" Controlls ***
            try
            {
                if (this._NdcsDatasource.ServerIdleTimeout > 0L)
                {
                    this.serverIdleCustom_TrafodionRadioButton.Checked = true;
                    this.serverIdleTimeoutCustom_TrafodionNumericUpDown.Text = this._NdcsDatasource.ServerIdleTimeout.ToString();
                }
                else if (this._NdcsDatasource.ServerIdleTimeout == 0L)
                {
                    this.serverIdleSysDefault_TrafodionRadioButton.Checked = true;
                }
                else
                {
                    this.serverIdleNone_TrafodionRadioButton.Checked = true;
                }
            }
            catch (Exception ex)
            {
                this.serverIdleNone_TrafodionRadioButton.Checked = true;
            }
        }

        private void PopulateConnectionIdleTimeout()
        {
            //** Populate the "Connection Idle Timeout" Controlls ***
            try
            {
                if (this._NdcsDatasource.ConnectionIdleTimeout > 0L)
                {
                    this.connectionIdleCustom_TrafodionRadioButton.Checked = true;
                    this.connectionIdleCustom_TrafodionNumericUpDown.Text = this._NdcsDatasource.ConnectionIdleTimeout.ToString();
                }
                else if (this._NdcsDatasource.ConnectionIdleTimeout == 0L)
                {
                    this.connectionIdleSysDefault_TrafodionRadioButton.Checked = true;
                }
                else
                {
                    this.connectionIdleNone_TrafodionRadioButton.Checked = true;
                }
            } catch(Exception ex)
            {
                this.connectionIdleNone_TrafodionRadioButton.Checked = true;
            }
        }

        private void PopulateProcessPriority()
        {
            //** Populate the "Process Priority" Controlls ***
            //Need to tweak this -- currently all proc priority is comming back as 0
            if (this._NdcsDatasource.ProcessPriority == NDCSDataSource.DEFAULT_PROCESS_PRIORITY)
            {
                this.processPriorityDefault_TrafodionRadioButton.Checked = true;
            }
            else
            {
                this.processPriorityInitial_TrafodionRadioButton.Checked = true;
                this.processPriorityInitial_TrafodionNumericUpDown.Value = this._NdcsDatasource.ProcessPriority;
            }
        }



        private void PopulateResourceStats()
        {
            if (null == this._NdcsDatasource.ResourceStats)
                return;

            this.stateStatExec_TrafodionCheckBox.Checked = this._NdcsDatasource.ResourceStats.SqlExecute;
            this.stateStatExecDirect_TrafodionCheckBox.Checked = this._NdcsDatasource.ResourceStats.SqlExecuteDirect;
            this.stateStatFetch_TrafodionCheckBox.Checked = this._NdcsDatasource.ResourceStats.SqlFetch;
            this.stateStatPrep_TrafodionCheckBox.Checked = this._NdcsDatasource.ResourceStats.SqlPrepare;
            this.stateStatStatement_TrafodionCheckBox.Checked = this._NdcsDatasource.ResourceStats.SqlStatement;
            this.sessionStatsConnection_TrafodionCheckBox.Checked = this._NdcsDatasource.ResourceStats.ConnectInfo;
            this.sessionStatsSummary_TrafodionCheckBox.Checked = this._NdcsDatasource.ResourceStats.SessionSummary;
        }

        private void PopulateResources()
        {
            //Populate the details
            //aNdcsDataSource.PopulateDetail();

                // This is not supported for SQ.
                return;

            //this.resourceMgmtAdd_TrafodionButton.Enabled = false;

            //this.ConnectivityiGrid.Rows.Clear();
            //if (null == this._NdcsDatasource.Resources || this._NdcsDatasource.Resources.Count <= 0)
            //{
            //    this.ConnectivityiGrid.UpdateCountControlText("There are {0} configured Resources");
            //    this.resourceMgmtAdd_TrafodionButton.Enabled = true;
            //    return;
            //}

            //foreach (NDCSResource theResource in this._NdcsDatasource.Resources)
            //{
            //    addResourceToIGrid(theResource);
            //    this.resourceMgmtAdd_TrafodionButton.Enabled = true;
            //}

            //this.resourceMgmtAdd_TrafodionButton.Enabled = false;
            //this.ConnectivityiGrid.Cols.AutoWidth();
            //this.ConnectivityiGrid.UpdateCountControlText("There are {0} configured Resources");
        }

        private void addResourceToIGrid(NDCSResource aResourceToAdd)
        {
            try
            {
                TenTec.Windows.iGridLib.iGRow row = this.ConnectivityiGrid.Rows.Add();
                row.Cells[0].Value = aResourceToAdd.AttributeName.ToUpper();
                row.Cells[1].Value = NDCSResource.ActionNames[(int)aResourceToAdd.ActionID];
                row.Cells[2].Value = aResourceToAdd.Limit;
                row.Tag = aResourceToAdd;
            }
            catch (Exception e)
            {
                //Error displaying Management policy
            }
        }

        #endregion Populate Controls

        #region Update Datasource Object
        
        private void UpdateNumServerControls()
        {
            //** Populate the "Number of Servers" Controlls ***
            this._NdcsDatasource.MaxServerCount = int.Parse(this.serverMax_TrafodionNumericUpDown.Text.ToString());
            this._NdcsDatasource.AvailableServerCount = int.Parse(this.serverAvail_TrafodionNumericUpDown.Text.ToString());
            this._NdcsDatasource.InitialServerCount = int.Parse(this.serverInitial_TrafodionNumericUpDown.Text.ToString());
        }

        private void UpdateServerIdleTimeout()
        {
            //** Populate the "Server Idle Timeout" Controlls ***
            try
            {
                if (this.serverIdleCustom_TrafodionRadioButton.Checked)
                {
                    //Replace with TryParse eventually
                    this._NdcsDatasource.ServerIdleTimeout = Int64.Parse(this.serverIdleTimeoutCustom_TrafodionNumericUpDown.Text.ToString());
                }
                else if (this.serverIdleSysDefault_TrafodionRadioButton.Checked)
                {
                    this._NdcsDatasource.ServerIdleTimeout = NDCSDataSource.TIMEOUT_VALUE_SYSTEM_DEFAULT;
                }
                else
                {
                    this._NdcsDatasource.ServerIdleTimeout = NDCSDataSource.TIMEOUT_VALUE_NO_TIMEOUT;
                }
            }
            catch (Exception ex)
            {
                //Error updating Server Idle Timeout properties
            }
        }


        private void UpdateConnectionIdleTimeout()
        {
            //** Populate the "Server Idle Timeout" Controlls ***
            try
            {
                if (this.connectionIdleCustom_TrafodionRadioButton.Checked)
                {
                    //Replace with TryParse eventually
                    this._NdcsDatasource.ConnectionIdleTimeout = Int64.Parse(this.connectionIdleCustom_TrafodionNumericUpDown.Text.ToString());
                }
                else if (this.connectionIdleSysDefault_TrafodionRadioButton.Checked)
                {
                    this._NdcsDatasource.ConnectionIdleTimeout = NDCSDataSource.TIMEOUT_VALUE_SYSTEM_DEFAULT;
                }
                else
                {
                    this._NdcsDatasource.ConnectionIdleTimeout = NDCSDataSource.TIMEOUT_VALUE_NO_TIMEOUT;
                }
            }
            catch (Exception ex)
            {
                //Error updating Server Idle Timeout properties
            }
        }

        private void UpdateProcessPriority()
        {
            //** Update the "Process Priority" Controlls ***
            if (this.processPriorityDefault_TrafodionRadioButton.Checked )
            {
                //Set default value for ProcessPriority
                this._NdcsDatasource.ProcessPriority = NDCSDataSource.DEFAULT_PROCESS_PRIORITY;
            }
            else
            {
                //Use TryParse eventually
                this._NdcsDatasource.ProcessPriority =ushort.Parse( this.processPriorityInitial_TrafodionNumericUpDown.Text.ToString());
            }
        }

        private void UpdateNodeUtilization()
        {
            //** Update the "Node Utilization" Controls ***
            if (this.nodeUtilizationDefault_TrafodionRadioButton.Checked)
            {
                //Set default value for Node Utilization
                this._NdcsDatasource.CPUList = NDCSDataSource.DEFAULT_CPU_LIST;
            }
            else
            {
                //Use TryParse eventually
                this._NdcsDatasource.CPUList = this.nodeUtilizationOrder_TrafodionTextBox.Text.Trim();
            }
        }

        private void UpdateResourceStats()
        {
            try
            {
                if (null == this._NdcsDatasource.ResourceStats)
                    return;

                this._NdcsDatasource.ResourceStats.SqlExecute = this.stateStatExec_TrafodionCheckBox.Checked;
                this._NdcsDatasource.ResourceStats.SqlExecuteDirect = this.stateStatExecDirect_TrafodionCheckBox.Checked;
                this._NdcsDatasource.ResourceStats.SqlFetch = this.stateStatFetch_TrafodionCheckBox.Checked;
                this._NdcsDatasource.ResourceStats.SqlPrepare = this.stateStatPrep_TrafodionCheckBox.Checked;
                this._NdcsDatasource.ResourceStats.SqlStatement = this.stateStatStatement_TrafodionCheckBox.Checked;
                this._NdcsDatasource.ResourceStats.ConnectInfo = this.sessionStatsConnection_TrafodionCheckBox.Checked;
                this._NdcsDatasource.ResourceStats.SessionSummary = this.sessionStatsSummary_TrafodionCheckBox.Checked;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private void UpdateStartMode()
        {
            this._NdcsDatasource.StartAutomatically = startModeAutomatic_TrafodionRadioButton.Checked;       
        }

       #endregion Update Datasource Object

        private void addEventHandlers()
        {
            this.nodeUtilizationOrder_TrafodionTextBox.KeyPress += new KeyPressEventHandler(nodeUtilizationOrder_TrafodionTextBox_KeyPress);

            this.serverMax_TrafodionNumericUpDown.ValueChanged += new System.EventHandler(ServerValueChangeHandler);
            this.serverAvail_TrafodionNumericUpDown.ValueChanged += new System.EventHandler(ServerValueChangeHandler);
            this.serverInitial_TrafodionNumericUpDown.ValueChanged += new System.EventHandler(ServerValueChangeHandler);

            this.serverMax_TrafodionNumericUpDown.ValueChanged += new System.EventHandler(this.genericChangeHandler);
            this.serverInitial_TrafodionNumericUpDown.ValueChanged += new System.EventHandler(this.genericChangeHandler);
            this.serverAvail_TrafodionNumericUpDown.ValueChanged += new System.EventHandler(this.genericChangeHandler);
            
            this.connectionIdleCustom_TrafodionNumericUpDown.ValueChanged += new EventHandler(this.genericChangeHandler);
            this.connectionIdleCustom_TrafodionRadioButton.CheckedChanged += new EventHandler(this.genericChangeHandler);
            this.connectionIdleNone_TrafodionRadioButton.CheckedChanged += new EventHandler(this.genericChangeHandler);
            this.connectionIdleSysDefault_TrafodionRadioButton.CheckedChanged += new EventHandler(this.genericChangeHandler);

            this.serverIdleCustom_TrafodionRadioButton.CheckedChanged += new EventHandler(this.genericChangeHandler);
            this.serverIdleNone_TrafodionRadioButton.CheckedChanged +=new EventHandler(this.genericChangeHandler);
            this.serverIdleSysDefault_TrafodionRadioButton.CheckedChanged +=new EventHandler(this.genericChangeHandler);
            this.serverIdleTimeoutCustom_TrafodionNumericUpDown.ValueChanged +=new EventHandler(this.genericChangeHandler);
                
            this.startModeAutomatic_TrafodionRadioButton.CheckedChanged +=new EventHandler(this.genericChangeHandler);
            this.startModeManual_TrafodionRadioButton.CheckedChanged +=new EventHandler(this.genericChangeHandler);
            
            this.processPriorityDefault_TrafodionRadioButton.CheckedChanged +=new EventHandler(this.genericChangeHandler);
            this.processPriorityInitial_TrafodionNumericUpDown.ValueChanged +=new EventHandler(this.genericChangeHandler);
            this.processPriorityInitial_TrafodionRadioButton.CheckedChanged +=new EventHandler(this.genericChangeHandler);

            this.nodeUtilizationDefault_TrafodionRadioButton.CheckedChanged +=new EventHandler(this.genericChangeHandler);
            this.nodeUtilizationOrder_TrafodionRadioButton.CheckedChanged +=new EventHandler(this.genericChangeHandler);
            this.nodeUtilizationOrder_TrafodionTextBox.TextChanged +=new EventHandler(this.genericChangeHandler);

            this.sessionStatsConnection_TrafodionCheckBox.CheckedChanged +=new EventHandler(this.genericChangeHandler);
            this.sessionStatsSummary_TrafodionCheckBox.CheckedChanged +=new EventHandler(this.genericChangeHandler);

            this.stateStatExec_TrafodionCheckBox.CheckedChanged +=new EventHandler(this.genericChangeHandler);
            this.stateStatExecDirect_TrafodionCheckBox.CheckedChanged +=new EventHandler(this.genericChangeHandler);
            this.stateStatFetch_TrafodionCheckBox.CheckedChanged +=new EventHandler(this.genericChangeHandler);
            this.stateStatPrep_TrafodionCheckBox.CheckedChanged +=new EventHandler(this.genericChangeHandler);
            this.stateStatStatement_TrafodionCheckBox.CheckedChanged += new EventHandler(this.genericChangeHandler);

        }

        private void ServerValueChangeHandler(object sender, EventArgs e)
        {
            decimal availParseResult = serverAvail_TrafodionNumericUpDown.Value;
            decimal maxParseResult = serverMax_TrafodionNumericUpDown.Value;
            decimal initial = serverInitial_TrafodionNumericUpDown.Value;

            if (availParseResult > maxParseResult)
            {
                serverAvail_TrafodionNumericUpDown.Value = serverMax_TrafodionNumericUpDown.Value;
            }

            if (initial > maxParseResult)
            {
                serverInitial_TrafodionNumericUpDown.Value = serverMax_TrafodionNumericUpDown.Value;
            }


        }


        public void SetupIGrid()
        {
            //Setup buttons
            this.resourceMgmtModify_TrafodionButton.Enabled = false;
            this.resourceMgmtRemove_TrafodionButton.Enabled = false;

            this.ConnectivityiGrid = new ConnectivityObjectsIGrid<Trafodion.Manager.ConnectivityArea.Model.NDCSObject>();

            //Control theButtonControl = this.ConnectivityiGrid.GetButtonControl();
            //theButtonControl.Dock = DockStyle.Fill;
            //this.resourceIGridOptions_TrafodionPanel.Controls.Add(theButtonControl);

            DataTable dataTable = new DataTable();
            dataTable.Columns.Add("Attribute", typeof(System.String));
            dataTable.Columns.Add("Action", typeof(System.String));
            dataTable.Columns.Add("Limit", typeof(System.Int64));

            ConnectivityiGrid.BeginUpdate();
            ConnectivityiGrid.FillWithData(dataTable);
            ConnectivityiGrid.ResizeGridColumns(dataTable);
            ConnectivityiGrid.EndUpdate();
            ConnectivityiGrid.Dock = DockStyle.Fill;

            this.resourceMgmt_TrafodionPanel.Controls.Add(ConnectivityiGrid);
            this.ConnectivityiGrid.AddCountControlToParent("There are {0} Resources configured", DockStyle.Top);

            //Configure event handlers
            ConnectivityiGrid.DoubleClickHandler = new TrafodionIGrid.DoubleClickDelegate(DoubleClick_ConnectivityiGrid);
            ConnectivityiGrid.SelectionChanged += new EventHandler(ConnectivityiGrid_SelectionChanged);
        }

        void ConnectivityiGrid_SelectionChanged(object sender, EventArgs e)
        {
            bool isRowSelected = (this.ConnectivityiGrid.SelectedRows.Count != 0);
            this.resourceMgmtModify_TrafodionButton.Enabled = isRowSelected;
            this.resourceMgmtRemove_TrafodionButton.Enabled = isRowSelected;
        }

        private void DoubleClick_ConnectivityiGrid(int row)
        {
            ModifyResource(row);
        }

        private void genericChangeHandler(object sender, EventArgs e)
        {
            ValueChanged();
        }

        private void ValueChanged()
        {
            this.valuesChanged = true;
            if (null != this._linkToParent)
            {
                this._linkToParent.EnableApplyButton();
            }
        }

        public bool CheckAndCommit()
        {
            //Perform final consistency validation
            
            //Commit values from GUI Controls --
            //Overwrite existing values in the Datasource object.
            try
            {
                //Update the name. For update the name stays the same
                if (_isCreate)
                {
                    this._NdcsDatasource.Name = this.dsName_TrafodionTextBox.Text;
                }
                UpdateStartMode();
                UpdateNumServerControls();
                UpdateServerIdleTimeout();
                UpdateConnectionIdleTimeout();
                UpdateNodeUtilization();
                UpdateProcessPriority();
                UpdateResourceStats();
                //UpdateResources();
            } catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
                return false;
            }

            this.valuesChanged = false;
            return true;
        }


        //Handle the event thrown when the user clicks "Add Resource"
        private void resourceMgmtAdd_TrafodionButton_Click(object sender, EventArgs e)
        {
            ConnectivityAreaConfigManagementAddDialog resourceMgmtAddDialog = new ConnectivityAreaConfigManagementAddDialog();
            DialogResult result = resourceMgmtAddDialog.New();
            if (result == DialogResult.OK)
            {
                //this.addResourceToIGrid(resourceMgmtAddDialog.TheResource);
                if(null == this._NdcsDatasource.Resources)
                    this._NdcsDatasource.Resources = new System.Collections.Generic.List<NDCSResource>();

                NDCSResource NewResource = resourceMgmtAddDialog.TheResource;

                this._NdcsDatasource.AddResource(NewResource.AttributeName, NewResource.Limit, NewResource.ActionID);
                this.resourceMgmtAdd_TrafodionButton.Enabled = false;

                ValueChanged();
                this.PopulateResources();
            }
        }

        //Handle the event thrown when the user clicks "Edit Resource"
        private void resourceMgmtModify_TrafodionButton_Click(object sender, EventArgs e)
        {
            if (this.ConnectivityiGrid.SelectedRows.Count != 1)
                return;
            
            ModifyResource(ConnectivityiGrid.SelectedRows[0].Index);
        }

        private void ModifyResource(int row)
        {
            ConnectivityAreaConfigManagementAddDialog resourceMgmtEditDialog = new ConnectivityAreaConfigManagementAddDialog();
            NDCSResource resourceToEdit = (NDCSResource)this.ConnectivityiGrid.Rows[row].Tag;
            DialogResult result = resourceMgmtEditDialog.Edit(resourceToEdit);
            if (result == DialogResult.OK)
            {
                //Handle resource edit
                NDCSResource NewResource = resourceMgmtEditDialog.TheResource;
                resourceToEdit.AttributeName = NewResource.AttributeName;
                resourceToEdit.ActionID = NewResource.ActionID;
                resourceToEdit.Limit = NewResource.Limit;

                ValueChanged();
                this.PopulateResources();
            }

        }

        private void resourceMgmtRemove_TrafodionButton_Click(object sender, EventArgs e)
        {

            if (this.ConnectivityiGrid.SelectedRows.Count != 1)
                return;

            //Prompt: Are you sure you want to remove this Resource?
            NDCSResource resourceToRemove = (NDCSResource)this.ConnectivityiGrid.SelectedRows[0].Tag;
            if (this._NdcsDatasource.Resources.Remove(resourceToRemove))
            {
                //Remove successfull
                ValueChanged();
                this.PopulateResources();
                this.resourceMgmtAdd_TrafodionButton.Enabled = true;
                this.resourceMgmtModify_TrafodionButton.Enabled = false;
                this.resourceMgmtRemove_TrafodionButton.Enabled = false;
            }
        }


        #region ICloneToWindow Members

        /// <summary>
        /// Creates a new instance
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            return new ConnectivityAreaDatasourceConfigGeneralUserControl(NdcsDatasource, this._isCreate);

        }

        /// <summary>
        /// Get the window title of the cloned window
        /// </summary>
        public string WindowTitle
        {
            get { return Properties.Resources.TabPageLabel_GeneralProperties + " - " + NdcsDatasource.ConnectionDefinition.Name; }
        }

        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return NdcsDatasource.ConnectionDefinition; }
        }

        #endregion

        private void dsName_TrafodionTextBox_TextChanged(object sender, EventArgs e)
        {
            if (NDCSName.ExternalInternalFormSame(dsName_TrafodionTextBox.Text))
            {
                if (this.dsName_TrafodionTextBox.Text.Length > 128)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), "Data source name cannot be longer than 128 characters.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    this.dsName_TrafodionTextBox.Text = this.dsName_TrafodionTextBox.Text.Substring(0, 128);
                }
            }
            else
            {
                if (this.dsName_TrafodionTextBox.Text.Length > 126)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), "Data source name contains special characters and the max allowed length is 126 characters", "Error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    this.dsName_TrafodionTextBox.Text = this.dsName_TrafodionTextBox.Text.Substring(0, 126);
                }
            }
        }
    }
}
