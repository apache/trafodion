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
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.WorkloadArea.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSAlterRuleUserControl : UserControl
    {
        #region Members
        private Form m_parent = null;
        private WmsSystem _wmsSystem = null;
        private WmsRule _wmsRule = null;
        
        private const string STATS_IDLE_MINUTES = "STATS_IDLE_MINUTES";
        private const string NO_ACTION = "NO-ACTION";

        private string[] m_warnLevels = { "WARN-HIGH", "WARN-MEDIUM", "WARN-LOW", "NO-WARN" };
        private string[] m_connWarnLevels = { "WARN-LOW", "NO-WARN" };
        private string[] m_logicalOperators = { "AND", "OR" };
        private string[] m_connAction = { "SQL_CMD" };
        private string[] m_compAction = { "REJECT", "HOLD", "SQL_CMD" };

        private string[] m_execAction = { "CANCEL" };

        private string[] m_connProperty = { "SESSION", "LOGIN", "APPL", "DSN" };
        private string[] m_compProperty =  { "EST_TOTAL_MEMORY", "EST_TOTAL_TIME", "EST_CARDINALITY", "EST_ACCESSED_ROWS", "EST_USED_ROWS", 
											"NUM_JOINS", "SCAN_SIZE", "UPDATE_STATS_WARNING", "CROSS_PRODUCT"};
        private string[] m_execProperty = { "USED_ROWS", "ACCESSED_ROWS", "TOTAL_MEM_ALLOC", "ELAPSED_TIME", "CPU_TIME", STATS_IDLE_MINUTES };
        private string[] m_connOperator = { "" };
        private string[] m_compOperator = { "", "=", ">=", ">", "<", "<=", "<>" };
        private string[] m_execOperator = { "=", ">=", ">", "<", "<=", "<>", "%" };
        private string[] m_nonExecValue = { "" };
        private string[] m_execValue = { "", "EST_USED_ROWS", "EST_ACCESSED_ROWS", "EST_TOTAL_MEMORY", "EST_CPU_TIME" };
        private List<string> originallyAssociatedServiceNames = new List<string>();
        private EventHandler<WmsObject.WmsModelEventArgs> ruleAssociationChangeListener = null;
        private WmsObjectsIGrid<WmsService> associatedServicesIGrid = null;

        #endregion

        public WMSAlterRuleUserControl(Form parent, WmsRule wmsRule)
        {
            InitializeComponent();
            m_parent = parent;
            _wmsSystem = wmsRule.WmsSystem;
            _wmsRule = wmsRule;

            //Listen to rule association changes done outside of this control, so ui can refresh accordingly.
            ruleAssociationChangeListener = new EventHandler<WmsObject.WmsModelEventArgs>(_wmsRule_WmsModelEvent);
            _wmsRule.WmsModelEvent += ruleAssociationChangeListener;

            associatedServicesIGrid = new WmsObjectsIGrid<WmsService>();
            setUpServicesGrid();
            TrafodionGroupBox1.Controls.Add(associatedServicesIGrid);

            PopulateAssociateServicesGrid();

            SetBackwardCompatibility();

            initializeControls();
            this.Load += new EventHandler(handleInfoChanged);
            this.ruleNameTextBox.TextChanged += new EventHandler(handleInfoChanged);
            this.ruleTypeConnRadioButton.Click += new EventHandler(handleInfoChanged);
            this.ruleTypeCompRadioButton.Click += new EventHandler(handleInfoChanged);
            this.ruleTypeExecRadioButton.Click += new EventHandler(handleInfoChanged);
            this.warnLevelComboBox.TextChanged += new EventHandler(handleInfoChanged);
            this.actionComboBox.TextChanged += new EventHandler(handleInfoChanged);

            this.sqlStringTextBox.TextChanged += new EventHandler(handleInfoChanged);
            this.expressionTextBox.TextChanged += new EventHandler(handleInfoChanged);
            this._commentTextBox.TextChanged += new EventHandler(handleInfoChanged);

            this.typeCheckedListBox.SelectedIndexChanged += new EventHandler(typeCheckedListBox_SelectedIndexChanged);
            this.checkAllLinkLabel.Click += new EventHandler(checkAllLinkLabel_Click);
            this.uncheckAllLinkLabel.Click += new EventHandler(uncheckAllLinkLabel_Click);
            this.wmsIntervalNumericUpDown.ValueChanged += new EventHandler(wmsIntervalNumericUpDown_ValueChanged);
            this.repositoryIntervalNumericUpDown.ValueChanged += new EventHandler(repositoryIntervalNumericUpDown_ValueChanged);
            this.executionIntervalNumericUpDown.ValueChanged += new EventHandler(executionIntervalNumericUpDown_ValueChanged);

            this.typeLinkLabel.MouseHover += new EventHandler(typeLinkLabel_MouseHover);
            this.intervalLinkLabel.MouseHover += new EventHandler(intervalLinkLabel_MouseHover);

        }

        void intervalLinkLabel_MouseHover(object sender, EventArgs e)
        {
            string caption =
            "WMS time interval:\r\n" +
            "\tSpecifies the interval, in minutes, that the NDCS process updates the WMS process after aggregation has started.\r\n" +
            "\tRange is 1 to 5 minutes. Default is 1 minute.\r\n" +
            "Repository time interval:\r\n" +
            "\tSpecifies the interval, in minutes, before the aggregated statistics are written to the Repository by the NDCS process.\r\n" +
            "\tRange is 1 to 5 minutes. Default is 1 minute.\r\n" +
            "Execute time interval:\r\n" +
            "\tSpecifies the expected execution time of a query, in minutes. If the query's execution time exceeds this interval, its statistics are written to the Repository without being aggregated.\r\n" +
            "\tIf the query's execution time is less than or equal to this interval, its statistics are aggregated.\r\n" +
            "\tRange is 1 to 5 minutes. Default is 1 minute.\r\n";
            this.toolTip1.IsBalloon = true;
            this.toolTip1.AutoPopDelay = 60 * 1000;
            this.toolTip1.SetToolTip(intervalLinkLabel, caption);
        }

        void typeLinkLabel_MouseHover(object sender, EventArgs e)
        {
            string caption =
                "Specify the SQL query types to be included in the aggregation:\r\n" +
                "\tINSERT (SQL_INSERT_UNIQUE/NON_UNIQUE)\r\n" +
                "\tUPDATE (SQL_UPDATE_UNIQUE/NON_UNIQUE)\r\n" +
                "\tDELETE (SQL_DELETE_UNIQUE/NON_UNIQUE)\r\n" +
                "\tSELECT (SQL_SELECT_UNIQUE)\r\n";
            this.toolTip1.AutoPopDelay = 60 * 1000;
            this.toolTip1.SetToolTip(typeLinkLabel, caption);
        }

        bool atLeastOneAggregateType()
        {
            for (int i = 0; i < this.typeCheckedListBox.Items.Count; i++)
            {
                if (this.typeCheckedListBox.GetItemChecked(i))
                    return true;
            }

            return false;
        }

        void checkEnableAggregate()
        {
            if (this.ruleTypeConnRadioButton.Checked)
            {
                if (typeCheckedListBox.CheckedItems.Count == 0)
                {
                    wmsIntervalNumericUpDown.Value = 1;
                    repositoryIntervalNumericUpDown.Value = 1;
                    executionIntervalNumericUpDown.Value = 1;
                    intervalGroupBox.Enabled = false;
                    _statsCheckBox.Checked = false;
                    _statisticsGroupBox.Enabled = false;
                }
                else
                {
                    _statisticsGroupBox.Enabled = true; 
                    intervalGroupBox.Enabled = true;
                }
            }
            _statisticsGroupBox.Visible = (_wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120);
        }

        void repositoryIntervalNumericUpDown_ValueChanged(object sender, EventArgs e)
        {
            previewTextBox.Text = getCommandPreview();
        }

        void wmsIntervalNumericUpDown_ValueChanged(object sender, EventArgs e)
        {
            previewTextBox.Text = getCommandPreview();
        }

        void executionIntervalNumericUpDown_ValueChanged(object sender, EventArgs e)
        {
            previewTextBox.Text = getCommandPreview();
        }

        void uncheckAllLinkLabel_Click(object sender, EventArgs e)
        {
            for (int i = 0; i < this.typeCheckedListBox.Items.Count; i++)
            {
                this.typeCheckedListBox.SetItemChecked(i, false);
            }
            wmsIntervalNumericUpDown.Value = 1;
            repositoryIntervalNumericUpDown.Value = 1;
            executionIntervalNumericUpDown.Value = 1;
            previewTextBox.Text = getCommandPreview();
            checkEnableAggregate();
        }

        void checkAllLinkLabel_Click(object sender, EventArgs e)
        {
            for (int i = 0; i < this.typeCheckedListBox.Items.Count; i++)
            {
                this.typeCheckedListBox.SetItemChecked(i, true);
            }
            previewTextBox.Text = getCommandPreview();
            checkEnableAggregate();
        }

        void typeCheckedListBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            previewTextBox.Text = getCommandPreview();
            checkEnableAggregate();
        }

        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                _wmsRule.WmsModelEvent -= ruleAssociationChangeListener;
            }
        }

        void _wmsRule_WmsModelEvent(object sender, WmsObject.WmsModelEventArgs e)
        {
            if (e.WmsAction == WmsCommand.WMS_ACTION.RULE_ASSOC_CHANGED)
            {
                PopulateAssociateServicesGrid();
            }
            else if (WmsCommand.WMS_ACTION.STATUS_RULE == e.WmsAction)
            {
                initializeControls();
                PopulateAssociateServicesGrid();
            }

        }

        private void setUpServicesGrid()
        {
            TenTec.Windows.iGridLib.iGCellStyleDesign iGCellStyleCheckBox;
            TenTec.Windows.iGridLib.iGCellStyle associatedServicesIGridCol4CellStyle;
            TenTec.Windows.iGridLib.iGColHdrStyle associatedServicesIGridCol4ColHdrStyle;
            TenTec.Windows.iGridLib.iGCellStyle associatedServicesIGridCol5CellStyle;
            TenTec.Windows.iGridLib.iGColHdrStyle associatedServicesIGridCol5ColHdrStyle;
            TenTec.Windows.iGridLib.iGCellStyle associatedServicesIGridCol6CellStyle;
            TenTec.Windows.iGridLib.iGColHdrStyle associatedServicesIGridCol6ColHdrStyle;
            TenTec.Windows.iGridLib.iGColPattern iGColPattern1 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern2 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern3 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern4 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern5 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern6 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern7 = new TenTec.Windows.iGridLib.iGColPattern();
            iGCellStyleCheckBox = new TenTec.Windows.iGridLib.iGCellStyleDesign();
            associatedServicesIGridCol4CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            associatedServicesIGridCol4ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            associatedServicesIGridCol5CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            associatedServicesIGridCol5ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            associatedServicesIGridCol6CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            associatedServicesIGridCol6ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            // 
            // iGCellStyleCheckBox
            // 
            iGCellStyleCheckBox.ContentIndent = new TenTec.Windows.iGridLib.iGIndent(1, 1, 1, 1);
            iGCellStyleCheckBox.EmptyStringAs = TenTec.Windows.iGridLib.iGEmptyStringAs.Null;
            iGCellStyleCheckBox.Enabled = TenTec.Windows.iGridLib.iGBool.True;
            iGCellStyleCheckBox.Flags = ((TenTec.Windows.iGridLib.iGCellFlags)((TenTec.Windows.iGridLib.iGCellFlags.DisplayText | TenTec.Windows.iGridLib.iGCellFlags.DisplayImage)));
            iGCellStyleCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            iGCellStyleCheckBox.ForeColor = System.Drawing.Color.Black;
            iGCellStyleCheckBox.ImageAlign = TenTec.Windows.iGridLib.iGContentAlignment.TopCenter;
            iGCellStyleCheckBox.ReadOnly = TenTec.Windows.iGridLib.iGBool.False;
            iGCellStyleCheckBox.Selectable = TenTec.Windows.iGridLib.iGBool.True;
            iGCellStyleCheckBox.SingleClickEdit = TenTec.Windows.iGridLib.iGBool.True;
            iGCellStyleCheckBox.TextAlign = TenTec.Windows.iGridLib.iGContentAlignment.TopLeft;
            iGCellStyleCheckBox.TextPosToImage = TenTec.Windows.iGridLib.iGTextPosToImage.Horizontally;
            iGCellStyleCheckBox.TextTrimming = TenTec.Windows.iGridLib.iGStringTrimming.EllipsisCharacter;
            iGCellStyleCheckBox.Type = TenTec.Windows.iGridLib.iGCellType.Check;

            this.associatedServicesIGrid.AllowWordWrap = false;
            this.associatedServicesIGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            iGColPattern1.AllowGrouping = false;
            iGColPattern1.AllowMoving = false;
            iGColPattern1.CellStyle = iGCellStyleCheckBox;
            iGColPattern1.DefaultCellImageIndex = 1;
            iGColPattern1.Key = "SelectCheckBox";
            iGColPattern1.MaxWidth = 20;
            iGColPattern1.MinWidth = 20;
            iGColPattern1.SortOrder = TenTec.Windows.iGridLib.iGSortOrder.None;
            iGColPattern1.SortType = TenTec.Windows.iGridLib.iGSortType.ByAuxValue;
            iGColPattern1.Text = "+";
            iGColPattern1.Width = 20;
            iGColPattern2.CustomGrouping = true;
            iGColPattern2.Key = "ServiceName";
            iGColPattern2.Text = "Service Name";
            iGColPattern2.Width = 178;
            iGColPattern3.Key = "State";
            iGColPattern3.Text = "State";
            iGColPattern3.Width = 78;
            iGColPattern4.Key = "Priority";
            iGColPattern4.SortType = TenTec.Windows.iGridLib.iGSortType.ByText;
            iGColPattern4.Text = "Priority";
            iGColPattern4.Width = 82;
            iGColPattern5.CellStyle = associatedServicesIGridCol4CellStyle;
            iGColPattern5.ColHdrStyle = associatedServicesIGridCol4ColHdrStyle;
            iGColPattern5.Key = "MaxCPUBusy";
            iGColPattern5.Text = "Max CPU Busy";
            iGColPattern5.Width = 92;
            iGColPattern6.CellStyle = associatedServicesIGridCol5CellStyle;
            iGColPattern6.ColHdrStyle = associatedServicesIGridCol5ColHdrStyle;
            iGColPattern6.Key = "MaxMemUsage";
            iGColPattern6.Text = "Max Memory Usage";
            iGColPattern6.Width = 122;
            iGColPattern7.CellStyle = associatedServicesIGridCol6CellStyle;
            iGColPattern7.ColHdrStyle = associatedServicesIGridCol6ColHdrStyle;
            iGColPattern7.Key = "MaxRowsFetched";
            iGColPattern7.Text = "Max Rows Fetched";
            iGColPattern7.Width = 137;
            this.associatedServicesIGrid.Cols.AddRange(new TenTec.Windows.iGridLib.iGColPattern[] {
            iGColPattern1,
            iGColPattern2,
            iGColPattern3,
            iGColPattern4,
            iGColPattern5,
            iGColPattern6,
            iGColPattern7});
            this.associatedServicesIGrid.CurrentFilter = null;
            this.associatedServicesIGrid.DefaultRow.Height = 18;
            this.associatedServicesIGrid.DefaultRow.NormalCellHeight = 18;
            this.associatedServicesIGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.associatedServicesIGrid.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.associatedServicesIGrid.ForeColor = System.Drawing.SystemColors.ControlText;
            associatedServicesIGrid.Header.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.associatedServicesIGrid.Header.Height = 20;
            this.associatedServicesIGrid.Location = new System.Drawing.Point(4, 19);
            this.associatedServicesIGrid.Margin = new System.Windows.Forms.Padding(3, 5, 3, 5);
            this.associatedServicesIGrid.Name = "associatedServicesIGrid";
            this.associatedServicesIGrid.RowMode = true;
            this.associatedServicesIGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this.associatedServicesIGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this.associatedServicesIGrid.SearchAsType.SearchCol = null;
            this.associatedServicesIGrid.Size = new System.Drawing.Size(728, 88);
            this.associatedServicesIGrid.TabIndex = 3;
            this.associatedServicesIGrid.TreeCol = null;
            this.associatedServicesIGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.associatedServicesIGrid.WordWrap = false;
            this.associatedServicesIGrid.CellClick += new TenTec.Windows.iGridLib.iGCellClickEventHandler(this.associatedServicesIGrid_CellClick);
        }

        private void PopulateAssociateServicesGrid()
        {
            originallyAssociatedServiceNames.Clear();
            associatedServicesIGrid.Rows.Clear();

            foreach (WmsService theService in _wmsRule.AssociatedServices)
            {
                addServiceToGrid(theService, true);
                originallyAssociatedServiceNames.Add(theService.Name);
            }

            foreach (WmsService aService in _wmsSystem.WmsServices)
            {
                if (!this.originallyAssociatedServiceNames.Contains(aService.Name))
                {
                    addServiceToGrid(aService, false);
                }
            }
        }

        private void addServiceToGrid(WmsService aService, bool isAssociated)
        {
            iGRow row = associatedServicesIGrid.Rows.Add();
            if (isAssociated)
            {
                row.Cells["SelectCheckBox"].AuxValue = 1;
                row.Cells["SelectCheckBox"].Value = true;

            }
            else
            {
                row.Cells["SelectCheckBox"].AuxValue = 0;
            }
            row.Cells["ServiceName"].Value = aService.Name;
            row.Cells["State"].Value = aService.State;
            row.Cells["Priority"].Value = aService.Priority;
            row.Cells["MaxCPUBusy"].Value = aService.MaxCpuBusy;
            row.Cells["MaxMemUsage"].Value = aService.MaxMemUsage;
            row.Cells["MaxRowsFetched"].Value = aService.MaxRowsFetched;
        }

        public Button CancelButton
        {
            get { return this.cancelButton; }
        }

        public Button OKButton 
        {
            get { return this.okButton; }
        }

        void handleInfoChanged(object sender, EventArgs e)
        {
            this.aggregateGroupBox.Enabled = this.ruleTypeConnRadioButton.Checked;
            checkEnableAggregate();

            previewTextBox.Text = getCommandPreview();
        }

        private bool isOldR24SUT()
        {
            // RR:  Wait for Anan/Matt gives a new API call -- we could use a hardcoded 2.4 version for now.
            //String serverVersion = this._wmsSystem.CurrentWorkspace.TrafodionConnection.ServerVersion;
            String serverVersion = this._wmsSystem.ConnectionDefinition.OdbcServerVersion;
            return ConnectionDefinition.isServerVersionOnOldR24SUT(serverVersion);
        }

        private void addRuleNoActionIfApplicable()
        {
            if (isOldR24SUT())
                return;

            this.actionComboBox.Items.Add(NO_ACTION);
        }



        /// <summary>
        /// Backward compatibility supprot
        /// </summary>
        /// <param name="serverVersion"></param>
        private void SetBackwardCompatibility()
        {
            // Remove the new STATS_IDLE_MINUTES for versions earlier than M9
            if (this._wmsSystem.ConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ140)
            {
                m_execProperty = Array.FindAll<String>(m_execProperty, stringValue => 0 != string.Compare(stringValue, STATS_IDLE_MINUTES, true));
            }
        }

        private void initializeControls()
        {
            this.ruleNameTextBox.Text = _wmsRule.Name;

            this.warnLevelComboBox.Items.Clear();
            this.actionComboBox.Items.Clear();
            this.propertyComboBox.Items.Clear();
            this.logicalOperatorComboBox.Items.Clear();
            this.operatorComboBox.Items.Clear();
            this.valueComboBox.Items.Clear();
            this.sqlStringTextBox.Text = "";
            this.expressionTextBox.Text = "";

            if (this._wmsRule.RuleType.Equals(WmsCommand.CONN_RULE_TYPE))
            {
                this.warnLevelComboBox.Items.AddRange(m_connWarnLevels);
                this.ruleTypeConnRadioButton.Checked = true;
                this.logicalOperatorComboBox.Enabled = false;
                this.actionComboBox.Items.AddRange(m_connAction);
                this.propertyComboBox.Items.AddRange(m_connProperty);
                this.operatorComboBox.Items.AddRange(m_connOperator);
                this.operatorComboBox.Enabled = false;
                this.valueComboBox.Items.AddRange(m_nonExecValue);
                this.upperCheckBox.Enabled = true;
            }
            else if (this._wmsRule.RuleType.Equals(WmsCommand.COMP_RULE_TYPE))
            {
                this.warnLevelComboBox.Items.AddRange(m_warnLevels);
                this.ruleTypeCompRadioButton.Checked = true;
                this.logicalOperatorComboBox.Enabled = true;
                this.actionComboBox.Items.AddRange(m_compAction);

                this.propertyComboBox.Items.AddRange(m_compProperty);
                this.operatorComboBox.Items.AddRange(m_compOperator);
                this.operatorComboBox.Enabled = true;
                this.valueComboBox.Items.AddRange(m_nonExecValue);
                this.upperCheckBox.Enabled = false;
            }
            else if (this._wmsRule.RuleType.Equals(WmsCommand.EXEC_RULE_TYPE))
            {
                this.warnLevelComboBox.Items.AddRange(m_warnLevels);
                this.ruleTypeExecRadioButton.Checked = true;
                this.logicalOperatorComboBox.Enabled = true;
                this.actionComboBox.Items.AddRange(m_execAction);
                if (_wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
                {
                    this.actionComboBox.Items.AddRange(new string[] { "STATS_PERTABLE", "STATS_PERTABLE_CANCEL" });
                }
                this.propertyComboBox.Items.AddRange(m_execProperty);
                this.operatorComboBox.Items.AddRange(m_execOperator);
                this.operatorComboBox.Enabled = true;
                this.valueComboBox.Items.AddRange(m_execValue);
                this.upperCheckBox.Enabled = false;
            }

            addRuleNoActionIfApplicable();
            this.logicalOperatorComboBox.Items.AddRange(m_logicalOperators);

            this.logicalOperatorComboBox.SelectedItem = _wmsRule.Operator;
            this.propertyComboBox.SelectedIndex = 0;
            this.operatorComboBox.SelectedIndex = 0;
            this.valueComboBox.SelectedIndex = 0;

            this.warnLevelComboBox.SelectedItem = _wmsRule.WarnLevel;

            this.sqlStringTextBox.Enabled = false;
            if (!String.IsNullOrEmpty(_wmsRule.ActionType))
            {
                this.actionComboBox.SelectedItem = _wmsRule.ActionType;

                if (this._wmsRule.RuleType.Equals(WmsCommand.CONN_RULE_TYPE) ||
                    this._wmsRule.RuleType.Equals(WmsCommand.COMP_RULE_TYPE))
                {
                    this.sqlStringTextBox.Text = _wmsRule.ActionCommand;
                    this.sqlStringTextBox.Enabled = true;
                }
            }
            else
            {
                this.actionComboBox.SelectedItem = NO_ACTION;
            }

            if (this._wmsRule.RuleType.Equals(WmsCommand.CONN_RULE_TYPE))
            {
                checkEnableAggregate();
                string aggregateTypes = _wmsRule.AggregateQueryTypes;
                if (aggregateTypes.Length > 0)
                {
                    string[] tokens = aggregateTypes.Split(',');
                    for (int i = 0; i < typeCheckedListBox.Items.Count; i++)
                    {
                        string item = typeCheckedListBox.Items[i].ToString().Trim();
                        foreach (string s in tokens)
                        {
                            if (item.Equals(s.Trim(), StringComparison.CurrentCultureIgnoreCase))
                            {
                                typeCheckedListBox.SetItemChecked(i, true);
                                break;
                            }
                        }
                    }
                    wmsIntervalNumericUpDown.Value = _wmsRule.AggregateWmsInterval;
                    repositoryIntervalNumericUpDown.Value = _wmsRule.AggregateRepositoryInterval;
                    executionIntervalNumericUpDown.Value = _wmsRule.AggregateExecInterval;
                    this._statsCheckBox.Checked = _wmsRule.IsStatsCollectOnce;
                }
            }
            
            this.expressionTextBox.Text = _wmsRule.Expression;
            this._commentTextBox.Text = _wmsRule.Comment;
        }

        private void comboBoxAction_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (this.actionComboBox.SelectedItem.Equals("SQL_CMD") ||
                (this.actionComboBox.SelectedItem.Equals("EXEC") && this.ruleTypeCompRadioButton.Checked))
            {
                this.sqlStringTextBox.Enabled = true;
            }
            else
            {
                this.sqlStringTextBox.Enabled = false;
                this.sqlStringTextBox.Text = "";
            }
        }

        private void comboBoxOperator_SelectedIndexChanged(object sender, EventArgs e)
        {
            bool percentageOperator = this.operatorComboBox.SelectedItem.ToString().Equals("%");
            this.percentageTextBox.ReadOnly = percentageOperator ? false : true;
            if (percentageOperator)
                this.percentageTextBox.Focus();
            else
                this.percentageTextBox.Text = "";
        }

        //check for selected properties in the expression
        private bool propertyExist(string prop)
        {
            bool exist = false;
            string prop2 = ", " + prop;
            string prop3 = "AND, " + prop;
            string prop4 = "OR, " + prop;

            string[] lines = this.expressionTextBox.Lines;
            for (int i = 0; i < lines.Length && !exist; i++)
            {
                if (lines[i].StartsWith(prop))
                {
                    exist = true;
                }
                else if (lines[i].StartsWith(prop2))
                {
                    exist = true;
                }
                else if (lines[i].StartsWith(prop3))
                {
                    exist = true;
                }
                else if (lines[i].StartsWith(prop4))
                {
                    exist = true;
                }
            }

            return exist;
        }

        private void buttonAdd_Click(object sender, EventArgs e)
        {
            string prop = (string)this.propertyComboBox.SelectedItem;
            string oper = (string)this.operatorComboBox.SelectedItem;
            string value = (string)this.valueComboBox.Text.Trim();

            if (propertyExist(prop) && this.ruleTypeConnRadioButton.Checked)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "The expression property is already selected", "Error altering rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                this.propertyComboBox.Focus();
                return;
            }

            if (this.ruleTypeConnRadioButton.Checked)
            {
                if (value.Length == 0)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), "Expression value not specified", "Error altering rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    this.valueComboBox.Focus();
                    return;
                }

                if (value.Contains(" ") && !value.StartsWith("\"") && !value.EndsWith("\""))
                {
                    value = "\"" + value + "\"";
                }

                if (this.upperCheckBox.Checked)
                    value = "ICASE( " + value + ")";
            }
            else if (this.ruleTypeCompRadioButton.Checked)
            {
                if (this.propertyComboBox.Text.Equals("EST_TOTAL_MEMORY") ||
                    this.propertyComboBox.Text.Equals("EST_TOTAL_TIME") ||
                    this.propertyComboBox.Text.Equals("EST_CARDINALITY") ||
                    this.propertyComboBox.Text.Equals("EST_ACCESSED_ROWS") ||
                    this.propertyComboBox.Text.Equals("EST_USED_ROWS") ||
                    this.propertyComboBox.Text.Equals("NUM_JOINS") ||
                    this.propertyComboBox.Text.Equals("SCAN_SIZE"))
                {
                    if (oper.Length == 0 || value.Length == 0)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), "Operator value not specified", "Error altering rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        if (oper.Length == 0)
                            this.operatorComboBox.Focus();
                        else
                            this.valueComboBox.Focus();
                        return;
                    }
                }
                else if (this.propertyComboBox.Text.Equals("UPDATE_STATS_WARNING") || this.propertyComboBox.Text.Equals("CROSS_PRODUCT"))
                {
                    if (oper.Length > 0 || value.Length > 0)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), "Operator value not required", "Error altering rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        if (oper.Length > 0)
                            this.operatorComboBox.Focus();
                        else
                            this.valueComboBox.Focus();
                        return;
                    }
                }
            }
            else if (this.ruleTypeExecRadioButton.Checked)
            {
                if (value.Length == 0)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), "Operator value not specified", "Error altering rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    this.valueComboBox.Focus();
                    return;
                }

                if (oper.Equals("%"))
                {
                    string percentage = this.percentageTextBox.Text;
                    if (percentage.Length == 0)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), "Percent value not specified", "Error altering rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        this.percentageTextBox.Focus();
                        return;
                    }
                }
            }

            string[] lines = this.expressionTextBox.Lines;
            if (0 < this.expressionTextBox.Text.Trim().Length)
                this.expressionTextBox.AppendText("," + Environment.NewLine + "    ");
            else
            {
                //if (!this.ruleTypeConnRadioButton.Checked)
                //{
                //    //this.expressionTextBox.AppendText(this.logicalOperatorComboBox.Text);
                //    this.expressionTextBox.AppendText(", ");
                //}
            }
            string expr = "";
            if (oper.Equals("%"))
            {
                string percentage = this.percentageTextBox.Text;
                expr = prop + " " + percentage + " " + oper + " " + value;
            }
            else
            {
                expr = prop + " " + oper + " " + value;
            }
            this.expressionTextBox.AppendText(expr);
        }

        private void buttonClear_Click(object sender, EventArgs e)
        {
            this.expressionTextBox.Clear();
        }

        string getAggregateString()
        {
            string aggregate = "";

            if (this.typeCheckedListBox.CheckedItems.Count > 0)
            {
                aggregate = "AGGREGATE (QUERY_TYPE(";
                for (int i = 0; i < this.typeCheckedListBox.CheckedItems.Count; i++)
                {
                    if (i > 0)
                        aggregate += ", ";
                    aggregate += this.typeCheckedListBox.CheckedItems[i];
                }
                aggregate += "), ";
                aggregate += "WMS_INTERVAL " + this.wmsIntervalNumericUpDown.Value + ", ";
                aggregate += "REPOS_INTERVAL " + this.repositoryIntervalNumericUpDown.Value + ", ";
                aggregate += "EXEC_INTERVAL " + this.executionIntervalNumericUpDown.Value;
                if (_wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
                {
                    aggregate += ", STATS_ONCE " + (this._statsCheckBox.Checked ? "ON" : "OFF");
                }
                aggregate += ") ";
            }
            else
            {
                aggregate = "AGGREGATE NO-ACTION";
            }

            return aggregate;
        }

        string getAggregateTypes()
        {
            string aggregateTypes = "";

            if (this.typeCheckedListBox.CheckedItems.Count > 0)
            {
                for (int i = 0; i < this.typeCheckedListBox.CheckedItems.Count; i++)
                {
                    if (i > 0)
                        aggregateTypes += ", ";
                    aggregateTypes += this.typeCheckedListBox.CheckedItems[i];
                }
            }

            return aggregateTypes;
        }

        private string getCommandPreview()
        {
            StringBuilder sb = new StringBuilder();
            string type = WmsCommand.CONN_RULE_TYPE;
            if (this.ruleTypeConnRadioButton.Checked)
            {
                type = WmsCommand.CONN_RULE_TYPE;
            }
            else if (this.ruleTypeCompRadioButton.Checked)
            {
                type = WmsCommand.COMP_RULE_TYPE;
            }
            else if (this.ruleTypeExecRadioButton.Checked)
            {
                type = WmsCommand.EXEC_RULE_TYPE;
            }

            sb.Append("ALTER RULE ");
            sb.Append(type);
            sb.Append(" ");
            sb.Append(this.ruleNameTextBox.Text.Trim());
            sb.Append(" (");

            if (!this.ruleTypeConnRadioButton.Checked)
            {
                sb.Append(this.logicalOperatorComboBox.Text);
                sb.Append(", ");
            }

            string[] lines = this.expressionTextBox.Lines;
            for (int i = 0; i < lines.Length; i++)
            {
                sb.Append(lines[i]);
            }

            string warnLevel = (string)this.warnLevelComboBox.SelectedItem;
            if (warnLevel != null && warnLevel.Length > 0)
            {
                sb.Append(", ");
                sb.Append(warnLevel);
            }

            string action = (string)this.actionComboBox.SelectedItem;
            if (String.IsNullOrEmpty(action) && !isOldR24SUT())
            {
                if (this._wmsRule.RuleType.Equals(WmsCommand.CONN_RULE_TYPE))
                {
                    if (!atLeastOneAggregateType())
                        action = NO_ACTION;
                }
            }

            if (this._wmsRule.RuleType.Equals(WmsCommand.CONN_RULE_TYPE) && action.Equals(NO_ACTION))
            {
                sb.Append(", SQL_CMD NO-ACTION ");
            }
            else
            {
                if (false == String.IsNullOrEmpty(action))
                {
                    sb.Append(", ");
                    sb.Append(action);
                    sb.Append(" ");
                }

                string sqlString = "";
                foreach (String aLine in this.sqlStringTextBox.Lines)
                {
                    //  If we already added a line -- seperate the lines by spaces for readability.
                    if (0 < sqlString.Length)
                        sqlString += " ";

                    sqlString += aLine.Trim();
                }

                if (sqlString != null && sqlString.Length > 0)
                {
                    bool append = true;
                    if (sqlString.StartsWith("\"") && sqlString.EndsWith("\""))
                        append = false;
                    if (append)
                        sb.Append("\"");
                    sb.Append(sqlString);
                    if (append)
                        sb.Append("\"");
                }
            }

            if (this._wmsRule.RuleType.Equals(WmsCommand.CONN_RULE_TYPE))
            {
                string aggregate = getAggregateString();
                if (aggregate != null && aggregate.Length > 0)
                {
                    sb.Append(", ");
                    sb.Append(aggregate);
                    sb.Append(" ");
                }
            }

            sb.Append(")");

            String commentString = getFormattedCommandLineFromTextBox(this._commentTextBox);
            if (0 < commentString.Length)
                sb.Append(" COMMENT \"").Append(commentString.Replace("\"", "'")).Append("\"");

            sb.Append(";");

            return sb.ToString();
        }

        private String getFormattedCommandLineFromTextBox(TextBox tb)
        {
            String cmdStr = "";
            foreach (String aLine in tb.Lines)
            {
                if (0 < cmdStr.Length)
                    cmdStr += " ";

                cmdStr += aLine.Trim();
            }

            return cmdStr;

        }  /*  End of  getFormattedCommandLineFromTextBox  method.  */

        private void getInfoFromControls(WmsRule wmsRule)
        {
            wmsRule.Operator = this.logicalOperatorComboBox.Text;
            wmsRule.Expression = this.expressionTextBox.Text;

            wmsRule.WarnLevel = (string)this.warnLevelComboBox.SelectedItem;
            wmsRule.ActionType = (this.actionComboBox.SelectedItem == null) ? "" : (string)this.actionComboBox.SelectedItem;

            if (ruleTypeConnRadioButton.Checked && wmsRule.ActionType.Equals(NO_ACTION))
            {
                wmsRule.ActionType = "SQL_CMD";
                wmsRule.ActionCommand = "NO-ACTION";
            }
            else
            {

                if (String.IsNullOrEmpty(wmsRule.ActionType))
                {
                    if (this._wmsRule.RuleType.Equals(WmsCommand.CONN_RULE_TYPE))
                    {
                        if (!atLeastOneAggregateType())
                            wmsRule.ActionType = NO_ACTION;
                    }
                }

                String ruleAction = getFormattedCommandLineFromTextBox(this.sqlStringTextBox);
                ruleAction = ruleAction.Trim();
                if (ruleAction.EndsWith(";"))
                    ruleAction = ruleAction.Substring(0, ruleAction.Length - 1);

                wmsRule.ActionCommand = ruleAction;
            }

            if (this._wmsRule.RuleType.Equals(WmsCommand.CONN_RULE_TYPE))
            {
                wmsRule.AggregateQueryTypes = getAggregateTypes();
                wmsRule.AggregateRepositoryInterval = (Int16)repositoryIntervalNumericUpDown.Value;
                wmsRule.AggregateWmsInterval = (Int16)wmsIntervalNumericUpDown.Value;
                wmsRule.AggregateExecInterval = (Int16)executionIntervalNumericUpDown.Value;
                wmsRule.IsStatsCollectOnce = _statsCheckBox.Checked;
            }

            String ruleComment = getFormattedCommandLineFromTextBox(this._commentTextBox);
            wmsRule.Comment = ruleComment.Replace("\"", "'");
        }

        private bool isInternalWMSObject(String objName)
        {
            if ((null != objName) && objName.StartsWith("HPS_"))
                return true;

            return false;
        }

        // Alter all of the list of services to associate or disassociate with the current rule depending on the isDisassociate param
        private void alterAssociatedServices(List<WmsService> aServicesList, bool isDisassociate)
        {
            StringBuilder sb = new StringBuilder();

            // Handle Connection Rules
            if (_wmsRule.RuleType == WmsCommand.CONN_RULE_TYPE)
            {
                // Connection rules can only be associated with one service
                if (aServicesList.Count > 1)
                {
                    throw new Exception("Connection rules can only be associated with a single WMS Service");
                }

                sb.Append("ALTER WMS CONN (");
                bool isFirstRule = true;
                bool wmsNeedsDefaultRuleAssoc = this._wmsSystem.NeedDefaultRuleAssociations;

                // Append all other connection rules
                foreach (WmsService currService in _wmsSystem.WmsServices)
                {
                    foreach (WmsRule theRule in currService.AssociatedConnectionRules)
                    {
                        if (!wmsNeedsDefaultRuleAssoc && isInternalWMSObject(theRule.Name))
                            continue;

                        if (!theRule.Name.Equals(_wmsRule.Name))
                        {
                            if (!isFirstRule)
                            {
                                sb.Append(",");
                            }
                            else
                            {
                                isFirstRule = false;
                            }
                            sb.Append(theRule.Name);
                            sb.Append(" ");
                            sb.Append(currService.Name);
                        }
                    }
                }

                // Append changed rule
                WmsService theService = aServicesList[0];
                if (!isDisassociate)
                {
                    if (!isFirstRule)
                        sb.Append(",");

                    sb.Append(_wmsRule.Name);
                    sb.Append(" ");
                    sb.Append(theService.Name);
                }

                sb.Append(")");

                String sql = sb.ToString();
                if (sql.Length > 0)
                {
                    _wmsSystem.AlterConnectionRuleAssociations(sql);
                    _wmsSystem.Refresh();
                }

                if (isDisassociate)
                {
                    foreach (WmsService service in aServicesList)
                    {
                        this.originallyAssociatedServiceNames.Remove(service.Name);
                    }
                }
            }
            else //Handle all other rules
            {
                foreach (WmsService theService in aServicesList)
                {
                    sb = new StringBuilder();
                    sb.Append("ALTER SERVICE ").Append(theService).Append(" ").Append(_wmsRule.RuleType).Append(" (");

                    int origAssociatedRulesCount = theService.AssociatedRules.Count;
                    bool addedAssociatedRules = false;

                    // Add all originally associated rules of this ruletype
                    for (int idx = 0; idx < origAssociatedRulesCount; idx++)
                    {
                        WmsRule theRule = theService.AssociatedRules[idx];
                        if (theRule.RuleType.Equals(_wmsRule.RuleType))
                        {
                            if (!theRule.Equals(_wmsRule))
                            {
                                if (addedAssociatedRules)
                                    sb.Append(", ");

                                addedAssociatedRules = true;
                                sb.Append(theRule.Name);
                            }

                        }
                    }

                    // Append this rule
                    if (!isDisassociate)
                    {
                        if (addedAssociatedRules)
                            sb.Append(",");
                        sb.Append(_wmsRule.Name);
                    }

                    sb.Append(")");

                    theService.AlterServiceRuleAssociation(sb.ToString());

                    if (isDisassociate)
                    {
                        this.originallyAssociatedServiceNames.Remove(theService.Name);
                    }

                }
            }

        }

        private void alterButton_Click(object sender, EventArgs e)
        {
            string ruleName = (string)this.ruleNameTextBox.Text.Trim();
            string expr = (string)this.expressionTextBox.Text.Trim();
            if (ruleName == null || ruleName.Length == 0)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "Rule name not specified", "Error altering rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                this.ruleNameTextBox.Focus();
                return;
            }
            else if (expr == null || expr.Length == 0)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "Expression not specified", "Error altering rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }
            else if (!String.IsNullOrEmpty(actionComboBox.Text.Trim()))
            {
                if (actionComboBox.Text.Trim().Equals("SQL_CMD") || actionComboBox.Text.Trim().Equals("EXEC"))
                {
                    if (String.IsNullOrEmpty(sqlStringTextBox.Text.Trim()))
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), "SQL string is required for the selected rule action", "Error adding rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        return;
                    }
                }
            }
            //fill the rule model with information from the ui controls
            getInfoFromControls(_wmsRule);

            try
            {
                this.Cursor = Cursors.WaitCursor;

                _wmsRule.Alter();

                string msg = "Rule " + _wmsRule.Name + " altered successfully";

                // Associate services
                List<WmsService> servicesNeedAssociating = new List<WmsService>();
                List<WmsService> servicesNeedDisassociating = new List<WmsService>();

                for (int i = 0; i < this.associatedServicesIGrid.Rows.Count; i++)
                {
                    string theServiceName = (string)associatedServicesIGrid.Cells[i, "ServiceName"].Value;

                    // If the service marked to be associated
                    if ((this.associatedServicesIGrid.Cells[i, "SelectCheckBox"].Value != null) &&
                        ((bool)this.associatedServicesIGrid.Cells[i, "SelectCheckBox"].Value == true))
                    {
                        if (!originallyAssociatedServiceNames.Contains(theServiceName))
                        {
                            // Find the service and associate it
                            foreach (WmsService aService in _wmsSystem.WmsServices)
                            {
                                if (aService.Name.Equals(theServiceName))
                                {
                                    servicesNeedAssociating.Add(aService);
                                }
                            }
                        }
                    }
                    else // If the service isn't associated, check if originally associated
                    {
                        if (originallyAssociatedServiceNames.Contains(theServiceName))
                        {
                            // Find the service and deAssociate it
                            foreach (WmsService aService in _wmsSystem.WmsServices)
                            {
                                if (aService.Name.Equals(theServiceName))
                                {
                                    servicesNeedDisassociating.Add(aService);
                                }
                            }
                        }
                    }
                }
                try
                {
                    // Disassociate services first
                    if (servicesNeedDisassociating.Count > 0)
                    {
                        alterAssociatedServices(servicesNeedDisassociating, true);
                        msg += "\n" + servicesNeedDisassociating.Count + " Services disassociated successfully";
                    }
                    // Associate Rule to selected services
                    if (servicesNeedAssociating.Count > 0)
                    {
                        alterAssociatedServices(servicesNeedAssociating, false);
                        msg += "\n" + servicesNeedAssociating.Count + " Services associated successfully";
                    }

                    // Refresh the rule if needed
                    if ((servicesNeedAssociating.Count > 0) || (servicesNeedDisassociating.Count > 0))
                    {
                        this._wmsRule.ResetAssociatedServiceNames();
                    }

                    MessageBox.Show(Utilities.GetForegroundControl(), msg, "Alter Rule", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    if (Parent != null && Parent is Form)
                    {
                        ((Form)Parent).DialogResult = DialogResult.OK;
                        //((Form)Parent).Close();
                    }

                }
                catch (Exception ex)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), "Exception: " + ex.Message, "Error associating rules", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }

            }
            catch (Exception ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, "Error altering rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                this.Cursor = Cursors.Default;
            }
        }

        private void resetDefaults()
        {
            // Reset originally associated services
            originallyAssociatedServiceNames.Clear();
            foreach (WmsService theService in _wmsSystem.WmsServices)
            {
                if (theService.AssociatedRules.Contains(_wmsRule))
                {
                    originallyAssociatedServiceNames.Add(theService.Name);
                }
            }
        }

        private void buttonCancel_Click(object sender, EventArgs e)
        {
            if (Parent is Form)
            {
                ((Form)Parent).Close();
            }
        }

        private void textBoxExpression_TextChanged(object sender, EventArgs e)
        {
            this.okButton.Enabled = (this.expressionTextBox.Text.Length > 0 && this.ruleNameTextBox.Text.Length > 0) ? true : false;
        }

        private void textBoxRuleName_TextChanged(object sender, EventArgs e)
        {
            this.okButton.Enabled = (this.expressionTextBox.Text.Length > 0 && this.ruleNameTextBox.Text.Length > 0) ? true : false;
        }

        private void propertyComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (this.propertyComboBox.SelectedItem.ToString().Equals("UPDATE_STATS_WARNING") ||
                this.propertyComboBox.SelectedItem.ToString().Equals("CROSS_PRODUCT"))
            {
                this.operatorComboBox.SelectedItem = "";
                this.valueComboBox.SelectedItem = "";
                this.operatorComboBox.Enabled = false;
                this.valueComboBox.Enabled = false;
            }
            else
            {
                this.operatorComboBox.Enabled = !this.ruleTypeConnRadioButton.Checked;
                this.valueComboBox.Enabled = true;
            }
        }

        private void associatedServicesIGrid_CellClick(object sender, iGCellClickEventArgs e)
        {
            int serviceIndex = e.RowIndex;

            if (this.associatedServicesIGrid.Cols["SelectCheckBox"].Index == e.ColIndex)
            {
                // Connection rules can only be associated with one service at a time
                if (_wmsRule.RuleType == WmsCommand.CONN_RULE_TYPE)
                {
                    for (int i = 0; i < this.associatedServicesIGrid.Rows.Count; i++)
                    {
                        if ((i != serviceIndex) &&
                            (associatedServicesIGrid.Cells[i, "SelectCheckBox"].Value != null) &&
                            (bool)(associatedServicesIGrid.Cells[i, "SelectCheckBox"].Value))
                        {
                            if (MessageBox.Show(Utilities.GetForegroundControl(), "This connection rule has already been associated with another service.  Do you want to change the association?", "Change association?", MessageBoxButtons.YesNo) == DialogResult.Yes)
                            {
                                // Untoggle old selected
                                toggleSelectedService(i);
                                // Toggle new selected
                                toggleSelectedService(serviceIndex);
                                return;
                            }
                            else
                            {
                                return;
                            }
                        }
                    }
                }

                toggleSelectedService(serviceIndex);
            }
        }

        private void toggleSelectedService(int rowNumber)
        {
            if ((0 > rowNumber) ||
                (this.associatedServicesIGrid.Rows.Count <= rowNumber))
                return;

            iGCell checkbox = this.associatedServicesIGrid.Rows[rowNumber].Cells["SelectCheckBox"];
            if (null == checkbox)
                return;

            if (null == checkbox.Value)
                checkbox.Value = true;
            else
                checkbox.Value = null;
        }

        private TextBox findTextBoxWithTheFocus()
        {
            if (previewTextBox.Focused)
                return previewTextBox;

            if (sqlStringTextBox.Focused)
                return sqlStringTextBox;

            if (expressionTextBox.Focused)
                return expressionTextBox;

            if (ruleNameTextBox.Focused)
                return ruleNameTextBox;

            return null;
        }

        private void selectAll()
        {
            TextBox focusedTextBoxControl = findTextBoxWithTheFocus();
            if (null == focusedTextBoxControl)
                return;

            focusedTextBoxControl.SelectionStart = 0;
            focusedTextBoxControl.SelectionLength = focusedTextBoxControl.Text.Length;
        }

        private void copyToClipboard()
        {
            TextBox focusedTextBoxControl = findTextBoxWithTheFocus();
            if (null == focusedTextBoxControl)
                return;

            String data = focusedTextBoxControl.Text;
            if (0 < focusedTextBoxControl.SelectionLength)
                data = focusedTextBoxControl.SelectedText;

            Clipboard.SetDataObject(data, true);
        }

        protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
        {
            switch (msg.Msg)
            {
                case 0x100:
                case 0x104:
                    switch (keyData)
                    {
                        case Keys.Control | Keys.C:
                        case Keys.Control | Keys.Shift | Keys.C:
                            copyToClipboard();
                            break;

                        default:
                            break;
                    }
                    break;

                default:
                    break;
            }

            return base.ProcessCmdKey(ref msg, keyData);
        }

        protected override bool ProcessDialogKey(Keys keyData)
        {
            switch (keyData)
            {
                case Keys.Control | Keys.A:
                case Keys.Control | Keys.Shift | Keys.A:
                    selectAll();
                    break;

                default:
                    break;
            }

            return base.ProcessDialogKey(keyData);
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            string helpTopic = HelpTopics.ConnectionRuleAttributes;

            if (this.ruleTypeCompRadioButton.Checked)
                helpTopic = HelpTopics.CompilationRuleAttributes;
            else
                if (this.ruleTypeExecRadioButton.Checked)
                    helpTopic = HelpTopics.ExectionRuleAttributes;

            TrafodionHelpProvider.Instance.ShowHelpTopic(helpTopic);
        }

        private void logicalOperatorComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            previewTextBox.Text = getCommandPreview();
        }

        private void _statsCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            previewTextBox.Text = getCommandPreview();
        }

    }
}
