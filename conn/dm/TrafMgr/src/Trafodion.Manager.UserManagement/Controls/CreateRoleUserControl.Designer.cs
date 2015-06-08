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
    partial class CreateRoleUserControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(CreateRoleUserControl));
            this._roleTrafodionGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.theRolesMappingGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this.TrafodionPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionToolStripRowAction = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._theAddRoleStripButton = new System.Windows.Forms.ToolStripButton();
            this._theDeleteRoleStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this._theRemoveAllStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.toolStripLabel1 = new System.Windows.Forms.ToolStripLabel();
            this._theCQSettingStats = new System.Windows.Forms.ToolStripLabel();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._okButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.errorLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._splitContainerCreateRole = new System.Windows.Forms.SplitContainer();
            this._thesplitContainerUserCom = new System.Windows.Forms.SplitContainer();
            this._userGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theUsersGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this.compPrivilegesToolStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this.addUserToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.RemoveUserToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this._theRemoveAllUsersStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this._theComponentPrivilegesUserControl = new Trafodion.Manager.UserManagement.Controls.ComponentPrivilegesUserControl();
            this.gbxGrantor = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._grantedByLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._grantedByComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._toolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._roleTrafodionGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.theRolesMappingGrid)).BeginInit();
            this.TrafodionPanel2.SuspendLayout();
            this.TrafodionToolStripRowAction.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this._splitContainerCreateRole.Panel1.SuspendLayout();
            this._splitContainerCreateRole.Panel2.SuspendLayout();
            this._splitContainerCreateRole.SuspendLayout();
            this._thesplitContainerUserCom.Panel1.SuspendLayout();
            this._thesplitContainerUserCom.Panel2.SuspendLayout();
            this._thesplitContainerUserCom.SuspendLayout();
            this._userGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theUsersGrid)).BeginInit();
            this.compPrivilegesToolStrip.SuspendLayout();
            this.gbxGrantor.SuspendLayout();
            this.SuspendLayout();
            // 
            // _roleTrafodionGroupBox
            // 
            this._roleTrafodionGroupBox.Controls.Add(this.theRolesMappingGrid);
            this._roleTrafodionGroupBox.Controls.Add(this.TrafodionPanel2);
            this._roleTrafodionGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._roleTrafodionGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this._roleTrafodionGroupBox.Location = new System.Drawing.Point(0, 0);
            this._roleTrafodionGroupBox.Name = "_roleTrafodionGroupBox";
            this._roleTrafodionGroupBox.Size = new System.Drawing.Size(910, 161);
            this._roleTrafodionGroupBox.TabIndex = 22;
            this._roleTrafodionGroupBox.TabStop = false;
            this._roleTrafodionGroupBox.Text = "Create Roles";
            // 
            // theRolesMappingGrid
            // 
            this.theRolesMappingGrid.AllowColumnFilter = true;
            this.theRolesMappingGrid.AllowWordWrap = false;
            this.theRolesMappingGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("theRolesMappingGrid.AlwaysHiddenColumnNames")));
            this.theRolesMappingGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this.theRolesMappingGrid.CurrentFilter = null;
            this.theRolesMappingGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theRolesMappingGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.theRolesMappingGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this.theRolesMappingGrid.Header.Height = 20;
            this.theRolesMappingGrid.HelpTopic = "";
            this.theRolesMappingGrid.Location = new System.Drawing.Point(3, 42);
            this.theRolesMappingGrid.Name = "theRolesMappingGrid";
            this.theRolesMappingGrid.ReadOnly = true;
            this.theRolesMappingGrid.RowMode = true;
            this.theRolesMappingGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this.theRolesMappingGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this.theRolesMappingGrid.SearchAsType.SearchCol = null;
            this.theRolesMappingGrid.Size = new System.Drawing.Size(904, 116);
            this.theRolesMappingGrid.TabIndex = 18;
            this.theRolesMappingGrid.TreeCol = null;
            this.theRolesMappingGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.theRolesMappingGrid.WordWrap = false;
            // 
            // TrafodionPanel2
            // 
            this.TrafodionPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel2.Controls.Add(this.TrafodionToolStripRowAction);
            this.TrafodionPanel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionPanel2.Location = new System.Drawing.Point(3, 17);
            this.TrafodionPanel2.Name = "TrafodionPanel2";
            this.TrafodionPanel2.Size = new System.Drawing.Size(904, 25);
            this.TrafodionPanel2.TabIndex = 21;
            // 
            // TrafodionToolStripRowAction
            // 
            this.TrafodionToolStripRowAction.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._theAddRoleStripButton,
            this._theDeleteRoleStripButton,
            this.toolStripSeparator1,
            this._theRemoveAllStripButton,
            this.toolStripSeparator2,
            this.toolStripLabel1,
            this._theCQSettingStats});
            this.TrafodionToolStripRowAction.Location = new System.Drawing.Point(0, 0);
            this.TrafodionToolStripRowAction.Name = "TrafodionToolStripRowAction";
            this.TrafodionToolStripRowAction.Size = new System.Drawing.Size(904, 25);
            this.TrafodionToolStripRowAction.TabIndex = 2;
            this.TrafodionToolStripRowAction.Text = "TrafodionToolStrip1";
            // 
            // _theAddRoleStripButton
            // 
            this._theAddRoleStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theAddRoleStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theAddRoleStripButton.Image")));
            this._theAddRoleStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theAddRoleStripButton.Name = "_theAddRoleStripButton";
            this._theAddRoleStripButton.Size = new System.Drawing.Size(23, 22);
            this._theAddRoleStripButton.Text = "Add";
            this._theAddRoleStripButton.ToolTipText = "Add a new role";
            this._theAddRoleStripButton.Click += new System.EventHandler(this._theAddRoleStripButton_Click);
            // 
            // _theDeleteRoleStripButton
            // 
            this._theDeleteRoleStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theDeleteRoleStripButton.Enabled = false;
            this._theDeleteRoleStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theDeleteRoleStripButton.Image")));
            this._theDeleteRoleStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theDeleteRoleStripButton.Name = "_theDeleteRoleStripButton";
            this._theDeleteRoleStripButton.Size = new System.Drawing.Size(23, 22);
            this._theDeleteRoleStripButton.Text = "Remove";
            this._theDeleteRoleStripButton.ToolTipText = "Remove selected role(s)";
            this._theDeleteRoleStripButton.Click += new System.EventHandler(this._theDeleteRoleStripButton_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
            // 
            // _theRemoveAllStripButton
            // 
            this._theRemoveAllStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theRemoveAllStripButton.Enabled = false;
            this._theRemoveAllStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theRemoveAllStripButton.Image")));
            this._theRemoveAllStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theRemoveAllStripButton.Name = "_theRemoveAllStripButton";
            this._theRemoveAllStripButton.Size = new System.Drawing.Size(23, 22);
            this._theRemoveAllStripButton.Text = "Remove All";
            this._theRemoveAllStripButton.ToolTipText = "Remove all roles";
            this._theRemoveAllStripButton.Click += new System.EventHandler(this._theRemoveAllStripButton_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(6, 25);
            // 
            // toolStripLabel1
            // 
            this.toolStripLabel1.AutoSize = false;
            this.toolStripLabel1.Name = "toolStripLabel1";
            this.toolStripLabel1.Overflow = System.Windows.Forms.ToolStripItemOverflow.Never;
            this.toolStripLabel1.Size = new System.Drawing.Size(50, 22);
            // 
            // _theCQSettingStats
            // 
            this._theCQSettingStats.AutoSize = false;
            this._theCQSettingStats.BackColor = System.Drawing.Color.AliceBlue;
            this._theCQSettingStats.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theCQSettingStats.ForeColor = System.Drawing.SystemColors.ControlText;
            this._theCQSettingStats.Name = "_theCQSettingStats";
            this._theCQSettingStats.Size = new System.Drawing.Size(250, 22);
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this.helpButton);
            this.TrafodionPanel1.Controls.Add(this.cancelButton);
            this.TrafodionPanel1.Controls.Add(this._okButton);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 450);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(910, 30);
            this.TrafodionPanel1.TabIndex = 23;
            // 
            // helpButton
            // 
            this.helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.helpButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.helpButton.Location = new System.Drawing.Point(832, 4);
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = new System.Drawing.Size(75, 23);
            this.helpButton.TabIndex = 3;
            this.helpButton.Text = "He&lp";
            this.helpButton.UseVisualStyleBackColor = true;
            this.helpButton.Click += new System.EventHandler(this.helpButton_Click);
            // 
            // cancelButton
            // 
            this.cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cancelButton.Location = new System.Drawing.Point(751, 4);
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = new System.Drawing.Size(75, 23);
            this.cancelButton.TabIndex = 2;
            this.cancelButton.Text = "&Cancel";
            this.cancelButton.UseVisualStyleBackColor = true;
            this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
            // 
            // _okButton
            // 
            this._okButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._okButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._okButton.Location = new System.Drawing.Point(670, 4);
            this._okButton.Name = "_okButton";
            this._okButton.Size = new System.Drawing.Size(75, 23);
            this._okButton.TabIndex = 1;
            this._okButton.Text = "&OK";
            this._okButton.UseVisualStyleBackColor = true;
            this._okButton.Click += new System.EventHandler(this._okButton_Click);
            // 
            // errorLabel
            // 
            this.errorLabel.Dock = System.Windows.Forms.DockStyle.Top;
            this.errorLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.errorLabel.ForeColor = System.Drawing.Color.Red;
            this.errorLabel.Location = new System.Drawing.Point(0, 0);
            this.errorLabel.Name = "errorLabel";
            this.errorLabel.Size = new System.Drawing.Size(910, 21);
            this.errorLabel.TabIndex = 24;
            // 
            // _splitContainerCreateRole
            // 
            this._splitContainerCreateRole.Dock = System.Windows.Forms.DockStyle.Fill;
            this._splitContainerCreateRole.Location = new System.Drawing.Point(0, 21);
            this._splitContainerCreateRole.Name = "_splitContainerCreateRole";
            this._splitContainerCreateRole.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // _splitContainerCreateRole.Panel1
            // 
            this._splitContainerCreateRole.Panel1.Controls.Add(this._roleTrafodionGroupBox);
            // 
            // _splitContainerCreateRole.Panel2
            // 
            this._splitContainerCreateRole.Panel2.Controls.Add(this._thesplitContainerUserCom);
            this._splitContainerCreateRole.Size = new System.Drawing.Size(910, 429);
            this._splitContainerCreateRole.SplitterDistance = 161;
            this._splitContainerCreateRole.TabIndex = 26;
            // 
            // _thesplitContainerUserCom
            // 
            this._thesplitContainerUserCom.Dock = System.Windows.Forms.DockStyle.Fill;
            this._thesplitContainerUserCom.Location = new System.Drawing.Point(0, 0);
            this._thesplitContainerUserCom.Name = "_thesplitContainerUserCom";
            // 
            // _thesplitContainerUserCom.Panel1
            // 
            this._thesplitContainerUserCom.Panel1.Controls.Add(this._userGroupBox);
            // 
            // _thesplitContainerUserCom.Panel2
            // 
            this._thesplitContainerUserCom.Panel2.Controls.Add(this._theComponentPrivilegesUserControl);
            this._thesplitContainerUserCom.Panel2.Controls.Add(this.gbxGrantor);
            this._thesplitContainerUserCom.Size = new System.Drawing.Size(910, 264);
            this._thesplitContainerUserCom.SplitterDistance = 332;
            this._thesplitContainerUserCom.TabIndex = 0;
            // 
            // _userGroupBox
            // 
            this._userGroupBox.Controls.Add(this._theUsersGrid);
            this._userGroupBox.Controls.Add(this.compPrivilegesToolStrip);
            this._userGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._userGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._userGroupBox.Location = new System.Drawing.Point(0, 0);
            this._userGroupBox.Name = "_userGroupBox";
            this._userGroupBox.Size = new System.Drawing.Size(332, 264);
            this._userGroupBox.TabIndex = 26;
            this._userGroupBox.TabStop = false;
            this._userGroupBox.Text = "Grant to these users";
            // 
            // _theUsersGrid
            // 
            this._theUsersGrid.AllowColumnFilter = true;
            this._theUsersGrid.AllowWordWrap = false;
            this._theUsersGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("_theUsersGrid.AlwaysHiddenColumnNames")));
            this._theUsersGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this._theUsersGrid.CurrentFilter = null;
            this._theUsersGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theUsersGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUsersGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._theUsersGrid.Header.Height = 20;
            this._theUsersGrid.HelpTopic = "";
            this._theUsersGrid.Location = new System.Drawing.Point(3, 42);
            this._theUsersGrid.Name = "_theUsersGrid";
            this._theUsersGrid.ReadOnly = true;
            this._theUsersGrid.RowMode = true;
            this._theUsersGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this._theUsersGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this._theUsersGrid.SearchAsType.SearchCol = null;
            this._theUsersGrid.Size = new System.Drawing.Size(326, 219);
            this._theUsersGrid.TabIndex = 1;
            this._theUsersGrid.TreeCol = null;
            this._theUsersGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this._theUsersGrid.WordWrap = false;
            // 
            // compPrivilegesToolStrip
            // 
            this.compPrivilegesToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.addUserToolStripButton,
            this.RemoveUserToolStripButton,
            this.toolStripSeparator3,
            this._theRemoveAllUsersStripButton,
            this.toolStripSeparator4});
            this.compPrivilegesToolStrip.Location = new System.Drawing.Point(3, 17);
            this.compPrivilegesToolStrip.Name = "compPrivilegesToolStrip";
            this.compPrivilegesToolStrip.Size = new System.Drawing.Size(326, 25);
            this.compPrivilegesToolStrip.TabIndex = 0;
            this.compPrivilegesToolStrip.Text = "TrafodionToolStrip2";
            // 
            // addUserToolStripButton
            // 
            this.addUserToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.addUserToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("addUserToolStripButton.Image")));
            this.addUserToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.addUserToolStripButton.Name = "addUserToolStripButton";
            this.addUserToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.addUserToolStripButton.Text = "Add";
            this.addUserToolStripButton.ToolTipText = "Add user...";
            this.addUserToolStripButton.Click += new System.EventHandler(this.addUserToolStripButton_Click);
            // 
            // RemoveUserToolStripButton
            // 
            this.RemoveUserToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.RemoveUserToolStripButton.Enabled = false;
            this.RemoveUserToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("RemoveUserToolStripButton.Image")));
            this.RemoveUserToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.RemoveUserToolStripButton.Name = "RemoveUserToolStripButton";
            this.RemoveUserToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.RemoveUserToolStripButton.Text = "Remove";
            this.RemoveUserToolStripButton.ToolTipText = "Remove selected user(s)";
            this.RemoveUserToolStripButton.Click += new System.EventHandler(this.RemoveUserToolStripButton_Click);
            // 
            // toolStripSeparator3
            // 
            this.toolStripSeparator3.Name = "toolStripSeparator3";
            this.toolStripSeparator3.Size = new System.Drawing.Size(6, 25);
            // 
            // _theRemoveAllUsersStripButton
            // 
            this._theRemoveAllUsersStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theRemoveAllUsersStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theRemoveAllUsersStripButton.Image")));
            this._theRemoveAllUsersStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theRemoveAllUsersStripButton.Name = "_theRemoveAllUsersStripButton";
            this._theRemoveAllUsersStripButton.Size = new System.Drawing.Size(23, 22);
            this._theRemoveAllUsersStripButton.Text = "Remove All";
            this._theRemoveAllUsersStripButton.ToolTipText = "Remove all users";
            this._theRemoveAllUsersStripButton.Click += new System.EventHandler(this._theRemoveAllUsersStripButton_Click);
            // 
            // toolStripSeparator4
            // 
            this.toolStripSeparator4.Name = "toolStripSeparator4";
            this.toolStripSeparator4.Size = new System.Drawing.Size(6, 25);
            // 
            // _theComponentPrivilegesUserControl
            // 
            this._theComponentPrivilegesUserControl.ConnectionDefinition = null;
            this._theComponentPrivilegesUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theComponentPrivilegesUserControl.Location = new System.Drawing.Point(0, 0);
            this._theComponentPrivilegesUserControl.Name = "_theComponentPrivilegesUserControl";
            this._theComponentPrivilegesUserControl.Size = new System.Drawing.Size(574, 216);
            this._theComponentPrivilegesUserControl.TabIndex = 0;
            // 
            // gbxGrantor
            // 
            this.gbxGrantor.Controls.Add(this._grantedByLabel);
            this.gbxGrantor.Controls.Add(this._grantedByComboBox);
            this.gbxGrantor.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.gbxGrantor.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.gbxGrantor.Location = new System.Drawing.Point(0, 216);
            this.gbxGrantor.Name = "gbxGrantor";
            this.gbxGrantor.Size = new System.Drawing.Size(574, 48);
            this.gbxGrantor.TabIndex = 24;
            this.gbxGrantor.TabStop = false;
            // 
            // _grantedByLabel
            // 
            this._grantedByLabel.AutoSize = true;
            this._grantedByLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this._grantedByLabel.Location = new System.Drawing.Point(6, 18);
            this._grantedByLabel.Name = "_grantedByLabel";
            this._grantedByLabel.Size = new System.Drawing.Size(70, 13);
            this._grantedByLabel.TabIndex = 6;
            this._grantedByLabel.Text = "Granted By";
            // 
            // _grantedByComboBox
            // 
            this._grantedByComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._grantedByComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._grantedByComboBox.FormattingEnabled = true;
            this._grantedByComboBox.Location = new System.Drawing.Point(101, 15);
            this._grantedByComboBox.Name = "_grantedByComboBox";
            this._grantedByComboBox.Size = new System.Drawing.Size(467, 21);
            this._grantedByComboBox.TabIndex = 5;
            // 
            // _toolTip
            // 
            this._toolTip.IsBalloon = true;
            // 
            // CreateRoleUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._splitContainerCreateRole);
            this.Controls.Add(this.TrafodionPanel1);
            this.Controls.Add(this.errorLabel);
            this.Name = "CreateRoleUserControl";
            this.Size = new System.Drawing.Size(910, 480);
            this._roleTrafodionGroupBox.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.theRolesMappingGrid)).EndInit();
            this.TrafodionPanel2.ResumeLayout(false);
            this.TrafodionPanel2.PerformLayout();
            this.TrafodionToolStripRowAction.ResumeLayout(false);
            this.TrafodionToolStripRowAction.PerformLayout();
            this.TrafodionPanel1.ResumeLayout(false);
            this._splitContainerCreateRole.Panel1.ResumeLayout(false);
            this._splitContainerCreateRole.Panel2.ResumeLayout(false);
            this._splitContainerCreateRole.ResumeLayout(false);
            this._thesplitContainerUserCom.Panel1.ResumeLayout(false);
            this._thesplitContainerUserCom.Panel2.ResumeLayout(false);
            this._thesplitContainerUserCom.ResumeLayout(false);
            this._userGroupBox.ResumeLayout(false);
            this._userGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theUsersGrid)).EndInit();
            this.compPrivilegesToolStrip.ResumeLayout(false);
            this.compPrivilegesToolStrip.PerformLayout();
            this.gbxGrantor.ResumeLayout(false);
            this.gbxGrantor.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionGroupBox _roleTrafodionGroupBox;
        private Framework.Controls.TrafodionIGrid theRolesMappingGrid;
        private Framework.Controls.TrafodionPanel TrafodionPanel2;
        private Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Framework.Controls.TrafodionButton helpButton;
        private Framework.Controls.TrafodionButton cancelButton;
        private Framework.Controls.TrafodionButton _okButton;
        private Framework.Controls.TrafodionLabel errorLabel;
        private Framework.Controls.TrafodionToolStrip TrafodionToolStripRowAction;
        private System.Windows.Forms.ToolStripButton _theAddRoleStripButton;
        private System.Windows.Forms.ToolStripButton _theDeleteRoleStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripButton _theRemoveAllStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripLabel toolStripLabel1;
        private System.Windows.Forms.ToolStripLabel _theCQSettingStats;
        private System.Windows.Forms.SplitContainer _splitContainerCreateRole;
        private System.Windows.Forms.SplitContainer _thesplitContainerUserCom;
        private Framework.Controls.TrafodionGroupBox _userGroupBox;
        private Framework.Controls.TrafodionIGrid _theUsersGrid;
        private Framework.Controls.TrafodionToolStrip compPrivilegesToolStrip;
        private System.Windows.Forms.ToolStripButton addUserToolStripButton;
        private System.Windows.Forms.ToolStripButton RemoveUserToolStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
        private System.Windows.Forms.ToolStripButton _theRemoveAllUsersStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
        private ComponentPrivilegesUserControl _theComponentPrivilegesUserControl;
        private Framework.Controls.TrafodionGroupBox gbxGrantor;
        private Framework.Controls.TrafodionLabel _grantedByLabel;
        private Framework.Controls.TrafodionComboBox _grantedByComboBox;
        private Framework.Controls.TrafodionToolTip _toolTip;
    }
}
