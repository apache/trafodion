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
    partial class SaveConfigurationDialog
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
            this._theStatusTextBox = new Trafodion.Manager.Framework.Controls.TrafodionMessagePanel();
            this._theWidgetPropertyInputControl = new Trafodion.Manager.MetricMiner.Controls.WidgetPropertyInputControl();
            this.TrafodionPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._theSaveButton);
            this.TrafodionPanel1.Controls.Add(this._theCancelButton);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 377);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(573, 62);
            this.TrafodionPanel1.TabIndex = 0;
            // 
            // _theSaveButton
            // 
            this._theSaveButton.Anchor = System.Windows.Forms.AnchorStyles.Top;
            this._theSaveButton.Enabled = false;
            this._theSaveButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theSaveButton.Location = new System.Drawing.Point(219, 32);
            this._theSaveButton.Name = "_theSaveButton";
            this._theSaveButton.Size = new System.Drawing.Size(75, 23);
            this._theSaveButton.TabIndex = 1;
            this._theSaveButton.Text = "Save";
            this._theSaveButton.UseVisualStyleBackColor = true;
            this._theSaveButton.Click += new System.EventHandler(this._theSaveButton_Click);
            // 
            // _theCancelButton
            // 
            this._theCancelButton.Anchor = System.Windows.Forms.AnchorStyles.Top;
            this._theCancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCancelButton.Location = new System.Drawing.Point(300, 32);
            this._theCancelButton.Name = "_theCancelButton";
            this._theCancelButton.Size = new System.Drawing.Size(75, 23);
            this._theCancelButton.TabIndex = 0;
            this._theCancelButton.Text = "Cancel";
            this._theCancelButton.UseVisualStyleBackColor = true;
            this._theCancelButton.Click += new System.EventHandler(this._theCancelButton_Click);
            // 
            // _theStatusTextBox
            // 
            this._theStatusTextBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._theStatusTextBox.Location = new System.Drawing.Point(0, 0);
            this._theStatusTextBox.Name = "_theStatusTextBox";
            this._theStatusTextBox.Size = new System.Drawing.Size(573, 24);
            this._theStatusTextBox.TabIndex = 2;
            // 
            // _theWidgetPropertyInputControl
            // 
            this._theWidgetPropertyInputControl.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._theWidgetPropertyInputControl.Author = "";
            this._theWidgetPropertyInputControl.Description = "";
            this._theWidgetPropertyInputControl.LibraryPath = "";
            this._theWidgetPropertyInputControl.Location = new System.Drawing.Point(0, 24);
            this._theWidgetPropertyInputControl.Name = "_theWidgetPropertyInputControl";
            this._theWidgetPropertyInputControl.QueryText = "";
            this._theWidgetPropertyInputControl.ReportFileName = "";
            this._theWidgetPropertyInputControl.ServerVersion = "";
            this._theWidgetPropertyInputControl.Size = new System.Drawing.Size(573, 353);
            this._theWidgetPropertyInputControl.TabIndex = 3;
            this._theWidgetPropertyInputControl.Title = "";
            this._theWidgetPropertyInputControl.WidgetName = "";
            this._theWidgetPropertyInputControl.WidgetVersion = "";
            // 
            // SaveConfigurationDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(573, 439);
            this.Controls.Add(this._theWidgetPropertyInputControl);
            this.Controls.Add(this._theStatusTextBox);
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "SaveConfigurationDialog";
            this.Text = "Trafodion Database Manager - SaveConfigurationDialog";
            this.TrafodionPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theSaveButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theCancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionMessagePanel _theStatusTextBox;
        private WidgetPropertyInputControl _theWidgetPropertyInputControl;
    }
}