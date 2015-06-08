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
﻿namespace Trafodion.Manager.SecurityArea.Controls
{
    partial class GrantRoleToUsersDialog
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
            this._headerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._roleNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._roleNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._buttonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._grantButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._usersPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._headerPanel.SuspendLayout();
            this._buttonPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _headerPanel
            // 
            this._headerPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._headerPanel.Controls.Add(this._roleNameTextBox);
            this._headerPanel.Controls.Add(this._roleNameLabel);
            this._headerPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._headerPanel.Location = new System.Drawing.Point(0, 0);
            this._headerPanel.Name = "_headerPanel";
            this._headerPanel.Size = new System.Drawing.Size(641, 53);
            this._headerPanel.TabIndex = 0;
            // 
            // _roleNameTextBox
            // 
            this._roleNameTextBox.Enabled = false;
            this._roleNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._roleNameTextBox.Location = new System.Drawing.Point(90, 19);
            this._roleNameTextBox.Name = "_roleNameTextBox";
            this._roleNameTextBox.Size = new System.Drawing.Size(354, 21);
            this._roleNameTextBox.TabIndex = 1;
            // 
            // _roleNameLabel
            // 
            this._roleNameLabel.AutoSize = true;
            this._roleNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._roleNameLabel.Location = new System.Drawing.Point(23, 22);
            this._roleNameLabel.Name = "_roleNameLabel";
            this._roleNameLabel.Size = new System.Drawing.Size(61, 13);
            this._roleNameLabel.TabIndex = 0;
            this._roleNameLabel.Text = "Role Name ";
            // 
            // _buttonPanel
            // 
            this._buttonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._buttonPanel.Controls.Add(this._cancelButton);
            this._buttonPanel.Controls.Add(this._grantButton);
            this._buttonPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._buttonPanel.Location = new System.Drawing.Point(0, 595);
            this._buttonPanel.Name = "_buttonPanel";
            this._buttonPanel.Size = new System.Drawing.Size(641, 41);
            this._buttonPanel.TabIndex = 1;
            // 
            // _cancelButton
            // 
            this._cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._cancelButton.Location = new System.Drawing.Point(336, 10);
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Size = new System.Drawing.Size(75, 23);
            this._cancelButton.TabIndex = 0;
            this._cancelButton.Text = "&Cancel";
            this._cancelButton.UseVisualStyleBackColor = true;
            // 
            // _grantButton
            // 
            this._grantButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._grantButton.Location = new System.Drawing.Point(230, 10);
            this._grantButton.Name = "_grantButton";
            this._grantButton.Size = new System.Drawing.Size(75, 23);
            this._grantButton.TabIndex = 0;
            this._grantButton.Text = "&Grant";
            this._grantButton.UseVisualStyleBackColor = true;
            this._grantButton.Click += new System.EventHandler(this._grantButton_Click);
            // 
            // _usersPanel
            // 
            this._usersPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._usersPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._usersPanel.Location = new System.Drawing.Point(0, 53);
            this._usersPanel.Name = "_usersPanel";
            this._usersPanel.Size = new System.Drawing.Size(641, 542);
            this._usersPanel.TabIndex = 2;
            // 
            // GrantRoleToUsersDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(641, 636);
            this.Controls.Add(this._usersPanel);
            this.Controls.Add(this._buttonPanel);
            this.Controls.Add(this._headerPanel);
            this.Name = "GrantRoleToUsersDialog";
            this.Text = "HP Database Manager - Grant Role";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.GrantRoleToUsersDialog_FormClosing);
            this._headerPanel.ResumeLayout(false);
            this._headerPanel.PerformLayout();
            this._buttonPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _headerPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _roleNameTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _roleNameLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _buttonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _usersPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _grantButton;
    }
}