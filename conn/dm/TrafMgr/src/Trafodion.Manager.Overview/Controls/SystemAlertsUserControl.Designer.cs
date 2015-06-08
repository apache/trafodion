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
    partial class SystemAlertsUserControl
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
            MyDispose(disposing);
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.widgetPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.bottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.exportButtonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.alarmsButtonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._applyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.bottomPanel.SuspendLayout();
            this.alarmsButtonPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // widgetPanel
            // 
            this.widgetPanel.AutoScroll = true;
            this.widgetPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.widgetPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.widgetPanel.Location = new System.Drawing.Point(0, 0);
            this.widgetPanel.Name = "widgetPanel";
            this.widgetPanel.Size = new System.Drawing.Size(912, 503);
            this.widgetPanel.TabIndex = 0;
            // 
            // bottomPanel
            // 
            this.bottomPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.bottomPanel.Controls.Add(this.exportButtonsPanel);
            this.bottomPanel.Controls.Add(this.alarmsButtonPanel);
            this.bottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.bottomPanel.Location = new System.Drawing.Point(0, 503);
            this.bottomPanel.Name = "bottomPanel";
            this.bottomPanel.Size = new System.Drawing.Size(912, 33);
            this.bottomPanel.TabIndex = 0;
            // 
            // exportButtonsPanel
            // 
            this.exportButtonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.exportButtonsPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.exportButtonsPanel.Location = new System.Drawing.Point(108, 0);
            this.exportButtonsPanel.Name = "exportButtonsPanel";
            this.exportButtonsPanel.Size = new System.Drawing.Size(804, 33);
            this.exportButtonsPanel.TabIndex = 2;
            // 
            // alarmsButtonPanel
            // 
            this.alarmsButtonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.alarmsButtonPanel.Controls.Add(this._applyButton);
            this.alarmsButtonPanel.Dock = System.Windows.Forms.DockStyle.Left;
            this.alarmsButtonPanel.Location = new System.Drawing.Point(0, 0);
            this.alarmsButtonPanel.Name = "alarmsButtonPanel";
            this.alarmsButtonPanel.Size = new System.Drawing.Size(108, 33);
            this.alarmsButtonPanel.TabIndex = 1;
            // 
            // _applyButton
            // 
            this._applyButton.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._applyButton.Enabled = false;
            this._applyButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._applyButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._applyButton.Image = global::Trafodion.Manager.OverviewArea.Properties.Resources.ApplyIcon;
            this._applyButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._applyButton.Location = new System.Drawing.Point(11, 5);
            this._applyButton.Name = "_applyButton";
            this._applyButton.Size = new System.Drawing.Size(68, 23);
            this._applyButton.TabIndex = 0;
            this._applyButton.Text = "   &Apply";
            this._applyButton.UseVisualStyleBackColor = true;
            this._applyButton.Visible = false;
            this._applyButton.Click += new System.EventHandler(this._applyButton_Click);
            // 
            // SystemAlertsUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.widgetPanel);
            this.Controls.Add(this.bottomPanel);
            this.Name = "SystemAlertsUserControl";
            this.Size = new System.Drawing.Size(912, 536);
            this.bottomPanel.ResumeLayout(false);
            this.alarmsButtonPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel widgetPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel bottomPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _applyButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel alarmsButtonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel exportButtonsPanel;


    }
}
