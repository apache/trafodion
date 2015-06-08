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
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{
    partial class TrafodionIGridCountUserControl
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
            this.theLabel = new ToolStripLabel();
            this.theCountToolStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this.SuspendLayout();
            // 
            // theLabel
            // 
            this.theLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theLabel.Name = "theLabel";
            this.theLabel.Size = new System.Drawing.Size(442, 20);
            this.theLabel.Text = "label1";
            this.theLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // theCountToolStrip
            // 
            this.theCountToolStrip.Location = new System.Drawing.Point(0, 0);
            this.theCountToolStrip.Name = "theCountToolStrip";
            this.theCountToolStrip.Size = new System.Drawing.Size(969, 25);
            this.theCountToolStrip.TabIndex = 1;
            this.theCountToolStrip.Text = "TrafodionToolStrip1";

            theCountToolStrip.Items.Add(theLabel);
            // 
            // TrafodionIGridCountUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.theCountToolStrip);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.ForeColor = System.Drawing.SystemColors.ControlText;
            this.Margin = new System.Windows.Forms.Padding(0);
            this.Name = "TrafodionIGridCountUserControl";
            this.Size = new System.Drawing.Size(969, 20);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private ToolStripLabel theLabel;
        private TrafodionToolStrip theCountToolStrip;
    }
}
