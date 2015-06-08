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
﻿namespace Trafodion.Manager.OverviewArea.Controls
{
    partial class AuditLogUserControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AuditLogUserControl));
            this._theContentPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theTexteventSplitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._theGraphEventSplitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._theTextEventsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theWidgetPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theFilterListPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theFiltertext = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this._theFilterPanel = new Trafodion.Manager.OverviewArea.Controls.AuditLogFilterPanel();
            this.TrafodionToolStrip2 = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this.toolStripLabel2 = new System.Windows.Forms.ToolStripLabel();
            this._theCloseFilterPanelButton = new System.Windows.Forms.ToolStripButton();
            this._theHelpToolStripButton = new System.Windows.Forms.ToolStripButton();
            this._theFilterButtonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theResetButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theApplyFilterButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theFilterToolStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._theShowAuditLogFilterButton = new System.Windows.Forms.ToolStripLabel();
            this._theShowAuditLogFilterArrowButton = new System.Windows.Forms.ToolStripLabel();
            this._theContentPanel.SuspendLayout();
            this._theTexteventSplitContainer.Panel1.SuspendLayout();
            this._theTexteventSplitContainer.Panel2.SuspendLayout();
            this._theTexteventSplitContainer.SuspendLayout();
            this._theGraphEventSplitContainer.Panel2.SuspendLayout();
            this._theGraphEventSplitContainer.SuspendLayout();
            this._theTextEventsPanel.SuspendLayout();
            this._theFilterListPanel.SuspendLayout();
            this.TrafodionToolStrip2.SuspendLayout();
            this._theFilterButtonPanel.SuspendLayout();
            this._theFilterToolStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theContentPanel
            // 
            this._theContentPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theContentPanel.Controls.Add(this._theTexteventSplitContainer);
            this._theContentPanel.Controls.Add(this._theFilterToolStrip);
            this._theContentPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theContentPanel.Location = new System.Drawing.Point(0, 0);
            this._theContentPanel.Name = "_theContentPanel";
            this._theContentPanel.Size = new System.Drawing.Size(1190, 576);
            this._theContentPanel.TabIndex = 3;
            // 
            // _theTexteventSplitContainer
            // 
            this._theTexteventSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTexteventSplitContainer.Location = new System.Drawing.Point(0, 0);
            this._theTexteventSplitContainer.Name = "_theTexteventSplitContainer";
            // 
            // _theTexteventSplitContainer.Panel1
            // 
            this._theTexteventSplitContainer.Panel1.Controls.Add(this._theGraphEventSplitContainer);
            // 
            // _theTexteventSplitContainer.Panel2
            // 
            this._theTexteventSplitContainer.Panel2.AutoScroll = true;
            this._theTexteventSplitContainer.Panel2.Controls.Add(this._theFilterPanel);
            this._theTexteventSplitContainer.Panel2.Controls.Add(this.TrafodionToolStrip2);
            this._theTexteventSplitContainer.Panel2.Controls.Add(this._theFilterButtonPanel);
            this._theTexteventSplitContainer.Size = new System.Drawing.Size(1167, 576);
            this._theTexteventSplitContainer.SplitterDistance = 826;
            this._theTexteventSplitContainer.SplitterWidth = 9;
            this._theTexteventSplitContainer.TabIndex = 0;
            // 
            // _theGraphEventSplitContainer
            // 
            this._theGraphEventSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theGraphEventSplitContainer.Location = new System.Drawing.Point(0, 0);
            this._theGraphEventSplitContainer.Name = "_theGraphEventSplitContainer";
            this._theGraphEventSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // _theGraphEventSplitContainer.Panel2
            // 
            this._theGraphEventSplitContainer.Panel2.Controls.Add(this._theTextEventsPanel);
            this._theGraphEventSplitContainer.Size = new System.Drawing.Size(826, 576);
            this._theGraphEventSplitContainer.SplitterDistance = 117;
            this._theGraphEventSplitContainer.SplitterWidth = 9;
            this._theGraphEventSplitContainer.TabIndex = 3;
            // 
            // _theTextEventsPanel
            // 
            this._theTextEventsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theTextEventsPanel.Controls.Add(this._theWidgetPanel);
            this._theTextEventsPanel.Controls.Add(this._theFilterListPanel);
            this._theTextEventsPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTextEventsPanel.Location = new System.Drawing.Point(0, 0);
            this._theTextEventsPanel.Name = "_theTextEventsPanel";
            this._theTextEventsPanel.Size = new System.Drawing.Size(826, 450);
            this._theTextEventsPanel.TabIndex = 2;
            // 
            // _theWidgetPanel
            // 
            this._theWidgetPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theWidgetPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theWidgetPanel.Location = new System.Drawing.Point(0, 77);
            this._theWidgetPanel.Name = "_theWidgetPanel";
            this._theWidgetPanel.Size = new System.Drawing.Size(826, 373);
            this._theWidgetPanel.TabIndex = 0;
            // 
            // _theFilterListPanel
            // 
            this._theFilterListPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theFilterListPanel.Controls.Add(this._theFiltertext);
            this._theFilterListPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theFilterListPanel.Location = new System.Drawing.Point(0, 0);
            this._theFilterListPanel.Name = "_theFilterListPanel";
            this._theFilterListPanel.Size = new System.Drawing.Size(826, 77);
            this._theFilterListPanel.TabIndex = 1;
            this._theFilterListPanel.Visible = false;
            // 
            // _theFiltertext
            // 
            this._theFiltertext.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._theFiltertext.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theFiltertext.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theFiltertext.Location = new System.Drawing.Point(0, 0);
            this._theFiltertext.Name = "_theFiltertext";
            this._theFiltertext.ReadOnly = true;
            this._theFiltertext.Size = new System.Drawing.Size(826, 77);
            this._theFiltertext.TabIndex = 0;
            this._theFiltertext.Text = "";
            // 
            // _theFilterPanel
            // 
            this._theFilterPanel.AutoScroll = true;
            this._theFilterPanel.AutoSize = true;
            this._theFilterPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._theFilterPanel.ConnectionDefinition = null;
            this._theFilterPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theFilterPanel.FilterModel = null;
            this._theFilterPanel.Location = new System.Drawing.Point(0, 25);
            this._theFilterPanel.Name = "_theFilterPanel";
            this._theFilterPanel.Size = new System.Drawing.Size(332, 520);
            this._theFilterPanel.TabIndex = 2;
            this._theFilterPanel.TheAuditLogDetails = null;
            this._theFilterPanel.OnUpdateButtonsImpl += new Trafodion.Manager.OverviewArea.Controls.AuditLogFilterPanel.OnUpdateButtonsHandle(this._theFilterPanel_OnUpdateButtonsImpl);
            // 
            // TrafodionToolStrip2
            // 
            this.TrafodionToolStrip2.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripLabel2,
            this._theCloseFilterPanelButton,
            this._theHelpToolStripButton});
            this.TrafodionToolStrip2.Location = new System.Drawing.Point(0, 0);
            this.TrafodionToolStrip2.Name = "TrafodionToolStrip2";
            this.TrafodionToolStrip2.Size = new System.Drawing.Size(332, 25);
            this.TrafodionToolStrip2.TabIndex = 1;
            this.TrafodionToolStrip2.Text = "TrafodionToolStrip2";
            // 
            // toolStripLabel2
            // 
            this.toolStripLabel2.Image = ((System.Drawing.Image)(resources.GetObject("toolStripLabel2.Image")));
            this.toolStripLabel2.Name = "toolStripLabel2";
            this.toolStripLabel2.Size = new System.Drawing.Size(104, 22);
            this.toolStripLabel2.Text = "Audit Log Filter";
            // 
            // _theCloseFilterPanelButton
            // 
            this._theCloseFilterPanelButton.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this._theCloseFilterPanelButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theCloseFilterPanelButton.Image = ((System.Drawing.Image)(resources.GetObject("_theCloseFilterPanelButton.Image")));
            this._theCloseFilterPanelButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theCloseFilterPanelButton.Name = "_theCloseFilterPanelButton";
            this._theCloseFilterPanelButton.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this._theCloseFilterPanelButton.Size = new System.Drawing.Size(23, 22);
            this._theCloseFilterPanelButton.ToolTipText = "Close Filter Panel";
            this._theCloseFilterPanelButton.Click += new System.EventHandler(this._theCloseFilterPanelButton_Click);
            // 
            // _theHelpToolStripButton
            // 
            this._theHelpToolStripButton.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this._theHelpToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theHelpToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theHelpToolStripButton.Image")));
            this._theHelpToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theHelpToolStripButton.Name = "_theHelpToolStripButton";
            this._theHelpToolStripButton.Size = new System.Drawing.Size(23, 22);
            this._theHelpToolStripButton.Text = "Help";
            this._theHelpToolStripButton.Click += new System.EventHandler(this._theHelpToolStripButton_Click);
            // 
            // _theFilterButtonPanel
            // 
            this._theFilterButtonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theFilterButtonPanel.Controls.Add(this._theResetButton);
            this._theFilterButtonPanel.Controls.Add(this._theApplyFilterButton);
            this._theFilterButtonPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theFilterButtonPanel.Location = new System.Drawing.Point(0, 545);
            this._theFilterButtonPanel.Name = "_theFilterButtonPanel";
            this._theFilterButtonPanel.Size = new System.Drawing.Size(332, 31);
            this._theFilterButtonPanel.TabIndex = 14;
            // 
            // _theResetButton
            // 
            this._theResetButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theResetButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theResetButton.Location = new System.Drawing.Point(252, 3);
            this._theResetButton.Name = "_theResetButton";
            this._theResetButton.Size = new System.Drawing.Size(75, 23);
            this._theResetButton.TabIndex = 1;
            this._theResetButton.Text = "Reset";
            this._theResetButton.UseVisualStyleBackColor = true;
            this._theResetButton.Click += new System.EventHandler(this._theResetButton_Click);
            // 
            // _theApplyFilterButton
            // 
            this._theApplyFilterButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theApplyFilterButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theApplyFilterButton.Location = new System.Drawing.Point(171, 3);
            this._theApplyFilterButton.Name = "_theApplyFilterButton";
            this._theApplyFilterButton.Size = new System.Drawing.Size(75, 23);
            this._theApplyFilterButton.TabIndex = 0;
            this._theApplyFilterButton.Text = "Apply Filter";
            this._theApplyFilterButton.UseVisualStyleBackColor = true;
            this._theApplyFilterButton.Click += new System.EventHandler(this._theApplyFilterButton_Click);
            // 
            // _theFilterToolStrip
            // 
            this._theFilterToolStrip.AutoSize = false;
            this._theFilterToolStrip.Dock = System.Windows.Forms.DockStyle.Right;
            this._theFilterToolStrip.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this._theFilterToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._theShowAuditLogFilterButton,
            this._theShowAuditLogFilterArrowButton});
            this._theFilterToolStrip.LayoutStyle = System.Windows.Forms.ToolStripLayoutStyle.VerticalStackWithOverflow;
            this._theFilterToolStrip.Location = new System.Drawing.Point(1167, 0);
            this._theFilterToolStrip.Name = "_theFilterToolStrip";
            this._theFilterToolStrip.Size = new System.Drawing.Size(23, 576);
            this._theFilterToolStrip.TabIndex = 1;
            this._theFilterToolStrip.Text = "TrafodionToolStrip1";
            this._theFilterToolStrip.TextDirection = System.Windows.Forms.ToolStripTextDirection.Vertical90;
            // 
            // _theShowAuditLogFilterButton
            // 
            this._theShowAuditLogFilterButton.AutoToolTip = true;
            this._theShowAuditLogFilterButton.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theShowAuditLogFilterButton.Image = ((System.Drawing.Image)(resources.GetObject("_theShowAuditLogFilterButton.Image")));
            this._theShowAuditLogFilterButton.ImageAlign = System.Drawing.ContentAlignment.TopCenter;
            this._theShowAuditLogFilterButton.Name = "_theShowAuditLogFilterButton";
            this._theShowAuditLogFilterButton.Size = new System.Drawing.Size(21, 118);
            this._theShowAuditLogFilterButton.Text = "Audit Log Filter";
            this._theShowAuditLogFilterButton.TextDirection = System.Windows.Forms.ToolStripTextDirection.Vertical90;
            this._theShowAuditLogFilterButton.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageAboveText;
            this._theShowAuditLogFilterButton.Click += new System.EventHandler(this._theShowAuditLogFilterButton_Click);
            // 
            // _theShowAuditLogFilterArrowButton
            // 
            this._theShowAuditLogFilterArrowButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theShowAuditLogFilterArrowButton.Image = ((System.Drawing.Image)(resources.GetObject("_theShowAuditLogFilterArrowButton.Image")));
            this._theShowAuditLogFilterArrowButton.Name = "_theShowAuditLogFilterArrowButton";
            this._theShowAuditLogFilterArrowButton.Size = new System.Drawing.Size(21, 16);
            this._theShowAuditLogFilterArrowButton.Text = "_theShowAuditLogFilterArrowButton";
            this._theShowAuditLogFilterArrowButton.ToolTipText = "Audit Log Filter";
            this._theShowAuditLogFilterArrowButton.Click += new System.EventHandler(this._theShowAuditLogFilterButton_Click);
            // 
            // AuditLogUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theContentPanel);
            this.Name = "AuditLogUserControl";
            this.Size = new System.Drawing.Size(1190, 576);
            this._theContentPanel.ResumeLayout(false);
            this._theTexteventSplitContainer.Panel1.ResumeLayout(false);
            this._theTexteventSplitContainer.Panel2.ResumeLayout(false);
            this._theTexteventSplitContainer.Panel2.PerformLayout();
            this._theTexteventSplitContainer.ResumeLayout(false);
            this._theGraphEventSplitContainer.Panel2.ResumeLayout(false);
            this._theGraphEventSplitContainer.ResumeLayout(false);
            this._theTextEventsPanel.ResumeLayout(false);
            this._theFilterListPanel.ResumeLayout(false);
            this.TrafodionToolStrip2.ResumeLayout(false);
            this.TrafodionToolStrip2.PerformLayout();
            this._theFilterButtonPanel.ResumeLayout(false);
            this._theFilterToolStrip.ResumeLayout(false);
            this._theFilterToolStrip.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionPanel _theContentPanel;
        private Framework.Controls.TrafodionSplitContainer _theTexteventSplitContainer;
        private Framework.Controls.TrafodionSplitContainer _theGraphEventSplitContainer;
        private Framework.Controls.TrafodionPanel _theTextEventsPanel;
        private Framework.Controls.TrafodionPanel _theWidgetPanel;
        private Framework.Controls.TrafodionPanel _theFilterListPanel;
        private Framework.Controls.TrafodionRichTextBox _theFiltertext;
        private Framework.Controls.TrafodionToolStrip _theFilterToolStrip;
        private System.Windows.Forms.ToolStripLabel _theShowAuditLogFilterButton;
        private System.Windows.Forms.ToolStripLabel _theShowAuditLogFilterArrowButton;
        private AuditLogFilterPanel _theFilterPanel;
        private Framework.Controls.TrafodionToolStrip TrafodionToolStrip2;
        private System.Windows.Forms.ToolStripLabel toolStripLabel2;
        private System.Windows.Forms.ToolStripButton _theCloseFilterPanelButton;
        private System.Windows.Forms.ToolStripButton _theHelpToolStripButton;
        private Framework.Controls.TrafodionPanel _theFilterButtonPanel;
        private Framework.Controls.TrafodionButton _theResetButton;
        private Framework.Controls.TrafodionButton _theApplyFilterButton;


    }
}
