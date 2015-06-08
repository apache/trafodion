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
using Trafodion.Manager.Framework.Controls;
namespace Trafodion.Manager.OverviewArea.Controls
{
    partial class SystemMessageControl
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
            if (disposing)
            {
                // Remove ourself from the system messageCol changes.
                if (_systemMessage != null)
                {
                    _systemMessage.Change -= _systemMessageChangeHandler;
                }
            }

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
            Trafodion.Manager.Framework.Controls.TrafodionLabel label1;
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(SystemMessageControl));
            this._messageRichTextBox = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this._messageContextMenuStrip = new Trafodion.Manager.Framework.Controls.TrafodionContextMenuStrip(this.components);
            this._cutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this._copyToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this._pasteToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this._undoToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this._redoToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this._headerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._lastUpdatedLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._buttonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._applyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._refreshButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.statusLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            label1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._messageContextMenuStrip.SuspendLayout();
            this._headerPanel.SuspendLayout();
            this._buttonPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            label1.Location = new System.Drawing.Point(4, 4);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(93, 14);
            label1.TabIndex = 0;
            label1.Text = "Last Updated:";
            label1.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // _messageRichTextBox
            // 
            this._messageRichTextBox.ContextMenuStrip = this._messageContextMenuStrip;
            this._messageRichTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._messageRichTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._messageRichTextBox.Location = new System.Drawing.Point(0, 25);
            this._messageRichTextBox.Name = "_messageRichTextBox";
            this._messageRichTextBox.Size = new System.Drawing.Size(789, 396);
            this._messageRichTextBox.TabIndex = 0;
            this._messageRichTextBox.TextChanged += new System.EventHandler(this.OnMessageTextChanged);
            // 
            // _messageContextMenuStrip
            // 
            this._messageContextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._cutToolStripMenuItem,
            this._copyToolStripMenuItem,
            this._pasteToolStripMenuItem,
            this.toolStripSeparator1,
            this._undoToolStripMenuItem,
            this._redoToolStripMenuItem});
            this._messageContextMenuStrip.Name = "_messageContextMenuStrip";
            this._messageContextMenuStrip.Size = new System.Drawing.Size(140, 120);
            this._messageContextMenuStrip.Opening += new System.ComponentModel.CancelEventHandler(this.MessageContextMenuStrip_Opening);
            // 
            // _cutToolStripMenuItem
            // 
            this._cutToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("_cutToolStripMenuItem.Image")));
            this._cutToolStripMenuItem.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._cutToolStripMenuItem.Name = "_cutToolStripMenuItem";
            this._cutToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.X)));
            this._cutToolStripMenuItem.Size = new System.Drawing.Size(139, 22);
            this._cutToolStripMenuItem.Text = "Cu&t";
            this._cutToolStripMenuItem.Click += new System.EventHandler(this.CutToolStripMenuItem_Click);
            // 
            // _copyToolStripMenuItem
            // 
            this._copyToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("_copyToolStripMenuItem.Image")));
            this._copyToolStripMenuItem.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._copyToolStripMenuItem.Name = "_copyToolStripMenuItem";
            this._copyToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.C)));
            this._copyToolStripMenuItem.Size = new System.Drawing.Size(139, 22);
            this._copyToolStripMenuItem.Text = "&Copy";
            this._copyToolStripMenuItem.Click += new System.EventHandler(this.CopyToolStripMenuItem_Click);
            // 
            // _pasteToolStripMenuItem
            // 
            this._pasteToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("_pasteToolStripMenuItem.Image")));
            this._pasteToolStripMenuItem.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._pasteToolStripMenuItem.Name = "_pasteToolStripMenuItem";
            this._pasteToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.V)));
            this._pasteToolStripMenuItem.Size = new System.Drawing.Size(139, 22);
            this._pasteToolStripMenuItem.Text = "&Paste";
            this._pasteToolStripMenuItem.Click += new System.EventHandler(this.PasteToolStripMenuItem_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(136, 6);
            // 
            // _undoToolStripMenuItem
            // 
            this._undoToolStripMenuItem.Name = "_undoToolStripMenuItem";
            this._undoToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Z)));
            this._undoToolStripMenuItem.Size = new System.Drawing.Size(139, 22);
            this._undoToolStripMenuItem.Text = "&Undo";
            this._undoToolStripMenuItem.Click += new System.EventHandler(this.UndoToolStripMenuItem_Click);
            // 
            // _redoToolStripMenuItem
            // 
            this._redoToolStripMenuItem.Name = "_redoToolStripMenuItem";
            this._redoToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Y)));
            this._redoToolStripMenuItem.Size = new System.Drawing.Size(139, 22);
            this._redoToolStripMenuItem.Text = "&Redo";
            this._redoToolStripMenuItem.Click += new System.EventHandler(this.RedoToolStripMenuItem_Click);
            // 
            // _headerPanel
            // 
            this._headerPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._headerPanel.Controls.Add(this._lastUpdatedLabel);
            this._headerPanel.Controls.Add(label1);
            this._headerPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._headerPanel.Location = new System.Drawing.Point(0, 0);
            this._headerPanel.Name = "_headerPanel";
            this._headerPanel.Size = new System.Drawing.Size(789, 25);
            this._headerPanel.TabIndex = 2;
            // 
            // _lastUpdatedLabel
            // 
            this._lastUpdatedLabel.AutoSize = true;
            this._lastUpdatedLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._lastUpdatedLabel.ImageAlign = System.Drawing.ContentAlignment.TopLeft;
            this._lastUpdatedLabel.Location = new System.Drawing.Point(100, 5);
            this._lastUpdatedLabel.Name = "_lastUpdatedLabel";
            this._lastUpdatedLabel.Size = new System.Drawing.Size(94, 13);
            this._lastUpdatedLabel.TabIndex = 1;
            this._lastUpdatedLabel.Text = "<not updated>";
            // 
            // _buttonPanel
            // 
            this._buttonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._buttonPanel.Controls.Add(this._applyButton);
            this._buttonPanel.Controls.Add(this._refreshButton);
            this._buttonPanel.Controls.Add(this.statusLabel);
            this._buttonPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._buttonPanel.Location = new System.Drawing.Point(0, 421);
            this._buttonPanel.Name = "_buttonPanel";
            this._buttonPanel.Size = new System.Drawing.Size(789, 29);
            this._buttonPanel.TabIndex = 1;
            // 
            // _applyButton
            // 
            this._applyButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._applyButton.BackColor = System.Drawing.SystemColors.Control;
            this._applyButton.Enabled = false;
            this._applyButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._applyButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._applyButton.Location = new System.Drawing.Point(630, 3);
            this._applyButton.Name = "_applyButton";
            this._applyButton.Size = new System.Drawing.Size(75, 23);
            this._applyButton.TabIndex = 3;
            this._applyButton.Text = "&Apply";
            this._applyButton.UseVisualStyleBackColor = false;
            this._applyButton.Click += new System.EventHandler(this.OnApplyButtonClick);
            // 
            // _refreshButton
            // 
            this._refreshButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._refreshButton.BackColor = System.Drawing.SystemColors.Control;
            this._refreshButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._refreshButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._refreshButton.Location = new System.Drawing.Point(711, 3);
            this._refreshButton.Name = "_refreshButton";
            this._refreshButton.Size = new System.Drawing.Size(75, 23);
            this._refreshButton.TabIndex = 2;
            this._refreshButton.Text = "&Refresh";
            this._refreshButton.UseVisualStyleBackColor = false;
            this._refreshButton.Click += new System.EventHandler(this.OnRefreshButtonClick);
            // 
            // statusLabel
            // 
            this.statusLabel.AutoSize = true;
            this.statusLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this.statusLabel.ImageAlign = System.Drawing.ContentAlignment.TopLeft;
            this.statusLabel.Location = new System.Drawing.Point(4, 8);
            this.statusLabel.Name = "statusLabel";
            this.statusLabel.Size = new System.Drawing.Size(0, 13);
            this.statusLabel.TabIndex = 1;
            // 
            // SystemMessageControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._messageRichTextBox);
            this.Controls.Add(this._headerPanel);
            this.Controls.Add(this._buttonPanel);
            this.Name = "SystemMessageControl";
            this.Size = new System.Drawing.Size(789, 450);
            this._messageContextMenuStrip.ResumeLayout(false);
            this._headerPanel.ResumeLayout(false);
            this._headerPanel.PerformLayout();
            this._buttonPanel.ResumeLayout(false);
            this._buttonPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionRichTextBox _messageRichTextBox;
        private TrafodionButton _refreshButton;
        private TrafodionButton _applyButton;
        private TrafodionLabel _lastUpdatedLabel;
        private TrafodionPanel _buttonPanel;
        private TrafodionPanel _headerPanel;
        private TrafodionContextMenuStrip _messageContextMenuStrip;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem _pasteToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem _cutToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem _copyToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem _undoToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem _redoToolStripMenuItem;
        private TrafodionLabel statusLabel;
    }
}
