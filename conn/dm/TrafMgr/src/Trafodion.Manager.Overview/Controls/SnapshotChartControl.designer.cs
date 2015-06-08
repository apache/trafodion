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
    partial class SnapshotChartControl
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
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea2 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Legend legend2 = new System.Windows.Forms.DataVisualization.Charting.Legend();
            System.Windows.Forms.DataVisualization.Charting.Series series2 = new System.Windows.Forms.DataVisualization.Charting.Series();
            this._theChart = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this._gridPanel = new System.Windows.Forms.Panel();
            ((System.ComponentModel.ISupportInitialize)(this._theChart)).BeginInit();
            this.SuspendLayout();
            // 
            // _theChart
            // 
            this._theChart.BorderSkin.BackColor = System.Drawing.Color.Empty;
            this._theChart.BorderSkin.BorderWidth = 0;
            chartArea2.Name = "ChartArea1";
            this._theChart.ChartAreas.Add(chartArea2);
            this._theChart.Dock = System.Windows.Forms.DockStyle.Top;
            legend2.DockedToChartArea = "ChartArea1";
            legend2.Name = "Legend1";
            this._theChart.Legends.Add(legend2);
            this._theChart.Location = new System.Drawing.Point(0, 0);
            this._theChart.Name = "_theChart";
            series2.ChartArea = "ChartArea1";
            series2.Color = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            series2.Legend = "Legend1";
            series2.Name = "Series1";
            series2.YValuesPerPoint = 2;
            this._theChart.Series.Add(series2);
            this._theChart.Size = new System.Drawing.Size(608, 311);
            this._theChart.TabIndex = 1;
            this._theChart.Text = "chart1";
            // 
            // _gridPanel
            // 
            this._gridPanel.AutoScroll = true;
            this._gridPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._gridPanel.Location = new System.Drawing.Point(0, 311);
            this._gridPanel.Name = "_gridPanel";
            this._gridPanel.Size = new System.Drawing.Size(608, 187);
            this._gridPanel.TabIndex = 2;
            this._gridPanel.Visible = false;
            // 
            // SnapshotChartControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._gridPanel);
            this.Controls.Add(this._theChart);
            this.Name = "SnapshotChartControl";
            this.Size = new System.Drawing.Size(608, 498);
            ((System.ComponentModel.ISupportInitialize)(this._theChart)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.DataVisualization.Charting.Chart _theChart;
        private System.Windows.Forms.Panel _gridPanel;
    }
}
