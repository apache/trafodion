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
#define INC_MONITOR_WORKLOAD 

namespace Trafodion.Manager.WorkloadArea.Controls
{
    partial class WMSChildrenProcess
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WMSChildrenProcess));
            this.viewInfoGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.viewInfoTabControl = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this.summaryTabPage = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this.summaryIGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0AT" +
                    "QB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAI" +
                    "ABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            this.iGrid1DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.iGrid1DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.iGrid1RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.graphTabPage = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this.m_zedGraphControl = new ZedGraph.ZedGraphControl();
            this.detailedTabPage = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this.detailedIGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0AT" +
                    "QB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAI" +
                    "ABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            this.label1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.panel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.queryTextTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.label2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.refreshButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.helpFilterLinkLabel = new Trafodion.Manager.Framework.Controls.TrafodionLinkLabel();
            this.filterNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.filterLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.parentProcessTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.label3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.queryIdTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.panel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.closeButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.progressBar1 = new Trafodion.Manager.Framework.Controls.TrafodionProgressBar();
            this.toolTip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.oneGuiBannerControl1 = new Trafodion.Manager.Framework.Controls.TrafodionBannerControl();
            this.viewInfoGroupBox.SuspendLayout();
            this.viewInfoTabControl.SuspendLayout();
            this.summaryTabPage.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.summaryIGrid)).BeginInit();
            this.graphTabPage.SuspendLayout();
            this.detailedTabPage.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.detailedIGrid)).BeginInit();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.filterNumericUpDown)).BeginInit();
            this.panel2.SuspendLayout();
            this.oneGuiPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // viewInfoGroupBox
            // 
            this.viewInfoGroupBox.Controls.Add(this.viewInfoTabControl);
            this.viewInfoGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.viewInfoGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.viewInfoGroupBox.Location = new System.Drawing.Point(0, 0);
            this.viewInfoGroupBox.Name = "viewInfoGroupBox";
            this.viewInfoGroupBox.Size = new System.Drawing.Size(883, 327);
            this.viewInfoGroupBox.TabIndex = 0;
            this.viewInfoGroupBox.TabStop = false;
            this.viewInfoGroupBox.Text = "Info";
            // 
            // viewInfoTabControl
            // 
            this.viewInfoTabControl.Controls.Add(this.summaryTabPage);
            this.viewInfoTabControl.Controls.Add(this.graphTabPage);
            this.viewInfoTabControl.Controls.Add(this.detailedTabPage);
            this.viewInfoTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this.viewInfoTabControl.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.viewInfoTabControl.HotTrack = true;
            this.viewInfoTabControl.Location = new System.Drawing.Point(3, 17);
            this.viewInfoTabControl.Multiline = true;
            this.viewInfoTabControl.Name = "viewInfoTabControl";
            this.viewInfoTabControl.Padding = new System.Drawing.Point(10, 5);
            this.viewInfoTabControl.SelectedIndex = 0;
            this.viewInfoTabControl.Size = new System.Drawing.Size(877, 307);
            this.viewInfoTabControl.TabIndex = 0;
            // 
            // summaryTabPage
            // 
            this.summaryTabPage.Controls.Add(this.summaryIGrid);
            this.summaryTabPage.Location = new System.Drawing.Point(4, 26);
            this.summaryTabPage.Name = "summaryTabPage";
            this.summaryTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.summaryTabPage.Size = new System.Drawing.Size(869, 277);
            this.summaryTabPage.TabIndex = 0;
            this.summaryTabPage.Text = "Summary Table";
            this.summaryTabPage.UseVisualStyleBackColor = true;
            // 
            // summaryIGrid
            // 
            this.summaryIGrid.AllowColumnFilter = true;
            this.summaryIGrid.AllowWordWrap = false;
            this.summaryIGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("summaryIGrid.AlwaysHiddenColumnNames")));
            this.summaryIGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this.summaryIGrid.CurrentFilter = null;
            this.summaryIGrid.DefaultCol.CellStyle = this.iGrid1DefaultCellStyle;
            this.summaryIGrid.DefaultCol.ColHdrStyle = this.iGrid1DefaultColHdrStyle;
            this.summaryIGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.summaryIGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.summaryIGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this.summaryIGrid.Header.Height = 19;
            this.summaryIGrid.HelpTopic = "";
            this.summaryIGrid.Location = new System.Drawing.Point(3, 3);
            this.summaryIGrid.Name = "summaryIGrid";
            this.summaryIGrid.ReadOnly = true;
            this.summaryIGrid.RowMode = true;
            this.summaryIGrid.RowTextCol.CellStyle = this.iGrid1RowTextColCellStyle;
            this.summaryIGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this.summaryIGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this.summaryIGrid.SearchAsType.SearchCol = null;
            this.summaryIGrid.Size = new System.Drawing.Size(863, 271);
            this.summaryIGrid.TabIndex = 0;
            this.summaryIGrid.TreeCol = null;
            this.summaryIGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.summaryIGrid.WordWrap = false;
            // 
            // graphTabPage
            // 
            this.graphTabPage.Controls.Add(this.m_zedGraphControl);
            this.graphTabPage.Location = new System.Drawing.Point(4, 26);
            this.graphTabPage.Name = "graphTabPage";
            this.graphTabPage.Size = new System.Drawing.Size(869, 277);
            this.graphTabPage.TabIndex = 2;
            this.graphTabPage.Text = "Summary Graph";
            this.graphTabPage.UseVisualStyleBackColor = true;
            // 
            // m_zedGraphControl
            // 
            this.m_zedGraphControl.Location = new System.Drawing.Point(54, 61);
            this.m_zedGraphControl.Name = "m_zedGraphControl";
            this.m_zedGraphControl.ScrollMaxX = 0;
            this.m_zedGraphControl.ScrollMaxY = 0;
            this.m_zedGraphControl.ScrollMaxY2 = 0;
            this.m_zedGraphControl.ScrollMinX = 0;
            this.m_zedGraphControl.ScrollMinY = 0;
            this.m_zedGraphControl.ScrollMinY2 = 0;
            this.m_zedGraphControl.Size = new System.Drawing.Size(718, 261);
            this.m_zedGraphControl.TabIndex = 0;
            // 
            // detailedTabPage
            // 
            this.detailedTabPage.Controls.Add(this.detailedIGrid);
            this.detailedTabPage.Location = new System.Drawing.Point(4, 26);
            this.detailedTabPage.Name = "detailedTabPage";
            this.detailedTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.detailedTabPage.Size = new System.Drawing.Size(869, 277);
            this.detailedTabPage.TabIndex = 1;
            this.detailedTabPage.Text = "Detail";
            this.detailedTabPage.UseVisualStyleBackColor = true;
            // 
            // detailedIGrid
            // 
            this.detailedIGrid.AllowColumnFilter = true;
            this.detailedIGrid.AllowWordWrap = false;
            this.detailedIGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("detailedIGrid.AlwaysHiddenColumnNames")));
            this.detailedIGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this.detailedIGrid.CurrentFilter = null;
            this.detailedIGrid.DefaultCol.CellStyle = this.iGrid1DefaultCellStyle;
            this.detailedIGrid.DefaultCol.ColHdrStyle = this.iGrid1DefaultColHdrStyle;
            this.detailedIGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.detailedIGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.detailedIGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this.detailedIGrid.Header.Height = 19;
            this.detailedIGrid.HelpTopic = "";
            this.detailedIGrid.Location = new System.Drawing.Point(3, 3);
            this.detailedIGrid.Name = "detailedIGrid";
            this.detailedIGrid.ReadOnly = true;
            this.detailedIGrid.RowMode = true;
            this.detailedIGrid.RowTextCol.CellStyle = this.iGrid1RowTextColCellStyle;
            this.detailedIGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this.detailedIGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this.detailedIGrid.SearchAsType.SearchCol = null;
            this.detailedIGrid.Size = new System.Drawing.Size(863, 271);
            this.detailedIGrid.TabIndex = 1;
            this.detailedIGrid.TreeCol = null;
            this.detailedIGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.detailedIGrid.WordWrap = false;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.label1.Location = new System.Drawing.Point(12, 17);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(55, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Query ID:";
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel1.Controls.Add(this.queryTextTextBox);
            this.panel1.Controls.Add(this.label2);
            this.panel1.Controls.Add(this.refreshButton);
            this.panel1.Controls.Add(this.helpFilterLinkLabel);
            this.panel1.Controls.Add(this.filterNumericUpDown);
            this.panel1.Controls.Add(this.filterLabel);
            this.panel1.Controls.Add(this.parentProcessTextBox);
            this.panel1.Controls.Add(this.label3);
            this.panel1.Controls.Add(this.queryIdTextBox);
            this.panel1.Controls.Add(this.label1);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel1.Location = new System.Drawing.Point(0, 51);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(883, 118);
            this.panel1.TabIndex = 0;
            // 
            // queryTextTextBox
            // 
            this.queryTextTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.queryTextTextBox.Location = new System.Drawing.Point(98, 44);
            this.queryTextTextBox.Name = "queryTextTextBox";
            this.queryTextTextBox.ReadOnly = true;
            this.queryTextTextBox.Size = new System.Drawing.Size(738, 21);
            this.queryTextTextBox.TabIndex = 3;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.label2.Location = new System.Drawing.Point(12, 48);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(66, 13);
            this.label2.TabIndex = 2;
            this.label2.Text = "Query Text:";
            // 
            // refreshButton
            // 
            this.refreshButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.refreshButton.Location = new System.Drawing.Point(777, 74);
            this.refreshButton.Name = "refreshButton";
            this.refreshButton.Size = new System.Drawing.Size(59, 23);
            this.refreshButton.TabIndex = 8;
            this.refreshButton.Text = "Refresh";
            this.refreshButton.UseVisualStyleBackColor = true;
            this.refreshButton.Click += new System.EventHandler(this.refreshButton_Click);
            // 
            // helpFilterLinkLabel
            // 
            this.helpFilterLinkLabel.AutoSize = true;
            this.helpFilterLinkLabel.Location = new System.Drawing.Point(654, 97);
            this.helpFilterLinkLabel.Name = "helpFilterLinkLabel";
            this.helpFilterLinkLabel.Size = new System.Drawing.Size(13, 13);
            this.helpFilterLinkLabel.TabIndex = 9;
            this.helpFilterLinkLabel.TabStop = true;
            this.helpFilterLinkLabel.Text = "?";
            // 
            // filterNumericUpDown
            // 
            this.filterNumericUpDown.Location = new System.Drawing.Point(717, 75);
            this.filterNumericUpDown.Name = "filterNumericUpDown";
            this.filterNumericUpDown.Size = new System.Drawing.Size(53, 20);
            this.filterNumericUpDown.TabIndex = 7;
            // 
            // filterLabel
            // 
            this.filterLabel.AutoSize = true;
            this.filterLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.filterLabel.Location = new System.Drawing.Point(625, 79);
            this.filterLabel.Name = "filterLabel";
            this.filterLabel.Size = new System.Drawing.Size(87, 13);
            this.filterLabel.TabIndex = 6;
            this.filterLabel.Text = "Filter CPU Value:";
            // 
            // parentProcessTextBox
            // 
            this.parentProcessTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.parentProcessTextBox.Location = new System.Drawing.Point(98, 75);
            this.parentProcessTextBox.Name = "parentProcessTextBox";
            this.parentProcessTextBox.ReadOnly = true;
            this.parentProcessTextBox.Size = new System.Drawing.Size(433, 21);
            this.parentProcessTextBox.TabIndex = 5;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.label3.Location = new System.Drawing.Point(12, 79);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(83, 13);
            this.label3.TabIndex = 4;
            this.label3.Text = "Parent Process:";
            // 
            // queryIdTextBox
            // 
            this.queryIdTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.queryIdTextBox.Location = new System.Drawing.Point(98, 13);
            this.queryIdTextBox.Name = "queryIdTextBox";
            this.queryIdTextBox.ReadOnly = true;
            this.queryIdTextBox.Size = new System.Drawing.Size(738, 21);
            this.queryIdTextBox.TabIndex = 1;
            // 
            // panel2
            // 
            this.panel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel2.Controls.Add(this.viewInfoGroupBox);
            this.panel2.Controls.Add(this.oneGuiPanel1);
            this.panel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel2.Location = new System.Drawing.Point(0, 169);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(883, 363);
            this.panel2.TabIndex = 2;
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this.closeButton);
            this.oneGuiPanel1.Controls.Add(this.helpButton);
            this.oneGuiPanel1.Controls.Add(this.progressBar1);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 327);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(883, 36);
            this.oneGuiPanel1.TabIndex = 4;
            // 
            // closeButton
            // 
            this.closeButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.closeButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.closeButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.closeButton.Location = new System.Drawing.Point(722, 7);
            this.closeButton.Name = "closeButton";
            this.closeButton.Size = new System.Drawing.Size(75, 23);
            this.closeButton.TabIndex = 0;
            this.closeButton.Text = "&Close";
            this.closeButton.UseVisualStyleBackColor = true;
            this.closeButton.Click += new System.EventHandler(this.closeButton_Click);
            // 
            // helpButton
            // 
            this.helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.helpButton.Location = new System.Drawing.Point(803, 7);
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = new System.Drawing.Size(75, 23);
            this.helpButton.TabIndex = 1;
            this.helpButton.Text = "&Help";
            this.helpButton.UseVisualStyleBackColor = true;
            this.helpButton.Click += new System.EventHandler(this.helpButton_Click);
            // 
            // progressBar1
            // 
            this.progressBar1.Location = new System.Drawing.Point(5, 9);
            this.progressBar1.Name = "progressBar1";
            this.progressBar1.Size = new System.Drawing.Size(183, 19);
            this.progressBar1.TabIndex = 2;
            // 
            // toolTip1
            // 
            this.toolTip1.IsBalloon = true;
            // 
            // oneGuiBannerControl1
            // 
            this.oneGuiBannerControl1.ConnectionDefinition = null;
            this.oneGuiBannerControl1.Dock = System.Windows.Forms.DockStyle.Top;
            this.oneGuiBannerControl1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiBannerControl1.Name = "oneGuiBannerControl1";
            this.oneGuiBannerControl1.ShowDescription = true;
            this.oneGuiBannerControl1.Size = new System.Drawing.Size(883, 51);
            this.oneGuiBannerControl1.TabIndex = 10;
            // 
            // WMSChildrenProcess
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(883, 532);
            this.Controls.Add(this.panel2);
            this.Controls.Add(this.panel1);
            this.Controls.Add(this.oneGuiBannerControl1);
            this.Name = "WMSChildrenProcess";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "HP Database Manager - Children Processes";
            this.viewInfoGroupBox.ResumeLayout(false);
            this.viewInfoTabControl.ResumeLayout(false);
            this.summaryTabPage.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.summaryIGrid)).EndInit();
            this.graphTabPage.ResumeLayout(false);
            this.detailedTabPage.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.detailedIGrid)).EndInit();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.filterNumericUpDown)).EndInit();
            this.panel2.ResumeLayout(false);
            this.oneGuiPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

		}

		#endregion

		private Trafodion.Manager.Framework.Controls.TrafodionGroupBox viewInfoGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionIGrid summaryIGrid;
		private TenTec.Windows.iGridLib.iGCellStyle iGrid1DefaultCellStyle;
		private TenTec.Windows.iGridLib.iGColHdrStyle iGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid1RowTextColCellStyle;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel2;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox parentProcessTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label3;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox queryIdTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip toolTip1;
        private Trafodion.Manager.Framework.Controls.TrafodionTabControl viewInfoTabControl;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage summaryTabPage;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage detailedTabPage;
        private Trafodion.Manager.Framework.Controls.TrafodionIGrid detailedIGrid;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown filterNumericUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel filterLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLinkLabel helpFilterLinkLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton refreshButton;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label2;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox queryTextTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage graphTabPage;
        private ZedGraph.ZedGraphControl zedGraphControl1;
        private System.Windows.Forms.Timer timer1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionProgressBar progressBar1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton closeButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton helpButton;
        private Trafodion.Manager.Framework.Controls.TrafodionBannerControl oneGuiBannerControl1;
	}
}