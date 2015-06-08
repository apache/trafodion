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
    partial class GrantRevokeControl
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
            this.components = new System.ComponentModel.Container();
            this.outerSplitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this.objectsGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.actionGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.privilegesGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.privilegesPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.progressPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.actionTypeGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.cascadeCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.withGrantOptionCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._grantedByLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._grantedByComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._grantRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.revokeRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.granteesGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.granteeListPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.granteeListBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckedListBox();
            this._selectUsersGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.selRolesRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.selUsersRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._publicRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.buttonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.resetButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.applyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._toolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.outerSplitContainer.Panel1.SuspendLayout();
            this.outerSplitContainer.Panel2.SuspendLayout();
            this.outerSplitContainer.SuspendLayout();
            this.actionGroupBox.SuspendLayout();
            this.privilegesGroupBox.SuspendLayout();
            this.actionTypeGroupBox.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this.TrafodionGroupBox1.SuspendLayout();
            this.granteesGroupBox.SuspendLayout();
            this.granteeListPanel.SuspendLayout();
            this._selectUsersGroupBox.SuspendLayout();
            this.buttonsPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // outerSplitContainer
            // 
            this.outerSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.outerSplitContainer.Location = new System.Drawing.Point(0, 0);
            this.outerSplitContainer.Name = "outerSplitContainer";
            // 
            // outerSplitContainer.Panel1
            // 
            this.outerSplitContainer.Panel1.Controls.Add(this.objectsGroupBox);
            // 
            // outerSplitContainer.Panel2
            // 
            this.outerSplitContainer.Panel2.AutoScroll = true;
            this.outerSplitContainer.Panel2.Controls.Add(this.actionGroupBox);
            this.outerSplitContainer.Panel2.Controls.Add(this.granteesGroupBox);
            this.outerSplitContainer.Panel2.Controls.Add(this.buttonsPanel);
            this.outerSplitContainer.Size = new System.Drawing.Size(1079, 693);
            this.outerSplitContainer.SplitterDistance = 211;
            this.outerSplitContainer.SplitterWidth = 9;
            this.outerSplitContainer.TabIndex = 0;
            // 
            // objectsGroupBox
            // 
            this.objectsGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.objectsGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.objectsGroupBox.Location = new System.Drawing.Point(0, 0);
            this.objectsGroupBox.Name = "objectsGroupBox";
            this.objectsGroupBox.Size = new System.Drawing.Size(211, 693);
            this.objectsGroupBox.TabIndex = 1;
            this.objectsGroupBox.TabStop = false;
            this.objectsGroupBox.Text = "Select Objects:";
            // 
            // actionGroupBox
            // 
            this.actionGroupBox.Controls.Add(this.privilegesGroupBox);
            this.actionGroupBox.Controls.Add(this.progressPanel);
            this.actionGroupBox.Controls.Add(this.actionTypeGroupBox);
            this.actionGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.actionGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.actionGroupBox.Location = new System.Drawing.Point(247, 0);
            this.actionGroupBox.Name = "actionGroupBox";
            this.actionGroupBox.Size = new System.Drawing.Size(612, 660);
            this.actionGroupBox.TabIndex = 2;
            this.actionGroupBox.TabStop = false;
            this.actionGroupBox.Text = "Action";
            // 
            // privilegesGroupBox
            // 
            this.privilegesGroupBox.Controls.Add(this.privilegesPanel);
            this.privilegesGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.privilegesGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.privilegesGroupBox.Location = new System.Drawing.Point(3, 195);
            this.privilegesGroupBox.Name = "privilegesGroupBox";
            this.privilegesGroupBox.Size = new System.Drawing.Size(606, 462);
            this.privilegesGroupBox.TabIndex = 1;
            this.privilegesGroupBox.TabStop = false;
            this.privilegesGroupBox.Text = "Privileges";
            // 
            // privilegesPanel
            // 
            this.privilegesPanel.AutoScroll = true;
            this.privilegesPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.privilegesPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.privilegesPanel.Location = new System.Drawing.Point(3, 17);
            this.privilegesPanel.Name = "privilegesPanel";
            this.privilegesPanel.Size = new System.Drawing.Size(600, 442);
            this.privilegesPanel.TabIndex = 0;
            // 
            // progressPanel
            // 
            this.progressPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.progressPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.progressPanel.Location = new System.Drawing.Point(3, 100);
            this.progressPanel.Name = "progressPanel";
            this.progressPanel.Size = new System.Drawing.Size(606, 95);
            this.progressPanel.TabIndex = 4;
            // 
            // actionTypeGroupBox
            // 
            this.actionTypeGroupBox.Controls.Add(this.TrafodionPanel1);
            this.actionTypeGroupBox.Controls.Add(this._grantedByLabel);
            this.actionTypeGroupBox.Controls.Add(this._grantedByComboBox);
            this.actionTypeGroupBox.Controls.Add(this.TrafodionGroupBox1);
            this.actionTypeGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this.actionTypeGroupBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.actionTypeGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.actionTypeGroupBox.Location = new System.Drawing.Point(3, 17);
            this.actionTypeGroupBox.Name = "actionTypeGroupBox";
            this.actionTypeGroupBox.Padding = new System.Windows.Forms.Padding(6, 0, 0, 2);
            this.actionTypeGroupBox.Size = new System.Drawing.Size(606, 83);
            this.actionTypeGroupBox.TabIndex = 0;
            this.actionTypeGroupBox.TabStop = false;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this.cascadeCheckBox);
            this.TrafodionPanel1.Controls.Add(this.withGrantOptionCheckBox);
            this.TrafodionPanel1.Location = new System.Drawing.Point(184, 11);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(419, 36);
            this.TrafodionPanel1.TabIndex = 5;
            // 
            // cascadeCheckBox
            // 
            this.cascadeCheckBox.AutoSize = true;
            this.cascadeCheckBox.Dock = System.Windows.Forms.DockStyle.Left;
            this.cascadeCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cascadeCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cascadeCheckBox.Location = new System.Drawing.Point(125, 0);
            this.cascadeCheckBox.Name = "cascadeCheckBox";
            this.cascadeCheckBox.Padding = new System.Windows.Forms.Padding(3);
            this.cascadeCheckBox.Size = new System.Drawing.Size(79, 36);
            this.cascadeCheckBox.TabIndex = 2;
            this.cascadeCheckBox.Text = "Cascade";
            this.cascadeCheckBox.UseVisualStyleBackColor = true;
            this.cascadeCheckBox.Visible = false;
            // 
            // withGrantOptionCheckBox
            // 
            this.withGrantOptionCheckBox.AutoSize = true;
            this.withGrantOptionCheckBox.Dock = System.Windows.Forms.DockStyle.Left;
            this.withGrantOptionCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.withGrantOptionCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.withGrantOptionCheckBox.Location = new System.Drawing.Point(0, 0);
            this.withGrantOptionCheckBox.Margin = new System.Windows.Forms.Padding(12, 3, 3, 3);
            this.withGrantOptionCheckBox.Name = "withGrantOptionCheckBox";
            this.withGrantOptionCheckBox.Padding = new System.Windows.Forms.Padding(3);
            this.withGrantOptionCheckBox.Size = new System.Drawing.Size(125, 36);
            this.withGrantOptionCheckBox.TabIndex = 2;
            this.withGrantOptionCheckBox.Text = "With Grant Option";
            this.withGrantOptionCheckBox.UseVisualStyleBackColor = true;
            // 
            // _grantedByLabel
            // 
            this._grantedByLabel.AutoSize = true;
            this._grantedByLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._grantedByLabel.Location = new System.Drawing.Point(15, 56);
            this._grantedByLabel.Name = "_grantedByLabel";
            this._grantedByLabel.Size = new System.Drawing.Size(61, 13);
            this._grantedByLabel.TabIndex = 4;
            this._grantedByLabel.Text = "Granted By";
            // 
            // _grantedByComboBox
            // 
            this._grantedByComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._grantedByComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._grantedByComboBox.FormattingEnabled = true;
            this._grantedByComboBox.Location = new System.Drawing.Point(80, 53);
            this._grantedByComboBox.Name = "_grantedByComboBox";
            this._grantedByComboBox.Size = new System.Drawing.Size(523, 21);
            this._grantedByComboBox.TabIndex = 3;
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.Controls.Add(this._grantRadioButton);
            this.TrafodionGroupBox1.Controls.Add(this.revokeRadioButton);
            this.TrafodionGroupBox1.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(6, 9);
            this.TrafodionGroupBox1.Margin = new System.Windows.Forms.Padding(0);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Padding = new System.Windows.Forms.Padding(0, 2, 0, 0);
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(175, 37);
            this.TrafodionGroupBox1.TabIndex = 1;
            this.TrafodionGroupBox1.TabStop = false;
            // 
            // _grantRadioButton
            // 
            this._grantRadioButton.AutoSize = true;
            this._grantRadioButton.Checked = true;
            this._grantRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._grantRadioButton.Location = new System.Drawing.Point(12, 12);
            this._grantRadioButton.Name = "_grantRadioButton";
            this._grantRadioButton.Size = new System.Drawing.Size(58, 18);
            this._grantRadioButton.TabIndex = 0;
            this._grantRadioButton.TabStop = true;
            this._grantRadioButton.Text = "Grant";
            this._grantRadioButton.UseVisualStyleBackColor = true;
            this._grantRadioButton.CheckedChanged += new System.EventHandler(this._grantRadioButton_CheckedChanged);
            this._grantRadioButton.Click += new System.EventHandler(this._grantRadioButton_Click);
            // 
            // revokeRadioButton
            // 
            this.revokeRadioButton.AutoSize = true;
            this.revokeRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.revokeRadioButton.Location = new System.Drawing.Point(101, 12);
            this.revokeRadioButton.Name = "revokeRadioButton";
            this.revokeRadioButton.Size = new System.Drawing.Size(67, 18);
            this.revokeRadioButton.TabIndex = 0;
            this.revokeRadioButton.TabStop = true;
            this.revokeRadioButton.Text = "Revoke";
            this.revokeRadioButton.UseVisualStyleBackColor = true;
            this.revokeRadioButton.CheckedChanged += new System.EventHandler(this.revokeRadioButton_CheckedChanged);
            this.revokeRadioButton.Click += new System.EventHandler(this.revokeRadioButton_Click);
            // 
            // granteesGroupBox
            // 
            this.granteesGroupBox.Controls.Add(this.granteeListPanel);
            this.granteesGroupBox.Dock = System.Windows.Forms.DockStyle.Left;
            this.granteesGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.granteesGroupBox.Location = new System.Drawing.Point(0, 0);
            this.granteesGroupBox.Name = "granteesGroupBox";
            this.granteesGroupBox.Size = new System.Drawing.Size(247, 660);
            this.granteesGroupBox.TabIndex = 0;
            this.granteesGroupBox.TabStop = false;
            this.granteesGroupBox.Text = "Grantees";
            // 
            // granteeListPanel
            // 
            this.granteeListPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.granteeListPanel.Controls.Add(this.granteeListBox);
            this.granteeListPanel.Controls.Add(this._selectUsersGroupBox);
            this.granteeListPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.granteeListPanel.Location = new System.Drawing.Point(3, 17);
            this.granteeListPanel.Name = "granteeListPanel";
            this.granteeListPanel.Size = new System.Drawing.Size(241, 640);
            this.granteeListPanel.TabIndex = 5;
            // 
            // granteeListBox
            // 
            this.granteeListBox.CheckOnClick = true;
            this.granteeListBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.granteeListBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.granteeListBox.FormattingEnabled = true;
            this.granteeListBox.HorizontalScrollbar = true;
            this.granteeListBox.Location = new System.Drawing.Point(0, 83);
            this.granteeListBox.Name = "granteeListBox";
            this.granteeListBox.Size = new System.Drawing.Size(241, 557);
            this.granteeListBox.TabIndex = 0;
            this.granteeListBox.ThreeDCheckBoxes = true;
            this.granteeListBox.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.granteeListBox_ItemCheck);
            this.granteeListBox.SelectedIndexChanged += new System.EventHandler(this.granteeListBox_SelectedIndexChanged);
            // 
            // _selectUsersGroupBox
            // 
            this._selectUsersGroupBox.Controls.Add(this.selRolesRadioButton);
            this._selectUsersGroupBox.Controls.Add(this.selUsersRadioButton);
            this._selectUsersGroupBox.Controls.Add(this._publicRadioButton);
            this._selectUsersGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._selectUsersGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._selectUsersGroupBox.Location = new System.Drawing.Point(0, 0);
            this._selectUsersGroupBox.Name = "_selectUsersGroupBox";
            this._selectUsersGroupBox.Size = new System.Drawing.Size(241, 83);
            this._selectUsersGroupBox.TabIndex = 1;
            this._selectUsersGroupBox.TabStop = false;
            // 
            // selRolesRadioButton
            // 
            this.selRolesRadioButton.AutoSize = true;
            this.selRolesRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.selRolesRadioButton.Location = new System.Drawing.Point(8, 60);
            this.selRolesRadioButton.Name = "selRolesRadioButton";
            this.selRolesRadioButton.Size = new System.Drawing.Size(89, 18);
            this.selRolesRadioButton.TabIndex = 0;
            this.selRolesRadioButton.Text = "These Roles";
            this.selRolesRadioButton.UseVisualStyleBackColor = true;
            this.selRolesRadioButton.CheckedChanged += new System.EventHandler(this.selRolesRadioButton_CheckedChanged);
            // 
            // selUsersRadioButton
            // 
            this.selUsersRadioButton.AutoSize = true;
            this.selUsersRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.selUsersRadioButton.Location = new System.Drawing.Point(8, 39);
            this.selUsersRadioButton.Name = "selUsersRadioButton";
            this.selUsersRadioButton.Size = new System.Drawing.Size(90, 18);
            this.selUsersRadioButton.TabIndex = 0;
            this.selUsersRadioButton.Text = "These Users";
            this.selUsersRadioButton.UseVisualStyleBackColor = true;
            this.selUsersRadioButton.CheckedChanged += new System.EventHandler(this.selUsersRadioButton_CheckedChanged);
            // 
            // _publicRadioButton
            // 
            this._publicRadioButton.AutoSize = true;
            this._publicRadioButton.Checked = true;
            this._publicRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._publicRadioButton.Location = new System.Drawing.Point(8, 16);
            this._publicRadioButton.Name = "_publicRadioButton";
            this._publicRadioButton.Size = new System.Drawing.Size(58, 18);
            this._publicRadioButton.TabIndex = 0;
            this._publicRadioButton.TabStop = true;
            this._publicRadioButton.Text = "Public";
            this._publicRadioButton.UseVisualStyleBackColor = true;
            this._publicRadioButton.CheckedChanged += new System.EventHandler(this._publicRadioButton_CheckedChanged);
            // 
            // buttonsPanel
            // 
            this.buttonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.buttonsPanel.Controls.Add(this.resetButton);
            this.buttonsPanel.Controls.Add(this.applyButton);
            this.buttonsPanel.Controls.Add(this.cancelButton);
            this.buttonsPanel.Controls.Add(this.helpButton);
            this.buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.buttonsPanel.Location = new System.Drawing.Point(0, 660);
            this.buttonsPanel.Name = "buttonsPanel";
            this.buttonsPanel.Size = new System.Drawing.Size(859, 33);
            this.buttonsPanel.TabIndex = 1;
            // 
            // resetButton
            // 
            this.resetButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.resetButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.resetButton.Location = new System.Drawing.Point(3, 5);
            this.resetButton.Name = "resetButton";
            this.resetButton.Size = new System.Drawing.Size(75, 23);
            this.resetButton.TabIndex = 0;
            this.resetButton.Text = "Re&set";
            this.resetButton.UseVisualStyleBackColor = true;
            this.resetButton.Click += new System.EventHandler(this.resetButton_Click);
            // 
            // applyButton
            // 
            this.applyButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.applyButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.applyButton.Location = new System.Drawing.Point(617, 5);
            this.applyButton.Name = "applyButton";
            this.applyButton.Size = new System.Drawing.Size(75, 23);
            this.applyButton.TabIndex = 0;
            this.applyButton.Text = "&Grant";
            this.applyButton.UseVisualStyleBackColor = true;
            this.applyButton.Click += new System.EventHandler(this.applyButton_Click);
            // 
            // cancelButton
            // 
            this.cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cancelButton.Location = new System.Drawing.Point(698, 5);
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = new System.Drawing.Size(75, 23);
            this.cancelButton.TabIndex = 0;
            this.cancelButton.Text = "&Cancel";
            this.cancelButton.UseVisualStyleBackColor = true;
            this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
            // 
            // helpButton
            // 
            this.helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.helpButton.Location = new System.Drawing.Point(779, 5);
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = new System.Drawing.Size(75, 23);
            this.helpButton.TabIndex = 0;
            this.helpButton.Text = "He&lp";
            this.helpButton.UseVisualStyleBackColor = true;
            this.helpButton.Click += new System.EventHandler(this.helpButton_Click);
            // 
            // _toolTip
            // 
            this._toolTip.IsBalloon = true;
            // 
            // GrantRevokeControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.outerSplitContainer);
            this.Name = "GrantRevokeControl";
            this.Size = new System.Drawing.Size(1079, 693);
            this.outerSplitContainer.Panel1.ResumeLayout(false);
            this.outerSplitContainer.Panel2.ResumeLayout(false);
            this.outerSplitContainer.ResumeLayout(false);
            this.actionGroupBox.ResumeLayout(false);
            this.privilegesGroupBox.ResumeLayout(false);
            this.actionTypeGroupBox.ResumeLayout(false);
            this.actionTypeGroupBox.PerformLayout();
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.TrafodionGroupBox1.PerformLayout();
            this.granteesGroupBox.ResumeLayout(false);
            this.granteeListPanel.ResumeLayout(false);
            this._selectUsersGroupBox.ResumeLayout(false);
            this._selectUsersGroupBox.PerformLayout();
            this.buttonsPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer outerSplitContainer;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel buttonsPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton applyButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton helpButton;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox privilegesGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox granteesGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckedListBox granteeListBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox objectsGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionButton resetButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel progressPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel granteeListPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _selectUsersGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton selUsersRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton _publicRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox actionGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox actionTypeGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton revokeRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton _grantRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel privilegesPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox withGrantOptionCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox TrafodionGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox cascadeCheckBox;
        private Framework.Controls.TrafodionRadioButton selRolesRadioButton;
        private Framework.Controls.TrafodionLabel _grantedByLabel;
        private Framework.Controls.TrafodionComboBox _grantedByComboBox;
        private Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Framework.Controls.TrafodionToolTip _toolTip;
    }
}