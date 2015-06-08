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
    partial class ComponentPrivilegesSelectionPanel
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
            this.TrafodionPanelMain = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._privilegesGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionGroupBoxSelectedPrivileges = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theAdditionalUsersPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._userListPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theDelPrivilegesBtn = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theAddPrivilegesBtn = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theTypedPrivilegesName = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabelSearch = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionGroupBoxSelectedPrivileges.SuspendLayout();
            this._theAdditionalUsersPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionPanelMain
            // 
            this.TrafodionPanelMain.AutoSize = true;
            this.TrafodionPanelMain.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.TrafodionPanelMain.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanelMain.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanelMain.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanelMain.Name = "TrafodionPanelMain";
            this.TrafodionPanelMain.Size = new System.Drawing.Size(467, 502);
            this.TrafodionPanelMain.TabIndex = 0;
            // 
            // _privilegesGroupBox
            // 
            this._privilegesGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._privilegesGroupBox.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._privilegesGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._privilegesGroupBox.Location = new System.Drawing.Point(7, 45);
            this._privilegesGroupBox.Name = "_privilegesGroupBox";
            this._privilegesGroupBox.Size = new System.Drawing.Size(450, 250);
            this._privilegesGroupBox.TabIndex = 12;
            this._privilegesGroupBox.TabStop = false;
            this._privilegesGroupBox.Text = "Privileges";
            // 
            // TrafodionGroupBoxSelectedPrivileges
            // 
            this.TrafodionGroupBoxSelectedPrivileges.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.TrafodionGroupBoxSelectedPrivileges.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.TrafodionGroupBoxSelectedPrivileges.Controls.Add(this._theAdditionalUsersPanel);
            this.TrafodionGroupBoxSelectedPrivileges.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBoxSelectedPrivileges.Location = new System.Drawing.Point(10, 301);
            this.TrafodionGroupBoxSelectedPrivileges.Name = "TrafodionGroupBoxSelectedPrivileges";
            this.TrafodionGroupBoxSelectedPrivileges.Size = new System.Drawing.Size(450, 198);
            this.TrafodionGroupBoxSelectedPrivileges.TabIndex = 11;
            this.TrafodionGroupBoxSelectedPrivileges.TabStop = false;
            this.TrafodionGroupBoxSelectedPrivileges.Text = "Selected Privileges";
            // 
            // _theAdditionalUsersPanel
            // 
            this._theAdditionalUsersPanel.AutoSize = true;
            this._theAdditionalUsersPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAdditionalUsersPanel.Controls.Add(this._userListPanel);
            this._theAdditionalUsersPanel.Controls.Add(this._theDelPrivilegesBtn);
            this._theAdditionalUsersPanel.Controls.Add(this._theAddPrivilegesBtn);
            this._theAdditionalUsersPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAdditionalUsersPanel.Location = new System.Drawing.Point(3, 17);
            this._theAdditionalUsersPanel.Name = "_theAdditionalUsersPanel";
            this._theAdditionalUsersPanel.Size = new System.Drawing.Size(444, 178);
            this._theAdditionalUsersPanel.TabIndex = 5;
            // 
            // _userListPanel
            // 
            this._userListPanel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._userListPanel.AutoSize = true;
            this._userListPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._userListPanel.Location = new System.Drawing.Point(122, 3);
            this._userListPanel.Name = "_userListPanel";
            this._userListPanel.Size = new System.Drawing.Size(319, 172);
            this._userListPanel.TabIndex = 1;
            // 
            // _theDelPrivilegesBtn
            // 
            this._theDelPrivilegesBtn.Enabled = false;
            this._theDelPrivilegesBtn.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDelPrivilegesBtn.Location = new System.Drawing.Point(3, 44);
            this._theDelPrivilegesBtn.Name = "_theDelPrivilegesBtn";
            this._theDelPrivilegesBtn.Size = new System.Drawing.Size(113, 23);
            this._theDelPrivilegesBtn.TabIndex = 0;
            this._theDelPrivilegesBtn.Text = "<- Remove";
            this._theDelPrivilegesBtn.UseVisualStyleBackColor = true;
            this._theDelPrivilegesBtn.Click += new System.EventHandler(this._theDelPrivilegesBtn_Click);
            // 
            // _theAddPrivilegesBtn
            // 
            this._theAddPrivilegesBtn.Enabled = false;
            this._theAddPrivilegesBtn.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAddPrivilegesBtn.Location = new System.Drawing.Point(3, 3);
            this._theAddPrivilegesBtn.Name = "_theAddPrivilegesBtn";
            this._theAddPrivilegesBtn.Size = new System.Drawing.Size(113, 23);
            this._theAddPrivilegesBtn.TabIndex = 0;
            this._theAddPrivilegesBtn.Text = "Add ->";
            this._theAddPrivilegesBtn.UseVisualStyleBackColor = true;
            this._theAddPrivilegesBtn.Click += new System.EventHandler(this._theAddPrivilegesBtn_Click);
            // 
            // _theTypedPrivilegesName
            // 
            this._theTypedPrivilegesName.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theTypedPrivilegesName.Location = new System.Drawing.Point(10, 19);
            this._theTypedPrivilegesName.Name = "_theTypedPrivilegesName";
            this._theTypedPrivilegesName.Size = new System.Drawing.Size(212, 21);
            this._theTypedPrivilegesName.TabIndex = 10;
            this._theTypedPrivilegesName.TextChanged += new System.EventHandler(this._theTypedPrivilegesName_TextChanged);
            // 
            // TrafodionLabelSearch
            // 
            this.TrafodionLabelSearch.AutoSize = true;
            this.TrafodionLabelSearch.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabelSearch.Location = new System.Drawing.Point(7, 2);
            this.TrafodionLabelSearch.Name = "TrafodionLabelSearch";
            this.TrafodionLabelSearch.Size = new System.Drawing.Size(195, 13);
            this.TrafodionLabelSearch.TabIndex = 9;
            this.TrafodionLabelSearch.Text = "Type Privilege Name or Select From List";
            // 
            // ComponentPrivilegesSelectionPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.Controls.Add(this._privilegesGroupBox);
            this.Controls.Add(this.TrafodionGroupBoxSelectedPrivileges);
            this.Controls.Add(this._theTypedPrivilegesName);
            this.Controls.Add(this.TrafodionLabelSearch);
            this.Controls.Add(this.TrafodionPanelMain);
            this.Name = "ComponentPrivilegesSelectionPanel";
            this.Size = new System.Drawing.Size(467, 502);
            this.Load += new System.EventHandler(this.ComponentPrivilegesSelectionPanel_Load);
            this.TrafodionGroupBoxSelectedPrivileges.ResumeLayout(false);
            this.TrafodionGroupBoxSelectedPrivileges.PerformLayout();
            this._theAdditionalUsersPanel.ResumeLayout(false);
            this._theAdditionalUsersPanel.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Framework.Controls.TrafodionPanel TrafodionPanelMain;
        private Framework.Controls.TrafodionGroupBox _privilegesGroupBox;
        private Framework.Controls.TrafodionGroupBox TrafodionGroupBoxSelectedPrivileges;
        private Framework.Controls.TrafodionPanel _theAdditionalUsersPanel;
        private Framework.Controls.TrafodionPanel _userListPanel;
        private Framework.Controls.TrafodionButton _theAddPrivilegesBtn;
        private Framework.Controls.TrafodionTextBox _theTypedPrivilegesName;
        private Framework.Controls.TrafodionLabel TrafodionLabelSearch;
        private Framework.Controls.TrafodionButton _theDelPrivilegesBtn;

    }
}
