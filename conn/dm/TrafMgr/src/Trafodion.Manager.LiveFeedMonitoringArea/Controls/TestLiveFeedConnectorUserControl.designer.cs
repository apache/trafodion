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
    partial class TestLiveFeedConnectorUserControl
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TestLiveFeedConnectorUserControl));
            TenTec.Windows.iGridLib.iGColPattern iGColPattern1 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern2 = new TenTec.Windows.iGridLib.iGColPattern();
            this._publicationGridCol0CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this._publicationGridCol0ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this._publicationGridCol1CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this._publicationGridCol1ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.oneGuiGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._publicationGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0AT" +
                    "QB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAI" +
                    "ABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            this.oneGuiIGrid1DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.oneGuiIGrid1DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.oneGuiIGrid1RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this._lastReceivedStats = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this.oneGuiPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._receivedTime = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._receivedCounter = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._connectorStatus = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiPanel1.SuspendLayout();
            this.oneGuiGroupBox1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._publicationGrid)).BeginInit();
            this.oneGuiPanel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // _publicationGridCol0CellStyle
            // 
            this._publicationGridCol0CellStyle.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            // 
            // _publicationGridCol1CellStyle
            // 
            this._publicationGridCol1CellStyle.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this.oneGuiGroupBox1);
            this.oneGuiPanel1.Controls.Add(this._lastReceivedStats);
            this.oneGuiPanel1.Controls.Add(this.oneGuiPanel2);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(569, 255);
            this.oneGuiPanel1.TabIndex = 0;
            // 
            // oneGuiGroupBox1
            // 
            this.oneGuiGroupBox1.Controls.Add(this._publicationGrid);
            this.oneGuiGroupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox1.Location = new System.Drawing.Point(0, 77);
            this.oneGuiGroupBox1.Name = "oneGuiGroupBox1";
            this.oneGuiGroupBox1.Size = new System.Drawing.Size(569, 178);
            this.oneGuiGroupBox1.TabIndex = 3;
            this.oneGuiGroupBox1.TabStop = false;
            this.oneGuiGroupBox1.Text = "Publications Stats";
            // 
            // _publicationGrid
            // 
            this._publicationGrid.AllowColumnFilter = true;
            this._publicationGrid.AllowWordWrap = false;
            this._publicationGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("_publicationGrid.AlwaysHiddenColumnNames")));
            this._publicationGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            iGColPattern1.CellStyle = this._publicationGridCol0CellStyle;
            iGColPattern1.ColHdrStyle = this._publicationGridCol0ColHdrStyle;
            iGColPattern1.Key = "thePublicationColumn";
            iGColPattern1.Text = "Publication";
            iGColPattern1.Width = 277;
            iGColPattern2.CellStyle = this._publicationGridCol1CellStyle;
            iGColPattern2.ColHdrStyle = this._publicationGridCol1ColHdrStyle;
            iGColPattern2.Key = "theTotalCountColumn";
            iGColPattern2.Text = "Total Received Count";
            iGColPattern2.Width = 282;
            this._publicationGrid.Cols.AddRange(new TenTec.Windows.iGridLib.iGColPattern[] {
            iGColPattern1,
            iGColPattern2});
            this._publicationGrid.CurrentFilter = null;
            this._publicationGrid.DefaultCol.CellStyle = this.oneGuiIGrid1DefaultCellStyle;
            this._publicationGrid.DefaultCol.ColHdrStyle = this.oneGuiIGrid1DefaultColHdrStyle;
            this._publicationGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._publicationGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._publicationGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._publicationGrid.Header.Height = 20;
            this._publicationGrid.HelpTopic = global::Trafodion.Manager.Properties.Resources.DefaultPersistenceFolder;
            this._publicationGrid.Location = new System.Drawing.Point(3, 17);
            this._publicationGrid.Name = "_publicationGrid";
            this._publicationGrid.ReadOnly = true;
            this._publicationGrid.RowMode = true;
            this._publicationGrid.RowTextCol.CellStyle = this.oneGuiIGrid1RowTextColCellStyle;
            this._publicationGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this._publicationGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this._publicationGrid.SearchAsType.SearchCol = null;
            this._publicationGrid.Size = new System.Drawing.Size(563, 158);
            this._publicationGrid.TabIndex = 0;
            this._publicationGrid.TreeCol = null;
            this._publicationGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this._publicationGrid.WordWrap = false;
            // 
            // _lastReceivedStats
            // 
            this._lastReceivedStats.Dock = System.Windows.Forms.DockStyle.Top;
            this._lastReceivedStats.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._lastReceivedStats.ForeColor = System.Drawing.SystemColors.Highlight;
            this._lastReceivedStats.Location = new System.Drawing.Point(0, 30);
            this._lastReceivedStats.Name = "_lastReceivedStats";
            this._lastReceivedStats.Size = new System.Drawing.Size(569, 47);
            this._lastReceivedStats.TabIndex = 1;
            this._lastReceivedStats.Text = global::Trafodion.Manager.Properties.Resources.DefaultPersistenceFolder;
            // 
            // oneGuiPanel2
            // 
            this.oneGuiPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel2.Controls.Add(this._receivedTime);
            this.oneGuiPanel2.Controls.Add(this._receivedCounter);
            this.oneGuiPanel2.Controls.Add(this._connectorStatus);
            this.oneGuiPanel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.oneGuiPanel2.Location = new System.Drawing.Point(0, 0);
            this.oneGuiPanel2.Name = "oneGuiPanel2";
            this.oneGuiPanel2.Size = new System.Drawing.Size(569, 30);
            this.oneGuiPanel2.TabIndex = 0;
            // 
            // _receivedTime
            // 
            this._receivedTime.AutoSize = true;
            this._receivedTime.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._receivedTime.Location = new System.Drawing.Point(336, 10);
            this._receivedTime.Name = "_receivedTime";
            this._receivedTime.Size = new System.Drawing.Size(58, 13);
            this._receivedTime.TabIndex = 2;
            this._receivedTime.Text = "Received: ";
            // 
            // _receivedCounter
            // 
            this._receivedCounter.AutoSize = true;
            this._receivedCounter.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._receivedCounter.Location = new System.Drawing.Point(175, 10);
            this._receivedCounter.Name = "_receivedCounter";
            this._receivedCounter.Size = new System.Drawing.Size(123, 13);
            this._receivedCounter.TabIndex = 1;
            this._receivedCounter.Text = "Total Received Count: 0";
            // 
            // _connectorStatus
            // 
            this._connectorStatus.AutoSize = true;
            this._connectorStatus.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._connectorStatus.Location = new System.Drawing.Point(3, 10);
            this._connectorStatus.Margin = new System.Windows.Forms.Padding(3);
            this._connectorStatus.Name = "_connectorStatus";
            this._connectorStatus.Size = new System.Drawing.Size(130, 13);
            this._connectorStatus.TabIndex = 0;
            this._connectorStatus.Text = "LiveFeed Connector Status: ";
            // 
            // DisplayLiveFeedConnectorUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.oneGuiPanel1);
            this.Name = "DisplayLiveFeedConnectorUserControl";
            this.Size = new System.Drawing.Size(569, 255);
            this.oneGuiPanel1.ResumeLayout(false);
            this.oneGuiGroupBox1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this._publicationGrid)).EndInit();
            this.oneGuiPanel2.ResumeLayout(false);
            this.oneGuiPanel2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel2;
        private Trafodion.Manager.Framework.Controls.TrafodionRichTextBox _lastReceivedStats;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _receivedCounter;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _connectorStatus;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _receivedTime;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionIGrid _publicationGrid;
        private TenTec.Windows.iGridLib.iGCellStyle oneGuiIGrid1DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle oneGuiIGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle oneGuiIGrid1RowTextColCellStyle;
        private TenTec.Windows.iGridLib.iGCellStyle _publicationGridCol0CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle _publicationGridCol0ColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle _publicationGridCol1CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle _publicationGridCol1ColHdrStyle;
    }
}
