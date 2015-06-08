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
    partial class DisplayPublicationsUserControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DisplayPublicationsUserControl));
            this._theOverallPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theLowerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._thePictureBox = new System.Windows.Forms.PictureBox();
            this._theUpperPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theSubscribeButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theOverallPanel.SuspendLayout();
            this._theLowerPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._thePictureBox)).BeginInit();
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
            this._theOverallPanel.Size = new System.Drawing.Size(540, 329);
            this._theOverallPanel.TabIndex = 0;
            // 
            // _theLowerPanel
            // 
            this._theLowerPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theLowerPanel.Controls.Add(this._thePictureBox);
            this._theLowerPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theLowerPanel.Location = new System.Drawing.Point(0, 35);
            this._theLowerPanel.Name = "_theLowerPanel";
            this._theLowerPanel.Size = new System.Drawing.Size(540, 294);
            this._theLowerPanel.TabIndex = 1;
            // 
            // _thePictureBox
            // 
            this._thePictureBox.Enabled = false;
            this._thePictureBox.Image = ((System.Drawing.Image)(resources.GetObject("_thePictureBox.Image")));
            this._thePictureBox.Location = new System.Drawing.Point(329, 241);
            this._thePictureBox.Name = "_thePictureBox";
            this._thePictureBox.Size = new System.Drawing.Size(21, 21);
            this._thePictureBox.TabIndex = 0;
            this._thePictureBox.TabStop = false;
            this._thePictureBox.Visible = false;
            // 
            // _theUpperPanel
            // 
            this._theUpperPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theUpperPanel.Controls.Add(this._theSubscribeButton);
            this._theUpperPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theUpperPanel.Location = new System.Drawing.Point(0, 0);
            this._theUpperPanel.Name = "_theUpperPanel";
            this._theUpperPanel.Size = new System.Drawing.Size(540, 35);
            this._theUpperPanel.TabIndex = 0;
            // 
            // _theSubscribeButton
            // 
            this._theSubscribeButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._theSubscribeButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theSubscribeButton.Location = new System.Drawing.Point(413, 6);
            this._theSubscribeButton.Name = "_theSubscribeButton";
            this._theSubscribeButton.Size = new System.Drawing.Size(121, 23);
            this._theSubscribeButton.TabIndex = 0;
            this._theSubscribeButton.Text = "Subscribe";
            this._theSubscribeButton.UseVisualStyleBackColor = true;
            this._theSubscribeButton.Click += new System.EventHandler(this._theSubscribeButton_Click);
            // 
            // DisplayPublicationsUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theOverallPanel);
            this.Name = "DisplayPublicationsUserControl";
            this.Size = new System.Drawing.Size(540, 329);
            this._theOverallPanel.ResumeLayout(false);
            this._theLowerPanel.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this._thePictureBox)).EndInit();
            this._theUpperPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionPanel _theOverallPanel;
        private Framework.Controls.TrafodionPanel _theLowerPanel;
        private Framework.Controls.TrafodionPanel _theUpperPanel;
        private Framework.Controls.TrafodionButton _theSubscribeButton;
        private System.Windows.Forms.PictureBox _thePictureBox;
    }
}
