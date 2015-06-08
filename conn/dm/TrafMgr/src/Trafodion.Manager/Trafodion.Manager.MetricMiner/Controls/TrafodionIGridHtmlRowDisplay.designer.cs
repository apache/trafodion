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
namespace Trafodion.Manager.MetricMiner.Controls
{
    partial class TrafodionIGridHtmlRowDisplay
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TrafodionIGridHtmlRowDisplay));
            this._theToolStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._theFirstRowButton = new System.Windows.Forms.ToolStripButton();
            this._thePreviousRowButton = new System.Windows.Forms.ToolStripButton();
            this._theCounterLabel = new System.Windows.Forms.ToolStripLabel();
            this._theNextRowButton = new System.Windows.Forms.ToolStripButton();
            this._theLastRowButton = new System.Windows.Forms.ToolStripButton();
            this._theBrowser = new System.Windows.Forms.WebBrowser();
            this._theToolStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theToolStrip
            // 
            this._theToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._theFirstRowButton,
            this._thePreviousRowButton,
            this._theCounterLabel,
            this._theNextRowButton,
            this._theLastRowButton});
            this._theToolStrip.Location = new System.Drawing.Point(0, 0);
            this._theToolStrip.Name = "_theToolStrip";
            this._theToolStrip.Size = new System.Drawing.Size(474, 25);
            this._theToolStrip.TabIndex = 2;
            this._theToolStrip.Text = "TrafodionToolStrip1";
            // 
            // _theFirstRowButton
            // 
            this._theFirstRowButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theFirstRowButton.Image = ((System.Drawing.Image)(resources.GetObject("_theFirstRowButton.Image")));
            this._theFirstRowButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theFirstRowButton.Name = "_theFirstRowButton";
            this._theFirstRowButton.Size = new System.Drawing.Size(23, 22);
            this._theFirstRowButton.Text = "First Row";
            this._theFirstRowButton.Click += new System.EventHandler(this._theFirstRowButton_Click);
            // 
            // _thePreviousRowButton
            // 
            this._thePreviousRowButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._thePreviousRowButton.Image = ((System.Drawing.Image)(resources.GetObject("_thePreviousRowButton.Image")));
            this._thePreviousRowButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._thePreviousRowButton.Name = "_thePreviousRowButton";
            this._thePreviousRowButton.Size = new System.Drawing.Size(23, 22);
            this._thePreviousRowButton.Text = "Previous Row";
            this._thePreviousRowButton.Click += new System.EventHandler(this._thePreviousRowButton_Click);
            // 
            // _theCounterLabel
            // 
            this._theCounterLabel.AutoSize = false;
            this._theCounterLabel.Name = "_theCounterLabel";
            this._theCounterLabel.Size = new System.Drawing.Size(130, 22);
            this._theCounterLabel.Text = "Row 0 of 0";
            // 
            // _theNextRowButton
            // 
            this._theNextRowButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theNextRowButton.Image = ((System.Drawing.Image)(resources.GetObject("_theNextRowButton.Image")));
            this._theNextRowButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theNextRowButton.Name = "_theNextRowButton";
            this._theNextRowButton.Size = new System.Drawing.Size(23, 22);
            this._theNextRowButton.Text = "Next Row";
            this._theNextRowButton.Click += new System.EventHandler(this._theNextRowButton_Click);
            // 
            // _theLastRowButton
            // 
            this._theLastRowButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theLastRowButton.Image = ((System.Drawing.Image)(resources.GetObject("_theLastRowButton.Image")));
            this._theLastRowButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theLastRowButton.Name = "_theLastRowButton";
            this._theLastRowButton.Size = new System.Drawing.Size(23, 22);
            this._theLastRowButton.Text = "Last Row";
            this._theLastRowButton.Click += new System.EventHandler(this._theLastRowButton_Click);
            // 
            // _theBrowser
            // 
            this._theBrowser.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theBrowser.Location = new System.Drawing.Point(0, 25);
            this._theBrowser.MinimumSize = new System.Drawing.Size(20, 20);
            this._theBrowser.Name = "_theBrowser";
            this._theBrowser.Size = new System.Drawing.Size(474, 533);
            this._theBrowser.TabIndex = 3;
            // 
            // TrafodionIGridHtmlRowDisplay
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theBrowser);
            this.Controls.Add(this._theToolStrip);
            this.Name = "TrafodionIGridHtmlRowDisplay";
            this.Size = new System.Drawing.Size(474, 558);
            this._theToolStrip.ResumeLayout(false);
            this._theToolStrip.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionToolStrip _theToolStrip;
        private System.Windows.Forms.ToolStripButton _theFirstRowButton;
        private System.Windows.Forms.ToolStripButton _thePreviousRowButton;
        private System.Windows.Forms.ToolStripLabel _theCounterLabel;
        private System.Windows.Forms.ToolStripButton _theNextRowButton;
        private System.Windows.Forms.ToolStripButton _theLastRowButton;
        private System.Windows.Forms.WebBrowser _theBrowser;
    }
}
