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
﻿namespace Trafodion.Manager.UniversalWidget.Controls
{
    partial class OutputControl
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
            MyDispose(disposing);
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(OutputControl));
            this._theOutputText = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this.TrafodionToolStrip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._theClearButton = new System.Windows.Forms.ToolStripButton();
            this.TrafodionToolStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theOutputText
            // 
            this._theOutputText.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theOutputText.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theOutputText.Location = new System.Drawing.Point(0, 25);
            this._theOutputText.Multiline = true;
            this._theOutputText.Name = "_theOutputText";
            this._theOutputText.ReadOnly = true;
            this._theOutputText.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.Both;
            this._theOutputText.Size = new System.Drawing.Size(337, 192);
            this._theOutputText.TabIndex = 0;
            // 
            // TrafodionToolStrip1
            // 
            this.TrafodionToolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._theClearButton});
            this.TrafodionToolStrip1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionToolStrip1.Name = "TrafodionToolStrip1";
            this.TrafodionToolStrip1.Size = new System.Drawing.Size(337, 25);
            this.TrafodionToolStrip1.TabIndex = 1;
            this.TrafodionToolStrip1.Text = "TrafodionToolStrip1";
            // 
            // _theClearButton
            // 
            this._theClearButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theClearButton.Image = ((System.Drawing.Image)(resources.GetObject("_theClearButton.Image")));
            this._theClearButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theClearButton.Name = "_theClearButton";
            this._theClearButton.Size = new System.Drawing.Size(23, 22);
            this._theClearButton.Text = "toolStripButton1";
            this._theClearButton.ToolTipText = "Clear Console History";
            this._theClearButton.Click += new System.EventHandler(this._theClearButton_Click);
            // 
            // OutputControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theOutputText);
            this.Controls.Add(this.TrafodionToolStrip1);
            this.Name = "OutputControl";
            this.Size = new System.Drawing.Size(337, 217);
            this.TrafodionToolStrip1.ResumeLayout(false);
            this.TrafodionToolStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionRichTextBox _theOutputText;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStrip TrafodionToolStrip1;
        private System.Windows.Forms.ToolStripButton _theClearButton;
    }
}
