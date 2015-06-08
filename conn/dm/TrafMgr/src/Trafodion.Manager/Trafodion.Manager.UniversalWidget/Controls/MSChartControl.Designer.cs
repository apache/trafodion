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
﻿namespace Trafodion.Manager.UniversalWidget.Controls
{
    partial class MSChartControl
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
            this._theMainPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theChart = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this._theMainPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theChart)).BeginInit();
            this.SuspendLayout();
            // 
            // _theMainPanel
            // 
            this._theMainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theMainPanel.Controls.Add(this._theChart);
            this._theMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMainPanel.Location = new System.Drawing.Point(0, 0);
            this._theMainPanel.Name = "_theMainPanel";
            this._theMainPanel.Size = new System.Drawing.Size(575, 287);
            this._theMainPanel.TabIndex = 0;
            // 
            // _theChart
            // 
            this._theChart.AllowDrop = true;
            this._theChart.BorderlineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.DashDotDot;
            this._theChart.BorderSkin.SkinStyle = System.Windows.Forms.DataVisualization.Charting.BorderSkinStyle.Emboss;
            this._theChart.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theChart.Location = new System.Drawing.Point(0, 0);
            this._theChart.Name = "_theChart";
            this._theChart.Size = new System.Drawing.Size(575, 287);
            this._theChart.TabIndex = 0;
            this._theChart.Text = "chart1";
            this._theChart.PrePaint += new System.EventHandler<System.Windows.Forms.DataVisualization.Charting.ChartPaintEventArgs>(this._theChart_PostPaint);
            this._theChart.Click += new System.EventHandler(this._theChart_Click);
            this._theChart.DragDrop += new System.Windows.Forms.DragEventHandler(this._theChart_DragDrop);
            this._theChart.DragEnter += new System.Windows.Forms.DragEventHandler(this._theChart_DragEnter);
            this._theChart.MouseClick += new System.Windows.Forms.MouseEventHandler(this._theChart_MouseClick);
            // 
            // MSChartControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theMainPanel);
            this.Name = "MSChartControl";
            this.Size = new System.Drawing.Size(575, 287);
            this._theMainPanel.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this._theChart)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theMainPanel;
        private System.Windows.Forms.DataVisualization.Charting.Chart _theChart;
    }
}
