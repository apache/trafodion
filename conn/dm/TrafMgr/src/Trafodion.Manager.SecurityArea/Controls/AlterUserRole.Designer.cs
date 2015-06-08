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
    partial class AlterUserRole
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
            this._theRoleLabel = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theRoleCombo = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theUserNameText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theMainPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theMainPanel
            // 
            this._theMainPanel.AutoSize = true;
            this._theMainPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._theMainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theMainPanel.Controls.Add(this._theRoleLabel);
            this._theMainPanel.Controls.Add(this._theRoleCombo);
            this._theMainPanel.Controls.Add(this._theNameLabel);
            this._theMainPanel.Controls.Add(this._theUserNameText);
            this._theMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMainPanel.Location = new System.Drawing.Point(0, 0);
            this._theMainPanel.Name = "_theMainPanel";
            this._theMainPanel.Size = new System.Drawing.Size(458, 59);
            this._theMainPanel.TabIndex = 0;
            // 
            // _theRoleLabel
            // 
            this._theRoleLabel.AutoSize = true;
            this._theRoleLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theRoleLabel.Location = new System.Drawing.Point(31, 38);
            this._theRoleLabel.Name = "_theRoleLabel";
            this._theRoleLabel.ShowRequired = true;
            this._theRoleLabel.Size = new System.Drawing.Size(89, 13);
            this._theRoleLabel.TabIndex = 20;
            // 
            // _theRoleCombo
            // 
            this._theRoleCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theRoleCombo.FormattingEnabled = true;
            this._theRoleCombo.Items.AddRange(new object[] {
            "ROLE.MGR",
            "ROLE.SECMGR",
            "SUPER.SERVICES",
            "SUPER.SUPER"});
            this._theRoleCombo.Location = new System.Drawing.Point(120, 35);
            this._theRoleCombo.Name = "_theRoleCombo";
            this._theRoleCombo.Size = new System.Drawing.Size(335, 21);
            this._theRoleCombo.TabIndex = 19;
            // 
            // _theNameLabel
            // 
            this._theNameLabel.AutoSize = true;
            this._theNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theNameLabel.Location = new System.Drawing.Point(31, 11);
            this._theNameLabel.Name = "_theNameLabel";
            this._theNameLabel.ShowRequired = true;
            this._theNameLabel.Size = new System.Drawing.Size(89, 18);
            this._theNameLabel.TabIndex = 17;
            // 
            // _theUserNameText
            // 
            this._theUserNameText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUserNameText.Location = new System.Drawing.Point(120, 8);
            this._theUserNameText.Name = "_theUserNameText";
            this._theUserNameText.ReadOnly = true;
            this._theUserNameText.Size = new System.Drawing.Size(335, 21);
            this._theUserNameText.TabIndex = 18;
            // 
            // AlterUserRole
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Controls.Add(this._theMainPanel);
            this.Name = "AlterUserRole";
            this.Size = new System.Drawing.Size(458, 59);
            this._theMainPanel.ResumeLayout(false);
            this._theMainPanel.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theMainPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _theNameLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theUserNameText;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _theRoleLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _theRoleCombo;
    }
}
