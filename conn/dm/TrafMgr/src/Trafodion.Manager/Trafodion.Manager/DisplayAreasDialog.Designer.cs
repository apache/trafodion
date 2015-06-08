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
﻿namespace Trafodion.Manager
{
    partial class DisplayAreasDialog
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
            this.areasCheckedListBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckedListBox();
            this.panel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.OkButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.displayAreasStatusBar = new Trafodion.Manager.Framework.Controls.TrafodionStatusBar();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.panel1.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // areasCheckedListBox
            // 
            this.areasCheckedListBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.areasCheckedListBox.CheckOnClick = true;
            this.areasCheckedListBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.areasCheckedListBox.FormattingEnabled = true;
            this.areasCheckedListBox.Location = new System.Drawing.Point(0, 0);
            this.areasCheckedListBox.Name = "areasCheckedListBox";
            this.areasCheckedListBox.Size = new System.Drawing.Size(321, 225);
            this.areasCheckedListBox.TabIndex = 0;
            this.areasCheckedListBox.ThreeDCheckBoxes = true;
            this.areasCheckedListBox.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.areasCheckedListBox_ItemCheck);
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.OkButton);
            this.panel1.Controls.Add(this.cancelButton);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel1.Location = new System.Drawing.Point(0, 231);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(321, 35);
            this.panel1.TabIndex = 5;
            // 
            // OkButton
            // 
            this.OkButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.OkButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.OkButton.Location = new System.Drawing.Point(163, 2);
            this.OkButton.Name = "OkButton";
            this.OkButton.Size = new System.Drawing.Size(75, 31);
            this.OkButton.TabIndex = 4;
            this.OkButton.Text = "OK";
            this.OkButton.UseVisualStyleBackColor = true;
            this.OkButton.Click += new System.EventHandler(this.okButton_Click);
            // 
            // cancelButton
            // 
            this.cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelButton.Location = new System.Drawing.Point(242, 2);
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = new System.Drawing.Size(75, 31);
            this.cancelButton.TabIndex = 5;
            this.cancelButton.Text = "Cancel";
            this.cancelButton.UseVisualStyleBackColor = true;
            // 
            // displayAreasStatusBar
            // 
            this.displayAreasStatusBar.Enabled = false;
            this.displayAreasStatusBar.Font = new System.Drawing.Font("Tahoma", 8F);
            this.displayAreasStatusBar.Location = new System.Drawing.Point(0, 209);
            this.displayAreasStatusBar.Name = "displayAreasStatusBar";
            this.displayAreasStatusBar.Size = new System.Drawing.Size(321, 22);
            this.displayAreasStatusBar.SizingGrip = false;
            this.displayAreasStatusBar.TabIndex = 6;
            this.displayAreasStatusBar.Text = global::Trafodion.Manager.Properties.Resources.Please_select_at_least_one_area_to_display;
            this.displayAreasStatusBar.Visible = false;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(255)))), ((int)(((byte)(255)))));
            this.TrafodionPanel1.Controls.Add(this.displayAreasStatusBar);
            this.TrafodionPanel1.Controls.Add(this.areasCheckedListBox);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(321, 231);
            this.TrafodionPanel1.TabIndex = 7;
            // 
            // DisplayAreasDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(321, 266);
            this.Controls.Add(this.TrafodionPanel1);
            this.Controls.Add(this.panel1);
            this.Name = "DisplayAreasDialog";
            this.Text = "Trafodion Database Manager - Show Areas";
            this.panel1.ResumeLayout(false);
            this.TrafodionPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel1;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckedListBox areasCheckedListBox;
        private Trafodion.Manager.Framework.Controls.TrafodionButton OkButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionStatusBar displayAreasStatusBar;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
    }
}