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
namespace Trafodion.Manager.Framework.Controls
{
    partial class WindowsManagerForm
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
            this.panel1 = new TrafodionPanel();
            this.theOKButton = new TrafodionButton();
            this.theCloseWindowsButton = new TrafodionButton();
            this.theActivateButton = new TrafodionButton();
            this.theBringMainWindowToFrontButton = new TrafodionButton();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.theBringMainWindowToFrontButton);
            this.panel1.Controls.Add(this.theOKButton);
            this.panel1.Controls.Add(this.theCloseWindowsButton);
            this.panel1.Controls.Add(this.theActivateButton);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Right;
            this.panel1.Location = new System.Drawing.Point(556, 10);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(166, 293);
            this.panel1.TabIndex = 1;
            // 
            // theOKButton
            // 
            this.theOKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            //this.theOKButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.theOKButton.Location = new System.Drawing.Point(16, 270);
            this.theOKButton.Name = "theOKButton";
            this.theOKButton.Size = new System.Drawing.Size(150, 23);
            this.theOKButton.TabIndex = 3;
            this.theOKButton.Text = "&OK";
            this.theOKButton.Click += new System.EventHandler(this.theOKButton_Click);
            // 
            // theCloseWindowsButton
            // 
            this.theCloseWindowsButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            //this.theCloseWindowsButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.theCloseWindowsButton.Location = new System.Drawing.Point(16, 32);
            this.theCloseWindowsButton.Name = "theCloseWindowsButton";
            this.theCloseWindowsButton.Size = new System.Drawing.Size(150, 23);
            this.theCloseWindowsButton.TabIndex = 1;
            this.theCloseWindowsButton.Text = "&Close Window(s)";
            this.theCloseWindowsButton.Click += new System.EventHandler(this.theCloseWindowsButton_Click);
            // 
            // theActivateButton
            // 
            this.theActivateButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            //this.theActivateButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.theActivateButton.Location = new System.Drawing.Point(16, 0);
            this.theActivateButton.Name = "theActivateButton";
            this.theActivateButton.Size = new System.Drawing.Size(150, 23);
            this.theActivateButton.TabIndex = 0;
            this.theActivateButton.Text = "Window(s) To &Front";
            this.theActivateButton.Click += new System.EventHandler(this.theActivateButton_Click);
            // 
            // theBringMainWindowToFrontButton
            // 
            this.theBringMainWindowToFrontButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            //this.theBringMainWindowToFrontButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.theBringMainWindowToFrontButton.Location = new System.Drawing.Point(16, 134);
            this.theBringMainWindowToFrontButton.Name = "theBringMainWindowToFrontButton";
            this.theBringMainWindowToFrontButton.Size = new System.Drawing.Size(150, 23);
            this.theBringMainWindowToFrontButton.TabIndex = 4;
            this.theBringMainWindowToFrontButton.Text = "&Main Window To Front";
            this.theBringMainWindowToFrontButton.Click += new System.EventHandler(this.theBringMainWindowToFrontButton_Click);
            // 
            // WindowsManagerForm
            // 
            this.AcceptButton = this.theOKButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.ClientSize = new System.Drawing.Size(732, 313);
            this.Controls.Add(this.panel1);
            this.Name = "WindowsManagerForm";
            this.Padding = new System.Windows.Forms.Padding(10);
            this.Text = "Windows Manager";
            this.panel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionPanel panel1;
        private TrafodionButton theOKButton;
        private TrafodionButton theCloseWindowsButton;
        private TrafodionButton theActivateButton;
        private TrafodionButton theBringMainWindowToFrontButton;
    }
}