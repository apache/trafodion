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
﻿namespace Trafodion.Manager.Main
{
    partial class DisplayAreasPanel
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
            this.displayAreasStatusBar = new Trafodion.Manager.Framework.Controls.TrafodionStatusBar();
            this.areasCheckedListBox = new System.Windows.Forms.CheckedListBox();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // displayAreasStatusBar
            // 
            this.displayAreasStatusBar.Dock = System.Windows.Forms.DockStyle.Top;
            this.displayAreasStatusBar.Enabled = false;
            this.displayAreasStatusBar.Font = new System.Drawing.Font("Tahoma", 8F);
            this.displayAreasStatusBar.Location = new System.Drawing.Point(0, 0);
            this.displayAreasStatusBar.Name = "displayAreasStatusBar";
            this.displayAreasStatusBar.Size = new System.Drawing.Size(413, 21);
            this.displayAreasStatusBar.SizingGrip = false;
            this.displayAreasStatusBar.TabIndex = 6;
            this.displayAreasStatusBar.Text = "Please select at least one area to display.";
            this.displayAreasStatusBar.Visible = false;
            // 
            // areasCheckedListBox
            // 
            this.areasCheckedListBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.areasCheckedListBox.CheckOnClick = true;
            this.areasCheckedListBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.areasCheckedListBox.FormattingEnabled = true;
            this.areasCheckedListBox.Location = new System.Drawing.Point(0, 21);
            this.areasCheckedListBox.Margin = new System.Windows.Forms.Padding(0);
            this.areasCheckedListBox.Name = "areasCheckedListBox";
            this.areasCheckedListBox.Size = new System.Drawing.Size(413, 332);
            this.areasCheckedListBox.TabIndex = 0;
            this.areasCheckedListBox.ThreeDCheckBoxes = true;
            this.areasCheckedListBox.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.areasCheckedListBox_ItemCheck);
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(255)))), ((int)(((byte)(255)))));
            this.TrafodionPanel1.Controls.Add(this.areasCheckedListBox);
            this.TrafodionPanel1.Controls.Add(this.displayAreasStatusBar);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(10, 10);
            this.TrafodionPanel1.Margin = new System.Windows.Forms.Padding(0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(413, 353);
            this.TrafodionPanel1.TabIndex = 9;
            // 
            // DisplayAreasPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.TrafodionPanel1);
            this.Margin = new System.Windows.Forms.Padding(0);
            this.Name = "DisplayAreasPanel";
            this.Padding = new System.Windows.Forms.Padding(10);
            this.Size = new System.Drawing.Size(433, 373);
            this.TrafodionPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionStatusBar displayAreasStatusBar;
        private System.Windows.Forms.CheckedListBox areasCheckedListBox;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
    }
}
