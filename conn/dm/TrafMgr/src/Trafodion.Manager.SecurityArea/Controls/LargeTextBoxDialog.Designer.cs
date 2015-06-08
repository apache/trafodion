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
﻿namespace Trafodion.Manager.SecurityArea.Controls
{
    partial class LargeTextBoxDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(LargeTextBoxDialog));
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theBottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theApplyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theCancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.oneGuiPanel1.SuspendLayout();
            this._theBottomPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.AutoScroll = true;
            this.oneGuiPanel1.AutoSize = true;
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this._theTextBox);
            this.oneGuiPanel1.Controls.Add(this._theBottomPanel);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(594, 248);
            this.oneGuiPanel1.TabIndex = 0;
            // 
            // _theTextBox
            // 
            this._theTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theTextBox.Location = new System.Drawing.Point(0, 0);
            this._theTextBox.Multiline = true;
            this._theTextBox.Name = "_theTextBox";
            this._theTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this._theTextBox.Size = new System.Drawing.Size(594, 215);
            this._theTextBox.TabIndex = 1;
            this._theTextBox.TextChanged += new System.EventHandler(this._theTextBox_TextChanged);
            // 
            // _theBottomPanel
            // 
            this._theBottomPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theBottomPanel.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this._theBottomPanel.Controls.Add(this._theApplyButton);
            this._theBottomPanel.Controls.Add(this._theCancelButton);
            this._theBottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theBottomPanel.Location = new System.Drawing.Point(0, 215);
            this._theBottomPanel.Name = "_theBottomPanel";
            this._theBottomPanel.Size = new System.Drawing.Size(594, 33);
            this._theBottomPanel.TabIndex = 0;
            // 
            // _theApplyButton
            // 
            this._theApplyButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._theApplyButton.AutoSize = true;
            this._theApplyButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this._theApplyButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theApplyButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theApplyButton.Location = new System.Drawing.Point(396, 4);
            this._theApplyButton.Name = "_theApplyButton";
            this._theApplyButton.Size = new System.Drawing.Size(92, 23);
            this._theApplyButton.TabIndex = 1;
            this._theApplyButton.Text = "&Apply";
            this._theApplyButton.UseVisualStyleBackColor = true;
            this._theApplyButton.Click += new System.EventHandler(this._theApplyButton_Click);
            // 
            // _theCancelButton
            // 
            this._theCancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._theCancelButton.AutoSize = true;
            this._theCancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._theCancelButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theCancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCancelButton.Location = new System.Drawing.Point(494, 4);
            this._theCancelButton.Name = "_theCancelButton";
            this._theCancelButton.Size = new System.Drawing.Size(87, 23);
            this._theCancelButton.TabIndex = 0;
            this._theCancelButton.Text = "&Cancel";
            this._theCancelButton.UseVisualStyleBackColor = true;
            this._theCancelButton.Click += new System.EventHandler(this._theCancelButton_Click);
            // 
            // LargeTextBoxDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(594, 248);
            this.Controls.Add(this.oneGuiPanel1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "LargeTextBoxDialog";
            this.Text = "HP Database Manager - ";
            this.oneGuiPanel1.ResumeLayout(false);
            this.oneGuiPanel1.PerformLayout();
            this._theBottomPanel.ResumeLayout(false);
            this._theBottomPanel.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theBottomPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theApplyButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theCancelButton;
    }
}