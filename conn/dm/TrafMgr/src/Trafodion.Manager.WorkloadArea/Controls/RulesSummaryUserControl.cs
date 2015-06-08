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
using System.Data;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class RulesSummaryUserControl : UserControl, ICloneToWindow
    {

        #region Member Variables

        private static readonly string _rulesListConfigName = "WMSRulesSummaryConfig";
        private string _ruleType = "ALL";
        UniversalWidgetConfig _rulesWidgetConfig = null;
        GenericUniversalWidget _rulesWidget = null;
        RulesGridDataHandler _rulesWidgetDisplayHandler = null;
        ConnectionDefinition _connectionDefinition;
        TrafodionIGrid _rulesGrid = null;


        #endregion Member Variables

        #region Public Properties

        //Rule Type can polymorphically use this control to display rule summary/CONN/COMP/EXEC rules
        public string RuleType 
        {
            get { return _ruleType; }
            set { _ruleType = value; }
        }


        public ConnectionDefinition ConnectionDefn 
        {
            get { return _connectionDefinition; }
            set 
            {
                if(_connectionDefinition != null)
                {
                    _rulesWidget.DataProvider.Stop();
                    _rulesGrid.Clear();
                }
                _connectionDefinition = value;

                if (_connectionDefinition != null)
                {
                    _rulesWidgetConfig.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
                    _rulesWidget.StartDataProvider();
                }
            }
        }

        #endregion Public Properties


        #region Constructors/Destructors
        
        public RulesSummaryUserControl()
        {
            InitializeComponent();            
        }

        public RulesSummaryUserControl(ConnectionDefinition aConnectionDefinition, string aruleType): this()
        {
            InitializeWidget();
            RuleType = aruleType;            
            ConnectionDefn = aConnectionDefinition;
        }

        

        #endregion Constrctors/Destructors

        #region ICloneToWindow Members

        /// <summary>
        /// Clones rules user control into a new window
        /// </summary>
        /// <return></return>
        public Control Clone()
        {
            RulesSummaryUserControl clonesRulesSummaryUserControl = new RulesSummaryUserControl(ConnectionDefn, RuleType);
            return clonesRulesSummaryUserControl;
        }
        /// <summary>
        /// Title for the cloned Window
        /// </summary>
        public string WindowTitle
        {
            get { return Properties.Resources.Rules; }
        }        

        #endregion


        #region Private Methods

        private void InitializeWidget()
        {
            //Remove all current contents and add rules summary widget
            _theWidgetPanel.Controls.Clear();
            _rulesWidgetConfig = WidgetRegistry.GetConfigFromPersistence(_rulesListConfigName);
            if(null == _rulesWidgetConfig)
            {
                _rulesWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
                _rulesWidgetConfig.Name = _rulesListConfigName;
                _rulesWidgetConfig.Title = Properties.Resources.Rules;
                _rulesWidgetConfig.ShowProperties = false;
                _rulesWidgetConfig.ShowToolBar = true;
                _rulesWidgetConfig.ShowChart = false;
                _rulesWidgetConfig.ShowTimerSetupButton = false;
                _rulesWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
                _rulesWidgetConfig.ShowHelpButton = true;
                _rulesWidgetConfig.HelpTopic = HelpTopics.RulesSummary;

                DatabaseDataProviderConfig _dbConfig = (DatabaseDataProviderConfig)_rulesWidgetConfig.DataProviderConfig;
                _dbConfig.OpenCommand = "WMSOPEN";
                _dbConfig.SQLText = "STATUS RULE";
                _dbConfig.CloseCommand = "WMSCLOSE";
                _dbConfig.CommandTimeout = 0;

            }

            //if (_rulesWidgetConfig.DataProviderConfig.ColumnMappings == null)
            {
                SetColumnMappings();
            }

            _rulesWidget = new GenericUniversalWidget();
            _rulesWidget.DataProvider = new DatabaseDataProvider((DatabaseDataProviderConfig)_rulesWidgetConfig.DataProviderConfig);

            //Set the display roperties of the widget and add it to the control
            ((TabularDataDisplayControl)_rulesWidget.DataDisplayControl).LineCountFormat = Properties.Resources.Rules;
            _rulesWidgetDisplayHandler = new RulesGridDataHandler(this);
            _rulesWidget.DataDisplayControl.DataDisplayHandler = _rulesWidgetDisplayHandler;
            _rulesWidget.UniversalWidgetConfiguration = _rulesWidgetConfig;
            _rulesWidget.Dock = DockStyle.Fill;
            _theWidgetPanel.Controls.Add(_rulesWidget);

            _rulesGrid = ((TabularDataDisplayControl)_rulesWidget.DataDisplayControl).DataGrid;
            _rulesGrid.DoubleClickHandler = RuleDetails_Handler;
            _rulesGrid.SelectionChanged += new EventHandler(_rulesGrid_SelectionChanged);
            
            addRuleMenuItem.Text = Properties.Resources.AddRule;
            addRuleMenuItem.Click += new EventHandler(addRuleMenuItem_Click);
            _rulesGrid.AddContextMenu(addRuleMenuItem);
            
            alterRuleMenuItem.Text = Properties.Resources.AlterRule;
            alterRuleMenuItem.Click += new EventHandler(alterRuleMenuItem_Click);
            _rulesGrid.AddContextMenu(alterRuleMenuItem);
            
            deleteRuleMenuItem.Text = Properties.Resources.DeleteRule;
            deleteRuleMenuItem.Click += new EventHandler(deleteRuleMenuItem_Click);
            _rulesGrid.AddContextMenu(deleteRuleMenuItem);
            
            associateRuleMenuItem.Text = Properties.Resources.AssociateRule;
            associateRuleMenuItem.Click += new EventHandler(associateRuleMenuItem_Click);
            _rulesGrid.AddContextMenu(associateRuleMenuItem);

            //Disable the export buttons so it does not show up within the universal widget panel
            //But get the export buttons from the grid and add them to their own panel
            ((TabularDataDisplayControl)_rulesWidget.DataDisplayControl).ShowExportButtons = false;

            _rulesGrid.RowMode = true;
            //These two handler are working on context sensitivity of command buttons and context menu items
            _rulesGrid.SelectionChanged += new EventHandler(_rulesGrid_SelectionChanged);
            _rulesGrid.VisibleChanged += new EventHandler(_rulesGrid_VisibleChanged);
        }

        void SetColumnMappings()
        {
            List<ColumnMapping> columnMappings = new List<ColumnMapping>();
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_RULE_TYPE, "Type", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_RULE_NAME, "Name", 128));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_RULE_WARN_LEVEL, "Warn Level", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_RULE_ACTION, "Action", 40));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_COMMENT, "Comment", 128));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_RULE_EXPR, "Expression", 128));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_RULE_OPER, "Logical Operator", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_RULE_AGGR_QUERY_TYPES, "Aggregate Query Type", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_RULE_AGGR_WMS_INTERVAL, "WMS Aggregate Interval", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_RULE_AGGR_REPOS_INTERVAL, "Repository Aggregate Interval", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_RULE_AGGR_EXEC_INTERVAL, "Execution Aggregate Interval", 10));
            columnMappings.Add(new ColumnMapping(WmsCommand.COL_RULE_AGGR_STATS_ONCE, "Collect Stats Once", 10));

            _rulesWidgetConfig.DataProviderConfig.ColumnMappings = columnMappings;
        }

        #endregion Private Methods

        #region RulesGridDataHandler Class

        public class RulesGridDataHandler : TabularDataDisplayHandler
        {
            #region Fields

            private RulesSummaryUserControl _theRulesSummaryUserControl;
            private object _locker;

            #endregion Fields

            #region Constructor

            public RulesGridDataHandler(RulesSummaryUserControl aRulesSummaryUserControl)
            {
                _theRulesSummaryUserControl=aRulesSummaryUserControl;
            }

            #endregion Constructor

            #region Public Methods

            public override void DoPopulate(UniversalWidgetConfig aConfig, DataTable aDataTable, TrafodionIGrid aDataGrid)
            {
                lock (this) 
                {
                    populate(aConfig, aDataTable, aDataGrid);
                }
            }
            #endregion Public Methods

            #region Private Methods

            private void populate(UniversalWidgetConfig aConfig, DataTable aDataTable, TrafodionIGrid aDataGrid)
            {
                if (aDataTable == null) { return; }

                WmsSystem wmsSystem = WmsSystem.FindWmsSystem(_theRulesSummaryUserControl.ConnectionDefn);
                DataTable dataTable = wmsSystem.ConsolidateRowsForRule(aDataTable);

                wmsSystem.WmsRules.Clear();
                wmsSystem.WmsRules = null;
                wmsSystem.ResetConnectionRuleAssociations();

                wmsSystem.LoadRules(dataTable);

                if (dataTable.Columns.Contains(WmsCommand.COL_RULE_TYPE))
                {
                    dataTable.Columns[WmsCommand.COL_RULE_TYPE].SetOrdinal(1);
                }

                if (!_theRulesSummaryUserControl.RuleType.Equals(WmsCommand.CONN_RULE_TYPE))
                {
                    if (dataTable.Columns.Contains(WmsCommand.COL_RULE_AGGR_QUERY_TYPES))
                    {
                        dataTable.Columns.Remove(WmsCommand.COL_RULE_AGGR_QUERY_TYPES);
                    }
                    if (dataTable.Columns.Contains(WmsCommand.COL_RULE_AGGR_EXEC_INTERVAL))
                    {
                        dataTable.Columns.Remove(WmsCommand.COL_RULE_AGGR_EXEC_INTERVAL);
                    }
                    if (dataTable.Columns.Contains(WmsCommand.COL_RULE_AGGR_REPOS_INTERVAL))
                    {
                        dataTable.Columns.Remove(WmsCommand.COL_RULE_AGGR_REPOS_INTERVAL);
                    }
                    if (dataTable.Columns.Contains(WmsCommand.COL_RULE_AGGR_WMS_INTERVAL))
                    {
                        dataTable.Columns.Remove(WmsCommand.COL_RULE_AGGR_WMS_INTERVAL);
                    }
                    if (dataTable.Columns.Contains(WmsCommand.COL_RULE_AGGR_STATS_ONCE))
                    {
                        dataTable.Columns.Remove(WmsCommand.COL_RULE_AGGR_STATS_ONCE);
                    }
                }

                //If rule type is not 'ALL', filter the rows and display only those rows that match the type
                if (!_theRulesSummaryUserControl.RuleType.Equals("ALL"))
                {
                    dataTable = _theRulesSummaryUserControl.FilterRulesByType(dataTable);
                }

                base.DoPopulate(aConfig, dataTable, aDataGrid);

                string gridHeaderText = string.Format(Properties.Resources.RulesSummaryHeader, dataTable.Rows.Count, WmsCommand.GetDisplayRuleType(_theRulesSummaryUserControl.RuleType).ToLower());          

                aDataGrid.UpdateCountControlText(gridHeaderText);

                if (dataTable.Rows.Count > 0)
                {
                    aDataGrid.ResizeGridColumns(dataTable, 7, 60);
                }

                _theRulesSummaryUserControl.UpdateControls();
            }

            #endregion Private Methods
        }

        #endregion RulesGridDataHandler Class

        void UpdateControls()
        {            
            WmsSystem wmsSystem = WmsSystem.FindWmsSystem(_connectionDefinition);
            _addButton.Enabled = addRuleMenuItem.Enabled = wmsSystem.IsRulesLoaded
                && wmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_ADD.ToString());

            _alterButton.Enabled = alterRuleMenuItem.Enabled = (_rulesGrid.SelectedRowIndexes.Count == 1)
                && wmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_ALTER.ToString());

            _associateButton.Enabled = associateRuleMenuItem.Enabled =
                wmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_ALTER.ToString());

            if (_rulesGrid.SelectedRowIndexes.Count == 0)
            {
                _deleteButton.Enabled = deleteRuleMenuItem.Enabled = false;
            }            
            else
            { 
                //If selected rules contain one user defined rule, we enable delete button
                bool containUserRule=false;
                for (int i = 0; i < _rulesGrid.SelectedRowIndexes.Count;i++ )
                {
                    string ruleName = _rulesGrid.Rows[_rulesGrid.SelectedRowIndexes[i]].Cells[WmsCommand.COL_RULE_NAME].Value as string;                    
                    WmsRule wmsRule = wmsSystem.FindRule(ruleName);
                    if(!wmsRule.isSystemRule)
                    {
                        containUserRule = true;
                        break;
                    }
                }
                _deleteButton.Enabled = deleteRuleMenuItem.Enabled = containUserRule
                    && wmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_DELETE.ToString());
            }
            
        }

        delegate void cmdButtonContextSenseDelegate();
        private void initButtonMenuItemContextStatus()
        {
            if (this.InvokeRequired)
            {
                this.Invoke(new cmdButtonContextSenseDelegate(initButtonMenuItemContextStatus));
                return;
            }

            _addButton.Enabled = addRuleMenuItem.Enabled = false;
            _associateButton.Enabled = associateRuleMenuItem.Enabled = false;
            _alterButton.Enabled = alterRuleMenuItem.Enabled = false;
            _deleteButton.Enabled = deleteRuleMenuItem.Enabled = false;
        }

        public DataTable FilterRulesByType(DataTable dataTable)
        {
            DataTable ruleTable = dataTable.Clone();
            DataRow[] rows = dataTable.Select(string.Format("{0} = '{1}'", WmsCommand.COL_RULE_TYPE, _ruleType));
            foreach (DataRow row in rows)
            {
                ruleTable.Rows.Add(row.ItemArray);
            }
            return ruleTable;
        }

        private void MyDispose(bool disposing) 
        {
            
        }

        internal void RuleDetails_Handler(int rowIndex) 
        {
            string ruleName = this._rulesGrid.Rows[rowIndex].Cells[WmsCommand.COL_RULE_NAME].Value.ToString();

            try 
            {
                WmsSystem wmsSystem = WmsSystem.FindWmsSystem(ConnectionDefn);
                WmsRule wmsRule = wmsSystem.FindRule(ruleName);
                if (wmsRule != null)
                {
                    WMSAlterRuleDialog alterRuleDialog = new WMSAlterRuleDialog(wmsRule);
                    if (alterRuleDialog.ShowDialog() == DialogResult.OK)
                    {
                        _rulesWidget.DataProvider.Start();
                    }
                }
            }
            catch(Exception ex)
            {
                if (Logger.IsTracingEnabled)
                    Logger.OutputToLog("Warning -- Unable to get the rule model. Details = " + ex.Message);
            }
        }

        internal void addRuleMenuItem_Click(object sender, EventArgs e)
        {
            _addButton.PerformClick();
        }
        
        internal void alterRuleMenuItem_Click(object sender, EventArgs e) 
        {
            _alterButton.PerformClick();
        }

        internal void deleteRuleMenuItem_Click(object sender, EventArgs e)
        {
            if (_rulesGrid.SelectedRowIndexes.Count > 0)
            {
                _deleteButton.PerformClick();
            }
        }

        internal void associateRuleMenuItem_Click(object sender, EventArgs e)
        {
            _associateButton.PerformClick();
        }        

        internal void _rulesGrid_SelectionChanged(object sender, EventArgs e) 
        {
            UpdateControls();
        }

        internal void _rulesGrid_VisibleChanged(object sender, EventArgs e)
        {
            initButtonMenuItemContextStatus();
        }

        private void _associateButton_Click(object sender, EventArgs e)
        {
            WmsSystem wmsSystem = WmsSystem.FindWmsSystem(ConnectionDefn);
            WmsRule wmsRule = null;
            string ruleType = _ruleType;

            if (_rulesGrid.SelectedRowIndexes.Count > 0)
            {
                string ruleName = this._rulesGrid.Rows[_rulesGrid.SelectedRowIndexes[0]].Cells[WmsCommand.COL_RULE_NAME].Value.ToString();
                 wmsRule = wmsSystem.FindRule(ruleName);
                 ruleType = wmsRule.RuleType;
            }

            AssociateRuleDialog ard = new AssociateRuleDialog(wmsSystem, ruleType, wmsRule);
            ard.ShowDialog();
        }

        private void _addButton_Click(object sender, EventArgs e)
        {
            WmsSystem wmsSystem = WmsSystem.FindWmsSystem(ConnectionDefn);
            AddRuleDialog addDialog = new AddRuleDialog(wmsSystem, _ruleType);

            if (addDialog.ShowDialog() == DialogResult.OK)
            {
                _rulesWidget.DataProvider.RefreshData();
            }

        }

        private void _alterButton_Click(object sender, EventArgs e)
        {
            if (_rulesGrid.SelectedRowIndexes.Count == 1)
            {
                RuleDetails_Handler(_rulesGrid.SelectedRowIndexes[0]);
            }
        }

        private void _deleteButton_Click(object sender, EventArgs e)
        {
            bool isDefaultRulesSkipped = false;
            string defaultRuleSkipped = "";
            bool rulesDeleted = false;

            List<WmsRule> rulesList = new List<WmsRule>();
            WmsSystem wmsSystem = WmsSystem.FindWmsSystem(_connectionDefinition);

            foreach (int rowIndex in _rulesGrid.SelectedRowIndexes)
            {
                string ruleName = this._rulesGrid.Rows[rowIndex].Cells[WmsCommand.COL_RULE_NAME].Value.ToString();
                WmsRule wmsRule = wmsSystem.FindRule(ruleName);
                if (wmsRule != null)
                {
                    if (wmsRule.isSystemRule)
                    {
                        isDefaultRulesSkipped = true;
                    }
                    else
                    {
                        rulesList.Add(wmsRule);
                    }
                }
            }

            if (rulesList.Count == 0)
            {
                //The only reason that the list could be empty is because default rules was the only selected rules
                //and we ignored it.
                if (isDefaultRulesSkipped)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.SystemRuleDeleteInfoMessage,
                                                        Properties.Resources.DeleteRule, MessageBoxButtons.OK, MessageBoxIcon.Information);
                    return;

                }
            }

            //If we ignored default rules, let the user know
            if (isDefaultRulesSkipped)
            {
                defaultRuleSkipped = Properties.Resources.SystemRuleDeleteIgnoreMessage + "\n";
            }


            StringBuilder skipList = new StringBuilder();
            int nRules = 0;
            foreach (WmsRule wmsRule in rulesList)
            {
                nRules++;
                if (20 >= nRules)
                    skipList.AppendLine(wmsRule.Name);
            }

            if (20 < nRules)
                skipList.Append("\n     ...  and " + (nRules - 20) + " more ... ");

            DialogResult result = MessageBox.Show(Utilities.GetForegroundControl(), defaultRuleSkipped +
                                                Properties.Resources.DeleteRuleConfirmationMessage + "\n" +
                                                skipList,
                                                Properties.Resources.DeleteRule, MessageBoxButtons.YesNo, MessageBoxIcon.Question);

            if (result == DialogResult.Yes)
            {
                StringBuilder errInfo_sb = new StringBuilder();
                this.Cursor = Cursors.WaitCursor;
                DataTable errorTable = new DataTable();
                errorTable.Columns.Add("Rule Name");
                errorTable.Columns.Add("Error Description");

                foreach (WmsRule wmsRule in rulesList)
                {
                    try
                    {
                        wmsRule.Delete();
                        rulesDeleted = true;
                    }
                    catch (Exception exc)
                    {
                        errorTable.Rows.Add(new string[] { wmsRule.Name, exc.Message });
                    }

                }
                this.Cursor = Cursors.Default;

                if (errorTable.Rows.Count > 0)
                {
                    TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog(Properties.Resources.DeleteRuleFailureHeader, errorTable, System.Drawing.SystemIcons.Error);
                    mmd.ShowDialog();
                }
            }

            if (rulesDeleted)
            {
                _rulesWidget.StartDataProvider();
            }
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
