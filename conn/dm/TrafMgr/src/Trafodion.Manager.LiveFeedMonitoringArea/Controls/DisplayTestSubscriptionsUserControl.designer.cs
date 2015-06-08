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
﻿namespace Trafodion.Manager.LiveFeedMonitoringArea.Controls
{
    partial class DisplayTestSubscriptionsUserControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DisplayTestSubscriptionsUserControl));
            TenTec.Windows.iGridLib.iGColPattern iGColPattern1 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern2 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern3 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern4 = new TenTec.Windows.iGridLib.iGColPattern();
            this._pubsGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0AT" +
                    "QB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAI" +
                    "ABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            this.oneGuiIGrid1DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.oneGuiIGrid1DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.oneGuiIGrid1RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this._queueGridCol0CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this._queueGridCol0ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this._queueGridCol1CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this._queueGridCol1ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this._queueGridCol2CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this._queueGridCol2ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this._queueGridCol3ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this._queueGridCol3CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            ((System.ComponentModel.ISupportInitialize)(this._pubsGrid)).BeginInit();
            this.SuspendLayout();
            // 
            // _queueGrid
            // 
            this._pubsGrid.AllowColumnFilter = true;
            this._pubsGrid.AllowWordWrap = false;
            this._pubsGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("_queueGrid.AlwaysHiddenColumnNames")));
            this._pubsGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            iGColPattern1.CellStyle = this._queueGridCol0CellStyle;
            iGColPattern1.ColHdrStyle = this._queueGridCol0ColHdrStyle;
            iGColPattern1.Key = ThePublicationNameColumn;
            iGColPattern1.Text = "Publication Name";
            iGColPattern1.Width = 277;
            iGColPattern2.CellStyle = this._queueGridCol1CellStyle;
            iGColPattern2.ColHdrStyle = this._queueGridCol1ColHdrStyle;
            iGColPattern2.Key = TheSubscriptionCountColumn;
            iGColPattern2.Text = "Subscription Count";
            iGColPattern2.Width = 169;
            iGColPattern3.CellStyle = this._queueGridCol2CellStyle;
            iGColPattern3.ColHdrStyle = this._queueGridCol2ColHdrStyle;
            iGColPattern3.Key = TheQueueLengthColumn;
            iGColPattern3.Text = "Queue Length";
            iGColPattern3.Width = 173;
            iGColPattern4.CellStyle = this._queueGridCol3CellStyle;
            iGColPattern4.ColHdrStyle = this._queueGridCol3ColHdrStyle;
            iGColPattern4.Key = "theProgressColumn";
            iGColPattern4.Text = "Progress";
            iGColPattern4.Visible = false;
            iGColPattern4.Width = 185;
            this._pubsGrid.Cols.AddRange(new TenTec.Windows.iGridLib.iGColPattern[] {
            iGColPattern1,
            iGColPattern2,
            iGColPattern3,
            iGColPattern4});
            this._pubsGrid.DefaultCol.CellStyle = this.oneGuiIGrid1DefaultCellStyle;
            this._pubsGrid.DefaultCol.ColHdrStyle = this.oneGuiIGrid1DefaultColHdrStyle;
            this._pubsGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._pubsGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._pubsGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._pubsGrid.Header.Height = 20;
            this._pubsGrid.HelpTopic = global::Trafodion.Manager.Properties.Resources.DefaultPersistenceFolder;
            this._pubsGrid.Location = new System.Drawing.Point(0, 0);
            this._pubsGrid.Name = "_pubsGrid";
            this._pubsGrid.ReadOnly = true;
            this._pubsGrid.RowMode = true;
            this._pubsGrid.RowTextCol.CellStyle = this.oneGuiIGrid1RowTextColCellStyle;
            this._pubsGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this._pubsGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this._pubsGrid.SearchAsType.SearchCol = null;
            this._pubsGrid.Size = new System.Drawing.Size(738, 255);
            this._pubsGrid.TabIndex = 0;
            this._pubsGrid.TreeCol = null;
            this._pubsGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this._pubsGrid.WordWrap = false;
            // 
            // _queueGridCol0CellStyle
            // 
            this._queueGridCol0CellStyle.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            // 
            // _queueGridCol3CellStyle
            // 
            this._queueGridCol3CellStyle.ValueType = typeof(object);
            // 
            // MonitorQueuesUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoScroll = true;
            this.Controls.Add(this._pubsGrid);
            this.Margin = new System.Windows.Forms.Padding(2);
            this.Name = "MonitorQueuesUserControl";
            this.Size = new System.Drawing.Size(738, 255);
            ((System.ComponentModel.ISupportInitialize)(this._pubsGrid)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionIGrid _pubsGrid;
        private TenTec.Windows.iGridLib.iGCellStyle _queueGridCol0CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle _queueGridCol0ColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle _queueGridCol1CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle _queueGridCol1ColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle oneGuiIGrid1DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle oneGuiIGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle oneGuiIGrid1RowTextColCellStyle;
        private TenTec.Windows.iGridLib.iGCellStyle _queueGridCol2CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle _queueGridCol2ColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle _queueGridCol3CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle _queueGridCol3ColHdrStyle;






    }
}
