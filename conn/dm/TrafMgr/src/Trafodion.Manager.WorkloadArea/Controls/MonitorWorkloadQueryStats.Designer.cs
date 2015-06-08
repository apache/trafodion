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
﻿namespace Trafodion.Manager.WorkloadArea.Controls
{
    partial class MonitorWorkloadQueryStats
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MonitorWorkloadQueryStats));
            this._theQueryTypePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.totalCompleteCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.totalCancelCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.totalRejectCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.totalSuspendCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.totalHoldCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.totalWaitCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.totalExecCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.buttonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.resetStatsButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.refreshButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.panel3 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theChart = new Trafodion.Manager.WorkloadArea.Controls.WorkloadQueryStatsChart();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._statsGridPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._statsGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0AT" +
                    "QB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAI" +
                    "ABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            this.TrafodionIGrid1DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionIGrid1DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.TrafodionIGrid1RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this._theToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._theQueryTypePanel.SuspendLayout();
            this.buttonsPanel.SuspendLayout();
            this.panel3.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this._statsGridPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._statsGrid)).BeginInit();
            this.SuspendLayout();
            // 
            // _theQueryTypePanel
            // 
            this._theQueryTypePanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theQueryTypePanel.Controls.Add(this.totalCompleteCheckBox);
            this._theQueryTypePanel.Controls.Add(this.totalCancelCheckBox);
            this._theQueryTypePanel.Controls.Add(this.totalRejectCheckBox);
            this._theQueryTypePanel.Controls.Add(this.totalSuspendCheckBox);
            this._theQueryTypePanel.Controls.Add(this.totalHoldCheckBox);
            this._theQueryTypePanel.Controls.Add(this.totalWaitCheckBox);
            this._theQueryTypePanel.Controls.Add(this.totalExecCheckBox);
            this._theQueryTypePanel.Dock = System.Windows.Forms.DockStyle.Right;
            this._theQueryTypePanel.Location = new System.Drawing.Point(851, 0);
            this._theQueryTypePanel.Name = "_theQueryTypePanel";
            this._theQueryTypePanel.Size = new System.Drawing.Size(110, 529);
            this._theQueryTypePanel.TabIndex = 0;
            // 
            // totalCompleteCheckBox
            // 
            this.totalCompleteCheckBox.AutoSize = true;
            this.totalCompleteCheckBox.Checked = true;
            this.totalCompleteCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.totalCompleteCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.totalCompleteCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.totalCompleteCheckBox.Location = new System.Drawing.Point(6, 172);
            this.totalCompleteCheckBox.Name = "totalCompleteCheckBox";
            this.totalCompleteCheckBox.Size = new System.Drawing.Size(110, 18);
            this.totalCompleteCheckBox.TabIndex = 12;
            this.totalCompleteCheckBox.Text = "Total Completed";
            this.totalCompleteCheckBox.UseVisualStyleBackColor = true;
            this.totalCompleteCheckBox.CheckedChanged += new System.EventHandler(this.totalCompleteCheckBox_CheckedChanged);
            // 
            // totalCancelCheckBox
            // 
            this.totalCancelCheckBox.AutoSize = true;
            this.totalCancelCheckBox.Checked = true;
            this.totalCancelCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.totalCancelCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.totalCancelCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.totalCancelCheckBox.Location = new System.Drawing.Point(6, 145);
            this.totalCancelCheckBox.Name = "totalCancelCheckBox";
            this.totalCancelCheckBox.Size = new System.Drawing.Size(103, 18);
            this.totalCancelCheckBox.TabIndex = 10;
            this.totalCancelCheckBox.Text = "Total Canceled";
            this.totalCancelCheckBox.UseVisualStyleBackColor = true;
            this.totalCancelCheckBox.CheckedChanged += new System.EventHandler(this.totalCancelCheckBox_CheckedChanged);
            // 
            // totalRejectCheckBox
            // 
            this.totalRejectCheckBox.AutoSize = true;
            this.totalRejectCheckBox.Checked = true;
            this.totalRejectCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.totalRejectCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.totalRejectCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.totalRejectCheckBox.Location = new System.Drawing.Point(6, 118);
            this.totalRejectCheckBox.Name = "totalRejectCheckBox";
            this.totalRejectCheckBox.Size = new System.Drawing.Size(102, 18);
            this.totalRejectCheckBox.TabIndex = 8;
            this.totalRejectCheckBox.Text = "Total Rejected";
            this.totalRejectCheckBox.UseVisualStyleBackColor = true;
            this.totalRejectCheckBox.CheckedChanged += new System.EventHandler(this.totalRejectCheckBox_CheckedChanged);
            // 
            // totalSuspendCheckBox
            // 
            this.totalSuspendCheckBox.AutoSize = true;
            this.totalSuspendCheckBox.Checked = true;
            this.totalSuspendCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.totalSuspendCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.totalSuspendCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.totalSuspendCheckBox.Location = new System.Drawing.Point(6, 91);
            this.totalSuspendCheckBox.Name = "totalSuspendCheckBox";
            this.totalSuspendCheckBox.Size = new System.Drawing.Size(112, 18);
            this.totalSuspendCheckBox.TabIndex = 6;
            this.totalSuspendCheckBox.Text = "Total Suspended";
            this.totalSuspendCheckBox.UseVisualStyleBackColor = true;
            this.totalSuspendCheckBox.CheckedChanged += new System.EventHandler(this.totalSuspendCheckBox_CheckedChanged);
            // 
            // totalHoldCheckBox
            // 
            this.totalHoldCheckBox.AutoSize = true;
            this.totalHoldCheckBox.Checked = true;
            this.totalHoldCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.totalHoldCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.totalHoldCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.totalHoldCheckBox.Location = new System.Drawing.Point(6, 64);
            this.totalHoldCheckBox.Name = "totalHoldCheckBox";
            this.totalHoldCheckBox.Size = new System.Drawing.Size(94, 18);
            this.totalHoldCheckBox.TabIndex = 4;
            this.totalHoldCheckBox.Text = "Total Holding";
            this.totalHoldCheckBox.UseVisualStyleBackColor = true;
            this.totalHoldCheckBox.CheckedChanged += new System.EventHandler(this.totalHoldCheckBox_CheckedChanged);
            // 
            // totalWaitCheckBox
            // 
            this.totalWaitCheckBox.AutoSize = true;
            this.totalWaitCheckBox.Checked = true;
            this.totalWaitCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.totalWaitCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.totalWaitCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.totalWaitCheckBox.Location = new System.Drawing.Point(6, 37);
            this.totalWaitCheckBox.Name = "totalWaitCheckBox";
            this.totalWaitCheckBox.Size = new System.Drawing.Size(95, 18);
            this.totalWaitCheckBox.TabIndex = 2;
            this.totalWaitCheckBox.Text = "Total Waiting";
            this.totalWaitCheckBox.UseVisualStyleBackColor = true;
            this.totalWaitCheckBox.CheckedChanged += new System.EventHandler(this.totalWaitCheckBox_CheckedChanged);
            // 
            // totalExecCheckBox
            // 
            this.totalExecCheckBox.AutoSize = true;
            this.totalExecCheckBox.Checked = true;
            this.totalExecCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.totalExecCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.totalExecCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.totalExecCheckBox.Location = new System.Drawing.Point(6, 10);
            this.totalExecCheckBox.Name = "totalExecCheckBox";
            this.totalExecCheckBox.Size = new System.Drawing.Size(104, 18);
            this.totalExecCheckBox.TabIndex = 0;
            this.totalExecCheckBox.Text = "Total Executed";
            this.totalExecCheckBox.UseVisualStyleBackColor = true;
            this.totalExecCheckBox.CheckedChanged += new System.EventHandler(this.totalExecCheckBox_CheckedChanged);
            // 
            // buttonsPanel
            // 
            this.buttonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.buttonsPanel.Controls.Add(this.helpButton);
            this.buttonsPanel.Controls.Add(this.cancelButton);
            this.buttonsPanel.Controls.Add(this.resetStatsButton);
            this.buttonsPanel.Controls.Add(this.refreshButton);
            this.buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.buttonsPanel.Location = new System.Drawing.Point(0, 593);
            this.buttonsPanel.Name = "buttonsPanel";
            this.buttonsPanel.Size = new System.Drawing.Size(961, 38);
            this.buttonsPanel.TabIndex = 1;
            // 
            // helpButton
            // 
            this.helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.helpButton.Location = new System.Drawing.Point(865, 8);
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = new System.Drawing.Size(90, 23);
            this.helpButton.TabIndex = 2;
            this.helpButton.Text = "&Help";
            this.helpButton.UseVisualStyleBackColor = true;
            this.helpButton.Click += new System.EventHandler(this.helpButton_Click);
            // 
            // cancelButton
            // 
            this.cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cancelButton.Location = new System.Drawing.Point(766, 8);
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = new System.Drawing.Size(90, 23);
            this.cancelButton.TabIndex = 2;
            this.cancelButton.Text = "&Cancel";
            this.cancelButton.UseVisualStyleBackColor = true;
            this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
            // 
            // resetStatsButton
            // 
            this.resetStatsButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.resetStatsButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.resetStatsButton.Location = new System.Drawing.Point(667, 8);
            this.resetStatsButton.Name = "resetStatsButton";
            this.resetStatsButton.Size = new System.Drawing.Size(90, 23);
            this.resetStatsButton.TabIndex = 1;
            this.resetStatsButton.Text = "Reset &Stats";
            this.resetStatsButton.UseVisualStyleBackColor = true;
            this.resetStatsButton.Click += new System.EventHandler(this.resetStatsButton_Click);
            // 
            // refreshButton
            // 
            this.refreshButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.refreshButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.refreshButton.Location = new System.Drawing.Point(568, 8);
            this.refreshButton.Name = "refreshButton";
            this.refreshButton.Size = new System.Drawing.Size(90, 23);
            this.refreshButton.TabIndex = 0;
            this.refreshButton.Text = "&Refresh";
            this.refreshButton.UseVisualStyleBackColor = true;
            this.refreshButton.Click += new System.EventHandler(this.refreshButton_Click);
            // 
            // panel3
            // 
            this.panel3.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel3.Controls.Add(this._theChart);
            this.panel3.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel3.Location = new System.Drawing.Point(0, 0);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(851, 529);
            this.panel3.TabIndex = 2;
            // 
            // _theChart
            // 
            this._theChart.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theChart.Location = new System.Drawing.Point(0, 0);
            this._theChart.Name = "_theChart";
            this._theChart.Size = new System.Drawing.Size(851, 529);
            this._theChart.TabIndex = 1;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this.panel3);
            this.TrafodionPanel1.Controls.Add(this._theQueryTypePanel);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(961, 529);
            this.TrafodionPanel1.TabIndex = 13;
            // 
            // _statsGridPanel
            // 
            this._statsGridPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._statsGridPanel.Controls.Add(this._statsGrid);
            this._statsGridPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._statsGridPanel.Location = new System.Drawing.Point(0, 529);
            this._statsGridPanel.Name = "_statsGridPanel";
            this._statsGridPanel.Size = new System.Drawing.Size(961, 64);
            this._statsGridPanel.TabIndex = 13;
            // 
            // _statsGrid
            // 
            this._statsGrid.AllowColumnFilter = true;
            this._statsGrid.AllowWordWrap = false;
            this._statsGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("_statsGrid.AlwaysHiddenColumnNames")));
            this._statsGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this._statsGrid.CurrentFilter = null;
            this._statsGrid.DefaultCol.CellStyle = this.TrafodionIGrid1DefaultCellStyle;
            this._statsGrid.DefaultCol.ColHdrStyle = this.TrafodionIGrid1DefaultColHdrStyle;
            this._statsGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._statsGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._statsGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._statsGrid.Header.Height = 20;
            this._statsGrid.HelpTopic = global::Trafodion.Manager.Properties.Resources.NCICustomPrompt;
            this._statsGrid.Location = new System.Drawing.Point(0, 0);
            this._statsGrid.Name = "_statsGrid";
            this._statsGrid.ReadOnly = true;
            this._statsGrid.RowMode = true;
            this._statsGrid.RowTextCol.CellStyle = this.TrafodionIGrid1RowTextColCellStyle;
            this._statsGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this._statsGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this._statsGrid.SearchAsType.SearchCol = null;
            this._statsGrid.Size = new System.Drawing.Size(961, 64);
            this._statsGrid.TabIndex = 0;
            this._statsGrid.TreeCol = null;
            this._statsGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this._statsGrid.WordWrap = false;
            // 
            // _theToolTip
            // 
            this._theToolTip.IsBalloon = true;
            // 
            // MonitorWorkloadQueryStats
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionPanel1);
            this.Controls.Add(this._statsGridPanel);
            this.Controls.Add(this.buttonsPanel);
            this.Name = "MonitorWorkloadQueryStats";
            this.Size = new System.Drawing.Size(961, 631);
            this._theQueryTypePanel.ResumeLayout(false);
            this._theQueryTypePanel.PerformLayout();
            this.buttonsPanel.ResumeLayout(false);
            this.panel3.ResumeLayout(false);
            this.TrafodionPanel1.ResumeLayout(false);
            this._statsGridPanel.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this._statsGrid)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theQueryTypePanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel buttonsPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton resetStatsButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton refreshButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel3;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox totalWaitCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox totalExecCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox totalRejectCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox totalSuspendCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox totalHoldCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox totalCompleteCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox totalCancelCheckBox;
        private WorkloadQueryStatsChart _theChart;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton helpButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _statsGridPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionIGrid _statsGrid;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle TrafodionIGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1RowTextColCellStyle;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip _theToolTip;
    }
}