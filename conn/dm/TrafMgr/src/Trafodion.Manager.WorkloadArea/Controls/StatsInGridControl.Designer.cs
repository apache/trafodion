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
    partial class StatsInGridControl
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
            MyDispose(disposing);
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(StatsInGridControl));
            TenTec.Windows.iGridLib.iGColPattern iGColPattern3 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern4 = new TenTec.Windows.iGridLib.iGColPattern();
            this._theGridCol1CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this._theGridCol1ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this._theGridCol2CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this._theGridCol2ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this._theGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0AT" +
                    "QB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAI" +
                    "ABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            this.TrafodionIGrid1DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionIGrid1DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.TrafodionIGrid1RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this._theGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theGrid)).BeginInit();
            this.SuspendLayout();
            // 
            // _theGroupBox
            // 
            this._theGroupBox.AutoSize = true;
            this._theGroupBox.Controls.Add(this._theGrid);
            this._theGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theGroupBox.Location = new System.Drawing.Point(0, 0);
            this._theGroupBox.Name = "_theGroupBox";
            this._theGroupBox.Size = new System.Drawing.Size(545, 392);
            this._theGroupBox.TabIndex = 0;
            this._theGroupBox.TabStop = false;
            this._theGroupBox.Text = "Statistics";
            // 
            // _theGrid
            // 
            this._theGrid.AllowColumnFilter = true;
            this._theGrid.AllowWordWrap = false;
            this._theGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("_theGrid.AlwaysHiddenColumnNames")));
            this._theGrid.AutoResizeCols = true;
            this._theGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            iGColPattern3.CellStyle = this._theGridCol1CellStyle;
            iGColPattern3.ColHdrStyle = this._theGridCol1ColHdrStyle;
            iGColPattern3.Key = "Name";
            iGColPattern3.Text = "Name";
            iGColPattern3.Width = 292;
            iGColPattern4.CellStyle = this._theGridCol2CellStyle;
            iGColPattern4.ColHdrStyle = this._theGridCol2ColHdrStyle;
            iGColPattern4.Key = "Value";
            iGColPattern4.Text = "Value";
            iGColPattern4.Width = 243;
            this._theGrid.Cols.AddRange(new TenTec.Windows.iGridLib.iGColPattern[] {
            iGColPattern3,
            iGColPattern4});
            this._theGrid.CurrentFilter = null;
            this._theGrid.DefaultCol.CellStyle = this.TrafodionIGrid1DefaultCellStyle;
            this._theGrid.DefaultCol.ColHdrStyle = this.TrafodionIGrid1DefaultColHdrStyle;
            this._theGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._theGrid.Header.Height = 20;
            this._theGrid.HelpTopic = "";
            this._theGrid.Location = new System.Drawing.Point(3, 17);
            this._theGrid.Name = "_theGrid";
            this._theGrid.ReadOnly = true;
            this._theGrid.RowMode = true;
            this._theGrid.RowTextCol.CellStyle = this.TrafodionIGrid1RowTextColCellStyle;
            this._theGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this._theGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this._theGrid.SearchAsType.SearchCol = null;
            this._theGrid.Size = new System.Drawing.Size(539, 372);
            this._theGrid.TabIndex = 0;
            this._theGrid.TreeCol = null;
            this._theGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this._theGrid.WordWrap = false;
            // 
            // StatsInGridControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoScroll = true;
            this.Controls.Add(this._theGroupBox);
            this.Name = "StatsInGridControl";
            this.Size = new System.Drawing.Size(545, 392);
            this._theGroupBox.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this._theGrid)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _theGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionIGrid _theGrid;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle TrafodionIGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1RowTextColCellStyle;
        private TenTec.Windows.iGridLib.iGCellStyle _theGridCol1CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle _theGridCol1ColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle _theGridCol2CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle _theGridCol2ColHdrStyle;
    }
}
