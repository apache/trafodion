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
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.UserManagement.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.UserManagement.Controls
{    
    public partial class AllSQLPrivilegesUserControl : UserControl
    {

        #region Fields

        private static readonly string _sqlPrivilegesListConfigName = "SQLPrivilegesForUserWidgetConfig";
        UniversalWidgetConfig _sqlPrivilegesWidgetConfig = null;
        GenericUniversalWidget _sqlPrivilegesWidget = null;
        ConnectionDefinition _connectionDefinition;
        TrafodionIGrid _sqlPrivilegesGrid = null;
        Connection _connection = null;
        bool _isInitialized = false;
        private WorkMode _currentMode = WorkMode.UserMode;
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
            get { return "SQL Privileges for User"; }
        }

        public string RoleName
        {
            get
            {
                return _userNameTextBox.Text;
            }
        }

        #endregion Properties

        #region Constructors

        public AllSQLPrivilegesUserControl()
        {
            InitializeComponent();
        }

        public AllSQLPrivilegesUserControl(ConnectionDefinition aConnectionDefinition)
        {
            InitializeComponent();
            _connectionDefinition = aConnectionDefinition;
            //if (aConnectionDefinition.IsServicesUser || aConnectionDefinition.isRoleMGR || aConnectionDefinition.isRoleDBA)
            {
                TrafodionSystem aTrafodionSystem = TrafodionSystem.FindTrafodionSystem(aConnectionDefinition);
                TrafodionProgressArgs args = new TrafodionProgressArgs("Getting list of users", aTrafodionSystem, "GetDatabaseUsers", new Object[0]);
                TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
                progressDialog.ShowDialog();

                if (progressDialog.Error == null && progressDialog.ReturnValue is List<string>)
                    _userNameTextBox.Text = aConnectionDefinition.RoleName;
            }

            ShowWidgets();
        }

        public AllSQLPrivilegesUserControl(WorkMode aWorkMode,ConnectionDefinition aConnectionDefinition, List<string> identifierList, int currentIndex)
        {
            InitializeComponent();
            _currentMode = aWorkMode;
            _connectionDefinition = aConnectionDefinition;
            _userNameTextBox.Text = identifierList[currentIndex];
            if (_currentMode == WorkMode.RoleMode)
            {
                _userNameLabel.Text = Properties.Resources.ShowSQLPrivilegesDialogObjectLabelforRole;
            }
            else if (_currentMode == WorkMode.UserMode)
            {
                _userNameLabel.Text = Properties.Resources.ShowSQLPrivilegesDialogObjectLabelforUser;
            }
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
            string currentUserName = _userNameTextBox.Text;
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
                _sqlPrivilegesWidgetConfig.ShowExportButtons = false;
                _sqlPrivilegesWidgetConfig.ShowHelpButton = true;
                _sqlPrivilegesWidgetConfig.HelpTopic = HelpTopics.Show_SQL_Privileges_Dialog_Box;

                DatabaseDataProviderConfig _dbConfig = _sqlPrivilegesWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;
                _dbConfig.SQLText = "select dummy";
                _dbConfig.CommandTimeout = 0;
                _sqlPrivilegesWidgetConfig.DataProviderConfig.ConnectionDefinition = ConnectionDefn;

                _sqlPrivilegesWidget = new GenericUniversalWidget();
                _sqlPrivilegesWidget.DataProvider = new SQLPrivilegesDataProvider(_currentMode,ConnectionDefn, _dbConfig);

                //Set the display properties of the widget and add it to the control
                ((TabularDataDisplayControl)_sqlPrivilegesWidget.DataDisplayControl).LineCountFormat = "SQL Privileges";
                ((TabularDataDisplayControl)_sqlPrivilegesWidget.DataDisplayControl).ShowExportButtons = true;
                _sqlPrivilegesWidget.UniversalWidgetConfiguration = _sqlPrivilegesWidgetConfig;
                _sqlPrivilegesWidget.Dock = DockStyle.Fill;
                _privilegesPanel.Controls.Add(_sqlPrivilegesWidget);

                _sqlPrivilegesGrid = ((TabularDataDisplayControl)_sqlPrivilegesWidget.DataDisplayControl).DataGrid;
                _sqlPrivilegesGrid.RowSelectionInCellMode = iGRowSelectionInCellModeTypes.MultipleRows;
                _sqlPrivilegesGrid.AutoWidthColMode = iGAutoWidthColMode.HeaderAndCells;
                _sqlPrivilegesGrid.AutoResizeCols = true;
                _sqlPrivilegesGrid.Select();
                //Add event handlers to deal with data provider events
                AddHandlers();

                //Start it.
                if (!string.IsNullOrEmpty(currentUserName))
                {
                    Hashtable parameters = new Hashtable();
                    parameters.Add("PRIV_FOR_USER_NAME", currentUserName);
                    _sqlPrivilegesWidget.DataProvider.Start(parameters);
                }

                _isInitialized = true;
            }
            else
            {
                _sqlPrivilegesGrid.Select();
                _sqlPrivilegesWidget.DataProvider.Stop();
                _sqlPrivilegesGrid.UpdateCountControlText("SQL Privileges");
                _sqlPrivilegesGrid.Clear();
                ((DatabaseDataProviderConfig)_sqlPrivilegesWidget.DataProvider.DataProviderConfig).SQLText = "select dummy";
                _sqlPrivilegesWidgetConfig.DataProviderConfig.ConnectionDefinition = ConnectionDefn;
                Hashtable parameters = new Hashtable();
                parameters.Add("PRIV_FOR_USER_NAME", currentUserName);
                _sqlPrivilegesWidget.DataProvider.Start(parameters);
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
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UserManagement,
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
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UserManagement,
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
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UserManagement,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        // Methods to deal with DataProvider events
        private void HandleError(Object obj, EventArgs e)
        {
        }

        private void HandleNewDataArrived(Object obj, EventArgs e)
        {
            if (_sqlPrivilegesGrid != null)
            {
                _sqlPrivilegesGrid.Select();
                _sqlPrivilegesGrid.DoAutoResizeCols();
            }
        }

        private void HandleFetchingData(Object obj, EventArgs e)
        {
        }

        void UpdateControls()
        {

        }

        private void helpButton_Click(object sender, EventArgs e)
        {

        }
    }
}
