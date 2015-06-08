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
    partial class TrafodionPrivilegeControl
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
            MyDispose();
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.buttonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.exportButtonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.grantRevokeButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.dataGridPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.buttonsPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // buttonsPanel
            // 
            this.buttonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.buttonsPanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.buttonsPanel.Controls.Add(this.exportButtonPanel);
            this.buttonsPanel.Controls.Add(this.grantRevokeButton);
            this.buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.buttonsPanel.Location = new System.Drawing.Point(0, 403);
            this.buttonsPanel.Name = "buttonsPanel";
            this.buttonsPanel.Size = new System.Drawing.Size(759, 33);
            this.buttonsPanel.TabIndex = 0;
            // 
            // exportButtonPanel
            // 
            this.exportButtonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.exportButtonPanel.Dock = System.Windows.Forms.DockStyle.Right;
            this.exportButtonPanel.Location = new System.Drawing.Point(136, 0);
            this.exportButtonPanel.Name = "exportButtonPanel";
            this.exportButtonPanel.Size = new System.Drawing.Size(621, 31);
            this.exportButtonPanel.TabIndex = 1;
            // 
            // grantRevokeButton
            // 
            this.grantRevokeButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.grantRevokeButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.grantRevokeButton.Location = new System.Drawing.Point(2, 5);
            this.grantRevokeButton.Name = "grantRevokeButton";
            this.grantRevokeButton.Size = new System.Drawing.Size(98, 23);
            this.grantRevokeButton.TabIndex = 0;
            this.grantRevokeButton.Text = "&Grant/Revoke ...";
            this.grantRevokeButton.UseVisualStyleBackColor = true;
            this.grantRevokeButton.Click += new System.EventHandler(this.grantRevokeButton_Click);
            // 
            // dataGridPanel
            // 
            this.dataGridPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.dataGridPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.dataGridPanel.Location = new System.Drawing.Point(0, 0);
            this.dataGridPanel.Name = "dataGridPanel";
            this.dataGridPanel.Size = new System.Drawing.Size(759, 403);
            this.dataGridPanel.TabIndex = 1;
            // 
            // TrafodionPrivilegeControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.dataGridPanel);
            this.Controls.Add(this.buttonsPanel);
            this.Name = "TrafodionPrivilegeControl";
            this.Size = new System.Drawing.Size(759, 436);
            this.buttonsPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel buttonsPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton grantRevokeButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel exportButtonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel dataGridPanel;
    }
}
