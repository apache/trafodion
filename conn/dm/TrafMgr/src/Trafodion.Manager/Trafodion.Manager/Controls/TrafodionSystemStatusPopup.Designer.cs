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
﻿
namespace Trafodion.Manager.Framework.Controls
{
    partial class SystemMonitorStatusLightPopup
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
            TenTec.Windows.iGridLib.iGColPattern iGColPattern1 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern2 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern3 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern4 = new TenTec.Windows.iGridLib.iGColPattern();
            this.TrafodionIGrid1DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionIGrid1DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.TrafodionIGrid1RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionIGrid1Col0CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionIGrid1Col0ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.TrafodionIGrid1Col1CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionIGrid1Col1ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.TrafodionIGrid1Col2CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionIGrid1Col2ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.TrafodionIGrid1Col3CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionIGrid1Col3ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.TrafodionIGrid1 = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0AT" +
                    "QB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAI" +
                    "ABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            ((System.ComponentModel.ISupportInitialize)(this.TrafodionIGrid1)).BeginInit();
            this.SuspendLayout();
            // 
            // TrafodionIGrid1Col0CellStyle
            // 
            this.TrafodionIGrid1Col0CellStyle.ReadOnly = TenTec.Windows.iGridLib.iGBool.True;
            // 
            // TrafodionIGrid1Col1CellStyle
            // 
            this.TrafodionIGrid1Col1CellStyle.ReadOnly = TenTec.Windows.iGridLib.iGBool.True;
            // 
            // TrafodionIGrid1Col2CellStyle
            // 
            this.TrafodionIGrid1Col2CellStyle.ReadOnly = TenTec.Windows.iGridLib.iGBool.True;
            // 
            // TrafodionIGrid1Col3CellStyle
            // 
            this.TrafodionIGrid1Col3CellStyle.ReadOnly = TenTec.Windows.iGridLib.iGBool.True;
            // 
            // TrafodionIGrid1
            // 
            this.TrafodionIGrid1.AutoResizeCols = true;
            this.TrafodionIGrid1.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            iGColPattern1.CellStyle = this.TrafodionIGrid1Col0CellStyle;
            iGColPattern1.ColHdrStyle = this.TrafodionIGrid1Col0ColHdrStyle;
            iGColPattern1.Key = "DateTime";
            iGColPattern1.Text = "Date/Time";
            iGColPattern1.Width = 56;
            iGColPattern2.CellStyle = this.TrafodionIGrid1Col1CellStyle;
            iGColPattern2.ColHdrStyle = this.TrafodionIGrid1Col1ColHdrStyle;
            iGColPattern2.Key = "SegNum";
            iGColPattern2.Text = "Segment Number";
            iGColPattern2.Width = 41;
            iGColPattern3.CellStyle = this.TrafodionIGrid1Col2CellStyle;
            iGColPattern3.ColHdrStyle = this.TrafodionIGrid1Col2ColHdrStyle;
            iGColPattern3.Key = "Entity";
            iGColPattern3.Text = "Entity";
            iGColPattern3.Width = 50;
            iGColPattern4.CellStyle = this.TrafodionIGrid1Col3CellStyle;
            iGColPattern4.ColHdrStyle = this.TrafodionIGrid1Col3ColHdrStyle;
            iGColPattern4.Key = "Details";
            iGColPattern4.Text = "Details";
            iGColPattern4.Width = 258;
            this.TrafodionIGrid1.Cols.AddRange(new TenTec.Windows.iGridLib.iGColPattern[] {
            iGColPattern1,
            iGColPattern2,
            iGColPattern3,
            iGColPattern4});
            this.TrafodionIGrid1.CurrentFilter = null;
            this.TrafodionIGrid1.DefaultCol.CellStyle = this.TrafodionIGrid1DefaultCellStyle;
            this.TrafodionIGrid1.DefaultCol.ColHdrStyle = this.TrafodionIGrid1DefaultColHdrStyle;
            this.TrafodionIGrid1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionIGrid1.ForeColor = System.Drawing.SystemColors.WindowText;
            this.TrafodionIGrid1.Header.Height = 19;
            this.TrafodionIGrid1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionIGrid1.Name = "TrafodionIGrid1";
            this.TrafodionIGrid1.ReadOnly = true;
            this.TrafodionIGrid1.RowMode = true;
            this.TrafodionIGrid1.RowTextCol.CellStyle = this.TrafodionIGrid1RowTextColCellStyle;
            this.TrafodionIGrid1.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this.TrafodionIGrid1.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this.TrafodionIGrid1.SearchAsType.SearchCol = null;
            this.TrafodionIGrid1.Size = new System.Drawing.Size(409, 108);
            this.TrafodionIGrid1.TabIndex = 0;
            this.TrafodionIGrid1.TreeCol = null;
            this.TrafodionIGrid1.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            // 
            // SystemMonitorStatusLightPopup
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(409, 108);
            this.Controls.Add(this.TrafodionIGrid1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.SizableToolWindow;
            this.Name = "SystemMonitorStatusLightPopup";
            this.ShowInTaskbar = false;
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Show;
            this.Text = "SystemMonitorStatusLightPopup";
            ((System.ComponentModel.ISupportInitialize)(this.TrafodionIGrid1)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionIGrid TrafodionIGrid1;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle TrafodionIGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1RowTextColCellStyle;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1Col0CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle TrafodionIGrid1Col0ColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1Col1CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle TrafodionIGrid1Col1ColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1Col2CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle TrafodionIGrid1Col2ColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1Col3CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle TrafodionIGrid1Col3ColHdrStyle;
    }
}