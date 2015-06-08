// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
//
namespace Trafodion.Manager.Framework.Controls
{
    partial class TrafodionDisplayMessageUserControl
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
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._messageTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._messageTextBox);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Padding = new System.Windows.Forms.Padding(30);
            this.TrafodionPanel1.Size = new System.Drawing.Size(586, 402);
            this.TrafodionPanel1.TabIndex = 0;
            // 
            // _exceptionTextBox
            // 
            this._messageTextBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._messageTextBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._messageTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._messageTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._messageTextBox.ForeColor = System.Drawing.SystemColors.ControlText;
            this._messageTextBox.Location = new System.Drawing.Point(30, 30);
            this._messageTextBox.Margin = new System.Windows.Forms.Padding(30);
            this._messageTextBox.Multiline = true;
            this._messageTextBox.Name = "_messageTextBox";
            this._messageTextBox.ReadOnly = true;
            this._messageTextBox.Size = new System.Drawing.Size(526, 342);
            this._messageTextBox.TabIndex = 0;
            // 
            // SecurityShowExceptionUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "TrafodionDisplayMessageUserControl";
            this.Size = new System.Drawing.Size(586, 402);
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _messageTextBox;
    }
}
