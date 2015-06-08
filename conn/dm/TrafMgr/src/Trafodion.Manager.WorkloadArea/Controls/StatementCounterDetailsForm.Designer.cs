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
    partial class StatementCounterDetailsForm
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(StatementCounterDetailsForm));
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea1 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Legend legend1 = new System.Windows.Forms.DataVisualization.Charting.Legend();
            System.Windows.Forms.DataVisualization.Charting.Series series1 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.Series series2 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.Series series3 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.Series series4 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.Series series5 = new System.Windows.Forms.DataVisualization.Charting.Series();
            this.statementCounterDetailsIGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0AT" +
                    "QB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAI" +
                    "ABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            this.TrafodionIGrid1DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionIGrid1DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.TrafodionIGrid1RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.statementCounterDetailsChart = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this._statementGridPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            ((System.ComponentModel.ISupportInitialize)(this.statementCounterDetailsIGrid)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.statementCounterDetailsChart)).BeginInit();
            this._statementGridPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // statementCounterDetailsIGrid
            // 
            this.statementCounterDetailsIGrid.AllowColumnFilter = true;
            this.statementCounterDetailsIGrid.AllowWordWrap = false;
            this.statementCounterDetailsIGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("statementCounterDetailsIGrid.AlwaysHiddenColumnNames")));
            this.statementCounterDetailsIGrid.AutoResizeCols = true;
            this.statementCounterDetailsIGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this.statementCounterDetailsIGrid.CurrentFilter = null;
            this.statementCounterDetailsIGrid.DefaultCol.CellStyle = this.TrafodionIGrid1DefaultCellStyle;
            this.statementCounterDetailsIGrid.DefaultCol.ColHdrStyle = this.TrafodionIGrid1DefaultColHdrStyle;
            this.statementCounterDetailsIGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.statementCounterDetailsIGrid.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.statementCounterDetailsIGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this.statementCounterDetailsIGrid.Header.Height = 20;
            this.statementCounterDetailsIGrid.HelpTopic = "";
            this.statementCounterDetailsIGrid.Location = new System.Drawing.Point(0, 0);
            this.statementCounterDetailsIGrid.Name = "statementCounterDetailsIGrid";
            this.statementCounterDetailsIGrid.ReadOnly = true;
            this.statementCounterDetailsIGrid.RowMode = true;
            this.statementCounterDetailsIGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this.statementCounterDetailsIGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this.statementCounterDetailsIGrid.SearchAsType.SearchCol = null;
            this.statementCounterDetailsIGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            this.statementCounterDetailsIGrid.Size = new System.Drawing.Size(1005, 198);
            this.statementCounterDetailsIGrid.StaySorted = true;
            this.statementCounterDetailsIGrid.TabIndex = 0;
            this.statementCounterDetailsIGrid.TreeCol = null;
            this.statementCounterDetailsIGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.statementCounterDetailsIGrid.WordWrap = false;
            // 
            // statementCounterDetailsChart
            // 
            this.statementCounterDetailsChart.BackColor = System.Drawing.Color.WhiteSmoke;
            this.statementCounterDetailsChart.BorderlineColor = System.Drawing.Color.Black;
            this.statementCounterDetailsChart.BorderlineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Solid;
            this.statementCounterDetailsChart.BorderSkin.SkinStyle = System.Windows.Forms.DataVisualization.Charting.BorderSkinStyle.Emboss;
            chartArea1.AxisX.IsMarginVisible = false;
            chartArea1.AxisX.MajorGrid.Enabled = false;
            chartArea1.AxisX.MajorTickMark.TickMarkStyle = System.Windows.Forms.DataVisualization.Charting.TickMarkStyle.AcrossAxis;
            chartArea1.AxisX.ScrollBar.BackColor = System.Drawing.SystemColors.Control;
            chartArea1.AxisX.ScrollBar.ButtonColor = System.Drawing.SystemColors.ScrollBar;
            chartArea1.AxisX.Title = "Time";
            chartArea1.AxisY.MajorGrid.Enabled = false;
            chartArea1.AxisY.MajorTickMark.TickMarkStyle = System.Windows.Forms.DataVisualization.Charting.TickMarkStyle.AcrossAxis;
            chartArea1.AxisY.Title = "Value";
            chartArea1.BackColor = System.Drawing.Color.LightSteelBlue;
            chartArea1.BackGradientStyle = System.Windows.Forms.DataVisualization.Charting.GradientStyle.DiagonalRight;
            chartArea1.BorderDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Solid;
            chartArea1.CursorX.IsUserEnabled = true;
            chartArea1.CursorX.IsUserSelectionEnabled = true;
            chartArea1.IsSameFontSizeForAllAxes = true;
            chartArea1.Name = "Default";
            chartArea1.ShadowColor = System.Drawing.Color.Transparent;
            this.statementCounterDetailsChart.ChartAreas.Add(chartArea1);
            legend1.Alignment = System.Drawing.StringAlignment.Center;
            legend1.BackColor = System.Drawing.Color.Transparent;
            legend1.BorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(80)))), ((int)(((byte)(99)))), ((int)(((byte)(129)))));
            legend1.Docking = System.Windows.Forms.DataVisualization.Charting.Docking.Top;
            legend1.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            legend1.IsTextAutoFit = false;
            legend1.Name = "Default";
            this.statementCounterDetailsChart.Legends.Add(legend1);
            this.statementCounterDetailsChart.Location = new System.Drawing.Point(12, 21);
            this.statementCounterDetailsChart.Name = "statementCounterDetailsChart";
            series1.BorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(180)))), ((int)(((byte)(26)))), ((int)(((byte)(59)))), ((int)(((byte)(105)))));
            series1.ChartArea = "Default";
            series1.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Line;
            series1.Color = System.Drawing.Color.Green;
            series1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            series1.Legend = "Default";
            series1.MarkerSize = 9;
            series1.MarkerStyle = System.Windows.Forms.DataVisualization.Charting.MarkerStyle.Circle;
            series1.Name = "Series1";
            series1.ShadowColor = System.Drawing.Color.Black;
            series1.ShadowOffset = 1;
            series1.XValueType = System.Windows.Forms.DataVisualization.Charting.ChartValueType.DateTime;
            series1.YValueType = System.Windows.Forms.DataVisualization.Charting.ChartValueType.Int32;
            series2.BorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(180)))), ((int)(((byte)(26)))), ((int)(((byte)(59)))), ((int)(((byte)(105)))));
            series2.ChartArea = "Default";
            series2.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Line;
            series2.Color = System.Drawing.Color.Navy;
            series2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            series2.Legend = "Default";
            series2.MarkerSize = 9;
            series2.MarkerStyle = System.Windows.Forms.DataVisualization.Charting.MarkerStyle.Diamond;
            series2.Name = "Series2";
            series2.ShadowColor = System.Drawing.Color.Black;
            series2.ShadowOffset = 1;
            series2.XValueType = System.Windows.Forms.DataVisualization.Charting.ChartValueType.DateTime;
            series2.YValueType = System.Windows.Forms.DataVisualization.Charting.ChartValueType.Int32;
            series3.BorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(180)))), ((int)(((byte)(26)))), ((int)(((byte)(59)))), ((int)(((byte)(105)))));
            series3.ChartArea = "Default";
            series3.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Line;
            series3.Color = System.Drawing.Color.DarkOrange;
            series3.Font = new System.Drawing.Font("Tahoma", 8.25F);
            series3.Legend = "Default";
            series3.MarkerSize = 9;
            series3.MarkerStyle = System.Windows.Forms.DataVisualization.Charting.MarkerStyle.Square;
            series3.Name = "Series3";
            series3.ShadowColor = System.Drawing.Color.Black;
            series3.ShadowOffset = 1;
            series3.XValueType = System.Windows.Forms.DataVisualization.Charting.ChartValueType.DateTime;
            series3.YValueType = System.Windows.Forms.DataVisualization.Charting.ChartValueType.Int32;
            series4.BorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(180)))), ((int)(((byte)(26)))), ((int)(((byte)(59)))), ((int)(((byte)(105)))));
            series4.ChartArea = "Default";
            series4.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Line;
            series4.Color = System.Drawing.Color.DarkGray;
            series4.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            series4.Legend = "Default";
            series4.MarkerSize = 9;
            series4.MarkerStyle = System.Windows.Forms.DataVisualization.Charting.MarkerStyle.Triangle;
            series4.Name = "Series4";
            series4.ShadowColor = System.Drawing.Color.Black;
            series4.ShadowOffset = 1;
            series4.XValueType = System.Windows.Forms.DataVisualization.Charting.ChartValueType.DateTime;
            series4.YValueType = System.Windows.Forms.DataVisualization.Charting.ChartValueType.Int32;
            series5.ChartArea = "Default";
            series5.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Line;
            series5.Color = System.Drawing.Color.Red;
            series5.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            series5.Legend = "Default";
            series5.MarkerSize = 9;
            series5.MarkerStyle = System.Windows.Forms.DataVisualization.Charting.MarkerStyle.Cross;
            series5.Name = "Series5";
            series5.ShadowColor = System.Drawing.Color.Black;
            series5.ShadowOffset = 1;
            series5.XValueType = System.Windows.Forms.DataVisualization.Charting.ChartValueType.DateTime;
            series5.YValueType = System.Windows.Forms.DataVisualization.Charting.ChartValueType.Int32;
            this.statementCounterDetailsChart.Series.Add(series1);
            this.statementCounterDetailsChart.Series.Add(series2);
            this.statementCounterDetailsChart.Series.Add(series3);
            this.statementCounterDetailsChart.Series.Add(series4);
            this.statementCounterDetailsChart.Series.Add(series5);
            this.statementCounterDetailsChart.Size = new System.Drawing.Size(1005, 260);
            this.statementCounterDetailsChart.TabIndex = 9;
            // 
            // _statementGridPanel
            // 
            this._statementGridPanel.AutoScroll = true;
            this._statementGridPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._statementGridPanel.Controls.Add(this.statementCounterDetailsIGrid);
            this._statementGridPanel.Location = new System.Drawing.Point(12, 300);
            this._statementGridPanel.Name = "_statementGridPanel";
            this._statementGridPanel.Size = new System.Drawing.Size(1005, 198);
            this._statementGridPanel.TabIndex = 10;
            // 
            // StatementCounterDetailsForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._statementGridPanel);
            this.Controls.Add(this.statementCounterDetailsChart);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "StatementCounterDetailsForm";
            this.Size = new System.Drawing.Size(1080, 523);
            this.Load += new System.EventHandler(this.StatementCounterDetailsForm_Load);
            ((System.ComponentModel.ISupportInitialize)(this.statementCounterDetailsIGrid)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.statementCounterDetailsChart)).EndInit();
            this._statementGridPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionIGrid statementCounterDetailsIGrid;
        private System.Windows.Forms.DataVisualization.Charting.Chart statementCounterDetailsChart;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle TrafodionIGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1RowTextColCellStyle;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _statementGridPanel;
    }
}