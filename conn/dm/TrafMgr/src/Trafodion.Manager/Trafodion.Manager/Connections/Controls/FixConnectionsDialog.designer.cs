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
using Trafodion.Manager.Framework.Controls;
namespace Trafodion.Manager.Framework.Connections.Controls
{
    public partial class FixConnectionsDialog
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
            this.theOKButton = new TrafodionButton();
            this.panel1 = new TrafodionPanel();
            this.theLoadedSystemsGroupBox = new TrafodionGroupBox();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // theOKButton
            // 
            this.theOKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            //this.theOKButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.theOKButton.Location = new System.Drawing.Point(368, 6);
            this.theOKButton.Name = "theOKButton";
            this.theOKButton.Size = new System.Drawing.Size(83, 23);
            this.theOKButton.TabIndex = 1000;
            this.theOKButton.Text = "&OK";
            this.theOKButton.Click += new System.EventHandler(this.theOKButton_Click);
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.theOKButton);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel1.Location = new System.Drawing.Point(0, 58);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(454, 32);
            this.panel1.TabIndex = 1;
            // 
            // theLoadedSystemsGroupBox
            // 
            this.theLoadedSystemsGroupBox.AutoSize = true;
            this.theLoadedSystemsGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theLoadedSystemsGroupBox.Location = new System.Drawing.Point(0, 0);
            this.theLoadedSystemsGroupBox.Name = "theLoadedSystemsGroupBox";
            this.theLoadedSystemsGroupBox.Size = new System.Drawing.Size(454, 58);
            this.theLoadedSystemsGroupBox.TabIndex = 2;
            this.theLoadedSystemsGroupBox.TabStop = false;
            this.theLoadedSystemsGroupBox.Text = "You may enter passwords now if desired";
            // 
            // FixupConnectionsDialog
            // 
            this.AcceptButton = this.theOKButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.ClientSize = new System.Drawing.Size(454, 90);
            this.Controls.Add(this.theLoadedSystemsGroupBox);
            this.Controls.Add(this.panel1);
            this.Name = Properties.Resources.FixConnectionsDialog;
            this.Text = Properties.Resources.ProductName + " - " + Properties.Resources.EnterPassword;
            this.Activated += new System.EventHandler(this.FixupConnectionsDialog_Activated);
            this.panel1.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private TrafodionButton theOKButton;
        private TrafodionPanel panel1;
        private TrafodionGroupBox theLoadedSystemsGroupBox;
    }
}