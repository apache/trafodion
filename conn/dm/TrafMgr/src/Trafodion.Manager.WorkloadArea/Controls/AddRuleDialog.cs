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
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.WorkloadArea.Model;


namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class AddRuleDialog : TrafodionForm
    {
        #region Members

        private WmsSystem _wmsSystem = null;

        private const string STATS_IDLE_MINUTES = "STATS_IDLE_MINUTES";

        private string[] m_warnLevels = { "WARN-HIGH", "WARN-MEDIUM", "WARN-LOW", "NO-WARN" };
        private string[] m_connWarnLevels = { "WARN-LOW", "NO-WARN" };
        private string[] m_logicalOperators = { "AND", "OR" };
        private string[] m_connAction = { "", "SQL_CMD" };
        private string[] m_compAction = { "", "REJECT", "HOLD", "SQL_CMD", "EXECUTE" };
        private string[] m_execAction = { "", "CANCEL"};
        private string[] m_connProperty = { "SESSION", "LOGIN", "APPL", "DSN" };
        private string[] m_compProperty =  { "EST_TOTAL_MEMORY", "EST_TOTAL_TIME", "EST_CARDINALITY", "EST_ACCESSED_ROWS", "EST_USED_ROWS", 
											"NUM_JOINS", "SCAN_SIZE", "UPDATE_STATS_WARNING", "CROSS_PRODUCT"};
        private string[] m_execProperty = { "USED_ROWS", "ACCESSED_ROWS", "TOTAL_MEM_ALLOC", "ELAPSED_TIME", "CPU_TIME", STATS_IDLE_MINUTES };
        private string[] m_connOperator = { "" };
        private string[] m_compOperator = { "", "=", ">=", ">", "<", "<=", "<>" };
        private string[] m_execOperator = { "=", ">=", ">", "<", "<=", "<>", "%" };
        private string[] m_nonExecValue = { "" };
        private string[] m_execValue = { "", "EST_USED_ROWS", "EST_ACCESSED_ROWS", "EST_TOTAL_MEMORY", "EST_CPU_TIME" };
        #endregion

        public string RuleType
        {
            get
            {
                if (this.ruleTypeCompRadioButton.Checked)
                {
                    return WmsCommand.COMP_RULE_TYPE;
                }
                else if (this.ruleTypeExecRadioButton.Checked)
                {
                    return WmsCommand.EXEC_RULE_TYPE;
                }
                else
                {
                    return WmsCommand.CONN_RULE_TYPE;
                }
            }
            set
            {
                if (WmsCommand.COMP_RULE_TYPE.Equals(value))
                {
                    this.ruleTypeCompRadioButton.Checked = true;
                }
                else
                    if (WmsCommand.EXEC_RULE_TYPE.Equals(value))
                    {
                        this.ruleTypeExecRadioButton.Checked = true;
                    }
                    else
                        this.ruleTypeConnRadioButton.Checked = true;
            }
        }

        public AddRuleDialog(WmsSystem wmsSystem, string ruleType)
        {
            InitializeComponent();
            _wmsSystem = wmsSystem;

            SetBackwardCompatibility();

            initializeControls();
            this.Load += new EventHandler(handleInfoChanged);
            this._ruleNameTextBox.TextChanged += new EventHandler(handleInfoChanged);
            this.ruleTypeConnRadioButton.Click += new EventHandler(handleInfoChanged);
            this.ruleTypeCompRadioButton.Click += new EventHandler(handleInfoChanged);
            this.ruleTypeExecRadioButton.Click += new EventHandler(handleInfoChanged);
            this._warnLevelComboBox.TextChanged += new EventHandler(handleInfoChanged);
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

            RuleType = ruleType;
            autoGenerateRuleComments();
            checkEnableAggregate();
            CancelButton = _cancelButton;
            CenterToParent();
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
            
            // Remove the new EXECUTE for versions earlier than M10
            if (this._wmsSystem.ConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                m_compAction = Array.FindAll<String>(m_compAction, stringValue => 0 != string.Compare(stringValue, "EXECUTE", true));
            }
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
            this.toolTip1.IsBalloon = true;
            this.toolTip1.AutoPopDelay = 60 * 1000;
            this.toolTip1.SetToolTip(typeLinkLabel, caption);
        }

        void repositoryIntervalNumericUpDown_ValueChanged(object sender, EventArgs e)
        {
            _previewTextBox.Text = getCommandPreview();
        }

        void wmsIntervalNumericUpDown_ValueChanged(object sender, EventArgs e)
        {
            _previewTextBox.Text = getCommandPreview();
        }

        void executionIntervalNumericUpDown_ValueChanged(object sender, EventArgs e)
        {
            _previewTextBox.Text = getCommandPreview();
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
            _previewTextBox.Text = getCommandPreview();
            checkEnableAggregate();
        }

        void checkAllLinkLabel_Click(object sender, EventArgs e)
        {
            for (int i = 0; i < this.typeCheckedListBox.Items.Count; i++)
            {
                this.typeCheckedListBox.SetItemChecked(i, true);
            }
            _previewTextBox.Text = getCommandPreview();
            checkEnableAggregate();
        }

        void typeCheckedListBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            _previewTextBox.Text = getCommandPreview();
            checkEnableAggregate();
        }

        void handleInfoChanged(object sender, EventArgs e)
        {
            this.aggregateGroupBox.Enabled = this.ruleTypeConnRadioButton.Checked;
            checkEnableAggregate();
            _previewTextBox.Text = getCommandPreview();
        }

        private void initializeControls()
        {
            this.ruleTypeConnRadioButton.Checked = true;
            this.logicalOperatorComboBox.Items.AddRange(m_logicalOperators);
            this.logicalOperatorComboBox.SelectedIndex = 0;
        }

        private void autoGenerateRuleComments()
        {
            //  Auto-generated rule comment.
            DateTime dt = DateTime.Now;

            //"12/11/2008 <Time> : New rule added by super.services"
            StringBuilder sb = new StringBuilder();
            sb.Append(dt.ToShortDateString()).Append(" ").Append(dt.ToShortTimeString());
            sb.Append(": New rule added by ").Append(this._wmsSystem.ConnectionDefinition.UserName);
            this._commentTextBox.Text = sb.ToString();

        }  /*  End of  autoGenerateRuleComments   method.  */

        private void radioButtonRuleTypeConn_CheckedChanged(object sender, EventArgs e)
        {
            this._warnLevelComboBox.Items.Clear();
            this._warnLevelComboBox.Items.AddRange(m_connWarnLevels);
            this._warnLevelComboBox.SelectedItem = "NO-WARN";

            this.actionComboBox.Items.Clear();
            this.actionComboBox.Items.AddRange(m_connAction);
            this.actionComboBox.SelectedIndex = 0;

            this.propertyComboBox.Items.Clear();
            this.propertyComboBox.Items.AddRange(m_connProperty);
            this.propertyComboBox.SelectedIndex = 0;
            this.operatorComboBox.Items.Clear();
            this.operatorComboBox.Items.AddRange(m_connOperator);
            this.operatorComboBox.Enabled = false;
            this.operatorComboBox.SelectedIndex = 0;
            this.valueComboBox.Items.Clear();
            this.valueComboBox.Items.AddRange(m_nonExecValue);
            this.valueComboBox.SelectedIndex = 0;

            this.logicalOperatorComboBox.Enabled = false;
            this.logicalOperatorComboBox.SelectedItem = "AND";
            this.expressionTextBox.Clear();
            this.upperCheckBox.Enabled = true;
        }

        private bool isOldR24SUT()
        {
            // RR:  Wait for Anan/Matt gives a new API call.

            return ConnectionDefinition.isServerVersionOnOldR24SUT(_wmsSystem.ConnectionDefinition.OdbcServerVersion);
        }

        private void radioButtonRuleTypeComp_CheckedChanged(object sender, EventArgs e)
        {
            this._warnLevelComboBox.Items.Clear();
            this._warnLevelComboBox.Items.AddRange(m_warnLevels);
            this._warnLevelComboBox.SelectedItem = "WARN-LOW";
            this.actionComboBox.Items.Clear();
            this.actionComboBox.Items.AddRange(m_compAction);
            this.actionComboBox.SelectedIndex = 0;

            this.propertyComboBox.Items.Clear();
            this.propertyComboBox.Items.AddRange(m_compProperty);
            this.propertyComboBox.SelectedIndex = 0;
            this.operatorComboBox.Items.Clear();
            this.operatorComboBox.Items.AddRange(m_compOperator);
            this.operatorComboBox.Enabled = true;
            this.operatorComboBox.SelectedIndex = 0;
            this.valueComboBox.Items.Clear();
            this.valueComboBox.Items.AddRange(m_nonExecValue);
            this.valueComboBox.SelectedIndex = 0;

            this.logicalOperatorComboBox.Enabled = true;
            this.expressionTextBox.Clear();
            this.upperCheckBox.Enabled = false;
        }

        private void radioButtonRuleTypeExec_CheckedChanged(object sender, EventArgs e)
        {
            this._warnLevelComboBox.Items.Clear();
            this._warnLevelComboBox.Items.AddRange(m_warnLevels);
            this._warnLevelComboBox.SelectedItem = "WARN-LOW";
            this.actionComboBox.Items.Clear();
            this.actionComboBox.Items.AddRange(m_execAction);
            if (this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                this.actionComboBox.Items.AddRange(new string[] {"STATS_PERTABLE", "STATS_PERTABLE_CANCEL"});
            }
            this.actionComboBox.SelectedIndex = 0;

            this.propertyComboBox.Items.Clear();
            this.propertyComboBox.Items.AddRange(m_execProperty);
            this.propertyComboBox.SelectedIndex = 0;
            this.operatorComboBox.Items.Clear();
            this.operatorComboBox.Items.AddRange(m_execOperator);
            this.operatorComboBox.Enabled = true;
            this.operatorComboBox.SelectedIndex = 0;
            this.valueComboBox.Items.Clear();
            this.valueComboBox.Items.AddRange(m_execValue);
            this.valueComboBox.SelectedIndex = 0;

            this.logicalOperatorComboBox.Enabled = true;
            this.expressionTextBox.Clear();
            this.upperCheckBox.Enabled = false;
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

        private void buttonAddExpr_Click(object sender, EventArgs e)
        {
            string prop = (string)this.propertyComboBox.SelectedItem;
            string oper = (string)this.operatorComboBox.SelectedItem;
            string value = (string)this.valueComboBox.Text.Trim();

            if (propertyExist(prop) && this.ruleTypeConnRadioButton.Checked)
            {
                MessageBox.Show("The expression property is already selected.", "Error adding rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                this.propertyComboBox.Focus();
                return;
            }

            if (this.ruleTypeConnRadioButton.Checked)
            {
                if (value.Length == 0)
                {
                    MessageBox.Show("Expression value is not selected.", "Error adding rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
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
                    if (oper.Length == 0)
                    {
                        MessageBox.Show("Operator not specified", "Error adding rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        this.operatorComboBox.Focus();
                        this.valueComboBox.Focus();
                        return;
                    }
                    if (value.Length == 0)
                    {
                        MessageBox.Show("Expression value not specified", "Error adding rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        this.valueComboBox.Focus();
                        return;
                    }
                }
                else if (this.propertyComboBox.Text.Equals("UPDATE_STATS_WARNING") || this.propertyComboBox.Text.Equals("CROSS_PRODUCT"))
                {
                    if (oper.Length > 0 || value.Length > 0)
                    {
                        MessageBox.Show("Operator value not required.", "Error adding rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
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
                    MessageBox.Show("Expression value not specified", "Error adding rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    this.valueComboBox.Focus();
                    return;
                }

                if (oper.Equals("%"))
                {
                    string percentage = this.percentageTextBox.Text;
                    if (percentage.Length == 0)
                    {
                        MessageBox.Show("The perecentage values is not specified", "Error adding rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        this.percentageTextBox.Focus();
                        return;
                    }
                }
            }

            if (this.expressionTextBox.Text.Trim().Length > 0)
            {
                this.expressionTextBox.AppendText("," + Environment.NewLine + "    ");
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
            wmsRule.Name = this._ruleNameTextBox.Text.Trim();
            wmsRule.RuleType = RuleType;
            wmsRule.Operator = this.logicalOperatorComboBox.Text;
            wmsRule.Expression = this.expressionTextBox.Text;

            wmsRule.WarnLevel = (string)this._warnLevelComboBox.SelectedItem;
            wmsRule.ActionType = (string)this.actionComboBox.SelectedItem;

            wmsRule.ActionCommand = getFormattedCommandLineFromTextBox(this.sqlStringTextBox);

            if (wmsRule.RuleType.Equals(WmsCommand.CONN_RULE_TYPE))
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

                if (this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
                {
                    if (this._statsCheckBox.Checked)
                    {
                        aggregate += ", STATS_ONCE ON";
                    }
                }
                aggregate +=  ") ";
            }

            return aggregate;
        }

        private string getCommandPreview()
        {
            //ADD RULE CONN myrule (SESSION mysession, AGGREGATE (QUERY_TYPE(INSERT,UPDATE,DELETE),REPOS_INTERVAL 3,WMS_INTERVAL 4))
            //add rule conn r28 (appl EXCEL2, session ABC2, dsn ANAN_WMS2, SQL_CMD "SET SCHEMA MANAGEABILITY.ANAN_SCHEMA;");;
            //ADD RULE COMP rc4 (OR, EST_USED_ROWS > 5000, EST_TOTAL_TIME > 1000, WARN-HIGH, REJECT);;
            StringBuilder sb = new StringBuilder();
            sb.Append("ADD RULE ");
            sb.Append(RuleType);
            sb.Append(" ");
            sb.Append(this._ruleNameTextBox.Text.Trim());
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

            string warnLevel = (string)this._warnLevelComboBox.SelectedItem;
            if (warnLevel != null && warnLevel.Length > 0)
            {
                sb.Append(", ");
                sb.Append(warnLevel);
            }

            string action = (string)this.actionComboBox.SelectedItem;
            if (action != null && action.Length > 0)
            {
                sb.Append(", ");
                sb.Append(action);
                sb.Append(" ");
            }

            string sqlString = this.sqlStringTextBox.Text.Trim();
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

            if (this.ruleTypeConnRadioButton.Checked)
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

        private void buttonAdd_Click(object sender, EventArgs e)
        {
            string ruleName = (string)this._ruleNameTextBox.Text.Trim();
            string expr = (string)this.expressionTextBox.Text.Trim();

            if (ruleName == null || ruleName.Length == 0)
            {
                MessageBox.Show("Rule name not specified", "Error adding rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                this._ruleNameTextBox.Focus();
                return;
            }
            else if (expr == null || expr.Length == 0)
            {
                MessageBox.Show("Expression value not specified", "Error adding rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }
            else if (!String.IsNullOrEmpty(actionComboBox.Text.Trim()))
            {
                if (actionComboBox.Text.Trim().Equals("SQL_CMD") || actionComboBox.Text.Trim().Equals("EXEC"))
                {
                    if (String.IsNullOrEmpty(sqlStringTextBox.Text.Trim()))
                    {
                        MessageBox.Show("SQL string is required for the selected rule action", "Error adding rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        return;
                    }
                }
            }

            WmsRule wmsRule = new WmsRule(_wmsSystem);

            //fill the rule model with information from the ui controls
            getInfoFromControls(wmsRule);

            try
            {
                this._addButton.Enabled = false;
                this.Cursor = Cursors.WaitCursor;
                wmsRule.Add();
                string msg = "Rule " + wmsRule.Name + " added successfully.";
                MessageBox.Show(msg, "Add rule", MessageBoxButtons.OK, MessageBoxIcon.Information);
                DialogResult = DialogResult.OK;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error adding rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                this.Cursor = Cursors.Default;
                this._addButton.Enabled = true;
            }
        }

        private void buttonCancel_Click(object sender, EventArgs e)
        {
            Close();
        }

        private TextBox findTextBoxWithTheFocus()
        {
            if (_previewTextBox.Focused)
                return _previewTextBox;

            if (sqlStringTextBox.Focused)
                return sqlStringTextBox;

            if (expressionTextBox.Focused)
                return expressionTextBox;

            if (_ruleNameTextBox.Focused)
                return _ruleNameTextBox;

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

        private void textBoxExpression_TextChanged(object sender, EventArgs e)
        {
            this._addButton.Enabled = (this.expressionTextBox.Text.Length > 0 && this._ruleNameTextBox.Text.Length > 0) ? true : false;
        }

        private void textBoxRuleName_TextChanged(object sender, EventArgs e)
        {
            this._addButton.Enabled = (this.expressionTextBox.Text.Length > 0 && this._ruleNameTextBox.Text.Length > 0) ? true : false;
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
            _previewTextBox.Text = getCommandPreview();
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
            else
            {
                typeCheckedListBox.ClearCheckedItems();
                wmsIntervalNumericUpDown.Value = 1;
                repositoryIntervalNumericUpDown.Value = 1;
                executionIntervalNumericUpDown.Value = 1;
                _statsCheckBox.Checked = false;
            }
            _statisticsGroupBox.Visible = this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120;
        }

        private void _statsCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            _previewTextBox.Text = getCommandPreview();
        }
    }
}
