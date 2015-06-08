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
    partial class ManagedWindow
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
            this.theMainMenuBar = new Trafodion.Manager.Framework.Controls.TrafodionMenuStrip();
            this.TrafodionBannerControl1 = new Trafodion.Manager.Framework.Controls.TrafodionBannerControl();
            this.theManagedWindowToolButtons = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this.SuspendLayout();
            // 
            // theMainMenuBar
            // 
            this.theMainMenuBar.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.theMainMenuBar.Location = new System.Drawing.Point(0, 0);
            this.theMainMenuBar.Name = "theMainMenuBar";
            this.theMainMenuBar.Size = new System.Drawing.Size(932, 24);
            this.theMainMenuBar.TabIndex = 1;
            this.theMainMenuBar.Text = "menuStrip1";
            // 
            // TrafodionBannerControl1
            // 
            this.TrafodionBannerControl1.ConnectionDefinition = null;
            this.TrafodionBannerControl1.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionBannerControl1.Location = new System.Drawing.Point(0, 49);
            this.TrafodionBannerControl1.Name = "TrafodionBannerControl1";
            this.TrafodionBannerControl1.ShowDescription = true;
            this.TrafodionBannerControl1.Size = new System.Drawing.Size(932, 51);
            this.TrafodionBannerControl1.TabIndex = 2;
            // 
            // theManagedWindowToolButtons
            // 
            this.theManagedWindowToolButtons.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.theManagedWindowToolButtons.Location = new System.Drawing.Point(0, 24);
            this.theManagedWindowToolButtons.Name = "theManagedWindowToolButtons";
            this.theManagedWindowToolButtons.Size = new System.Drawing.Size(932, 25);
            this.theManagedWindowToolButtons.TabIndex = 3;
            this.theManagedWindowToolButtons.Text = "theMainToolBarStrip";
            this.theManagedWindowToolButtons.Visible = false;
            // 
            // ManagedWindow
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(932, 423);
            this.Controls.Add(this.TrafodionBannerControl1);
            this.Controls.Add(this.theManagedWindowToolButtons);
            this.Controls.Add(this.theMainMenuBar);
            this.Margin = new System.Windows.Forms.Padding(4);
            this.Name = "ManagedWindow";
            this.Text = "Trafodion Database Manager - ManagedWindow";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.ManagedWindow_FormClosing);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private TrafodionMenuStrip theMainMenuBar;
        private TrafodionBannerControl TrafodionBannerControl1;
        private TrafodionToolStrip theManagedWindowToolButtons;
    }
}