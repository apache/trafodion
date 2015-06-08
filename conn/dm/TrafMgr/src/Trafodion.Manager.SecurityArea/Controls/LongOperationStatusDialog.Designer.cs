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
    partial class LongOperationStatusDialog
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
            this.components = new System.ComponentModel.Container();
            this._theProgressBar = new Trafodion.Manager.Framework.Controls.TrafodionProgressBar();
            this._theStatusBox = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this._theFillerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.SuspendLayout();
            // 
            // _theProgressBar
            // 
            this._theProgressBar.Location = new System.Drawing.Point(32, 114);
            this._theProgressBar.Name = "_theProgressBar";
            this._theProgressBar.Size = new System.Drawing.Size(388, 23);
            this._theProgressBar.TabIndex = 0;
            // 
            // _theStatusBox
            // 
            this._theStatusBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theStatusBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._theStatusBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theStatusBox.Location = new System.Drawing.Point(35, 21);
            this._theStatusBox.Name = "_theStatusBox";
            this._theStatusBox.ReadOnly = true;
            this._theStatusBox.Size = new System.Drawing.Size(384, 69);
            this._theStatusBox.TabIndex = 1;
            this._theStatusBox.Text = global::Trafodion.Manager.SecurityArea.Properties.Resources.DeployCACertSuccess;
            // 
            // _theFillerPanel
            // 
            this._theFillerPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theFillerPanel.Location = new System.Drawing.Point(12, 143);
            this._theFillerPanel.Name = "_theFillerPanel";
            this._theFillerPanel.Size = new System.Drawing.Size(429, 27);
            this._theFillerPanel.TabIndex = 2;
            // 
            // LongOperationStatusDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.ClientSize = new System.Drawing.Size(453, 182);
            this.Controls.Add(this._theFillerPanel);
            this.Controls.Add(this._theStatusBox);
            this.Controls.Add(this._theProgressBar);
            this.Name = "LongOperationStatusDialog";
            this.Text = "HP Database Manager - LongOperationStatusDialog";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.LongOperationStatusDialog_FormClosing);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionProgressBar _theProgressBar;
        private Trafodion.Manager.Framework.Controls.TrafodionRichTextBox _theStatusBox;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theFillerPanel;
    }
}