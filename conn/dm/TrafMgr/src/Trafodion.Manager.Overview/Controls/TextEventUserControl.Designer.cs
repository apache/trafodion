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
    partial class TextEventUserControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TextEventUserControl));
            this._theTexteventSplitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._theGraphEventSplitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._theTextEventsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theWidgetPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theFilterListPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theFiltertext = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this._theFilterPanel = new Trafodion.Manager.OverviewArea.Controls.AMQPEventsFilterPanel();
            this.TrafodionToolStrip2 = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this.toolStripLabel2 = new System.Windows.Forms.ToolStripLabel();
            this._theCloseFilterPanelButton = new System.Windows.Forms.ToolStripButton();
            this._theHelpToolStripButton = new System.Windows.Forms.ToolStripButton();
            this._theFilterToolStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._theShowEventFilterButton = new System.Windows.Forms.ToolStripLabel();
            this._theShowEventFilterArrowButton = new System.Windows.Forms.ToolStripLabel();
            this._theContentPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theProgressPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theTexteventSplitContainer.Panel1.SuspendLayout();
            this._theTexteventSplitContainer.Panel2.SuspendLayout();
            this._theTexteventSplitContainer.SuspendLayout();
            this._theGraphEventSplitContainer.Panel2.SuspendLayout();
            this._theGraphEventSplitContainer.SuspendLayout();
            this._theTextEventsPanel.SuspendLayout();
            this._theFilterListPanel.SuspendLayout();
            this.TrafodionToolStrip2.SuspendLayout();
            this._theFilterToolStrip.SuspendLayout();
            this._theContentPanel.SuspendLayout();
            this.SuspendLayout();
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
            this._theTexteventSplitContainer.Panel2.Controls.Add(this._theFilterPanel);
            this._theTexteventSplitContainer.Panel2.Controls.Add(this.TrafodionToolStrip2);
            this._theTexteventSplitContainer.Size = new System.Drawing.Size(811, 530);
            this._theTexteventSplitContainer.SplitterDistance = 512;
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
            this._theGraphEventSplitContainer.Size = new System.Drawing.Size(512, 530);
            this._theGraphEventSplitContainer.SplitterDistance = 110;
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
            this._theTextEventsPanel.Size = new System.Drawing.Size(512, 411);
            this._theTextEventsPanel.TabIndex = 2;
            // 
            // _theWidgetPanel
            // 
            this._theWidgetPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theWidgetPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theWidgetPanel.Location = new System.Drawing.Point(0, 77);
            this._theWidgetPanel.Name = "_theWidgetPanel";
            this._theWidgetPanel.Size = new System.Drawing.Size(512, 334);
            this._theWidgetPanel.TabIndex = 0;
            // 
            // _theFilterListPanel
            // 
            this._theFilterListPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theFilterListPanel.Controls.Add(this._theFiltertext);
            this._theFilterListPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theFilterListPanel.Location = new System.Drawing.Point(0, 0);
            this._theFilterListPanel.Name = "_theFilterListPanel";
            this._theFilterListPanel.Size = new System.Drawing.Size(512, 77);
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
            this._theFiltertext.Size = new System.Drawing.Size(512, 77);
            this._theFiltertext.TabIndex = 0;
            this._theFiltertext.Text = "";
            // 
            // _theFilterPanel
            // 
            this._theFilterPanel.AutoScroll = true;
            this._theFilterPanel.AutoScrollMinSize = new System.Drawing.Size(340, 670);
            this._theFilterPanel.ConnectionDefinition = null;
            this._theFilterPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theFilterPanel.FilterModel = null;
            this._theFilterPanel.ForLiveEvents = false;
            this._theFilterPanel.Location = new System.Drawing.Point(0, 25);
            this._theFilterPanel.Name = "_theFilterPanel";
            this._theFilterPanel.Size = new System.Drawing.Size(290, 505);
            this._theFilterPanel.TabIndex = 0;
            this._theFilterPanel.TheEventDetails = null;
            // 
            // TrafodionToolStrip2
            // 
            this.TrafodionToolStrip2.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripLabel2,
            this._theCloseFilterPanelButton,
            this._theHelpToolStripButton});
            this.TrafodionToolStrip2.Location = new System.Drawing.Point(0, 0);
            this.TrafodionToolStrip2.Name = "TrafodionToolStrip2";
            this.TrafodionToolStrip2.Size = new System.Drawing.Size(290, 25);
            this.TrafodionToolStrip2.TabIndex = 1;
            this.TrafodionToolStrip2.Text = "TrafodionToolStrip2";
            // 
            // toolStripLabel2
            // 
            this.toolStripLabel2.Image = ((System.Drawing.Image)(resources.GetObject("toolStripLabel2.Image")));
            this.toolStripLabel2.Name = "toolStripLabel2";
            this.toolStripLabel2.Size = new System.Drawing.Size(81, 22);
            this.toolStripLabel2.Text = "Event Filter";
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
            // _theFilterToolStrip
            // 
            this._theFilterToolStrip.AutoSize = false;
            this._theFilterToolStrip.Dock = System.Windows.Forms.DockStyle.Right;
            this._theFilterToolStrip.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this._theFilterToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._theShowEventFilterButton,
            this._theShowEventFilterArrowButton});
            this._theFilterToolStrip.LayoutStyle = System.Windows.Forms.ToolStripLayoutStyle.VerticalStackWithOverflow;
            this._theFilterToolStrip.Location = new System.Drawing.Point(811, 0);
            this._theFilterToolStrip.Name = "_theFilterToolStrip";
            this._theFilterToolStrip.Size = new System.Drawing.Size(23, 530);
            this._theFilterToolStrip.TabIndex = 1;
            this._theFilterToolStrip.Text = "TrafodionToolStrip1";
            this._theFilterToolStrip.TextDirection = System.Windows.Forms.ToolStripTextDirection.Vertical90;
            // 
            // _theShowEventFilterButton
            // 
            this._theShowEventFilterButton.AutoToolTip = true;
            this._theShowEventFilterButton.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theShowEventFilterButton.Image = ((System.Drawing.Image)(resources.GetObject("_theShowEventFilterButton.Image")));
            this._theShowEventFilterButton.ImageAlign = System.Drawing.ContentAlignment.TopCenter;
            this._theShowEventFilterButton.Name = "_theShowEventFilterButton";
            this._theShowEventFilterButton.Size = new System.Drawing.Size(21, 92);
            this._theShowEventFilterButton.Text = "Event Filter";
            this._theShowEventFilterButton.TextDirection = System.Windows.Forms.ToolStripTextDirection.Vertical90;
            this._theShowEventFilterButton.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageAboveText;
            this._theShowEventFilterButton.Click += new System.EventHandler(this._theShowEventFilterButton_Click);
            // 
            // _theShowEventFilterArrowButton
            // 
            this._theShowEventFilterArrowButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theShowEventFilterArrowButton.Image = ((System.Drawing.Image)(resources.GetObject("_theShowEventFilterArrowButton.Image")));
            this._theShowEventFilterArrowButton.Name = "_theShowEventFilterArrowButton";
            this._theShowEventFilterArrowButton.Size = new System.Drawing.Size(21, 16);
            this._theShowEventFilterArrowButton.Text = "_theShowEventFilterArrowButton";
            this._theShowEventFilterArrowButton.ToolTipText = "Event Filter";
            this._theShowEventFilterArrowButton.Click += new System.EventHandler(this._theShowEventFilterButton_Click);
            // 
            // _theContentPanel
            // 
            this._theContentPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theContentPanel.Controls.Add(this._theTexteventSplitContainer);
            this._theContentPanel.Controls.Add(this._theFilterToolStrip);
            this._theContentPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theContentPanel.Location = new System.Drawing.Point(0, 104);
            this._theContentPanel.Name = "_theContentPanel";
            this._theContentPanel.Size = new System.Drawing.Size(834, 530);
            this._theContentPanel.TabIndex = 0;
            // 
            // _theProgressPanel
            // 
            this._theProgressPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theProgressPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theProgressPanel.Location = new System.Drawing.Point(0, 0);
            this._theProgressPanel.Name = "_theProgressPanel";
            this._theProgressPanel.Size = new System.Drawing.Size(834, 104);
            this._theProgressPanel.TabIndex = 1;
            // 
            // TextEventUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theContentPanel);
            this.Controls.Add(this._theProgressPanel);
            this.Name = "TextEventUserControl";
            this.Size = new System.Drawing.Size(834, 634);
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
            this._theFilterToolStrip.ResumeLayout(false);
            this._theFilterToolStrip.PerformLayout();
            this._theContentPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer _theTexteventSplitContainer;
        private AMQPEventsFilterPanel _theFilterPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theWidgetPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theFilterListPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionRichTextBox _theFiltertext;
        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer _theGraphEventSplitContainer;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theTextEventsPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStrip _theFilterToolStrip;
        private System.Windows.Forms.ToolStripLabel _theShowEventFilterButton;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStrip TrafodionToolStrip2;
        private System.Windows.Forms.ToolStripLabel toolStripLabel2;
        private System.Windows.Forms.ToolStripButton _theCloseFilterPanelButton;
        private System.Windows.Forms.ToolStripButton _theHelpToolStripButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theContentPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theProgressPanel;
        private System.Windows.Forms.ToolStripLabel _theShowEventFilterArrowButton;

    }
}
