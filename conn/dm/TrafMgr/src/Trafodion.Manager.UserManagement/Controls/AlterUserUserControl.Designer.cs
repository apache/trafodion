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
    partial class AlterUserUserControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AlterUserUserControl));
            this._contentPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theSplitContainer = new System.Windows.Forms.SplitContainer();
            this._rolesGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._rolesGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this.rolesToolStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this.addRoleToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.deleteRoleToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this._theRemoveAllRoleStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this._theComponentPrivilegesUserControl = new Trafodion.Manager.UserManagement.Controls.ComponentPrivilegesUserControl();
            this.gbxGrantor = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._grantedByLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._grantedByComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._progressPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._headerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theDropdownAuthenticationType = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theValidUser = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._dbUserNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._dirServiceUserNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.lblAuthType = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.lblNotNull = new System.Windows.Forms.Label();
            this._buttonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._resetButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._applyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._toolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._theImmutableUser = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._contentPanel.SuspendLayout();
            this._theSplitContainer.Panel1.SuspendLayout();
            this._theSplitContainer.Panel2.SuspendLayout();
            this._theSplitContainer.SuspendLayout();
            this._rolesGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._rolesGrid)).BeginInit();
            this.rolesToolStrip.SuspendLayout();
            this.gbxGrantor.SuspendLayout();
            this._headerPanel.SuspendLayout();
            this._buttonsPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _contentPanel
            // 
            this._contentPanel.AutoScroll = true;
            this._contentPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._contentPanel.Controls.Add(this._theSplitContainer);
            this._contentPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._contentPanel.Location = new System.Drawing.Point(0, 226);
            this._contentPanel.Name = "_contentPanel";
            this._contentPanel.Size = new System.Drawing.Size(910, 270);
            this._contentPanel.TabIndex = 0;
            // 
            // _theSplitContainer
            // 
            this._theSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theSplitContainer.Location = new System.Drawing.Point(0, 0);
            this._theSplitContainer.Name = "_theSplitContainer";
            // 
            // _theSplitContainer.Panel1
            // 
            this._theSplitContainer.Panel1.Controls.Add(this._rolesGroupBox);
            // 
            // _theSplitContainer.Panel2
            // 
            this._theSplitContainer.Panel2.Controls.Add(this._theComponentPrivilegesUserControl);
            this._theSplitContainer.Panel2.Controls.Add(this.gbxGrantor);
            this._theSplitContainer.Size = new System.Drawing.Size(910, 270);
            this._theSplitContainer.SplitterDistance = 331;
            this._theSplitContainer.TabIndex = 6;
            // 
            // _rolesGroupBox
            // 
            this._rolesGroupBox.Controls.Add(this._rolesGrid);
            this._rolesGroupBox.Controls.Add(this.rolesToolStrip);
            this._rolesGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._rolesGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._rolesGroupBox.Location = new System.Drawing.Point(0, 0);
            this._rolesGroupBox.Name = "_rolesGroupBox";
            this._rolesGroupBox.Size = new System.Drawing.Size(331, 270);
            this._rolesGroupBox.TabIndex = 4;
            this._rolesGroupBox.TabStop = false;
            this._rolesGroupBox.Text = "Granted Roles";
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
            this._rolesGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            this._rolesGrid.Size = new System.Drawing.Size(325, 225);
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
            this.toolStripSeparator1,
            this._theRemoveAllRoleStripButton,
            this.toolStripSeparator2});
            this.rolesToolStrip.Location = new System.Drawing.Point(3, 17);
            this.rolesToolStrip.Name = "rolesToolStrip";
            this.rolesToolStrip.Size = new System.Drawing.Size(325, 25);
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
            this.addRoleToolStripButton.Text = "Add Role...";
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
            this._theRemoveAllRoleStripButton.Text = "Remove All Roles";
            this._theRemoveAllRoleStripButton.ToolTipText = "Remove all roles";
            this._theRemoveAllRoleStripButton.Click += new System.EventHandler(this._theRemoveAllRoleStripButton_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(6, 25);
            // 
            // _theComponentPrivilegesUserControl
            // 
            this._theComponentPrivilegesUserControl.ConnectionDefinition = null;
            this._theComponentPrivilegesUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theComponentPrivilegesUserControl.Location = new System.Drawing.Point(0, 0);
            this._theComponentPrivilegesUserControl.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._theComponentPrivilegesUserControl.Name = "_theComponentPrivilegesUserControl";
            this._theComponentPrivilegesUserControl.Size = new System.Drawing.Size(575, 222);
            this._theComponentPrivilegesUserControl.TabIndex = 0;
            // 
            // gbxGrantor
            // 
            this.gbxGrantor.Controls.Add(this._grantedByLabel);
            this.gbxGrantor.Controls.Add(this._grantedByComboBox);
            this.gbxGrantor.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.gbxGrantor.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.gbxGrantor.Location = new System.Drawing.Point(0, 222);
            this.gbxGrantor.Name = "gbxGrantor";
            this.gbxGrantor.Size = new System.Drawing.Size(575, 48);
            this.gbxGrantor.TabIndex = 25;
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
            this._grantedByComboBox.Size = new System.Drawing.Size(469, 21);
            this._grantedByComboBox.TabIndex = 5;
            this._grantedByComboBox.TextChanged += new System.EventHandler(this._grantedByComboBox_TextChanged);
            // 
            // _progressPanel
            // 
            this._progressPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._progressPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._progressPanel.Location = new System.Drawing.Point(0, 131);
            this._progressPanel.Name = "_progressPanel";
            this._progressPanel.Size = new System.Drawing.Size(910, 95);
            this._progressPanel.TabIndex = 6;
            // 
            // _headerPanel
            // 
            this._headerPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._headerPanel.Controls.Add(this._theDropdownAuthenticationType);
            this._headerPanel.Controls.Add(this.TrafodionLabel1);
            this._headerPanel.Controls.Add(this._theImmutableUser);
            this._headerPanel.Controls.Add(this._theValidUser);
            this._headerPanel.Controls.Add(this._dbUserNameTextBox);
            this._headerPanel.Controls.Add(this._dirServiceUserNameTextBox);
            this._headerPanel.Controls.Add(this.lblAuthType);
            this._headerPanel.Controls.Add(this.TrafodionLabel2);
            this._headerPanel.Controls.Add(this.lblNotNull);
            this._headerPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._headerPanel.Location = new System.Drawing.Point(0, 0);
            this._headerPanel.Name = "_headerPanel";
            this._headerPanel.Size = new System.Drawing.Size(910, 131);
            this._headerPanel.TabIndex = 6;
            // 
            // _theDropdownAuthenticationType
            // 
            this._theDropdownAuthenticationType.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theDropdownAuthenticationType.FormattingEnabled = true;
            this._theDropdownAuthenticationType.Location = new System.Drawing.Point(175, 78);
            this._theDropdownAuthenticationType.Name = "_theDropdownAuthenticationType";
            this._theDropdownAuthenticationType.Size = new System.Drawing.Size(121, 21);
            this._theDropdownAuthenticationType.TabIndex = 6;
            // 
            // TrafodionLabel1
            // 
            this.TrafodionLabel1.AutoSize = true;
            this.TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.TrafodionLabel1.Location = new System.Drawing.Point(13, 37);
            this.TrafodionLabel1.Name = "TrafodionLabel1";
            this.TrafodionLabel1.Size = new System.Drawing.Size(145, 13);
            this.TrafodionLabel1.TabIndex = 0;
            this.TrafodionLabel1.Text = "Directory-Service User Name";
            // 
            // _theValidUser
            // 
            this._theValidUser.AutoSize = true;
            this._theValidUser.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this._theValidUser.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theValidUser.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theValidUser.Location = new System.Drawing.Point(97, 58);
            this._theValidUser.Name = "_theValidUser";
            this._theValidUser.Size = new System.Drawing.Size(91, 18);
            this._theValidUser.TabIndex = 2;
            this._theValidUser.Text = "Valid User    ";
            this._theValidUser.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this._theValidUser.UseVisualStyleBackColor = true;
            // 
            // _dbUserNameTextBox
            // 
            this._dbUserNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._dbUserNameTextBox.Location = new System.Drawing.Point(175, 6);
            this._dbUserNameTextBox.Name = "_dbUserNameTextBox";
            this._dbUserNameTextBox.ReadOnly = true;
            this._dbUserNameTextBox.Size = new System.Drawing.Size(691, 21);
            this._dbUserNameTextBox.TabIndex = 0;
            // 
            // _dirServiceUserNameTextBox
            // 
            this._dirServiceUserNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._dirServiceUserNameTextBox.Location = new System.Drawing.Point(175, 34);
            this._dirServiceUserNameTextBox.Name = "_dirServiceUserNameTextBox";
            this._dirServiceUserNameTextBox.Size = new System.Drawing.Size(691, 21);
            this._dirServiceUserNameTextBox.TabIndex = 1;
            // 
            // lblAuthType
            // 
            this.lblAuthType.AutoSize = true;
            this.lblAuthType.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblAuthType.Location = new System.Drawing.Point(81, 81);
            this.lblAuthType.Name = "lblAuthType";
            this.lblAuthType.Size = new System.Drawing.Size(77, 13);
            this.lblAuthType.TabIndex = 2;
            this.lblAuthType.Text = "Authentication";
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.TrafodionLabel2.Location = new System.Drawing.Point(50, 9);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(108, 13);
            this.TrafodionLabel2.TabIndex = 2;
            this.TrafodionLabel2.Text = "Database User Name";
            // 
            // lblNotNull
            // 
            this.lblNotNull.AutoSize = true;
            this.lblNotNull.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblNotNull.ForeColor = System.Drawing.Color.Red;
            this.lblNotNull.Location = new System.Drawing.Point(159, 37);
            this.lblNotNull.Name = "lblNotNull";
            this.lblNotNull.Size = new System.Drawing.Size(15, 20);
            this.lblNotNull.TabIndex = 5;
            this.lblNotNull.Text = "*";
            // 
            // _buttonsPanel
            // 
            this._buttonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._buttonsPanel.Controls.Add(this._resetButton);
            this._buttonsPanel.Controls.Add(this.helpButton);
            this._buttonsPanel.Controls.Add(this.cancelButton);
            this._buttonsPanel.Controls.Add(this._applyButton);
            this._buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._buttonsPanel.Location = new System.Drawing.Point(0, 496);
            this._buttonsPanel.Name = "_buttonsPanel";
            this._buttonsPanel.Size = new System.Drawing.Size(910, 30);
            this._buttonsPanel.TabIndex = 1;
            // 
            // _resetButton
            // 
            this._resetButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._resetButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._resetButton.Location = new System.Drawing.Point(5, 4);
            this._resetButton.Name = "_resetButton";
            this._resetButton.Size = new System.Drawing.Size(75, 23);
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
            this.helpButton.Location = new System.Drawing.Point(832, 4);
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = new System.Drawing.Size(75, 23);
            this.helpButton.TabIndex = 7;
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
            this.cancelButton.TabIndex = 6;
            this.cancelButton.Text = "&Cancel";
            this.cancelButton.UseVisualStyleBackColor = true;
            this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
            // 
            // _applyButton
            // 
            this._applyButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._applyButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._applyButton.Location = new System.Drawing.Point(670, 4);
            this._applyButton.Name = "_applyButton";
            this._applyButton.Size = new System.Drawing.Size(75, 23);
            this._applyButton.TabIndex = 5;
            this._applyButton.Text = "&Apply";
            this._applyButton.UseVisualStyleBackColor = true;
            this._applyButton.Click += new System.EventHandler(this._okButton_Click);
            // 
            // _toolTip
            // 
            this._toolTip.IsBalloon = true;
            // 
            // _theImmutableUser
            // 
            this._theImmutableUser.AutoSize = true;
            this._theImmutableUser.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this._theImmutableUser.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theImmutableUser.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theImmutableUser.Location = new System.Drawing.Point(69, 105);
            this._theImmutableUser.Name = "_theImmutableUser";
            this._theImmutableUser.Size = new System.Drawing.Size(119, 18);
            this._theImmutableUser.TabIndex = 2;
            this._theImmutableUser.Text = "Immutable User    ";
            this._theImmutableUser.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this._theImmutableUser.UseVisualStyleBackColor = true;
            // 
            // AlterUserUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._contentPanel);
            this.Controls.Add(this._progressPanel);
            this.Controls.Add(this._headerPanel);
            this.Controls.Add(this._buttonsPanel);
            this.Name = "AlterUserUserControl";
            this.Size = new System.Drawing.Size(910, 526);
            this._contentPanel.ResumeLayout(false);
            this._theSplitContainer.Panel1.ResumeLayout(false);
            this._theSplitContainer.Panel2.ResumeLayout(false);
            this._theSplitContainer.ResumeLayout(false);
            this._rolesGroupBox.ResumeLayout(false);
            this._rolesGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._rolesGrid)).EndInit();
            this.rolesToolStrip.ResumeLayout(false);
            this.rolesToolStrip.PerformLayout();
            this.gbxGrantor.ResumeLayout(false);
            this.gbxGrantor.PerformLayout();
            this._headerPanel.ResumeLayout(false);
            this._headerPanel.PerformLayout();
            this._buttonsPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _buttonsPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton helpButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _applyButton;
        private Framework.Controls.TrafodionGroupBox _rolesGroupBox;
        private Framework.Controls.TrafodionIGrid _rolesGrid;
        private Framework.Controls.TrafodionToolStrip rolesToolStrip;
        private System.Windows.Forms.ToolStripButton addRoleToolStripButton;
        private System.Windows.Forms.ToolStripButton deleteRoleToolStripButton;
        private Framework.Controls.TrafodionPanel _contentPanel;
        private Framework.Controls.TrafodionTextBox _dbUserNameTextBox;
        private Framework.Controls.TrafodionTextBox _dirServiceUserNameTextBox;
        private Framework.Controls.TrafodionLabel TrafodionLabel2;
        private Framework.Controls.TrafodionLabel TrafodionLabel1;

        private Framework.Controls.TrafodionPanel _progressPanel;
        private Framework.Controls.TrafodionPanel _headerPanel;
        private System.Windows.Forms.SplitContainer _theSplitContainer;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripButton _theRemoveAllRoleStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private Framework.Controls.TrafodionButton _resetButton;
        private Framework.Controls.TrafodionCheckBox _theValidUser;
        private System.Windows.Forms.Label lblNotNull;
        private ComponentPrivilegesUserControl _theComponentPrivilegesUserControl;
        private Framework.Controls.TrafodionGroupBox gbxGrantor;
        private Framework.Controls.TrafodionLabel _grantedByLabel;
        private Framework.Controls.TrafodionComboBox _grantedByComboBox;
        private Framework.Controls.TrafodionToolTip _toolTip;
        private Framework.Controls.TrafodionComboBox _theDropdownAuthenticationType;
        private Framework.Controls.TrafodionLabel lblAuthType;
        private Framework.Controls.TrafodionCheckBox _theImmutableUser;
    }
}
