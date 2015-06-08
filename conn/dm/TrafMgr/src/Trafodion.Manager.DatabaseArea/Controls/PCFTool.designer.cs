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
    partial class PCFTool
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
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._statusStrip = new System.Windows.Forms.StatusStrip();
            this._statusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this._progressBar = new System.Windows.Forms.ToolStripProgressBar();
            this._theFileStatus = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._okButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._refreshGuiButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theDownloadButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theRenameButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theDeleteButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theUploadButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._pcfBrowserPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._splitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._codeFilesTree = new Trafodion.Manager.DatabaseArea.Controls.Tree.PCFTreeView();
            this._helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.oneGuiPanel1.SuspendLayout();
            this._statusStrip.SuspendLayout();
            this._pcfBrowserPanel.SuspendLayout();
            this._splitContainer.Panel1.SuspendLayout();
            this._splitContainer.SuspendLayout();
            this.SuspendLayout();
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.oneGuiPanel1.Controls.Add(this._statusStrip);
            this.oneGuiPanel1.Controls.Add(this._theFileStatus);
            this.oneGuiPanel1.Controls.Add(this._helpButton);
            this.oneGuiPanel1.Controls.Add(this._cancelButton);
            this.oneGuiPanel1.Controls.Add(this._okButton);
            this.oneGuiPanel1.Controls.Add(this._refreshGuiButton);
            this.oneGuiPanel1.Controls.Add(this._theDownloadButton);
            this.oneGuiPanel1.Controls.Add(this._theRenameButton);
            this.oneGuiPanel1.Controls.Add(this._theDeleteButton);
            this.oneGuiPanel1.Controls.Add(this._theUploadButton);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 438);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(778, 56);
            this.oneGuiPanel1.TabIndex = 1;
            // 
            // _statusStrip
            // 
            this._statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._statusLabel,
            this._progressBar});
            this._statusStrip.Location = new System.Drawing.Point(0, 32);
            this._statusStrip.Name = "_statusStrip";
            this._statusStrip.Size = new System.Drawing.Size(776, 22);
            this._statusStrip.TabIndex = 5;
            this._statusStrip.Text = "oneGuiStatusStrip1";
            // 
            // _statusLabel
            // 
            this._statusLabel.BorderStyle = System.Windows.Forms.Border3DStyle.SunkenInner;
            this._statusLabel.Name = "_statusLabel";
            this._statusLabel.Size = new System.Drawing.Size(0, 17);
            // 
            // _progressBar
            // 
            this._progressBar.Name = "_progressBar";
            this._progressBar.Size = new System.Drawing.Size(300, 16);
            this._progressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            // 
            // _theFileStatus
            // 
            this._theFileStatus.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theFileStatus.AutoSize = true;
            this._theFileStatus.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theFileStatus.Location = new System.Drawing.Point(336, 33);
            this._theFileStatus.Name = "_theFileStatus";
            this._theFileStatus.Size = new System.Drawing.Size(0, 13);
            this._theFileStatus.TabIndex = 4;
            // 
            // _cancelButton
            // 
            this._cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._cancelButton.Location = new System.Drawing.Point(613, 6);
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Size = new System.Drawing.Size(75, 23);
            this._cancelButton.TabIndex = 3;
            this._cancelButton.Text = "&Cancel";
            this._cancelButton.UseVisualStyleBackColor = true;
            this._cancelButton.Click += new System.EventHandler(this._cancelButton_Click);
            // 
            // _okButton
            // 
            this._okButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._okButton.Enabled = false;
            this._okButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._okButton.Location = new System.Drawing.Point(529, 6);
            this._okButton.Name = "_okButton";
            this._okButton.Size = new System.Drawing.Size(75, 23);
            this._okButton.TabIndex = 3;
            this._okButton.Text = "&OK";
            this._okButton.UseVisualStyleBackColor = true;
            this._okButton.Visible = false;
            this._okButton.Click += new System.EventHandler(this._okButton_Click);
            // 
            // _refreshGuiButton
            // 
            this._refreshGuiButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._refreshGuiButton.Enabled = false;
            this._refreshGuiButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._refreshGuiButton.Location = new System.Drawing.Point(327, 6);
            this._refreshGuiButton.Name = "_refreshGuiButton";
            this._refreshGuiButton.Size = new System.Drawing.Size(75, 23);
            this._refreshGuiButton.TabIndex = 3;
            this._refreshGuiButton.Text = "Re&fresh";
            this._refreshGuiButton.UseVisualStyleBackColor = true;
            this._refreshGuiButton.Click += new System.EventHandler(this._refreshGuiButton_Click);
            // 
            // _theDownloadButton
            // 
            this._theDownloadButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theDownloadButton.Enabled = false;
            this._theDownloadButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDownloadButton.Location = new System.Drawing.Point(246, 6);
            this._theDownloadButton.Name = "_theDownloadButton";
            this._theDownloadButton.Size = new System.Drawing.Size(75, 23);
            this._theDownloadButton.TabIndex = 3;
            this._theDownloadButton.Text = "Dow&nload";
            this._theDownloadButton.UseVisualStyleBackColor = true;
            this._theDownloadButton.Click += new System.EventHandler(this._theDownloadButton_Click);
            // 
            // _theRenameButton
            // 
            this._theRenameButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theRenameButton.Enabled = false;
            this._theRenameButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theRenameButton.Location = new System.Drawing.Point(165, 6);
            this._theRenameButton.Name = "_theRenameButton";
            this._theRenameButton.Size = new System.Drawing.Size(75, 23);
            this._theRenameButton.TabIndex = 2;
            this._theRenameButton.Text = "&Rename";
            this._theRenameButton.UseVisualStyleBackColor = true;
            this._theRenameButton.Click += new System.EventHandler(this._theRenameButton_Click);
            // 
            // _theDeleteButton
            // 
            this._theDeleteButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theDeleteButton.Enabled = false;
            this._theDeleteButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDeleteButton.Location = new System.Drawing.Point(84, 6);
            this._theDeleteButton.Name = "_theDeleteButton";
            this._theDeleteButton.Size = new System.Drawing.Size(75, 23);
            this._theDeleteButton.TabIndex = 1;
            this._theDeleteButton.Text = "&Delete";
            this._theDeleteButton.UseVisualStyleBackColor = true;
            this._theDeleteButton.Click += new System.EventHandler(this._theDeleteButton_Click);
            // 
            // _theUploadButton
            // 
            this._theUploadButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theUploadButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUploadButton.Location = new System.Drawing.Point(3, 6);
            this._theUploadButton.Name = "_theUploadButton";
            this._theUploadButton.Size = new System.Drawing.Size(75, 23);
            this._theUploadButton.TabIndex = 0;
            this._theUploadButton.Text = "&Upload";
            this._theUploadButton.UseVisualStyleBackColor = true;
            this._theUploadButton.Click += new System.EventHandler(this._theUploadButton_Click);
            // 
            // _pcfBrowserPanel
            // 
            this._pcfBrowserPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._pcfBrowserPanel.Controls.Add(this._splitContainer);
            this._pcfBrowserPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._pcfBrowserPanel.Location = new System.Drawing.Point(0, 0);
            this._pcfBrowserPanel.Name = "_pcfBrowserPanel";
            this._pcfBrowserPanel.Size = new System.Drawing.Size(778, 438);
            this._pcfBrowserPanel.TabIndex = 2;
            // 
            // _splitContainer
            // 
            this._splitContainer.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this._splitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._splitContainer.Location = new System.Drawing.Point(0, 0);
            this._splitContainer.Name = "_splitContainer";
            // 
            // _splitContainer.Panel1
            // 
            this._splitContainer.Panel1.Controls.Add(this._codeFilesTree);
            this._splitContainer.Size = new System.Drawing.Size(778, 438);
            this._splitContainer.SplitterDistance = 218;
            this._splitContainer.SplitterWidth = 9;
            this._splitContainer.TabIndex = 2;
            // 
            // _codeFilesTree
            // 
            this._codeFilesTree.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._codeFilesTree.ConnectionDefinition = null;
            this._codeFilesTree.Dock = System.Windows.Forms.DockStyle.Fill;
            this._codeFilesTree.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._codeFilesTree.ImageKey = "BLANK_DOCUMENT_ICON";
            this._codeFilesTree.Location = new System.Drawing.Point(0, 0);
            this._codeFilesTree.Name = "_codeFilesTree";
            this._codeFilesTree.SelectedImageKey = "BLANK_DOCUMENT_ICON";
            this._codeFilesTree.Size = new System.Drawing.Size(216, 436);
            this._codeFilesTree.TabIndex = 0;
            // 
            // _helpButton
            // 
            this._helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._helpButton.Location = new System.Drawing.Point(694, 6);
            this._helpButton.Name = "_helpButton";
            this._helpButton.Size = new System.Drawing.Size(75, 23);
            this._helpButton.TabIndex = 3;
            this._helpButton.Text = "&Help";
            this._helpButton.UseVisualStyleBackColor = true;
            this._helpButton.Click += new System.EventHandler(this._helpButton_Click);
            // 
            // PCFTool
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._pcfBrowserPanel);
            this.Controls.Add(this.oneGuiPanel1);
            this.Name = "PCFTool";
            this.Size = new System.Drawing.Size(778, 494);
            this.Load += new System.EventHandler(this.PCFTool_Load);
            this.oneGuiPanel1.ResumeLayout(false);
            this.oneGuiPanel1.PerformLayout();
            this._statusStrip.ResumeLayout(false);
            this._statusStrip.PerformLayout();
            this._pcfBrowserPanel.ResumeLayout(false);
            this._splitContainer.Panel1.ResumeLayout(false);
            this._splitContainer.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _theFileStatus;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _pcfBrowserPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theDownloadButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theRenameButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theDeleteButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theUploadButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _okButton;
        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer _splitContainer;
        private Trafodion.Manager.DatabaseArea.Controls.Tree.PCFTreeView _codeFilesTree;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _refreshGuiButton;
        private System.Windows.Forms.StatusStrip _statusStrip;
        private System.Windows.Forms.ToolStripStatusLabel _statusLabel;
        private System.Windows.Forms.ToolStripProgressBar _progressBar;
        private Framework.Controls.TrafodionButton _helpButton;
    }
}
