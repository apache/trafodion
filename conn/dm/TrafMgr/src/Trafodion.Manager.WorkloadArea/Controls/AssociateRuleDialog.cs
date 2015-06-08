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
using System.Data.Odbc;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.WorkloadArea.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class AssociateRuleDialog : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        #region Constants
        private const int COLUMN_WIDTH = 200;
        private const int IGRIDINFO_MIN_WIDTH = 100;
        #endregion

        #region Members
        private WmsSystem _wmsSystem = null;
        private int _infoIGridDefaultRowHeight = 16;

        private bool _associateFlag = false;
        private bool _unassociateFlag = false;

        #endregion

        public AssociateRuleDialog(WmsSystem wmsSystem, string ruleType, WmsRule passedinRule)
        {
            InitializeComponent();
            _wmsSystem = wmsSystem;

            this.connRuleRadioButton.Checked = false;
            this.compRuleRadioButton.Checked = false;
            this.execRuleRadioButton.Checked = false;

            ResetButtons();
            addEventHandlers();

            populateRules();
            populateServices();

            switch (ruleType)
            {
                case WmsCommand.COMP_RULE_TYPE:
                    compRuleRadioButton.Checked = true;
                    break;
                case WmsCommand.EXEC_RULE_TYPE:
                    execRuleRadioButton.Checked = true;
                    break;
                default:
                    connRuleRadioButton.Checked = true;
                    break;

            }
            if (passedinRule != null)
            {
                this.rulesListBox.SelectedItem = passedinRule;
            }
            previewTextBox.Text = getCommandPreview();


        }


        private void ResetButtons()
        {
            this.rulesListBox.SelectedIndex = -1;
            servicesListBox.SelectedIndex = -1;
            this.servicesToRuleButton.Enabled = false;
            this.showRuleButton.Enabled = false;
            this.associateButton.Enabled = false;
            this.showServiceButton.Enabled = false;
            this.rulesToServiceButton.Enabled = false;
            this.moveUpButton.Enabled = false;
            this.moveDownButton.Enabled = false;
            this.unselectButton.Enabled = false;
        }


        private void adjustIGridRowHeight(iGrid theGrid)
        {
            theGrid.Rows.AutoHeight();

            foreach (iGRow aRow in theGrid.Rows)
            {
                if (aRow.Height < this._infoIGridDefaultRowHeight)
                    aRow.Height = this._infoIGridDefaultRowHeight;

            }
        }


        private void addEventHandlers()
        {
            this.rulesListBox.SelectedIndexChanged += new EventHandler(listBoxRules_SelectedIndexChanged);
            this.servicesListBox.SelectedIndexChanged += new EventHandler(listBoxServices_SelectedIndexChanged);
            this.associatedRulesIGrid.SelectionChanged += new EventHandler(iGridSelectedRules_SelectionChanged);

            this.connRuleRadioButton.Click += new EventHandler(handleInfoChanged);
            this.compRuleRadioButton.Click += new EventHandler(handleInfoChanged);
            this.execRuleRadioButton.Click += new EventHandler(handleInfoChanged);
            this.servicesListBox.SelectedIndexChanged += new EventHandler(handleInfoChanged);
        }

        void handleInfoChanged(object sender, EventArgs e)
        {
            previewTextBox.Text = getCommandPreview();
        }


        private bool isInternalWMSObject(String objName)
        {
            if ((null != objName) && objName.StartsWith("HPS_"))
                return true;

            return false;
        }


        bool enableAssociate()
        {
            if (this.rulesListBox.SelectedItem != null && this.servicesListBox.SelectedItem != null)
            {
                if (this.connRuleRadioButton.Checked)
                {
                    string ruleText = this.rulesListBox.SelectedItem.ToString();
                    string serviceText = this.servicesListBox.SelectedItem.ToString();
                    if (isInternalWMSObject(ruleText) || isInternalWMSObject(serviceText))
                        return false;
                }
                return true;
            }

            return false;
        }

        void iGridSelectedRules_SelectionChanged(object sender, EventArgs e)
        {
            if (this.associatedRulesIGrid.SelectedRowIndexes.Count > 0)
            {
                this.unselectButton.Enabled = true;
                int rowIndex = this.associatedRulesIGrid.SelectedRowIndexes[0];
                this.moveUpButton.Enabled = (rowIndex > 0) ? true : false;
                this.moveDownButton.Enabled = (rowIndex < this.associatedRulesIGrid.Rows.Count - 1) ? true : false;
            }
            else
            {
                moveDownButton.Enabled = moveUpButton.Enabled = false;
                unselectButton.Enabled = false;
            }
        }

        void listBoxRules_SelectedIndexChanged(object sender, EventArgs e)
        {
            this.servicesToRuleButton.Enabled = showRuleButton.Enabled = (rulesListBox.SelectedItems.Count > 0);
            this.associateButton.Enabled = enableAssociate();
        }

        void listBoxServices_SelectedIndexChanged(object sender, EventArgs e)
        {
            showServiceButton.Enabled = rulesToServiceButton.Enabled = (servicesListBox.SelectedItems.Count > 0);
            this.associateButton.Enabled = enableAssociate();

            populateAssociatedRules();
            this.moveUpButton.Enabled = false;
            this.moveDownButton.Enabled = false;
            this.unselectButton.Enabled = false;
        }

        private void populateRules()
        {
            this.rulesListBox.Items.Clear();

            foreach (WmsRule wmsRule in _wmsSystem.WmsRules)
            {
                if (this.connRuleRadioButton.Checked)
                {
                    if (wmsRule.RuleType.Equals(WmsCommand.CONN_RULE_TYPE))
                        this.rulesListBox.Items.Add(wmsRule);
                }
                else
                    if (this.compRuleRadioButton.Checked)
                    {
                        if (wmsRule.RuleType.Equals(WmsCommand.COMP_RULE_TYPE))
                            this.rulesListBox.Items.Add(wmsRule);
                    }
                    else
                        if (this.execRuleRadioButton.Checked)
                        {
                            if (wmsRule.RuleType.Equals(WmsCommand.EXEC_RULE_TYPE))
                                this.rulesListBox.Items.Add(wmsRule);
                        }
            }
            this.rulesListBox.Sorted = true;
        }

        private void populateServices()
        {
            this.servicesListBox.Items.Clear();

            foreach (WmsService wmsService in _wmsSystem.WmsServices)
            {
                this.servicesListBox.Items.Add(wmsService);
            }
            this.servicesListBox.Sorted = true;
        }

        private void populateAssociatedConnRules()
        {
            this.associatedRulesIGrid.Rows.Clear();
            this.associatedRulesIGrid.Cols.Clear();
            this.associatedRulesIGrid.Cols.Add("Rules", COLUMN_WIDTH);
            this.associatedRulesIGrid.Cols.Add("Services", COLUMN_WIDTH);

            try
            {
                short row = 0;
                foreach (WmsSystem.ConnRuleAssociation de in _wmsSystem.ConnectionRuleAssociations)
                {
                    if (isInternalWMSObject(de.ruleName) || isInternalWMSObject(de.serviceName))
                    {
                        continue;
                    }
                    this.associatedRulesIGrid.Rows.Add();
                    this.associatedRulesIGrid.Cells[row, 0].Value = de.ruleName;
                    this.associatedRulesIGrid.Cells[row, 1].Value = de.serviceName;
                    row++;
                }
            }
            catch (OdbcException ex)
            {
                MessageBox.Show("Error: Internal WMS error fetching connection rules. \n\n" +
                                "Problem: \t Unable to fetch WMS connection rules. \n\n" +
                                "Solution: \t Please check error details for recovery information. \n\n" +
                                "Details: " + "\t " + ex.Message + "\n\n",
                                "Error fetching Connection Rules", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void populateAssociatedRules()
        {
            if (this.connRuleRadioButton.Checked)
            {
                return;
            }

            this.associatedRulesIGrid.Rows.Clear();
            this.associatedRulesIGrid.Cols.Clear();
            this.associatedRulesIGrid.Cols.Add("Rules", COLUMN_WIDTH);
            this.associatedRulesIGrid.Cols.Add("Services", COLUMN_WIDTH);

            if (this.servicesListBox.SelectedItem != null)
            {
                try
                {
                    WmsService wmsService = (WmsService)this.servicesListBox.SelectedItem;
                    if (wmsService != null)
                    {
                        int row = 0;
                        List<string> ruleNames = new List<string>();
                        if (this.compRuleRadioButton.Checked)
                        {
                            ruleNames = wmsService.CompilationRules;
                        }
                        else
                            ruleNames = wmsService.ExecutionRules;
                        foreach (string ruleName in ruleNames)
                        {
                            this.associatedRulesIGrid.Rows.Add();
                            this.associatedRulesIGrid.Cells[row, 0].Value = ruleName;
                            this.associatedRulesIGrid.Cells[row, 1].Value = wmsService.Name;
                            row++;
                        }
                    }
                }
                catch (OdbcException ex)
                {
                    MessageBox.Show("Internal WMS error fetching rules. \n\n" +
                                    "Problem: \t Unable to fetch WMS rules. \n\n" +
                                    "Solution: \t Please check error details for recovery information. \n\n" +
                                    "Details: " + "\t " + ex.Message + "\n\n",
                                    "Error fetching Rules", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        private void showRule()
        {
            try
            {
                WmsRule wmsRule = (WmsRule)this.rulesListBox.SelectedItem;
                DataTable dataTable = new DataTable();
                dataTable.Columns.Add("Rule Info");
                string createString = wmsRule.CreateCommandString;
                createString = createString.Replace(Environment.NewLine, "");
                dataTable.Rows.Add(new object[] { createString });
                infoIGrid.FillWithData(dataTable);
                adjustIGridRowHeight(this.infoIGrid);

                for (int i = 0; i < infoIGrid.Cols.Count; i++)
                {
                    infoIGrid.Cols[i].AutoWidth();
                    infoIGrid.Cols[i].MinWidth = IGRIDINFO_MIN_WIDTH;
                }
            }
            catch (OdbcException ex)
            {
                MessageBox.Show("Error: Internal WMS error fetching rule details. \n\n" +
                                "Problem: \t Unable to fetch WMS rule details. \n\n" +
                                "Solution: \t Please check error details for recovery information. \n\n" +
                                "Details: " + "\t " + ex.Message + "\n\n",
                                "Error fetching Rule Details", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void buttonShowRule_Click(object sender, EventArgs e)
        {
            showRule();
        }

        private void showService()
        {
            try
            {
                WmsService wmsService = (WmsService)this.servicesListBox.SelectedItem;
                DataTable dataTable = new DataTable();
                dataTable.Columns.Add("Service Info");
                dataTable.Rows.Add(new object[] { wmsService.CreateCommandString });

                infoIGrid.FillWithData(dataTable);
                adjustIGridRowHeight(this.infoIGrid);

                for (int i = 0; i < infoIGrid.Cols.Count; i++)
                {
                    infoIGrid.Cols[i].AutoWidth();
                    infoIGrid.Cols[i].MinWidth = IGRIDINFO_MIN_WIDTH;
                }
            }
            catch (OdbcException ex)
            {
                MessageBox.Show("Error: Internal WMS error fetching service details. \n\n" +
                                "Problem: \t Unable to fetch WMS service details. \n\n" +
                                "Solution: \t Please check error details for recovery information. \n\n" +
                                "Details: " + "\t " + ex.Message + "\n\n",
                                "Error fetching Service Details", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void buttonShowService_Click(object sender, EventArgs e)
        {
            showService();
        }

        private void buttonAssociate_Click(object sender, EventArgs e)
        {
            string ruleName = (string)this.rulesListBox.SelectedItem.ToString();
            bool ruleNameExist = false;
            for (int r = 0; r < this.associatedRulesIGrid.Rows.Count; r++)
            {
                if (this.associatedRulesIGrid.Cells[r, 0].Value.ToString().Equals(ruleName))
                {
                    ruleNameExist = true;
                    break;
                }
            }
            if (ruleNameExist)
            {
                MessageBox.Show("Error: Rule is already assigned. \n\n" +
                                "Problem: \t The selected rule '" + ruleName + "' is already assigned. \n\n" +
                                "Solution: \t Unassign this rule and then re-assign it to the specified service. \n\n",
                                "Error associating Rule", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else
            {
                this.associatedRulesIGrid.Rows.Add();
                int row = this.associatedRulesIGrid.Rows.Count;
                this.associatedRulesIGrid.Cells[row - 1, 0].Value = ruleName;
                string serviceName = ((WmsService)this.servicesListBox.SelectedItem).Name;
                this.associatedRulesIGrid.Cells[row - 1, 1].Value = serviceName;
                previewTextBox.Text = getCommandPreview();
                _associateFlag = true;
            }
        }

        private void buttonMoveUp_Click(object sender, EventArgs e)
        {
            int rowIndex = this.associatedRulesIGrid.SelectedRowIndexes[0];
            if (rowIndex > 0)
            {
                this.associatedRulesIGrid.Rows[rowIndex].Move(rowIndex - 1);
                rowIndex--;
                this.moveUpButton.Enabled = (rowIndex > 0) ? true : false;
                this.moveDownButton.Enabled = (rowIndex < this.associatedRulesIGrid.Rows.Count - 1) ? true : false;
                previewTextBox.Text = getCommandPreview();
            }
        }

        private void buttonMoveDown_Click(object sender, EventArgs e)
        {
            int rowIndex = this.associatedRulesIGrid.SelectedRowIndexes[0];
            if (rowIndex < this.associatedRulesIGrid.Rows.Count - 1)
            {
                this.associatedRulesIGrid.Rows[rowIndex + 1].Move(rowIndex);
                rowIndex++;
                this.moveUpButton.Enabled = (rowIndex > 0) ? true : false;
                this.moveDownButton.Enabled = (rowIndex < this.associatedRulesIGrid.Rows.Count - 1) ? true : false;
                previewTextBox.Text = getCommandPreview();
            }
        }

        private void buttonRemove_Click(object sender, EventArgs e)
        {
            int rowIndex = this.associatedRulesIGrid.SelectedRowIndexes[0];
            this.associatedRulesIGrid.Rows.RemoveAt(rowIndex);
            this.moveUpButton.Enabled = false;
            this.moveDownButton.Enabled = false;
            this.unselectButton.Enabled = false;
            previewTextBox.Text = getCommandPreview();
            _unassociateFlag = true;
        }

        private void buttonRemoveAll_Click(object sender, EventArgs e)
        {
            this.associatedRulesIGrid.Rows.Clear();
            this.moveUpButton.Enabled = false;
            this.moveDownButton.Enabled = false;
            this.unselectButton.Enabled = false;
            previewTextBox.Text = getCommandPreview();
            _unassociateFlag = true;
        }

        //private string getConnectionRuleCommandForR24()
        //{
        //    StringBuilder sb = new StringBuilder();
        //    sb.Append("ALTER WMS CONN (");
        //    bool isSystemRuleAdded = false;

        //    List<WmsSystem.ConnRuleAssociation> systemRuleAssociations = new List<WmsSystem.ConnRuleAssociation>();
        //    for (int i = 0; i < _wmsSystem.ConnectionRuleAssociations.Count; i++)
        //    {
        //        WmsRule connRule = _wmsSystem.FindRule(_wmsSystem.ConnectionRuleAssociations[i].ruleName);
        //        if (connRule != null && connRule.isSystemRule)
        //        {
        //            systemRuleAssociations.Add(_wmsSystem.ConnectionRuleAssociations[i]);
        //        }
        //    }

        //    for (int i = 0; i < systemRuleAssociations.Count; i++)
        //    {
        //        isSystemRuleAdded = true;
        //        sb.Append(systemRuleAssociations[i].ruleName);
        //        sb.Append(" ");
        //        sb.Append(systemRuleAssociations[i].serviceName);
        //        if (i < systemRuleAssociations.Count - 1)
        //            sb.Append(", ");
        //    }

        //    if (isSystemRuleAdded && associatedRulesIGrid.Rows.Count > 0)
        //    {
        //        sb.Append(", ");
        //    }

        //    for (int r = 0; r < this.associatedRulesIGrid.Rows.Count; r++)
        //    {
        //        string ruleName = this.associatedRulesIGrid.Cells[r, 0].Value.ToString();
        //        sb.Append(ruleName);
        //        sb.Append(" ");
        //        string serviceName = this.associatedRulesIGrid.Cells[r, 1].Value.ToString();
        //        sb.Append(serviceName);
        //        if (r < this.associatedRulesIGrid.Rows.Count - 1)
        //            sb.Append(", ");
        //    }
        //    sb.Append(")");
        //    return sb.ToString();
        //}

        private string getCommandPreview()
        {
            StringBuilder sb = new StringBuilder();

            if (this.connRuleRadioButton.Checked)
            {
                //if (_wmsSystem.NeedDefaultRuleAssociations)
                //    return getConnectionRuleCommandForR24();

                sb.Append("ALTER WMS CONN (");
                for (int r = 0; r < this.associatedRulesIGrid.Rows.Count; r++)
                {
                    string ruleName = this.associatedRulesIGrid.Cells[r, 0].Value.ToString();
                    sb.Append(ruleName);
                    sb.Append(" ");
                    string serviceName = this.associatedRulesIGrid.Cells[r, 1].Value.ToString();
                    sb.Append(serviceName);
                    if (r < this.associatedRulesIGrid.Rows.Count - 1)
                        sb.Append(",");
                }
                sb.Append(")");
            }
            else
            {
                if (this.servicesListBox.SelectedItem != null)
                {
                    string serviceName = this.servicesListBox.SelectedItem.ToString();
                    string type = this.compRuleRadioButton.Checked ? "COMP" : "EXEC";
                    sb.Append("ALTER SERVICE ").Append(serviceName).Append(" ").Append(type).Append(" (");
                    for (int r = 0; r < this.associatedRulesIGrid.Rows.Count; r++)
                    {
                        string rn = this.associatedRulesIGrid.Cells[r, 0].Value.ToString();
                        sb.Append(rn);
                        if (r < this.associatedRulesIGrid.Rows.Count - 1)
                            sb.Append(",");
                    }
                    sb.Append(")");
                }
            }
            return sb.ToString();
        }

        private void buttonRefresh_Click(object sender, EventArgs e)
        {
            try
            {
                this.refreshButton.Enabled = false;
                this.Cursor = Cursors.WaitCursor;
                this._wmsSystem.Refresh();

                populateServices();
                populateRules();
                if (this.connRuleRadioButton.Checked)
                {
                    populateAssociatedConnRules();
                }
                else
                {
                    populateAssociatedRules();
                }
                previewTextBox.Text = getCommandPreview();

            }
            catch (Exception exc)
            {
                MessageBox.Show("Error: Error fetching WMS system configuration. \n\n" +
                                "Problem: \t Unable to refresh the WMS system configuration. \n\n" +
                                "Solution: \t Please check error details for recovery information. \n\n" +
                                "Details: " + "\t " + exc.Message + "\n\n",
                                "Refresh Trafodion WMS configuration", MessageBoxButtons.OK, MessageBoxIcon.Error);

            }
            finally
            {
                this.Cursor = Cursors.Default;
                this.refreshButton.Enabled = true;
                _associateFlag = false;
                _unassociateFlag = false;
                ResetButtons();
            }
        }

        private void radioButtonConnRule_CheckedChanged(object sender, EventArgs e)
        {
            populateRules();
            this.servicesToRuleButton.Enabled = false;
            this.showRuleButton.Enabled = false;
            this.associateButton.Enabled = false;

            populateAssociatedConnRules();
            this.moveUpButton.Enabled = false;
            this.moveDownButton.Enabled = false;
            this.unselectButton.Enabled = false;
        }

        private void radioButtonCompRule_CheckedChanged(object sender, EventArgs e)
        {
            populateRules();
            this.servicesToRuleButton.Enabled = false;
            this.showRuleButton.Enabled = false;
            this.associateButton.Enabled = false;

            populateAssociatedRules();
            this.moveUpButton.Enabled = false;
            this.moveDownButton.Enabled = false;
            this.unselectButton.Enabled = false;
        }

        private void radioButtonExecRule_CheckedChanged(object sender, EventArgs e)
        {
            populateRules();
            this.servicesToRuleButton.Enabled = false;
            this.showRuleButton.Enabled = false;
            this.associateButton.Enabled = false;

            populateAssociatedRules();
            this.moveUpButton.Enabled = false;
            this.moveDownButton.Enabled = false;
            this.unselectButton.Enabled = false;

            infoIGrid.Cols.Clear();
            infoIGrid.Rows.Clear();
        }

        private void showRulesToService()
        {
            try
            {
                WmsService wmsService = (WmsService)this.servicesListBox.SelectedItem;
                DataTable dtNew = getAssociatedDataTable(wmsService);
                infoIGrid.FillWithData(dtNew);
                adjustIGridRowHeight(this.infoIGrid);

                for (int i = 0; i < infoIGrid.Cols.Count; i++)
                {
                    infoIGrid.Cols[i].AutoWidth();
                    infoIGrid.Cols[i].MinWidth = IGRIDINFO_MIN_WIDTH;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error: Internal WMS error fetching rules associated to service. \n\n" +
                                "Problem: \t Unable to fetch WMS rules associated to service. \n\n" +
                                "Solution: \t Please check error details for recovery information. \n\n" +
                                "Details: " + "\t " + ex.Message + "\n\n",
                                "Error fetching Associated Rules", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

        }

        private void showServicesToRule()
        {
            try
            {
                WmsRule wmsRule = (WmsRule)this.rulesListBox.SelectedItem;

                DataTable dataTable = new DataTable();
                string[] columns = { "TYPE", "RULE_NAME", "SERVICE_NAME" };
                Type typeString = System.Type.GetType("System.String");
                Type[] types = { typeString, typeString, typeString };
                for (int c = 0; c < columns.Length; c++)
                {
                    DataColumn dcNew = new DataColumn(columns[c], types[c]);
                    dataTable.Columns.Add(dcNew);
                }

                foreach (string serviceName in wmsRule.AssociatedServiceNames)
                {
                    dataTable.Rows.Add(new Object[] { wmsRule.RuleType, wmsRule.Name, serviceName });
                }

                infoIGrid.FillWithData(dataTable);
                adjustIGridRowHeight(this.infoIGrid);

                for (int i = 0; i < infoIGrid.Cols.Count; i++)
                {
                    infoIGrid.Cols[i].AutoWidth();
                    infoIGrid.Cols[i].MinWidth = IGRIDINFO_MIN_WIDTH;
                }
            }
            catch (OdbcException ex)
            {
                MessageBox.Show("Error: Internal WMS error fetching services associated with a rule. \n\n" +
                                "Problem: \t Unable to fetch WMS services associated with a rule. \n\n" +
                                "Solution: \t Please check error details for recovery information. \n\n" +
                                "Details: " + "\t " + ex.Message + "\n\n",
                                "Error fetching WMS Services associated with a Rule",
                                MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void buttonRulesToService_Click(object sender, EventArgs e)
        {
            showRulesToService();
        }

        private void buttonServicesToRule_Click(object sender, EventArgs e)
        {
            showServicesToRule();
        }

        public DataTable getAssociatedDataTable(WmsService wmsService)
        {
            DataTable dtNew = new DataTable();
            string[] columns = { "TYPE", "RULE_NAME", "SERVICE_NAME" };
            Type typeString = System.Type.GetType("System.String");
            Type[] types = { typeString, typeString, typeString };
            for (int c = 0; c < columns.Length; c++)
            {
                DataColumn dcNew = new DataColumn(columns[c], types[c]);
                dtNew.Columns.Add(dcNew);
            }

            if (wmsService != null)
            {
                foreach (WmsSystem.ConnRuleAssociation de in _wmsSystem.ConnectionRuleAssociations)
                {
                    if (de.serviceName.Equals(wmsService.Name))
                    {
                        dtNew.Rows.Add(new Object[] { WmsCommand.CONN_RULE_TYPE, de.ruleName, wmsService.Name });
                    }
                }
                foreach (string ruleName in wmsService.CompilationRules)
                {
                    dtNew.Rows.Add(new Object[] { WmsCommand.COMP_RULE_TYPE, ruleName, wmsService.Name });
                }
                foreach (string ruleName in wmsService.ExecutionRules)
                {
                    dtNew.Rows.Add(new Object[] { WmsCommand.EXEC_RULE_TYPE, ruleName, wmsService.Name });
                }
            }

            return dtNew;
        }

        private void cancelButton_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        //Track if only new associations or only new disassociations were made. If
        //so display a suitable message for the association or disassociation. If both
        //were made then provide a generic message.
        private string getApplyButtonResultMessage()
        {
            string resultMessage = "";
            if ((_associateFlag == true && _unassociateFlag == true) || (_associateFlag == false && _unassociateFlag == false))
            {
                resultMessage = "Rule-service operation succeeded!";
            }
            else if (_associateFlag)
            {
                resultMessage = "Rule associated successfully!";
            }
            else if (_unassociateFlag == true)
            {
                resultMessage = "Rule unassociated successfully!";
            }
            return resultMessage;
        }

        private void applyButton_Click(object sender, EventArgs e)
        {
            try
            {
                this.Cursor = Cursors.WaitCursor;
                string sql = getCommandPreview();

                if (sql.Length > 0)
                {
                    
                    WmsCommand.executeNonQuery(sql, _wmsSystem.ConnectionDefinition);

                    //If connection rule association, clear the connection rule associations
                    //on the system model, so that it is fetched again.
                    if (connRuleRadioButton.Checked)
                    {
                        _wmsSystem.ResetConnectionRuleAssociations();
                    }
                    else
                    {
                        try
                        {
                            if (null != this.servicesListBox.SelectedItem)
                            {
                                WmsService theService = (WmsService)this.servicesListBox.SelectedItem;
                                List<WmsRule> rulesAssocToTheService = new List<WmsRule>();
                                if (this.compRuleRadioButton.Checked)
                                    rulesAssocToTheService = theService.AssociatedCompilationRules;
                                else
                                    rulesAssocToTheService = theService.AssociatedExecutionRules;

                                foreach (WmsRule aRule in rulesAssocToTheService)
                                    aRule.ResetAssociatedServiceNames();
                            }

                        }
                        catch (Exception)
                        {
                        }
                    }
                    for (int r = 0; r < this.associatedRulesIGrid.Rows.Count; r++)
                    {
                        string ruleName = this.associatedRulesIGrid.Cells[r, 0].Value.ToString();
                        WmsRule wmsRule = _wmsSystem.FindRule(ruleName);
                        if (wmsRule != null)
                        {
                            wmsRule.ResetAssociatedServiceNames();
                        }
                    }
                    
                    infoIGrid.Rows.Clear();
                    this.previewTextBox.Text = sql;
                    populateAssociatedRules();
                    this.moveUpButton.Enabled = false;
                    this.moveDownButton.Enabled = false;
                    if (associatedRulesIGrid.SelectedRowIndexes.Count <= 0)
                    {
                        this.unselectButton.Enabled = false;
                    }
                    string message = getApplyButtonResultMessage();
                    MessageBox.Show(message, "Associate Rule",
                                    MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
            }
            catch (OdbcException ex)
            {
                MessageBox.Show("Error: Internal WMS error associating rules. \n\n" +
                                "Problem: \t Unable to associate WMS rules. \n\n" +
                                "Solution: \t Please check error details for recovery information. \n\n" +
                                "Details: " + "\t " + ex.Message + "\n\n",
                                "Error associating Rules", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                this.Cursor = Cursors.Default;
                _associateFlag = false;
                _unassociateFlag = false;
            }
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.AssociateRule);
        }
    }
}
