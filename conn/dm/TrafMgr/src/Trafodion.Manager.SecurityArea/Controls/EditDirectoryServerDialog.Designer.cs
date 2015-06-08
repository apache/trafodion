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
    partial class EditDirectoryServerDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(EditDirectoryServerDialog));
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theReloadButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theApplyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theCancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._configureDirectoryServerUserControl = new Trafodion.Manager.SecurityArea.Controls.ConfigureDirectoryServerUserControl();
            this.oneGuiPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.oneGuiPanel1.Controls.Add(this._theReloadButton);
            this.oneGuiPanel1.Controls.Add(this._theApplyButton);
            this.oneGuiPanel1.Controls.Add(this._theCancelButton);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 497);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(686, 36);
            this.oneGuiPanel1.TabIndex = 1;
            // 
            // _theReloadButton
            // 
            this._theReloadButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theReloadButton.Location = new System.Drawing.Point(10, 6);
            this._theReloadButton.Name = "_theReloadButton";
            this._theReloadButton.Size = new System.Drawing.Size(84, 23);
            this._theReloadButton.TabIndex = 2;
            this._theReloadButton.Text = "&Reload";
            this._theReloadButton.UseVisualStyleBackColor = true;
            this._theReloadButton.Click += new System.EventHandler(this._theReloadButton_Click);
            // 
            // _theApplyButton
            // 
            this._theApplyButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._theApplyButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theApplyButton.Location = new System.Drawing.Point(489, 6);
            this._theApplyButton.Name = "_theApplyButton";
            this._theApplyButton.Size = new System.Drawing.Size(84, 23);
            this._theApplyButton.TabIndex = 1;
            this._theApplyButton.Text = "&Apply";
            this._theApplyButton.UseVisualStyleBackColor = true;
            this._theApplyButton.Click += new System.EventHandler(this._theApplyButton_Click);
            // 
            // _theCancelButton
            // 
            this._theCancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._theCancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCancelButton.Location = new System.Drawing.Point(588, 6);
            this._theCancelButton.Name = "_theCancelButton";
            this._theCancelButton.Size = new System.Drawing.Size(84, 23);
            this._theCancelButton.TabIndex = 0;
            this._theCancelButton.Text = "&Cancel";
            this._theCancelButton.UseVisualStyleBackColor = true;
            this._theCancelButton.Click += new System.EventHandler(this._theCancelButton_Click);
            // 
            // _configureDirectoryServerUserControl
            // 
            this._configureDirectoryServerUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._configureDirectoryServerUserControl.Location = new System.Drawing.Point(0, 0);
            this._configureDirectoryServerUserControl.Name = "_configureDirectoryServerUserControl";
            this._configureDirectoryServerUserControl.Size = new System.Drawing.Size(686, 497);
            this._configureDirectoryServerUserControl.TabIndex = 2;
            // 
            // EditDirectoryServerDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(686, 533);
            this.Controls.Add(this._configureDirectoryServerUserControl);
            this.Controls.Add(this.oneGuiPanel1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "EditDirectoryServerDialog";
            this.Text = "HP Database Manager - Edit Directory Server";
            this.oneGuiPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theApplyButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theCancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theReloadButton;
        private ConfigureDirectoryServerUserControl _configureDirectoryServerUserControl;
    }
}