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
    partial class WMSPlatformCounterHistory
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
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea1 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea2 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WMSPlatformCounterHistory));
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea3 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            this._avgNodeBusyChart = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this._avgMemUsageChart = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this._nodeBusySplitContainer = new System.Windows.Forms.SplitContainer();
            this._nodeBusyVerticalProgressBar = new Trafodion.Manager.Framework.Controls.TrafodionVerticalProgressBar();
            this._nodeBusyTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._memUsageSplitContainer = new System.Windows.Forms.SplitContainer();
            this._memUsageVerticalProgressBar = new Trafodion.Manager.Framework.Controls.TrafodionVerticalProgressBar();
            this._memUsageTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionIGrid1DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle();
            this.TrafodionIGrid1DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle();
            this.TrafodionIGrid1RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle();
            this._dataGridPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._countersGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this._execQueriesSplitContainer = new System.Windows.Forms.SplitContainer();
            this._execQueriesTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._curExecQueriesChart = new System.Windows.Forms.DataVisualization.Charting.Chart();
            ((System.ComponentModel.ISupportInitialize)(this._avgNodeBusyChart)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._avgMemUsageChart)).BeginInit();
            this._nodeBusySplitContainer.Panel1.SuspendLayout();
            this._nodeBusySplitContainer.Panel2.SuspendLayout();
            this._nodeBusySplitContainer.SuspendLayout();
            this._memUsageSplitContainer.Panel1.SuspendLayout();
            this._memUsageSplitContainer.Panel2.SuspendLayout();
            this._memUsageSplitContainer.SuspendLayout();
            this._dataGridPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._countersGrid)).BeginInit();
            this._execQueriesSplitContainer.Panel2.SuspendLayout();
            this._execQueriesSplitContainer.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._curExecQueriesChart)).BeginInit();
            this.SuspendLayout();
            // 
            // _avgNodeBusyChart
            // 
            this._avgNodeBusyChart.BorderSkin.BackColor = System.Drawing.Color.Transparent;
            this._avgNodeBusyChart.BorderSkin.BorderDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Solid;
            chartArea1.Name = "ChartArea1";
            this._avgNodeBusyChart.ChartAreas.Add(chartArea1);
            this._avgNodeBusyChart.Location = new System.Drawing.Point(3, 3);
            this._avgNodeBusyChart.Name = "_avgNodeBusyChart";
            this._avgNodeBusyChart.Size = new System.Drawing.Size(300, 157);
            this._avgNodeBusyChart.TabIndex = 2;
            this._avgNodeBusyChart.Text = "AvgNodeBusyChart";
            // 
            // _avgMemUsageChart
            // 
            chartArea2.Name = "ChartArea1";
            this._avgMemUsageChart.ChartAreas.Add(chartArea2);
            this._avgMemUsageChart.Location = new System.Drawing.Point(3, 166);
            this._avgMemUsageChart.Name = "_avgMemUsageChart";
            this._avgMemUsageChart.Size = new System.Drawing.Size(300, 157);
            this._avgMemUsageChart.TabIndex = 2;
            this._avgMemUsageChart.Text = "AvgMemUsageChart";
            // 
            // _nodeBusySplitContainer
            // 
            this._nodeBusySplitContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this._nodeBusySplitContainer.IsSplitterFixed = true;
            this._nodeBusySplitContainer.Location = new System.Drawing.Point(324, 3);
            this._nodeBusySplitContainer.Name = "_nodeBusySplitContainer";
            // 
            // _nodeBusySplitContainer.Panel1
            // 
            this._nodeBusySplitContainer.Panel1.Controls.Add(this._nodeBusyVerticalProgressBar);
            // 
            // _nodeBusySplitContainer.Panel2
            // 
            this._nodeBusySplitContainer.Panel2.Controls.Add(this._nodeBusyTextBox);
            this._nodeBusySplitContainer.Size = new System.Drawing.Size(109, 113);
            this._nodeBusySplitContainer.SplitterDistance = 25;
            this._nodeBusySplitContainer.SplitterWidth = 1;
            this._nodeBusySplitContainer.TabIndex = 3;
            // 
            // _nodeBusyVerticalProgressBar
            // 
            this._nodeBusyVerticalProgressBar.Dock = System.Windows.Forms.DockStyle.Fill;
            this._nodeBusyVerticalProgressBar.Location = new System.Drawing.Point(0, 0);
            this._nodeBusyVerticalProgressBar.Name = "_nodeBusyVerticalProgressBar";
            this._nodeBusyVerticalProgressBar.Size = new System.Drawing.Size(25, 113);
            this._nodeBusyVerticalProgressBar.Step = 5;
            this._nodeBusyVerticalProgressBar.TabIndex = 0;
            this._nodeBusyVerticalProgressBar.Value = 20;
            // 
            // _nodeBusyTextBox
            // 
            this._nodeBusyTextBox.BackColor = System.Drawing.Color.Black;
            this._nodeBusyTextBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._nodeBusyTextBox.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._nodeBusyTextBox.Font = new System.Drawing.Font("Tahoma", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._nodeBusyTextBox.ForeColor = System.Drawing.Color.ForestGreen;
            this._nodeBusyTextBox.Location = new System.Drawing.Point(0, 93);
            this._nodeBusyTextBox.Name = "_nodeBusyTextBox";
            this._nodeBusyTextBox.ReadOnly = true;
            this._nodeBusyTextBox.Size = new System.Drawing.Size(83, 20);
            this._nodeBusyTextBox.TabIndex = 0;
            this._nodeBusyTextBox.Text = "0%";
            this._nodeBusyTextBox.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            // 
            // _memUsageSplitContainer
            // 
            this._memUsageSplitContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this._memUsageSplitContainer.IsSplitterFixed = true;
            this._memUsageSplitContainer.Location = new System.Drawing.Point(462, 3);
            this._memUsageSplitContainer.Name = "_memUsageSplitContainer";
            // 
            // _memUsageSplitContainer.Panel1
            // 
            this._memUsageSplitContainer.Panel1.Controls.Add(this._memUsageVerticalProgressBar);
            // 
            // _memUsageSplitContainer.Panel2
            // 
            this._memUsageSplitContainer.Panel2.Controls.Add(this._memUsageTextBox);
            this._memUsageSplitContainer.Size = new System.Drawing.Size(109, 113);
            this._memUsageSplitContainer.SplitterDistance = 25;
            this._memUsageSplitContainer.SplitterWidth = 1;
            this._memUsageSplitContainer.TabIndex = 3;
            // 
            // _memUsageVerticalProgressBar
            // 
            this._memUsageVerticalProgressBar.Dock = System.Windows.Forms.DockStyle.Fill;
            this._memUsageVerticalProgressBar.Location = new System.Drawing.Point(0, 0);
            this._memUsageVerticalProgressBar.Name = "_memUsageVerticalProgressBar";
            this._memUsageVerticalProgressBar.Size = new System.Drawing.Size(25, 113);
            this._memUsageVerticalProgressBar.Step = 5;
            this._memUsageVerticalProgressBar.TabIndex = 0;
            this._memUsageVerticalProgressBar.Value = 20;
            // 
            // _memUsageTextBox
            // 
            this._memUsageTextBox.BackColor = System.Drawing.Color.Black;
            this._memUsageTextBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._memUsageTextBox.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._memUsageTextBox.Font = new System.Drawing.Font("Tahoma", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._memUsageTextBox.ForeColor = System.Drawing.Color.ForestGreen;
            this._memUsageTextBox.Location = new System.Drawing.Point(0, 93);
            this._memUsageTextBox.Name = "_memUsageTextBox";
            this._memUsageTextBox.ReadOnly = true;
            this._memUsageTextBox.Size = new System.Drawing.Size(83, 20);
            this._memUsageTextBox.TabIndex = 0;
            this._memUsageTextBox.Text = "0%";
            this._memUsageTextBox.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            // 
            // _dataGridPanel
            // 
            this._dataGridPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._dataGridPanel.Controls.Add(this._countersGrid);
            this._dataGridPanel.Location = new System.Drawing.Point(13, 479);
            this._dataGridPanel.Name = "_dataGridPanel";
            this._dataGridPanel.Size = new System.Drawing.Size(636, 113);
            this._dataGridPanel.TabIndex = 4;
            // 
            // _countersGrid
            // 
            this._countersGrid.AllowColumnFilter = true;
            this._countersGrid.AllowWordWrap = false;
            this._countersGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("_countersGrid.AlwaysHiddenColumnNames")));
            this._countersGrid.AutoResizeCols = true;
            this._countersGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this._countersGrid.CurrentFilter = null;
            this._countersGrid.DefaultCol.CellStyle = this.TrafodionIGrid1DefaultCellStyle;
            this._countersGrid.DefaultCol.ColHdrStyle = this.TrafodionIGrid1DefaultColHdrStyle;
            this._countersGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._countersGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._countersGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._countersGrid.Header.Height = 20;
            this._countersGrid.HelpTopic = "";
            this._countersGrid.Location = new System.Drawing.Point(0, 0);
            this._countersGrid.Name = "_countersGrid";
            this._countersGrid.ReadOnly = true;
            this._countersGrid.RowMode = true;
            this._countersGrid.RowTextCol.CellStyle = this.TrafodionIGrid1RowTextColCellStyle;
            this._countersGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this._countersGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this._countersGrid.SearchAsType.SearchCol = null;
            this._countersGrid.Size = new System.Drawing.Size(636, 113);
            this._countersGrid.TabIndex = 0;
            this._countersGrid.TreeCol = null;
            this._countersGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this._countersGrid.WordWrap = false;
            // 
            // _execQueriesSplitContainer
            // 
            this._execQueriesSplitContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel2;
            this._execQueriesSplitContainer.IsSplitterFixed = true;
            this._execQueriesSplitContainer.Location = new System.Drawing.Point(324, 155);
            this._execQueriesSplitContainer.Name = "_execQueriesSplitContainer";
            this._execQueriesSplitContainer.Panel1Collapsed = true;
            this._execQueriesSplitContainer.Panel1MinSize = 0;
            // 
            // _execQueriesSplitContainer.Panel2
            // 
            this._execQueriesSplitContainer.Panel2.Controls.Add(this._execQueriesTextBox);
            this._execQueriesSplitContainer.Size = new System.Drawing.Size(109, 113);
            this._execQueriesSplitContainer.SplitterDistance = 25;
            this._execQueriesSplitContainer.SplitterWidth = 1;
            this._execQueriesSplitContainer.TabIndex = 7;
            // 
            // _execQueriesTextBox
            // 
            this._execQueriesTextBox.BackColor = System.Drawing.Color.Black;
            this._execQueriesTextBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._execQueriesTextBox.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._execQueriesTextBox.Font = new System.Drawing.Font("Tahoma", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._execQueriesTextBox.ForeColor = System.Drawing.Color.ForestGreen;
            this._execQueriesTextBox.Location = new System.Drawing.Point(0, 93);
            this._execQueriesTextBox.Name = "_execQueriesTextBox";
            this._execQueriesTextBox.ReadOnly = true;
            this._execQueriesTextBox.Size = new System.Drawing.Size(109, 20);
            this._execQueriesTextBox.TabIndex = 0;
            this._execQueriesTextBox.Text = "0%";
            this._execQueriesTextBox.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            // 
            // _curExecQueriesChart
            // 
            chartArea3.Name = "ChartArea1";
            this._curExecQueriesChart.ChartAreas.Add(chartArea3);
            this._curExecQueriesChart.Location = new System.Drawing.Point(462, 146);
            this._curExecQueriesChart.Name = "_curExecQueriesChart";
            this._curExecQueriesChart.Size = new System.Drawing.Size(300, 157);
            this._curExecQueriesChart.TabIndex = 8;
            this._curExecQueriesChart.Text = "CurExecQueriesChart";
            // 
            // WMSPlatformCounterHistory
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._curExecQueriesChart);
            this.Controls.Add(this._execQueriesSplitContainer);
            this.Controls.Add(this._dataGridPanel);
            this.Controls.Add(this._memUsageSplitContainer);
            this.Controls.Add(this._nodeBusySplitContainer);
            this.Controls.Add(this._avgMemUsageChart);
            this.Controls.Add(this._avgNodeBusyChart);
            this.Name = "WMSPlatformCounterHistory";
            this.Size = new System.Drawing.Size(781, 604);
            this.Load += new System.EventHandler(this.WMSPlatformCounterHistory_Load);
            ((System.ComponentModel.ISupportInitialize)(this._avgNodeBusyChart)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._avgMemUsageChart)).EndInit();
            this._nodeBusySplitContainer.Panel1.ResumeLayout(false);
            this._nodeBusySplitContainer.Panel2.ResumeLayout(false);
            this._nodeBusySplitContainer.Panel2.PerformLayout();
            this._nodeBusySplitContainer.ResumeLayout(false);
            this._memUsageSplitContainer.Panel1.ResumeLayout(false);
            this._memUsageSplitContainer.Panel2.ResumeLayout(false);
            this._memUsageSplitContainer.Panel2.PerformLayout();
            this._memUsageSplitContainer.ResumeLayout(false);
            this._dataGridPanel.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this._countersGrid)).EndInit();
            this._execQueriesSplitContainer.Panel2.ResumeLayout(false);
            this._execQueriesSplitContainer.Panel2.PerformLayout();
            this._execQueriesSplitContainer.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this._curExecQueriesChart)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.DataVisualization.Charting.Chart _avgNodeBusyChart;
        private System.Windows.Forms.DataVisualization.Charting.Chart _avgMemUsageChart;
        private System.Windows.Forms.SplitContainer _nodeBusySplitContainer;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _nodeBusyTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionVerticalProgressBar _nodeBusyVerticalProgressBar;
        private System.Windows.Forms.SplitContainer _memUsageSplitContainer;
        private Trafodion.Manager.Framework.Controls.TrafodionVerticalProgressBar _memUsageVerticalProgressBar;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _memUsageTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _dataGridPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionIGrid _countersGrid;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle TrafodionIGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1RowTextColCellStyle;
        private System.Windows.Forms.SplitContainer _execQueriesSplitContainer;
        private Framework.Controls.TrafodionTextBox _execQueriesTextBox;
        private System.Windows.Forms.DataVisualization.Charting.Chart _curExecQueriesChart;

    }
}
