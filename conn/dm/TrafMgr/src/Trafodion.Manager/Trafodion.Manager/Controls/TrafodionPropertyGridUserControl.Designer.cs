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
    partial class TrafodionPropertyGridUserControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TrafodionPropertyGridUserControl));
            TenTec.Windows.iGridLib.iGColPattern iGColPattern1 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern2 = new TenTec.Windows.iGridLib.iGColPattern();
            this._theGridCol0CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this._theGridCol0ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this._theGridCol1CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this._theGridCol1ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.TrafodionIGrid1DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionIGrid1DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.TrafodionIGrid1RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0AT" +
                    "QB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAI" +
                    "ABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            this.TrafodionPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theFilterButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theFilterAppliedLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionPanel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theGrid)).BeginInit();
            this.TrafodionPanel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theGridCol0CellStyle
            // 
            this._theGridCol0CellStyle.Font = new System.Drawing.Font("Calibri", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theGridCol0CellStyle.ForeColor = System.Drawing.Color.MidnightBlue;
            // 
            // _theGridCol1CellStyle
            // 
            this._theGridCol1CellStyle.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theGridCol1CellStyle.ForeColor = System.Drawing.Color.MidnightBlue;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._theGrid);
            this.TrafodionPanel1.Controls.Add(this.TrafodionPanel2);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(258, 340);
            this.TrafodionPanel1.TabIndex = 1;
            // 
            // _theGrid
            // 
            this._theGrid.AllowColumnFilter = false;
            this._theGrid.AllowWordWrap = false;
            this._theGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("_theGrid.AlwaysHiddenColumnNames")));
            this._theGrid.AutoResizeCols = true;
            this._theGrid.BackColorEvenRows = System.Drawing.SystemColors.Window;
            this._theGrid.BackColorOddRows = System.Drawing.SystemColors.Window;
            iGColPattern1.CellStyle = this._theGridCol0CellStyle;
            iGColPattern1.ColHdrStyle = this._theGridCol0ColHdrStyle;
            iGColPattern1.Key = "Name";
            iGColPattern1.Text = "Name";
            iGColPattern1.Width = 127;
            iGColPattern2.CellStyle = this._theGridCol1CellStyle;
            iGColPattern2.ColHdrStyle = this._theGridCol1ColHdrStyle;
            iGColPattern2.Key = "Value";
            iGColPattern2.Text = "Value";
            iGColPattern2.Width = 127;
            this._theGrid.Cols.AddRange(new TenTec.Windows.iGridLib.iGColPattern[] {
            iGColPattern1,
            iGColPattern2});
            this._theGrid.CurrentFilter = null;
            this._theGrid.DefaultCol.CellStyle = this.TrafodionIGrid1DefaultCellStyle;
            this._theGrid.DefaultCol.ColHdrStyle = this.TrafodionIGrid1DefaultColHdrStyle;
            this._theGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theGrid.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._theGrid.GridLines.Horizontal = new TenTec.Windows.iGridLib.iGPenStyle(System.Drawing.SystemColors.ControlLight, 1, System.Drawing.Drawing2D.DashStyle.Solid);
            this._theGrid.GridLines.Vertical = new TenTec.Windows.iGridLib.iGPenStyle(System.Drawing.SystemColors.ControlLight, 1, System.Drawing.Drawing2D.DashStyle.Solid);
            this._theGrid.Header.Height = 17;
            this._theGrid.HelpTopic = global::Trafodion.Manager.Properties.Resources.NCICustomPrompt;
            this._theGrid.Location = new System.Drawing.Point(0, 22);
            this._theGrid.Margin = new System.Windows.Forms.Padding(0);
            this._theGrid.Name = "_theGrid";
            this._theGrid.ReadOnly = true;
            this._theGrid.RowMode = true;
            this._theGrid.RowTextCol.CellStyle = this.TrafodionIGrid1RowTextColCellStyle;
            this._theGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this._theGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this._theGrid.SearchAsType.SearchCol = null;
            this._theGrid.Size = new System.Drawing.Size(258, 318);
            this._theGrid.TabIndex = 0;
            this._theGrid.TreeCol = null;
            this._theGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this._theGrid.WordWrap = false;
            // 
            // TrafodionPanel2
            // 
            this.TrafodionPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel2.Controls.Add(this._theFilterButton);
            this.TrafodionPanel2.Controls.Add(this._theFilterAppliedLabel);
            this.TrafodionPanel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionPanel2.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel2.Name = "TrafodionPanel2";
            this.TrafodionPanel2.Size = new System.Drawing.Size(258, 22);
            this.TrafodionPanel2.TabIndex = 1;
            // 
            // _theFilterButton
            // 
            this._theFilterButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._theFilterButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theFilterButton.Image = global::Trafodion.Manager.Properties.Resources.Filter2HS;
            this._theFilterButton.Location = new System.Drawing.Point(183, 1);
            this._theFilterButton.Name = "_theFilterButton";
            this._theFilterButton.Size = new System.Drawing.Size(75, 20);
            this._theFilterButton.TabIndex = 1;
            this._theFilterButton.Text = "Filter";
            this._theFilterButton.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this._theFilterButton.UseVisualStyleBackColor = true;
            this._theFilterButton.Click += new System.EventHandler(this._theFilterButton_Click);
            // 
            // _theFilterAppliedLabel
            // 
            this._theFilterAppliedLabel.Font = new System.Drawing.Font("Verdana", 8.25F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theFilterAppliedLabel.ForeColor = System.Drawing.Color.ForestGreen;
            this._theFilterAppliedLabel.Location = new System.Drawing.Point(10, 5);
            this._theFilterAppliedLabel.Name = "_theFilterAppliedLabel";
            this._theFilterAppliedLabel.Size = new System.Drawing.Size(100, 13);
            this._theFilterAppliedLabel.TabIndex = 0;
            this._theFilterAppliedLabel.Text = "Filter Applied";
            // 
            // TrafodionPropertyGridUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionPanel1);
            this.Margin = new System.Windows.Forms.Padding(0);
            this.Name = "TrafodionPropertyGridUserControl";
            this.Size = new System.Drawing.Size(258, 340);
            this.TrafodionPanel1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this._theGrid)).EndInit();
            this.TrafodionPanel2.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle TrafodionIGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1RowTextColCellStyle;
        private TenTec.Windows.iGridLib.iGCellStyle _theGridCol0CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle _theGridCol0ColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle _theGridCol1CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle _theGridCol1ColHdrStyle;
        private Trafodion.Manager.Framework.Controls.TrafodionIGrid _theGrid;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _theFilterAppliedLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theFilterButton;
    }
}
