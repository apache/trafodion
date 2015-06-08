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
﻿namespace Trafodion.Manager.LiveFeedFramework.Controls
{
    partial class LiveFeedBrokerConfigDialog
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
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._applyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.oneGuiPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._portNumberComboBox = new System.Windows.Forms.ComboBox();
            this._sessionRetryTimer = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.oneGuiLabel3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._brokerIPAddress = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.oneGuiLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theMessagePanel = new Trafodion.Manager.Framework.Controls.TrafodionMessagePanel();
            this._helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.oneGuiPanel1.SuspendLayout();
            this.oneGuiPanel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this._helpButton);
            this.oneGuiPanel1.Controls.Add(this._cancelButton);
            this.oneGuiPanel1.Controls.Add(this._applyButton);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 187);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(513, 33);
            this.oneGuiPanel1.TabIndex = 0;
            // 
            // _cancelButton
            // 
            this._cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._cancelButton.Location = new System.Drawing.Point(324, 5);
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Size = new System.Drawing.Size(89, 23);
            this._cancelButton.TabIndex = 1;
            this._cancelButton.Text = "&Cancel";
            this._cancelButton.UseVisualStyleBackColor = true;
            this._cancelButton.Click += new System.EventHandler(this._cancelButton_Click);
            // 
            // _applyButton
            // 
            this._applyButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._applyButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._applyButton.Location = new System.Drawing.Point(229, 5);
            this._applyButton.Name = "_applyButton";
            this._applyButton.Size = new System.Drawing.Size(89, 23);
            this._applyButton.TabIndex = 0;
            this._applyButton.Text = "&Apply";
            this._applyButton.UseVisualStyleBackColor = true;
            this._applyButton.Click += new System.EventHandler(this._applyButton_Click);
            // 
            // oneGuiPanel2
            // 
            this.oneGuiPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel2.Controls.Add(this._portNumberComboBox);
            this.oneGuiPanel2.Controls.Add(this._sessionRetryTimer);
            this.oneGuiPanel2.Controls.Add(this.oneGuiLabel3);
            this.oneGuiPanel2.Controls.Add(this.oneGuiLabel2);
            this.oneGuiPanel2.Controls.Add(this._brokerIPAddress);
            this.oneGuiPanel2.Controls.Add(this.oneGuiLabel1);
            this.oneGuiPanel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiPanel2.Location = new System.Drawing.Point(0, 0);
            this.oneGuiPanel2.Name = "oneGuiPanel2";
            this.oneGuiPanel2.Size = new System.Drawing.Size(513, 187);
            this.oneGuiPanel2.TabIndex = 1;
            // 
            // _portNumberComboBox
            // 
            this._portNumberComboBox.FormattingEnabled = true;
            this._portNumberComboBox.Items.AddRange(new object[] {
            "Default Port Number"});
            this._portNumberComboBox.Location = new System.Drawing.Point(201, 86);
            this._portNumberComboBox.Name = "_portNumberComboBox";
            this._portNumberComboBox.Size = new System.Drawing.Size(121, 21);
            this._portNumberComboBox.TabIndex = 6;
            // 
            // _sessionRetryTimer
            // 
            this._sessionRetryTimer.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sessionRetryTimer.Location = new System.Drawing.Point(201, 128);
            this._sessionRetryTimer.Name = "_sessionRetryTimer";
            this._sessionRetryTimer.Size = new System.Drawing.Size(121, 21);
            this._sessionRetryTimer.TabIndex = 5;
            // 
            // oneGuiLabel3
            // 
            this.oneGuiLabel3.AutoSize = true;
            this.oneGuiLabel3.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel3.Location = new System.Drawing.Point(34, 131);
            this.oneGuiLabel3.Name = "oneGuiLabel3";
            this.oneGuiLabel3.Size = new System.Drawing.Size(133, 13);
            this.oneGuiLabel3.TabIndex = 4;
            this.oneGuiLabel3.Text = "Session Retry Timer (sec):";
            // 
            // oneGuiLabel2
            // 
            this.oneGuiLabel2.AutoSize = true;
            this.oneGuiLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel2.Location = new System.Drawing.Point(34, 89);
            this.oneGuiLabel2.Name = "oneGuiLabel2";
            this.oneGuiLabel2.Size = new System.Drawing.Size(120, 13);
            this.oneGuiLabel2.TabIndex = 2;
            this.oneGuiLabel2.Text = "Live Feed Port Number:";
            // 
            // _brokerIPAddress
            // 
            this._brokerIPAddress.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._brokerIPAddress.Location = new System.Drawing.Point(201, 44);
            this._brokerIPAddress.Name = "_brokerIPAddress";
            this._brokerIPAddress.ReadOnly = true;
            this._brokerIPAddress.Size = new System.Drawing.Size(280, 21);
            this._brokerIPAddress.TabIndex = 1;
            // 
            // oneGuiLabel1
            // 
            this.oneGuiLabel1.AutoSize = true;
            this.oneGuiLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel1.Location = new System.Drawing.Point(34, 47);
            this.oneGuiLabel1.Name = "oneGuiLabel1";
            this.oneGuiLabel1.Size = new System.Drawing.Size(122, 13);
            this.oneGuiLabel1.TabIndex = 0;
            this.oneGuiLabel1.Text = "Live Feed Server Name:";
            // 
            // _theMessagePanel
            // 
            this._theMessagePanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theMessagePanel.Location = new System.Drawing.Point(0, 0);
            this._theMessagePanel.Name = "_theMessagePanel";
            this._theMessagePanel.Size = new System.Drawing.Size(513, 22);
            this._theMessagePanel.TabIndex = 2;
            // 
            // _helpButton
            // 
            this._helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._helpButton.Location = new System.Drawing.Point(419, 5);
            this._helpButton.Name = "_helpButton";
            this._helpButton.Size = new System.Drawing.Size(89, 23);
            this._helpButton.TabIndex = 2;
            this._helpButton.Text = "&Help";
            this._helpButton.UseVisualStyleBackColor = true;
            this._helpButton.Click += new System.EventHandler(this._helpButton_Click);
            // 
            // LiveFeedBrokerConfigDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.ClientSize = new System.Drawing.Size(513, 220);
            this.Controls.Add(this._theMessagePanel);
            this.Controls.Add(this.oneGuiPanel2);
            this.Controls.Add(this.oneGuiPanel1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.Name = "LiveFeedBrokerConfigDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "HP Database Manager - Live Feed Server Configuration";
            this.TopMost = true;
            this.oneGuiPanel1.ResumeLayout(false);
            this.oneGuiPanel2.ResumeLayout(false);
            this.oneGuiPanel2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _applyButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _brokerIPAddress;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _sessionRetryTimer;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel3;
        private Framework.Controls.TrafodionMessagePanel _theMessagePanel;
        private System.Windows.Forms.ComboBox _portNumberComboBox;
        private Framework.Controls.TrafodionButton _helpButton;
    }
}