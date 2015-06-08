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
﻿namespace Trafodion.Manager.UniversalWidget.Controls
{
    partial class ChartDesignerUserControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ChartDesignerUserControl));
            this._theSplitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theChartsTreeView = new Trafodion.Manager.Framework.Controls.TrafodionTreeView();
            this.TrafodionToolStrip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._theAddToolStripButton = new System.Windows.Forms.ToolStripButton();
            this._theDeleteToolStripButton = new System.Windows.Forms.ToolStripButton();
            this._theTablePanel = new System.Windows.Forms.TableLayoutPanel();
            this._theAttributePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theStatusTextBox = new Trafodion.Manager.Framework.Controls.TrafodionMessagePanel();
            this._theSplitContainer.Panel1.SuspendLayout();
            this._theSplitContainer.Panel2.SuspendLayout();
            this._theSplitContainer.SuspendLayout();
            this.TrafodionGroupBox1.SuspendLayout();
            this.TrafodionToolStrip1.SuspendLayout();
            this._theTablePanel.SuspendLayout();
            this.TrafodionPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theSplitContainer
            // 
            this._theSplitContainer.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theSplitContainer.Location = new System.Drawing.Point(0, 25);
            this._theSplitContainer.Name = "_theSplitContainer";
            // 
            // _theSplitContainer.Panel1
            // 
            this._theSplitContainer.Panel1.Controls.Add(this.TrafodionGroupBox1);
            // 
            // _theSplitContainer.Panel2
            // 
            this._theSplitContainer.Panel2.AutoScroll = true;
            this._theSplitContainer.Panel2.Controls.Add(this._theTablePanel);
            this._theSplitContainer.Size = new System.Drawing.Size(801, 297);
            this._theSplitContainer.SplitterDistance = 265;
            this._theSplitContainer.SplitterWidth = 9;
            this._theSplitContainer.TabIndex = 1;
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.Controls.Add(this._theChartsTreeView);
            this.TrafodionGroupBox1.Controls.Add(this.TrafodionToolStrip1);
            this.TrafodionGroupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(265, 297);
            this.TrafodionGroupBox1.TabIndex = 0;
            this.TrafodionGroupBox1.TabStop = false;
            this.TrafodionGroupBox1.Text = "Chart Tree";
            // 
            // _theChartsTreeView
            // 
            this._theChartsTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theChartsTreeView.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theChartsTreeView.ImageKey = "BLANK_DOCUMENT_ICON";
            this._theChartsTreeView.Location = new System.Drawing.Point(3, 42);
            this._theChartsTreeView.Name = "_theChartsTreeView";
            this._theChartsTreeView.SelectedImageKey = "BLANK_DOCUMENT_ICON";
            this._theChartsTreeView.Size = new System.Drawing.Size(259, 252);
            this._theChartsTreeView.TabIndex = 4;
            this._theChartsTreeView.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this._theChartsTreeView_AfterSelect);
            // 
            // TrafodionToolStrip1
            // 
            this.TrafodionToolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._theAddToolStripButton,
            this._theDeleteToolStripButton});
            this.TrafodionToolStrip1.Location = new System.Drawing.Point(3, 17);
            this.TrafodionToolStrip1.Name = "TrafodionToolStrip1";
            this.TrafodionToolStrip1.Size = new System.Drawing.Size(259, 25);
            this.TrafodionToolStrip1.TabIndex = 3;
            this.TrafodionToolStrip1.Text = "TrafodionToolStrip1";
            // 
            // _theAddToolStripButton
            // 
            this._theAddToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theAddToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theAddToolStripButton.Image")));
            this._theAddToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theAddToolStripButton.Name = "_theAddToolStripButton";
            this._theAddToolStripButton.Size = new System.Drawing.Size(23, 22);
            this._theAddToolStripButton.Text = "Add a new chart element";
            this._theAddToolStripButton.Click += new System.EventHandler(this._theAddToolStripButton_Click);
            // 
            // _theDeleteToolStripButton
            // 
            this._theDeleteToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theDeleteToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theDeleteToolStripButton.Image")));
            this._theDeleteToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theDeleteToolStripButton.Name = "_theDeleteToolStripButton";
            this._theDeleteToolStripButton.Size = new System.Drawing.Size(23, 22);
            this._theDeleteToolStripButton.Text = "Remove the selected chart element";
            this._theDeleteToolStripButton.Click += new System.EventHandler(this.theDeleteToolStripButton_Click);
            // 
            // _theTablePanel
            // 
            this._theTablePanel.ColumnCount = 1;
            this._theTablePanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this._theTablePanel.Controls.Add(this._theAttributePanel, 0, 0);
            this._theTablePanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTablePanel.Location = new System.Drawing.Point(0, 0);
            this._theTablePanel.Name = "_theTablePanel";
            this._theTablePanel.RowCount = 1;
            this._theTablePanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 90.43928F));
            this._theTablePanel.Size = new System.Drawing.Size(527, 297);
            this._theTablePanel.TabIndex = 1;
            // 
            // _theAttributePanel
            // 
            this._theAttributePanel.AutoScroll = true;
            this._theAttributePanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAttributePanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAttributePanel.Location = new System.Drawing.Point(3, 3);
            this._theAttributePanel.Name = "_theAttributePanel";
            this._theAttributePanel.Size = new System.Drawing.Size(521, 291);
            this._theAttributePanel.TabIndex = 0;
            // 
            // TrafodionPanel
            // 
            this.TrafodionPanel.AutoScroll = true;
            this.TrafodionPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel.Controls.Add(this._theStatusTextBox);
            this.TrafodionPanel.Controls.Add(this._theSplitContainer);
            this.TrafodionPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel.Name = "TrafodionPanel";
            this.TrafodionPanel.Size = new System.Drawing.Size(801, 322);
            this.TrafodionPanel.TabIndex = 2;
            // 
            // _theStatusTextBox
            // 
            this._theStatusTextBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._theStatusTextBox.Location = new System.Drawing.Point(0, 0);
            this._theStatusTextBox.Name = "_theStatusTextBox";
            this._theStatusTextBox.Size = new System.Drawing.Size(801, 24);
            this._theStatusTextBox.TabIndex = 3;
            // 
            // ChartDesignerUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionPanel);
            this.Name = "ChartDesignerUserControl";
            this.Size = new System.Drawing.Size(801, 322);
            this._theSplitContainer.Panel1.ResumeLayout(false);
            this._theSplitContainer.Panel2.ResumeLayout(false);
            this._theSplitContainer.ResumeLayout(false);
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.TrafodionGroupBox1.PerformLayout();
            this.TrafodionToolStrip1.ResumeLayout(false);
            this.TrafodionToolStrip1.PerformLayout();
            this._theTablePanel.ResumeLayout(false);
            this.TrafodionPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionSplitContainer _theSplitContainer;
        private Framework.Controls.TrafodionGroupBox TrafodionGroupBox1;
        private Framework.Controls.TrafodionToolStrip TrafodionToolStrip1;
        private System.Windows.Forms.ToolStripButton _theAddToolStripButton;
        private System.Windows.Forms.ToolStripButton _theDeleteToolStripButton;
        private System.Windows.Forms.TableLayoutPanel _theTablePanel;
        private Framework.Controls.TrafodionPanel _theAttributePanel;
        private Framework.Controls.TrafodionPanel TrafodionPanel;
        private Framework.Controls.TrafodionTreeView _theChartsTreeView;
        private Framework.Controls.TrafodionMessagePanel _theStatusTextBox;



    }
}
