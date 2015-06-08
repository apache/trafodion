// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using Trafodion.Manager.Framework;
namespace Trafodion.Manager.WorkloadArea.Controls
{
    partial class WMSEditServiceUserControl
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            MyDispose(disposing);
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WMSEditServiceUserControl));
            this.iGCellStyleDesign1 = new TenTec.Windows.iGridLib.iGCellStyleDesign();
            this.nccWorkbenchControlImageList = new System.Windows.Forms.ImageList(this.components);
            this.associatedRulesIGridCol6ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle();
            this.associatedServicesIGridCol4CellStyle = new TenTec.Windows.iGridLib.iGCellStyle();
            this.associatedServicesIGridCol4ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle();
            this.iGCellStyleDesign2 = new TenTec.Windows.iGridLib.iGCellStyleDesign();
            this._sqlDefaultsTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.associatedRulesWidget = new Trafodion.Manager.Framework.WidgetCanvas();
            this.associatedRulesIGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this._commentsGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._commentTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.overrideDSNPriorityCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._buttonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._okButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._thresholdsGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.ddlCheckQueryEstimatedResourceUse = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.ddlCancelQueryIfClientDisappears = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.lblCheckQueryEstimatedResourceUse = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.lblCancelQuery = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._maxCpuComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._maxRowsFetchedUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.lblMaxESP = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.nudMaxESP = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.lblMaxExecQuery = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.nudMaxExecQuery = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._maxOverflowUsageNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._maxSSDUsageLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._maxRowsFetchedLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.label8 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.label6 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._maxMemoryUsageNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._stateLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.label5 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.label11 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._servicePriorityComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._stateTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._serviceNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._optionsGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._sqlPlanCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._sqlTextCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._activeTimeGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._toMMMaskedTextbox = new Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox();
            this._fromMMMaskedTextbox = new Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox();
            this.label9 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.label3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.label1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._fromHHMaskedTextbox = new Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox();
            this._toHHMaskedTextbox = new Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox();
            this.label2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._timeoutGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._holdTimeOutNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._holdTimeOutLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.label4 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._execTimeOutNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.label10 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._waitTimeOutNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.nccWMSEditServiceToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.associatedRulesWidget.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.associatedRulesIGrid)).BeginInit();
            this._commentsGroupBox.SuspendLayout();
            this._buttonPanel.SuspendLayout();
            this._thresholdsGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._maxRowsFetchedUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nudMaxESP)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nudMaxExecQuery)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._maxOverflowUsageNumericUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._maxMemoryUsageNumericUpDown)).BeginInit();
            this._optionsGroupBox.SuspendLayout();
            this._activeTimeGroupBox.SuspendLayout();
            this._timeoutGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._holdTimeOutNumericUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._execTimeOutNumericUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._waitTimeOutNumericUpDown)).BeginInit();
            this.TrafodionPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // iGCellStyleDesign1
            // 
            this.iGCellStyleDesign1.ImageList = this.nccWorkbenchControlImageList;
            this.iGCellStyleDesign1.Type = TenTec.Windows.iGridLib.iGCellType.Check;
            // 
            // nccWorkbenchControlImageList
            // 
            this.nccWorkbenchControlImageList.ColorDepth = System.Windows.Forms.ColorDepth.Depth8Bit;
            this.nccWorkbenchControlImageList.ImageSize = new System.Drawing.Size(16, 16);
            this.nccWorkbenchControlImageList.TransparentColor = System.Drawing.Color.Transparent;
            // 
            // associatedRulesIGridCol6ColHdrStyle
            // 
            this.associatedRulesIGridCol6ColHdrStyle.ForeColor = System.Drawing.Color.Black;
            // 
            // _sqlDefaultsTextBox
            // 
            this._sqlDefaultsTextBox.BackColor = System.Drawing.SystemColors.Window;
            this._sqlDefaultsTextBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._sqlDefaultsTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sqlDefaultsTextBox.Location = new System.Drawing.Point(0, 392);
            this._sqlDefaultsTextBox.Margin = new System.Windows.Forms.Padding(3, 0, 3, 5);
            this._sqlDefaultsTextBox.MaxLength = 1024;
            this._sqlDefaultsTextBox.Multiline = true;
            this._sqlDefaultsTextBox.Name = "_sqlDefaultsTextBox";
            this._sqlDefaultsTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this._sqlDefaultsTextBox.Size = new System.Drawing.Size(731, 63);
            this._sqlDefaultsTextBox.TabIndex = 71;
            // 
            // associatedRulesWidget
            // 
            this.associatedRulesWidget.ActiveWidget = null;
            this.associatedRulesWidget.AllowDelete = false;
            this.associatedRulesWidget.AllowDrop = true;
            this.associatedRulesWidget.Controls.Add(this.associatedRulesIGrid);
            this.associatedRulesWidget.Dock = System.Windows.Forms.DockStyle.Fill;
            this.associatedRulesWidget.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.associatedRulesWidget.LayoutManager = null;
            this.associatedRulesWidget.Location = new System.Drawing.Point(0, 455);
            this.associatedRulesWidget.LockBackColor = System.Drawing.SystemColors.Control;
            this.associatedRulesWidget.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.associatedRulesWidget.MinimumSize = new System.Drawing.Size(150, 150);
            this.associatedRulesWidget.Name = "associatedRulesWidget";
            this.associatedRulesWidget.Padding = new System.Windows.Forms.Padding(4);
            this.associatedRulesWidget.Size = new System.Drawing.Size(731, 171);
            this.associatedRulesWidget.TabIndex = 62;
            this.associatedRulesWidget.TabStop = false;
            this.associatedRulesWidget.ThePersistenceKey = null;
            this.associatedRulesWidget.UnlockBackColor = System.Drawing.Color.Azure;
            this.associatedRulesWidget.ViewName = null;
            this.associatedRulesWidget.ViewNum = 0;
            this.associatedRulesWidget.ViewText = null;
            this.associatedRulesWidget.WidgetsModel = ((System.Collections.Hashtable)(resources.GetObject("associatedRulesWidget.WidgetsModel")));
            // 
            // associatedRulesIGrid
            // 
            this.associatedRulesIGrid.AllowColumnFilter = true;
            this.associatedRulesIGrid.AllowWordWrap = false;
            this.associatedRulesIGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("associatedRulesIGrid.AlwaysHiddenColumnNames")));
            this.associatedRulesIGrid.AutoResizeCols = true;
            this.associatedRulesIGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this.associatedRulesIGrid.CurrentFilter = null;
            this.associatedRulesIGrid.DefaultRow.Height = 18;
            this.associatedRulesIGrid.DefaultRow.NormalCellHeight = 18;
            this.associatedRulesIGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.associatedRulesIGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.associatedRulesIGrid.ForeColor = System.Drawing.SystemColors.ControlText;
            this.associatedRulesIGrid.Header.BackColor = System.Drawing.Color.WhiteSmoke;
            this.associatedRulesIGrid.Header.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.associatedRulesIGrid.Header.ForeColor = System.Drawing.SystemColors.ControlText;
            this.associatedRulesIGrid.Header.Height = 20;
            this.associatedRulesIGrid.HelpTopic = "";
            this.associatedRulesIGrid.Location = new System.Drawing.Point(4, 4);
            this.associatedRulesIGrid.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.associatedRulesIGrid.Name = "associatedRulesIGrid";
            this.associatedRulesIGrid.ReadOnly = true;
            this.associatedRulesIGrid.RowMode = true;
            this.associatedRulesIGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this.associatedRulesIGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this.associatedRulesIGrid.SearchAsType.SearchCol = null;
            this.associatedRulesIGrid.Size = new System.Drawing.Size(723, 163);
            this.associatedRulesIGrid.TabIndex = 61;
            this.associatedRulesIGrid.TreeCol = null;
            this.associatedRulesIGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.associatedRulesIGrid.WordWrap = false;
            this.associatedRulesIGrid.CellClick += new TenTec.Windows.iGridLib.iGCellClickEventHandler(this.associatedRulesIGrid_CellClick);
            // 
            // _commentsGroupBox
            // 
            this._commentsGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._commentsGroupBox.Controls.Add(this._commentTextBox);
            this._commentsGroupBox.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._commentsGroupBox.Location = new System.Drawing.Point(344, 156);
            this._commentsGroupBox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._commentsGroupBox.Name = "_commentsGroupBox";
            this._commentsGroupBox.Padding = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._commentsGroupBox.Size = new System.Drawing.Size(375, 231);
            this._commentsGroupBox.TabIndex = 107;
            this._commentsGroupBox.TabStop = false;
            this._commentsGroupBox.Text = "Comments";
            // 
            // _commentTextBox
            // 
            this._commentTextBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._commentTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._commentTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._commentTextBox.ForeColor = System.Drawing.Color.ForestGreen;
            this._commentTextBox.Location = new System.Drawing.Point(3, 17);
            this._commentTextBox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._commentTextBox.MaxLength = 256;
            this._commentTextBox.Multiline = true;
            this._commentTextBox.Name = "_commentTextBox";
            this._commentTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this._commentTextBox.Size = new System.Drawing.Size(369, 210);
            this._commentTextBox.TabIndex = 51;
            // 
            // overrideDSNPriorityCheckBox
            // 
            this.overrideDSNPriorityCheckBox.AutoSize = true;
            this.overrideDSNPriorityCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.overrideDSNPriorityCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.overrideDSNPriorityCheckBox.Location = new System.Drawing.Point(382, 43);
            this.overrideDSNPriorityCheckBox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.overrideDSNPriorityCheckBox.Name = "overrideDSNPriorityCheckBox";
            this.overrideDSNPriorityCheckBox.Size = new System.Drawing.Size(169, 18);
            this.overrideDSNPriorityCheckBox.TabIndex = 3;
            this.overrideDSNPriorityCheckBox.Text = "Override Datasource Priority";
            this.overrideDSNPriorityCheckBox.UseVisualStyleBackColor = true;
            this.overrideDSNPriorityCheckBox.Visible = false;
            // 
            // _buttonPanel
            // 
            this._buttonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._buttonPanel.Controls.Add(this.helpButton);
            this._buttonPanel.Controls.Add(this._cancelButton);
            this._buttonPanel.Controls.Add(this._okButton);
            this._buttonPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._buttonPanel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._buttonPanel.Location = new System.Drawing.Point(0, 626);
            this._buttonPanel.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._buttonPanel.Name = "_buttonPanel";
            this._buttonPanel.Size = new System.Drawing.Size(731, 37);
            this._buttonPanel.TabIndex = 110;
            // 
            // helpButton
            // 
            this.helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.helpButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this.helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.helpButton.ForeColor = System.Drawing.SystemColors.ControlText;
            this.helpButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.helpButton.Location = new System.Drawing.Point(648, 6);
            this.helpButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = new System.Drawing.Size(80, 25);
            this.helpButton.TabIndex = 91;
            this.helpButton.Text = "&Help";
            this.helpButton.UseVisualStyleBackColor = true;
            this.helpButton.Click += new System.EventHandler(this.helpButton_Click);
            // 
            // _cancelButton
            // 
            this._cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._cancelButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this._cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._cancelButton.ForeColor = System.Drawing.SystemColors.ControlText;
            this._cancelButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._cancelButton.Location = new System.Drawing.Point(562, 6);
            this._cancelButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Size = new System.Drawing.Size(80, 25);
            this._cancelButton.TabIndex = 91;
            this._cancelButton.Text = "&Cancel";
            this._cancelButton.UseVisualStyleBackColor = true;
            this._cancelButton.Click += new System.EventHandler(this._cancelButton_Click);
            // 
            // _okButton
            // 
            this._okButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._okButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this._okButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._okButton.ForeColor = System.Drawing.SystemColors.ControlText;
            this._okButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._okButton.Location = new System.Drawing.Point(476, 6);
            this._okButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._okButton.Name = "_okButton";
            this._okButton.Size = new System.Drawing.Size(80, 25);
            this._okButton.TabIndex = 92;
            this._okButton.Text = "&Alter";
            this._okButton.UseVisualStyleBackColor = true;
            this._okButton.Click += new System.EventHandler(this._alterButton_Click);
            // 
            // _thresholdsGroupBox
            // 
            this._thresholdsGroupBox.Controls.Add(this.ddlCheckQueryEstimatedResourceUse);
            this._thresholdsGroupBox.Controls.Add(this.ddlCancelQueryIfClientDisappears);
            this._thresholdsGroupBox.Controls.Add(this.lblCheckQueryEstimatedResourceUse);
            this._thresholdsGroupBox.Controls.Add(this.lblCancelQuery);
            this._thresholdsGroupBox.Controls.Add(this._maxCpuComboBox);
            this._thresholdsGroupBox.Controls.Add(this._maxRowsFetchedUpDown);
            this._thresholdsGroupBox.Controls.Add(this.lblMaxESP);
            this._thresholdsGroupBox.Controls.Add(this.nudMaxESP);
            this._thresholdsGroupBox.Controls.Add(this.lblMaxExecQuery);
            this._thresholdsGroupBox.Controls.Add(this.nudMaxExecQuery);
            this._thresholdsGroupBox.Controls.Add(this._maxOverflowUsageNumericUpDown);
            this._thresholdsGroupBox.Controls.Add(this._maxSSDUsageLabel);
            this._thresholdsGroupBox.Controls.Add(this._maxRowsFetchedLabel);
            this._thresholdsGroupBox.Controls.Add(this.label8);
            this._thresholdsGroupBox.Controls.Add(this.label6);
            this._thresholdsGroupBox.Controls.Add(this._maxMemoryUsageNumericUpDown);
            this._thresholdsGroupBox.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._thresholdsGroupBox.Location = new System.Drawing.Point(12, 63);
            this._thresholdsGroupBox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._thresholdsGroupBox.Name = "_thresholdsGroupBox";
            this._thresholdsGroupBox.Padding = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._thresholdsGroupBox.Size = new System.Drawing.Size(326, 228);
            this._thresholdsGroupBox.TabIndex = 104;
            this._thresholdsGroupBox.TabStop = false;
            this._thresholdsGroupBox.Text = "Thresholds";
            // 
            // ddlCheckQueryEstimatedResourceUse
            // 
            this.ddlCheckQueryEstimatedResourceUse.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.ddlCheckQueryEstimatedResourceUse.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.ddlCheckQueryEstimatedResourceUse.Font = new System.Drawing.Font("Tahoma", 8F);
            this.ddlCheckQueryEstimatedResourceUse.FormattingEnabled = true;
            this.ddlCheckQueryEstimatedResourceUse.Location = new System.Drawing.Point(197, 66);
            this.ddlCheckQueryEstimatedResourceUse.Name = "ddlCheckQueryEstimatedResourceUse";
            this.ddlCheckQueryEstimatedResourceUse.Size = new System.Drawing.Size(101, 21);
            this.ddlCheckQueryEstimatedResourceUse.TabIndex = 31;
            // 
            // ddlCancelQueryIfClientDisappears
            // 
            this.ddlCancelQueryIfClientDisappears.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.ddlCancelQueryIfClientDisappears.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.ddlCancelQueryIfClientDisappears.Font = new System.Drawing.Font("Tahoma", 8F);
            this.ddlCancelQueryIfClientDisappears.FormattingEnabled = true;
            this.ddlCancelQueryIfClientDisappears.Location = new System.Drawing.Point(197, 173);
            this.ddlCancelQueryIfClientDisappears.Name = "ddlCancelQueryIfClientDisappears";
            this.ddlCancelQueryIfClientDisappears.Size = new System.Drawing.Size(101, 21);
            this.ddlCancelQueryIfClientDisappears.TabIndex = 30;
            // 
            // lblCheckQueryEstimatedResourceUse
            // 
            this.lblCheckQueryEstimatedResourceUse.AutoSize = true;
            this.lblCheckQueryEstimatedResourceUse.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lblCheckQueryEstimatedResourceUse.Location = new System.Drawing.Point(11, 73);
            this.lblCheckQueryEstimatedResourceUse.Name = "lblCheckQueryEstimatedResourceUse";
            this.lblCheckQueryEstimatedResourceUse.Size = new System.Drawing.Size(169, 13);
            this.lblCheckQueryEstimatedResourceUse.TabIndex = 29;
            this.lblCheckQueryEstimatedResourceUse.Text = "Include Query\'s Est Resource Use";
            // 
            // lblCancelQuery
            // 
            this.lblCancelQuery.AutoSize = true;
            this.lblCancelQuery.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lblCancelQuery.Location = new System.Drawing.Point(11, 178);
            this.lblCancelQuery.Name = "lblCancelQuery";
            this.lblCancelQuery.Size = new System.Drawing.Size(169, 13);
            this.lblCancelQuery.TabIndex = 27;
            this.lblCancelQuery.Text = "Cancel Query If Client Disappears";
            // 
            // _maxCpuComboBox
            // 
            this._maxCpuComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._maxCpuComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._maxCpuComboBox.FormattingEnabled = true;
            this._maxCpuComboBox.Location = new System.Drawing.Point(197, 12);
            this._maxCpuComboBox.Name = "_maxCpuComboBox";
            this._maxCpuComboBox.Size = new System.Drawing.Size(101, 21);
            this._maxCpuComboBox.TabIndex = 25;
            // 
            // _maxRowsFetchedUpDown
            // 
            this._maxRowsFetchedUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._maxRowsFetchedUpDown.Location = new System.Drawing.Point(197, 94);
            this._maxRowsFetchedUpDown.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._maxRowsFetchedUpDown.Maximum = new decimal(new int[] {
            -1,
            2147483647,
            0,
            0});
            this._maxRowsFetchedUpDown.Name = "_maxRowsFetchedUpDown";
            this._maxRowsFetchedUpDown.Size = new System.Drawing.Size(101, 20);
            this._maxRowsFetchedUpDown.TabIndex = 13;
            this._maxRowsFetchedUpDown.TabStop = false;
            // 
            // lblMaxESP
            // 
            this.lblMaxESP.AutoSize = true;
            this.lblMaxESP.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lblMaxESP.Location = new System.Drawing.Point(11, 151);
            this.lblMaxESP.Name = "lblMaxESP";
            this.lblMaxESP.Size = new System.Drawing.Size(75, 13);
            this.lblMaxESP.TabIndex = 23;
            this.lblMaxESP.Text = "Max Avg ESPs";
            // 
            // nudMaxESP
            // 
            this.nudMaxESP.Font = new System.Drawing.Font("Tahoma", 8F);
            this.nudMaxESP.Location = new System.Drawing.Point(197, 146);
            this.nudMaxESP.Maximum = new decimal(new int[] {
            4000,
            0,
            0,
            0});
            this.nudMaxESP.Name = "nudMaxESP";
            this.nudMaxESP.Size = new System.Drawing.Size(101, 20);
            this.nudMaxESP.TabIndex = 24;
            // 
            // lblMaxExecQuery
            // 
            this.lblMaxExecQuery.AutoSize = true;
            this.lblMaxExecQuery.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lblMaxExecQuery.Location = new System.Drawing.Point(11, 125);
            this.lblMaxExecQuery.Name = "lblMaxExecQuery";
            this.lblMaxExecQuery.Size = new System.Drawing.Size(93, 13);
            this.lblMaxExecQuery.TabIndex = 21;
            this.lblMaxExecQuery.Text = "Max Exec Queries";
            // 
            // nudMaxExecQuery
            // 
            this.nudMaxExecQuery.Font = new System.Drawing.Font("Tahoma", 8F);
            this.nudMaxExecQuery.Location = new System.Drawing.Point(197, 120);
            this.nudMaxExecQuery.Maximum = new decimal(new int[] {
            32000,
            0,
            0,
            0});
            this.nudMaxExecQuery.Name = "nudMaxExecQuery";
            this.nudMaxExecQuery.Size = new System.Drawing.Size(101, 20);
            this.nudMaxExecQuery.TabIndex = 22;
            // 
            // _maxOverflowUsageNumericUpDown
            // 
            this._maxOverflowUsageNumericUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._maxOverflowUsageNumericUpDown.Location = new System.Drawing.Point(197, 201);
            this._maxOverflowUsageNumericUpDown.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._maxOverflowUsageNumericUpDown.Name = "_maxOverflowUsageNumericUpDown";
            this._maxOverflowUsageNumericUpDown.Size = new System.Drawing.Size(101, 20);
            this._maxOverflowUsageNumericUpDown.TabIndex = 15;
            // 
            // _maxSSDUsageLabel
            // 
            this._maxSSDUsageLabel.AutoSize = true;
            this._maxSSDUsageLabel.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._maxSSDUsageLabel.Location = new System.Drawing.Point(11, 206);
            this._maxSSDUsageLabel.Name = "_maxSSDUsageLabel";
            this._maxSSDUsageLabel.Size = new System.Drawing.Size(104, 13);
            this._maxSSDUsageLabel.TabIndex = 14;
            this._maxSSDUsageLabel.Text = "Max SSD Usage (%)";
            // 
            // _maxRowsFetchedLabel
            // 
            this._maxRowsFetchedLabel.AutoSize = true;
            this._maxRowsFetchedLabel.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._maxRowsFetchedLabel.Location = new System.Drawing.Point(11, 99);
            this._maxRowsFetchedLabel.Name = "_maxRowsFetchedLabel";
            this._maxRowsFetchedLabel.Size = new System.Drawing.Size(98, 13);
            this._maxRowsFetchedLabel.TabIndex = 13;
            this._maxRowsFetchedLabel.Text = "Max Rows Fetched";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Font = new System.Drawing.Font("Tahoma", 8F);
            this.label8.Location = new System.Drawing.Point(11, 17);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(98, 13);
            this.label8.TabIndex = 11;
            this.label8.Text = "Max CPU Busy (%)";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Font = new System.Drawing.Font("Tahoma", 8F);
            this.label6.Location = new System.Drawing.Point(11, 44);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(123, 13);
            this.label6.TabIndex = 12;
            this.label6.Text = "Max Memory Usage (%)";
            // 
            // _maxMemoryUsageNumericUpDown
            // 
            this._maxMemoryUsageNumericUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._maxMemoryUsageNumericUpDown.Location = new System.Drawing.Point(197, 39);
            this._maxMemoryUsageNumericUpDown.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._maxMemoryUsageNumericUpDown.Name = "_maxMemoryUsageNumericUpDown";
            this._maxMemoryUsageNumericUpDown.Size = new System.Drawing.Size(101, 20);
            this._maxMemoryUsageNumericUpDown.TabIndex = 12;
            // 
            // _stateLabel
            // 
            this._stateLabel.AutoSize = true;
            this._stateLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._stateLabel.Location = new System.Drawing.Point(352, 42);
            this._stateLabel.Name = "_stateLabel";
            this._stateLabel.Size = new System.Drawing.Size(33, 13);
            this._stateLabel.TabIndex = 111;
            this._stateLabel.Text = "State";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.label5.Location = new System.Drawing.Point(12, 43);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(79, 13);
            this.label5.TabIndex = 103;
            this.label5.Text = "Service Priority";
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.label11.Location = new System.Drawing.Point(12, 15);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(72, 13);
            this.label11.TabIndex = 100;
            this.label11.Text = "Service Name";
            // 
            // _servicePriorityComboBox
            // 
            this._servicePriorityComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._servicePriorityComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._servicePriorityComboBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._servicePriorityComboBox.FormattingEnabled = true;
            this._servicePriorityComboBox.Items.AddRange(new object[] {
            "LOW",
            "LOW-MEDIUM",
            "MEDIUM",
            "MEDIUM-HIGH",
            "HIGH",
            "URGENT"});
            this._servicePriorityComboBox.Location = new System.Drawing.Point(122, 39);
            this._servicePriorityComboBox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._servicePriorityComboBox.Name = "_servicePriorityComboBox";
            this._servicePriorityComboBox.Size = new System.Drawing.Size(185, 21);
            this._servicePriorityComboBox.TabIndex = 2;
            // 
            // _stateTextBox
            // 
            this._stateTextBox.Font = new System.Drawing.Font("Tahoma", 9.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._stateTextBox.Location = new System.Drawing.Point(400, 39);
            this._stateTextBox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._stateTextBox.Name = "_stateTextBox";
            this._stateTextBox.ReadOnly = true;
            this._stateTextBox.Size = new System.Drawing.Size(187, 22);
            this._stateTextBox.TabIndex = 4;
            this._stateTextBox.TabStop = false;
            // 
            // _serviceNameTextBox
            // 
            this._serviceNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._serviceNameTextBox.Location = new System.Drawing.Point(122, 11);
            this._serviceNameTextBox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._serviceNameTextBox.MaxLength = 24;
            this._serviceNameTextBox.Name = "_serviceNameTextBox";
            this._serviceNameTextBox.Size = new System.Drawing.Size(430, 21);
            this._serviceNameTextBox.TabIndex = 1;
            // 
            // _optionsGroupBox
            // 
            this._optionsGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._optionsGroupBox.Controls.Add(this._sqlPlanCheckBox);
            this._optionsGroupBox.Controls.Add(this._sqlTextCheckBox);
            this._optionsGroupBox.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._optionsGroupBox.Location = new System.Drawing.Point(608, 63);
            this._optionsGroupBox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._optionsGroupBox.Name = "_optionsGroupBox";
            this._optionsGroupBox.Padding = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._optionsGroupBox.Size = new System.Drawing.Size(111, 90);
            this._optionsGroupBox.TabIndex = 107;
            this._optionsGroupBox.TabStop = false;
            this._optionsGroupBox.Text = "Options";
            // 
            // _sqlPlanCheckBox
            // 
            this._sqlPlanCheckBox.AutoSize = true;
            this._sqlPlanCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._sqlPlanCheckBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._sqlPlanCheckBox.Location = new System.Drawing.Point(23, 26);
            this._sqlPlanCheckBox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._sqlPlanCheckBox.Name = "_sqlPlanCheckBox";
            this._sqlPlanCheckBox.Size = new System.Drawing.Size(74, 18);
            this._sqlPlanCheckBox.TabIndex = 31;
            this._sqlPlanCheckBox.Text = "SQL Plan";
            this._sqlPlanCheckBox.UseVisualStyleBackColor = true;
            // 
            // _sqlTextCheckBox
            // 
            this._sqlTextCheckBox.AutoSize = true;
            this._sqlTextCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._sqlTextCheckBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._sqlTextCheckBox.Location = new System.Drawing.Point(23, 56);
            this._sqlTextCheckBox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._sqlTextCheckBox.Name = "_sqlTextCheckBox";
            this._sqlTextCheckBox.Size = new System.Drawing.Size(76, 18);
            this._sqlTextCheckBox.TabIndex = 32;
            this._sqlTextCheckBox.Text = "SQL Text";
            this._sqlTextCheckBox.UseVisualStyleBackColor = true;
            // 
            // _activeTimeGroupBox
            // 
            this._activeTimeGroupBox.Controls.Add(this._toMMMaskedTextbox);
            this._activeTimeGroupBox.Controls.Add(this._fromMMMaskedTextbox);
            this._activeTimeGroupBox.Controls.Add(this.label9);
            this._activeTimeGroupBox.Controls.Add(this.label3);
            this._activeTimeGroupBox.Controls.Add(this.label1);
            this._activeTimeGroupBox.Controls.Add(this._fromHHMaskedTextbox);
            this._activeTimeGroupBox.Controls.Add(this._toHHMaskedTextbox);
            this._activeTimeGroupBox.Controls.Add(this.label2);
            this._activeTimeGroupBox.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._activeTimeGroupBox.Location = new System.Drawing.Point(344, 63);
            this._activeTimeGroupBox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._activeTimeGroupBox.Name = "_activeTimeGroupBox";
            this._activeTimeGroupBox.Padding = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._activeTimeGroupBox.Size = new System.Drawing.Size(258, 90);
            this._activeTimeGroupBox.TabIndex = 106;
            this._activeTimeGroupBox.TabStop = false;
            this._activeTimeGroupBox.Text = "Active Time";
            // 
            // _toMMMaskedTextbox
            // 
            this._toMMMaskedTextbox.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._toMMMaskedTextbox.Location = new System.Drawing.Point(201, 56);
            this._toMMMaskedTextbox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._toMMMaskedTextbox.Mask = "00";
            this._toMMMaskedTextbox.Name = "_toMMMaskedTextbox";
            this._toMMMaskedTextbox.Size = new System.Drawing.Size(25, 20);
            this._toMMMaskedTextbox.TabIndex = 44;
            this._toMMMaskedTextbox.Text = "00";
            // 
            // _fromMMMaskedTextbox
            // 
            this._fromMMMaskedTextbox.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._fromMMMaskedTextbox.Location = new System.Drawing.Point(201, 23);
            this._fromMMMaskedTextbox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._fromMMMaskedTextbox.Mask = "00";
            this._fromMMMaskedTextbox.Name = "_fromMMMaskedTextbox";
            this._fromMMMaskedTextbox.Size = new System.Drawing.Size(25, 20);
            this._fromMMMaskedTextbox.TabIndex = 42;
            this._fromMMMaskedTextbox.Text = "00";
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Font = new System.Drawing.Font("Tahoma", 8F);
            this.label9.Location = new System.Drawing.Point(20, 59);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(89, 13);
            this.label9.TabIndex = 33;
            this.label9.Text = "To Time (HH:mm)";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Tahoma", 8F);
            this.label3.Location = new System.Drawing.Point(20, 26);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(101, 13);
            this.label3.TabIndex = 31;
            this.label3.Text = "From Time (HH:mm)";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Tahoma", 8F);
            this.label1.Location = new System.Drawing.Point(189, 25);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(14, 13);
            this.label1.TabIndex = 32;
            this.label1.Text = ": ";
            // 
            // _fromHHMaskedTextbox
            // 
            this._fromHHMaskedTextbox.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._fromHHMaskedTextbox.Location = new System.Drawing.Point(160, 23);
            this._fromHHMaskedTextbox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._fromHHMaskedTextbox.Mask = "00";
            this._fromHHMaskedTextbox.Name = "_fromHHMaskedTextbox";
            this._fromHHMaskedTextbox.Size = new System.Drawing.Size(25, 20);
            this._fromHHMaskedTextbox.TabIndex = 41;
            this._fromHHMaskedTextbox.Text = "00";
            // 
            // _toHHMaskedTextbox
            // 
            this._toHHMaskedTextbox.AllowDrop = true;
            this._toHHMaskedTextbox.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._toHHMaskedTextbox.Location = new System.Drawing.Point(160, 56);
            this._toHHMaskedTextbox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._toHHMaskedTextbox.Mask = "00";
            this._toHHMaskedTextbox.Name = "_toHHMaskedTextbox";
            this._toHHMaskedTextbox.Size = new System.Drawing.Size(25, 20);
            this._toHHMaskedTextbox.TabIndex = 43;
            this._toHHMaskedTextbox.Text = "24";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("Tahoma", 8F);
            this.label2.Location = new System.Drawing.Point(189, 58);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(14, 13);
            this.label2.TabIndex = 34;
            this.label2.Text = ": ";
            // 
            // _timeoutGroupBox
            // 
            this._timeoutGroupBox.Controls.Add(this._holdTimeOutNumericUpDown);
            this._timeoutGroupBox.Controls.Add(this._holdTimeOutLabel);
            this._timeoutGroupBox.Controls.Add(this.label4);
            this._timeoutGroupBox.Controls.Add(this._execTimeOutNumericUpDown);
            this._timeoutGroupBox.Controls.Add(this.label10);
            this._timeoutGroupBox.Controls.Add(this._waitTimeOutNumericUpDown);
            this._timeoutGroupBox.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._timeoutGroupBox.Location = new System.Drawing.Point(12, 290);
            this._timeoutGroupBox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._timeoutGroupBox.Name = "_timeoutGroupBox";
            this._timeoutGroupBox.Padding = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._timeoutGroupBox.Size = new System.Drawing.Size(326, 97);
            this._timeoutGroupBox.TabIndex = 105;
            this._timeoutGroupBox.TabStop = false;
            this._timeoutGroupBox.Text = "Timeout (in minutes)";
            // 
            // _holdTimeOutNumericUpDown
            // 
            this._holdTimeOutNumericUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._holdTimeOutNumericUpDown.Location = new System.Drawing.Point(197, 68);
            this._holdTimeOutNumericUpDown.Maximum = new decimal(new int[] {
            1440,
            0,
            0,
            0});
            this._holdTimeOutNumericUpDown.Name = "_holdTimeOutNumericUpDown";
            this._holdTimeOutNumericUpDown.Size = new System.Drawing.Size(101, 20);
            this._holdTimeOutNumericUpDown.TabIndex = 23;
            // 
            // _holdTimeOutLabel
            // 
            this._holdTimeOutLabel.AutoSize = true;
            this._holdTimeOutLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._holdTimeOutLabel.Location = new System.Drawing.Point(11, 72);
            this._holdTimeOutLabel.Name = "_holdTimeOutLabel";
            this._holdTimeOutLabel.Size = new System.Drawing.Size(69, 13);
            this._holdTimeOutLabel.TabIndex = 23;
            this._holdTimeOutLabel.Text = "Hold Timeout";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("Tahoma", 8F);
            this.label4.Location = new System.Drawing.Point(11, 18);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(95, 13);
            this.label4.TabIndex = 21;
            this.label4.Text = "Execution Timeout";
            // 
            // _execTimeOutNumericUpDown
            // 
            this._execTimeOutNumericUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._execTimeOutNumericUpDown.Location = new System.Drawing.Point(197, 14);
            this._execTimeOutNumericUpDown.Maximum = new decimal(new int[] {
            1440,
            0,
            0,
            0});
            this._execTimeOutNumericUpDown.Name = "_execTimeOutNumericUpDown";
            this._execTimeOutNumericUpDown.Size = new System.Drawing.Size(101, 20);
            this._execTimeOutNumericUpDown.TabIndex = 21;
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Font = new System.Drawing.Font("Tahoma", 8F);
            this.label10.Location = new System.Drawing.Point(11, 45);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(70, 13);
            this.label10.TabIndex = 22;
            this.label10.Text = "Wait Timeout";
            // 
            // _waitTimeOutNumericUpDown
            // 
            this._waitTimeOutNumericUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._waitTimeOutNumericUpDown.Location = new System.Drawing.Point(197, 41);
            this._waitTimeOutNumericUpDown.Maximum = new decimal(new int[] {
            1440,
            0,
            0,
            0});
            this._waitTimeOutNumericUpDown.Name = "_waitTimeOutNumericUpDown";
            this._waitTimeOutNumericUpDown.Size = new System.Drawing.Size(101, 20);
            this._waitTimeOutNumericUpDown.TabIndex = 22;
            // 
            // nccWMSEditServiceToolTip
            // 
            this.nccWMSEditServiceToolTip.IsBalloon = true;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this.label11);
            this.TrafodionPanel1.Controls.Add(this._timeoutGroupBox);
            this.TrafodionPanel1.Controls.Add(this._activeTimeGroupBox);
            this.TrafodionPanel1.Controls.Add(this._commentsGroupBox);
            this.TrafodionPanel1.Controls.Add(this._optionsGroupBox);
            this.TrafodionPanel1.Controls.Add(this.overrideDSNPriorityCheckBox);
            this.TrafodionPanel1.Controls.Add(this._serviceNameTextBox);
            this.TrafodionPanel1.Controls.Add(this._stateTextBox);
            this.TrafodionPanel1.Controls.Add(this._thresholdsGroupBox);
            this.TrafodionPanel1.Controls.Add(this._servicePriorityComboBox);
            this.TrafodionPanel1.Controls.Add(this._stateLabel);
            this.TrafodionPanel1.Controls.Add(this.label5);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(731, 392);
            this.TrafodionPanel1.TabIndex = 113;
            // 
            // WMSEditServiceUserControl
            // 
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.associatedRulesWidget);
            this.Controls.Add(this._sqlDefaultsTextBox);
            this.Controls.Add(this.TrafodionPanel1);
            this.Controls.Add(this._buttonPanel);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "WMSEditServiceUserControl";
            this.Size = new System.Drawing.Size(731, 663);
            this.associatedRulesWidget.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.associatedRulesIGrid)).EndInit();
            this._commentsGroupBox.ResumeLayout(false);
            this._commentsGroupBox.PerformLayout();
            this._buttonPanel.ResumeLayout(false);
            this._thresholdsGroupBox.ResumeLayout(false);
            this._thresholdsGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._maxRowsFetchedUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nudMaxESP)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nudMaxExecQuery)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._maxOverflowUsageNumericUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._maxMemoryUsageNumericUpDown)).EndInit();
            this._optionsGroupBox.ResumeLayout(false);
            this._optionsGroupBox.PerformLayout();
            this._activeTimeGroupBox.ResumeLayout(false);
            this._activeTimeGroupBox.PerformLayout();
            this._timeoutGroupBox.ResumeLayout(false);
            this._timeoutGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._holdTimeOutNumericUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._execTimeOutNumericUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._waitTimeOutNumericUpDown)).EndInit();
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionToolTip nccWMSEditServiceToolTip;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _holdTimeOutNumericUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _maxRowsFetchedLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label8;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _buttonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _okButton;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label6;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _sqlDefaultsTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _maxMemoryUsageNumericUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _thresholdsGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _holdTimeOutLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _stateLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label5;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label11;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _servicePriorityComboBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _stateTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _serviceNameTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label4;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _sqlTextCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _execTimeOutNumericUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _commentTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _sqlPlanCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label9;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _optionsGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label3;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _activeTimeGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label1;
        private Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox _fromHHMaskedTextbox;
        private Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox _toHHMaskedTextbox;
        private Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox _fromMMMaskedTextbox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label2;
        private Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox _toMMMaskedTextbox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _timeoutGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label10;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _waitTimeOutNumericUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox overrideDSNPriorityCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionIGrid associatedRulesIGrid;
        private TenTec.Windows.iGridLib.iGCellStyle associatedServicesIGridCol4CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle associatedServicesIGridCol4ColHdrStyle;
        private System.Windows.Forms.ImageList nccWorkbenchControlImageList;
        private TenTec.Windows.iGridLib.iGCellStyleDesign iGCellStyleDesign1;
        private TenTec.Windows.iGridLib.iGColHdrStyle associatedRulesIGridCol6ColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyleDesign iGCellStyleDesign2;
        private Trafodion.Manager.Framework.WidgetCanvas associatedRulesWidget;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _commentsGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionButton helpButton;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _maxSSDUsageLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _maxOverflowUsageNumericUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Framework.Controls.TrafodionLabel lblMaxESP;
        private Framework.Controls.TrafodionNumericUpDown nudMaxESP;
        private Framework.Controls.TrafodionLabel lblMaxExecQuery;
        private Framework.Controls.TrafodionNumericUpDown nudMaxExecQuery;
        private Framework.Controls.TrafodionNumericUpDown _maxRowsFetchedUpDown;
        private Framework.Controls.TrafodionComboBox _maxCpuComboBox;
        private Framework.Controls.TrafodionLabel lblCancelQuery;
        private Framework.Controls.TrafodionLabel lblCheckQueryEstimatedResourceUse;
        private Framework.Controls.TrafodionComboBox ddlCancelQueryIfClientDisappears;
        private Framework.Controls.TrafodionComboBox ddlCheckQueryEstimatedResourceUse;
    
    }
}
