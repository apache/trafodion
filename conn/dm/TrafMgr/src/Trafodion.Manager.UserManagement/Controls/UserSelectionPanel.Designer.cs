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
    partial class UserSelectionPanel
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
            this._theAdditionalUsersBtn = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theUsersGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theTypedUserName = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._userListPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theAdditionalUsersPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theDelUserBtn = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.TrafodionGroupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theMainPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._userNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._userNameFilterPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theAdditionalUsersPanel.SuspendLayout();
            this.TrafodionGroupBox2.SuspendLayout();
            this._theMainPanel.SuspendLayout();
            this._userNameFilterPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theAdditionalUsersBtn
            // 
            this._theAdditionalUsersBtn.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAdditionalUsersBtn.Location = new System.Drawing.Point(3, 3);
            this._theAdditionalUsersBtn.Name = "_theAdditionalUsersBtn";
            this._theAdditionalUsersBtn.Size = new System.Drawing.Size(113, 23);
            this._theAdditionalUsersBtn.TabIndex = 0;
            this._theAdditionalUsersBtn.Text = "Add ->";
            this._theAdditionalUsersBtn.UseVisualStyleBackColor = true;
            this._theAdditionalUsersBtn.Click += new System.EventHandler(this._theAdditionalUsersBtn_Click);
            // 
            // _theUsersGroupBox
            // 
            this._theUsersGroupBox.AutoSize = true;
            this._theUsersGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theUsersGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUsersGroupBox.Location = new System.Drawing.Point(0, 61);
            this._theUsersGroupBox.Name = "_theUsersGroupBox";
            this._theUsersGroupBox.Size = new System.Drawing.Size(577, 254);
            this._theUsersGroupBox.TabIndex = 2;
            this._theUsersGroupBox.TabStop = false;
            this._theUsersGroupBox.Text = "Users";
            // 
            // _theTypedUserName
            // 
            this._theTypedUserName.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theTypedUserName.Location = new System.Drawing.Point(6, 25);
            this._theTypedUserName.Name = "_theTypedUserName";
            this._theTypedUserName.Size = new System.Drawing.Size(553, 21);
            this._theTypedUserName.TabIndex = 1;
            this._theTypedUserName.TextChanged += new System.EventHandler(this._theTypedUserName_TextChanged);
            // 
            // _userListPanel
            // 
            this._userListPanel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._userListPanel.AutoSize = true;
            this._userListPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._userListPanel.Location = new System.Drawing.Point(122, 3);
            this._userListPanel.Name = "_userListPanel";
            this._userListPanel.Size = new System.Drawing.Size(446, 172);
            this._userListPanel.TabIndex = 1;
            // 
            // _theAdditionalUsersPanel
            // 
            this._theAdditionalUsersPanel.AutoSize = true;
            this._theAdditionalUsersPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAdditionalUsersPanel.Controls.Add(this._theDelUserBtn);
            this._theAdditionalUsersPanel.Controls.Add(this._userListPanel);
            this._theAdditionalUsersPanel.Controls.Add(this._theAdditionalUsersBtn);
            this._theAdditionalUsersPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAdditionalUsersPanel.Location = new System.Drawing.Point(3, 17);
            this._theAdditionalUsersPanel.Name = "_theAdditionalUsersPanel";
            this._theAdditionalUsersPanel.Size = new System.Drawing.Size(571, 178);
            this._theAdditionalUsersPanel.TabIndex = 5;
            // 
            // _theDelUserBtn
            // 
            this._theDelUserBtn.Enabled = false;
            this._theDelUserBtn.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDelUserBtn.Location = new System.Drawing.Point(3, 43);
            this._theDelUserBtn.Name = "_theDelUserBtn";
            this._theDelUserBtn.Size = new System.Drawing.Size(113, 23);
            this._theDelUserBtn.TabIndex = 2;
            this._theDelUserBtn.Text = "<- Remove";
            this._theDelUserBtn.UseVisualStyleBackColor = true;
            this._theDelUserBtn.Click += new System.EventHandler(this._theDelUserBtn_Click);
            // 
            // TrafodionGroupBox2
            // 
            this.TrafodionGroupBox2.AutoSize = true;
            this.TrafodionGroupBox2.Controls.Add(this._theAdditionalUsersPanel);
            this.TrafodionGroupBox2.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionGroupBox2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox2.Location = new System.Drawing.Point(0, 315);
            this.TrafodionGroupBox2.Name = "TrafodionGroupBox2";
            this.TrafodionGroupBox2.Size = new System.Drawing.Size(577, 198);
            this.TrafodionGroupBox2.TabIndex = 3;
            this.TrafodionGroupBox2.TabStop = false;
            this.TrafodionGroupBox2.Text = "Selected Users";
            // 
            // _theMainPanel
            // 
            this._theMainPanel.AutoScroll = true;
            this._theMainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theMainPanel.Controls.Add(this._theUsersGroupBox);
            this._theMainPanel.Controls.Add(this._userNameFilterPanel);
            this._theMainPanel.Controls.Add(this.TrafodionGroupBox2);
            this._theMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMainPanel.Location = new System.Drawing.Point(0, 0);
            this._theMainPanel.Name = "_theMainPanel";
            this._theMainPanel.Size = new System.Drawing.Size(577, 513);
            this._theMainPanel.TabIndex = 1;
            // 
            // _userNameLabel
            // 
            this._userNameLabel.AutoSize = true;
            this._userNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._userNameLabel.Location = new System.Drawing.Point(3, 6);
            this._userNameLabel.Name = "_userNameLabel";
            this._userNameLabel.Size = new System.Drawing.Size(177, 13);
            this._userNameLabel.TabIndex = 0;
            this._userNameLabel.Text = "Type User Name or Select From List";
            // 
            // _userNameFilterPanel
            // 
            this._userNameFilterPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._userNameFilterPanel.Controls.Add(this._userNameLabel);
            this._userNameFilterPanel.Controls.Add(this._theTypedUserName);
            this._userNameFilterPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._userNameFilterPanel.Location = new System.Drawing.Point(0, 0);
            this._userNameFilterPanel.Name = "_userNameFilterPanel";
            this._userNameFilterPanel.Size = new System.Drawing.Size(577, 61);
            this._userNameFilterPanel.TabIndex = 4;
            // 
            // UserSelectionPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.Controls.Add(this._theMainPanel);
            this.Name = "UserSelectionPanel";
            this.Size = new System.Drawing.Size(577, 513);
            this._theAdditionalUsersPanel.ResumeLayout(false);
            this._theAdditionalUsersPanel.PerformLayout();
            this.TrafodionGroupBox2.ResumeLayout(false);
            this.TrafodionGroupBox2.PerformLayout();
            this._theMainPanel.ResumeLayout(false);
            this._theMainPanel.PerformLayout();
            this._userNameFilterPanel.ResumeLayout(false);
            this._userNameFilterPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionButton _theAdditionalUsersBtn;
        private Framework.Controls.TrafodionGroupBox _theUsersGroupBox;
        private Framework.Controls.TrafodionTextBox _theTypedUserName;
        private Framework.Controls.TrafodionPanel _userListPanel;
        private Framework.Controls.TrafodionPanel _theAdditionalUsersPanel;
        private Framework.Controls.TrafodionGroupBox TrafodionGroupBox2;
        private Framework.Controls.TrafodionPanel _theMainPanel;
        private Framework.Controls.TrafodionLabel _userNameLabel;
        private Framework.Controls.TrafodionPanel _userNameFilterPanel;
        private Framework.Controls.TrafodionButton _theDelUserBtn;
    }
}
