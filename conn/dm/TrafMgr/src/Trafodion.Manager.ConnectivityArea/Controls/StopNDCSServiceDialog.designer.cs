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

using System.Windows.Forms;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// 
    /// </summary>
    partial class StopNDCSServiceDialog
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
            this._theStopNDCSServicePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._buttonStopImmediately = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._buttonOnClientDisconnect = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theCancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._textBoxStopReason = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.ReasonLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.MessageLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.MessageLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.AffectedConnections = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.ConnectionsAffectedLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theStopNDCSServicePanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theStopNDCSServicePanel
            // 
            this._theStopNDCSServicePanel.AutoSize = true;
            this._theStopNDCSServicePanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this._theStopNDCSServicePanel.Controls.Add(this._buttonStopImmediately);
            this._theStopNDCSServicePanel.Controls.Add(this._buttonOnClientDisconnect);
            this._theStopNDCSServicePanel.Controls.Add(this._theCancelButton);
            this._theStopNDCSServicePanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theStopNDCSServicePanel.Location = new System.Drawing.Point(0, 220);
            this._theStopNDCSServicePanel.Name = "_theStopHPDCSServicePanel";
            this._theStopNDCSServicePanel.Size = new System.Drawing.Size(497, 34);
            this._theStopNDCSServicePanel.TabIndex = 2;
            // 
            // _buttonStopImmediately
            // 
            this._buttonStopImmediately.AutoSize = true;
            this._buttonStopImmediately.Enabled = false;
            this._buttonStopImmediately.Location = new System.Drawing.Point(316, 5);
            this._buttonStopImmediately.Name = "_buttonStopImmediately";
            this._buttonStopImmediately.Size = new System.Drawing.Size(97, 23);
            this._buttonStopImmediately.TabIndex = 4;
            this._buttonStopImmediately.Text = "Stop &Immediately";
            this._buttonStopImmediately.UseVisualStyleBackColor = true;
            this._buttonStopImmediately.Click += new System.EventHandler(this._buttonStopImmediately_Click);
            // 
            // _buttonOnClientDisconnect
            // 
            this._buttonOnClientDisconnect.AutoSize = true;
            this._buttonOnClientDisconnect.Cursor = System.Windows.Forms.Cursors.Default;
            this._buttonOnClientDisconnect.Enabled = false;
            this._buttonOnClientDisconnect.Location = new System.Drawing.Point(164, 4);
            this._buttonOnClientDisconnect.Name = "_buttonOnClientDisconnect";
            this._buttonOnClientDisconnect.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
            this._buttonOnClientDisconnect.Size = new System.Drawing.Size(147, 24);
            this._buttonOnClientDisconnect.TabIndex = 3;
            this._buttonOnClientDisconnect.Text = "Stop On Client &Disconnect";
            this._buttonOnClientDisconnect.UseCompatibleTextRendering = true;
            this._buttonOnClientDisconnect.UseVisualStyleBackColor = true;
            this._buttonOnClientDisconnect.Click += new System.EventHandler(this._buttonOnClientDisconnect_Click);
            // 
            // _theCancelButton
            // 
            this._theCancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theCancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._theCancelButton.Location = new System.Drawing.Point(418, 5);
            this._theCancelButton.Name = "_theCancelButton";
            this._theCancelButton.Size = new System.Drawing.Size(75, 23);
            this._theCancelButton.TabIndex = 2;
            this._theCancelButton.Text = "&Cancel";
            this._theCancelButton.UseVisualStyleBackColor = true;
            this._theCancelButton.Click += new System.EventHandler(this._theCancelButton_Click);
            // 
            // _textBoxStopReason
            // 
            this._textBoxStopReason.Location = new System.Drawing.Point(12, 81);
            this._textBoxStopReason.Multiline = true;
            this._textBoxStopReason.Name = "_textBoxStopReason";
            this._textBoxStopReason.Size = new System.Drawing.Size(473, 99);
            this._textBoxStopReason.TabIndex = 3;
            this._textBoxStopReason.TextChanged += new System.EventHandler(this._textBoxStopReason_TextChanged);
            // 
            // ReasonLabel
            // 
            this.ReasonLabel.AutoSize = true;
            this.ReasonLabel.Location = new System.Drawing.Point(12, 63);
            this.ReasonLabel.Name = "ReasonLabel";
            this.ReasonLabel.Size = new System.Drawing.Size(47, 13);
            this.ReasonLabel.TabIndex = 5;
            this.ReasonLabel.Text = "Reason:";
            // 
            // MessageLabel1
            // 
            this.MessageLabel1.AutoSize = true;
            this.MessageLabel1.Location = new System.Drawing.Point(13, 13);
            this.MessageLabel1.Name = "MessageLabel1";
            this.MessageLabel1.Size = new System.Drawing.Size(299, 13);
            this.MessageLabel1.TabIndex = 6;
            this.MessageLabel1.Text = "A reason is needed before a stop operation can be performed.";
            // 
            // MessageLabel2
            // 
            this.MessageLabel2.AutoSize = true;
            this.MessageLabel2.Location = new System.Drawing.Point(13, 31);
            this.MessageLabel2.Name = "MessageLabel2";
            this.MessageLabel2.Size = new System.Drawing.Size(355, 13);
            this.MessageLabel2.TabIndex = 7;
            this.MessageLabel2.Text = "Please provide one in the box below then click the desired stop operation.";
            // 
            // AffectedConnections
            // 
            this.AffectedConnections.AutoSize = true;
            this.AffectedConnections.Location = new System.Drawing.Point(266, 190);
            this.AffectedConnections.Name = "AffectedConnections";
            this.AffectedConnections.Size = new System.Drawing.Size(13, 13);
            this.AffectedConnections.TabIndex = 0;
            this.AffectedConnections.Text = "0";
            this.AffectedConnections.Visible = false;
            // 
            // ConnectionsAffectedLabel
            // 
            this.ConnectionsAffectedLabel.AutoSize = true;
            this.ConnectionsAffectedLabel.Location = new System.Drawing.Point(144, 190);
            this.ConnectionsAffectedLabel.Name = "ConnectionsAffectedLabel";
            this.ConnectionsAffectedLabel.Size = new System.Drawing.Size(115, 13);
            this.ConnectionsAffectedLabel.TabIndex = 4;
            this.ConnectionsAffectedLabel.Text = "Connections Affected: ";
            this.ConnectionsAffectedLabel.Visible = false;
            // 
            // StopNDCSServiceDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(497, 254);
            this.Controls.Add(this._textBoxStopReason);
            this.Controls.Add(this.MessageLabel2);
            this.Controls.Add(this.MessageLabel1);
            this.Controls.Add(this.ReasonLabel);
            this.Controls.Add(this.AffectedConnections);
            this.Controls.Add(this.ConnectionsAffectedLabel);
            this.Controls.Add(this._theStopNDCSServicePanel);
            this.Name = "StopHPDCSServiceDialog";
            this.Text = "Trafodion Database Manager - Stopping Service - ";
            this._theStopNDCSServicePanel.ResumeLayout(false);
            this._theStopNDCSServicePanel.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theStopNDCSServicePanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theCancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _buttonStopImmediately;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _buttonOnClientDisconnect;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _textBoxStopReason;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel ReasonLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel MessageLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel MessageLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel AffectedConnections;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel ConnectionsAffectedLabel;
    }
}