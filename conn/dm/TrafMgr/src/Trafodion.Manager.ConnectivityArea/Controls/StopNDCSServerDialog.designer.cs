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
    partial class StopNDCSServerDialog
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
            this._stopNDCSServerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._buttonStopImmediately = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theCancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.MessageLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.MessageLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.AffectedConnections = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.ConnectionsAffectedLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._stopNDCSServerPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _stopNDCSServerPanel
            // 
            this._stopNDCSServerPanel.AutoSize = true;
            this._stopNDCSServerPanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this._stopNDCSServerPanel.Controls.Add(this._buttonStopImmediately);
            this._stopNDCSServerPanel.Controls.Add(this._theCancelButton);
            this._stopNDCSServerPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._stopNDCSServerPanel.Location = new System.Drawing.Point(0, 106);
            this._stopNDCSServerPanel.Name = "_stopHPDCSServerPanel";
            this._stopNDCSServerPanel.Size = new System.Drawing.Size(378, 34);
            this._stopNDCSServerPanel.TabIndex = 2;
            // 
            // _buttonStopImmediately
            // 
            this._buttonStopImmediately.AutoSize = true;
            this._buttonStopImmediately.Enabled = false;
            this._buttonStopImmediately.Location = new System.Drawing.Point(197, 5);
            this._buttonStopImmediately.Name = "_buttonStopImmediately";
            this._buttonStopImmediately.Size = new System.Drawing.Size(97, 23);
            this._buttonStopImmediately.TabIndex = 4;
            this._buttonStopImmediately.Text = "Stop &Immediately";
            this._buttonStopImmediately.UseVisualStyleBackColor = true;
            this._buttonStopImmediately.Click += new System.EventHandler(this._buttonStopImmediately_Click);
            // 
            // _theCancelButton
            // 
            this._theCancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theCancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._theCancelButton.Location = new System.Drawing.Point(299, 5);
            this._theCancelButton.Name = "_theCancelButton";
            this._theCancelButton.Size = new System.Drawing.Size(75, 23);
            this._theCancelButton.TabIndex = 2;
            this._theCancelButton.Text = "&Cancel";
            this._theCancelButton.UseVisualStyleBackColor = true;
            this._theCancelButton.Click += new System.EventHandler(this._theCancelButton_Click);
            // 
            // MessageLabel1
            // 
            this.MessageLabel1.AutoSize = true;
            this.MessageLabel1.Location = new System.Drawing.Point(13, 13);
            this.MessageLabel1.Name = "MessageLabel1";
            this.MessageLabel1.Size = new System.Drawing.Size(239, 13);
            this.MessageLabel1.TabIndex = 6;
            this.MessageLabel1.Text = "The selected servers will be stopped immediately.";
            // 
            // MessageLabel2
            // 
            this.MessageLabel2.AutoSize = true;
            this.MessageLabel2.Location = new System.Drawing.Point(13, 31);
            this.MessageLabel2.Name = "MessageLabel2";
            this.MessageLabel2.Size = new System.Drawing.Size(344, 13);
            this.MessageLabel2.TabIndex = 7;
            this.MessageLabel2.Text = "Please click the \"Stop Immediately\" button to confirm your stop request.";
            // 
            // AffectedConnections
            // 
            this.AffectedConnections.AutoSize = true;
            this.AffectedConnections.Location = new System.Drawing.Point(242, 70);
            this.AffectedConnections.Name = "AffectedConnections";
            this.AffectedConnections.Size = new System.Drawing.Size(13, 13);
            this.AffectedConnections.TabIndex = 8;
            this.AffectedConnections.Text = "0";
            this.AffectedConnections.Visible = false;
            // 
            // ConnectionsAffectedLabel
            // 
            this.ConnectionsAffectedLabel.AutoSize = true;
            this.ConnectionsAffectedLabel.Location = new System.Drawing.Point(121, 70);
            this.ConnectionsAffectedLabel.Name = "ConnectionsAffectedLabel";
            this.ConnectionsAffectedLabel.Size = new System.Drawing.Size(115, 13);
            this.ConnectionsAffectedLabel.TabIndex = 9;
            this.ConnectionsAffectedLabel.Text = "Connections Affected: ";
            this.ConnectionsAffectedLabel.Visible = false;
            // 
            // StopNDCSServerDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(378, 140);
            this.Controls.Add(this.ConnectionsAffectedLabel);
            this.Controls.Add(this.AffectedConnections);
            this.Controls.Add(this.MessageLabel2);
            this.Controls.Add(this.MessageLabel1);
            this.Controls.Add(this._stopNDCSServerPanel);
            this.Name = "StopNDCSServerDialog";
            this.Text = "Trafodion Database Manager - Stopping Server";
            this._stopNDCSServerPanel.ResumeLayout(false);
            this._stopNDCSServerPanel.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _stopNDCSServerPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theCancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _buttonStopImmediately;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel MessageLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel MessageLabel2;
        private Label AffectedConnections;
        private Label ConnectionsAffectedLabel;
    }
}