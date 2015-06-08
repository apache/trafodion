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
﻿using Trafodion.Manager.Framework.Controls;
namespace Trafodion.Manager.Framework.Connections.Controls
{
    partial class EditSystemUserControl
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
            TrafodionLabel label1;
            this._theEditSystemButton = new TrafodionButton();
            this.panel1 = new TrafodionPanel();
            label1 = new TrafodionLabel();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            label1.Dock = System.Windows.Forms.DockStyle.Fill;
            label1.Location = new System.Drawing.Point(0, 0);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(724, 371);
            label1.TabIndex = 8;
            label1.Text = "This system\'s settings are incomplete and/or incorrect.\r\n\r\nPlease edit it if you " +
                "wish to use it.";
            label1.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // _theEditSystemButton
            // 
            this._theEditSystemButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theEditSystemButton.AutoSize = true;
            //this._theEditSystemButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theEditSystemButton.Location = new System.Drawing.Point(634, 2);
            this._theEditSystemButton.Name = "_theEditSystemButton";
            this._theEditSystemButton.Size = new System.Drawing.Size(87, 25);
            this._theEditSystemButton.TabIndex = 6;
            this._theEditSystemButton.Text = "&Edit System ...";
            this._theEditSystemButton.Click += new System.EventHandler(this.TheEditSystemButtonClick);
            // 
            // panel1
            // 
            this.panel1.AutoSize = true;
            this.panel1.Controls.Add(this._theEditSystemButton);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel1.Location = new System.Drawing.Point(0, 371);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(724, 30);
            this.panel1.TabIndex = 9;
            // 
            // EditSystemUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(label1);
            this.Controls.Add(this.panel1);
            this.Name = "EditSystemUserControl";
            this.Size = new System.Drawing.Size(724, 401);
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private TrafodionButton _theEditSystemButton;
        private TrafodionPanel panel1;
    }
}
