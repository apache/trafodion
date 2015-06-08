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
﻿namespace Trafodion.Manager.ConnectivityArea.Controls
{
    partial class NDCSPermissionsUserControl
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
            this._tablePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._bottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._bottomLeftPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._refreshButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._revokeButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._grantButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._bottomRightPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._bottomPanel.SuspendLayout();
            this._bottomLeftPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _tablePanel
            // 
            this._tablePanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._tablePanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this._tablePanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._tablePanel.Location = new System.Drawing.Point(0, 0);
            this._tablePanel.Name = "_tablePanel";
            this._tablePanel.Size = new System.Drawing.Size(786, 398);
            this._tablePanel.TabIndex = 1;
            // 
            // _bottomPanel
            // 
            this._bottomPanel.Controls.Add(this._bottomLeftPanel);
            this._bottomPanel.Controls.Add(this._bottomRightPanel);
            this._bottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._bottomPanel.Location = new System.Drawing.Point(0, 398);
            this._bottomPanel.Name = "_bottomPanel";
            this._bottomPanel.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
            this._bottomPanel.Size = new System.Drawing.Size(786, 31);
            this._bottomPanel.TabIndex = 5;
            // 
            // _bottomLeftPanel
            // 
            this._bottomLeftPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._bottomLeftPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._bottomLeftPanel.Controls.Add(this._refreshButton);
            this._bottomLeftPanel.Controls.Add(this._revokeButton);
            this._bottomLeftPanel.Controls.Add(this._grantButton);
            this._bottomLeftPanel.Location = new System.Drawing.Point(0, 0);
            this._bottomLeftPanel.Name = "_bottomLeftPanel";
            this._bottomLeftPanel.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
            this._bottomLeftPanel.Size = new System.Drawing.Size(221, 31);
            this._bottomLeftPanel.TabIndex = 5;
            // 
            // _refreshButton
            // 
            this._refreshButton.Location = new System.Drawing.Point(146, 5);
            this._refreshButton.Name = "_refreshButton";
            this._refreshButton.Size = new System.Drawing.Size(63, 23);
            this._refreshButton.TabIndex = 0;
            this._refreshButton.Text = "Re&fresh";
            this._refreshButton.UseVisualStyleBackColor = true;
            this._refreshButton.Click += new System.EventHandler(this._refreshButton_Click);
            // 
            // _revokeButton
            // 
            this._revokeButton.Enabled = false;
            this._revokeButton.Location = new System.Drawing.Point(75, 5);
            this._revokeButton.Name = "_revokeButton";
            this._revokeButton.Size = new System.Drawing.Size(63, 23);
            this._revokeButton.TabIndex = 0;
            this._revokeButton.Text = "&Revoke";
            this._revokeButton.UseVisualStyleBackColor = true;
            this._revokeButton.Click += new System.EventHandler(this._revokeButton_Click);
            // 
            // _grantButton
            // 
            this._grantButton.Location = new System.Drawing.Point(3, 5);
            this._grantButton.Name = "_grantButton";
            this._grantButton.Size = new System.Drawing.Size(63, 23);
            this._grantButton.TabIndex = 0;
            this._grantButton.Text = "&Grant";
            this._grantButton.UseVisualStyleBackColor = true;
            this._grantButton.Click += new System.EventHandler(this._grantButton_Click);
            // 
            // _bottomRightPanel
            // 
            this._bottomRightPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._bottomRightPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._bottomRightPanel.Location = new System.Drawing.Point(224, 0);
            this._bottomRightPanel.Name = "_bottomRightPanel";
            this._bottomRightPanel.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this._bottomRightPanel.Size = new System.Drawing.Size(559, 31);
            this._bottomRightPanel.TabIndex = 5;
            // 
            // NDCSPermissionsUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._tablePanel);
            this.Controls.Add(this._bottomPanel);
            this.Name = "HPDCSPermissionsUserControl";
            this.Size = new System.Drawing.Size(786, 429);
            this._bottomPanel.ResumeLayout(false);
            this._bottomLeftPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _tablePanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _bottomPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _bottomRightPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _bottomLeftPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _grantButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _revokeButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _refreshButton;
    }
}
