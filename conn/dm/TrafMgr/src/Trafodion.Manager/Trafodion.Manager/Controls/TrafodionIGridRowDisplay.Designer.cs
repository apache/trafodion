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
    partial class TrafodionIGridRowDisplay
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TrafodionIGridRowDisplay));
            this.TrafodionIGrid1DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionIGrid1DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.TrafodionIGrid1RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this._theRowDataGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theRowDataGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0AT" +
                    "QB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAI" +
                    "ABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            this._theToolStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._theFirstRowButton = new System.Windows.Forms.ToolStripButton();
            this._thePreviousRowButton = new System.Windows.Forms.ToolStripButton();
            this._theCounterLabel = new System.Windows.Forms.ToolStripLabel();
            this._theNextRowButton = new System.Windows.Forms.ToolStripButton();
            this._theLastRowButton = new System.Windows.Forms.ToolStripButton();
            this._theRowDataGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theRowDataGrid)).BeginInit();
            this._theToolStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theRowDataGroupBox
            // 
            this._theRowDataGroupBox.Controls.Add(this._theRowDataGrid);
            this._theRowDataGroupBox.Controls.Add(this._theToolStrip);
            this._theRowDataGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theRowDataGroupBox.Location = new System.Drawing.Point(0, 0);
            this._theRowDataGroupBox.Name = "_theRowDataGroupBox";
            this._theRowDataGroupBox.Size = new System.Drawing.Size(603, 404);
            this._theRowDataGroupBox.TabIndex = 0;
            this._theRowDataGroupBox.TabStop = false;
            this._theRowDataGroupBox.Text = "Row Data";
            // 
            // _theRowDataGrid
            // 
            this._theRowDataGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this._theRowDataGrid.CurrentFilter = null;
            this._theRowDataGrid.DefaultCol.CellStyle = this.TrafodionIGrid1DefaultCellStyle;
            this._theRowDataGrid.DefaultCol.ColHdrStyle = this.TrafodionIGrid1DefaultColHdrStyle;
            this._theRowDataGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theRowDataGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._theRowDataGrid.Header.Height = 19;
            this._theRowDataGrid.Location = new System.Drawing.Point(3, 41);
            this._theRowDataGrid.Name = "_theRowDataGrid";
            this._theRowDataGrid.ReadOnly = true;
            this._theRowDataGrid.RowMode = true;
            this._theRowDataGrid.RowTextCol.CellStyle = this.TrafodionIGrid1RowTextColCellStyle;
            this._theRowDataGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this._theRowDataGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this._theRowDataGrid.SearchAsType.SearchCol = null;
            this._theRowDataGrid.Size = new System.Drawing.Size(597, 360);
            this._theRowDataGrid.TabIndex = 0;
            this._theRowDataGrid.TreeCol = null;
            this._theRowDataGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            // 
            // _theToolStrip
            // 
            this._theToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._theFirstRowButton,
            this._thePreviousRowButton,
            this._theCounterLabel,
            this._theNextRowButton,
            this._theLastRowButton});
            this._theToolStrip.Location = new System.Drawing.Point(3, 16);
            this._theToolStrip.Name = "_theToolStrip";
            this._theToolStrip.Size = new System.Drawing.Size(597, 25);
            this._theToolStrip.TabIndex = 1;
            this._theToolStrip.Text = "TrafodionToolStrip1";
            // 
            // _theFirstRowButton
            // 
            this._theFirstRowButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theFirstRowButton.Image = ((System.Drawing.Image)(resources.GetObject("_theFirstRowButton.Image")));
            this._theFirstRowButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theFirstRowButton.Name = "_theFirstRowButton";
            this._theFirstRowButton.Size = new System.Drawing.Size(23, 22);
            this._theFirstRowButton.Text = "First Row";
            this._theFirstRowButton.Click += new System.EventHandler(this._theFirstRowButton_Click);
            // 
            // _thePreviousRowButton
            // 
            this._thePreviousRowButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._thePreviousRowButton.Image = ((System.Drawing.Image)(resources.GetObject("_thePreviousRowButton.Image")));
            this._thePreviousRowButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._thePreviousRowButton.Name = "_thePreviousRowButton";
            this._thePreviousRowButton.Size = new System.Drawing.Size(23, 22);
            this._thePreviousRowButton.Text = "Previous Row";
            this._thePreviousRowButton.Click += new System.EventHandler(this._thePreviousRowButton_Click);
            // 
            // _theCounterLabel
            // 
            this._theCounterLabel.AutoSize = false;
            this._theCounterLabel.Name = "_theCounterLabel";
            this._theCounterLabel.Size = new System.Drawing.Size(130, 22);
            this._theCounterLabel.Text = "Row 0 of 0";
            // 
            // _theNextRowButton
            // 
            this._theNextRowButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theNextRowButton.Image = ((System.Drawing.Image)(resources.GetObject("_theNextRowButton.Image")));
            this._theNextRowButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theNextRowButton.Name = "_theNextRowButton";
            this._theNextRowButton.Size = new System.Drawing.Size(23, 22);
            this._theNextRowButton.Text = "Next Row";
            this._theNextRowButton.Click += new System.EventHandler(this._theNextRowButton_Click);
            // 
            // _theLastRowButton
            // 
            this._theLastRowButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theLastRowButton.Image = ((System.Drawing.Image)(resources.GetObject("_theLastRowButton.Image")));
            this._theLastRowButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theLastRowButton.Name = "_theLastRowButton";
            this._theLastRowButton.Size = new System.Drawing.Size(23, 22);
            this._theLastRowButton.Text = "Last Row";
            this._theLastRowButton.Click += new System.EventHandler(this._theLastRowButton_Click);
            // 
            // TrafodionIGridRowDisplay
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theRowDataGroupBox);
            this.Name = "TrafodionIGridRowDisplay";
            this.Size = new System.Drawing.Size(603, 404);
            this._theRowDataGroupBox.ResumeLayout(false);
            this._theRowDataGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theRowDataGrid)).EndInit();
            this._theToolStrip.ResumeLayout(false);
            this._theToolStrip.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _theRowDataGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionIGrid _theRowDataGrid;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle TrafodionIGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1RowTextColCellStyle;
        private TrafodionToolStrip _theToolStrip;
        private System.Windows.Forms.ToolStripButton _theFirstRowButton;
        private System.Windows.Forms.ToolStripButton _thePreviousRowButton;
        private System.Windows.Forms.ToolStripButton _theNextRowButton;
        private System.Windows.Forms.ToolStripButton _theLastRowButton;
        private System.Windows.Forms.ToolStripLabel _theCounterLabel;
    }
}
