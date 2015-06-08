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
    partial class WMSRulesAssociated
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WMSRulesAssociated));
            this.ruleAssignGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.ruleAssignedIGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0AT" +
                    "QB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAI" +
                    "ABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            this.iGrid1DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.iGrid1DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.iGrid1RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.infoIGridGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.infoIGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0AT" +
                    "QB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAI" +
                    "ABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            this.iGrid2DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.iGrid2DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.iGrid2RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.serviceInfoButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.ruleInfoButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.InfoPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.splitContainer1 = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this.buttonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.oneGuiBannerControl1 = new Trafodion.Manager.Framework.Controls.TrafodionBannerControl();
            this.ruleAssignGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.ruleAssignedIGrid)).BeginInit();
            this.infoIGridGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.infoIGrid)).BeginInit();
            this.InfoPanel.SuspendLayout();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.buttonPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // ruleAssignGroupBox
            // 
            this.ruleAssignGroupBox.Controls.Add(this.ruleAssignedIGrid);
            this.ruleAssignGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.ruleAssignGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.ruleAssignGroupBox.Location = new System.Drawing.Point(0, 0);
            this.ruleAssignGroupBox.Name = "ruleAssignGroupBox";
            this.ruleAssignGroupBox.Size = new System.Drawing.Size(873, 201);
            this.ruleAssignGroupBox.TabIndex = 0;
            this.ruleAssignGroupBox.TabStop = false;
            this.ruleAssignGroupBox.Text = "Rules Associated";
            // 
            // ruleAssignedIGrid
            // 
            this.ruleAssignedIGrid.AllowColumnFilter = true;
            this.ruleAssignedIGrid.AllowWordWrap = false;
            this.ruleAssignedIGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("ruleAssignedIGrid.AlwaysHiddenColumnNames")));
            this.ruleAssignedIGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this.ruleAssignedIGrid.CurrentFilter = null;
            this.ruleAssignedIGrid.DefaultCol.CellStyle = this.iGrid1DefaultCellStyle;
            this.ruleAssignedIGrid.DefaultCol.ColHdrStyle = this.iGrid1DefaultColHdrStyle;
            this.ruleAssignedIGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.ruleAssignedIGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.ruleAssignedIGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this.ruleAssignedIGrid.Header.Height = 19;
            this.ruleAssignedIGrid.HelpTopic = "";
            this.ruleAssignedIGrid.Location = new System.Drawing.Point(3, 17);
            this.ruleAssignedIGrid.Name = "ruleAssignedIGrid";
            this.ruleAssignedIGrid.ReadOnly = true;
            this.ruleAssignedIGrid.RowMode = true;
            this.ruleAssignedIGrid.RowTextCol.CellStyle = this.iGrid1RowTextColCellStyle;
            this.ruleAssignedIGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this.ruleAssignedIGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this.ruleAssignedIGrid.SearchAsType.SearchCol = null;
            this.ruleAssignedIGrid.Size = new System.Drawing.Size(867, 181);
            this.ruleAssignedIGrid.TabIndex = 0;
            this.ruleAssignedIGrid.TreeCol = null;
            this.ruleAssignedIGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.ruleAssignedIGrid.WordWrap = false;
            // 
            // infoIGridGroupBox
            // 
            this.infoIGridGroupBox.Controls.Add(this.infoIGrid);
            this.infoIGridGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.infoIGridGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.infoIGridGroupBox.Location = new System.Drawing.Point(0, 0);
            this.infoIGridGroupBox.Name = "infoIGridGroupBox";
            this.infoIGridGroupBox.Size = new System.Drawing.Size(873, 200);
            this.infoIGridGroupBox.TabIndex = 0;
            this.infoIGridGroupBox.TabStop = false;
            this.infoIGridGroupBox.Text = "Service/Rule Info";
            // 
            // infoIGrid
            // 
            this.infoIGrid.AllowColumnFilter = true;
            this.infoIGrid.AllowWordWrap = false;
            this.infoIGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("infoIGrid.AlwaysHiddenColumnNames")));
            this.infoIGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this.infoIGrid.CurrentFilter = null;
            this.infoIGrid.DefaultCol.CellStyle = this.iGrid2DefaultCellStyle;
            this.infoIGrid.DefaultCol.ColHdrStyle = this.iGrid2DefaultColHdrStyle;
            this.infoIGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.infoIGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.infoIGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this.infoIGrid.Header.Height = 19;
            this.infoIGrid.HelpTopic = "";
            this.infoIGrid.Location = new System.Drawing.Point(3, 17);
            this.infoIGrid.Name = "infoIGrid";
            this.infoIGrid.ReadOnly = true;
            this.infoIGrid.RowMode = true;
            this.infoIGrid.RowTextCol.CellStyle = this.iGrid2RowTextColCellStyle;
            this.infoIGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this.infoIGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this.infoIGrid.SearchAsType.SearchCol = null;
            this.infoIGrid.Size = new System.Drawing.Size(867, 180);
            this.infoIGrid.TabIndex = 0;
            this.infoIGrid.TreeCol = null;
            this.infoIGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.infoIGrid.WordWrap = false;
            // 
            // serviceInfoButton
            // 
            this.serviceInfoButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.serviceInfoButton.Enabled = false;
            this.serviceInfoButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.serviceInfoButton.Location = new System.Drawing.Point(489, 5);
            this.serviceInfoButton.Name = "serviceInfoButton";
            this.serviceInfoButton.Size = new System.Drawing.Size(90, 28);
            this.serviceInfoButton.TabIndex = 2;
            this.serviceInfoButton.Text = "&Service Info";
            this.serviceInfoButton.UseVisualStyleBackColor = true;
            this.serviceInfoButton.Click += new System.EventHandler(this.buttonServiceInfo_Click);
            // 
            // ruleInfoButton
            // 
            this.ruleInfoButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.ruleInfoButton.Enabled = false;
            this.ruleInfoButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.ruleInfoButton.Location = new System.Drawing.Point(585, 5);
            this.ruleInfoButton.Name = "ruleInfoButton";
            this.ruleInfoButton.Size = new System.Drawing.Size(90, 28);
            this.ruleInfoButton.TabIndex = 3;
            this.ruleInfoButton.Text = "&Rule Info";
            this.ruleInfoButton.UseVisualStyleBackColor = true;
            this.ruleInfoButton.Click += new System.EventHandler(this.buttonRuleInfo_Click);
            // 
            // InfoPanel
            // 
            this.InfoPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.InfoPanel.Controls.Add(this.splitContainer1);
            this.InfoPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.InfoPanel.Location = new System.Drawing.Point(0, 51);
            this.InfoPanel.Name = "InfoPanel";
            this.InfoPanel.Size = new System.Drawing.Size(873, 410);
            this.InfoPanel.TabIndex = 0;
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(0, 0);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.ruleAssignGroupBox);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.infoIGridGroupBox);
            this.splitContainer1.Size = new System.Drawing.Size(873, 410);
            this.splitContainer1.SplitterDistance = 201;
            this.splitContainer1.SplitterWidth = 9;
            this.splitContainer1.TabIndex = 2;
            // 
            // buttonPanel
            // 
            this.buttonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.buttonPanel.Controls.Add(this.serviceInfoButton);
            this.buttonPanel.Controls.Add(this.helpButton);
            this.buttonPanel.Controls.Add(this.cancelButton);
            this.buttonPanel.Controls.Add(this.ruleInfoButton);
            this.buttonPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.buttonPanel.Location = new System.Drawing.Point(0, 461);
            this.buttonPanel.Name = "buttonPanel";
            this.buttonPanel.Size = new System.Drawing.Size(873, 37);
            this.buttonPanel.TabIndex = 1;
            // 
            // helpButton
            // 
            this.helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.helpButton.Location = new System.Drawing.Point(777, 5);
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = new System.Drawing.Size(90, 28);
            this.helpButton.TabIndex = 3;
            this.helpButton.Text = "&Help";
            this.helpButton.UseVisualStyleBackColor = true;
            this.helpButton.Click += new System.EventHandler(this.helpButton_Click);
            // 
            // cancelButton
            // 
            this.cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cancelButton.Location = new System.Drawing.Point(681, 5);
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = new System.Drawing.Size(90, 28);
            this.cancelButton.TabIndex = 3;
            this.cancelButton.Text = "&Close";
            this.cancelButton.UseVisualStyleBackColor = true;
            this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
            // 
            // oneGuiBannerControl1
            // 
            this.oneGuiBannerControl1.ConnectionDefinition = null;
            this.oneGuiBannerControl1.Dock = System.Windows.Forms.DockStyle.Top;
            this.oneGuiBannerControl1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiBannerControl1.Name = "oneGuiBannerControl1";
            this.oneGuiBannerControl1.ShowDescription = true;
            this.oneGuiBannerControl1.Size = new System.Drawing.Size(873, 51);
            this.oneGuiBannerControl1.TabIndex = 9;
            // 
            // WMSRulesAssociated
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(873, 498);
            this.Controls.Add(this.InfoPanel);
            this.Controls.Add(this.oneGuiBannerControl1);
            this.Controls.Add(this.buttonPanel);
            this.Name = "WMSRulesAssociated";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "HP Database Manager - Rules Associated";
            this.ruleAssignGroupBox.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.ruleAssignedIGrid)).EndInit();
            this.infoIGridGroupBox.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.infoIGrid)).EndInit();
            this.InfoPanel.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            this.buttonPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox ruleAssignGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionIGrid ruleAssignedIGrid;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid1DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle iGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid1RowTextColCellStyle;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox infoIGridGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionIGrid infoIGrid;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid2DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle iGrid2DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid2RowTextColCellStyle;
        private Trafodion.Manager.Framework.Controls.TrafodionButton serviceInfoButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton ruleInfoButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel InfoPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer splitContainer1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel buttonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton helpButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionBannerControl oneGuiBannerControl1;
    }
}