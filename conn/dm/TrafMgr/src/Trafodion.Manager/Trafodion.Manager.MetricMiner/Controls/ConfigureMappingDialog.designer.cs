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
﻿namespace Trafodion.Manager.MetricMiner.Controls
{
    partial class ConfigureMappingDialog
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
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theSaveButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theCancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theMappingUserControl = new Trafodion.Manager.MetricMiner.Controls.ConfigureMappingUserControl();
            this.TrafodionPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.Controls.Add(this._theSaveButton);
            this.TrafodionPanel1.Controls.Add(this._theCancelButton);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 425);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(539, 42);
            this.TrafodionPanel1.TabIndex = 0;
            // 
            // _theSaveButton
            // 
            this._theSaveButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theSaveButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theSaveButton.Location = new System.Drawing.Point(188, 9);
            this._theSaveButton.Name = "_theSaveButton";
            this._theSaveButton.Size = new System.Drawing.Size(75, 23);
            this._theSaveButton.TabIndex = 1;
            this._theSaveButton.Text = "OK";
            this._theSaveButton.UseVisualStyleBackColor = true;
            this._theSaveButton.Click += new System.EventHandler(this._theSaveButton_Click);
            // 
            // _theCancelButton
            // 
            this._theCancelButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theCancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCancelButton.Location = new System.Drawing.Point(269, 9);
            this._theCancelButton.Name = "_theCancelButton";
            this._theCancelButton.Size = new System.Drawing.Size(75, 23);
            this._theCancelButton.TabIndex = 0;
            this._theCancelButton.Text = "Cancel";
            this._theCancelButton.UseVisualStyleBackColor = true;
            this._theCancelButton.Click += new System.EventHandler(this._theCancelButton_Click);
            // 
            // _theMappingUserControl
            // 
            this._theMappingUserControl.Config = null;
            this._theMappingUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMappingUserControl.Location = new System.Drawing.Point(0, 0);
            this._theMappingUserControl.Name = "_theMappingUserControl";
            this._theMappingUserControl.Size = new System.Drawing.Size(539, 425);
            this._theMappingUserControl.TabIndex = 1;
            // 
            // ConfigureMappingDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(539, 467);
            this.Controls.Add(this._theMappingUserControl);
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "ConfigureMappingDialog";
            this.Text = "Trafodion Database Manager - SaveConfigurationDialog";
            this.TrafodionPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theSaveButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theCancelButton;
        private ConfigureMappingUserControl _theMappingUserControl;
    }
}