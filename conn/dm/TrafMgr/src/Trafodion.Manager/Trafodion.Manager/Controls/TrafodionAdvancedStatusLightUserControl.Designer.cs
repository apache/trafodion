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
﻿namespace Trafodion.Manager.Framework.Controls
{
    partial class TrafodionAdvancedStatusLightUserControl
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
            TenTec.Windows.iGridLib.iGColPattern iGColPattern1 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern2 = new TenTec.Windows.iGridLib.iGColPattern();
            this.TrafodionIGrid1Col0CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionIGrid1Col0ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.TrafodionIGrid1Col1CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionIGrid1Col1ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.TrafodionIGrid1DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionIGrid1DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.TrafodionIGrid1RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.groupBox_TrafodionGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.tableLayoutPanel2 = new System.Windows.Forms.TableLayoutPanel();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.detailsIG_TrafodionIGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0AT" +
                    "QB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAI" +
                    "ABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            this.panel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.statusLight_pictureBox = new Trafodion.Manager.Framework.Controls.TrafodionPictureBox();
            this.CSelectedIndicator = new Trafodion.Manager.Framework.Controls.TrafodionTabGraphic();
            this.groupBox_TrafodionGroupBox.SuspendLayout();
            this.tableLayoutPanel2.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.detailsIG_TrafodionIGrid)).BeginInit();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.statusLight_pictureBox)).BeginInit();
            this.SuspendLayout();
            // 
            // TrafodionIGrid1Col1CellStyle
            // 
            this.TrafodionIGrid1Col1CellStyle.Selectable = TenTec.Windows.iGridLib.iGBool.False;
            this.TrafodionIGrid1Col1CellStyle.TextAlign = TenTec.Windows.iGridLib.iGContentAlignment.MiddleCenter;
            // 
            // TrafodionIGrid1Col1ColHdrStyle
            // 
            this.TrafodionIGrid1Col1ColHdrStyle.TextAlign = TenTec.Windows.iGridLib.iGContentAlignment.TopCenter;
            // 
            // groupBox_TrafodionGroupBox
            // 
            this.groupBox_TrafodionGroupBox.BackColor = System.Drawing.SystemColors.Control;
            this.groupBox_TrafodionGroupBox.Controls.Add(this.tableLayoutPanel2);
            this.groupBox_TrafodionGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox_TrafodionGroupBox.Location = new System.Drawing.Point(0, 0);
            this.groupBox_TrafodionGroupBox.Name = "groupBox_TrafodionGroupBox";
            this.groupBox_TrafodionGroupBox.Padding = new System.Windows.Forms.Padding(7);
            this.groupBox_TrafodionGroupBox.Size = new System.Drawing.Size(227, 309);
            this.groupBox_TrafodionGroupBox.TabIndex = 2;
            this.groupBox_TrafodionGroupBox.TabStop = false;
            this.groupBox_TrafodionGroupBox.Text = "DefaultTitle";
            // 
            // tableLayoutPanel2
            // 
            this.tableLayoutPanel2.BackColor = System.Drawing.SystemColors.Control;
            this.tableLayoutPanel2.ColumnCount = 1;
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel2.Controls.Add(this.TrafodionPanel1, 0, 1);
            this.tableLayoutPanel2.Controls.Add(this.panel1, 0, 0);
            this.tableLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel2.Location = new System.Drawing.Point(7, 20);
            this.tableLayoutPanel2.Name = "tableLayoutPanel2";
            this.tableLayoutPanel2.RowCount = 2;
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 74F));
            this.tableLayoutPanel2.Size = new System.Drawing.Size(213, 282);
            this.tableLayoutPanel2.TabIndex = 16;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.Controls.Add(this.detailsIG_TrafodionIGrid);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(3, 211);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(207, 68);
            this.TrafodionPanel1.TabIndex = 0;
            // 
            // detailsIG_TrafodionIGrid
            // 
            this.detailsIG_TrafodionIGrid.Appearance = TenTec.Windows.iGridLib.iGControlPaintAppearance.StyleFlat;
            this.detailsIG_TrafodionIGrid.AutoResizeCols = true;
            this.detailsIG_TrafodionIGrid.AutoWidthColMode = TenTec.Windows.iGridLib.iGAutoWidthColMode.Cells;
            this.detailsIG_TrafodionIGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this.detailsIG_TrafodionIGrid.BorderStyle = TenTec.Windows.iGridLib.iGBorderStyle.Flat;
            iGColPattern1.AllowGrouping = false;
            iGColPattern1.AllowMoving = false;
            iGColPattern1.AllowSizing = false;
            iGColPattern1.CellStyle = this.TrafodionIGrid1Col0CellStyle;
            iGColPattern1.ColHdrStyle = this.TrafodionIGrid1Col0ColHdrStyle;
            iGColPattern1.IncludeInSelect = false;
            iGColPattern1.Key = "Name";
            iGColPattern1.Text = "Name";
            iGColPattern2.AllowGrouping = false;
            iGColPattern2.AllowMoving = false;
            iGColPattern2.AllowSizing = false;
            iGColPattern2.CellStyle = this.TrafodionIGrid1Col1CellStyle;
            iGColPattern2.ColHdrStyle = this.TrafodionIGrid1Col1ColHdrStyle;
            iGColPattern2.IncludeInSelect = false;
            iGColPattern2.Key = "Value";
            iGColPattern2.Text = "Value";
            iGColPattern2.Width = 20;
            this.detailsIG_TrafodionIGrid.Cols.AddRange(new TenTec.Windows.iGridLib.iGColPattern[] {
            iGColPattern1,
            iGColPattern2});
            this.detailsIG_TrafodionIGrid.CurrentFilter = null;
            this.detailsIG_TrafodionIGrid.DefaultCol.CellStyle = this.TrafodionIGrid1DefaultCellStyle;
            this.detailsIG_TrafodionIGrid.DefaultCol.ColHdrStyle = this.TrafodionIGrid1DefaultColHdrStyle;
            this.detailsIG_TrafodionIGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.detailsIG_TrafodionIGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this.detailsIG_TrafodionIGrid.GridLines.Mode = TenTec.Windows.iGridLib.iGGridLinesMode.None;
            this.detailsIG_TrafodionIGrid.Header.Height = 19;
            this.detailsIG_TrafodionIGrid.Header.Visible = false;
            this.detailsIG_TrafodionIGrid.HighlightSelCells = false;
            this.detailsIG_TrafodionIGrid.Location = new System.Drawing.Point(0, 0);
            this.detailsIG_TrafodionIGrid.Name = "detailsIG_TrafodionIGrid";
            this.detailsIG_TrafodionIGrid.ReadOnly = true;
            this.detailsIG_TrafodionIGrid.RowMode = true;
            this.detailsIG_TrafodionIGrid.RowTextCol.CellStyle = this.TrafodionIGrid1RowTextColCellStyle;
            this.detailsIG_TrafodionIGrid.Size = new System.Drawing.Size(207, 68);
            this.detailsIG_TrafodionIGrid.TabIndex = 0;
            this.detailsIG_TrafodionIGrid.TreeCol = null;
            this.detailsIG_TrafodionIGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.SystemColors.Control;
            this.panel1.Controls.Add(this.statusLight_pictureBox);
            this.panel1.Controls.Add(this.CSelectedIndicator);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel1.Location = new System.Drawing.Point(0, 3);
            this.panel1.Margin = new System.Windows.Forms.Padding(0, 3, 0, 0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(213, 205);
            this.panel1.TabIndex = 1;
            // 
            // statusLight_pictureBox
            // 
            this.statusLight_pictureBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.statusLight_pictureBox.BackColor = System.Drawing.SystemColors.Control;
            this.statusLight_pictureBox.Image = global::Trafodion.Manager.Properties.Resources.GrayIcon;
            this.statusLight_pictureBox.Location = new System.Drawing.Point(8, 16);
            this.statusLight_pictureBox.Name = "statusLight_pictureBox";
            this.statusLight_pictureBox.Size = new System.Drawing.Size(196, 186);
            this.statusLight_pictureBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
            this.statusLight_pictureBox.TabIndex = 2;
            this.statusLight_pictureBox.TabStop = false;
            this.statusLight_pictureBox.Click += new System.EventHandler(this.statusLight_pictureBox_Click);
            // 
            // CSelectedIndicator
            // 
            this.CSelectedIndicator.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.CSelectedIndicator.BackColor = System.Drawing.SystemColors.Control;
            this.CSelectedIndicator.Location = new System.Drawing.Point(0, 0);
            this.CSelectedIndicator.Name = "CSelectedIndicator";
            this.CSelectedIndicator.Size = new System.Drawing.Size(213, 205);
            this.CSelectedIndicator.TabIndex = 14;
            this.CSelectedIndicator.Visible = false;
            // 
            // TrafodionAdvancedStatusLightUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.groupBox_TrafodionGroupBox);
            this.Name = "TrafodionAdvancedStatusLightUserControl";
            this.Size = new System.Drawing.Size(227, 309);
            this.groupBox_TrafodionGroupBox.ResumeLayout(false);
            this.tableLayoutPanel2.ResumeLayout(false);
            this.TrafodionPanel1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.detailsIG_TrafodionIGrid)).EndInit();
            this.panel1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.statusLight_pictureBox)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPictureBox statusLight_pictureBox;
        private TrafodionTabGraphic CSelectedIndicator;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel2;
        private TrafodionGroupBox groupBox_TrafodionGroupBox;
        private TrafodionPanel TrafodionPanel1;
        private TrafodionIGrid detailsIG_TrafodionIGrid;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle TrafodionIGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1RowTextColCellStyle;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1Col0CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle TrafodionIGrid1Col0ColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1Col1CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle TrafodionIGrid1Col1ColHdrStyle;

    }
}
