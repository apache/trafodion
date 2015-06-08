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
﻿namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    partial class QueryListDataGridView
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
                myDispose(disposing);
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(QueryListDataGridView));
            TenTec.Windows.iGridLib.iGColPattern iGColPattern1 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern2 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern3 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern4 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern5 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern6 = new TenTec.Windows.iGridLib.iGColPattern();
            this.iGrid1Col0CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.iGrid1Col0ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.iGrid1Col1CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.imageList1 = new System.Windows.Forms.ImageList(this.components);
            this.iGrid1Col1ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.iGrid1Col2CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.iGrid1Col2ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.iGrid1Col3CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.iGrid1Col3ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.iGrid1DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.iGrid1DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.iGCellStyleDesign1 = new TenTec.Windows.iGridLib.iGCellStyleDesign();
            this.iGrid1RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.iGrid1 = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0AT" +
                    "QB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAI" +
                    "ABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            ((System.ComponentModel.ISupportInitialize)(this.iGrid1)).BeginInit();
            this.SuspendLayout();
            // 
            // iGrid1Col0CellStyle
            // 
            this.iGrid1Col0CellStyle.Type = TenTec.Windows.iGridLib.iGCellType.Check;
            // 
            // iGrid1Col1CellStyle
            // 
            this.iGrid1Col1CellStyle.ImageList = this.imageList1;
            // 
            // imageList1
            // 
            this.imageList1.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList1.ImageStream")));
            this.imageList1.TransparentColor = System.Drawing.Color.Transparent;
            this.imageList1.Images.SetKeyName(0, "CheckboxUnChecked.png");
            this.imageList1.Images.SetKeyName(1, "CheckboxChecked.png");
            // 
            // iGrid1Col3CellStyle
            // 
            this.iGrid1Col3CellStyle.ReadOnly = TenTec.Windows.iGridLib.iGBool.True;
            this.iGrid1Col3CellStyle.TextFormatFlags = TenTec.Windows.iGridLib.iGStringFormatFlags.FitBlackBox;
            // 
            // iGCellStyleDesign1
            // 
            this.iGCellStyleDesign1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(235)))), ((int)(((byte)(240)))), ((int)(((byte)(248)))));
            this.iGCellStyleDesign1.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.iGCellStyleDesign1.ForeColor = System.Drawing.SystemColors.ControlText;
            // 
            // iGrid1
            // 
            this.iGrid1.AllowColumnFilter = false;
            this.iGrid1.AllowWordWrap = false;
            this.iGrid1.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("iGrid1.AlwaysHiddenColumnNames")));
            this.iGrid1.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            iGColPattern1.AllowGrouping = false;
            iGColPattern1.AllowMoving = false;
            iGColPattern1.AllowSizing = false;
            iGColPattern1.CellStyle = this.iGrid1Col0CellStyle;
            iGColPattern1.ColHdrStyle = this.iGrid1Col0ColHdrStyle;
            iGColPattern1.Key = "theCheckBoxColumn";
            iGColPattern1.MaxWidth = 20;
            iGColPattern1.MinWidth = 20;
            iGColPattern1.ShowWhenGrouped = true;
            iGColPattern1.SortOrder = TenTec.Windows.iGridLib.iGSortOrder.None;
            iGColPattern1.Text = "+";
            iGColPattern1.Width = 20;
            iGColPattern2.AllowMoving = false;
            iGColPattern2.AllowSizing = false;
            iGColPattern2.CellStyle = this.iGrid1Col1CellStyle;
            iGColPattern2.ColHdrStyle = this.iGrid1Col1ColHdrStyle;
            iGColPattern2.Key = "theGroupNameColumn";
            iGColPattern2.Text = "Loaded From";
            iGColPattern3.CellStyle = this.iGrid1Col2CellStyle;
            iGColPattern3.ColHdrStyle = this.iGrid1Col2ColHdrStyle;
            iGColPattern3.Key = "theReportDefinitionColumn";
            iGColPattern3.Text = "Name";
            iGColPattern3.Width = 150;
            iGColPattern4.CellStyle = this.iGrid1Col2CellStyle;
            iGColPattern4.ColHdrStyle = this.iGrid1Col2ColHdrStyle;
            iGColPattern4.Key = "theStartTimeColumn";
            iGColPattern4.Text = "Start Time";
            iGColPattern4.Width = 135;
            iGColPattern5.CellStyle = this.iGrid1Col3CellStyle;
            iGColPattern5.ColHdrStyle = this.iGrid1Col3ColHdrStyle;
            iGColPattern5.Key = "theOneLineSummaryColumn";
            iGColPattern5.Text = "Statement Preview";
            iGColPattern5.Width = 400;
            iGColPattern6.CellStyle = this.iGrid1Col3CellStyle;
            iGColPattern6.ColHdrStyle = this.iGrid1Col3ColHdrStyle;
            iGColPattern6.Key = "theTotalCountColumn";
            iGColPattern6.Text = "Count";
            iGColPattern6.Visible = false;
            iGColPattern6.Width = 20;
            this.iGrid1.Cols.AddRange(new TenTec.Windows.iGridLib.iGColPattern[] {
            iGColPattern1,
            iGColPattern2,
            iGColPattern3,
            iGColPattern4,
            iGColPattern5,
            iGColPattern6});
            this.iGrid1.CurrentFilter = null;
            this.iGrid1.DefaultCol.CellStyle = this.iGrid1DefaultCellStyle;
            this.iGrid1.DefaultCol.ColHdrStyle = this.iGrid1DefaultColHdrStyle;
            this.iGrid1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.iGrid1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.iGrid1.ForeColor = System.Drawing.SystemColors.WindowText;
            this.iGrid1.GroupRowLevelStyles = new TenTec.Windows.iGridLib.iGCellStyle[] {
        ((TenTec.Windows.iGridLib.iGCellStyle)(this.iGCellStyleDesign1))};
            this.iGrid1.Header.Height = 20;
            this.iGrid1.HelpTopic = "";
            this.iGrid1.Location = new System.Drawing.Point(0, 0);
            this.iGrid1.Name = "iGrid1";
            this.iGrid1.ReadOnly = true;
            this.iGrid1.RowMode = true;
            this.iGrid1.RowSelectionInCellMode = TenTec.Windows.iGridLib.iGRowSelectionInCellModeTypes.SingleRow;
            this.iGrid1.RowTextCol.CellStyle = this.iGrid1RowTextColCellStyle;
            this.iGrid1.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this.iGrid1.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this.iGrid1.SearchAsType.SearchCol = null;
            this.iGrid1.SelRowsBackColor = System.Drawing.SystemColors.Highlight;
            this.iGrid1.SelRowsBackColorNoFocus = System.Drawing.SystemColors.Highlight;
            this.iGrid1.SelRowsForeColor = System.Drawing.SystemColors.HighlightText;
            this.iGrid1.SelRowsForeColorNoFocus = System.Drawing.SystemColors.HighlightText;
            this.iGrid1.Size = new System.Drawing.Size(422, 300);
            this.iGrid1.TabIndex = 0;
            this.iGrid1.TreeCol = null;
            this.iGrid1.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.iGrid1.WordWrap = false;
            this.iGrid1.CellMouseUp += new TenTec.Windows.iGridLib.iGCellMouseUpEventHandler(this.iGrid1_CellMouseUp);
            this.iGrid1.SelectionChanged += new System.EventHandler(this.iGrid1_SelectionChanged);
            // 
            // QueryListDataGridView
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.iGrid1);
            this.Name = "QueryListDataGridView";
            this.Size = new System.Drawing.Size(422, 300);
            ((System.ComponentModel.ISupportInitialize)(this.iGrid1)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        //private TenTec.Windows.iGridLib.iGrid iGrid1;
        private Trafodion.Manager.Framework.Controls.TrafodionIGrid iGrid1;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid1DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle iGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid1RowTextColCellStyle;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid1Col0CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle iGrid1Col0ColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid1Col1CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle iGrid1Col1ColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid1Col2CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle iGrid1Col2ColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid1Col3CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle iGrid1Col3ColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyleDesign iGCellStyleDesign1;
        private System.Windows.Forms.ImageList imageList1;
    }
}
