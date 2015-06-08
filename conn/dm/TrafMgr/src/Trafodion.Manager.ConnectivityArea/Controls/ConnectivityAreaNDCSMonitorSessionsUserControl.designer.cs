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
    /// <summary>
    /// 
    /// </summary>
    partial class ConnectivityAreaNDCSMonitorSessionsUserControl
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
            this.label1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._tablePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._bottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._bottomRightPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._bottomLeftPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._stopButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._refreshButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._bottomPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(3, 2);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(49, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "HPDCS Services Status Tab";
            // 
            // _tablePanel
            // 
            this._tablePanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._tablePanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this._tablePanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._tablePanel.Location = new System.Drawing.Point(0, 0);
            this._tablePanel.Name = "_tablePanel";
            this._tablePanel.Size = new System.Drawing.Size(677, 398);
            this._tablePanel.TabIndex = 1;
            // 
            // _bottomPanel
            // 
            this._bottomPanel.Controls.Add(this._bottomRightPanel);
            this._bottomPanel.Controls.Add(this._bottomLeftPanel);
            this._bottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._bottomPanel.Location = new System.Drawing.Point(0, 398);
            this._bottomPanel.Name = "_bottomPanel";
            this._bottomPanel.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
            this._bottomPanel.Size = new System.Drawing.Size(677, 31);
            this._bottomPanel.TabIndex = 6;
            // 
            // _bottomRightPanel
            // 
            this._bottomRightPanel.Dock = System.Windows.Forms.DockStyle.Right;
            this._bottomRightPanel.Location = new System.Drawing.Point(206, 0);
            this._bottomRightPanel.Name = "_bottomRightPanel";
            this._bottomRightPanel.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this._bottomRightPanel.Size = new System.Drawing.Size(471, 31);
            this._bottomRightPanel.TabIndex = 5;
            // 
            // _bottomLeftPanel
            // 
            this._bottomLeftPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._bottomLeftPanel.Dock = System.Windows.Forms.DockStyle.Left;
            this._bottomLeftPanel.Location = new System.Drawing.Point(0, 0);
            this._bottomLeftPanel.Name = "_bottomLeftPanel";
            this._bottomLeftPanel.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
            this._bottomLeftPanel.Size = new System.Drawing.Size(183, 31);
            this._bottomLeftPanel.TabIndex = 5;
            // 
            // _stopButton
            // 
            this._stopButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._stopButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._stopButton.Location = new System.Drawing.Point(3, 4);
            this._stopButton.Name = "_stopButton";
            this._stopButton.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this._stopButton.Size = new System.Drawing.Size(54, 23);
            this._stopButton.TabIndex = 2;
            this._stopButton.Text = "Sto&p...";
            this._stopButton.UseVisualStyleBackColor = true;
            this._stopButton.Click += new System.EventHandler(this._stopButton_Click);
            // 
            // _refreshButton
            // 
            this._refreshButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._refreshButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._refreshButton.Location = new System.Drawing.Point(60, 4);
            this._refreshButton.Name = "_refreshButton";
            this._refreshButton.Size = new System.Drawing.Size(60, 23);
            this._refreshButton.TabIndex = 1;
            this._refreshButton.Text = "&Refresh";
            this._refreshButton.UseVisualStyleBackColor = true;
            this._refreshButton.Click += new System.EventHandler(this._refreshButton_Click);
            // 
            // ConnectivityAreaNDCSMonitorSessionsUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._tablePanel);
            this.Controls.Add(this._bottomPanel);
            this.Name = "ConnectivityAreaNDCSMonitorSessionsUserControl";
            this.Size = new System.Drawing.Size(677, 429);
            this._bottomPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _tablePanel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _bottomPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _bottomRightPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _bottomLeftPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _stopButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _refreshButton;
    }
}
