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
    partial class ChartDataDisplayControl
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
            Dundas.Charting.WinControl.ChartArea chartArea1 = new Dundas.Charting.WinControl.ChartArea();
            Dundas.Charting.WinControl.Legend legend1 = new Dundas.Charting.WinControl.Legend();
            Dundas.Charting.WinControl.Series series1 = new Dundas.Charting.WinControl.Series();
            Dundas.Charting.WinControl.Series series2 = new Dundas.Charting.WinControl.Series();
            Dundas.Charting.WinControl.Title title1 = new Dundas.Charting.WinControl.Title();
            this._theChart = new Dundas.Charting.WinControl.Chart();
            ((System.ComponentModel.ISupportInitialize)(this._theChart)).BeginInit();
            this.SuspendLayout();
            // 
            // ChartDataDisplayControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            // 
            // _theChart
            // 
            this._theChart.AlwaysRecreateHotregions = true;
            this._theChart.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theChart.BackGradientEndColor = System.Drawing.Color.White;
            this._theChart.BackGradientType = Dundas.Charting.WinControl.GradientType.DiagonalLeft;
            this._theChart.BorderLineColor = System.Drawing.Color.FromArgb(((int)(((byte)(26)))), ((int)(((byte)(59)))), ((int)(((byte)(105)))));
            this._theChart.BorderLineStyle = Dundas.Charting.WinControl.ChartDashStyle.Solid;
            this._theChart.BorderSkin.FrameBackColor = System.Drawing.Color.CornflowerBlue;
            this._theChart.BorderSkin.FrameBackGradientEndColor = System.Drawing.Color.CornflowerBlue;
            this._theChart.BorderSkin.PageColor = System.Drawing.SystemColors.Control;
            this._theChart.BorderSkin.SkinStyle = Dundas.Charting.WinControl.BorderSkinStyle.Emboss;
            chartArea1.Area3DStyle.WallWidth = 0;
            chartArea1.AxisX.MajorGrid.LineColor = System.Drawing.Color.Silver;
            chartArea1.AxisX.MinorGrid.LineColor = System.Drawing.Color.Silver;
            chartArea1.AxisX.MinorTickMark.Size = 2F;
            chartArea1.AxisX2.MajorGrid.LineColor = System.Drawing.Color.Silver;
            chartArea1.AxisX2.MinorGrid.LineColor = System.Drawing.Color.Silver;
            chartArea1.AxisY.MajorGrid.LineColor = System.Drawing.Color.Silver;
            chartArea1.AxisY.MinorGrid.LineColor = System.Drawing.Color.Silver;
            chartArea1.AxisY.MinorTickMark.Size = 2F;
            chartArea1.AxisY2.MajorGrid.LineColor = System.Drawing.Color.Silver;
            chartArea1.AxisY2.MinorGrid.LineColor = System.Drawing.Color.Silver;
            chartArea1.BackColor = System.Drawing.Color.White;
            chartArea1.BorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(26)))), ((int)(((byte)(59)))), ((int)(((byte)(105)))));
            chartArea1.BorderStyle = Dundas.Charting.WinControl.ChartDashStyle.Solid;
            chartArea1.Name = "Default";
            chartArea1.ShadowOffset = 2;
            this._theChart.ChartAreas.Add(chartArea1);
            this._theChart.Dock = System.Windows.Forms.DockStyle.Fill;
            legend1.BackColor = System.Drawing.Color.White;
            legend1.BorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(26)))), ((int)(((byte)(59)))), ((int)(((byte)(105)))));
            legend1.DockToChartArea = "Default";
            legend1.Name = "Default";
            legend1.ShadowOffset = 2;
            this._theChart.Legends.Add(legend1);
            this._theChart.Location = new System.Drawing.Point(0, 0);
            this._theChart.Name = "_theChart";
            this._theChart.Palette = Dundas.Charting.WinControl.ChartColorPalette.Dundas;
            
            series1.BorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(26)))), ((int)(((byte)(59)))), ((int)(((byte)(105)))));
            series1.ChartType = "StackedColumn";
            series1.Name = "Series1";
            series1.PaletteCustomColors = new System.Drawing.Color[0];
            series1.ShadowOffset = 2;
            
            series2.BorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(26)))), ((int)(((byte)(59)))), ((int)(((byte)(105)))));
            series2.ChartType = "StackedColumn";
            series2.Name = "Series2";
            series2.PaletteCustomColors = new System.Drawing.Color[0];
            series2.ShadowOffset = 2;
            this._theChart.Series.Add(series1);
            this._theChart.Series.Add(series2);
            this._theChart.Size = new System.Drawing.Size(572, 369);
            this._theChart.TabIndex = 0;
            this._theChart.Text = "chart1";
            title1.Name = "Title1";
            title1.Text = "Chart";
            title1.Visible = false;
            this._theChart.Titles.Add(title1);
            this._theChart.UI.Toolbar.BorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(26)))), ((int)(((byte)(59)))), ((int)(((byte)(105)))));
            this._theChart.UI.Toolbar.BorderSkin.PageColor = System.Drawing.Color.Transparent;
            this._theChart.UI.Toolbar.BorderSkin.SkinStyle = Dundas.Charting.WinControl.BorderSkinStyle.Emboss;
            this.Controls.Add(this._theChart);
            this.Name = "ChartDataDisplayControl";
            this.Size = new System.Drawing.Size(572, 369);
            ((System.ComponentModel.ISupportInitialize)(this._theChart)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private Dundas.Charting.WinControl.Chart _theChart;

    }
}
