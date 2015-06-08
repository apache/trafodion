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
namespace Trafodion.Manager.WorkloadArea.Controls
{
    partial class EditSystemConfigurationUserControl
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
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this._configurationPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.grbCanaryQuery = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.lblCanaryTimeout = new System.Windows.Forms.Label();
            this.nudCanaryTimeout = new System.Windows.Forms.NumericUpDown();
            this.btnReset = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.lblCanaryQuery = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.txtCanaryQuery = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.lblCanaryExec = new System.Windows.Forms.Label();
            this.nudCanaryExec = new System.Windows.Forms.NumericUpDown();
            this.lblCanaryInterval = new System.Windows.Forms.Label();
            this.nudCanaryInterval = new System.Windows.Forms.NumericUpDown();
            this._buttonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._startButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._stopButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._exportButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._releaseButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._holdButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._refreshButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._applyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.wmsSettingGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._ruleIntervalQueryExecTimeLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._ruleIntervalLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._maxTransactionRollbackLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._ruleIntervalUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._maxTransactionRollbackUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._statsIntervalLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._statsIntervalUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._ruleIntervalQueryExecTimeUpDown = new System.Windows.Forms.NumericUpDown();
            this._timeOutGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._waitTimeoutLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._execTimeoutLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._holdTimeoutUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._execTimeoutUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._holdTimeoutLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._waitTimeoutUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._statusGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._statusTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._stateLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.traceGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._traceFileNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._traceObjectLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._traceFilePathTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._traceFilePathLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._traceFileNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._traceObjectComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.thresholdsGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.ddlCheckQueryEstimatedResourceUse = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.ddlCancelQueryIfClientDisappears = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.lblCheckQueryEstimatedResourceUse = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.lblCancelQuery = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.lblMaxESP = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.nudMaxESP = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.lblMaxExecQuery = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.nudMaxExecQuery = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._maxOverflowUsageLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._maxOverflowUsageUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._maxRowsFetchedUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._maxRowsFetchedLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._maxMemoryUsageLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._maxCPUBusyLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._maxCPUBusyUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._maxMemoryUsageUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.contentPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._configurationPanel.SuspendLayout();
            this.grbCanaryQuery.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nudCanaryTimeout)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nudCanaryExec)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nudCanaryInterval)).BeginInit();
            this._buttonPanel.SuspendLayout();
            this.wmsSettingGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._ruleIntervalUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._maxTransactionRollbackUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._statsIntervalUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._ruleIntervalQueryExecTimeUpDown)).BeginInit();
            this._timeOutGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._holdTimeoutUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._execTimeoutUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._waitTimeoutUpDown)).BeginInit();
            this._statusGroupBox.SuspendLayout();
            this.traceGroupBox.SuspendLayout();
            this.thresholdsGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nudMaxESP)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nudMaxExecQuery)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._maxOverflowUsageUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._maxRowsFetchedUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._maxCPUBusyUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._maxMemoryUsageUpDown)).BeginInit();
            this.contentPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _configurationPanel
            // 
            this._configurationPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._configurationPanel.Controls.Add(this.contentPanel);
            this._configurationPanel.Controls.Add(this._buttonPanel);
            this._configurationPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._configurationPanel.Location = new System.Drawing.Point(0, 0);
            this._configurationPanel.Name = "_configurationPanel";
            this._configurationPanel.Size = new System.Drawing.Size(722, 562);
            this._configurationPanel.TabIndex = 6;
            // 
            // grbCanaryQuery
            // 
            this.grbCanaryQuery.Controls.Add(this.lblCanaryTimeout);
            this.grbCanaryQuery.Controls.Add(this.nudCanaryTimeout);
            this.grbCanaryQuery.Controls.Add(this.btnReset);
            this.grbCanaryQuery.Controls.Add(this.lblCanaryQuery);
            this.grbCanaryQuery.Controls.Add(this.txtCanaryQuery);
            this.grbCanaryQuery.Controls.Add(this.lblCanaryExec);
            this.grbCanaryQuery.Controls.Add(this.nudCanaryExec);
            this.grbCanaryQuery.Controls.Add(this.lblCanaryInterval);
            this.grbCanaryQuery.Controls.Add(this.nudCanaryInterval);
            this.grbCanaryQuery.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.grbCanaryQuery.ForeColor = System.Drawing.SystemColors.ControlText;
            this.grbCanaryQuery.Location = new System.Drawing.Point(11, 318);
            this.grbCanaryQuery.Margin = new System.Windows.Forms.Padding(0);
            this.grbCanaryQuery.Name = "grbCanaryQuery";
            this.grbCanaryQuery.Padding = new System.Windows.Forms.Padding(0);
            this.grbCanaryQuery.Size = new System.Drawing.Size(700, 85);
            this.grbCanaryQuery.TabIndex = 10;
            this.grbCanaryQuery.TabStop = false;
            this.grbCanaryQuery.Text = "Canary Query";
            // 
            // lblCanaryTimeout
            // 
            this.lblCanaryTimeout.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblCanaryTimeout.Location = new System.Drawing.Point(447, 24);
            this.lblCanaryTimeout.Name = "lblCanaryTimeout";
            this.lblCanaryTimeout.Size = new System.Drawing.Size(135, 13);
            this.lblCanaryTimeout.TabIndex = 21;
            this.lblCanaryTimeout.Text = "Canary Timeout (seconds)";
            // 
            // nudCanaryTimeout
            // 
            this.nudCanaryTimeout.Font = new System.Drawing.Font("Tahoma", 8F);
            this.nudCanaryTimeout.Location = new System.Drawing.Point(588, 19);
            this.nudCanaryTimeout.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.nudCanaryTimeout.Maximum = new decimal(new int[] {
            2147483647,
            0,
            0,
            0});
            this.nudCanaryTimeout.Name = "nudCanaryTimeout";
            this.nudCanaryTimeout.Size = new System.Drawing.Size(50, 20);
            this.nudCanaryTimeout.TabIndex = 20;
            // 
            // btnReset
            // 
            this.btnReset.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
            this.btnReset.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.btnReset.Location = new System.Drawing.Point(645, 48);
            this.btnReset.Name = "btnReset";
            this.btnReset.Size = new System.Drawing.Size(50, 23);
            this.btnReset.TabIndex = 19;
            this.btnReset.Text = "Reset";
            this.btnReset.UseVisualStyleBackColor = true;
            this.btnReset.Click += new System.EventHandler(this.btnReset_Click);
            // 
            // lblCanaryQuery
            // 
            this.lblCanaryQuery.AutoSize = true;
            this.lblCanaryQuery.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lblCanaryQuery.Location = new System.Drawing.Point(10, 54);
            this.lblCanaryQuery.Name = "lblCanaryQuery";
            this.lblCanaryQuery.Size = new System.Drawing.Size(75, 13);
            this.lblCanaryQuery.TabIndex = 19;
            this.lblCanaryQuery.Text = "Canary Query";
            // 
            // txtCanaryQuery
            // 
            this.txtCanaryQuery.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtCanaryQuery.Font = new System.Drawing.Font("Tahoma", 8F);
            this.txtCanaryQuery.Location = new System.Drawing.Point(154, 51);
            this.txtCanaryQuery.MaxLength = 1024;
            this.txtCanaryQuery.Name = "txtCanaryQuery";
            this.txtCanaryQuery.Size = new System.Drawing.Size(487, 20);
            this.txtCanaryQuery.TabIndex = 18;
            this.txtCanaryQuery.Text = "SELECT ROW COUNT FROM MANAGEABILITY.NWMS_SCHEMA.WMS_CANARY";
            // 
            // lblCanaryExec
            // 
            this.lblCanaryExec.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblCanaryExec.Location = new System.Drawing.Point(240, 24);
            this.lblCanaryExec.Name = "lblCanaryExec";
            this.lblCanaryExec.Size = new System.Drawing.Size(120, 13);
            this.lblCanaryExec.TabIndex = 17;
            this.lblCanaryExec.Text = "Canary Exec (seconds)";
            // 
            // nudCanaryExec
            // 
            this.nudCanaryExec.Font = new System.Drawing.Font("Tahoma", 8F);
            this.nudCanaryExec.Location = new System.Drawing.Point(368, 19);
            this.nudCanaryExec.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.nudCanaryExec.Maximum = new decimal(new int[] {
            60,
            0,
            0,
            0});
            this.nudCanaryExec.Name = "nudCanaryExec";
            this.nudCanaryExec.Size = new System.Drawing.Size(50, 20);
            this.nudCanaryExec.TabIndex = 16;
            // 
            // lblCanaryInterval
            // 
            this.lblCanaryInterval.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblCanaryInterval.Location = new System.Drawing.Point(10, 24);
            this.lblCanaryInterval.Name = "lblCanaryInterval";
            this.lblCanaryInterval.Size = new System.Drawing.Size(135, 13);
            this.lblCanaryInterval.TabIndex = 15;
            this.lblCanaryInterval.Text = "Canary Interval (minutes)";
            // 
            // nudCanaryInterval
            // 
            this.nudCanaryInterval.Font = new System.Drawing.Font("Tahoma", 8F);
            this.nudCanaryInterval.Location = new System.Drawing.Point(154, 20);
            this.nudCanaryInterval.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.nudCanaryInterval.Maximum = new decimal(new int[] {
            60,
            0,
            0,
            0});
            this.nudCanaryInterval.Minimum = new decimal(new int[] {
            5,
            0,
            0,
            0});
            this.nudCanaryInterval.Name = "nudCanaryInterval";
            this.nudCanaryInterval.Size = new System.Drawing.Size(50, 20);
            this.nudCanaryInterval.TabIndex = 14;
            this.nudCanaryInterval.Value = new decimal(new int[] {
            5,
            0,
            0,
            0});
            // 
            // _buttonPanel
            // 
            this._buttonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._buttonPanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this._buttonPanel.Controls.Add(this._startButton);
            this._buttonPanel.Controls.Add(this._stopButton);
            this._buttonPanel.Controls.Add(this.helpButton);
            this._buttonPanel.Controls.Add(this._exportButton);
            this._buttonPanel.Controls.Add(this._releaseButton);
            this._buttonPanel.Controls.Add(this._holdButton);
            this._buttonPanel.Controls.Add(this._refreshButton);
            this._buttonPanel.Controls.Add(this._applyButton);
            this._buttonPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._buttonPanel.Location = new System.Drawing.Point(0, 529);
            this._buttonPanel.Name = "_buttonPanel";
            this._buttonPanel.Size = new System.Drawing.Size(722, 33);
            this._buttonPanel.TabIndex = 6;
            // 
            // _startButton
            // 
            this._startButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._startButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._startButton.Location = new System.Drawing.Point(188, 3);
            this._startButton.Name = "_startButton";
            this._startButton.Size = new System.Drawing.Size(69, 23);
            this._startButton.TabIndex = 5;
            this._startButton.Text = "S&tart";
            this._startButton.UseVisualStyleBackColor = true;
            this._startButton.Click += new System.EventHandler(this._startButton_Click);
            // 
            // _stopButton
            // 
            this._stopButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._stopButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._stopButton.Location = new System.Drawing.Point(278, 3);
            this._stopButton.Name = "_stopButton";
            this._stopButton.Size = new System.Drawing.Size(69, 23);
            this._stopButton.TabIndex = 4;
            this._stopButton.Text = "&Stop";
            this._stopButton.UseVisualStyleBackColor = true;
            this._stopButton.Click += new System.EventHandler(this._stopButton_Click);
            // 
            // helpButton
            // 
            this.helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.helpButton.Location = new System.Drawing.Point(638, 3);
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = new System.Drawing.Size(69, 23);
            this.helpButton.TabIndex = 3;
            this.helpButton.Text = "&Help";
            this.helpButton.UseVisualStyleBackColor = true;
            this.helpButton.Click += new System.EventHandler(this.helpButton_Click);
            // 
            // _exportButton
            // 
            this._exportButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._exportButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._exportButton.Location = new System.Drawing.Point(548, 3);
            this._exportButton.Name = "_exportButton";
            this._exportButton.Size = new System.Drawing.Size(69, 23);
            this._exportButton.TabIndex = 3;
            this._exportButton.Text = "&Export";
            this._exportButton.UseVisualStyleBackColor = true;
            this._exportButton.Click += new System.EventHandler(this._exportButton_Click);
            // 
            // _releaseButton
            // 
            this._releaseButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._releaseButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._releaseButton.Location = new System.Drawing.Point(458, 3);
            this._releaseButton.Name = "_releaseButton";
            this._releaseButton.Size = new System.Drawing.Size(69, 23);
            this._releaseButton.TabIndex = 3;
            this._releaseButton.Text = "&Release";
            this._releaseButton.UseVisualStyleBackColor = true;
            this._releaseButton.Click += new System.EventHandler(this._releaseButton_Click);
            // 
            // _holdButton
            // 
            this._holdButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._holdButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._holdButton.Location = new System.Drawing.Point(368, 3);
            this._holdButton.Name = "_holdButton";
            this._holdButton.Size = new System.Drawing.Size(69, 23);
            this._holdButton.TabIndex = 2;
            this._holdButton.Text = "H&old";
            this._holdButton.UseVisualStyleBackColor = true;
            this._holdButton.Click += new System.EventHandler(this._holdButton_Click);
            // 
            // _refreshButton
            // 
            this._refreshButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._refreshButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._refreshButton.Location = new System.Drawing.Point(8, 3);
            this._refreshButton.Name = "_refreshButton";
            this._refreshButton.Size = new System.Drawing.Size(69, 23);
            this._refreshButton.TabIndex = 1;
            this._refreshButton.Text = "Re&fresh";
            this._refreshButton.UseVisualStyleBackColor = true;
            this._refreshButton.Click += new System.EventHandler(this._resetButton_Click);
            // 
            // _applyButton
            // 
            this._applyButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._applyButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._applyButton.Location = new System.Drawing.Point(98, 3);
            this._applyButton.Name = "_applyButton";
            this._applyButton.Size = new System.Drawing.Size(69, 23);
            this._applyButton.TabIndex = 0;
            this._applyButton.Text = "&Apply";
            this._applyButton.UseVisualStyleBackColor = true;
            this._applyButton.Click += new System.EventHandler(this._applyButton_Click);
            // 
            // wmsSettingGroupBox
            // 
            this.wmsSettingGroupBox.Controls.Add(this._ruleIntervalQueryExecTimeLabel);
            this.wmsSettingGroupBox.Controls.Add(this._ruleIntervalLabel);
            this.wmsSettingGroupBox.Controls.Add(this._maxTransactionRollbackLabel);
            this.wmsSettingGroupBox.Controls.Add(this._ruleIntervalUpDown);
            this.wmsSettingGroupBox.Controls.Add(this._maxTransactionRollbackUpDown);
            this.wmsSettingGroupBox.Controls.Add(this._statsIntervalLabel);
            this.wmsSettingGroupBox.Controls.Add(this._statsIntervalUpDown);
            this.wmsSettingGroupBox.Controls.Add(this._ruleIntervalQueryExecTimeUpDown);
            this.wmsSettingGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.wmsSettingGroupBox.ForeColor = System.Drawing.SystemColors.ControlText;
            this.wmsSettingGroupBox.Location = new System.Drawing.Point(366, 165);
            this.wmsSettingGroupBox.Name = "wmsSettingGroupBox";
            this.wmsSettingGroupBox.Padding = new System.Windows.Forms.Padding(0);
            this.wmsSettingGroupBox.Size = new System.Drawing.Size(344, 150);
            this.wmsSettingGroupBox.TabIndex = 9;
            this.wmsSettingGroupBox.TabStop = false;
            this.wmsSettingGroupBox.Text = "Default WMS Settings";
            this.wmsSettingGroupBox.UseCompatibleTextRendering = true;
            // 
            // _ruleIntervalQueryExecTimeLabel
            // 
            this._ruleIntervalQueryExecTimeLabel.AutoSize = true;
            this._ruleIntervalQueryExecTimeLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._ruleIntervalQueryExecTimeLabel.Location = new System.Drawing.Point(6, 57);
            this._ruleIntervalQueryExecTimeLabel.Name = "_ruleIntervalQueryExecTimeLabel";
            this._ruleIntervalQueryExecTimeLabel.Size = new System.Drawing.Size(144, 13);
            this._ruleIntervalQueryExecTimeLabel.TabIndex = 14;
            this._ruleIntervalQueryExecTimeLabel.Text = "Rule Start Interval (minutes)";
            // 
            // _ruleIntervalLabel
            // 
            this._ruleIntervalLabel.AutoSize = true;
            this._ruleIntervalLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._ruleIntervalLabel.Location = new System.Drawing.Point(6, 26);
            this._ruleIntervalLabel.Name = "_ruleIntervalLabel";
            this._ruleIntervalLabel.Size = new System.Drawing.Size(119, 13);
            this._ruleIntervalLabel.TabIndex = 4;
            this._ruleIntervalLabel.Text = "Rule Interval (seconds)";
            // 
            // _maxTransactionRollbackLabel
            // 
            this._maxTransactionRollbackLabel.AutoSize = true;
            this._maxTransactionRollbackLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._maxTransactionRollbackLabel.Location = new System.Drawing.Point(6, 118);
            this._maxTransactionRollbackLabel.Name = "_maxTransactionRollbackLabel";
            this._maxTransactionRollbackLabel.Size = new System.Drawing.Size(176, 13);
            this._maxTransactionRollbackLabel.TabIndex = 21;
            this._maxTransactionRollbackLabel.Text = "Max Transaction Rollback (minutes)";
            // 
            // _ruleIntervalUpDown
            // 
            this._ruleIntervalUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._ruleIntervalUpDown.Location = new System.Drawing.Point(196, 23);
            this._ruleIntervalUpDown.Maximum = new decimal(new int[] {
            3600,
            0,
            0,
            0});
            this._ruleIntervalUpDown.Minimum = new decimal(new int[] {
            60,
            0,
            0,
            0});
            this._ruleIntervalUpDown.Name = "_ruleIntervalUpDown";
            this._ruleIntervalUpDown.Size = new System.Drawing.Size(87, 20);
            this._ruleIntervalUpDown.TabIndex = 5;
            this._ruleIntervalUpDown.Value = new decimal(new int[] {
            60,
            0,
            0,
            0});
            // 
            // _maxTransactionRollbackUpDown
            // 
            this._maxTransactionRollbackUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._maxTransactionRollbackUpDown.Location = new System.Drawing.Point(196, 114);
            this._maxTransactionRollbackUpDown.Maximum = new decimal(new int[] {
            300,
            0,
            0,
            0});
            this._maxTransactionRollbackUpDown.Name = "_maxTransactionRollbackUpDown";
            this._maxTransactionRollbackUpDown.Size = new System.Drawing.Size(87, 20);
            this._maxTransactionRollbackUpDown.TabIndex = 4;
            this._maxTransactionRollbackUpDown.ValueChanged += new System.EventHandler(this._maxTransactionRollbackUpDown_ValueChanged);
            // 
            // _statsIntervalLabel
            // 
            this._statsIntervalLabel.AutoSize = true;
            this._statsIntervalLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._statsIntervalLabel.Location = new System.Drawing.Point(6, 87);
            this._statsIntervalLabel.Name = "_statsIntervalLabel";
            this._statsIntervalLabel.Size = new System.Drawing.Size(123, 13);
            this._statsIntervalLabel.TabIndex = 2;
            this._statsIntervalLabel.Text = "Stats Interval (seconds)";
            // 
            // _statsIntervalUpDown
            // 
            this._statsIntervalUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._statsIntervalUpDown.Location = new System.Drawing.Point(196, 84);
            this._statsIntervalUpDown.Maximum = new decimal(new int[] {
            300,
            0,
            0,
            0});
            this._statsIntervalUpDown.Minimum = new decimal(new int[] {
            5,
            0,
            0,
            0});
            this._statsIntervalUpDown.Name = "_statsIntervalUpDown";
            this._statsIntervalUpDown.Size = new System.Drawing.Size(87, 20);
            this._statsIntervalUpDown.TabIndex = 4;
            this._statsIntervalUpDown.Value = new decimal(new int[] {
            5,
            0,
            0,
            0});
            // 
            // _ruleIntervalQueryExecTimeUpDown
            // 
            this._ruleIntervalQueryExecTimeUpDown.Font = new System.Drawing.Font("Arial", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._ruleIntervalQueryExecTimeUpDown.Location = new System.Drawing.Point(196, 53);
            this._ruleIntervalQueryExecTimeUpDown.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._ruleIntervalQueryExecTimeUpDown.Maximum = new decimal(new int[] {
            30,
            0,
            0,
            0});
            this._ruleIntervalQueryExecTimeUpDown.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this._ruleIntervalQueryExecTimeUpDown.Name = "_ruleIntervalQueryExecTimeUpDown";
            this._ruleIntervalQueryExecTimeUpDown.Size = new System.Drawing.Size(87, 21);
            this._ruleIntervalQueryExecTimeUpDown.TabIndex = 6;
            this._ruleIntervalQueryExecTimeUpDown.Value = new decimal(new int[] {
            5,
            0,
            0,
            0});
            // 
            // _timeOutGroupBox
            // 
            this._timeOutGroupBox.Controls.Add(this._waitTimeoutLabel);
            this._timeOutGroupBox.Controls.Add(this._execTimeoutLabel);
            this._timeOutGroupBox.Controls.Add(this._holdTimeoutUpDown);
            this._timeOutGroupBox.Controls.Add(this._execTimeoutUpDown);
            this._timeOutGroupBox.Controls.Add(this._holdTimeoutLabel);
            this._timeOutGroupBox.Controls.Add(this._waitTimeoutUpDown);
            this._timeOutGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._timeOutGroupBox.ForeColor = System.Drawing.SystemColors.ControlText;
            this._timeOutGroupBox.Location = new System.Drawing.Point(366, 52);
            this._timeOutGroupBox.Name = "_timeOutGroupBox";
            this._timeOutGroupBox.Padding = new System.Windows.Forms.Padding(0);
            this._timeOutGroupBox.Size = new System.Drawing.Size(344, 110);
            this._timeOutGroupBox.TabIndex = 9;
            this._timeOutGroupBox.TabStop = false;
            this._timeOutGroupBox.Text = "Default Service Timeouts (minutes)";
            this._timeOutGroupBox.UseCompatibleTextRendering = true;
            // 
            // _waitTimeoutLabel
            // 
            this._waitTimeoutLabel.AutoSize = true;
            this._waitTimeoutLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._waitTimeoutLabel.Location = new System.Drawing.Point(7, 52);
            this._waitTimeoutLabel.Name = "_waitTimeoutLabel";
            this._waitTimeoutLabel.Size = new System.Drawing.Size(70, 13);
            this._waitTimeoutLabel.TabIndex = 7;
            this._waitTimeoutLabel.Text = "Wait Timeout";
            // 
            // _execTimeoutLabel
            // 
            this._execTimeoutLabel.AutoSize = true;
            this._execTimeoutLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._execTimeoutLabel.Location = new System.Drawing.Point(7, 26);
            this._execTimeoutLabel.Name = "_execTimeoutLabel";
            this._execTimeoutLabel.Size = new System.Drawing.Size(71, 13);
            this._execTimeoutLabel.TabIndex = 5;
            this._execTimeoutLabel.Text = "Exec Timeout";
            // 
            // _holdTimeoutUpDown
            // 
            this._holdTimeoutUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._holdTimeoutUpDown.Location = new System.Drawing.Point(196, 76);
            this._holdTimeoutUpDown.Maximum = new decimal(new int[] {
            1440,
            0,
            0,
            0});
            this._holdTimeoutUpDown.Name = "_holdTimeoutUpDown";
            this._holdTimeoutUpDown.Size = new System.Drawing.Size(87, 20);
            this._holdTimeoutUpDown.TabIndex = 10;
            // 
            // _execTimeoutUpDown
            // 
            this._execTimeoutUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._execTimeoutUpDown.Location = new System.Drawing.Point(196, 23);
            this._execTimeoutUpDown.Maximum = new decimal(new int[] {
            1440,
            0,
            0,
            0});
            this._execTimeoutUpDown.Name = "_execTimeoutUpDown";
            this._execTimeoutUpDown.Size = new System.Drawing.Size(87, 20);
            this._execTimeoutUpDown.TabIndex = 6;
            // 
            // _holdTimeoutLabel
            // 
            this._holdTimeoutLabel.AutoSize = true;
            this._holdTimeoutLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._holdTimeoutLabel.Location = new System.Drawing.Point(7, 79);
            this._holdTimeoutLabel.Name = "_holdTimeoutLabel";
            this._holdTimeoutLabel.Size = new System.Drawing.Size(69, 13);
            this._holdTimeoutLabel.TabIndex = 9;
            this._holdTimeoutLabel.Text = "Hold Timeout";
            // 
            // _waitTimeoutUpDown
            // 
            this._waitTimeoutUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._waitTimeoutUpDown.Location = new System.Drawing.Point(196, 49);
            this._waitTimeoutUpDown.Maximum = new decimal(new int[] {
            1440,
            0,
            0,
            0});
            this._waitTimeoutUpDown.Name = "_waitTimeoutUpDown";
            this._waitTimeoutUpDown.Size = new System.Drawing.Size(87, 20);
            this._waitTimeoutUpDown.TabIndex = 8;
            // 
            // _statusGroupBox
            // 
            this._statusGroupBox.Controls.Add(this._statusTextBox);
            this._statusGroupBox.Controls.Add(this._stateLabel);
            this._statusGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._statusGroupBox.ForeColor = System.Drawing.SystemColors.ControlText;
            this._statusGroupBox.Location = new System.Drawing.Point(11, 2);
            this._statusGroupBox.Margin = new System.Windows.Forms.Padding(0);
            this._statusGroupBox.Name = "_statusGroupBox";
            this._statusGroupBox.Padding = new System.Windows.Forms.Padding(0);
            this._statusGroupBox.Size = new System.Drawing.Size(700, 46);
            this._statusGroupBox.TabIndex = 8;
            this._statusGroupBox.TabStop = false;
            this._statusGroupBox.Text = "Status";
            // 
            // _statusTextBox
            // 
            this._statusTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._statusTextBox.Location = new System.Drawing.Point(56, 17);
            this._statusTextBox.Name = "_statusTextBox";
            this._statusTextBox.ReadOnly = true;
            this._statusTextBox.Size = new System.Drawing.Size(105, 20);
            this._statusTextBox.TabIndex = 1;
            this._statusTextBox.TextChanged += new System.EventHandler(this._statusTextBox_TextChanged);
            // 
            // _stateLabel
            // 
            this._stateLabel.AutoSize = true;
            this._stateLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._stateLabel.Location = new System.Drawing.Point(12, 21);
            this._stateLabel.Name = "_stateLabel";
            this._stateLabel.Size = new System.Drawing.Size(33, 13);
            this._stateLabel.TabIndex = 0;
            this._stateLabel.Text = "State";
            // 
            // traceGroupBox
            // 
            this.traceGroupBox.Controls.Add(this._traceFileNameLabel);
            this.traceGroupBox.Controls.Add(this._traceObjectLabel);
            this.traceGroupBox.Controls.Add(this._traceFilePathTextBox);
            this.traceGroupBox.Controls.Add(this._traceFilePathLabel);
            this.traceGroupBox.Controls.Add(this._traceFileNameTextBox);
            this.traceGroupBox.Controls.Add(this._traceObjectComboBox);
            this.traceGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.traceGroupBox.ForeColor = System.Drawing.SystemColors.ControlText;
            this.traceGroupBox.Location = new System.Drawing.Point(11, 406);
            this.traceGroupBox.Name = "traceGroupBox";
            this.traceGroupBox.Size = new System.Drawing.Size(700, 113);
            this.traceGroupBox.TabIndex = 7;
            this.traceGroupBox.TabStop = false;
            this.traceGroupBox.Text = "Trace Options";
            // 
            // _traceFileNameLabel
            // 
            this._traceFileNameLabel.AutoSize = true;
            this._traceFileNameLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._traceFileNameLabel.Location = new System.Drawing.Point(10, 57);
            this._traceFileNameLabel.Name = "_traceFileNameLabel";
            this._traceFileNameLabel.Size = new System.Drawing.Size(83, 13);
            this._traceFileNameLabel.TabIndex = 5;
            this._traceFileNameLabel.Text = "Trace File Name";
            // 
            // _traceObjectLabel
            // 
            this._traceObjectLabel.AutoSize = true;
            this._traceObjectLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._traceObjectLabel.Location = new System.Drawing.Point(10, 24);
            this._traceObjectLabel.Name = "_traceObjectLabel";
            this._traceObjectLabel.Size = new System.Drawing.Size(69, 13);
            this._traceObjectLabel.TabIndex = 3;
            this._traceObjectLabel.Text = "Trace Object";
            // 
            // _traceFilePathTextBox
            // 
            this._traceFilePathTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._traceFilePathTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._traceFilePathTextBox.Location = new System.Drawing.Point(113, 85);
            this._traceFilePathTextBox.Name = "_traceFilePathTextBox";
            this._traceFilePathTextBox.Size = new System.Drawing.Size(570, 20);
            this._traceFilePathTextBox.TabIndex = 5;
            // 
            // _traceFilePathLabel
            // 
            this._traceFilePathLabel.AutoSize = true;
            this._traceFilePathLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._traceFilePathLabel.Location = new System.Drawing.Point(10, 89);
            this._traceFilePathLabel.Name = "_traceFilePathLabel";
            this._traceFilePathLabel.Size = new System.Drawing.Size(78, 13);
            this._traceFilePathLabel.TabIndex = 4;
            this._traceFilePathLabel.Text = "Trace File Path";
            // 
            // _traceFileNameTextBox
            // 
            this._traceFileNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._traceFileNameTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._traceFileNameTextBox.Location = new System.Drawing.Point(113, 53);
            this._traceFileNameTextBox.Name = "_traceFileNameTextBox";
            this._traceFileNameTextBox.Size = new System.Drawing.Size(570, 20);
            this._traceFileNameTextBox.TabIndex = 4;
            // 
            // _traceObjectComboBox
            // 
            this._traceObjectComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._traceObjectComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._traceObjectComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._traceObjectComboBox.FormattingEnabled = true;
            this._traceObjectComboBox.Items.AddRange(new object[] {
            "OFF",
            "ALL",
            "QSMGR",
            "QSCOM",
            "QSSTATS",
            "QSSYNC",
            "QSSYNCAS"});
            this._traceObjectComboBox.Location = new System.Drawing.Point(113, 20);
            this._traceObjectComboBox.Name = "_traceObjectComboBox";
            this._traceObjectComboBox.Size = new System.Drawing.Size(570, 21);
            this._traceObjectComboBox.TabIndex = 3;
            // 
            // thresholdsGroupBox
            // 
            this.thresholdsGroupBox.Controls.Add(this.ddlCheckQueryEstimatedResourceUse);
            this.thresholdsGroupBox.Controls.Add(this.ddlCancelQueryIfClientDisappears);
            this.thresholdsGroupBox.Controls.Add(this.lblCheckQueryEstimatedResourceUse);
            this.thresholdsGroupBox.Controls.Add(this.lblCancelQuery);
            this.thresholdsGroupBox.Controls.Add(this.lblMaxESP);
            this.thresholdsGroupBox.Controls.Add(this.nudMaxESP);
            this.thresholdsGroupBox.Controls.Add(this.lblMaxExecQuery);
            this.thresholdsGroupBox.Controls.Add(this.nudMaxExecQuery);
            this.thresholdsGroupBox.Controls.Add(this._maxOverflowUsageLabel);
            this.thresholdsGroupBox.Controls.Add(this._maxOverflowUsageUpDown);
            this.thresholdsGroupBox.Controls.Add(this._maxRowsFetchedUpDown);
            this.thresholdsGroupBox.Controls.Add(this._maxRowsFetchedLabel);
            this.thresholdsGroupBox.Controls.Add(this._maxMemoryUsageLabel);
            this.thresholdsGroupBox.Controls.Add(this._maxCPUBusyLabel);
            this.thresholdsGroupBox.Controls.Add(this._maxCPUBusyUpDown);
            this.thresholdsGroupBox.Controls.Add(this._maxMemoryUsageUpDown);
            this.thresholdsGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.thresholdsGroupBox.ForeColor = System.Drawing.SystemColors.ControlText;
            this.thresholdsGroupBox.Location = new System.Drawing.Point(11, 52);
            this.thresholdsGroupBox.Margin = new System.Windows.Forms.Padding(0);
            this.thresholdsGroupBox.Name = "thresholdsGroupBox";
            this.thresholdsGroupBox.Padding = new System.Windows.Forms.Padding(0);
            this.thresholdsGroupBox.Size = new System.Drawing.Size(344, 263);
            this.thresholdsGroupBox.TabIndex = 6;
            this.thresholdsGroupBox.TabStop = false;
            this.thresholdsGroupBox.Text = "Default Service Thresholds";
            this.thresholdsGroupBox.UseCompatibleTextRendering = true;
            // 
            // ddlCheckQueryEstimatedResourceUse
            // 
            this.ddlCheckQueryEstimatedResourceUse.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.ddlCheckQueryEstimatedResourceUse.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.ddlCheckQueryEstimatedResourceUse.Font = new System.Drawing.Font("Tahoma", 8F);
            this.ddlCheckQueryEstimatedResourceUse.FormattingEnabled = true;
            this.ddlCheckQueryEstimatedResourceUse.Location = new System.Drawing.Point(196, 77);
            this.ddlCheckQueryEstimatedResourceUse.Name = "ddlCheckQueryEstimatedResourceUse";
            this.ddlCheckQueryEstimatedResourceUse.Size = new System.Drawing.Size(87, 21);
            this.ddlCheckQueryEstimatedResourceUse.TabIndex = 33;
            // 
            // ddlCancelQueryIfClientDisappears
            // 
            this.ddlCancelQueryIfClientDisappears.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.ddlCancelQueryIfClientDisappears.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.ddlCancelQueryIfClientDisappears.Font = new System.Drawing.Font("Tahoma", 8F);
            this.ddlCancelQueryIfClientDisappears.FormattingEnabled = true;
            this.ddlCancelQueryIfClientDisappears.Location = new System.Drawing.Point(196, 191);
            this.ddlCancelQueryIfClientDisappears.Name = "ddlCancelQueryIfClientDisappears";
            this.ddlCancelQueryIfClientDisappears.Size = new System.Drawing.Size(87, 21);
            this.ddlCancelQueryIfClientDisappears.TabIndex = 32;
            // 
            // lblCheckQueryEstimatedResourceUse
            // 
            this.lblCheckQueryEstimatedResourceUse.AutoSize = true;
            this.lblCheckQueryEstimatedResourceUse.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lblCheckQueryEstimatedResourceUse.Location = new System.Drawing.Point(10, 82);
            this.lblCheckQueryEstimatedResourceUse.Name = "lblCheckQueryEstimatedResourceUse";
            this.lblCheckQueryEstimatedResourceUse.Size = new System.Drawing.Size(166, 13);
            this.lblCheckQueryEstimatedResourceUse.TabIndex = 24;
            this.lblCheckQueryEstimatedResourceUse.Text = "Include Query Est. Resource Use";
            // 
            // lblCancelQuery
            // 
            this.lblCancelQuery.AutoSize = true;
            this.lblCancelQuery.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lblCancelQuery.Location = new System.Drawing.Point(9, 197);
            this.lblCancelQuery.Name = "lblCancelQuery";
            this.lblCancelQuery.Size = new System.Drawing.Size(169, 13);
            this.lblCancelQuery.TabIndex = 22;
            this.lblCancelQuery.Text = "Cancel Query If Client Disappears";
            // 
            // lblMaxESP
            // 
            this.lblMaxESP.AutoSize = true;
            this.lblMaxESP.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lblMaxESP.Location = new System.Drawing.Point(9, 167);
            this.lblMaxESP.Name = "lblMaxESP";
            this.lblMaxESP.Size = new System.Drawing.Size(75, 13);
            this.lblMaxESP.TabIndex = 19;
            this.lblMaxESP.Text = "Max Avg ESPs";
            // 
            // nudMaxESP
            // 
            this.nudMaxESP.Font = new System.Drawing.Font("Tahoma", 8F);
            this.nudMaxESP.Location = new System.Drawing.Point(196, 163);
            this.nudMaxESP.Maximum = new decimal(new int[] {
            4000,
            0,
            0,
            0});
            this.nudMaxESP.Name = "nudMaxESP";
            this.nudMaxESP.Size = new System.Drawing.Size(87, 20);
            this.nudMaxESP.TabIndex = 7;
            // 
            // lblMaxExecQuery
            // 
            this.lblMaxExecQuery.AutoSize = true;
            this.lblMaxExecQuery.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lblMaxExecQuery.Location = new System.Drawing.Point(9, 139);
            this.lblMaxExecQuery.Name = "lblMaxExecQuery";
            this.lblMaxExecQuery.Size = new System.Drawing.Size(93, 13);
            this.lblMaxExecQuery.TabIndex = 17;
            this.lblMaxExecQuery.Text = "Max Exec Queries";
            // 
            // nudMaxExecQuery
            // 
            this.nudMaxExecQuery.Font = new System.Drawing.Font("Tahoma", 8F);
            this.nudMaxExecQuery.Location = new System.Drawing.Point(196, 135);
            this.nudMaxExecQuery.Maximum = new decimal(new int[] {
            32000,
            0,
            0,
            0});
            this.nudMaxExecQuery.Name = "nudMaxExecQuery";
            this.nudMaxExecQuery.Size = new System.Drawing.Size(87, 20);
            this.nudMaxExecQuery.TabIndex = 6;
            // 
            // _maxOverflowUsageLabel
            // 
            this._maxOverflowUsageLabel.AutoSize = true;
            this._maxOverflowUsageLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._maxOverflowUsageLabel.Location = new System.Drawing.Point(9, 225);
            this._maxOverflowUsageLabel.Name = "_maxOverflowUsageLabel";
            this._maxOverflowUsageLabel.Size = new System.Drawing.Size(104, 13);
            this._maxOverflowUsageLabel.TabIndex = 16;
            this._maxOverflowUsageLabel.Text = "Max SSD Usage (%)";
            // 
            // _maxOverflowUsageUpDown
            // 
            this._maxOverflowUsageUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._maxOverflowUsageUpDown.Location = new System.Drawing.Point(196, 222);
            this._maxOverflowUsageUpDown.Name = "_maxOverflowUsageUpDown";
            this._maxOverflowUsageUpDown.Size = new System.Drawing.Size(87, 20);
            this._maxOverflowUsageUpDown.TabIndex = 3;
            // 
            // _maxRowsFetchedUpDown
            // 
            this._maxRowsFetchedUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._maxRowsFetchedUpDown.Location = new System.Drawing.Point(196, 107);
            this._maxRowsFetchedUpDown.Maximum = new decimal(new int[] {
            -1,
            2147483647,
            0,
            0});
            this._maxRowsFetchedUpDown.Name = "_maxRowsFetchedUpDown";
            this._maxRowsFetchedUpDown.Size = new System.Drawing.Size(87, 20);
            this._maxRowsFetchedUpDown.TabIndex = 5;
            // 
            // _maxRowsFetchedLabel
            // 
            this._maxRowsFetchedLabel.AutoSize = true;
            this._maxRowsFetchedLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._maxRowsFetchedLabel.Location = new System.Drawing.Point(9, 111);
            this._maxRowsFetchedLabel.Name = "_maxRowsFetchedLabel";
            this._maxRowsFetchedLabel.Size = new System.Drawing.Size(98, 13);
            this._maxRowsFetchedLabel.TabIndex = 11;
            this._maxRowsFetchedLabel.Text = "Max Rows Fetched";
            // 
            // _maxMemoryUsageLabel
            // 
            this._maxMemoryUsageLabel.AutoSize = true;
            this._maxMemoryUsageLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._maxMemoryUsageLabel.Location = new System.Drawing.Point(9, 52);
            this._maxMemoryUsageLabel.Name = "_maxMemoryUsageLabel";
            this._maxMemoryUsageLabel.Size = new System.Drawing.Size(123, 13);
            this._maxMemoryUsageLabel.TabIndex = 1;
            this._maxMemoryUsageLabel.Text = "Max Memory Usage (%)";
            // 
            // _maxCPUBusyLabel
            // 
            this._maxCPUBusyLabel.AutoSize = true;
            this._maxCPUBusyLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._maxCPUBusyLabel.Location = new System.Drawing.Point(9, 24);
            this._maxCPUBusyLabel.Name = "_maxCPUBusyLabel";
            this._maxCPUBusyLabel.Size = new System.Drawing.Size(98, 13);
            this._maxCPUBusyLabel.TabIndex = 0;
            this._maxCPUBusyLabel.Text = "Max CPU Busy (%)";
            // 
            // _maxCPUBusyUpDown
            // 
            this._maxCPUBusyUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._maxCPUBusyUpDown.Location = new System.Drawing.Point(196, 21);
            this._maxCPUBusyUpDown.Name = "_maxCPUBusyUpDown";
            this._maxCPUBusyUpDown.Size = new System.Drawing.Size(87, 20);
            this._maxCPUBusyUpDown.TabIndex = 1;
            // 
            // _maxMemoryUsageUpDown
            // 
            this._maxMemoryUsageUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._maxMemoryUsageUpDown.Location = new System.Drawing.Point(196, 49);
            this._maxMemoryUsageUpDown.Name = "_maxMemoryUsageUpDown";
            this._maxMemoryUsageUpDown.Size = new System.Drawing.Size(87, 20);
            this._maxMemoryUsageUpDown.TabIndex = 2;
            // 
            // contentPanel
            // 
            this.contentPanel.AutoScroll = true;
            this.contentPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.contentPanel.Controls.Add(this._statusGroupBox);
            this.contentPanel.Controls.Add(this.thresholdsGroupBox);
            this.contentPanel.Controls.Add(this.grbCanaryQuery);
            this.contentPanel.Controls.Add(this.traceGroupBox);
            this.contentPanel.Controls.Add(this.wmsSettingGroupBox);
            this.contentPanel.Controls.Add(this._timeOutGroupBox);
            this.contentPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.contentPanel.Location = new System.Drawing.Point(0, 0);
            this.contentPanel.Name = "contentPanel";
            this.contentPanel.Size = new System.Drawing.Size(722, 529);
            this.contentPanel.TabIndex = 11;
            // 
            // EditSystemConfigurationUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.Controls.Add(this._configurationPanel);
            this.Name = "EditSystemConfigurationUserControl";
            this.Size = new System.Drawing.Size(722, 562);
            this._configurationPanel.ResumeLayout(false);
            this.grbCanaryQuery.ResumeLayout(false);
            this.grbCanaryQuery.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nudCanaryTimeout)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nudCanaryExec)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nudCanaryInterval)).EndInit();
            this._buttonPanel.ResumeLayout(false);
            this.wmsSettingGroupBox.ResumeLayout(false);
            this.wmsSettingGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._ruleIntervalUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._maxTransactionRollbackUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._statsIntervalUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._ruleIntervalQueryExecTimeUpDown)).EndInit();
            this._timeOutGroupBox.ResumeLayout(false);
            this._timeOutGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._holdTimeoutUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._execTimeoutUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._waitTimeoutUpDown)).EndInit();
            this._statusGroupBox.ResumeLayout(false);
            this._statusGroupBox.PerformLayout();
            this.traceGroupBox.ResumeLayout(false);
            this.traceGroupBox.PerformLayout();
            this.thresholdsGroupBox.ResumeLayout(false);
            this.thresholdsGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nudMaxESP)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nudMaxExecQuery)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._maxOverflowUsageUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._maxRowsFetchedUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._maxCPUBusyUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._maxMemoryUsageUpDown)).EndInit();
            this.contentPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionLabel _maxCPUBusyLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _maxMemoryUsageLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _statsIntervalLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _traceObjectLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _traceFilePathLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _traceFileNameLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _configurationPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _buttonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _refreshButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _applyButton;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _maxCPUBusyUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _traceFilePathTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _traceFileNameTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _traceObjectComboBox;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _statsIntervalUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _maxMemoryUsageUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox traceGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox thresholdsGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _statusGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _stateLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _statusTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _ruleIntervalLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _ruleIntervalUpDown;
        //private Trafodion.Manager.Framework.Controls.TrafodionTextBox _maxRowsFetchedTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _maxRowsFetchedUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _maxRowsFetchedLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _holdTimeoutUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _holdTimeoutLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _waitTimeoutUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _waitTimeoutLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _execTimeoutUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _execTimeoutLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _timeOutGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _holdButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _releaseButton;
        private System.Windows.Forms.NumericUpDown _ruleIntervalQueryExecTimeUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _startButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _stopButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _exportButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton helpButton;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _maxOverflowUsageLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _maxOverflowUsageUpDown;
        private Framework.Controls.TrafodionGroupBox grbCanaryQuery;
        private Framework.Controls.TrafodionLabel lblMaxESP;
        private Framework.Controls.TrafodionNumericUpDown nudMaxESP;
        private Framework.Controls.TrafodionLabel lblMaxExecQuery;
        private Framework.Controls.TrafodionNumericUpDown nudMaxExecQuery;
        private System.Windows.Forms.Label lblCanaryExec;
        private System.Windows.Forms.NumericUpDown nudCanaryExec;
        private System.Windows.Forms.Label lblCanaryInterval;
        private System.Windows.Forms.NumericUpDown nudCanaryInterval;
        private Framework.Controls.TrafodionLabel lblCanaryQuery;
        private Framework.Controls.TrafodionTextBox txtCanaryQuery;
        private Framework.Controls.TrafodionGroupBox wmsSettingGroupBox;
        private Framework.Controls.TrafodionButton btnReset;
        private System.Windows.Forms.Label lblCanaryTimeout;
        private System.Windows.Forms.NumericUpDown nudCanaryTimeout;
        private Framework.Controls.TrafodionLabel _ruleIntervalQueryExecTimeLabel;
        private Framework.Controls.TrafodionLabel lblCancelQuery;
        private Framework.Controls.TrafodionLabel _maxTransactionRollbackLabel;
        private Framework.Controls.TrafodionNumericUpDown _maxTransactionRollbackUpDown;
        private Framework.Controls.TrafodionLabel lblCheckQueryEstimatedResourceUse;
        private Framework.Controls.TrafodionComboBox ddlCheckQueryEstimatedResourceUse;
        private Framework.Controls.TrafodionComboBox ddlCancelQueryIfClientDisappears;
        private Framework.Controls.TrafodionPanel contentPanel;
    }
}
