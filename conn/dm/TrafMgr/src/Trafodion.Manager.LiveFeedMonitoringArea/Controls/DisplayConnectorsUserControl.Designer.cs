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
﻿namespace Trafodion.Manager.LiveFeedMonitoringArea.Controls
{
    partial class DisplayConnectorsUserControl
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
            this._theOverallPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theLowerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theUpperPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theConfigButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theOverallPanel.SuspendLayout();
            this._theUpperPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theOverallPanel
            // 
            this._theOverallPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theOverallPanel.Controls.Add(this._theLowerPanel);
            this._theOverallPanel.Controls.Add(this._theUpperPanel);
            this._theOverallPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theOverallPanel.Location = new System.Drawing.Point(0, 0);
            this._theOverallPanel.Name = "_theOverallPanel";
            this._theOverallPanel.Size = new System.Drawing.Size(585, 348);
            this._theOverallPanel.TabIndex = 0;
            // 
            // _theLowerPanel
            // 
            this._theLowerPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theLowerPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theLowerPanel.Location = new System.Drawing.Point(0, 34);
            this._theLowerPanel.Name = "_theLowerPanel";
            this._theLowerPanel.Size = new System.Drawing.Size(585, 314);
            this._theLowerPanel.TabIndex = 1;
            // 
            // _theUpperPanel
            // 
            this._theUpperPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theUpperPanel.Controls.Add(this._theConfigButton);
            this._theUpperPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theUpperPanel.Enabled = false;
            this._theUpperPanel.Location = new System.Drawing.Point(0, 0);
            this._theUpperPanel.Name = "_theUpperPanel";
            this._theUpperPanel.Size = new System.Drawing.Size(585, 34);
            this._theUpperPanel.TabIndex = 0;
            this._theUpperPanel.Visible = false;
            // 
            // _theConfigButton
            // 
            this._theConfigButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._theConfigButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theConfigButton.Location = new System.Drawing.Point(452, 6);
            this._theConfigButton.Name = "_theConfigButton";
            this._theConfigButton.Size = new System.Drawing.Size(124, 23);
            this._theConfigButton.TabIndex = 0;
            this._theConfigButton.Text = "Configure";
            this._theConfigButton.UseVisualStyleBackColor = true;
            this._theConfigButton.Click += new System.EventHandler(this._theConfigButton_Click);
            // 
            // DisplayConnectorsUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theOverallPanel);
            this.Name = "DisplayConnectorsUserControl";
            this.Size = new System.Drawing.Size(585, 348);
            this._theOverallPanel.ResumeLayout(false);
            this._theUpperPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionPanel _theOverallPanel;
        private Framework.Controls.TrafodionPanel _theLowerPanel;
        private Framework.Controls.TrafodionPanel _theUpperPanel;
        private Framework.Controls.TrafodionButton _theConfigButton;
    }
}
