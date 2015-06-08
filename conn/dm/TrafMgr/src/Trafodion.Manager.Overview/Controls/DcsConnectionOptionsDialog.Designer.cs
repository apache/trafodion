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
﻿namespace Trafodion.Manager.OverviewArea.Controls
{
    partial class DcsConnectionOptionsDialog
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
            this._okButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.TrafodionLabel4 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._trafodionWebServerHost = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._trafodionWebServerPort = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionGroupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // _okButton
            // 
            this._okButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._okButton.Location = new System.Drawing.Point(209, 129);
            this._okButton.Name = "_okButton";
            this._okButton.Size = new System.Drawing.Size(75, 23);
            this._okButton.TabIndex = 1;
            this._okButton.Text = "&OK";
            this._okButton.UseVisualStyleBackColor = true;
            this._okButton.Click += new System.EventHandler(this._okButton_Click);
            // 
            // _cancelButton
            // 
            this._cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._cancelButton.Location = new System.Drawing.Point(301, 129);
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Size = new System.Drawing.Size(75, 23);
            this._cancelButton.TabIndex = 2;
            this._cancelButton.Text = "&Cancel";
            this._cancelButton.UseVisualStyleBackColor = true;
            // 
            // TrafodionLabel4
            // 
            this.TrafodionLabel4.AutoSize = true;
            this.TrafodionLabel4.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel4.Location = new System.Drawing.Point(22, 29);
            this.TrafodionLabel4.Name = "TrafodionLabel4";
            this.TrafodionLabel4.Size = new System.Drawing.Size(29, 13);
            this.TrafodionLabel4.TabIndex = 0;
            this.TrafodionLabel4.Text = "Host";
            // 
            // _trafodionWebServerHost
            // 
            this._trafodionWebServerHost.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._trafodionWebServerHost.Location = new System.Drawing.Point(57, 26);
            this._trafodionWebServerHost.Name = "_trafodionWebServerHost";
            this._trafodionWebServerHost.Size = new System.Drawing.Size(497, 21);
            this._trafodionWebServerHost.TabIndex = 1;
            // 
            // TrafodionLabel1
            // 
            this.TrafodionLabel1.AutoSize = true;
            this.TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel1.Location = new System.Drawing.Point(22, 59);
            this.TrafodionLabel1.Name = "TrafodionLabel1";
            this.TrafodionLabel1.Size = new System.Drawing.Size(27, 13);
            this.TrafodionLabel1.TabIndex = 2;
            this.TrafodionLabel1.Text = "Port";
            // 
            // _trafodionWebServerPort
            // 
            this._trafodionWebServerPort.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._trafodionWebServerPort.Location = new System.Drawing.Point(57, 56);
            this._trafodionWebServerPort.Name = "_trafodionWebServerPort";
            this._trafodionWebServerPort.Size = new System.Drawing.Size(94, 21);
            this._trafodionWebServerPort.TabIndex = 3;
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.Controls.Add(this.TrafodionLabel4);
            this.TrafodionGroupBox1.Controls.Add(this._trafodionWebServerHost);
            this.TrafodionGroupBox1.Controls.Add(this.TrafodionLabel1);
            this.TrafodionGroupBox1.Controls.Add(this._trafodionWebServerPort);
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(6, 12);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(571, 100);
            this.TrafodionGroupBox1.TabIndex = 0;
            this.TrafodionGroupBox1.TabStop = false;
            this.TrafodionGroupBox1.Text = "DCS Webserver ";
            // 
            // DcsConnectionOptionsDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(584, 159);
            this.Controls.Add(this.TrafodionGroupBox1);
            this.Controls.Add(this._cancelButton);
            this.Controls.Add(this._okButton);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Name = "DcsConnectionOptionsDialog";
            this.Text = "Trafodion Database Manager (Trafodion) - DCS Connection Options";
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.TrafodionGroupBox1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionButton _okButton;
        private Framework.Controls.TrafodionButton _cancelButton;
        private Framework.Controls.TrafodionLabel TrafodionLabel4;
        private Framework.Controls.TrafodionTextBox _trafodionWebServerHost;
        private Framework.Controls.TrafodionLabel TrafodionLabel1;
        private Framework.Controls.TrafodionTextBox _trafodionWebServerPort;
        private Framework.Controls.TrafodionGroupBox TrafodionGroupBox1;
    }
}