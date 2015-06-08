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
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.WorkloadArea.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class ServicesSummaryUserControl : UserControl, ICloneToWindow
    {
        #region Member variables

        private static readonly string _servicesListConfigName = "WMSServiceSummaryConfig";
        UniversalWidgetConfig _servicesWidgetConfig = null;
        GenericUniversalWidget _servicesWidget = null;
        ServicesGridDataHandler _servicesWidgetDisplayHandler = null;
        ConnectionDefinition _connectionDefinition;
        TrafodionIGrid _servicesGrid = null;

        #endregion Member variables
        
        #region Public properties

        public ConnectionDefinition ConnectionDefn
        {
            get { return _connectionDefinition; }
            set
            {
                if (_connectionDefinition != null)
                {
                    _servicesWidget.DataProvider.Stop();
                    _servicesGrid.Rows.Clear();
                }
                _connectionDefinition = value;
                if (_connectionDefinition != null)
                {
                    _servicesWidgetConfig.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
                    _servicesWidget.StartDataProvider();
                }
            }
        }

        #endregion Public properties

        #region Constructors/Destructors

        public ServicesSummaryUserControl()
        {
            InitializeComponent();
        }

        public ServicesSummaryUserControl(ConnectionDefinition aConnectionDefinition)
            :this()
        {
            InitializeWidgetConfig();
            InitializeWidget();
            ConnectionDefn = aConnectionDefinition;

            if (aConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                _servicesGrid.AlwaysHiddenColumnNames.Add(WmsCommand.COL_MAX_SSD_USAGE);
            }
        }

        #endregion Constructors/Destructors

        #region IClone methods

        /// <summary>
        /// Clones the services user control into a new window
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            ServicesSummaryUserControl clonedServicesControl = new ServicesSummaryUserControl(ConnectionDefn);
            return clonedServicesControl;
        }

        /// <summary>
        /// Title for the cloned window
        /// </summary>
        public string WindowTitle
        {
            get
            {
                return Properties.Resources.Services;
            }
        }

        #endregion IClone methods

        #region private methods

        void InitializeWidgetConfig()
        {
            _servicesWidgetConfig = WidgetRegistry.GetConfigFromPersistence(_servicesListConfigName);
            if (_servicesWidgetConfig == null)
            {
                _servicesWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
                _servicesWidgetConfig.Name = _servicesListConfigName;
                _servicesWidgetConfig.Title = Properties.Resources.Services;
                _servicesWidgetConfig.ShowProperties = false;
                _servicesWidgetConfig.ShowToolBar = true;
                _servicesWidgetConfig.ShowChart = false;
                _servicesWidgetConfig.ShowTimerSetupButton = false;
                _servicesWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
                _servicesWidgetConfig.HelpTopic = HelpTopics.ServiceSummary;
                _servicesWidgetConfig.ShowHelpButton = true;

                DatabaseDataProviderConfig _dbConfig = _servicesWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;
                _dbConfig.OpenCommand = "WMSOPEN";
                _dbConfig.SQLText = "STATUS SERVICE";
                _dbConfig.CloseCommand = "WMSCLOSE";
                _dbConfig.CommandTimeout = 0;
            }

        }

        void InitializeWidget()
        {
            // Remove all current contents and add the services summary widget
            _theWidgetPanel.Controls.Clear();

            SetColumnMappings();

            _servicesWidget = new GenericUniversalWidget();
            _servicesWidget.DataProvider = new DatabaseDataProvider(_servicesWidgetConfig.DataProviderConfig);

            _servicesGrid = ((TabularDataDisplayControl)_servicesWidget.DataDisplayControl).DataGrid;

            //Add Context Menu
            AddContextMenuItems();            

            //Set the display properties of the widget and add it to the control
            ((TabularDataDisplayControl)_servicesWidget.DataDisplayControl).LineCountFormat = Properties.Resources.Services;
            _servicesWidgetDisplayHandler = new ServicesGridDataHandler(this);
            _servicesWidget.DataDisplayControl.DataDisplayHandler = _servicesWidgetDisplayHandler;
            _servicesWidget.UniversalWidgetConfiguration = _servicesWidgetConfig;
            _servicesWidget.Dock = DockStyle.Fill;
            _theWidgetPanel.Controls.Add(_servicesWidget);

            _servicesGrid.RowMode = true;
            //Double Click            
            _servicesGrid.DoubleClickHandler = ServiceDetails_Handler;

            //These two handler are working on context sensitivity of command buttons and context menu items
            _servicesGrid.SelectionChanged += new EventHandler(_servicesGrid_SelectionChanged);
            _servicesGrid.VisibleChanged += new EventHandler(_servicesGrid_VisibleChanged);
        }

        /// <summary>
        /// Cleanup
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            
        }

        void SetColumnMappings()
        {
            List<ColumnMapping> columnMappings = new List<ColumnMapping>();
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_SERVICE_NAME, "Name", 128));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_STATE, "State", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_SERVICE_PRIORITY, "Priority", 40));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_COMMENT, "Comment", 128));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_MAX_CPU_BUSY, "Max CPU Busy", 128));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_MAX_MEM_USAGE, "Max Memory Usage", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_MAX_SSD_USAGE, "Max SSD Usage", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_MAX_ROWS_FETCHED, "Max Rows Fetched", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_MAX_EXEC_QUERIES, "Max Exec Queries", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_MAX_ESPS, "Max Avg ESPs", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_ACTIVE_TIME, "Active Time", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_SQL_PLAN, "Save SQL Plan", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_SQL_TEXT, "Save SQL Text", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_EXEC_TIMEOUT, "Execution Timeout", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_HOLD_TIMEOUT, "Hold Timeout", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_WAIT_TIMEOUT, "Wait Timeout", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_SQL_DEFAULTS, "SQL Defaults", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_CANCEL_QUERY_IF_CLIENT_DISAPPEARS, "Cancel Query If \r\nClient Disappears", 50));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_CHECK_QUERY_EST_RESOURCE_USE, "Include Query Est. \r\nResource Use", 50));

            _servicesWidgetConfig.DataProviderConfig.ColumnMappings = columnMappings;            
        }


        private void AddContextMenuItems() 
        {
            //Add Service Context Menu
            addServiceMenuItem.Text = Properties.Resources.AddService;
            addServiceMenuItem.Click += new EventHandler(addServiceMenuItem_Click);
            _servicesGrid.AddContextMenu(addServiceMenuItem);

            //Alter Service Context Menu
            alterServiceMenuItem.Text = Properties.Resources.AlterService;
            alterServiceMenuItem.Click += new EventHandler(alterServiceMenuItem_Click);
            _servicesGrid.AddContextMenu(alterServiceMenuItem);

            //Delete Service Context Menu
            deleteServiceMenuItem.Text = Properties.Resources.DeleteService;
            deleteServiceMenuItem.Click += new EventHandler(deleteServiceMenuItem_Click);
            _servicesGrid.AddContextMenu(deleteServiceMenuItem);

            //Add A Separator
            ToolStripSeparator separator1 = new ToolStripSeparator();
            _servicesGrid.AddToolStripSeparator(separator1);

            //Start Servie Context Menu
            startServiceMenuItem.Text = Properties.Resources.StartService;
            startServiceMenuItem.Click += new EventHandler(startServiceMenuItem_Click);
            _servicesGrid.AddContextMenu(startServiceMenuItem);            

            //Stop Service Context Menu
            stopServiceMenuItem.Text = Properties.Resources.StopService;
            stopServiceMenuItem.Click += new EventHandler(stopServiceMenuItem_Click);
            _servicesGrid.AddContextMenu(stopServiceMenuItem);

            //Add A Separator
            ToolStripSeparator separator2 = new ToolStripSeparator();
            _servicesGrid.AddToolStripSeparator(separator2);

            //Hold Service Context Menu
            holdServiceMenuItem.Text = Properties.Resources.HoldService;
            holdServiceMenuItem.Click += new EventHandler(holdServiceMenuItem_Click);
            _servicesGrid.AddContextMenu(holdServiceMenuItem);

            //Release Service Context Menu
            releaseServiceMenuItem.Text = Properties.Resources.ReleaseService;
            releaseServiceMenuItem.Click += new EventHandler(releaseServiceMenuItem_Click);
            _servicesGrid.AddContextMenu(releaseServiceMenuItem);

            //Hold All Service Context Menu
            holdAllServicesMenuItem.Text = Properties.Resources.HoldAllServices;
            holdAllServicesMenuItem.Click += new EventHandler(holdAllServicesMenuItem_Click);
            _servicesGrid.AddContextMenu(holdAllServicesMenuItem);

            //Release All Services Context Menu
            releaseAllServicesMenuItem.Text = Properties.Resources.ReleaseAllServices;
            releaseAllServicesMenuItem.Click += new EventHandler(releaseAllServicesMenuItem_Click);
            _servicesGrid.AddContextMenu(releaseAllServicesMenuItem);
        }

        void ServiceDetails_Handler(int rowIndex)
        {            
            string serviceName = _servicesGrid.Rows[rowIndex].Cells["SERVICE_NAME"].Value.ToString();
            try
            {            
                WmsSystem wmsSystem = WmsSystem.FindWmsSystem(ConnectionDefn);            
                WmsService wmsService = wmsSystem.FindService(serviceName);

                WMSEditServiceDialog editDialog = new WMSEditServiceDialog(WmsCommand.WMS_ACTION.ALTER_SERVICE, wmsSystem, wmsService);
                if (editDialog.ShowDialog() == DialogResult.OK)
                {
                    refreshGrid("");
                }
            }
            catch (Exception ex)
            {
                if (Logger.IsTracingEnabled)
                    Logger.OutputToLog("Warning -- Unable to get the service model. Details = " + ex.Message);
            } 
        }
     
        void addServiceMenuItem_Click(object sender, EventArgs e) 
        {
            _addButton.PerformClick();
        }

        void deleteServiceMenuItem_Click(object sender, EventArgs e) 
        {
            _deleteButton.PerformClick();
        }

        void alterServiceMenuItem_Click(object sender, EventArgs e)
        {
            _alterButton.PerformClick();
        }

        void startServiceMenuItem_Click(object sender, EventArgs e) 
        {
            _startButton.PerformClick();
        }

        void stopServiceMenuItem_Click(object sender, EventArgs e) 
        {
            _stopButton.PerformClick();
        }

        void holdServiceMenuItem_Click(object sender, EventArgs e) 
        {
            _holdButton.PerformClick();
        }

        void holdAllServicesMenuItem_Click(object sender, EventArgs e) 
        {
            _holdAllButton.PerformClick();
        }

        void releaseServiceMenuItem_Click(object sender, EventArgs e) 
        {
            _releaseButton.PerformClick();
        }

        void releaseAllServicesMenuItem_Click(object sender, EventArgs e) 
        {
            _releaseAllButton.PerformClick();
        }

        void _servicesGrid_SelectionChanged(object sender, EventArgs e)
        {
            enableDisableServiceLevelButtons();
        }

        void _servicesGrid_VisibleChanged(object sender, EventArgs e) 
        {
            initButtonContextStatus();
        }

        void UpdateControls()
        {
            enableDisableServiceLevelButtons();
        }

        #endregion private methods

        #region ServicesGridDataHandler Class

        public class ServicesGridDataHandler : TabularDataDisplayHandler
        {
            #region Fields

            private ServicesSummaryUserControl _theServicesUserControl;
            private object _locker;

            #endregion Fields

            #region Constructors

            public ServicesGridDataHandler(ServicesSummaryUserControl aServicesUserControl)
            {
                _theServicesUserControl = aServicesUserControl;
            }

            #endregion Constructors

            #region Public methods

            public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                            Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
            {
                lock (this)
                {
                    populate(aConfig, aDataTable, aDataGrid);
                }
            }

            #endregion Public methods

            #region Private methods

            private void populate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                            Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
            {
                if (null == aDataTable)
                {
                    return;
                }

                base.DoPopulate(aConfig, aDataTable, aDataGrid);

                string gridHeaderText = string.Format("There are {0} services", aDataTable.Rows.Count);
                aDataGrid.UpdateCountControlText(gridHeaderText);
                if (aDataTable.Rows.Count > 0)
                {
                    aDataGrid.ResizeGridColumns(aDataTable, 7, 20);
                }

                WmsSystem wmsSystem = WmsSystem.FindWmsSystem(_theServicesUserControl.ConnectionDefn);
                wmsSystem.LoadServices(aDataTable);
                wmsSystem.ResetConfiguration();
                wmsSystem.LoadConfiguration();

                _theServicesUserControl.UpdateControls();

            }
            #endregion Private methods
        }

        #endregion ServicesGridDataHandler Class


        private void addService()
        {
            WmsSystem wmsSystem = WmsSystem.FindWmsSystem(ConnectionDefn);
            if (null == wmsSystem)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "WMS system does not exist.\n", TrafodionForm.TitlePrefix + "Warning", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else 
            {
                WMSEditServiceDialog addDialog = new WMSEditServiceDialog(WmsCommand.WMS_ACTION.ADD_SERVICE, wmsSystem, null);
                if (addDialog.ShowDialog() == DialogResult.OK)
                {
                    refreshGrid("");
                }
            }            
        }

        private void deleteService()
        {
            string notAllStoppedServices = "";
            bool isAllRowsMatched = false;
            string defaultServiceSkipped = "";
            bool isDefaultServiceSkipped = false;
            List<WmsService> services = new List<WmsService>();

            //only stopped services can be deleted.
            services = this.getSelectedServicesWithState(WmsCommand.STOPPED_STATE, out isAllRowsMatched);
            if (services.Count == 0)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "None of the selected services are in STOPPED state. \nOnly STOPPED services can be deleted.\n",
                                TrafodionForm.TitlePrefix + "Delete Service(s)", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }
                 
            if (!isAllRowsMatched)
            {
                notAllStoppedServices = "\nNot all the selected services are in STOPPED state. " +
                                        "Only the STOPPED services will be deleted. \n";
            }

            //Default service cannot be deleted. So ignore it from the selected list
            foreach (WmsService wmsService in services)
            {
                if (wmsService.isSystemService)
                {
                    services.Remove(wmsService);
                    isDefaultServiceSkipped = true;
                    break;
                }
            }

            if (services.Count == 0)
            {
                //The only reason that the list could be empty is because default service was the only selected service
                //and we ignored it.
                if (isDefaultServiceSkipped)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), "System services cannot be deleted.\n",
                                    TrafodionForm.TitlePrefix + "Delete Service(s)", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    return;

                }
            }            

            //Now that all the check is passed, let's confirm users whether they are sure that they will delete the selected services.
           
            //If we ignored default service, let the user know
            if (isDefaultServiceSkipped)
            {
                defaultServiceSkipped = "\nSystem services cannot be deleted. Ignoring delete request for system services.\n";
            }

            StringBuilder servicesList = new StringBuilder();
            int nServices = 0;
            foreach (WmsService wmsService in services)
            {
                nServices++;
                if (20 >= nServices)
                    servicesList.AppendLine(wmsService.Name);
            }

            if (20 < nServices)
                servicesList.Append("\n     ...  and " + (nServices - 20) + " more ... ");

            DialogResult result = MessageBox.Show(Utilities.GetForegroundControl(), notAllStoppedServices + defaultServiceSkipped +
                                                "\nPlease confirm that you wish to delete the following service(s)? \n" +
                                                servicesList + "\n",
                                                TrafodionForm.TitlePrefix + "Delete Service(s)", MessageBoxButtons.YesNo, MessageBoxIcon.Question);

            StringBuilder deleteErrors_sb = new StringBuilder();
            if (result == DialogResult.Yes)
            {
                this.Cursor = Cursors.WaitCursor;
                foreach (WmsService wmsService in services)
                {
                    try
                    {
                        wmsService.Delete(true);
                    }
                    catch (Exception deleteExc)
                    {
                        if (0 < deleteErrors_sb.Length)
                            deleteErrors_sb.Append("\n         ");

                        deleteErrors_sb.Append("\t Service ").Append(wmsService.Name).Append(" : ");
                        deleteErrors_sb.Append(deleteExc.Message);
                    }
                }

                this.Cursor = Cursors.Default;
                refreshGrid("");

                if (0 < deleteErrors_sb.Length)
                    MessageBox.Show(Utilities.GetForegroundControl(), "\nError: WMS System error deleting the selected services. \n\n" +
                                    "Problem: \t Unable to delete some of the selected services. \n\n" +
                                    "Solution: \t Please check error details for recovery information. \n\n" +
                                    "Details: " + deleteErrors_sb.ToString() + "\n\n",
                                    TrafodionForm.TitlePrefix + "Delete Service(s) Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                else
                    MessageBox.Show(Utilities.GetForegroundControl(), "\n  The selected services were deleted. \n", 
                                    TrafodionForm.TitlePrefix + "Delete Service(s)",
                                    MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
        }
        
        
        private void startButton_Click(object sender, EventArgs e)
        {
            startService();
        }

        private void startService()
        {
            bool isAllRowsMatched = false;
            List<WmsService> services = this.getSelectedServicesWithState(WmsCommand.STOPPED_STATE, out isAllRowsMatched);
            if (services.Count == 0)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "None of the selected services are in STOPPED state. \nOnly STOPPED services can be started.\n",
                                TrafodionForm.TitlePrefix + "Start Service(s)", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            if (!isAllRowsMatched)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "\nNot all the selected services are in STOPPED state. " +
                                      "Only the STOPPED services will be started. \n",
                                      TrafodionForm.TitlePrefix + "Start Service(s)", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }

            StringBuilder startErrors_sb = new StringBuilder();
            this.Cursor = Cursors.WaitCursor;
            foreach (WmsService wmsService in services)
            {
                try
                {
                    wmsService.Start();

                }
                catch (Exception startExc)
                {
                    startErrors_sb.Append("\t " + wmsService.Name + " - " + startExc.Message + "\n\t");
                }

            }

            this.Cursor = Cursors.Default;
            refreshGrid("");
            if (0 < startErrors_sb.Length)
                MessageBox.Show(Utilities.GetForegroundControl(), "\nWMS System error starting the selected services. \n\n" +
                                "Problem: \t Unable to start some of the selected services. \n\n" +
                                "Solution: \t Please check error details for recovery information. \n\n" +
                                "Details: " + startErrors_sb.ToString() + "\n\n",
                                TrafodionForm.TitlePrefix + "Start Service(s) Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            else
                MessageBox.Show(Utilities.GetForegroundControl(), "\n  The selected services were started. \n",
                                TrafodionForm.TitlePrefix + "Start Service(s)",
                                MessageBoxButtons.OK, MessageBoxIcon.Information);


        }

        private void stopService()
        {
            bool isAllRowsMatched = false;
            bool isDefaultServiceSkipped = false;

            String[] validStates = new String[] { WmsCommand.ACTIVE_STATE, WmsCommand.HOLD_STATE,
                                                  WmsCommand.TRANSIENT_STATE
                                                };
            List<WmsService> services = this.getSelectedServicesWithStates(validStates, out isAllRowsMatched);

            //Default service cannot be stopped. So remove it from the list, if present.
            List<WmsService> servicesToLoopThru = new List<WmsService>(services);
            foreach (WmsService wmsService in servicesToLoopThru)
            {
                if (wmsService.isSystemService)
                {
                    services.Remove(wmsService);
                    isDefaultServiceSkipped = true;
                }
            }

            if (services.Count == 0)
            {
                if (isDefaultServiceSkipped)
                {
                    //Reason list is now empty is that only default service was active in selected list and we are ignoring it.
                    MessageBox.Show(Utilities.GetForegroundControl(), "Stopping of System services is not allowed.\n",
                                    TrafodionForm.TitlePrefix + "Stop Service(s)", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                else
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), "None of the selected services are in ACTIVE or HOLD or TRANSIENT states. \n" +
                                    "Only ACTIVE or on HOLD or TRANSIENT services can be stopped.\n",
                                    TrafodionForm.TitlePrefix + "Stop Service(s)", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                return;
            }


            bool hasImmediateStop = true;

            WMSCommandStopConfirmationDialog servicesStopDialog =
                             new WMSCommandStopConfirmationDialog(services, hasImmediateStop,
                                                                     isDefaultServiceSkipped, !isAllRowsMatched);
            DialogResult result = servicesStopDialog.ShowDialog();

            StringBuilder stopErrors_sb = new StringBuilder();
            if (result == DialogResult.Yes)
            {
                this.Cursor = Cursors.WaitCursor;
                foreach (WmsService wmsService in services)
                {
                    try
                    {
                        wmsService.Stop(servicesStopDialog.StopImmediately);

                    }
                    catch (Exception stopExc)
                    {
                        stopErrors_sb.Append("\t " + wmsService.Name + " - " + stopExc.Message + "\n\t");
                    }

                }
                this.Cursor = Cursors.Default;
                refreshGrid("");

                if (0 < stopErrors_sb.Length)
                    MessageBox.Show(Utilities.GetForegroundControl(), "\nError: WMS System error stopping the selected services. \n\n" +
                                    "Problem: \t Unable to stop some of the selected services. \n\n" +
                                    "Solution: \t Please check error details for recovery information. \n\n" +
                                    "Details: " + stopErrors_sb.ToString() + "\n\n",
                                    TrafodionForm.TitlePrefix + "Stop Service(s)", MessageBoxButtons.OK, MessageBoxIcon.Error);
                else
                    MessageBox.Show(Utilities.GetForegroundControl(), "\n  The selected services were stopped. \n", 
                                    TrafodionForm.TitlePrefix + "Stop Service(s)",
                                    MessageBoxButtons.OK, MessageBoxIcon.Information);

            }

        }

        private void holdService()
        {
            string notAllActiveServices = "";

            bool isAllRowsMatched = false;
            List<WmsService> services = this.getSelectedServicesWithState(WmsCommand.ACTIVE_STATE, out isAllRowsMatched);

            if (services.Count == 0)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "None of the selected services are in ACTIVE state. \nOnly ACTIVE services can be placed on hold.\n",
                                TrafodionForm.TitlePrefix + "Hold Service(s)", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            if (!isAllRowsMatched)
            {
                notAllActiveServices = "\nNot all the selected services are in ACTIVE state. " +
                                       "Only the ACTIVE services will be placed on hold. \n";
            }

            StringBuilder servicesList = new StringBuilder();
            int nServices = 0;
            foreach (WmsService wmsService in services)
            {
                nServices++;
                if (20 >= nServices)
                    servicesList.AppendLine(wmsService.Name);
            }

            if (20 < nServices)
                servicesList.Append("\n     ...  and " + (nServices - 20) + " more ... ");

            DialogResult result = MessageBox.Show(Utilities.GetForegroundControl(), notAllActiveServices +
                                                "\nAre you sure you want to put following service(s) on hold? \n" +
                                                servicesList + "\n",
                                                TrafodionForm.TitlePrefix + "Hold Service(s)", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
            StringBuilder holdErrors_sb = new StringBuilder();
            if (result == DialogResult.Yes)
            {
                this.Cursor = Cursors.WaitCursor;
                foreach (WmsService wmsService in services)
                {
                    try
                    {
                        wmsService.Hold();

                    }
                    catch (Exception holdExc)
                    {
                        holdErrors_sb.Append("\t " + wmsService.Name + " - " + holdExc.Message + "\n\t");
                    }
                }

                this.Cursor = Cursors.Default;
                refreshGrid("");

                if (0 < holdErrors_sb.Length)
                    MessageBox.Show(Utilities.GetForegroundControl(), "\nError : WMS System error putting the selected services on hold. \n\n" +
                                    "Problem: \t Unable to hold some of the selected services. \n\n" +
                                    "Solution: \t Please check error details for recovery information. \n\n" +
                                    "Details: " + holdErrors_sb.ToString() + "\n\n",
                                    TrafodionForm.TitlePrefix + "Hold Service(s) Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                else
                    MessageBox.Show(Utilities.GetForegroundControl(), "\n  The selected services were put on hold. \n", TrafodionForm.TitlePrefix + "Hold Service(s)",
                                    MessageBoxButtons.OK, MessageBoxIcon.Information);

            }

        }

        private void holdAllServices() 
        {
            WmsSystem wmsSystem = WmsSystem.FindWmsSystem(ConnectionDefn);
            if (null != wmsSystem)
            {
                DialogResult result = MessageBox.Show(Utilities.GetForegroundControl(), "\nAre you sure that you wish to place all services on hold?",
                                                TrafodionForm.TitlePrefix + "Hold All Services", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                StringBuilder holdAllErrors_sb = new StringBuilder();
                if (result == DialogResult.Yes)
                {

                    this.Cursor = Cursors.WaitCursor;
                    try
                    {
                        wmsSystem.HoldAllServices();
                    }
                    catch (Exception holdAllExc)
                    {
                        holdAllErrors_sb.Append("\t Hold All Services - " + holdAllExc.Message + "\n\t");
                    }
                    this.Cursor = Cursors.Default;
                    refreshGrid("");

                    if (0 < holdAllErrors_sb.Length)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), "\nError : Error putting all WMS Services on hold. \n\n" +
                                    "Problem: \t Internal error putting all WMS services on hold.\n\n" +
                                    "Solution: \t Please check the error details for recovery information.\n\n" +
                                    "Details: \t " + holdAllErrors_sb.ToString() + "\n\n", TrafodionForm.TitlePrefix + "Hold All Services Error",
                                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                    else
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), "\n  All WMS Services have been put on hold. \n",
                                    TrafodionForm.TitlePrefix + "Hold All Services", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                } 
            }
            else 
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "No WMS Services Exist, please reconnect to the system.");
            }
        }

        private void releaseService()
        {
            bool isAllRowsMatched = false;
            List<WmsService> services = getSelectedServicesWithState(WmsCommand.HOLD_STATE, out isAllRowsMatched);
            if (services.Count == 0)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "None of the selected services are in HOLD state. \nOnly services on HOLD can be released.\n",
                                                    TrafodionForm.TitlePrefix + "Release Service(s)", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            if (!isAllRowsMatched)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "\nNot all the selected services are in HOLD state. " +
                                      "Only the services on HOLD will be released. \n",
                                      TrafodionForm.TitlePrefix + "Release Service(s)", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }

            StringBuilder releaseErrors_sb = new StringBuilder();
            this.Cursor = Cursors.WaitCursor;
            foreach (WmsService wmsService in services)
            {
                try
                {
                    wmsService.Release();

                }
                catch (Exception releaseExc)
                {
                    releaseErrors_sb.Append("\t " + wmsService.Name + " - " + releaseExc.Message + "\n\t");
                }

            }

            this.Cursor = Cursors.Default;
            refreshGrid("");
            if (0 < releaseErrors_sb.Length)
                MessageBox.Show(Utilities.GetForegroundControl(), "\nError: WMS System error releasing the selected services. \n\n" +
                                "Problem: \t Unable to release some of the selected services. \n\n" +
                                "Solution: \t Please check error details for recovery information. \n\n" +
                                "Details: " + releaseErrors_sb.ToString() + "\n\n",
                                TrafodionForm.TitlePrefix + "Release Service(s) Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            else
                MessageBox.Show(Utilities.GetForegroundControl(), "\n  The selected services were released. \n", 
                                TrafodionForm.TitlePrefix + "Release Service(s)",
                                MessageBoxButtons.OK, MessageBoxIcon.Information);


        }

        private void releaseAllServices() 
        {
            WmsSystem wmsSystem = WmsSystem.FindWmsSystem(ConnectionDefn);
            if (null != wmsSystem)
            {
                DialogResult result = MessageBox.Show(Utilities.GetForegroundControl(), "\nAre you sure that you wish to release all services?",
                                                TrafodionForm.TitlePrefix + "Release All Services", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                StringBuilder releaseAllErrors_sb = new StringBuilder();
                if (result == DialogResult.Yes)
                {

                    this.Cursor = Cursors.WaitCursor;
                    try
                    {
                        wmsSystem.ReleaseAllServices();
                    }
                    catch (Exception holdAllExc)
                    {
                        releaseAllErrors_sb.Append("\t Release All Services - " + holdAllExc.Message + "\n\t");
                    }
                    this.Cursor = Cursors.Default;
                    refreshGrid("");

                    if (0 < releaseAllErrors_sb.Length)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), "\nError: Error releasing all WMS Services. \n\n" +
                                    "Problem: \t Internal error releasing all WMS services.\n\n" +
                                    "Solution: \t Please check the error details for recovery information.\n\n" +
                                    "Details: \t " + releaseAllErrors_sb.ToString() + "\n\n", TrafodionForm.TitlePrefix + "Release All Services Error",
                                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                    else
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), "\n  All WMS Services have been released. \n",
                                    TrafodionForm.TitlePrefix + "Release All Services", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                }
            }
            else
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "\nError: No WMS Services Exist, please reconnect to the system.");
            } 
        }

        private List<WmsService> getSelectedServices()
        {
            bool matchedAllRows = false;
            return getListOfSelectedServices(false, new Hashtable(), out matchedAllRows);
        }

        private List<WmsService> getSelectedServicesWithState(String aState, out bool isAllRowsMatched)
        {
            return getSelectedServicesWithStates(new String[] { aState }, out isAllRowsMatched);
        }

        private List<WmsService> getSelectedServicesWithStates(String[] theStates, out bool isAllRowsMatched)
        {
            Hashtable states_ht = new Hashtable();
            foreach (String aState in theStates)
                states_ht.Add(aState, aState);

            return getListOfSelectedServices(true, states_ht, out isAllRowsMatched);
        }

        private List<WmsService> getListOfSelectedServices(bool matchState, Hashtable theStates,
                                                       out bool matchedAllRows)
        {
            List<WmsService> selectedServices = new List<WmsService>();
            matchedAllRows = true;
            
            foreach (int rowIndex in _servicesGrid.SelectedRowIndexes)
            {
                WmsService wmsService = null;
                iGRow aRow = this._servicesGrid.Rows[rowIndex];
                string serviceName = aRow.Cells[WmsCommand.COL_SERVICE_NAME].Value as string;
                WmsSystem wmsSystem = WmsSystem.FindWmsSystem(ConnectionDefn);
                wmsService = wmsSystem.FindService(serviceName); 
                if (matchState)
                {
                    String serviceState = aRow.Cells[WmsCommand.COL_STATE].Value.ToString();

                    if (false == theStates.ContainsKey(serviceState))
                    {
                        matchedAllRows = false;
                        continue;
                    }
                }
                if (wmsService != null)
                {
                    selectedServices.Add(wmsService);
                } 
            }

            return selectedServices;
        }

        private void refreshGrid(string serviceName)
        {
            //loadServiceLevelDetails();
            //disableWmsControls();
            enableDisableServiceLevelButtons();
            this.ConnectionDefn = _connectionDefinition;
        }

        //The buttons have to be context sensitive. 
        //For example, delete button should not be enabled by default. Alter should only 
        //be enabled when only one row is selected etc¡­
        delegate void cmdButtonContextSenseDelegate();
        private void initButtonContextStatus()
        {
            if (this.InvokeRequired)
            {
                this.Invoke(new cmdButtonContextSenseDelegate(initButtonContextStatus));
                return;
            }

            WmsSystem wmsSystem = WmsSystem.FindWmsSystem(_connectionDefinition);
            _addButton.Enabled = addServiceMenuItem.Enabled = wmsSystem.IsServicesLoaded
                && this.ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_ADD.ToString());

            _alterButton.Enabled = alterServiceMenuItem.Enabled = false;
            _deleteButton.Enabled = deleteServiceMenuItem.Enabled = false;

            _startButton.Visible = startServiceMenuItem.Visible = true;
            _stopButton.Visible = stopServiceMenuItem.Visible = true;
            _startButton.Enabled = startServiceMenuItem.Enabled = false;
            _stopButton.Enabled = stopServiceMenuItem.Enabled = false;
            _holdButton.Enabled = holdServiceMenuItem.Enabled = false;
            _releaseButton.Enabled = releaseServiceMenuItem.Enabled = false;
        }

        private void enableDisableServiceLevelButtons()
        {
            initButtonContextStatus();
            if (_servicesGrid.SelectedRowIndexes.Count == 1)
            {
                string serviceName = this._servicesGrid.Rows[_servicesGrid.SelectedRowIndexes[0]].Cells[WmsCommand.COL_SERVICE_NAME].Value as string;
                string state = this._servicesGrid.Rows[_servicesGrid.SelectedRowIndexes[0]].Cells[WmsCommand.COL_STATE].Value as string;

                //_alterButton.Enabled = alterServiceMenuItem.Enabled = true;
                //if (state.Equals(WmsCommand.ACTIVE_STATE))
                //{
                //    _holdButton.Enabled = holdServiceMenuItem.Enabled = true;
                //    _stopButton.Enabled = stopServiceMenuItem.Enabled = true;
                //}
                //else if (state.Equals(WmsCommand.TRANSIENT_STATE))
                //{
                //    _stopButton.Enabled = stopServiceMenuItem.Enabled = true;
                //}
                //else if (state.Equals(WmsCommand.HOLD_STATE))
                //{
                //    _releaseButton.Enabled = releaseServiceMenuItem.Enabled = true;
                //    _stopButton.Enabled = stopServiceMenuItem.Enabled = true;
                //}
                //else if (state.Equals(WmsCommand.STOPPED_STATE))
                //{
                //    _deleteButton.Enabled = deleteServiceMenuItem.Enabled = true;
                //    _startButton.Enabled = startServiceMenuItem.Enabled = true;
                //}

                _alterButton.Enabled = alterServiceMenuItem.Enabled = 
                    this.ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_ALTER.ToString());

                _deleteButton.Enabled = deleteServiceMenuItem.Enabled = state.Equals(WmsCommand.STOPPED_STATE)
                    && this.ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_DELETE.ToString());

                _startButton.Enabled = startServiceMenuItem.Enabled = state.Equals(WmsCommand.STOPPED_STATE)
                    && this.ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_START.ToString());

                _stopButton.Enabled = stopServiceMenuItem.Enabled = (!state.Equals(WmsCommand.STOPPED_STATE))
                    && this.ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STOP.ToString());

                _holdButton.Enabled = holdServiceMenuItem.Enabled = state.Equals(WmsCommand.ACTIVE_STATE)
                    && this.ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_HOLD.ToString());

                _releaseButton.Enabled = releaseServiceMenuItem.Enabled = state.Equals(WmsCommand.HOLD_STATE)
                    && this.ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_RELEASE.ToString());

                try
                {
                    WmsSystem wmsSystem = WmsSystem.FindWmsSystem(ConnectionDefn);
                    WmsService wmsService = wmsSystem.FindService(serviceName);
                    if (wmsService.isSystemService)
                    {
                        _stopButton.Enabled = stopServiceMenuItem.Enabled = false;
                        _deleteButton.Enabled = deleteServiceMenuItem.Enabled = false;
                    }
                }
                catch (Exception ex) { }
            }
            else if (_servicesGrid.SelectedRowIndexes.Count > 1)
            {
                _holdButton.Enabled = holdServiceMenuItem.Enabled = ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_HOLD.ToString());
                _releaseButton.Enabled = releaseServiceMenuItem.Enabled = ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_RELEASE.ToString());
                _startButton.Enabled = startServiceMenuItem.Enabled = ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_START.ToString());
                _stopButton.Enabled = stopServiceMenuItem.Enabled = ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STOP.ToString());
                _deleteButton.Enabled = deleteServiceMenuItem.Enabled = ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_DELETE.ToString());
            }

            _holdAllButton.Enabled = holdAllServicesMenuItem.Enabled = this.ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_HOLD.ToString());
            _releaseAllButton.Enabled = releaseAllServicesMenuItem.Enabled = this.ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_RELEASE.ToString());
            _exportButton.Enabled = this.ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STATUS.ToString());
        }

        private void holdButton_Click(object sender, EventArgs e)
        {
            holdService();
        }

        private void _addButton_Click(object sender, EventArgs e)
        {
            addService();
        }

        private void _alterButton_Click(object sender, EventArgs e)
        {
            if (_servicesGrid.SelectedRowIndexes.Count == 1)
            {
                ServiceDetails_Handler(_servicesGrid.SelectedRowIndexes[0]);
            }
        }

        private void _holdAllButton_Click(object sender, EventArgs e)
        {
            holdAllServices();
        }

        private void _releaseAllButton_Click(object sender, EventArgs e)
        {
            releaseAllServices();
        }

        private void _releaseButton_Click(object sender, EventArgs e)
        {
            releaseService();
        }

        private void stopButton_Click(object sender, EventArgs e)
        {
            stopService();
        }

        private void _deleteButton_Click(object sender, EventArgs e)
        {
            deleteService();
        }

        private void _exportButton_Click(object sender, EventArgs e)
        {
            WmsSystem wmsSystem = WmsSystem.FindWmsSystem(_connectionDefinition);
            ExportConfigControl exportConfigControl = new ExportConfigControl(wmsSystem);

            //Place the ExportConfigControl user control into a managed window
            Trafodion.Manager.Framework.Controls.WindowsManager.PutInWindow(exportConfigControl.Size, exportConfigControl, Properties.Resources.ExportConfiiguration + " - " + wmsSystem.ConnectionDefinition.Name, wmsSystem.ConnectionDefinition);
        }

    }
}
