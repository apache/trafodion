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
    partial class RoleDetailsDialog
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
            Trafodion.Manager.Framework.TrafodionDateTimeFormatProvider oneGuiDateTimeFormatProvider1 = new Trafodion.Manager.Framework.TrafodionDateTimeFormatProvider();
            this._roleNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._headerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._sqlPrivilegesButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._roleNamesComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._defaultRoleCountTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._createdTimeTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._createdByTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._grantCountTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._defaultRoleCountLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._createTimeLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._createLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._userGrantCountLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._usersPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._usersForRoleControl = new Trafodion.Manager.SecurityArea.Controls.UsersForRoleControl();
            this._headerPanel.SuspendLayout();
            this._usersPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _roleNameLabel
            // 
            this._roleNameLabel.AutoSize = true;
            this._roleNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._roleNameLabel.Location = new System.Drawing.Point(9, 13);
            this._roleNameLabel.Name = "_roleNameLabel";
            this._roleNameLabel.Size = new System.Drawing.Size(58, 13);
            this._roleNameLabel.TabIndex = 0;
            this._roleNameLabel.Text = "Role Name";
            // 
            // _headerPanel
            // 
            this._headerPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._headerPanel.Controls.Add(this._sqlPrivilegesButton);
            this._headerPanel.Controls.Add(this._roleNamesComboBox);
            this._headerPanel.Controls.Add(this._defaultRoleCountTextBox);
            this._headerPanel.Controls.Add(this._createdTimeTextBox);
            this._headerPanel.Controls.Add(this._createdByTextBox);
            this._headerPanel.Controls.Add(this._grantCountTextBox);
            this._headerPanel.Controls.Add(this._defaultRoleCountLabel);
            this._headerPanel.Controls.Add(this._createTimeLabel);
            this._headerPanel.Controls.Add(this._createLabel);
            this._headerPanel.Controls.Add(this._userGrantCountLabel);
            this._headerPanel.Controls.Add(this._roleNameLabel);
            this._headerPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._headerPanel.Location = new System.Drawing.Point(0, 0);
            this._headerPanel.Name = "_headerPanel";
            this._headerPanel.Size = new System.Drawing.Size(840, 101);
            this._headerPanel.TabIndex = 1;
            // 
            // _sqlPrivilegesButton
            // 
            this._sqlPrivilegesButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sqlPrivilegesButton.Location = new System.Drawing.Point(557, 64);
            this._sqlPrivilegesButton.Name = "_sqlPrivilegesButton";
            this._sqlPrivilegesButton.Size = new System.Drawing.Size(136, 23);
            this._sqlPrivilegesButton.TabIndex = 6;
            this._sqlPrivilegesButton.Text = "View SQL &Privileges...";
            this._sqlPrivilegesButton.UseVisualStyleBackColor = true;
            this._sqlPrivilegesButton.Click += new System.EventHandler(this._sqlPrivilegesButton_Click);
            // 
            // _roleNamesComboBox
            // 
            this._roleNamesComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._roleNamesComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._roleNamesComboBox.FormattingEnabled = true;
            this._roleNamesComboBox.Location = new System.Drawing.Point(83, 10);
            this._roleNamesComboBox.Name = "_roleNamesComboBox";
            this._roleNamesComboBox.Size = new System.Drawing.Size(454, 21);
            this._roleNamesComboBox.TabIndex = 5;
            // 
            // _defaultRoleCountTextBox
            // 
            this._defaultRoleCountTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._defaultRoleCountTextBox.Location = new System.Drawing.Point(455, 66);
            this._defaultRoleCountTextBox.Name = "_defaultRoleCountTextBox";
            this._defaultRoleCountTextBox.ReadOnly = true;
            this._defaultRoleCountTextBox.Size = new System.Drawing.Size(83, 21);
            this._defaultRoleCountTextBox.TabIndex = 4;
            // 
            // _createdTimeTextBox
            // 
            this._createdTimeTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createdTimeTextBox.Location = new System.Drawing.Point(83, 64);
            this._createdTimeTextBox.Name = "_createdTimeTextBox";
            this._createdTimeTextBox.ReadOnly = true;
            this._createdTimeTextBox.Size = new System.Drawing.Size(249, 21);
            this._createdTimeTextBox.TabIndex = 2;
            // 
            // _createdByTextBox
            // 
            this._createdByTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createdByTextBox.Location = new System.Drawing.Point(83, 37);
            this._createdByTextBox.Name = "_createdByTextBox";
            this._createdByTextBox.ReadOnly = true;
            this._createdByTextBox.Size = new System.Drawing.Size(249, 21);
            this._createdByTextBox.TabIndex = 1;
            // 
            // _grantCountTextBox
            // 
            this._grantCountTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._grantCountTextBox.Location = new System.Drawing.Point(455, 39);
            this._grantCountTextBox.Name = "_grantCountTextBox";
            this._grantCountTextBox.ReadOnly = true;
            this._grantCountTextBox.Size = new System.Drawing.Size(83, 21);
            this._grantCountTextBox.TabIndex = 3;
            // 
            // _defaultRoleCountLabel
            // 
            this._defaultRoleCountLabel.AutoSize = true;
            this._defaultRoleCountLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._defaultRoleCountLabel.Location = new System.Drawing.Point(351, 71);
            this._defaultRoleCountLabel.Name = "_defaultRoleCountLabel";
            this._defaultRoleCountLabel.Size = new System.Drawing.Size(98, 13);
            this._defaultRoleCountLabel.TabIndex = 0;
            this._defaultRoleCountLabel.Text = "Default Role Count";
            // 
            // _createTimeLabel
            // 
            this._createTimeLabel.AutoSize = true;
            this._createTimeLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createTimeLabel.Location = new System.Drawing.Point(9, 68);
            this._createTimeLabel.Name = "_createTimeLabel";
            this._createTimeLabel.Size = new System.Drawing.Size(65, 13);
            this._createTimeLabel.TabIndex = 0;
            this._createTimeLabel.Text = "Create Time";
            // 
            // _createLabel
            // 
            this._createLabel.AutoSize = true;
            this._createLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createLabel.Location = new System.Drawing.Point(9, 40);
            this._createLabel.Name = "_createLabel";
            this._createLabel.Size = new System.Drawing.Size(61, 13);
            this._createLabel.TabIndex = 0;
            this._createLabel.Text = "Created By";
            // 
            // _userGrantCountLabel
            // 
            this._userGrantCountLabel.AutoSize = true;
            this._userGrantCountLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._userGrantCountLabel.Location = new System.Drawing.Point(351, 45);
            this._userGrantCountLabel.Name = "_userGrantCountLabel";
            this._userGrantCountLabel.Size = new System.Drawing.Size(93, 13);
            this._userGrantCountLabel.TabIndex = 0;
            this._userGrantCountLabel.Text = "Total Grant Count";
            // 
            // _usersPanel
            // 
            this._usersPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._usersPanel.Controls.Add(this._usersForRoleControl);
            this._usersPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._usersPanel.Location = new System.Drawing.Point(0, 101);
            this._usersPanel.Name = "_usersPanel";
            this._usersPanel.Size = new System.Drawing.Size(840, 400);
            this._usersPanel.TabIndex = 2;
            // 
            // _usersForRoleControl
            // 
            this._usersForRoleControl.BackColor = System.Drawing.Color.WhiteSmoke;
            this._usersForRoleControl.ConnectionDefn = null;
            this._usersForRoleControl.DataDisplayHandler = null;
            this._usersForRoleControl.DataProvider = null;
            this._usersForRoleControl.DateFormatProvider = oneGuiDateTimeFormatProvider1;
            this._usersForRoleControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._usersForRoleControl.DrillDownManager = null;
            this._usersForRoleControl.Location = new System.Drawing.Point(0, 0);
            this._usersForRoleControl.Name = "_usersForRoleControl";
            this._usersForRoleControl.Size = new System.Drawing.Size(840, 400);
            this._usersForRoleControl.TabIndex = 0;
            this._usersForRoleControl.UniversalWidgetConfiguration = null;
            // 
            // RoleDetailsDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(840, 501);
            this.Controls.Add(this._usersPanel);
            this.Controls.Add(this._headerPanel);
            this.Name = "RoleDetailsDialog";
            this.Text = "HP Database Manager - Role Details";
            this._headerPanel.ResumeLayout(false);
            this._headerPanel.PerformLayout();
            this._usersPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionLabel _roleNameLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _headerPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _usersPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _defaultRoleCountTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _grantCountTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _defaultRoleCountLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _userGrantCountLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _createdTimeTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _createdByTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _createTimeLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _createLabel;
        private UsersForRoleControl _usersForRoleControl;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _roleNamesComboBox;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _sqlPrivilegesButton;
    }
}