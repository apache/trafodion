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
    partial class TabularDataDisplayControl
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
            MyDispose(disposing);
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.TrafodionIGrid1DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionIGrid1DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.TrafodionIGrid1RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this._theDataGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0AT" +
                    "QB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAI" +
                    "ABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            ((System.ComponentModel.ISupportInitialize)(this._theDataGrid)).BeginInit();
            this.SuspendLayout();
            // 
            // _theDataGrid
            // 
            this._theDataGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this._theDataGrid.DefaultCol.CellStyle = this.TrafodionIGrid1DefaultCellStyle;
            this._theDataGrid.DefaultCol.ColHdrStyle = this.TrafodionIGrid1DefaultColHdrStyle;
            this._theDataGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theDataGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._theDataGrid.Header.Height = 19;
            this._theDataGrid.Location = new System.Drawing.Point(0, 0);
            this._theDataGrid.Name = "_theDataGrid";
            this._theDataGrid.ReadOnly = true;
            this._theDataGrid.RowMode = true;
            this._theDataGrid.RowTextCol.CellStyle = this.TrafodionIGrid1RowTextColCellStyle;
            this._theDataGrid.Size = new System.Drawing.Size(481, 295);
            this._theDataGrid.TabIndex = 0;
            this._theDataGrid.TreeCol = null;
            this._theDataGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this._theDataGrid.CellDoubleClick += new TenTec.Windows.iGridLib.iGCellDoubleClickEventHandler(this._theDataGrid_CellDoubleClick);
            // 
            // TabularDataDisplayControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theDataGrid);
            this.Name = "TabularDataDisplayControl";
            this.Size = new System.Drawing.Size(481, 295);
            ((System.ComponentModel.ISupportInitialize)(this._theDataGrid)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionIGrid _theDataGrid;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle TrafodionIGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1RowTextColCellStyle;




    }
}
