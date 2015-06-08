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
﻿namespace Trafodion.Manager.UserManagement.Controls
{
    partial class AlterRoleUserControl
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AlterRoleUserControl));
            this._headerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._sqlPrivilegesButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._defaultRoleCountTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._createdTimeTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._createdByTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._grantCountTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._defaultRoleCountLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._createTimeLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._createLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._userGrantCountLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._roleNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._buttonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._resetButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._applyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theSplitContainer = new System.Windows.Forms.SplitContainer();
            this._usersGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._usersGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this.usersToolStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this.addUserToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.deleteUserToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this._theRemoveAllRoleStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this.gbxGrantor = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._grantedByLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._grantedByComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._progressPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._toolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._roleNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theComponentPrivilegesUserControl = new Trafodion.Manager.UserManagement.Controls.ComponentPrivilegesUserControl();
            this._headerPanel.SuspendLayout();
            this._buttonsPanel.SuspendLayout();
            this._theSplitContainer.Panel1.SuspendLayout();
            this._theSplitContainer.Panel2.SuspendLayout();
            this._theSplitContainer.SuspendLayout();
            this._usersGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._usersGrid)).BeginInit();
            this.usersToolStrip.SuspendLayout();
            this.gbxGrantor.SuspendLayout();
            this.SuspendLayout();
            // 
            // _headerPanel
            // 
            this._headerPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._headerPanel.Controls.Add(this._roleNameTextBox);
            this._headerPanel.Controls.Add(this._sqlPrivilegesButton);
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
            this._headerPanel.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._headerPanel.Name = "_headerPanel";
            this._headerPanel.Size = new System.Drawing.Size(1213, 117);
            this._headerPanel.TabIndex = 4;
            // 
            // _sqlPrivilegesButton
            // 
            this._sqlPrivilegesButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sqlPrivilegesButton.Location = new System.Drawing.Point(743, 43);
            this._sqlPrivilegesButton.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._sqlPrivilegesButton.Name = "_sqlPrivilegesButton";
            this._sqlPrivilegesButton.Size = new System.Drawing.Size(181, 27);
            this._sqlPrivilegesButton.TabIndex = 5;
            this._sqlPrivilegesButton.Text = "View SQL &Privileges...";
            this._sqlPrivilegesButton.UseVisualStyleBackColor = true;
            this._sqlPrivilegesButton.Click += new System.EventHandler(this._sqlPrivilegesButton_Click);
            // 
            // _defaultRoleCountTextBox
            // 
            this._defaultRoleCountTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._defaultRoleCountTextBox.Location = new System.Drawing.Point(607, 76);
            this._defaultRoleCountTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._defaultRoleCountTextBox.Name = "_defaultRoleCountTextBox";
            this._defaultRoleCountTextBox.ReadOnly = true;
            this._defaultRoleCountTextBox.Size = new System.Drawing.Size(109, 24);
            this._defaultRoleCountTextBox.TabIndex = 4;
            // 
            // _createdTimeTextBox
            // 
            this._createdTimeTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createdTimeTextBox.Location = new System.Drawing.Point(128, 74);
            this._createdTimeTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._createdTimeTextBox.Name = "_createdTimeTextBox";
            this._createdTimeTextBox.ReadOnly = true;
            this._createdTimeTextBox.Size = new System.Drawing.Size(313, 24);
            this._createdTimeTextBox.TabIndex = 2;
            // 
            // _createdByTextBox
            // 
            this._createdByTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createdByTextBox.Location = new System.Drawing.Point(128, 43);
            this._createdByTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._createdByTextBox.Name = "_createdByTextBox";
            this._createdByTextBox.ReadOnly = true;
            this._createdByTextBox.Size = new System.Drawing.Size(313, 24);
            this._createdByTextBox.TabIndex = 1;
            // 
            // _grantCountTextBox
            // 
            this._grantCountTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._grantCountTextBox.Location = new System.Drawing.Point(607, 45);
            this._grantCountTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._grantCountTextBox.Name = "_grantCountTextBox";
            this._grantCountTextBox.ReadOnly = true;
            this._grantCountTextBox.Size = new System.Drawing.Size(109, 24);
            this._grantCountTextBox.TabIndex = 3;
            // 
            // _defaultRoleCountLabel
            // 
            this._defaultRoleCountLabel.AutoSize = true;
            this._defaultRoleCountLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._defaultRoleCountLabel.Location = new System.Drawing.Point(468, 82);
            this._defaultRoleCountLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._defaultRoleCountLabel.Name = "_defaultRoleCountLabel";
            this._defaultRoleCountLabel.Size = new System.Drawing.Size(127, 17);
            this._defaultRoleCountLabel.TabIndex = 0;
            this._defaultRoleCountLabel.Text = "Primary Role Count";
            // 
            // _createTimeLabel
            // 
            this._createTimeLabel.AutoSize = true;
            this._createTimeLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createTimeLabel.Location = new System.Drawing.Point(17, 78);
            this._createTimeLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._createTimeLabel.Name = "_createTimeLabel";
            this._createTimeLabel.Size = new System.Drawing.Size(92, 17);
            this._createTimeLabel.TabIndex = 0;
            this._createTimeLabel.Text = "Creation Time";
            // 
            // _createLabel
            // 
            this._createLabel.AutoSize = true;
            this._createLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createLabel.Location = new System.Drawing.Point(33, 46);
            this._createLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._createLabel.Name = "_createLabel";
            this._createLabel.Size = new System.Drawing.Size(76, 17);
            this._createLabel.TabIndex = 0;
            this._createLabel.Text = "Created By";
            // 
            // _userGrantCountLabel
            // 
            this._userGrantCountLabel.AutoSize = true;
            this._userGrantCountLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._userGrantCountLabel.Location = new System.Drawing.Point(476, 52);
            this._userGrantCountLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._userGrantCountLabel.Name = "_userGrantCountLabel";
            this._userGrantCountLabel.Size = new System.Drawing.Size(118, 17);
            this._userGrantCountLabel.TabIndex = 0;
            this._userGrantCountLabel.Text = "Total Grant Count";
            // 
            // _roleNameLabel
            // 
            this._roleNameLabel.AutoSize = true;
            this._roleNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._roleNameLabel.Location = new System.Drawing.Point(37, 15);
            this._roleNameLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._roleNameLabel.Name = "_roleNameLabel";
            this._roleNameLabel.Size = new System.Drawing.Size(73, 17);
            this._roleNameLabel.TabIndex = 0;
            this._roleNameLabel.Text = "Role Name";
            // 
            // _buttonsPanel
            // 
            this._buttonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._buttonsPanel.Controls.Add(this._resetButton);
            this._buttonsPanel.Controls.Add(this.helpButton);
            this._buttonsPanel.Controls.Add(this.cancelButton);
            this._buttonsPanel.Controls.Add(this._applyButton);
            this._buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._buttonsPanel.Location = new System.Drawing.Point(0, 482);
            this._buttonsPanel.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._buttonsPanel.Name = "_buttonsPanel";
            this._buttonsPanel.Size = new System.Drawing.Size(1213, 35);
            this._buttonsPanel.TabIndex = 5;
            // 
            // _resetButton
            // 
            this._resetButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._resetButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._resetButton.Location = new System.Drawing.Point(7, 5);
            this._resetButton.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._resetButton.Name = "_resetButton";
            this._resetButton.Size = new System.Drawing.Size(100, 27);
            this._resetButton.TabIndex = 3;
            this._resetButton.Text = "&Reset";
            this._resetButton.UseVisualStyleBackColor = true;
            this._resetButton.Click += new System.EventHandler(this._resetButton_Click);
            // 
            // helpButton
            // 
            this.helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.helpButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.helpButton.Location = new System.Drawing.Point(1109, 5);
            this.helpButton.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = new System.Drawing.Size(100, 27);
            this.helpButton.TabIndex = 8;
            this.helpButton.Text = "He&lp";
            this.helpButton.UseVisualStyleBackColor = true;
            this.helpButton.Click += new System.EventHandler(this.helpButton_Click);
            // 
            // cancelButton
            // 
            this.cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cancelButton.Location = new System.Drawing.Point(1001, 5);
            this.cancelButton.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = new System.Drawing.Size(100, 27);
            this.cancelButton.TabIndex = 7;
            this.cancelButton.Text = "&Cancel";
            this.cancelButton.UseVisualStyleBackColor = true;
            this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
            // 
            // _applyButton
            // 
            this._applyButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._applyButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._applyButton.Location = new System.Drawing.Point(893, 5);
            this._applyButton.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._applyButton.Name = "_applyButton";
            this._applyButton.Size = new System.Drawing.Size(100, 27);
            this._applyButton.TabIndex = 6;
            this._applyButton.Text = "&Apply";
            this._applyButton.UseVisualStyleBackColor = true;
            this._applyButton.Click += new System.EventHandler(this._applyButton_Click);
            // 
            // _theSplitContainer
            // 
            this._theSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theSplitContainer.Location = new System.Drawing.Point(0, 227);
            this._theSplitContainer.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._theSplitContainer.Name = "_theSplitContainer";
            // 
            // _theSplitContainer.Panel1
            // 
            this._theSplitContainer.Panel1.Controls.Add(this._usersGroupBox);
            // 
            // _theSplitContainer.Panel2
            // 
            this._theSplitContainer.Panel2.Controls.Add(this._theComponentPrivilegesUserControl);
            this._theSplitContainer.Panel2.Controls.Add(this.gbxGrantor);
            this._theSplitContainer.Size = new System.Drawing.Size(1213, 255);
            this._theSplitContainer.SplitterDistance = 442;
            this._theSplitContainer.SplitterWidth = 5;
            this._theSplitContainer.TabIndex = 6;
            // 
            // _usersGroupBox
            // 
            this._usersGroupBox.Controls.Add(this._usersGrid);
            this._usersGroupBox.Controls.Add(this.usersToolStrip);
            this._usersGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._usersGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._usersGroupBox.Location = new System.Drawing.Point(0, 0);
            this._usersGroupBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._usersGroupBox.Name = "_usersGroupBox";
            this._usersGroupBox.Padding = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._usersGroupBox.Size = new System.Drawing.Size(442, 255);
            this._usersGroupBox.TabIndex = 5;
            this._usersGroupBox.TabStop = false;
            this._usersGroupBox.Text = "Users with this Role";
            // 
            // _usersGrid
            // 
            this._usersGrid.AllowColumnFilter = true;
            this._usersGrid.AllowWordWrap = false;
            this._usersGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("_usersGrid.AlwaysHiddenColumnNames")));
            this._usersGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this._usersGrid.CurrentFilter = null;
            this._usersGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._usersGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._usersGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._usersGrid.Header.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._usersGrid.Header.Height = 23;
            this._usersGrid.HelpTopic = "";
            this._usersGrid.Location = new System.Drawing.Point(4, 45);
            this._usersGrid.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._usersGrid.Name = "_usersGrid";
            this._usersGrid.ReadOnly = true;
            this._usersGrid.RowMode = true;
            this._usersGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this._usersGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this._usersGrid.SearchAsType.SearchCol = null;
            this._usersGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            this._usersGrid.Size = new System.Drawing.Size(434, 207);
            this._usersGrid.TabIndex = 1;
            this._usersGrid.TreeCol = null;
            this._usersGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this._usersGrid.WordWrap = false;
            // 
            // usersToolStrip
            // 
            this.usersToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.addUserToolStripButton,
            this.deleteUserToolStripButton,
            this.toolStripSeparator1,
            this._theRemoveAllRoleStripButton,
            this.toolStripSeparator4});
            this.usersToolStrip.Location = new System.Drawing.Point(4, 20);
            this.usersToolStrip.Name = "usersToolStrip";
            this.usersToolStrip.Size = new System.Drawing.Size(434, 25);
            this.usersToolStrip.TabIndex = 0;
            this.usersToolStrip.Text = "TrafodionToolStrip2";
            // 
            // addUserToolStripButton
            // 
            this.addUserToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.addUserToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("addUserToolStripButton.Image")));
            this.addUserToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.addUserToolStripButton.Name = "addUserToolStripButton";
            this.addUserToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.addUserToolStripButton.Text = "Add User...";
            this.addUserToolStripButton.Click += new System.EventHandler(this.addUserToolStripButton_Click);
            // 
            // deleteUserToolStripButton
            // 
            this.deleteUserToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.deleteUserToolStripButton.Enabled = false;
            this.deleteUserToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("deleteUserToolStripButton.Image")));
            this.deleteUserToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.deleteUserToolStripButton.Name = "deleteUserToolStripButton";
            this.deleteUserToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.deleteUserToolStripButton.Text = "Remove selected user(s)";
            this.deleteUserToolStripButton.Click += new System.EventHandler(this.deleteUserToolStripButton_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
            // 
            // _theRemoveAllRoleStripButton
            // 
            this._theRemoveAllRoleStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theRemoveAllRoleStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theRemoveAllRoleStripButton.Image")));
            this._theRemoveAllRoleStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theRemoveAllRoleStripButton.Name = "_theRemoveAllRoleStripButton";
            this._theRemoveAllRoleStripButton.Size = new System.Drawing.Size(23, 22);
            this._theRemoveAllRoleStripButton.Text = "Remove All";
            this._theRemoveAllRoleStripButton.ToolTipText = "Remove all users";
            this._theRemoveAllRoleStripButton.Click += new System.EventHandler(this._theRemoveAllRoleStripButton_Click);
            // 
            // toolStripSeparator4
            // 
            this.toolStripSeparator4.Name = "toolStripSeparator4";
            this.toolStripSeparator4.Size = new System.Drawing.Size(6, 25);
            // 
            // gbxGrantor
            // 
            this.gbxGrantor.Controls.Add(this._grantedByLabel);
            this.gbxGrantor.Controls.Add(this._grantedByComboBox);
            this.gbxGrantor.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.gbxGrantor.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.gbxGrantor.Location = new System.Drawing.Point(0, 200);
            this.gbxGrantor.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this.gbxGrantor.Name = "gbxGrantor";
            this.gbxGrantor.Padding = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this.gbxGrantor.Size = new System.Drawing.Size(766, 55);
            this.gbxGrantor.TabIndex = 26;
            this.gbxGrantor.TabStop = false;
            // 
            // _grantedByLabel
            // 
            this._grantedByLabel.AutoSize = true;
            this._grantedByLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this._grantedByLabel.Location = new System.Drawing.Point(8, 21);
            this._grantedByLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._grantedByLabel.Name = "_grantedByLabel";
            this._grantedByLabel.Size = new System.Drawing.Size(86, 17);
            this._grantedByLabel.TabIndex = 6;
            this._grantedByLabel.Text = "Granted By";
            // 
            // _grantedByComboBox
            // 
            this._grantedByComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._grantedByComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._grantedByComboBox.FormattingEnabled = true;
            this._grantedByComboBox.Location = new System.Drawing.Point(135, 17);
            this._grantedByComboBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._grantedByComboBox.Name = "_grantedByComboBox";
            this._grantedByComboBox.Size = new System.Drawing.Size(622, 25);
            this._grantedByComboBox.TabIndex = 5;
            this._grantedByComboBox.TextChanged += new System.EventHandler(this._grantedByComboBox_TextChanged);
            // 
            // _progressPanel
            // 
            this._progressPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._progressPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._progressPanel.Location = new System.Drawing.Point(0, 117);
            this._progressPanel.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._progressPanel.Name = "_progressPanel";
            this._progressPanel.Size = new System.Drawing.Size(1213, 110);
            this._progressPanel.TabIndex = 7;
            // 
            // _toolTip
            // 
            this._toolTip.IsBalloon = true;
            // 
            // _roleNameTextBox
            // 
            this._roleNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._roleNameTextBox.Location = new System.Drawing.Point(128, 8);
            this._roleNameTextBox.Name = "_roleNameTextBox";
            this._roleNameTextBox.ReadOnly = true;
            this._roleNameTextBox.Size = new System.Drawing.Size(796, 24);
            this._roleNameTextBox.TabIndex = 6;
            // 
            // _theComponentPrivilegesUserControl
            // 
            this._theComponentPrivilegesUserControl.ConnectionDefinition = null;
            this._theComponentPrivilegesUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theComponentPrivilegesUserControl.Location = new System.Drawing.Point(0, 0);
            this._theComponentPrivilegesUserControl.Margin = new System.Windows.Forms.Padding(5, 3, 5, 3);
            this._theComponentPrivilegesUserControl.Name = "_theComponentPrivilegesUserControl";
            this._theComponentPrivilegesUserControl.Size = new System.Drawing.Size(766, 200);
            this._theComponentPrivilegesUserControl.TabIndex = 0;
            // 
            // AlterRoleUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theSplitContainer);
            this.Controls.Add(this._progressPanel);
            this.Controls.Add(this._buttonsPanel);
            this.Controls.Add(this._headerPanel);
            this.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this.Name = "AlterRoleUserControl";
            this.Size = new System.Drawing.Size(1213, 517);
            this._headerPanel.ResumeLayout(false);
            this._headerPanel.PerformLayout();
            this._buttonsPanel.ResumeLayout(false);
            this._theSplitContainer.Panel1.ResumeLayout(false);
            this._theSplitContainer.Panel2.ResumeLayout(false);
            this._theSplitContainer.ResumeLayout(false);
            this._usersGroupBox.ResumeLayout(false);
            this._usersGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._usersGrid)).EndInit();
            this.usersToolStrip.ResumeLayout(false);
            this.usersToolStrip.PerformLayout();
            this.gbxGrantor.ResumeLayout(false);
            this.gbxGrantor.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionPanel _headerPanel;
        private Framework.Controls.TrafodionButton _sqlPrivilegesButton;
        private Framework.Controls.TrafodionTextBox _defaultRoleCountTextBox;
        private Framework.Controls.TrafodionTextBox _createdTimeTextBox;
        private Framework.Controls.TrafodionTextBox _createdByTextBox;
        private Framework.Controls.TrafodionTextBox _grantCountTextBox;
        private Framework.Controls.TrafodionLabel _defaultRoleCountLabel;
        private Framework.Controls.TrafodionLabel _createTimeLabel;
        private Framework.Controls.TrafodionLabel _createLabel;
        private Framework.Controls.TrafodionLabel _userGrantCountLabel;
        private Framework.Controls.TrafodionLabel _roleNameLabel;
        private Framework.Controls.TrafodionPanel _buttonsPanel;
        private Framework.Controls.TrafodionButton helpButton;
        private Framework.Controls.TrafodionButton cancelButton;
        private Framework.Controls.TrafodionButton _applyButton;
        private System.Windows.Forms.SplitContainer _theSplitContainer;
        private Framework.Controls.TrafodionGroupBox _usersGroupBox;
        private Framework.Controls.TrafodionIGrid _usersGrid;
        private Framework.Controls.TrafodionToolStrip usersToolStrip;
        private System.Windows.Forms.ToolStripButton addUserToolStripButton;
        private System.Windows.Forms.ToolStripButton deleteUserToolStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripButton _theRemoveAllRoleStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
        private Framework.Controls.TrafodionButton _resetButton;
        private ComponentPrivilegesUserControl _theComponentPrivilegesUserControl;
        private Framework.Controls.TrafodionPanel _progressPanel;
        private Framework.Controls.TrafodionGroupBox gbxGrantor;
        private Framework.Controls.TrafodionLabel _grantedByLabel;
        private Framework.Controls.TrafodionComboBox _grantedByComboBox;
        private Framework.Controls.TrafodionToolTip _toolTip;
        private Framework.Controls.TrafodionTextBox _roleNameTextBox;
    }
}
