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
namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    partial class QueryResultControl
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
            _theTrafodionIGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0ATQB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAIABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            ((System.ComponentModel.ISupportInitialize)(this._theTrafodionIGrid)).BeginInit();
            this.SuspendLayout();
            // 
            // _theTrafodionDataGridView
            // 
            this._theTrafodionIGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTrafodionIGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._theTrafodionIGrid.Header.Height = 19;
            this._theTrafodionIGrid.Location = new System.Drawing.Point(0, 0);
            this._theTrafodionIGrid.Name = "_theTrafodionDataGridView";
            this._theTrafodionIGrid.ReadOnly = true;
            this._theTrafodionIGrid.Size = new System.Drawing.Size(714, 356);
            this._theTrafodionIGrid.TabIndex = 0;
            this._theTrafodionIGrid.TreeCol = null;
            this._theTrafodionIGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            // 
            // QueryResultControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theTrafodionIGrid);
            this.Name = "QueryResultControl";
            this.Size = new System.Drawing.Size(714, 356);
            ((System.ComponentModel.ISupportInitialize)(this._theTrafodionIGrid)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionIGrid _theTrafodionIGrid;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid1DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle iGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid1RowTextColCellStyle;
    }
}
