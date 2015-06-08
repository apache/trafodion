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
using System.Collections.Generic;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.SecurityArea.Model;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class RoleSQLPrivilegesUserControl : UserControl
    {
        #region Fields

        private static readonly string _sqlPrivilegesListConfigName = "SQLPrivilegesForRoleWidgetConfig";
        UniversalWidgetConfig _sqlPrivilegesWidgetConfig = null;
        GenericUniversalWidget _sqlPrivilegesWidget = null;
        UserForRoleGridDataHandler _sqlPrivilegesWidgetDisplayHandler = null;
        ConnectionDefinition _connectionDefinition;
        TrafodionIGrid _sqlPrivilegesGrid = null;
        Connection _connection = null;
        bool _isInitialized = false;
        private delegate void HandleEvent(object obj, EventArgs e);  

        #endregion Fields

        #region Properties

        public ConnectionDefinition ConnectionDefn
        {
            get { return _connectionDefinition; }
            set 
            {
                _connectionDefinition = value;
                if ((_sqlPrivilegesWidgetConfig != null) && (_sqlPrivilegesWidgetConfig.DataProviderConfig != null))
                {
                    _sqlPrivilegesWidgetConfig.DataProviderConfig.ConnectionDefinition = value;
                }
            }
        }

        public DataProvider DataProvider
        {
            get;
            set;
        }

        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get { return _sqlPrivilegesWidgetConfig; }
            set { _sqlPrivilegesWidgetConfig = value; }
        }

        public IDataDisplayHandler DataDisplayHandler
        {
            get;
            set;
        }

        public DrillDownManager DrillDownManager
        {
            get;
            set;
        }

        public string WindowTitle
        {
            get { return Properties.Resources.RoleSQLPrivileges; }
        }

        public string RoleName
        {
            get
            {
                if(_roleNameComboBox.SelectedIndex >=0 )
                    return _roleNameComboBox.SelectedItem as string;
                return "";
            }
        }

        #endregion Properties

        #region Constructors

        public RoleSQLPrivilegesUserControl()
        {
            InitializeComponent();
        }

        public RoleSQLPrivilegesUserControl(ConnectionDefinition aConnectionDefinition)
        {
            InitializeComponent();
            _connectionDefinition = aConnectionDefinition;
            //if (aConnectionDefinition.IsServicesUser || aConnectionDefinition.isRoleMGR || aConnectionDefinition.isRoleDBA)
            {
                SystemSecurity systemSecurity = SystemSecurity.FindSystemModel(aConnectionDefinition);
                TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.LookupSQLPrivilegesForRole, systemSecurity, "LoadRoleNames", new Object[0]);
                TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
                progressDialog.ShowDialog();

                if (progressDialog.Error == null && progressDialog.ReturnValue is List<string>)
                {
                    _roleNameComboBox.DataSource = progressDialog.ReturnValue;
                    _roleNameComboBox.SelectedItem = aConnectionDefinition.RoleName;
                    _roleNameComboBox.SelectedIndexChanged += _roleNameComboBox_SelectedIndexChanged;
                    _roleNameComboBox.Enabled = true;
                }
            }
            //else
            //{
            //    _roleNameComboBox.Items.Add(aConnectionDefinition.RoleName);
            //    _roleNameComboBox.Enabled = false;
            //    _roleNameComboBox.SelectedIndex = 0;
            //}
            ShowWidgets();
        }

        public RoleSQLPrivilegesUserControl(ConnectionDefinition aConnectionDefinition, List<string> roleNames, int currentIndex)
        {
            InitializeComponent();
            _connectionDefinition = aConnectionDefinition;
            _roleNameComboBox.DataSource = roleNames;
            _roleNameComboBox.SelectedIndex = -1;
            _roleNameComboBox.SelectedIndex = currentIndex;
            _roleNameComboBox.SelectedIndexChanged += _roleNameComboBox_SelectedIndexChanged;
            _roleNameComboBox.Enabled = (aConnectionDefinition.IsServicesUser);
            ShowWidgets();
        }

        void _roleNameComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            ShowWidgets();
        }

        #endregion Constructors

        /// <summary>
        /// Cleanup
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                //Remove the event handlers
                this.RemoveHandlers();
            }
        }

        private void ShowWidgets()
        {
            if (!_isInitialized)
            {
                _privilegesPanel.Controls.Clear();
                _sqlPrivilegesWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
                _sqlPrivilegesWidgetConfig.Name = WindowTitle;
                _sqlPrivilegesWidgetConfig.Title = WindowTitle;
                _sqlPrivilegesWidgetConfig.ShowProperties = false;
                _sqlPrivilegesWidgetConfig.ShowToolBar = true;
                _sqlPrivilegesWidgetConfig.ShowChart = false;
                _sqlPrivilegesWidgetConfig.ShowTimerSetupButton = false;
                _sqlPrivilegesWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;

                DatabaseDataProviderConfig _dbConfig = _sqlPrivilegesWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;
                _dbConfig.SQLText = Queries.SelectTrafCatPrivilegesByRoleNameQueryText(_connectionDefinition.SystemCatalogName, RoleName, "TRAFODION", 2400);

                _dbConfig.CommandTimeout = 0;

                //_dbConfig.TimerPaused = true;
                _sqlPrivilegesWidgetConfig.DataProviderConfig.ConnectionDefinition = ConnectionDefn;

                _sqlPrivilegesWidget = new GenericUniversalWidget();
                _sqlPrivilegesWidget.DataProvider = new DatabaseDataProvider(_dbConfig);

                //Set the display properties of the widget and add it to the control
                ((TabularDataDisplayControl)_sqlPrivilegesWidget.DataDisplayControl).LineCountFormat = Properties.Resources.RoleSQLPrivileges;
                ((TabularDataDisplayControl)_sqlPrivilegesWidget.DataDisplayControl).ShowExportButtons = true;
                _sqlPrivilegesWidget.UniversalWidgetConfiguration = _sqlPrivilegesWidgetConfig;
                _sqlPrivilegesWidget.Dock = DockStyle.Fill;
                _privilegesPanel.Controls.Add(_sqlPrivilegesWidget);

                _sqlPrivilegesGrid = ((TabularDataDisplayControl)_sqlPrivilegesWidget.DataDisplayControl).DataGrid;
                _sqlPrivilegesGrid.RowSelectionInCellMode = iGRowSelectionInCellModeTypes.MultipleRows;

                //Add event handlers to deal with data provider events
                AddHandlers();

                //Start it.
                _sqlPrivilegesWidget.StartDataProvider();

                _isInitialized = true;
            }
            else
            {
                _sqlPrivilegesWidget.DataProvider.Stop();
                _sqlPrivilegesGrid.UpdateCountControlText(Properties.Resources.RoleSQLPrivileges);

                _sqlPrivilegesGrid.Clear();
                ((DatabaseDataProviderConfig)_sqlPrivilegesWidget.DataProvider.DataProviderConfig).SQLText = Queries.SelectTrafCatPrivilegesByRoleNameQueryText(_connectionDefinition.SystemCatalogName, RoleName, "TRAFODION", 2400);
                _sqlPrivilegesWidgetConfig.DataProviderConfig.ConnectionDefinition = ConnectionDefn;
                _sqlPrivilegesWidget.DataProvider.Start();
            }
        }

        private void AddHandlers()
        {
            if (_sqlPrivilegesWidget != null && _sqlPrivilegesWidget.DataProvider != null)
            {
                //Associate the event handlers
                _sqlPrivilegesWidget.DataProvider.OnErrorEncountered += InvokeHandleError;
                _sqlPrivilegesWidget.DataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                _sqlPrivilegesWidget.DataProvider.OnFetchingData += InvokeHandleFetchingData;
            }
        }

        private void RemoveHandlers()
        {
            if (_sqlPrivilegesWidget != null && _sqlPrivilegesWidget.DataProvider != null)
            {
                //Remove the event handlers
                _sqlPrivilegesWidget.DataProvider.OnErrorEncountered -= InvokeHandleError;
                _sqlPrivilegesWidget.DataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                _sqlPrivilegesWidget.DataProvider.OnFetchingData -= InvokeHandleFetchingData;
            }
        }

        private void InvokeHandleError(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(HandleError), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Security,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        private void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(HandleNewDataArrived), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Security,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        private void InvokeHandleFetchingData(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(HandleFetchingData), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Security,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        // Methods to deal with DataProvider events
        private void HandleError(Object obj, EventArgs e)
        {
        }

        private void HandleNewDataArrived(Object obj, EventArgs e)
        {
        }

        private void HandleFetchingData(Object obj, EventArgs e)
        {
        }
    }
}
