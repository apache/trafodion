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
    partial class GranteeSelectionPanel
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
            this._granteeGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionGroupBoxSelectedGrantees = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theAdditionalUsersPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._granteeListPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theDelGranteesBtn = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theAddGranteesBtn = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.TrafodionPanelMain = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._thePublicRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._theRolesRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._theUserRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._theAllRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._theTypedGranteeName = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabelSearch = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionGroupBoxSelectedGrantees.SuspendLayout();
            this._theAdditionalUsersPanel.SuspendLayout();
            this.TrafodionPanelMain.SuspendLayout();
            this.SuspendLayout();
            // 
            // _granteeGroupBox
            // 
            this._granteeGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._granteeGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._granteeGroupBox.Location = new System.Drawing.Point(0, 44);
            this._granteeGroupBox.Name = "_granteeGroupBox";
            this._granteeGroupBox.Size = new System.Drawing.Size(593, 270);
            this._granteeGroupBox.TabIndex = 17;
            this._granteeGroupBox.TabStop = false;
            this._granteeGroupBox.Text = "Grantees";
            // 
            // TrafodionGroupBoxSelectedGrantees
            // 
            this.TrafodionGroupBoxSelectedGrantees.Controls.Add(this._theAdditionalUsersPanel);
            this.TrafodionGroupBoxSelectedGrantees.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionGroupBoxSelectedGrantees.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBoxSelectedGrantees.Location = new System.Drawing.Point(0, 314);
            this.TrafodionGroupBoxSelectedGrantees.Name = "TrafodionGroupBoxSelectedGrantees";
            this.TrafodionGroupBoxSelectedGrantees.Size = new System.Drawing.Size(593, 198);
            this.TrafodionGroupBoxSelectedGrantees.TabIndex = 16;
            this.TrafodionGroupBoxSelectedGrantees.TabStop = false;
            this.TrafodionGroupBoxSelectedGrantees.Text = "Selected Grantees";
            // 
            // _theAdditionalUsersPanel
            // 
            this._theAdditionalUsersPanel.AutoSize = true;
            this._theAdditionalUsersPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAdditionalUsersPanel.Controls.Add(this._granteeListPanel);
            this._theAdditionalUsersPanel.Controls.Add(this._theDelGranteesBtn);
            this._theAdditionalUsersPanel.Controls.Add(this._theAddGranteesBtn);
            this._theAdditionalUsersPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAdditionalUsersPanel.Location = new System.Drawing.Point(3, 17);
            this._theAdditionalUsersPanel.Name = "_theAdditionalUsersPanel";
            this._theAdditionalUsersPanel.Size = new System.Drawing.Size(587, 178);
            this._theAdditionalUsersPanel.TabIndex = 5;
            // 
            // _granteeListPanel
            // 
            this._granteeListPanel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._granteeListPanel.AutoSize = true;
            this._granteeListPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._granteeListPanel.Location = new System.Drawing.Point(122, 3);
            this._granteeListPanel.Name = "_granteeListPanel";
            this._granteeListPanel.Size = new System.Drawing.Size(462, 172);
            this._granteeListPanel.TabIndex = 1;
            // 
            // _theDelGranteesBtn
            // 
            this._theDelGranteesBtn.Enabled = false;
            this._theDelGranteesBtn.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDelGranteesBtn.Location = new System.Drawing.Point(3, 41);
            this._theDelGranteesBtn.Name = "_theDelGranteesBtn";
            this._theDelGranteesBtn.Size = new System.Drawing.Size(113, 23);
            this._theDelGranteesBtn.TabIndex = 0;
            this._theDelGranteesBtn.Text = "<- Remove";
            this._theDelGranteesBtn.UseVisualStyleBackColor = true;
            this._theDelGranteesBtn.Click += new System.EventHandler(this._theDelGranteesBtn_Click);
            // 
            // _theAddGranteesBtn
            // 
            this._theAddGranteesBtn.Enabled = false;
            this._theAddGranteesBtn.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAddGranteesBtn.Location = new System.Drawing.Point(3, 3);
            this._theAddGranteesBtn.Name = "_theAddGranteesBtn";
            this._theAddGranteesBtn.Size = new System.Drawing.Size(113, 23);
            this._theAddGranteesBtn.TabIndex = 0;
            this._theAddGranteesBtn.Text = "Add ->";
            this._theAddGranteesBtn.UseVisualStyleBackColor = true;
            this._theAddGranteesBtn.Click += new System.EventHandler(this._theAddGranteesBtn_Click);
            // 
            // TrafodionPanelMain
            // 
            this.TrafodionPanelMain.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanelMain.Controls.Add(this._thePublicRadioButton);
            this.TrafodionPanelMain.Controls.Add(this._theRolesRadioButton);
            this.TrafodionPanelMain.Controls.Add(this._theUserRadioButton);
            this.TrafodionPanelMain.Controls.Add(this._theAllRadioButton);
            this.TrafodionPanelMain.Controls.Add(this._theTypedGranteeName);
            this.TrafodionPanelMain.Controls.Add(this.TrafodionLabelSearch);
            this.TrafodionPanelMain.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionPanelMain.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanelMain.Name = "TrafodionPanelMain";
            this.TrafodionPanelMain.Size = new System.Drawing.Size(593, 44);
            this.TrafodionPanelMain.TabIndex = 13;
            // 
            // _thePublicRadioButton
            // 
            this._thePublicRadioButton.AutoSize = true;
            this._thePublicRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._thePublicRadioButton.Location = new System.Drawing.Point(158, 20);
            this._thePublicRadioButton.Name = "_thePublicRadioButton";
            this._thePublicRadioButton.Size = new System.Drawing.Size(60, 18);
            this._thePublicRadioButton.TabIndex = 18;
            this._thePublicRadioButton.TabStop = true;
            this._thePublicRadioButton.Text = "Public";
            this._thePublicRadioButton.UseVisualStyleBackColor = true;
            this._thePublicRadioButton.Click += new System.EventHandler(this.radioButtonFilter_Click);
            // 
            // _theRolesRadioButton
            // 
            this._theRolesRadioButton.AutoSize = true;
            this._theRolesRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theRolesRadioButton.Location = new System.Drawing.Point(105, 20);
            this._theRolesRadioButton.Name = "_theRolesRadioButton";
            this._theRolesRadioButton.Size = new System.Drawing.Size(58, 18);
            this._theRolesRadioButton.TabIndex = 18;
            this._theRolesRadioButton.TabStop = true;
            this._theRolesRadioButton.Text = "Roles";
            this._theRolesRadioButton.UseVisualStyleBackColor = true;
            this._theRolesRadioButton.Click += new System.EventHandler(this.radioButtonFilter_Click);
            // 
            // _theUserRadioButton
            // 
            this._theUserRadioButton.AutoSize = true;
            this._theUserRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theUserRadioButton.Location = new System.Drawing.Point(50, 20);
            this._theUserRadioButton.Name = "_theUserRadioButton";
            this._theUserRadioButton.Size = new System.Drawing.Size(58, 18);
            this._theUserRadioButton.TabIndex = 17;
            this._theUserRadioButton.TabStop = true;
            this._theUserRadioButton.Text = "Users";
            this._theUserRadioButton.UseVisualStyleBackColor = true;
            this._theUserRadioButton.Click += new System.EventHandler(this.radioButtonFilter_Click);
            // 
            // _theAllRadioButton
            // 
            this._theAllRadioButton.AutoSize = true;
            this._theAllRadioButton.Checked = true;
            this._theAllRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theAllRadioButton.Location = new System.Drawing.Point(11, 20);
            this._theAllRadioButton.Name = "_theAllRadioButton";
            this._theAllRadioButton.Size = new System.Drawing.Size(42, 18);
            this._theAllRadioButton.TabIndex = 16;
            this._theAllRadioButton.TabStop = true;
            this._theAllRadioButton.Text = "All";
            this._theAllRadioButton.UseVisualStyleBackColor = true;
            this._theAllRadioButton.Click += new System.EventHandler(this.radioButtonFilter_Click);
            // 
            // _theTypedGranteeName
            // 
            this._theTypedGranteeName.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theTypedGranteeName.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theTypedGranteeName.Location = new System.Drawing.Point(248, 20);
            this._theTypedGranteeName.Name = "_theTypedGranteeName";
            this._theTypedGranteeName.Size = new System.Drawing.Size(336, 21);
            this._theTypedGranteeName.TabIndex = 15;
            this._theTypedGranteeName.TextChanged += new System.EventHandler(this._theTypedGranteeName_TextChanged);
            // 
            // TrafodionLabelSearch
            // 
            this.TrafodionLabelSearch.AutoSize = true;
            this.TrafodionLabelSearch.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabelSearch.Location = new System.Drawing.Point(243, 3);
            this.TrafodionLabelSearch.Name = "TrafodionLabelSearch";
            this.TrafodionLabelSearch.Size = new System.Drawing.Size(194, 13);
            this.TrafodionLabelSearch.TabIndex = 14;
            this.TrafodionLabelSearch.Text = "Type Grantee Name or Select From List";
            // 
            // GranteeSelectionPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._granteeGroupBox);
            this.Controls.Add(this.TrafodionGroupBoxSelectedGrantees);
            this.Controls.Add(this.TrafodionPanelMain);
            this.Name = "GranteeSelectionPanel";
            this.Size = new System.Drawing.Size(593, 512);
            this.Load += new System.EventHandler(this.GranteeSelectionPanel_Load);
            this.TrafodionGroupBoxSelectedGrantees.ResumeLayout(false);
            this.TrafodionGroupBoxSelectedGrantees.PerformLayout();
            this._theAdditionalUsersPanel.ResumeLayout(false);
            this._theAdditionalUsersPanel.PerformLayout();
            this.TrafodionPanelMain.ResumeLayout(false);
            this.TrafodionPanelMain.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionPanel _granteeListPanel;
        private Framework.Controls.TrafodionTextBox _theTypedGranteeName;
        private Framework.Controls.TrafodionButton _theDelGranteesBtn;
        private Framework.Controls.TrafodionButton _theAddGranteesBtn;
        private Framework.Controls.TrafodionPanel _theAdditionalUsersPanel;
        private Framework.Controls.TrafodionGroupBox _granteeGroupBox;
        private Framework.Controls.TrafodionGroupBox TrafodionGroupBoxSelectedGrantees;
        private Framework.Controls.TrafodionLabel TrafodionLabelSearch;
        private Framework.Controls.TrafodionPanel TrafodionPanelMain;
        private Framework.Controls.TrafodionRadioButton _theRolesRadioButton;
        private Framework.Controls.TrafodionRadioButton _theUserRadioButton;
        private Framework.Controls.TrafodionRadioButton _theAllRadioButton;
        private Framework.Controls.TrafodionRadioButton _thePublicRadioButton;

    }
}
