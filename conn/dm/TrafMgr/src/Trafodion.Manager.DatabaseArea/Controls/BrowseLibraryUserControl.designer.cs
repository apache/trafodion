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
﻿namespace Trafodion.Manager.DatabaseArea.Controls
{
    partial class BrowseLibraryUserControl
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
            MyDispose(disposing);
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(BrowseLibraryUserControl));
            this._libBrowserPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._splitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._theObjectInfoGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this._theCreateButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theDropButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theDownloadButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theRefreshGuiButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theCloseButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theOKButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theHelpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._statusStrip = new System.Windows.Forms.StatusStrip();
            this._progressBar = new System.Windows.Forms.ToolStripProgressBar();
            this._statusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this._libBrowserPanel.SuspendLayout();
            this._splitContainer.Panel2.SuspendLayout();
            this._splitContainer.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theObjectInfoGrid)).BeginInit();
            this.TrafodionPanel1.SuspendLayout();
            this._statusStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // _libBrowserPanel
            // 
            this._libBrowserPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._libBrowserPanel.Controls.Add(this._splitContainer);
            this._libBrowserPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._libBrowserPanel.Location = new System.Drawing.Point(0, 0);
            this._libBrowserPanel.Name = "_libBrowserPanel";
            this._libBrowserPanel.Size = new System.Drawing.Size(778, 432);
            this._libBrowserPanel.TabIndex = 2;
            // 
            // _splitContainer
            // 
            this._splitContainer.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this._splitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._splitContainer.Location = new System.Drawing.Point(0, 0);
            this._splitContainer.Name = "_splitContainer";
            // 
            // _splitContainer.Panel2
            // 
            this._splitContainer.Panel2.Controls.Add(this._theObjectInfoGrid);
            this._splitContainer.Size = new System.Drawing.Size(778, 432);
            this._splitContainer.SplitterDistance = 218;
            this._splitContainer.SplitterWidth = 9;
            this._splitContainer.TabIndex = 2;
            // 
            // _theObjectInfoGrid
            // 
            this._theObjectInfoGrid.AllowColumnFilter = true;
            this._theObjectInfoGrid.AllowWordWrap = false;
            this._theObjectInfoGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("_theObjectInfoGrid.AlwaysHiddenColumnNames")));
            this._theObjectInfoGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this._theObjectInfoGrid.BorderStyle = TenTec.Windows.iGridLib.iGBorderStyle.Standard;
            this._theObjectInfoGrid.CurrentFilter = null;
            this._theObjectInfoGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theObjectInfoGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theObjectInfoGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._theObjectInfoGrid.Header.Height = 20;
            this._theObjectInfoGrid.HelpTopic = "";
            this._theObjectInfoGrid.Location = new System.Drawing.Point(0, 0);
            this._theObjectInfoGrid.Name = "_theObjectInfoGrid";
            this._theObjectInfoGrid.ReadOnly = true;
            this._theObjectInfoGrid.RowMode = true;
            this._theObjectInfoGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this._theObjectInfoGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this._theObjectInfoGrid.SearchAsType.SearchCol = null;
            this._theObjectInfoGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            this._theObjectInfoGrid.Size = new System.Drawing.Size(549, 430);
            this._theObjectInfoGrid.TabIndex = 2;
            this._theObjectInfoGrid.TreeCol = null;
            this._theObjectInfoGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this._theObjectInfoGrid.WordWrap = false;
            // 
            // _theCreateButton
            // 
            this._theCreateButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theCreateButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCreateButton.Location = new System.Drawing.Point(3, 9);
            this._theCreateButton.Name = "_theCreateButton";
            this._theCreateButton.Size = new System.Drawing.Size(75, 23);
            this._theCreateButton.TabIndex = 0;
            this._theCreateButton.Text = "C&reate";
            this._theCreateButton.UseVisualStyleBackColor = true;
            this._theCreateButton.Click += new System.EventHandler(this._theCreateButton_Click);
            // 
            // _theDropButton
            // 
            this._theDropButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theDropButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDropButton.Location = new System.Drawing.Point(84, 9);
            this._theDropButton.Name = "_theDropButton";
            this._theDropButton.Size = new System.Drawing.Size(75, 23);
            this._theDropButton.TabIndex = 1;
            this._theDropButton.Text = "&Drop";
            this._theDropButton.UseVisualStyleBackColor = true;
            this._theDropButton.Click += new System.EventHandler(this._theDropButton_Click);
            // 
            // _theDownloadButton
            // 
            this._theDownloadButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theDownloadButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDownloadButton.Location = new System.Drawing.Point(165, 9);
            this._theDownloadButton.Name = "_theDownloadButton";
            this._theDownloadButton.Size = new System.Drawing.Size(75, 23);
            this._theDownloadButton.TabIndex = 3;
            this._theDownloadButton.Text = "Dow&nload";
            this._theDownloadButton.UseVisualStyleBackColor = true;
            this._theDownloadButton.Click += new System.EventHandler(this._theDownloadButton_Click);
            // 
            // _theRefreshGuiButton
            // 
            this._theRefreshGuiButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theRefreshGuiButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theRefreshGuiButton.Location = new System.Drawing.Point(246, 9);
            this._theRefreshGuiButton.Name = "_theRefreshGuiButton";
            this._theRefreshGuiButton.Size = new System.Drawing.Size(75, 23);
            this._theRefreshGuiButton.TabIndex = 3;
            this._theRefreshGuiButton.Text = "Re&fresh";
            this._theRefreshGuiButton.UseVisualStyleBackColor = true;
            this._theRefreshGuiButton.Click += new System.EventHandler(this._theRefreshGuiButton_Click);
            // 
            // _theCloseButton
            // 
            this._theCloseButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theCloseButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCloseButton.Location = new System.Drawing.Point(617, 10);
            this._theCloseButton.Name = "_theCloseButton";
            this._theCloseButton.Size = new System.Drawing.Size(75, 23);
            this._theCloseButton.TabIndex = 3;
            this._theCloseButton.Text = "&Close";
            this._theCloseButton.UseVisualStyleBackColor = true;
            this._theCloseButton.Click += new System.EventHandler(this._theCloseButton_Click);
            // 
            // _theOKButton
            // 
            this._theOKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theOKButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theOKButton.Location = new System.Drawing.Point(536, 10);
            this._theOKButton.Name = "_theOKButton";
            this._theOKButton.Size = new System.Drawing.Size(75, 23);
            this._theOKButton.TabIndex = 3;
            this._theOKButton.Text = "&OK";
            this._theOKButton.UseVisualStyleBackColor = true;
            this._theOKButton.Visible = false;
            this._theOKButton.Click += new System.EventHandler(this._theOKButton_Click);
            // 
            // _theHelpButton
            // 
            this._theHelpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theHelpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theHelpButton.Location = new System.Drawing.Point(698, 10);
            this._theHelpButton.Name = "_theHelpButton";
            this._theHelpButton.Size = new System.Drawing.Size(75, 23);
            this._theHelpButton.TabIndex = 3;
            this._theHelpButton.Text = "He&lp";
            this._theHelpButton.UseVisualStyleBackColor = true;
            this._theHelpButton.Click += new System.EventHandler(this._theHelpButton_Click);
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.TrafodionPanel1.Controls.Add(this._statusStrip);
            this.TrafodionPanel1.Controls.Add(this._theHelpButton);
            this.TrafodionPanel1.Controls.Add(this._theOKButton);
            this.TrafodionPanel1.Controls.Add(this._theCloseButton);
            this.TrafodionPanel1.Controls.Add(this._theRefreshGuiButton);
            this.TrafodionPanel1.Controls.Add(this._theDownloadButton);
            this.TrafodionPanel1.Controls.Add(this._theDropButton);
            this.TrafodionPanel1.Controls.Add(this._theCreateButton);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 432);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(778, 62);
            this.TrafodionPanel1.TabIndex = 1;
            // 
            // _statusStrip
            // 
            this._statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._progressBar,
            this._statusLabel});
            this._statusStrip.Location = new System.Drawing.Point(0, 38);
            this._statusStrip.Name = "_statusStrip";
            this._statusStrip.Size = new System.Drawing.Size(776, 22);
            this._statusStrip.TabIndex = 6;
            this._statusStrip.Text = "TrafodionStatusStrip1";
            // 
            // _progressBar
            // 
            this._progressBar.Name = "_progressBar";
            this._progressBar.Size = new System.Drawing.Size(240, 16);
            this._progressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            // 
            // _statusLabel
            // 
            this._statusLabel.BorderStyle = System.Windows.Forms.Border3DStyle.SunkenInner;
            this._statusLabel.Name = "_statusLabel";
            this._statusLabel.Size = new System.Drawing.Size(0, 17);
            // 
            // BrowseLibraryUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._libBrowserPanel);
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "BrowseLibraryUserControl";
            this.Size = new System.Drawing.Size(778, 494);
            this.Load += new System.EventHandler(this.BrowseLibraryUserControl_Load);
            this._libBrowserPanel.ResumeLayout(false);
            this._splitContainer.Panel2.ResumeLayout(false);
            this._splitContainer.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this._theObjectInfoGrid)).EndInit();
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this._statusStrip.ResumeLayout(false);
            this._statusStrip.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _libBrowserPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer _splitContainer;
        private Framework.Controls.TrafodionIGrid _theObjectInfoGrid;
        private Framework.Controls.TrafodionButton _theCreateButton;
        private Framework.Controls.TrafodionButton _theDropButton;
        private Framework.Controls.TrafodionButton _theDownloadButton;
        private Framework.Controls.TrafodionButton _theRefreshGuiButton;
        private Framework.Controls.TrafodionButton _theCloseButton;
        private Framework.Controls.TrafodionButton _theOKButton;
        private Framework.Controls.TrafodionButton _theHelpButton;
        private Framework.Controls.TrafodionPanel TrafodionPanel1;
        private System.Windows.Forms.StatusStrip _statusStrip;
        private System.Windows.Forms.ToolStripStatusLabel _statusLabel;
        private System.Windows.Forms.ToolStripProgressBar _progressBar;
    }
}
