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
﻿namespace Trafodion.Manager.OverviewArea.Controls
{
    partial class SystemSummaryConfigurationUserControl
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
            this.buttonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.btnHelp = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.btnClose = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.btnReset = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.btnApply = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.contentPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.perfMetricsGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.tableLayoutPanelPerformanceCharts = new System.Windows.Forms.TableLayoutPanel();
            this.tseButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.tbxTse = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.cbxTse = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.cbxAllMetrics = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.cbxCore = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.cbxDisk = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.cbxLoad_Avg = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.cbxVirtual_Memory = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.cbxNetwork_Rcv = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.cbxNetwork_Txn = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.cbxMemory = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.cbxSwap = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.metricsLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.colorSettingLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.thresholdsLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.coreBusyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.diskIOButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.loadAvgButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.contextSwitchButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.networkRcvButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.networkXmitButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.memUsedButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.swapUsedButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.fileSystemButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.tbxCore = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.tbxDisk = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.tbxLoad_Avg = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.tbxVirtual_Memory = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.tbxNetwork_Rcv = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.tbxNetwork_Txn = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.tbxMemory = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.tbxSwap = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.tbxFile_System = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.cbxFile_System = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.healthStateGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.lblAlarmSound = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.btnBrowseAlarm = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.btnTestAlarm = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.ddlAlarmSounds = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.cbxAllHealthState = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.cbxAccess = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.cbxDatabase = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.cbxStorage = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.cbxFoundation = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.cbxServer = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.cbxOS = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.portPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.tbxPortNumberComboBox = new System.Windows.Forms.ComboBox();
            this.sessionRetryTimerLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.tbxSessionRetryTimer = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.portNumberLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theMessagePanel = new Trafodion.Manager.Framework.Controls.TrafodionMessagePanel();
            this.buttonsPanel.SuspendLayout();
            this.contentPanel.SuspendLayout();
            this.perfMetricsGroupBox.SuspendLayout();
            this.tableLayoutPanelPerformanceCharts.SuspendLayout();
            this.healthStateGroupBox.SuspendLayout();
            this.portPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // buttonsPanel
            // 
            this.buttonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.buttonsPanel.Controls.Add(this.btnHelp);
            this.buttonsPanel.Controls.Add(this.btnClose);
            this.buttonsPanel.Controls.Add(this.btnReset);
            this.buttonsPanel.Controls.Add(this.btnApply);
            this.buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.buttonsPanel.Location = new System.Drawing.Point(5, 537);
            this.buttonsPanel.Name = "buttonsPanel";
            this.buttonsPanel.Padding = new System.Windows.Forms.Padding(2);
            this.buttonsPanel.Size = new System.Drawing.Size(604, 29);
            this.buttonsPanel.TabIndex = 1;
            // 
            // btnHelp
            // 
            this.btnHelp.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnHelp.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.btnHelp.Location = new System.Drawing.Point(514, 2);
            this.btnHelp.Name = "btnHelp";
            this.btnHelp.Size = new System.Drawing.Size(88, 25);
            this.btnHelp.TabIndex = 3;
            this.btnHelp.Text = "&Help";
            this.btnHelp.UseVisualStyleBackColor = true;
            this.btnHelp.Click += new System.EventHandler(this.btnHelp_Click);
            // 
            // btnClose
            // 
            this.btnClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnClose.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.btnClose.Location = new System.Drawing.Point(423, 2);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(88, 25);
            this.btnClose.TabIndex = 2;
            this.btnClose.Text = "&Close";
            this.btnClose.UseVisualStyleBackColor = true;
            this.btnClose.Click += new System.EventHandler(this.btnClose_Click);
            // 
            // btnReset
            // 
            this.btnReset.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnReset.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.btnReset.Location = new System.Drawing.Point(332, 2);
            this.btnReset.Name = "btnReset";
            this.btnReset.Size = new System.Drawing.Size(88, 25);
            this.btnReset.TabIndex = 1;
            this.btnReset.Text = "&Reset";
            this.btnReset.UseVisualStyleBackColor = true;
            this.btnReset.Click += new System.EventHandler(this.btnReset_Click);
            // 
            // btnApply
            // 
            this.btnApply.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnApply.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.btnApply.Location = new System.Drawing.Point(242, 2);
            this.btnApply.Name = "btnApply";
            this.btnApply.Size = new System.Drawing.Size(88, 25);
            this.btnApply.TabIndex = 0;
            this.btnApply.Text = "&Apply";
            this.btnApply.UseVisualStyleBackColor = true;
            this.btnApply.Click += new System.EventHandler(this.btnApply_Click);
            // 
            // contentPanel
            // 
            this.contentPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.contentPanel.Controls.Add(this.perfMetricsGroupBox);
            this.contentPanel.Controls.Add(this.healthStateGroupBox);
            this.contentPanel.Controls.Add(this.portPanel);
            this.contentPanel.Controls.Add(this._theMessagePanel);
            this.contentPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.contentPanel.Location = new System.Drawing.Point(5, 5);
            this.contentPanel.Name = "contentPanel";
            this.contentPanel.Size = new System.Drawing.Size(604, 532);
            this.contentPanel.TabIndex = 0;
            // 
            // perfMetricsGroupBox
            // 
            this.perfMetricsGroupBox.Controls.Add(this.tableLayoutPanelPerformanceCharts);
            this.perfMetricsGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.perfMetricsGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.perfMetricsGroupBox.Location = new System.Drawing.Point(0, 150);
            this.perfMetricsGroupBox.Name = "perfMetricsGroupBox";
            this.perfMetricsGroupBox.Size = new System.Drawing.Size(604, 382);
            this.perfMetricsGroupBox.TabIndex = 6;
            this.perfMetricsGroupBox.TabStop = false;
            this.perfMetricsGroupBox.Text = "Performance Metrics";
            // 
            // tableLayoutPanelPerformanceCharts
            // 
            this.tableLayoutPanelPerformanceCharts.CellBorderStyle = System.Windows.Forms.TableLayoutPanelCellBorderStyle.Single;
            this.tableLayoutPanelPerformanceCharts.ColumnCount = 3;
            this.tableLayoutPanelPerformanceCharts.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 159F));
            this.tableLayoutPanelPerformanceCharts.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 266F));
            this.tableLayoutPanelPerformanceCharts.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.tseButton, 2, 3);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.tbxTse, 1, 3);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.cbxTse, 0, 3);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.cbxAllMetrics, 0, 11);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.cbxCore, 0, 1);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.cbxDisk, 0, 2);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.cbxLoad_Avg, 0, 4);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.cbxVirtual_Memory, 0, 5);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.cbxNetwork_Rcv, 0, 6);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.cbxNetwork_Txn, 0, 7);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.cbxMemory, 0, 8);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.cbxSwap, 0, 9);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.metricsLabel, 0, 0);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.colorSettingLabel, 2, 0);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.thresholdsLabel, 1, 0);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.coreBusyButton, 2, 1);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.diskIOButton, 2, 2);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.loadAvgButton, 2, 4);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.contextSwitchButton, 2, 5);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.networkRcvButton, 2, 6);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.networkXmitButton, 2, 7);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.memUsedButton, 2, 8);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.swapUsedButton, 2, 9);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.fileSystemButton, 2, 10);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.tbxCore, 1, 1);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.tbxDisk, 1, 2);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.tbxLoad_Avg, 1, 4);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.tbxVirtual_Memory, 1, 5);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.tbxNetwork_Rcv, 1, 6);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.tbxNetwork_Txn, 1, 7);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.tbxMemory, 1, 8);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.tbxSwap, 1, 9);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.tbxFile_System, 1, 10);
            this.tableLayoutPanelPerformanceCharts.Controls.Add(this.cbxFile_System, 0, 10);
            this.tableLayoutPanelPerformanceCharts.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanelPerformanceCharts.GrowStyle = System.Windows.Forms.TableLayoutPanelGrowStyle.FixedSize;
            this.tableLayoutPanelPerformanceCharts.Location = new System.Drawing.Point(3, 17);
            this.tableLayoutPanelPerformanceCharts.Name = "tableLayoutPanelPerformanceCharts";
            this.tableLayoutPanelPerformanceCharts.RowCount = 12;
            this.tableLayoutPanelPerformanceCharts.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 30F));
            this.tableLayoutPanelPerformanceCharts.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelPerformanceCharts.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelPerformanceCharts.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelPerformanceCharts.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelPerformanceCharts.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelPerformanceCharts.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelPerformanceCharts.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelPerformanceCharts.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelPerformanceCharts.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelPerformanceCharts.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelPerformanceCharts.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 30F));
            this.tableLayoutPanelPerformanceCharts.Size = new System.Drawing.Size(598, 362);
            this.tableLayoutPanelPerformanceCharts.TabIndex = 7;
            // 
            // tseButton
            // 
            this.tseButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.tseButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.tseButton.Location = new System.Drawing.Point(490, 95);
            this.tseButton.Name = "tseButton";
            this.tseButton.Size = new System.Drawing.Size(45, 23);
            this.tseButton.TabIndex = 185;
            this.tseButton.Text = "...";
            this.tseButton.UseVisualStyleBackColor = true;
            this.tseButton.Click += new System.EventHandler(this.tseButton_Click);
            // 
            // tbxTse
            // 
            this.tbxTse.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.tbxTse.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.tbxTse.Location = new System.Drawing.Point(164, 96);
            this.tbxTse.Name = "tbxTse";
            this.tbxTse.Size = new System.Drawing.Size(260, 21);
            this.tbxTse.TabIndex = 184;
            this.tbxTse.Tag = "Disk IOs";
            this.tbxTse.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.tbxTse.TextChanged += new System.EventHandler(this.tbxMetric_TextChanged);
            // 
            // cbxTse
            // 
            this.cbxTse.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.cbxTse.AutoSize = true;
            this.cbxTse.Checked = true;
            this.cbxTse.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbxTse.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxTse.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxTse.Location = new System.Drawing.Point(4, 97);
            this.cbxTse.Name = "cbxTse";
            this.cbxTse.Size = new System.Drawing.Size(153, 18);
            this.cbxTse.TabIndex = 183;
            this.cbxTse.Text = "TSE Skew";
            this.cbxTse.UseVisualStyleBackColor = true;
            this.cbxTse.CheckedChanged += new System.EventHandler(this.cbxTse_CheckedChanged);
            // 
            // cbxAllMetrics
            // 
            this.cbxAllMetrics.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.cbxAllMetrics.Appearance = System.Windows.Forms.Appearance.Button;
            this.cbxAllMetrics.AutoSize = true;
            this.cbxAllMetrics.BackColor = System.Drawing.Color.WhiteSmoke;
            this.cbxAllMetrics.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
            this.cbxAllMetrics.Checked = true;
            this.cbxAllMetrics.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbxAllMetrics.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxAllMetrics.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxAllMetrics.Location = new System.Drawing.Point(4, 331);
            this.cbxAllMetrics.Name = "cbxAllMetrics";
            this.cbxAllMetrics.Size = new System.Drawing.Size(153, 23);
            this.cbxAllMetrics.TabIndex = 182;
            this.cbxAllMetrics.Text = "Select/Clear All Metrics";
            this.cbxAllMetrics.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.cbxAllMetrics.UseVisualStyleBackColor = false;
            this.cbxAllMetrics.Click += new System.EventHandler(this.cbxAllMetrics_Click);
            // 
            // cbxCore
            // 
            this.cbxCore.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.cbxCore.AutoSize = true;
            this.cbxCore.Checked = true;
            this.cbxCore.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbxCore.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxCore.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxCore.Location = new System.Drawing.Point(4, 37);
            this.cbxCore.Name = "cbxCore";
            this.cbxCore.Size = new System.Drawing.Size(153, 18);
            this.cbxCore.TabIndex = 113;
            this.cbxCore.Text = "Node/Core - %Busy";
            this.cbxCore.UseVisualStyleBackColor = true;
            this.cbxCore.CheckedChanged += new System.EventHandler(this.cbxCore_CheckedChanged);
            // 
            // cbxDisk
            // 
            this.cbxDisk.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.cbxDisk.AutoSize = true;
            this.cbxDisk.Checked = true;
            this.cbxDisk.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbxDisk.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxDisk.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxDisk.Location = new System.Drawing.Point(4, 67);
            this.cbxDisk.Name = "cbxDisk";
            this.cbxDisk.Size = new System.Drawing.Size(153, 18);
            this.cbxDisk.TabIndex = 121;
            this.cbxDisk.Text = "Disk IOs";
            this.cbxDisk.UseVisualStyleBackColor = true;
            this.cbxDisk.CheckedChanged += new System.EventHandler(this.cbxDisk_CheckedChanged);
            // 
            // cbxLoad_Avg
            // 
            this.cbxLoad_Avg.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.cbxLoad_Avg.AutoSize = true;
            this.cbxLoad_Avg.Checked = true;
            this.cbxLoad_Avg.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbxLoad_Avg.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxLoad_Avg.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxLoad_Avg.Location = new System.Drawing.Point(4, 127);
            this.cbxLoad_Avg.Name = "cbxLoad_Avg";
            this.cbxLoad_Avg.Size = new System.Drawing.Size(153, 18);
            this.cbxLoad_Avg.TabIndex = 129;
            this.cbxLoad_Avg.Text = "Load Average";
            this.cbxLoad_Avg.UseVisualStyleBackColor = true;
            this.cbxLoad_Avg.CheckedChanged += new System.EventHandler(this.cbxLoadAvg_CheckedChanged);
            // 
            // cbxVirtual_Memory
            // 
            this.cbxVirtual_Memory.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.cbxVirtual_Memory.AutoSize = true;
            this.cbxVirtual_Memory.Checked = true;
            this.cbxVirtual_Memory.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbxVirtual_Memory.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxVirtual_Memory.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxVirtual_Memory.Location = new System.Drawing.Point(4, 156);
            this.cbxVirtual_Memory.Name = "cbxVirtual_Memory";
            this.cbxVirtual_Memory.Size = new System.Drawing.Size(153, 18);
            this.cbxVirtual_Memory.TabIndex = 133;
            this.cbxVirtual_Memory.Text = "Context Switches";
            this.cbxVirtual_Memory.UseVisualStyleBackColor = true;
            this.cbxVirtual_Memory.CheckedChanged += new System.EventHandler(this.cbxVirtualMemory_CheckedChanged);
            // 
            // cbxNetwork_Rcv
            // 
            this.cbxNetwork_Rcv.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.cbxNetwork_Rcv.AutoSize = true;
            this.cbxNetwork_Rcv.Checked = true;
            this.cbxNetwork_Rcv.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbxNetwork_Rcv.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxNetwork_Rcv.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxNetwork_Rcv.Location = new System.Drawing.Point(4, 185);
            this.cbxNetwork_Rcv.Name = "cbxNetwork_Rcv";
            this.cbxNetwork_Rcv.Size = new System.Drawing.Size(153, 18);
            this.cbxNetwork_Rcv.TabIndex = 138;
            this.cbxNetwork_Rcv.Text = "Network Rcvs";
            this.cbxNetwork_Rcv.UseVisualStyleBackColor = true;
            this.cbxNetwork_Rcv.CheckedChanged += new System.EventHandler(this.cbxNetworkRcv_CheckedChanged);
            // 
            // cbxNetwork_Txn
            // 
            this.cbxNetwork_Txn.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.cbxNetwork_Txn.AutoSize = true;
            this.cbxNetwork_Txn.Checked = true;
            this.cbxNetwork_Txn.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbxNetwork_Txn.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxNetwork_Txn.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxNetwork_Txn.Location = new System.Drawing.Point(4, 214);
            this.cbxNetwork_Txn.Name = "cbxNetwork_Txn";
            this.cbxNetwork_Txn.Size = new System.Drawing.Size(153, 18);
            this.cbxNetwork_Txn.TabIndex = 141;
            this.cbxNetwork_Txn.Text = "Network Xmits";
            this.cbxNetwork_Txn.UseVisualStyleBackColor = true;
            this.cbxNetwork_Txn.CheckedChanged += new System.EventHandler(this.cbxNetworkTxn_CheckedChanged);
            // 
            // cbxMemory
            // 
            this.cbxMemory.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.cbxMemory.AutoSize = true;
            this.cbxMemory.Checked = true;
            this.cbxMemory.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbxMemory.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxMemory.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxMemory.Location = new System.Drawing.Point(4, 243);
            this.cbxMemory.Name = "cbxMemory";
            this.cbxMemory.Size = new System.Drawing.Size(153, 18);
            this.cbxMemory.TabIndex = 145;
            this.cbxMemory.Text = "Memory -%Memory Used";
            this.cbxMemory.UseVisualStyleBackColor = true;
            this.cbxMemory.CheckedChanged += new System.EventHandler(this.cbxMemory_CheckedChanged);
            // 
            // cbxSwap
            // 
            this.cbxSwap.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.cbxSwap.AutoSize = true;
            this.cbxSwap.Checked = true;
            this.cbxSwap.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbxSwap.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxSwap.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxSwap.Location = new System.Drawing.Point(4, 272);
            this.cbxSwap.Name = "cbxSwap";
            this.cbxSwap.Size = new System.Drawing.Size(153, 18);
            this.cbxSwap.TabIndex = 149;
            this.cbxSwap.Text = "Memory -%Swap Used";
            this.cbxSwap.UseVisualStyleBackColor = true;
            this.cbxSwap.CheckedChanged += new System.EventHandler(this.cbxSwap_CheckedChanged);
            // 
            // metricsLabel
            // 
            this.metricsLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.metricsLabel.AutoSize = true;
            this.metricsLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.metricsLabel.Location = new System.Drawing.Point(4, 9);
            this.metricsLabel.Name = "metricsLabel";
            this.metricsLabel.Size = new System.Drawing.Size(153, 13);
            this.metricsLabel.TabIndex = 1;
            this.metricsLabel.Text = "Metric";
            this.metricsLabel.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // colorSettingLabel
            // 
            this.colorSettingLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.colorSettingLabel.AutoSize = true;
            this.colorSettingLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.colorSettingLabel.Location = new System.Drawing.Point(431, 9);
            this.colorSettingLabel.Name = "colorSettingLabel";
            this.colorSettingLabel.Size = new System.Drawing.Size(163, 13);
            this.colorSettingLabel.TabIndex = 161;
            this.colorSettingLabel.Text = "Color Settings";
            this.colorSettingLabel.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // thresholdsLabel
            // 
            this.thresholdsLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.thresholdsLabel.AutoSize = true;
            this.thresholdsLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.thresholdsLabel.Location = new System.Drawing.Point(164, 9);
            this.thresholdsLabel.Name = "thresholdsLabel";
            this.thresholdsLabel.Size = new System.Drawing.Size(260, 13);
            this.thresholdsLabel.TabIndex = 2;
            this.thresholdsLabel.Text = "Threshold";
            this.thresholdsLabel.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // coreBusyButton
            // 
            this.coreBusyButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.coreBusyButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.coreBusyButton.Location = new System.Drawing.Point(490, 35);
            this.coreBusyButton.Name = "coreBusyButton";
            this.coreBusyButton.Size = new System.Drawing.Size(45, 23);
            this.coreBusyButton.TabIndex = 162;
            this.coreBusyButton.Text = "...";
            this.coreBusyButton.UseVisualStyleBackColor = true;
            this.coreBusyButton.Click += new System.EventHandler(this.coreBusyButton_Click);
            // 
            // diskIOButton
            // 
            this.diskIOButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.diskIOButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.diskIOButton.Location = new System.Drawing.Point(490, 65);
            this.diskIOButton.Name = "diskIOButton";
            this.diskIOButton.Size = new System.Drawing.Size(45, 23);
            this.diskIOButton.TabIndex = 164;
            this.diskIOButton.Text = "...";
            this.diskIOButton.UseVisualStyleBackColor = true;
            this.diskIOButton.Click += new System.EventHandler(this.diskIOButton_Click);
            // 
            // loadAvgButton
            // 
            this.loadAvgButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.loadAvgButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.loadAvgButton.Location = new System.Drawing.Point(490, 125);
            this.loadAvgButton.Name = "loadAvgButton";
            this.loadAvgButton.Size = new System.Drawing.Size(45, 22);
            this.loadAvgButton.TabIndex = 165;
            this.loadAvgButton.Text = "...";
            this.loadAvgButton.UseVisualStyleBackColor = true;
            this.loadAvgButton.Click += new System.EventHandler(this.loadAvgButton_Click);
            // 
            // contextSwitchButton
            // 
            this.contextSwitchButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.contextSwitchButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.contextSwitchButton.Location = new System.Drawing.Point(490, 154);
            this.contextSwitchButton.Name = "contextSwitchButton";
            this.contextSwitchButton.Size = new System.Drawing.Size(45, 22);
            this.contextSwitchButton.TabIndex = 166;
            this.contextSwitchButton.Text = "...";
            this.contextSwitchButton.UseVisualStyleBackColor = true;
            this.contextSwitchButton.Click += new System.EventHandler(this.contextSwitchButton_Click);
            // 
            // networkRcvButton
            // 
            this.networkRcvButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.networkRcvButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.networkRcvButton.Location = new System.Drawing.Point(490, 183);
            this.networkRcvButton.Name = "networkRcvButton";
            this.networkRcvButton.Size = new System.Drawing.Size(45, 22);
            this.networkRcvButton.TabIndex = 167;
            this.networkRcvButton.Text = "...";
            this.networkRcvButton.UseVisualStyleBackColor = true;
            this.networkRcvButton.Click += new System.EventHandler(this.networkRcvButton_Click);
            // 
            // networkXmitButton
            // 
            this.networkXmitButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.networkXmitButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.networkXmitButton.Location = new System.Drawing.Point(490, 212);
            this.networkXmitButton.Name = "networkXmitButton";
            this.networkXmitButton.Size = new System.Drawing.Size(45, 22);
            this.networkXmitButton.TabIndex = 168;
            this.networkXmitButton.Text = "...";
            this.networkXmitButton.UseVisualStyleBackColor = true;
            this.networkXmitButton.Click += new System.EventHandler(this.networkXmitButton_Click);
            // 
            // memUsedButton
            // 
            this.memUsedButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.memUsedButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.memUsedButton.Location = new System.Drawing.Point(490, 241);
            this.memUsedButton.Name = "memUsedButton";
            this.memUsedButton.Size = new System.Drawing.Size(45, 22);
            this.memUsedButton.TabIndex = 169;
            this.memUsedButton.Text = "...";
            this.memUsedButton.UseVisualStyleBackColor = true;
            this.memUsedButton.Click += new System.EventHandler(this.memUsedButton_Click);
            // 
            // swapUsedButton
            // 
            this.swapUsedButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.swapUsedButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.swapUsedButton.Location = new System.Drawing.Point(490, 270);
            this.swapUsedButton.Name = "swapUsedButton";
            this.swapUsedButton.Size = new System.Drawing.Size(45, 22);
            this.swapUsedButton.TabIndex = 170;
            this.swapUsedButton.Text = "...";
            this.swapUsedButton.UseVisualStyleBackColor = true;
            this.swapUsedButton.Click += new System.EventHandler(this.swapUsedButton_Click);
            // 
            // fileSystemButton
            // 
            this.fileSystemButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.fileSystemButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.fileSystemButton.Location = new System.Drawing.Point(490, 299);
            this.fileSystemButton.Name = "fileSystemButton";
            this.fileSystemButton.Size = new System.Drawing.Size(45, 22);
            this.fileSystemButton.TabIndex = 171;
            this.fileSystemButton.Text = "...";
            this.fileSystemButton.UseVisualStyleBackColor = true;
            this.fileSystemButton.Click += new System.EventHandler(this.fileSystemButton_Click);
            // 
            // tbxCore
            // 
            this.tbxCore.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.tbxCore.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.tbxCore.Location = new System.Drawing.Point(164, 36);
            this.tbxCore.Name = "tbxCore";
            this.tbxCore.Size = new System.Drawing.Size(260, 21);
            this.tbxCore.TabIndex = 163;
            this.tbxCore.Tag = "Core Busy";
            this.tbxCore.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.tbxCore.TextChanged += new System.EventHandler(this.tbxMetric_TextChanged);
            // 
            // tbxDisk
            // 
            this.tbxDisk.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.tbxDisk.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.tbxDisk.Location = new System.Drawing.Point(164, 66);
            this.tbxDisk.Name = "tbxDisk";
            this.tbxDisk.Size = new System.Drawing.Size(260, 21);
            this.tbxDisk.TabIndex = 123;
            this.tbxDisk.Tag = "Disk IOs";
            this.tbxDisk.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.tbxDisk.TextChanged += new System.EventHandler(this.tbxMetric_TextChanged);
            // 
            // tbxLoad_Avg
            // 
            this.tbxLoad_Avg.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.tbxLoad_Avg.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.tbxLoad_Avg.Location = new System.Drawing.Point(164, 125);
            this.tbxLoad_Avg.Name = "tbxLoad_Avg";
            this.tbxLoad_Avg.Size = new System.Drawing.Size(260, 21);
            this.tbxLoad_Avg.TabIndex = 131;
            this.tbxLoad_Avg.Tag = "Load Average";
            this.tbxLoad_Avg.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.tbxLoad_Avg.TextChanged += new System.EventHandler(this.tbxMetric_TextChanged);
            // 
            // tbxVirtual_Memory
            // 
            this.tbxVirtual_Memory.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.tbxVirtual_Memory.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.tbxVirtual_Memory.Location = new System.Drawing.Point(164, 154);
            this.tbxVirtual_Memory.Name = "tbxVirtual_Memory";
            this.tbxVirtual_Memory.Size = new System.Drawing.Size(260, 21);
            this.tbxVirtual_Memory.TabIndex = 135;
            this.tbxVirtual_Memory.Tag = "Context Switches";
            this.tbxVirtual_Memory.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.tbxVirtual_Memory.TextChanged += new System.EventHandler(this.tbxMetric_TextChanged);
            // 
            // tbxNetwork_Rcv
            // 
            this.tbxNetwork_Rcv.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.tbxNetwork_Rcv.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.tbxNetwork_Rcv.Location = new System.Drawing.Point(164, 183);
            this.tbxNetwork_Rcv.Name = "tbxNetwork_Rcv";
            this.tbxNetwork_Rcv.Size = new System.Drawing.Size(260, 21);
            this.tbxNetwork_Rcv.TabIndex = 136;
            this.tbxNetwork_Rcv.Tag = "Network Rcvs";
            this.tbxNetwork_Rcv.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.tbxNetwork_Rcv.TextChanged += new System.EventHandler(this.tbxMetric_TextChanged);
            // 
            // tbxNetwork_Txn
            // 
            this.tbxNetwork_Txn.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.tbxNetwork_Txn.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.tbxNetwork_Txn.Location = new System.Drawing.Point(164, 212);
            this.tbxNetwork_Txn.Name = "tbxNetwork_Txn";
            this.tbxNetwork_Txn.Size = new System.Drawing.Size(260, 21);
            this.tbxNetwork_Txn.TabIndex = 143;
            this.tbxNetwork_Txn.Tag = "Network Xmits";
            this.tbxNetwork_Txn.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.tbxNetwork_Txn.TextChanged += new System.EventHandler(this.tbxMetric_TextChanged);
            // 
            // tbxMemory
            // 
            this.tbxMemory.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.tbxMemory.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.tbxMemory.Location = new System.Drawing.Point(164, 241);
            this.tbxMemory.Name = "tbxMemory";
            this.tbxMemory.Size = new System.Drawing.Size(260, 21);
            this.tbxMemory.TabIndex = 147;
            this.tbxMemory.Tag = "Memory Used";
            this.tbxMemory.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.tbxMemory.TextChanged += new System.EventHandler(this.tbxMetric_TextChanged);
            // 
            // tbxSwap
            // 
            this.tbxSwap.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.tbxSwap.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.tbxSwap.Location = new System.Drawing.Point(164, 270);
            this.tbxSwap.Name = "tbxSwap";
            this.tbxSwap.Size = new System.Drawing.Size(260, 21);
            this.tbxSwap.TabIndex = 151;
            this.tbxSwap.Tag = "Swap Used";
            this.tbxSwap.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.tbxSwap.TextChanged += new System.EventHandler(this.tbxMetric_TextChanged);
            // 
            // tbxFile_System
            // 
            this.tbxFile_System.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.tbxFile_System.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.tbxFile_System.Location = new System.Drawing.Point(164, 299);
            this.tbxFile_System.Name = "tbxFile_System";
            this.tbxFile_System.Size = new System.Drawing.Size(260, 21);
            this.tbxFile_System.TabIndex = 155;
            this.tbxFile_System.Tag = "File System Consumed";
            this.tbxFile_System.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.tbxFile_System.TextChanged += new System.EventHandler(this.tbxMetric_TextChanged);
            // 
            // cbxFile_System
            // 
            this.cbxFile_System.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.cbxFile_System.AutoSize = true;
            this.cbxFile_System.Checked = true;
            this.cbxFile_System.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbxFile_System.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxFile_System.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxFile_System.Location = new System.Drawing.Point(4, 301);
            this.cbxFile_System.Name = "cbxFile_System";
            this.cbxFile_System.Size = new System.Drawing.Size(153, 18);
            this.cbxFile_System.TabIndex = 153;
            this.cbxFile_System.Text = "FileSys - %Consumed";
            this.cbxFile_System.UseVisualStyleBackColor = true;
            this.cbxFile_System.CheckedChanged += new System.EventHandler(this.cbxFileSystem_CheckedChanged);
            // 
            // healthStateGroupBox
            // 
            this.healthStateGroupBox.Controls.Add(this.lblAlarmSound);
            this.healthStateGroupBox.Controls.Add(this.btnBrowseAlarm);
            this.healthStateGroupBox.Controls.Add(this.btnTestAlarm);
            this.healthStateGroupBox.Controls.Add(this.ddlAlarmSounds);
            this.healthStateGroupBox.Controls.Add(this.cbxAllHealthState);
            this.healthStateGroupBox.Controls.Add(this.cbxAccess);
            this.healthStateGroupBox.Controls.Add(this.cbxDatabase);
            this.healthStateGroupBox.Controls.Add(this.cbxStorage);
            this.healthStateGroupBox.Controls.Add(this.cbxFoundation);
            this.healthStateGroupBox.Controls.Add(this.cbxServer);
            this.healthStateGroupBox.Controls.Add(this.cbxOS);
            this.healthStateGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this.healthStateGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.healthStateGroupBox.Location = new System.Drawing.Point(0, 59);
            this.healthStateGroupBox.Name = "healthStateGroupBox";
            this.healthStateGroupBox.Size = new System.Drawing.Size(604, 91);
            this.healthStateGroupBox.TabIndex = 5;
            this.healthStateGroupBox.TabStop = false;
            this.healthStateGroupBox.Text = "Health/State";
            // 
            // lblAlarmSound
            // 
            this.lblAlarmSound.AutoSize = true;
            this.lblAlarmSound.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.lblAlarmSound.Location = new System.Drawing.Point(6, 58);
            this.lblAlarmSound.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.lblAlarmSound.Name = "lblAlarmSound";
            this.lblAlarmSound.Size = new System.Drawing.Size(251, 13);
            this.lblAlarmSound.TabIndex = 10;
            this.lblAlarmSound.Text = "When a health state changes, play a sound:";
            this.lblAlarmSound.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // btnBrowseAlarm
            // 
            this.btnBrowseAlarm.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.btnBrowseAlarm.Location = new System.Drawing.Point(498, 54);
            this.btnBrowseAlarm.Name = "btnBrowseAlarm";
            this.btnBrowseAlarm.Size = new System.Drawing.Size(89, 23);
            this.btnBrowseAlarm.TabIndex = 9;
            this.btnBrowseAlarm.Text = "&Browse...";
            this.btnBrowseAlarm.UseVisualStyleBackColor = true;
            this.btnBrowseAlarm.Click += new System.EventHandler(this.btnBrowseAlarm_Click);
            // 
            // btnTestAlarm
            // 
            this.btnTestAlarm.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.btnTestAlarm.Location = new System.Drawing.Point(453, 54);
            this.btnTestAlarm.Name = "btnTestAlarm";
            this.btnTestAlarm.Size = new System.Drawing.Size(42, 23);
            this.btnTestAlarm.TabIndex = 8;
            this.btnTestAlarm.Text = "&Test";
            this.btnTestAlarm.UseVisualStyleBackColor = true;
            this.btnTestAlarm.Click += new System.EventHandler(this.btnTestAlarm_Click);
            // 
            // ddlAlarmSounds
            // 
            this.ddlAlarmSounds.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.ddlAlarmSounds.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.ddlAlarmSounds.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.ddlAlarmSounds.FormattingEnabled = true;
            this.ddlAlarmSounds.Items.AddRange(new object[] {
            ""});
            this.ddlAlarmSounds.Location = new System.Drawing.Point(263, 55);
            this.ddlAlarmSounds.Name = "ddlAlarmSounds";
            this.ddlAlarmSounds.Size = new System.Drawing.Size(186, 21);
            this.ddlAlarmSounds.TabIndex = 7;
            this.ddlAlarmSounds.SelectedIndexChanged += new System.EventHandler(this.ddlAlarmSounds_SelectedIndexChanged);
            // 
            // cbxAllHealthState
            // 
            this.cbxAllHealthState.Appearance = System.Windows.Forms.Appearance.Button;
            this.cbxAllHealthState.AutoSize = true;
            this.cbxAllHealthState.BackColor = System.Drawing.Color.WhiteSmoke;
            this.cbxAllHealthState.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxAllHealthState.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxAllHealthState.Location = new System.Drawing.Point(497, 20);
            this.cbxAllHealthState.Name = "cbxAllHealthState";
            this.cbxAllHealthState.Size = new System.Drawing.Size(89, 23);
            this.cbxAllHealthState.TabIndex = 6;
            this.cbxAllHealthState.Text = "Select/Clear All";
            this.cbxAllHealthState.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.cbxAllHealthState.UseVisualStyleBackColor = false;
            this.cbxAllHealthState.Click += new System.EventHandler(this.cbxAllHealthState_Click);
            // 
            // cbxAccess
            // 
            this.cbxAccess.AutoSize = true;
            this.cbxAccess.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxAccess.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxAccess.Location = new System.Drawing.Point(6, 23);
            this.cbxAccess.Name = "cbxAccess";
            this.cbxAccess.Size = new System.Drawing.Size(65, 18);
            this.cbxAccess.TabIndex = 0;
            this.cbxAccess.Text = "Access";
            this.cbxAccess.UseVisualStyleBackColor = true;
            this.cbxAccess.CheckedChanged += new System.EventHandler(this.cbxHealthStateCheckedChanged);
            // 
            // cbxDatabase
            // 
            this.cbxDatabase.AutoSize = true;
            this.cbxDatabase.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxDatabase.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxDatabase.Location = new System.Drawing.Point(86, 23);
            this.cbxDatabase.Name = "cbxDatabase";
            this.cbxDatabase.Size = new System.Drawing.Size(78, 18);
            this.cbxDatabase.TabIndex = 1;
            this.cbxDatabase.Text = "Database";
            this.cbxDatabase.UseVisualStyleBackColor = true;
            this.cbxDatabase.CheckedChanged += new System.EventHandler(this.cbxHealthStateCheckedChanged);
            // 
            // cbxStorage
            // 
            this.cbxStorage.AutoSize = true;
            this.cbxStorage.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxStorage.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxStorage.Location = new System.Drawing.Point(420, 23);
            this.cbxStorage.Name = "cbxStorage";
            this.cbxStorage.Size = new System.Drawing.Size(70, 18);
            this.cbxStorage.TabIndex = 5;
            this.cbxStorage.Text = "Storage";
            this.cbxStorage.UseVisualStyleBackColor = true;
            this.cbxStorage.CheckedChanged += new System.EventHandler(this.cbxHealthStateCheckedChanged);
            // 
            // cbxFoundation
            // 
            this.cbxFoundation.AutoSize = true;
            this.cbxFoundation.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxFoundation.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxFoundation.Location = new System.Drawing.Point(179, 23);
            this.cbxFoundation.Name = "cbxFoundation";
            this.cbxFoundation.Size = new System.Drawing.Size(86, 18);
            this.cbxFoundation.TabIndex = 2;
            this.cbxFoundation.Text = "Foundation";
            this.cbxFoundation.UseVisualStyleBackColor = true;
            this.cbxFoundation.CheckedChanged += new System.EventHandler(this.cbxHealthStateCheckedChanged);
            // 
            // cbxServer
            // 
            this.cbxServer.AutoSize = true;
            this.cbxServer.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxServer.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxServer.Location = new System.Drawing.Point(341, 23);
            this.cbxServer.Name = "cbxServer";
            this.cbxServer.Size = new System.Drawing.Size(64, 18);
            this.cbxServer.TabIndex = 4;
            this.cbxServer.Text = "Server";
            this.cbxServer.UseVisualStyleBackColor = true;
            this.cbxServer.CheckedChanged += new System.EventHandler(this.cbxHealthStateCheckedChanged);
            // 
            // cbxOS
            // 
            this.cbxOS.AutoSize = true;
            this.cbxOS.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cbxOS.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cbxOS.Location = new System.Drawing.Point(280, 23);
            this.cbxOS.Name = "cbxOS";
            this.cbxOS.Size = new System.Drawing.Size(46, 18);
            this.cbxOS.TabIndex = 3;
            this.cbxOS.Text = "OS";
            this.cbxOS.UseVisualStyleBackColor = true;
            this.cbxOS.CheckedChanged += new System.EventHandler(this.cbxHealthStateCheckedChanged);
            // 
            // portPanel
            // 
            this.portPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.portPanel.Controls.Add(this.tbxPortNumberComboBox);
            this.portPanel.Controls.Add(this.sessionRetryTimerLabel);
            this.portPanel.Controls.Add(this.tbxSessionRetryTimer);
            this.portPanel.Controls.Add(this.portNumberLabel);
            this.portPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.portPanel.Location = new System.Drawing.Point(0, 22);
            this.portPanel.Name = "portPanel";
            this.portPanel.Size = new System.Drawing.Size(604, 37);
            this.portPanel.TabIndex = 4;
            this.portPanel.Visible = false;
            // 
            // tbxPortNumberComboBox
            // 
            this.tbxPortNumberComboBox.FormattingEnabled = true;
            this.tbxPortNumberComboBox.Items.AddRange(new object[] {
            "Default Port Number"});
            this.tbxPortNumberComboBox.Location = new System.Drawing.Point(147, 8);
            this.tbxPortNumberComboBox.Name = "tbxPortNumberComboBox";
            this.tbxPortNumberComboBox.Size = new System.Drawing.Size(125, 21);
            this.tbxPortNumberComboBox.TabIndex = 7;
            // 
            // sessionRetryTimerLabel
            // 
            this.sessionRetryTimerLabel.AutoSize = true;
            this.sessionRetryTimerLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.sessionRetryTimerLabel.Location = new System.Drawing.Point(302, 11);
            this.sessionRetryTimerLabel.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.sessionRetryTimerLabel.Name = "sessionRetryTimerLabel";
            this.sessionRetryTimerLabel.Size = new System.Drawing.Size(153, 13);
            this.sessionRetryTimerLabel.TabIndex = 2;
            this.sessionRetryTimerLabel.Text = "Session Retry Timer (sec)";
            this.sessionRetryTimerLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // tbxSessionRetryTimer
            // 
            this.tbxSessionRetryTimer.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.tbxSessionRetryTimer.Location = new System.Drawing.Point(461, 8);
            this.tbxSessionRetryTimer.Name = "tbxSessionRetryTimer";
            this.tbxSessionRetryTimer.Size = new System.Drawing.Size(125, 21);
            this.tbxSessionRetryTimer.TabIndex = 3;
            // 
            // portNumberLabel
            // 
            this.portNumberLabel.AutoSize = true;
            this.portNumberLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.portNumberLabel.Location = new System.Drawing.Point(6, 11);
            this.portNumberLabel.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.portNumberLabel.Name = "portNumberLabel";
            this.portNumberLabel.Size = new System.Drawing.Size(134, 13);
            this.portNumberLabel.TabIndex = 0;
            this.portNumberLabel.Text = "Live Feed Port Number";
            this.portNumberLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // _theMessagePanel
            // 
            this._theMessagePanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theMessagePanel.Location = new System.Drawing.Point(0, 0);
            this._theMessagePanel.Name = "_theMessagePanel";
            this._theMessagePanel.Size = new System.Drawing.Size(604, 22);
            this._theMessagePanel.TabIndex = 0;
            this._theMessagePanel.TabStop = false;
            // 
            // SystemSummaryConfigurationUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(614, 571);
            this.Controls.Add(this.contentPanel);
            this.Controls.Add(this.buttonsPanel);
            this.MaximizeBox = false;
            this.Name = "SystemSummaryConfigurationUserControl";
            this.Padding = new System.Windows.Forms.Padding(5);
            this.Text = "HP Database Manager - System Monitor Configuration";
            this.buttonsPanel.ResumeLayout(false);
            this.contentPanel.ResumeLayout(false);
            this.perfMetricsGroupBox.ResumeLayout(false);
            this.tableLayoutPanelPerformanceCharts.ResumeLayout(false);
            this.tableLayoutPanelPerformanceCharts.PerformLayout();
            this.healthStateGroupBox.ResumeLayout(false);
            this.healthStateGroupBox.PerformLayout();
            this.portPanel.ResumeLayout(false);
            this.portPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionPanel buttonsPanel;
        private Framework.Controls.TrafodionButton btnReset;
        private Framework.Controls.TrafodionButton btnApply;
        private Framework.Controls.TrafodionButton btnClose;
        private Framework.Controls.TrafodionPanel contentPanel;
        private Framework.Controls.TrafodionGroupBox perfMetricsGroupBox;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanelPerformanceCharts;
        private Framework.Controls.TrafodionCheckBox cbxCore;
        private Framework.Controls.TrafodionCheckBox cbxDisk;
        private Framework.Controls.TrafodionCheckBox cbxLoad_Avg;
        private Framework.Controls.TrafodionCheckBox cbxVirtual_Memory;
        private Framework.Controls.TrafodionCheckBox cbxNetwork_Rcv;
        private Framework.Controls.TrafodionCheckBox cbxNetwork_Txn;
        private Framework.Controls.TrafodionCheckBox cbxMemory;
        private Framework.Controls.TrafodionCheckBox cbxSwap;
        private Framework.Controls.TrafodionCheckBox cbxFile_System;
        private Framework.Controls.TrafodionLabel metricsLabel;
        private Framework.Controls.TrafodionLabel colorSettingLabel;
        private Framework.Controls.TrafodionLabel thresholdsLabel;
        private Framework.Controls.TrafodionButton coreBusyButton;
        private Framework.Controls.TrafodionButton diskIOButton;
        private Framework.Controls.TrafodionButton loadAvgButton;
        private Framework.Controls.TrafodionButton contextSwitchButton;
        private Framework.Controls.TrafodionButton networkRcvButton;
        private Framework.Controls.TrafodionButton networkXmitButton;
        private Framework.Controls.TrafodionButton memUsedButton;
        private Framework.Controls.TrafodionButton swapUsedButton;
        private Framework.Controls.TrafodionButton fileSystemButton;
        private Framework.Controls.TrafodionTextBox tbxCore;
        private Framework.Controls.TrafodionTextBox tbxDisk;
        private Framework.Controls.TrafodionTextBox tbxLoad_Avg;
        private Framework.Controls.TrafodionTextBox tbxVirtual_Memory;
        private Framework.Controls.TrafodionTextBox tbxNetwork_Rcv;
        private Framework.Controls.TrafodionTextBox tbxNetwork_Txn;
        private Framework.Controls.TrafodionTextBox tbxMemory;
        private Framework.Controls.TrafodionTextBox tbxSwap;
        private Framework.Controls.TrafodionTextBox tbxFile_System;
        private Framework.Controls.TrafodionPanel portPanel;
        private System.Windows.Forms.ComboBox tbxPortNumberComboBox;
        private Framework.Controls.TrafodionLabel sessionRetryTimerLabel;
        private Framework.Controls.TrafodionTextBox tbxSessionRetryTimer;
        private Framework.Controls.TrafodionLabel portNumberLabel;
        private Framework.Controls.TrafodionGroupBox healthStateGroupBox;
        private Framework.Controls.TrafodionCheckBox cbxAccess;
        private Framework.Controls.TrafodionCheckBox cbxDatabase;
        private Framework.Controls.TrafodionCheckBox cbxStorage;
        private Framework.Controls.TrafodionCheckBox cbxFoundation;
        private Framework.Controls.TrafodionCheckBox cbxServer;
        private Framework.Controls.TrafodionCheckBox cbxOS;
        private Framework.Controls.TrafodionMessagePanel _theMessagePanel;
        private Framework.Controls.TrafodionCheckBox cbxAllMetrics;
        private Framework.Controls.TrafodionCheckBox cbxAllHealthState;
        private Framework.Controls.TrafodionButton btnHelp;
        private Framework.Controls.TrafodionButton btnBrowseAlarm;
        private Framework.Controls.TrafodionButton btnTestAlarm;
        private Framework.Controls.TrafodionComboBox ddlAlarmSounds;
        private Framework.Controls.TrafodionLabel lblAlarmSound;
        private Framework.Controls.TrafodionButton tseButton;
        private Framework.Controls.TrafodionTextBox tbxTse;
        private Framework.Controls.TrafodionCheckBox cbxTse;

    }
}
