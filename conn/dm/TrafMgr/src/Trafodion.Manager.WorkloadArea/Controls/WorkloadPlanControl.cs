//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.DatabaseArea.Controls;
using Trafodion.Manager.DatabaseArea.NCC;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WorkloadPlanControl : UserControl
    {
        string _queryID = null;
        string _startTime = null;
        ConnectionDefinition _connectionDefinition;
        private delegate void HandleEvents(Object obj, DataProviderEventArgs e);
        DatabaseDataProviderConfig _dbConfig;
        WMSOffenderDatabaseDataProvider _offenderDatabaseDataProvider = null;
        QueryPlanDataDisplayControl _queryPlanDataDisplayControl;
        DatabaseDataProvider _explainDataProvider;
        bool _fetchExplain = false;
        string _queryText = "";
        int _fullQueryLength = 0;
        string _dataSource = "";
        readonly Size _childrenWindowSize = new Size(800, 600);
        bool _useQueryIDForExplain = false;
        bool _retryRegularExplain = false;

        public WorkloadPlanControl()
        {
            InitializeComponent();
        }

        public WorkloadPlanControl(ConnectionDefinition aConnectionDefinition, string aQueryID, string startTime, string queryText, int fullQueryLength, bool fetchExplain, string dataSource, bool useQueryIDforExplain)
            :this()
        {
            _connectionDefinition = aConnectionDefinition;
            _queryID = aQueryID;
            _queryIDTextBox.Text = aQueryID;
            _splitContainer.Panel2Collapsed = !fetchExplain;
            _queryText = queryText.Trim();
            _fullQueryLength = fullQueryLength;
            _fetchExplain = fetchExplain;
            _startTime = startTime;
            _sqlTextBox.Text = queryText;
            _toolStripStatusLabel.Visible = false;
            _toolStripProgressBar.Visible = false;
            _dataSource = dataSource;

            if (_fetchExplain)
            {
                btnGenerateMaintainScript.Visible = true;
                btnGenerateMaintainScript.Enabled = false;
            }
            else
            {
                btnGenerateMaintainScript.Visible = false;
            }

            _useQueryIDForExplain = useQueryIDforExplain;

            _dbConfig = (DatabaseDataProviderConfig)WidgetRegistry.GetDefaultDBConfig().DataProviderConfig;
            _dbConfig.TimerPaused = true;
            _dbConfig.RefreshRate = 0;
            _dbConfig.SQLText = "";
            _dbConfig.ConnectionDefinition = _connectionDefinition;

            if (useQueryIDforExplain)
            {
                _splitContainer.Panel1Collapsed = true;
                _planGroupBox.Text = "Explain Plan (From RMS)";
                FetchExplainPlan();
            }
            else
            {
                FetchSQLText();
            }
        }

        void FetchSQLText()
        {
            _theRefreshButton.Enabled = false;
            _dbConfig.ConnectionDefinition = _connectionDefinition;

            if (string.IsNullOrEmpty(_queryText) || (_queryText.Length < _fullQueryLength))
            {
                _toolStripStatusLabel.Text = "Fetching full SQL text...";
                _toolStripStatusLabel.Visible = _toolStripProgressBar.Visible = true;
                _offenderDatabaseDataProvider = new WMSOffenderDatabaseDataProvider(_dbConfig);
                _offenderDatabaseDataProvider.FetchRepositoryDataOption = WMSOffenderDatabaseDataProvider.FetchDataOption.Option_SQLText;
                _offenderDatabaseDataProvider.QueryID = _queryID;
                _offenderDatabaseDataProvider.START_TS = _startTime;
                _offenderDatabaseDataProvider.OnNewDataArrived += InvokeHandleNewSQLTextDataArrived;
                _offenderDatabaseDataProvider.OnErrorEncountered += InvokeHandleSQLTextError;
                _offenderDatabaseDataProvider.Start();
            }
            else
            {
                if (_fetchExplain)
                {
                    FetchExplainPlan();
                }
            }
        }

        void MyDispose(bool disposing)
        {
            if (disposing)
            {
                if (_offenderDatabaseDataProvider != null && _offenderDatabaseDataProvider.FetchInProgress)
                {
                    _offenderDatabaseDataProvider.OnNewDataArrived -= InvokeHandleNewSQLTextDataArrived;
                    _offenderDatabaseDataProvider.OnErrorEncountered -= InvokeHandleSQLTextError;
                    _offenderDatabaseDataProvider.Stop();
                }
                if (_explainDataProvider != null && _explainDataProvider.FetchInProgress)
                {
                    _explainDataProvider.OnNewDataArrived -= this.InvokeHandleNewPlanDataArrived;
                    _explainDataProvider.OnErrorEncountered -= this.InvokeHandlePlanError;
                    _explainDataProvider.Stop();
                }
            }
        }

        #region Handle Full SQL Text

        /// <summary>
        /// The invoker on data provider error
        /// </summary>
        /// <param name="obj"></param>
        /// <param name="e"></param>
        private void InvokeHandleSQLTextError(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvents(SQLTextDataProvider_OnErrorEncountered), new object[] { obj, (DataProviderEventArgs)e });
                }
            }
            catch (Exception ex)
            {
                _sqlTextBox.Text = ex.Message;
            }
        }

        /// <summary>
        /// Event handler for provider error
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void SQLTextDataProvider_OnErrorEncountered(object sender, DataProviderEventArgs e)
        {
            _toolStripStatusLabel.Text = "";
            _toolStripProgressBar.Visible = false;
            _offenderDatabaseDataProvider.OnNewDataArrived -= SQLTextDataProvider_OnNewDataArrived;
            _offenderDatabaseDataProvider.OnErrorEncountered -= SQLTextDataProvider_OnErrorEncountered;
            MessageBox.Show("Failed to get Full SQL text of the query : " + e.Exception.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        /// <summary>
        /// New data arrival invoker
        /// </summary>
        /// <param name="obj"></param>
        /// <param name="e"></param>
        private void InvokeHandleNewSQLTextDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvents(SQLTextDataProvider_OnNewDataArrived), new object[] { obj, (DataProviderEventArgs)e });
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Failed to get Full SQL text of the query : " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// Event handler for new data arrived from the data provider
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void SQLTextDataProvider_OnNewDataArrived(object sender, DataProviderEventArgs e)
        {
            _offenderDatabaseDataProvider.OnNewDataArrived -= SQLTextDataProvider_OnNewDataArrived;
            _offenderDatabaseDataProvider.OnErrorEncountered -= SQLTextDataProvider_OnErrorEncountered;

            if (!string.IsNullOrEmpty(_offenderDatabaseDataProvider.QueryText))
            {
                _sqlTextBox.Text = _offenderDatabaseDataProvider.QueryText + Environment.NewLine;

                if (_fetchExplain && !string.IsNullOrEmpty(_sqlTextBox.Text.Trim()))
                {
                    FetchExplainPlan();
                }
                else
                {
                    _toolStripStatusLabel.Visible = false;
                    _toolStripProgressBar.Visible = false;
                }
            }
            else
            {
                _theRefreshButton.Enabled = true;
                _toolStripStatusLabel.Visible = false;
                _toolStripProgressBar.Visible = false;

                //query text is empty. It is possible that query text has not been yet written by UNC.
                MessageBox.Show("Full SQL text is not yet available in the repository. Please try again using the refresh button", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
        }

        void FetchExplainPlan()
        {
            if (_queryPlanDataDisplayControl == null)
            {
                _queryPlanDataDisplayControl = new QueryPlanDataDisplayControl();
                _queryPlanDataDisplayControl.Dock = DockStyle.Fill;
                _queryPlanPanel.Controls.Add(_queryPlanDataDisplayControl);
            }
            _dbConfig.SQLText = _sqlTextBox.Text;
            if ((_useQueryIDForExplain == true) && (_retryRegularExplain == false))
                _dbConfig.SQLText = string.Format("QID={0}", _queryID);

            if (!string.IsNullOrEmpty(_dataSource))
            {
                _dbConfig.ConnectionDefinition.ConnectedDataSource = null;
                _dbConfig.ConnectionDefinition.ClientDataSource = _dataSource;
            }
            _explainDataProvider = new DatabaseDataProvider(_dbConfig);
            _explainDataProvider.ExplainMode = true;
            _queryPlanDataDisplayControl.DataProvider = _explainDataProvider;
            _toolStripStatusLabel.Visible = _toolStripProgressBar.Visible = true;
            if ((_useQueryIDForExplain == true) && (_retryRegularExplain == false))
                _toolStripStatusLabel.Text = "Fetching Explain Plan from RMS...";
            else
                _toolStripStatusLabel.Text = "Fetching Explain Plan by explicit prepare...";
            _explainDataProvider.OnErrorEncountered += InvokeHandlePlanError;
            _explainDataProvider.OnNewDataArrived += InvokeHandleNewPlanDataArrived;
            _explainDataProvider.Start();
        }

        private void InvokeHandleNewPlanDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvents(ResetStatusStrip), new object[] { obj, (DataProviderEventArgs)e });

                    if (_fetchExplain)
                    {
                    Invoke(new MethodInvoker(() => btnGenerateMaintainScript.Enabled = true));
                }
            }
            }
            catch (Exception ex)
            {
            }
        }

        private void InvokeHandlePlanError(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvents(SQLPlanDataProvider_OnErrorEncountered), new object[] { obj, (DataProviderEventArgs)e });

                    if (_fetchExplain)
                    {
                    Invoke(new MethodInvoker(() => btnGenerateMaintainScript.Enabled = false));
                }
            }
            }
            catch (Exception ex)
            {
                
            }
        }

        void SQLPlanDataProvider_OnErrorEncountered(object sender, DataProviderEventArgs e)
        {
            ResetStatusStrip(sender, e);
            if (_useQueryIDForExplain)
            {
                if (_retryRegularExplain == false)
                {
                    _planGroupBox.Text = "Explain Plan (Explicit Prepare)";
                    _retryRegularExplain = true;
                    _explainDataProvider.OnNewDataArrived += InvokeHandleNewPlanDataArrived;
                    _explainDataProvider.OnErrorEncountered += InvokeHandlePlanError;
                    _splitContainer.Panel1Collapsed = false;
                    FetchSQLText();
                    return;
                }
            }
            if (DialogResult.OK == MessageBox.Show("Failed to get explain plan of the query : " + e.Exception.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error))
            {
                _sqlTextBox.ReadOnly = false;
                _sqlTextBox.BackColor = Color.White;
                _explainDataProvider.OnNewDataArrived += InvokeHandleNewPlanDataArrived;
                _explainDataProvider.OnErrorEncountered += InvokeHandlePlanError;
                _theRefreshButton.Enabled = true;
            }
        }

        /// <summary>
        /// Event handler for new data arrived from the data provider
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void ResetStatusStrip(object sender, DataProviderEventArgs e)
        {
            _toolStripStatusLabel.Text = "";
            _toolStripStatusLabel.Visible = false;
            _toolStripProgressBar.Visible = false;
            _explainDataProvider.OnNewDataArrived -= InvokeHandleNewPlanDataArrived;
            _explainDataProvider.OnErrorEncountered -= InvokeHandlePlanError;
        }

        private List<string> ExtractTables()
        {
            List<string> tableNames = new List<string>();
            ArrayList queryPlanDatas = this._explainDataProvider.WorkbenchQueryData.QueryPlanArray;
            foreach (NCCWorkbenchQueryData.QueryPlanData queryPlanData in queryPlanDatas)
            {
                String tableName = queryPlanData.GetTableName();
                if (tableName != null && tableName.Trim().Length > 0)
                {
                    tableName = tableName.Trim().ToUpper();
                    if (!tableNames.Contains(tableName))
                    {
                        tableNames.Add(tableName);
                    }
                }
            }
            return tableNames;
        }

        #endregion Handle Full SQL Text

        private void _theRefreshButton_Click(object sender, EventArgs e)
        {
            if (!_sqlTextBox.ReadOnly)
            {
                _sqlTextBox.ReadOnly = true;
                _sqlTextBox.BackColor = Color.WhiteSmoke;
            }
            FetchSQLText();
        }

        private void _theHelpButton_Click(object sender, EventArgs e)
        {
            if (this._fetchExplain)
                TrafodionHelpProvider.Instance.ShowHelpTopic(Trafodion.Manager.WorkloadArea.HelpTopics.PlanText);
            else
                TrafodionHelpProvider.Instance.ShowHelpTopic(Trafodion.Manager.WorkloadArea.HelpTopics.SQLText);
        }

        private void btnGenerateMaintainScript_Click(object sender, EventArgs e)
        {
            string windowTitle = string.Format(Properties.Resources.TitleMaintainScript, this._queryID);
            List<string> tableNames = ExtractTables();
            if (tableNames != null && tableNames.Count > 0)
            {
                if (!Utilities.BringExistingWindowToFront(windowTitle, this._connectionDefinition))
                {
                    string fullQueryText = _sqlTextBox.Text.Trim();
                    MaintainScript scriptUserControl = new MaintainScript(this._queryID, tableNames, this._connectionDefinition);
                    Utilities.LaunchManagedWindow(windowTitle, scriptUserControl, this._connectionDefinition, new Size(750, 550), true);
                }
            }
            else
            {
                MessageBox.Show(this.ParentForm, Properties.Resources.MaintainScript_TableNotFound, Properties.Resources.MaintainScript_TableNotFoundCaption, MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }
    }
}
