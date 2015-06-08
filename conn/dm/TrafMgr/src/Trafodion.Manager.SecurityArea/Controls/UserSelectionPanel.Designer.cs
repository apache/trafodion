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
            this._theMainPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.oneGuiGroupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theAdditionalUsersPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._userListPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theAdditionalUsersBtn = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theUsersGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theTypedUserName = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.oneGuiLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theMainPanel.SuspendLayout();
            this.oneGuiGroupBox2.SuspendLayout();
            this._theAdditionalUsersPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theMainPanel
            // 
            this._theMainPanel.AutoSize = true;
            this._theMainPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._theMainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theMainPanel.Controls.Add(this.oneGuiGroupBox2);
            this._theMainPanel.Controls.Add(this._theUsersGroupBox);
            this._theMainPanel.Controls.Add(this._theTypedUserName);
            this._theMainPanel.Controls.Add(this.oneGuiLabel1);
            this._theMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMainPanel.Location = new System.Drawing.Point(0, 0);
            this._theMainPanel.Name = "_theMainPanel";
            this._theMainPanel.Size = new System.Drawing.Size(519, 536);
            this._theMainPanel.TabIndex = 0;
            // 
            // oneGuiGroupBox2
            // 
            this.oneGuiGroupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.oneGuiGroupBox2.Controls.Add(this._theAdditionalUsersPanel);
            this.oneGuiGroupBox2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox2.Location = new System.Drawing.Point(9, 312);
            this.oneGuiGroupBox2.Name = "oneGuiGroupBox2";
            this.oneGuiGroupBox2.Size = new System.Drawing.Size(501, 221);
            this.oneGuiGroupBox2.TabIndex = 3;
            this.oneGuiGroupBox2.TabStop = false;
            this.oneGuiGroupBox2.Text = "Selected Users";
            // 
            // _theAdditionalUsersPanel
            // 
            this._theAdditionalUsersPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAdditionalUsersPanel.Controls.Add(this._userListPanel);
            this._theAdditionalUsersPanel.Controls.Add(this._theAdditionalUsersBtn);
            this._theAdditionalUsersPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAdditionalUsersPanel.Location = new System.Drawing.Point(3, 17);
            this._theAdditionalUsersPanel.Name = "_theAdditionalUsersPanel";
            this._theAdditionalUsersPanel.Size = new System.Drawing.Size(495, 201);
            this._theAdditionalUsersPanel.TabIndex = 5;
            // 
            // _userListPanel
            // 
            this._userListPanel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._userListPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._userListPanel.Location = new System.Drawing.Point(122, 3);
            this._userListPanel.Name = "_userListPanel";
            this._userListPanel.Size = new System.Drawing.Size(370, 195);
            this._userListPanel.TabIndex = 1;
            // 
            // _theAdditionalUsersBtn
            // 
            this._theAdditionalUsersBtn.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAdditionalUsersBtn.Location = new System.Drawing.Point(3, 3);
            this._theAdditionalUsersBtn.Name = "_theAdditionalUsersBtn";
            this._theAdditionalUsersBtn.Size = new System.Drawing.Size(113, 23);
            this._theAdditionalUsersBtn.TabIndex = 0;
            this._theAdditionalUsersBtn.Text = "Additional Users ->";
            this._theAdditionalUsersBtn.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this._theAdditionalUsersBtn.UseVisualStyleBackColor = true;
            this._theAdditionalUsersBtn.Click += new System.EventHandler(this._theAdditionalUsersBtn_Click);
            // 
            // _theUsersGroupBox
            // 
            this._theUsersGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._theUsersGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUsersGroupBox.Location = new System.Drawing.Point(9, 55);
            this._theUsersGroupBox.Name = "_theUsersGroupBox";
            this._theUsersGroupBox.Size = new System.Drawing.Size(501, 251);
            this._theUsersGroupBox.TabIndex = 2;
            this._theUsersGroupBox.TabStop = false;
            this._theUsersGroupBox.Text = "Users";
            // 
            // _theTypedUserName
            // 
            this._theTypedUserName.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theTypedUserName.Location = new System.Drawing.Point(9, 28);
            this._theTypedUserName.Name = "_theTypedUserName";
            this._theTypedUserName.Size = new System.Drawing.Size(171, 21);
            this._theTypedUserName.TabIndex = 1;
            this._theTypedUserName.TextChanged += new System.EventHandler(this._theTypedUserName_TextChanged);
            // 
            // oneGuiLabel1
            // 
            this.oneGuiLabel1.AutoSize = true;
            this.oneGuiLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel1.Location = new System.Drawing.Point(6, 12);
            this.oneGuiLabel1.Name = "oneGuiLabel1";
            this.oneGuiLabel1.Size = new System.Drawing.Size(177, 13);
            this.oneGuiLabel1.TabIndex = 0;
            this.oneGuiLabel1.Text = "Type User Name or Select From List";
            // 
            // UserSelectionPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theMainPanel);
            this.Name = "UserSelectionPanel";
            this.Size = new System.Drawing.Size(519, 536);
            this._theMainPanel.ResumeLayout(false);
            this._theMainPanel.PerformLayout();
            this.oneGuiGroupBox2.ResumeLayout(false);
            this._theAdditionalUsersPanel.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theMainPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theTypedUserName;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _theUsersGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox2;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theAdditionalUsersBtn;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theAdditionalUsersPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _userListPanel;
    }
}
