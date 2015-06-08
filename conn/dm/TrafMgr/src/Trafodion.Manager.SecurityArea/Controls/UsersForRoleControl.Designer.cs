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
    partial class UsersForRoleControl
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
            this._bottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theAdditionalButtonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._revokeButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._grantButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theExportButtonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theWidgetPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._bottomPanel.SuspendLayout();
            this._theAdditionalButtonPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _bottomPanel
            // 
            this._bottomPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._bottomPanel.Controls.Add(this._theAdditionalButtonPanel);
            this._bottomPanel.Controls.Add(this._theExportButtonPanel);
            this._bottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._bottomPanel.Location = new System.Drawing.Point(0, 371);
            this._bottomPanel.Name = "_bottomPanel";
            this._bottomPanel.Size = new System.Drawing.Size(814, 33);
            this._bottomPanel.TabIndex = 0;
            // 
            // _theAdditionalButtonPanel
            // 
            this._theAdditionalButtonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAdditionalButtonPanel.Controls.Add(this._revokeButton);
            this._theAdditionalButtonPanel.Controls.Add(this._grantButton);
            this._theAdditionalButtonPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAdditionalButtonPanel.Location = new System.Drawing.Point(0, 0);
            this._theAdditionalButtonPanel.Name = "_theAdditionalButtonPanel";
            this._theAdditionalButtonPanel.Size = new System.Drawing.Size(174, 33);
            this._theAdditionalButtonPanel.TabIndex = 1;
            // 
            // _revokeButton
            // 
            this._revokeButton.AutoSize = true;
            this._revokeButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._revokeButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._revokeButton.Location = new System.Drawing.Point(79, 5);
            this._revokeButton.Name = "_revokeButton";
            this._revokeButton.Size = new System.Drawing.Size(70, 23);
            this._revokeButton.TabIndex = 1;
            this._revokeButton.Text = "&Revoke";
            this._revokeButton.UseVisualStyleBackColor = true;
            this._revokeButton.Click += new System.EventHandler(this._revokeButton_Click);
            // 
            // _grantButton
            // 
            this._grantButton.AutoSize = true;
            this._grantButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._grantButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._grantButton.Location = new System.Drawing.Point(6, 5);
            this._grantButton.Name = "_grantButton";
            this._grantButton.Size = new System.Drawing.Size(70, 23);
            this._grantButton.TabIndex = 0;
            this._grantButton.Text = "&Grant";
            this._grantButton.UseVisualStyleBackColor = true;
            this._grantButton.Click += new System.EventHandler(this._grantButton_Click);
            // 
            // _theExportButtonPanel
            // 
            this._theExportButtonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theExportButtonPanel.Dock = System.Windows.Forms.DockStyle.Right;
            this._theExportButtonPanel.Location = new System.Drawing.Point(174, 0);
            this._theExportButtonPanel.Name = "_theExportButtonPanel";
            this._theExportButtonPanel.Size = new System.Drawing.Size(640, 33);
            this._theExportButtonPanel.TabIndex = 0;
            // 
            // _theWidgetPanel
            // 
            this._theWidgetPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theWidgetPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theWidgetPanel.Location = new System.Drawing.Point(0, 0);
            this._theWidgetPanel.Name = "_theWidgetPanel";
            this._theWidgetPanel.Size = new System.Drawing.Size(814, 371);
            this._theWidgetPanel.TabIndex = 1;
            // 
            // UsersForRoleControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._theWidgetPanel);
            this.Controls.Add(this._bottomPanel);
            this.Name = "UsersForRoleControl";
            this.Size = new System.Drawing.Size(814, 404);
            this._bottomPanel.ResumeLayout(false);
            this._theAdditionalButtonPanel.ResumeLayout(false);
            this._theAdditionalButtonPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _bottomPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theExportButtonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theAdditionalButtonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _revokeButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theWidgetPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _grantButton;




    }
}
