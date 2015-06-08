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
    partial class RegisterUserUserControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(RegisterUserUserControl));
            this._TrafodionGroupBoxUser = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.theUsersMappingGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this.TrafodionPanel3 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.requiredLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionToolStripRowAction = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._theAddUserStripButton = new System.Windows.Forms.ToolStripButton();
            this._theDeleteUserStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this._theRemoveAllUserStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.toolStripLabel1 = new System.Windows.Forms.ToolStripLabel();
            this._theCQSettingStats = new System.Windows.Forms.ToolStripLabel();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._okButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.errorLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theSplitContainerUser = new System.Windows.Forms.SplitContainer();
            this._theSplitContainerRoleCom = new System.Windows.Forms.SplitContainer();
            this._rolesGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._rolesGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this.rolesToolStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this.addRoleToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.deleteRoleToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this._theRemoveAllRoleStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this._theComponentPrivilegesUserControl = new Trafodion.Manager.UserManagement.Controls.ComponentPrivilegesUserControl();
            this.gbxGrantor = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._grantedByLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._grantedByComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._toolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._TrafodionGroupBoxUser.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.theUsersMappingGrid)).BeginInit();
            this.TrafodionPanel3.SuspendLayout();
            this.TrafodionPanel2.SuspendLayout();
            this.TrafodionToolStripRowAction.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this._theSplitContainerUser.Panel1.SuspendLayout();
            this._theSplitContainerUser.Panel2.SuspendLayout();
            this._theSplitContainerUser.SuspendLayout();
            this._theSplitContainerRoleCom.Panel1.SuspendLayout();
            this._theSplitContainerRoleCom.Panel2.SuspendLayout();
            this._theSplitContainerRoleCom.SuspendLayout();
            this._rolesGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._rolesGrid)).BeginInit();
            this.rolesToolStrip.SuspendLayout();
            this.gbxGrantor.SuspendLayout();
            this.SuspendLayout();
            // 
            // _TrafodionGroupBoxUser
            // 
            this._TrafodionGroupBoxUser.Controls.Add(this.theUsersMappingGrid);
            this._TrafodionGroupBoxUser.Controls.Add(this.TrafodionPanel3);
            this._TrafodionGroupBoxUser.Controls.Add(this.TrafodionPanel2);
            this._TrafodionGroupBoxUser.Dock = System.Windows.Forms.DockStyle.Fill;
            this._TrafodionGroupBoxUser.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this._TrafodionGroupBoxUser.Location = new System.Drawing.Point(0, 0);
            this._TrafodionGroupBoxUser.Name = "_TrafodionGroupBoxUser";
            this._TrafodionGroupBoxUser.Size = new System.Drawing.Size(910, 240);
            this._TrafodionGroupBoxUser.TabIndex = 17;
            this._TrafodionGroupBoxUser.TabStop = false;
            this._TrafodionGroupBoxUser.Text = "Register Users";
            // 
            // theUsersMappingGrid
            // 
            this.theUsersMappingGrid.AllowColumnFilter = true;
            this.theUsersMappingGrid.AllowWordWrap = false;
            this.theUsersMappingGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("theUsersMappingGrid.AlwaysHiddenColumnNames")));
            this.theUsersMappingGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this.theUsersMappingGrid.CurrentFilter = null;
            this.theUsersMappingGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theUsersMappingGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.theUsersMappingGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this.theUsersMappingGrid.Header.Height = 20;
            this.theUsersMappingGrid.HelpTopic = "";
            this.theUsersMappingGrid.Location = new System.Drawing.Point(3, 42);
            this.theUsersMappingGrid.Name = "theUsersMappingGrid";
            this.theUsersMappingGrid.ReadOnly = true;
            this.theUsersMappingGrid.RowMode = true;
            this.theUsersMappingGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this.theUsersMappingGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this.theUsersMappingGrid.SearchAsType.SearchCol = null;
            this.theUsersMappingGrid.Size = new System.Drawing.Size(904, 173);
            this.theUsersMappingGrid.TabIndex = 18;
            this.theUsersMappingGrid.TreeCol = null;
            this.theUsersMappingGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.theUsersMappingGrid.WordWrap = false;
            // 
            // TrafodionPanel3
            // 
            this.TrafodionPanel3.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel3.Controls.Add(this.requiredLabel);
            this.TrafodionPanel3.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionPanel3.Location = new System.Drawing.Point(3, 215);
            this.TrafodionPanel3.Name = "TrafodionPanel3";
            this.TrafodionPanel3.Size = new System.Drawing.Size(904, 22);
            this.TrafodionPanel3.TabIndex = 22;
            // 
            // requiredLabel
            // 
            this.requiredLabel.AutoSize = true;
            this.requiredLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.requiredLabel.ForeColor = System.Drawing.Color.Blue;
            this.requiredLabel.Location = new System.Drawing.Point(3, 5);
            this.requiredLabel.Name = "requiredLabel";
            this.requiredLabel.Size = new System.Drawing.Size(92, 13);
            this.requiredLabel.TabIndex = 19;
            this.requiredLabel.Text = "*  Required Fields";
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
            this._theAddUserStripButton,
            this._theDeleteUserStripButton,
            this.toolStripSeparator1,
            this._theRemoveAllUserStripButton,
            this.toolStripSeparator2,
            this.toolStripLabel1,
            this._theCQSettingStats});
            this.TrafodionToolStripRowAction.Location = new System.Drawing.Point(0, 0);
            this.TrafodionToolStripRowAction.Name = "TrafodionToolStripRowAction";
            this.TrafodionToolStripRowAction.Size = new System.Drawing.Size(904, 25);
            this.TrafodionToolStripRowAction.TabIndex = 3;
            this.TrafodionToolStripRowAction.Text = "TrafodionToolStrip1";
            // 
            // _theAddUserStripButton
            // 
            this._theAddUserStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theAddUserStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theAddUserStripButton.Image")));
            this._theAddUserStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theAddUserStripButton.Name = "_theAddUserStripButton";
            this._theAddUserStripButton.Size = new System.Drawing.Size(23, 22);
            this._theAddUserStripButton.Text = "Add";
            this._theAddUserStripButton.ToolTipText = "Add a new user";
            this._theAddUserStripButton.Click += new System.EventHandler(this._addRowButton_Click);
            // 
            // _theDeleteUserStripButton
            // 
            this._theDeleteUserStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theDeleteUserStripButton.Enabled = false;
            this._theDeleteUserStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theDeleteUserStripButton.Image")));
            this._theDeleteUserStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theDeleteUserStripButton.Name = "_theDeleteUserStripButton";
            this._theDeleteUserStripButton.Size = new System.Drawing.Size(23, 22);
            this._theDeleteUserStripButton.Text = "Remove";
            this._theDeleteUserStripButton.ToolTipText = "Remove selected user(s)";
            this._theDeleteUserStripButton.Click += new System.EventHandler(this._deleteRowButton_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
            // 
            // _theRemoveAllUserStripButton
            // 
            this._theRemoveAllUserStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theRemoveAllUserStripButton.Enabled = false;
            this._theRemoveAllUserStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theRemoveAllUserStripButton.Image")));
            this._theRemoveAllUserStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theRemoveAllUserStripButton.Name = "_theRemoveAllUserStripButton";
            this._theRemoveAllUserStripButton.Size = new System.Drawing.Size(23, 22);
            this._theRemoveAllUserStripButton.Text = "Remove All";
            this._theRemoveAllUserStripButton.ToolTipText = "Remove all users";
            this._theRemoveAllUserStripButton.Click += new System.EventHandler(this._theRemoveAllStripButton_Click);
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
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 501);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(910, 30);
            this.TrafodionPanel1.TabIndex = 18;
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
            this.errorLabel.TabIndex = 21;
            // 
            // _theSplitContainerUser
            // 
            this._theSplitContainerUser.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theSplitContainerUser.Location = new System.Drawing.Point(0, 21);
            this._theSplitContainerUser.Name = "_theSplitContainerUser";
            this._theSplitContainerUser.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // _theSplitContainerUser.Panel1
            // 
            this._theSplitContainerUser.Panel1.Controls.Add(this._TrafodionGroupBoxUser);
            // 
            // _theSplitContainerUser.Panel2
            // 
            this._theSplitContainerUser.Panel2.Controls.Add(this._theSplitContainerRoleCom);
            this._theSplitContainerUser.Size = new System.Drawing.Size(910, 480);
            this._theSplitContainerUser.SplitterDistance = 240;
            this._theSplitContainerUser.TabIndex = 23;
            // 
            // _theSplitContainerRoleCom
            // 
            this._theSplitContainerRoleCom.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theSplitContainerRoleCom.Location = new System.Drawing.Point(0, 0);
            this._theSplitContainerRoleCom.Name = "_theSplitContainerRoleCom";
            // 
            // _theSplitContainerRoleCom.Panel1
            // 
            this._theSplitContainerRoleCom.Panel1.Controls.Add(this._rolesGroupBox);
            // 
            // _theSplitContainerRoleCom.Panel2
            // 
            this._theSplitContainerRoleCom.Panel2.Controls.Add(this._theComponentPrivilegesUserControl);
            this._theSplitContainerRoleCom.Panel2.Controls.Add(this.gbxGrantor);
            this._theSplitContainerRoleCom.Size = new System.Drawing.Size(910, 236);
            this._theSplitContainerRoleCom.SplitterDistance = 332;
            this._theSplitContainerRoleCom.TabIndex = 0;
            // 
            // _rolesGroupBox
            // 
            this._rolesGroupBox.Controls.Add(this._rolesGrid);
            this._rolesGroupBox.Controls.Add(this.rolesToolStrip);
            this._rolesGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._rolesGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._rolesGroupBox.Location = new System.Drawing.Point(0, 0);
            this._rolesGroupBox.Name = "_rolesGroupBox";
            this._rolesGroupBox.Size = new System.Drawing.Size(332, 236);
            this._rolesGroupBox.TabIndex = 23;
            this._rolesGroupBox.TabStop = false;
            this._rolesGroupBox.Text = "Grant these roles";
            // 
            // _rolesGrid
            // 
            this._rolesGrid.AllowColumnFilter = true;
            this._rolesGrid.AllowWordWrap = false;
            this._rolesGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("_rolesGrid.AlwaysHiddenColumnNames")));
            this._rolesGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this._rolesGrid.CurrentFilter = null;
            this._rolesGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._rolesGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._rolesGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._rolesGrid.Header.Height = 20;
            this._rolesGrid.HelpTopic = "";
            this._rolesGrid.Location = new System.Drawing.Point(3, 42);
            this._rolesGrid.Name = "_rolesGrid";
            this._rolesGrid.ReadOnly = true;
            this._rolesGrid.RowMode = true;
            this._rolesGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this._rolesGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this._rolesGrid.SearchAsType.SearchCol = null;
            this._rolesGrid.Size = new System.Drawing.Size(326, 191);
            this._rolesGrid.TabIndex = 1;
            this._rolesGrid.TreeCol = null;
            this._rolesGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this._rolesGrid.WordWrap = false;
            // 
            // rolesToolStrip
            // 
            this.rolesToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.addRoleToolStripButton,
            this.deleteRoleToolStripButton,
            this.toolStripSeparator3,
            this._theRemoveAllRoleStripButton,
            this.toolStripSeparator4});
            this.rolesToolStrip.Location = new System.Drawing.Point(3, 17);
            this.rolesToolStrip.Name = "rolesToolStrip";
            this.rolesToolStrip.Size = new System.Drawing.Size(326, 25);
            this.rolesToolStrip.TabIndex = 0;
            this.rolesToolStrip.Text = "TrafodionToolStrip2";
            // 
            // addRoleToolStripButton
            // 
            this.addRoleToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.addRoleToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("addRoleToolStripButton.Image")));
            this.addRoleToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.addRoleToolStripButton.Name = "addRoleToolStripButton";
            this.addRoleToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.addRoleToolStripButton.Text = "Add role...";
            this.addRoleToolStripButton.Click += new System.EventHandler(this.addRoleToolStripButton_Click);
            // 
            // deleteRoleToolStripButton
            // 
            this.deleteRoleToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.deleteRoleToolStripButton.Enabled = false;
            this.deleteRoleToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("deleteRoleToolStripButton.Image")));
            this.deleteRoleToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.deleteRoleToolStripButton.Name = "deleteRoleToolStripButton";
            this.deleteRoleToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.deleteRoleToolStripButton.Text = "Remove selected role(s)";
            this.deleteRoleToolStripButton.Click += new System.EventHandler(this.deleteRoleToolStripButton_Click);
            // 
            // toolStripSeparator3
            // 
            this.toolStripSeparator3.Name = "toolStripSeparator3";
            this.toolStripSeparator3.Size = new System.Drawing.Size(6, 25);
            // 
            // _theRemoveAllRoleStripButton
            // 
            this._theRemoveAllRoleStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theRemoveAllRoleStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theRemoveAllRoleStripButton.Image")));
            this._theRemoveAllRoleStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theRemoveAllRoleStripButton.Name = "_theRemoveAllRoleStripButton";
            this._theRemoveAllRoleStripButton.Size = new System.Drawing.Size(23, 22);
            this._theRemoveAllRoleStripButton.Text = "Remove All";
            this._theRemoveAllRoleStripButton.ToolTipText = "Remove all roles";
            this._theRemoveAllRoleStripButton.Click += new System.EventHandler(this._theRemoveAllRoleStripButton_Click);
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
            this._theComponentPrivilegesUserControl.Size = new System.Drawing.Size(574, 188);
            this._theComponentPrivilegesUserControl.TabIndex = 0;
            // 
            // gbxGrantor
            // 
            this.gbxGrantor.Controls.Add(this._grantedByLabel);
            this.gbxGrantor.Controls.Add(this._grantedByComboBox);
            this.gbxGrantor.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.gbxGrantor.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.gbxGrantor.Location = new System.Drawing.Point(0, 188);
            this.gbxGrantor.Name = "gbxGrantor";
            this.gbxGrantor.Size = new System.Drawing.Size(574, 48);
            this.gbxGrantor.TabIndex = 23;
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
            // RegisterUserUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._theSplitContainerUser);
            this.Controls.Add(this.errorLabel);
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "RegisterUserUserControl";
            this.Size = new System.Drawing.Size(910, 531);
            this._TrafodionGroupBoxUser.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.theUsersMappingGrid)).EndInit();
            this.TrafodionPanel3.ResumeLayout(false);
            this.TrafodionPanel3.PerformLayout();
            this.TrafodionPanel2.ResumeLayout(false);
            this.TrafodionPanel2.PerformLayout();
            this.TrafodionToolStripRowAction.ResumeLayout(false);
            this.TrafodionToolStripRowAction.PerformLayout();
            this.TrafodionPanel1.ResumeLayout(false);
            this._theSplitContainerUser.Panel1.ResumeLayout(false);
            this._theSplitContainerUser.Panel2.ResumeLayout(false);
            this._theSplitContainerUser.ResumeLayout(false);
            this._theSplitContainerRoleCom.Panel1.ResumeLayout(false);
            this._theSplitContainerRoleCom.Panel2.ResumeLayout(false);
            this._theSplitContainerRoleCom.ResumeLayout(false);
            this._rolesGroupBox.ResumeLayout(false);
            this._rolesGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._rolesGrid)).EndInit();
            this.rolesToolStrip.ResumeLayout(false);
            this.rolesToolStrip.PerformLayout();
            this.gbxGrantor.ResumeLayout(false);
            this.gbxGrantor.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _TrafodionGroupBoxUser;
        private Trafodion.Manager.Framework.Controls.TrafodionIGrid theUsersMappingGrid;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel requiredLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton helpButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _okButton;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel errorLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel3;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel2;
        private Framework.Controls.TrafodionToolStrip TrafodionToolStripRowAction;
        private System.Windows.Forms.ToolStripButton _theAddUserStripButton;
        private System.Windows.Forms.ToolStripButton _theDeleteUserStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripButton _theRemoveAllUserStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripLabel toolStripLabel1;
        private System.Windows.Forms.ToolStripLabel _theCQSettingStats;
        private System.Windows.Forms.SplitContainer _theSplitContainerUser;
        private System.Windows.Forms.SplitContainer _theSplitContainerRoleCom;
        private Framework.Controls.TrafodionGroupBox _rolesGroupBox;
        private Framework.Controls.TrafodionIGrid _rolesGrid;
        private Framework.Controls.TrafodionToolStrip rolesToolStrip;
        private System.Windows.Forms.ToolStripButton addRoleToolStripButton;
        private System.Windows.Forms.ToolStripButton deleteRoleToolStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
        private System.Windows.Forms.ToolStripButton _theRemoveAllRoleStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
        private ComponentPrivilegesUserControl _theComponentPrivilegesUserControl;
        private Framework.Controls.TrafodionGroupBox gbxGrantor;
        private Framework.Controls.TrafodionLabel _grantedByLabel;
        private Framework.Controls.TrafodionComboBox _grantedByComboBox;
        private Framework.Controls.TrafodionToolTip _toolTip;
    }
}
