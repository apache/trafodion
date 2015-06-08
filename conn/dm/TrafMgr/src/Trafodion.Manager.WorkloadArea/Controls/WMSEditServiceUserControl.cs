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
using System.Drawing;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.WorkloadArea.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSEditServiceUserControl : UserControl
    {
        #region Constants
        private const int DIALOG_HEIGHT = 420;
        #endregion Constants

        #region Member variables

        private WmsCommand.WMS_ACTION _operation;
        private WmsSystem _wmsSystem;
        private WmsService _wmsService;
        private EventHandler<WmsObject.WmsModelEventArgs> _wmsServiceModelEventHandler = null;
        private List<WmsRule> originallyAssociatedRules = new List<WmsRule>();
        private Hashtable rules_ht = new Hashtable();
        //private NCCWorkspaceView _view = null;

        #endregion Member variables

        #region Property

        private string CancelQueryIfClientDisappears
        {
            get
            {
                return (string)ddlCancelQueryIfClientDisappears.SelectedItem;
            }
            set
            {
                ddlCancelQueryIfClientDisappears.SelectedItem = value;
            }
        }

        private string CheckQueryEstimatedResourceUse
        {
            get
            {
                return (string)ddlCheckQueryEstimatedResourceUse.SelectedItem;
            }
            set
            {
                ddlCheckQueryEstimatedResourceUse.SelectedItem = value;
            }
        }

        #endregion

        public WMSEditServiceUserControl(WmsCommand.WMS_ACTION operation, WmsSystem wmsSystem, WmsService wmsService)
        {
            InitializeComponent();
            SetGridPattern();

            _wmsSystem = wmsSystem;
            _wmsService = wmsService;
            if (operation == WmsCommand.WMS_ACTION.ALTER_SERVICE)
            {
                _wmsServiceModelEventHandler = new EventHandler<WmsObject.WmsModelEventArgs>(_wmsService_WmsModelEvent);
                _wmsService.WmsModelEvent += _wmsServiceModelEventHandler;
                _cancelButton.Visible = true;
                //_okButton.Location = _cancelButton.Location;

                TrafodionIGridUtils.initIGridRowHeight(this.associatedRulesIGrid);

                associatedRulesWidget.Visible = false; 
                associatedRulesWidget.Visible = true;
                setupRules();
            }

            SetBackwardCompatibility();     
            _operation = operation;
            setupComponent();           
        }

        /// <summary>
        /// Backward compatibility supprot
        /// </summary>
        private void SetBackwardCompatibility()
        {
            // For M8 or later
            lblMaxExecQuery.Visible = nudMaxExecQuery.Visible 
                = lblMaxESP.Visible = nudMaxESP.Visible 
                = this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ130;
                        
            // For M9 or later
            lblCancelQuery.Visible = ddlCancelQueryIfClientDisappears.Visible
                = this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140;

            // For M10 or later
            lblCheckQueryEstimatedResourceUse.Visible = ddlCheckQueryEstimatedResourceUse.Visible
                = this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150;

            if (this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
            {
                _maxCpuComboBox.Items.Add(WmsCommand.EST_MAX_CPU_BUSY);
            }

            // Hide MAX SSD Usage
            _maxSSDUsageLabel.Visible = _maxOverflowUsageNumericUpDown.Visible = false;
        }

        /// <summary>
        /// Put the pattern code here. Those code is easily to be lost when opening the file using visual studio editor
        /// </summary>
        private void SetGridPattern()
        {
            TenTec.Windows.iGridLib.iGColPattern iGColPattern1 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern2 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern3 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern4 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern5 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern6 = new TenTec.Windows.iGridLib.iGColPattern();
            iGColPattern1.AllowGrouping = false;
            iGColPattern1.AllowMoving = false;
            iGColPattern1.CellStyle = this.iGCellStyleDesign1;
            iGColPattern1.DefaultCellImageIndex = 1;
            iGColPattern1.Key = "SelectCheckBox";
            iGColPattern1.MaxWidth = 20;
            iGColPattern1.MinWidth = 20;
            iGColPattern1.SortOrder = TenTec.Windows.iGridLib.iGSortOrder.None;
            iGColPattern1.SortType = TenTec.Windows.iGridLib.iGSortType.ByAuxValue;
            iGColPattern1.Text = "+";
            iGColPattern1.Width = 20;
            iGColPattern2.CellStyle = this.iGCellStyleDesign1;
            iGColPattern2.ColHdrStyle = this.associatedRulesIGridCol6ColHdrStyle;
            iGColPattern2.Key = "RuleGroup";
            iGColPattern2.Text = "Rule Group";
            iGColPattern2.Width = 140;
            iGColPattern3.CustomGrouping = true;
            iGColPattern3.Key = "RuleName";
            iGColPattern3.Text = "Rule Name";
            iGColPattern3.Width = 102;
            iGColPattern4.Key = "RuleType";
            iGColPattern4.Text = "Rule Type";
            iGColPattern4.Width = 74;
            iGColPattern5.Key = "WarnLevel";
            iGColPattern5.SortType = TenTec.Windows.iGridLib.iGSortType.ByText;
            iGColPattern5.Text = "Warn Level";
            iGColPattern5.Width = 90;
            iGColPattern6.CellStyle = this.associatedServicesIGridCol4CellStyle;
            iGColPattern6.ColHdrStyle = this.associatedServicesIGridCol4ColHdrStyle;
            iGColPattern6.Key = "Expression";
            iGColPattern6.Text = "Expression";
            iGColPattern6.Width = 282;
            this.associatedRulesIGrid.Cols.AddRange(new TenTec.Windows.iGridLib.iGColPattern[] {
            iGColPattern1,
            iGColPattern2,
            iGColPattern3,
            iGColPattern4,
            iGColPattern5,
            iGColPattern6});
        }

        private void setupRules()
        {
            //Add each associated connection rule and make them readonly
            foreach (WmsSystem.ConnRuleAssociation connectionRuleAssociation in _wmsSystem.ConnectionRuleAssociations)
            {
                if (connectionRuleAssociation.serviceName.Equals(_wmsService.Name))
                {
                    WmsRule theRule = _wmsSystem.FindRule(connectionRuleAssociation.ruleName);
                    if (theRule != null)
                    {
                        addRuleToGrid(theRule, true, " Connection Rule");
                    }
                }

            }
            foreach (WmsRule theRule in _wmsService.AssociatedCompilationRules)
            {
                addRuleToGrid(theRule, true, "Compilation Rules");
                originallyAssociatedRules.Add(theRule);
            }
            foreach (WmsRule theRule in _wmsService.AssociatedExecutionRules)
            {
                addRuleToGrid(theRule, true, "Execution Rules");
                originallyAssociatedRules.Add(theRule);
            }

            // Add all rules
            foreach (WmsRule theRule in _wmsSystem.WmsCompilationRules)
            {
                if (!originallyAssociatedRules.Contains(theRule))
                    addRuleToGrid(theRule, false, "Compilation Rules");
            }
            foreach (WmsRule theRule in _wmsSystem.WmsExecutionRules)
            {
                if (!originallyAssociatedRules.Contains(theRule))
                    addRuleToGrid(theRule, false, "Execution Rules");
            }

            try
            {
                this.associatedRulesIGrid.GroupObject.Add("RuleGroup");
                this.associatedRulesIGrid.Group();
                this.associatedRulesIGrid.Sort();

                foreach (iGRow aRow in this.associatedRulesIGrid.Rows)
                {
                    if (iGRowType.Normal != aRow.Type)
                    {
                        //   aRow.RowTextCell.BackColor = System.Drawing.Color.FromArgb(192, 235, 240, 248);
                        aRow.RowTextCell.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);
                    }

                }

            }
            catch (Exception e)
            {
                if (Logger.IsTracingEnabled)
                    Logger.OutputToLog("Internal error loading and grouping QueryList IGrid. " +
                                                "Details = " + e.Message + "\n");
            }
        }


        private void setRowBackColor(iGRow aRow, Color bgColor)
        {
            foreach (iGCell aCell in aRow.Cells)
                aCell.BackColor = bgColor;

        }


        private void addRuleToGrid(WmsRule aRule, bool isAssociated, String aRuleGroupName)
        {
            bool setBackgroundColor = false;
            iGRow row = associatedRulesIGrid.Rows.Add();
            if (isAssociated)
            {
                row.Cells["SelectCheckBox"].AuxValue = 1;
                row.Cells["SelectCheckBox"].Value = true;
                if (aRule.RuleType.Equals(WmsCommand.CONN_RULE_TYPE))
                {
                    //To Do: Disable the checkbox for connection rules.
                    row.Cells["SelectCheckBox"].Type = iGCellType.Text;
                    row.Cells["SelectCheckBox"].Value = "";
                    row.Cells["SelectCheckBox"].ImageIndex = -1;
                    setBackgroundColor = true;
                }
            }
            else
            {
                row.Cells["SelectCheckBox"].AuxValue = 0;
            }

            row.Cells["RuleGroup"].Value = aRuleGroupName;
            row.Cells["RuleName"].Value = aRule.Name;
            row.Cells["RuleType"].Value = aRule.RuleType;
            row.Cells["WarnLevel"].Value = aRule.WarnLevel;
            row.Cells["Expression"].Value = aRule.Expression.Replace("\n","");

            // Add the rule to the hash
            // Avoid Adding Duplicated keys to Hash Table
            if(!rules_ht.ContainsKey(aRule.Name))
            {
                rules_ht.Add(aRule.Name, aRule);
            }
            //if (setBackgroundColor)
            //    setRowBackColor(row, Color.WhiteSmoke);

        }

        public Button CancelButton
        {
            get { return this._cancelButton; }
        }

        public Button OKButton 
        {
            get { return this._okButton; }
        }


        void MyDispose(bool disposing)
        {
            if (disposing)
            {
                if (_wmsService != null && _wmsServiceModelEventHandler != null)
                {
                    _wmsService.WmsModelEvent -= _wmsServiceModelEventHandler;
                }
            }
        }

        void _wmsService_WmsModelEvent(object sender, WmsObject.WmsModelEventArgs e)
        {
            if (e.WmsAction == WmsCommand.WMS_ACTION.HOLD_SERVICE ||
                e.WmsAction == WmsCommand.WMS_ACTION.RELEASE_SERVICE ||
                (WmsCommand.WMS_ACTION.STATUS_SERVICE == e.WmsAction) ||
                e.WmsAction == WmsCommand.WMS_ACTION.START_SERVICE ||
                e.WmsAction == WmsCommand.WMS_ACTION.STOP_SERVICE )
                
            {
                //Update the state from the model
                getServiceInfo();
            }

            if (e.WmsAction == WmsCommand.WMS_ACTION.ALTER_SERVICE ||
                e.WmsAction == WmsCommand.WMS_ACTION.ADD_SERVICE)
            {
                //MessageBox.Show("Hello");
                getServiceInfo();
            }
        }


        private bool supportsPriorityOverride()
        {
            //ConnectionDefinition.SERVER_VERSION theWMSVersion = _wmsSystem.ConnectionDefinition.ServerVersion;

            //return WMSUtils.isWMSOnR24AndLater(theWMSVersion) && (ConnectionDefinition.SERVER_VERSION.R293 != theWMSVersion);
            
            return false;
        }  /*  End of  supportsPriorityOverride  method.  */



        private void setupComponent()
        {
            ddlCancelQueryIfClientDisappears.Items.AddRange(new string[] { WmsCommand.SWITCH_ON, WmsCommand.SWITCH_OFF });
            ddlCheckQueryEstimatedResourceUse.Items.AddRange(new string[] { WmsCommand.SWITCH_ON, WmsCommand.SWITCH_OFF});

            //Widget Canvas            
            GridLayoutManager gridLayoutManager = new GridLayoutManager(3, 1);
            gridLayoutManager.CellSpacing = 4;
            this.associatedRulesWidget.LayoutManager = gridLayoutManager;
            int myWidth = this.Size.Width;
            int myHeight = this.Size.Height;

            //1. SQL Default
            GridConstraint gridConstraint = new GridConstraint(0, 0, 1, 1);            
            WidgetContainer widgetContainerSqlDefaults = new WidgetContainer(associatedRulesWidget, _sqlDefaultsTextBox, "SQL Defaults");
            widgetContainerSqlDefaults.Name = "SQL Defaults";
            widgetContainerSqlDefaults.AllowDelete = false;
            widgetContainerSqlDefaults.Font = new Font(widgetContainerSqlDefaults.Font, FontStyle.Bold);
            //widgetContainer.BackColor = Color.WhiteSmoke;            
            this.associatedRulesWidget.AddWidget(widgetContainerSqlDefaults, gridConstraint, -1);

            //2. Associated Rules
            gridConstraint = new GridConstraint(1, 0, 2, 1);
            WidgetContainer widgetContainerAssociatedRules = new WidgetContainer(associatedRulesWidget, this.associatedRulesIGrid, "Associated Rules");
            widgetContainerAssociatedRules.Name = "Associated Rules";
            widgetContainerAssociatedRules.AllowDelete = false;
            //widgetContainerAssociatedRules.BackColor = Color.WhiteSmoke;
            widgetContainerAssociatedRules.Font = new Font(widgetContainerAssociatedRules.Font, FontStyle.Bold);
            this.associatedRulesWidget.AddWidget(widgetContainerAssociatedRules, gridConstraint, -1);

            this.associatedRulesWidget.InitializeCanvas();


            if (!this._wmsSystem.AdminCanSetSQLDefaultsOnService)
            {
                hideServicesControls();
                widgetContainerSqlDefaults.Visible = false;
                SetBounds(Bounds.X, Bounds.Y, Bounds.Width, DIALOG_HEIGHT);
            }

            if (_operation == WmsCommand.WMS_ACTION.ADD_SERVICE)
            {
                Text = "Add Service";
                _okButton.Text = "&Add";
                _stateLabel.Visible = _stateTextBox.Visible = false;

                //if (supportsPriorityOverride() &&
                //    this._wmsSystem.ConnectionDefinition.IsWmsAdminRole)
                //    overrideDSNPriorityCheckBox.Visible = true;

                //hideAssociatedRulesControls();
                widgetContainerAssociatedRules.Visible = false;
                SetBounds(Bounds.X, Bounds.Y, Bounds.Width, DIALOG_HEIGHT);
            }
            else
            {
                Text = "Alter Service";
                _okButton.Text = "&Alter";
                _serviceNameTextBox.ReadOnly = true;
                if (_wmsService.isSystemService)
                {
                    _servicePriorityComboBox.Enabled = false;
                    _fromHHMaskedTextbox.ReadOnly = true;
                    _fromMMMaskedTextbox.ReadOnly = true;
                    _toHHMaskedTextbox.ReadOnly = true;
                    _toMMMaskedTextbox.ReadOnly = true;
                }
            }

            try
            {
                if (WmsCommand.WMS_ACTION.ADD_SERVICE == this._operation)
                {
                    _servicePriorityComboBox.SelectedItem = "MEDIUM";
                    getSystemInfo();
                    DateTime dt = DateTime.Now;
                    //"12/11/2008 <Time> : Added by super.services"
                    StringBuilder sb = new StringBuilder();
                    sb.Append(dt.ToShortDateString()).Append(" ").Append(dt.ToShortTimeString());
                    sb.Append(": Added by ").Append(_wmsSystem.ConnectionDefinition.UserName);
                    _commentTextBox.Text = sb.ToString();
                }
                else
                {
                    getServiceInfo();
                    if (_commentTextBox.Text.Length == 0)
                    {
                        DateTime dt = DateTime.Now;
                        //"12/11/2008 <Time> : Added by super.services"
                        StringBuilder sb = new StringBuilder();
                        sb.Append(dt.ToShortDateString()).Append(" ").Append(dt.ToShortTimeString());
                        sb.Append(": Altered by ").Append(_wmsSystem.ConnectionDefinition.UserName);
                        _commentTextBox.Text = sb.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(),"Failed to fetch WMS Service information. \n\n" +
                                "Problem: \t Unable to get WMS service information. \n\n" +
                                "Solution: \t Please check error details for recovery information. \n\n" +
                                "Details: " + "\t " + ex.Message + "\n\n",
                                "Fetch WMS Service Information", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

        }



        public void restorePreviousSettings()
        {
            WidgetCanvas[] resizableWidgets = { this.associatedRulesWidget };
            foreach (WidgetCanvas theWidget in resizableWidgets)
            {
                //restoreWidgetSettings(theWidget);
                theWidget.SizeChanged += new EventHandler(widget_SizeChanged);
                theWidget.LocationChanged += new EventHandler(widget_LocationChanged);
            }

        }


        private void widget_LocationChanged(Object sender, EventArgs e)
        {
            try
            {
                String widgetName = ((WidgetCanvas)sender).Name;

                if (DockStyle.Fill == this.Controls[widgetName].Dock)
                    return;

                //NCCWorkspace ws = this._wmsSystem.CurrentWorkspace;
                //if (null != ws)
                //{
                //    String thePrefix = this.Name + "." + widgetName + ".";
                //    ws.replaceCustomSettings(thePrefix + "XPos", this.Controls[widgetName].Location.X);
                //    ws.replaceCustomSettings(thePrefix + "YPos", this.Controls[widgetName].Location.Y);
                //}

            }
            catch (Exception)
            {
            }

        }

        private void Close(DialogResult result)
        {
            if (Parent != null && Parent is Form)
            {
                ((Form)Parent).DialogResult = result;
                ((Form)Parent).Close();
            }
        }

        private void widget_SizeChanged(Object sender, EventArgs e)
        {
            try
            {
                String widgetName = ((WidgetCanvas)sender).Name;

                if (DockStyle.Fill == this.Controls[widgetName].Dock)
                    return;

                //NCCWorkspace ws = this._wmsSystem.CurrentWorkspace;
                //if (null != ws)
                //{
                //    String thePrefix = this.Name + "." + widgetName + ".";
                //    ws.replaceCustomSettings(thePrefix + "Width", this.Controls[widgetName].Width);
                //    ws.replaceCustomSettings(thePrefix + "Height", this.Controls[widgetName].Height);
                //}

            }
            catch (Exception)
            {
            }

        }


        private void restoreWidgetSettings(WidgetCanvas theWidget)
        {
            //String widgetName = theWidget.Name;

            //NCCWorkspace ws = null;

            //try { ws = this._wmsSystem.CurrentWorkspace; }
            //catch (Exception) { }

            //if (null == ws)
            //    return;

            //try
            //{
            //    String thePrefix = this.Name + "." + widgetName + ".";

            //    Object theHeight = ws.getCustomSettings(thePrefix + "Height");
            //    if (null != theHeight) theWidget.Height = (int)theHeight;

            //    Object theWidth = ws.getCustomSettings(thePrefix + "Width");
            //    if (null != theWidth) theWidget.Width = (int)theWidth;

            //}
            //catch (Exception)
            //{
            //}


            //try
            //{
            //    String thePrefix = this.Name + "." + widgetName + ".";

            //    Object xPos = ws.getCustomSettings(thePrefix + "XPos");
            //    Object yPos = ws.getCustomSettings(thePrefix + "YPos");

            //    if ((null != xPos) && (null != yPos))
            //        theWidget.Location = new Point((int)xPos, (int)yPos);

            //}
            //catch (Exception)
            //{
            //}

        }



        public void hideR24Controls()
        {
            _maxRowsFetchedLabel.Visible = _maxRowsFetchedUpDown.Visible = false;
            _holdTimeOutLabel.Visible = _holdTimeOutNumericUpDown.Visible = false;
        }

        public void hideServicesControls()
        {
            this._sqlDefaultsTextBox.Visible = false;
        }

        public void hideAssociatedRulesControls()
        {
            //this.associatedRulesWidget.Visible = false;
            this.associatedRulesIGrid.Visible = false;
        }

        private void getSystemInfo()
        {
            _maxCpuComboBox.Text = _wmsSystem.MaxCpuBusy.ToString();
            _maxMemoryUsageNumericUpDown.Value = _wmsSystem.MaxMemUsage;
            this._maxOverflowUsageNumericUpDown.Value = _wmsSystem.MaxSSDUsage;

            _maxRowsFetchedUpDown.Value = _wmsSystem.MaxRowsFetched;
            this._execTimeOutNumericUpDown.Value = _wmsSystem.ExecTimeout;
            this._waitTimeOutNumericUpDown.Value = _wmsSystem.WaitTimeout;
            this._holdTimeOutNumericUpDown.Value = _wmsSystem.HoldTimeout;
            nudMaxExecQuery.Value = _wmsSystem.MaxExecQueries;
            nudMaxESP.Value = _wmsSystem.MaxEsps;

            // For M9 or later
            if (this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
            {
                CancelQueryIfClientDisappears = _wmsSystem.CancelQueryIfClientDisappears;
            }

            // For M10 or later
            if (this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                CheckQueryEstimatedResourceUse = _wmsSystem.CheckQueryEstimatedResourceUse;
            }
        }

        private void getServiceInfo()
        {
            this._serviceNameTextBox.Text = _wmsService.Name;
            this._stateTextBox.Text = _wmsService.State;
            this._servicePriorityComboBox.Text = _wmsService.Priority;
            this._maxCpuComboBox.Text = _wmsService.MaxCpuBusy;
            this._maxMemoryUsageNumericUpDown.Value = _wmsService.MaxMemUsage;
            nudMaxExecQuery.Value = _wmsService.MaxExecQueries;
            nudMaxESP.Value = _wmsService.MaxEsps;

            // For M9 or later
            if (this._wmsService.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
            {
                CancelQueryIfClientDisappears = _wmsService.CancelQueryIfClientDisappears;
            }

            // For M10 or later
            if (this._wmsService.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                CheckQueryEstimatedResourceUse = _wmsService.CheckQueryEstimatedResourceUse;
            }

            string activeTime = _wmsService.ActiveTime;
            MatchCollection mc1 = Regex.Matches(activeTime, "([0-9]+):([0-9]+)");
            Match fromMatch = mc1[0]; // FROM time match
            Match toMatch = mc1[1]; // TO time match
            _fromHHMaskedTextbox.Text = fromMatch.Groups[1].Value;
            _fromMMMaskedTextbox.Text = fromMatch.Groups[2].Value;
            _toHHMaskedTextbox.Text = toMatch.Groups[1].Value;
            _toMMMaskedTextbox.Text = toMatch.Groups[2].Value;
            this._sqlPlanCheckBox.Checked = _wmsService.IsSqlPlan;
            this._sqlTextCheckBox.Checked = _wmsService.IsSqlText;
            this._execTimeOutNumericUpDown.Value = _wmsService.ExecTimeOut;
            this._waitTimeOutNumericUpDown.Value = _wmsService.WaitTimeOut;
            this._maxOverflowUsageNumericUpDown.Value = _wmsService.MaxSSDUsage;

            this._maxRowsFetchedUpDown.Value = _wmsService.MaxRowsFetched;
            this._holdTimeOutNumericUpDown.Value = _wmsService.HoldTimeOut;

            if (this._wmsSystem.AdminCanSetSQLDefaultsOnService)
                this._sqlDefaultsTextBox.Text = _wmsService.SqlDefaults;


            string comment = _wmsService.Comment;
            if (!comment.Equals("''"))
            {
                this._commentTextBox.Text = comment;
            }
        }

        private bool validateActiveTime()
        {
            int fromHHTime = -1;
            int fromMMTime = -1;
            try
            {
                if (_fromHHMaskedTextbox.Text.Length == 2)
                {
                    fromHHTime = int.Parse(_fromHHMaskedTextbox.Text.Trim());
                }
                if (_fromMMMaskedTextbox.Text.Length == 2)
                {
                    fromMMTime = int.Parse(_fromMMMaskedTextbox.Text.Trim());
                }
            }
            catch (Exception)
            {
            }

            if (fromHHTime < 0 || fromHHTime > 24 || fromMMTime < 0 || fromMMTime > 59 || (fromHHTime == 24 && fromMMTime > 0))
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "Invalid service active time information. \n\n" +
                                "Problem: \t The specified active time range is invalid. \n\n" +
                                "Solution: \t Please specify the Active 'From Time' between 00:00 to 24:00 [HH:MM format]. \n\n",
                                "Alter WMS Service Information", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return false;
            }

            int toHHTime = -1;
            int toMMTime = -1;
            try
            {
                if (_toHHMaskedTextbox.Text.Length == 2)
                {
                    toHHTime = int.Parse(_toHHMaskedTextbox.Text.Trim());
                }
                if (_toMMMaskedTextbox.Text.Length == 2)
                {
                    toMMTime = int.Parse(_toMMMaskedTextbox.Text.Trim());
                }
            }
            catch (Exception)
            {
            }

            if (toHHTime < 0 || toHHTime > 24 || toMMTime < 0 || toMMTime > 59 || (toHHTime == 24 && toMMTime > 0))
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "Invalid service active time information. \n\n" +
                                "Problem: \t The specified active time range is invalid. \n\n" +
                                "Solution: \t Please specify the Active 'To Time' between 00:00 to 24:00 [HH:MM format]. \n\n",
                                "Alter WMS Service Information", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return false;
            }

            if (fromHHTime > toHHTime || (fromHHTime == toHHTime && fromMMTime >= toMMTime))
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "Invalid service active time information. \n\n" +
                                "Problem: \t The specified active time range is invalid. \n\n" +
                                "Solution: \t Please specify the Active 'To Time' which is later than the 'From Time'. \n\n",
                                "Alter WMS Service Information", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Fills the service model with information from the UI
        /// </summary>
        /// <returns></returns>
        private bool getInfoFromControls()
        {
            if (_operation == WmsCommand.WMS_ACTION.ADD_SERVICE)
            {
                _wmsService = new WmsService(_wmsSystem);
                _wmsService.Name = this._serviceNameTextBox.Text.Trim();
                if (string.IsNullOrEmpty(_wmsService.Name))
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), "Invalid service name specified. \n\n" +
                                    "Problem: \t No service name was specified. \n\n" +
                                    "Solution: \t Please specify a service name and retry the Add Service operation. \n\n",
                                    "Add WMS Service", MessageBoxButtons.OK, MessageBoxIcon.Error);

                    this._serviceNameTextBox.Focus();
                    return false;
                }
                else if (_wmsService.isSystemService)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), "Invalid service name specified. \n\n" +
                                    "Problem: \t The service name is a reserved name. \n\n" +
                                    "Solution: \t Please specify a valid service name and retry the Add Service operation. \n\n",
                                    "Add WMS Service", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    this._serviceNameTextBox.Focus();
                    return false;
                }
            }
            _wmsService.Priority = this._servicePriorityComboBox.Text;

            try
            {
                int serviceBusy = int.Parse(_maxCpuComboBox.Text.Trim());
                if(serviceBusy >= 0 && serviceBusy <= 100)
                {
                    _wmsService.MaxCpuBusy = serviceBusy.ToString();
                }
                else
                {
                    MessageBox.Show("Maximum processor utilization should be a number between 0 and 100.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    this._maxCpuComboBox.Focus();
                    return false;
                }
            }
            catch(Exception ex)
            {
                if(_wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
                {
                    if (_maxCpuComboBox.Text.Trim().Equals(WmsCommand.EST_MAX_CPU_BUSY))
                    {
                        _wmsService.MaxCpuBusy = _maxCpuComboBox.Text.Trim();
                    }
                    else
                    {
                        MessageBox.Show("Maximum processor utilization should be set to " + WmsCommand.EST_MAX_CPU_BUSY +" or a number between 0 and 100.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        this._maxCpuComboBox.Focus();
                        return false;
                    }
                }
                else
                {
                    MessageBox.Show("Maximum processor utilization should be a number between 0 and 100.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    this._maxCpuComboBox.Focus();
                    return false;
                }
            }


            _wmsService.MaxMemUsage = int.Parse(this._maxMemoryUsageNumericUpDown.Text);
            _wmsService.MaxSSDUsage = int.Parse(this._maxOverflowUsageNumericUpDown.Text);
            _wmsService.MaxExecQueries = int.Parse(nudMaxExecQuery.Text);
            _wmsService.MaxEsps = int.Parse(nudMaxESP.Text);

            // For M9 or later
            if (this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
            {
                _wmsService.CancelQueryIfClientDisappears = CancelQueryIfClientDisappears;
            }

            // For M10 or later
            if (this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                _wmsService.CheckQueryEstimatedResourceUse = CheckQueryEstimatedResourceUse;
            }

            if (supportsPriorityOverride())
                _wmsService.OverrideDSNPriority = overrideDSNPriorityCheckBox.Checked;

            if (!validateActiveTime())
                return false;

            _wmsService.ActiveTime = _fromHHMaskedTextbox.Text + ":" + _fromMMMaskedTextbox.Text + " TO " +
                         _toHHMaskedTextbox.Text + ":" + _toMMMaskedTextbox.Text;

            _wmsService.IsSqlPlan = this._sqlPlanCheckBox.Checked;
            _wmsService.IsSqlText = this._sqlTextCheckBox.Checked;
            _wmsService.ExecTimeOut = int.Parse(this._execTimeOutNumericUpDown.Text);
            _wmsService.WaitTimeOut = int.Parse(this._waitTimeOutNumericUpDown.Text);
            _wmsService.Comment = TrafodionIGridUtils.ConvertBreakToBlank(this._commentTextBox.Text.Trim());

            try
            {
                _wmsService.MaxRowsFetched = Int64.Parse(this._maxRowsFetchedUpDown.Text);

            }
            catch (Exception exc)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "Invalid maximum rows (fetch limit) specified. \n\n" +
                                "Problem: \t The maximum rows or fetch limit was invalid. \n\n" +
                                "Solution: \t Please specify a valid value for the maximum rows. \n\n" +
                                "Details:  \t " + exc.Message + "\n\n",
                                "Invalid Max Rows or Fetch Limit", MessageBoxButtons.OK, MessageBoxIcon.Error);
                this._maxRowsFetchedUpDown.Focus();
                return false;
            }

            _wmsService.HoldTimeOut = int.Parse(this._holdTimeOutNumericUpDown.Text);

            if (this._wmsSystem.AdminCanSetSQLDefaultsOnService)
            {
                String sqlDefaultsCmd = "";
                foreach (String aLine in this._sqlDefaultsTextBox.Lines)
                {
                    String trimmedLine = aLine.Trim();
                    if (0 < trimmedLine.Length)
                    {
                        //  If we already added a line -- separate the lines by spaces for readability.
                        if (0 < sqlDefaultsCmd.Length)
                            sqlDefaultsCmd += " ";

                        sqlDefaultsCmd += trimmedLine;
                    }
                }

                if ((0 < sqlDefaultsCmd.Length) && !sqlDefaultsCmd.EndsWith(";"))
                    sqlDefaultsCmd += ";";

                _wmsService.SqlDefaults = sqlDefaultsCmd;
            }


            return true;
        }

        private void doAddService()
        {
            if (!getInfoFromControls())
            {
                return;
            }

            try
            {
                this._okButton.Enabled = false;
                this.Cursor = Cursors.WaitCursor;
                _wmsService.Add();

                string msg = "Service " + _wmsService.Name + " added successfully";
                MessageBox.Show(Utilities.GetForegroundControl(), msg, "Add Service", MessageBoxButtons.OK, MessageBoxIcon.Information);
                Close(DialogResult.OK);
            }
            catch (Exception addExc)
            {
                if (Logger.IsTracingEnabled)
                    Logger.OutputToLog("Failed to add the WMS service. Will retry without the override option as " +
                                                "it may not be supported. Details = " + addExc.Message);

                MessageBox.Show(Utilities.GetForegroundControl(), "Failed to add the WMS service. \n\n" +
                                "Problem: \t Unable to add the new WMS service. \n\n" +
                                "Solution: \t Please check error details for recovery information. \n\n" +
                                "Details: " + "\t Service Name = " + _wmsService.Name + "\n" +
                                "         \t " + addExc.Message + "\n\n",
                                "Add WMS Service", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                this.Cursor = Cursors.Default;
                this._okButton.Enabled = true;
                //this._wmsSystem.ConnectionDefinition=
            }
        }

        private void doAlterService()
        {
            //Take a backup of the model incase the alter fails
            WmsService savedCopy = (WmsService)_wmsService.Clone();

            //Now read the values from the UI controls into the model
            if (!getInfoFromControls())
            {
                return;
            }

            try
            {
                this._okButton.Enabled = false;
                this.Cursor = Cursors.WaitCursor;
                _wmsService.Alter();

                // Associate Rules
                associateRules();

                string message = String.Format("Service {0} altered sucessfully", _wmsService.Name);
                MessageBox.Show(Utilities.GetForegroundControl(), message, "Alter Service", MessageBoxButtons.OK, MessageBoxIcon.Information);
                Close(DialogResult.OK);
            }
            catch (Exception ex)
            {
                //Since alter failed, restore the attributes from the backup to the active model
                Utilities.CopyProperties(savedCopy, ref _wmsService);

                if (Logger.IsTracingEnabled)
                    Logger.OutputToLog("Failed to alter the WMS service. Details = " + ex.Message);

                MessageBox.Show(Utilities.GetForegroundControl(), "Failed to alter the WMS service configuration. \n\n" +
                                "Problem: \t Unable to modify the WMS service configuration. \n\n" +
                                "Solution: \t Please check error details for recovery information. \n\n" +
                                "Details: " + "\t Service Name = " + _wmsService.Name + "\n" +
                                "         \t " + ex.Message + "\n\n",
                                "Alter WMS Service", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                this.Cursor = Cursors.Default;
                this._okButton.Enabled = true;
            }
        }

        private void _alterButton_Click(object sender, EventArgs e)
        {
            if (WmsCommand.WMS_ACTION.ADD_SERVICE == this._operation)
            {
                doAddService();
            }
            else
            {
                doAlterService();
            }
            //Refresh DataGrid in ServiceSummaryUserControl
            //Parent is WMSEditServiceDialog; Parent.Parent is ServiceSummaryUserControl
            //((ServicesSummaryUserControl)Parent.Parent).ConnectionDefn = this._wmsSystem.ConnectionDefinition;
        }

        private void _cancelButton_Click(object sender, EventArgs e)
        {
            //If control is hosted inside a form, close the form
            Close(DialogResult.Cancel);
        }

        private void associatedRulesIGrid_CellClick(object sender, iGCellClickEventArgs e)
        {
            int ruleIndex = e.RowIndex;

            changeQueryGroupFolderIcon(ruleIndex);
            if ((associatedRulesIGrid.Cols["SelectCheckBox"].Index == e.ColIndex) &&
                (iGCellType.Text != associatedRulesIGrid.Rows[e.RowIndex].Cells["SelectCheckBox"].Type))
                toggleSelectedRule(ruleIndex);
        }

        private void toggleSelectedRule(int rowNumber)
        {
            if ((0 > rowNumber) ||
                (this.associatedRulesIGrid.Rows.Count <= rowNumber))
                return;

            iGCell checkbox = this.associatedRulesIGrid.Rows[rowNumber].Cells["SelectCheckBox"];
            if (null == checkbox)
                return;

            if (null == checkbox.Value)
                checkbox.Value = true;
            else
                checkbox.Value = null;
        }

        private void changeQueryGroupFolderIcon(int rowIndex)
        {
            try
            {
                iGRow aRow = associatedRulesIGrid.Rows[rowIndex];

                if (1 >= aRow.RowTextCell.ImageIndex)
                {
                    bool isExpanded = aRow.Expanded;
                    aRow.RowTextCell.ImageIndex = isExpanded ? 1 : 0;
                }

            }
            catch (Exception)
            {
            }

        }


        private void associateRules()
        {
            StringBuilder sb = new StringBuilder();
            List<WmsRule> associatedConnectionRules = new List<WmsRule>();
            List<WmsRule> associatedCompilationRules = new List<WmsRule>();
            List<WmsRule> associatedExecutionRules = new List<WmsRule>();
            List<WmsRule> alteredRules = new List<WmsRule>();

            // Figure out which rules are selected and put in buckets
            for (int i = 0; i < this.associatedRulesIGrid.Rows.Count; i++)
            {
                String ruleName = associatedRulesIGrid.Cells[i, "RuleName"].Value as string;
                if (ruleName != null)
                {
                    WmsRule theRule = rules_ht[ruleName] as WmsRule;
                    if (associatedRulesIGrid.Cells[i, "SelectCheckBox"].Value != null)
                    {
                        if (theRule != null)
                        {
                            if (theRule.RuleType.Equals(WmsCommand.COMP_RULE_TYPE))
                            {
                                associatedCompilationRules.Add(theRule);
                            }
                            else if (theRule.RuleType.Equals(WmsCommand.EXEC_RULE_TYPE))
                            {
                                associatedExecutionRules.Add(theRule);
                            }

                            // Check if rule was changed
                            if (!originallyAssociatedRules.Contains(theRule))
                                alteredRules.Add(theRule);
                        }
                    }
                    else
                    {
                        if (originallyAssociatedRules.Contains(theRule))
                        {
                            alteredRules.Add(theRule);
                        }
                    }
                }
            }

            // Alter compilation rules
            sb = new StringBuilder();
            sb.Append("ALTER SERVICE ").Append(_wmsService).Append(" ").Append(WmsCommand.COMP_RULE_TYPE).Append(" (");
            int counter = 0;
            foreach (WmsRule theRule in associatedCompilationRules)
            {
                sb.Append(theRule.Name);
                if (counter < associatedCompilationRules.Count - 1)
                    sb.Append(",");
                counter++;
            }
            sb.Append(")");

            Connection connection = new Connection(_wmsSystem.ConnectionDefinition);
            WmsCommand.executeNonQuery(sb.ToString(), connection.OpenOdbcConnection, 60);


            // Alter Execution Rules
            sb = new StringBuilder();
            sb.Append("ALTER SERVICE ").Append(_wmsService).Append(" ").Append(WmsCommand.EXEC_RULE_TYPE).Append(" (");
            counter = 0;
            foreach (WmsRule theRule in associatedExecutionRules)
            {
                sb.Append(theRule.Name);
                if (counter < associatedExecutionRules.Count - 1)
                    sb.Append(",");
                counter++;
            }
            sb.Append(")");

            // Execute
            WmsCommand.executeNonQuery(sb.ToString(), connection.OpenOdbcConnection, 60);

            // Refresh the rule/service associations of the altered rules.
            refreshRuleAssociations(alteredRules);

        }

        private void refreshRuleAssociations(List<WmsRule> alteredRules)
        {
            foreach (WmsRule theRule in alteredRules)
            {
                theRule.ResetAssociatedServiceNames();
            }
        }


        private TextBox findTextBoxWithTheFocus()
        {
            if (this._sqlDefaultsTextBox.Focused)
                return this._sqlDefaultsTextBox;

            if (this._commentTextBox.Focused)
                return this._commentTextBox;

            if (this._serviceNameTextBox.Focused)
                return this._serviceNameTextBox;

           

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
            if (_operation == WmsCommand.WMS_ACTION.ALTER_SERVICE)
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.ServiceAttributes);
            }
            else
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.ServiceAttributes);
            }
        }
    }
}
