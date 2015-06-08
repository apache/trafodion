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
﻿namespace Trafodion.Manager.Connections.Controls
{
    partial class BackgroundConnectDialog
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
            this.components = new System.ComponentModel.Container();
            this._progressBar = new Trafodion.Manager.Framework.Controls.TrafodionProgressBar();
            this._statusTextBox = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this._cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._connectionTimer = new System.Windows.Forms.Timer(this.components);
            this._connectionTimerLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.SuspendLayout();
            // 
            // _progressBar
            // 
            this._progressBar.Location = new System.Drawing.Point(12, 40);
            this._progressBar.Name = "_progressBar";
            this._progressBar.Size = new System.Drawing.Size(506, 20);
            this._progressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            this._progressBar.TabIndex = 0;
            // 
            // _statusTextBox
            // 
            this._statusTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._statusTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._statusTextBox.Location = new System.Drawing.Point(12, 12);
            this._statusTextBox.Name = "_statusTextBox";
            this._statusTextBox.ReadOnly = true;
            this._statusTextBox.Size = new System.Drawing.Size(506, 23);
            this._statusTextBox.TabIndex = 1;
            this._statusTextBox.Text = "Connection in progress...";
            // 
            // _cancelButton
            // 
            this._cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._cancelButton.Location = new System.Drawing.Point(443, 63);
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Size = new System.Drawing.Size(75, 23);
            this._cancelButton.TabIndex = 2;
            this._cancelButton.Text = "&Cancel";
            this._cancelButton.UseVisualStyleBackColor = true;
            this._cancelButton.Click += new System.EventHandler(this._cancelButton_Click);
            // 
            // _connectionTimerLabel
            // 
            this._connectionTimerLabel.AutoSize = true;
            this._connectionTimerLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._connectionTimerLabel.Location = new System.Drawing.Point(12, 73);
            this._connectionTimerLabel.Name = "_connectionTimerLabel";
            this._connectionTimerLabel.Size = new System.Drawing.Size(85, 13);
            this._connectionTimerLabel.TabIndex = 3;
            this._connectionTimerLabel.Text = "Elapsed Time : 0";
            // 
            // BackgroundConnectDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(534, 91);
            this.Controls.Add(this._connectionTimerLabel);
            this.Controls.Add(this._cancelButton);
            this.Controls.Add(this._statusTextBox);
            this.Controls.Add(this._progressBar);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "BackgroundConnectDialog";
            this.Text = "Trafodion Database Manager (Trafodion) -  Connect";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Framework.Controls.TrafodionProgressBar _progressBar;
        private Framework.Controls.TrafodionRichTextBox _statusTextBox;
        private Framework.Controls.TrafodionButton _cancelButton;
        private System.Windows.Forms.Timer _connectionTimer;
        private Framework.Controls.TrafodionLabel _connectionTimerLabel;
    }
}