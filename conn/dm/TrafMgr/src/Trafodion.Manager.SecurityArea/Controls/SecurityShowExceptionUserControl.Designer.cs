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
namespace Trafodion.Manager.SecurityArea.Controls
{
    partial class SecurityShowExceptionUserControl
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
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._exceptionTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.oneGuiPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this._exceptionTextBox);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Padding = new System.Windows.Forms.Padding(30);
            this.oneGuiPanel1.Size = new System.Drawing.Size(586, 402);
            this.oneGuiPanel1.TabIndex = 0;
            // 
            // _exceptionTextBox
            // 
            this._exceptionTextBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._exceptionTextBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._exceptionTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._exceptionTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._exceptionTextBox.ForeColor = System.Drawing.SystemColors.ControlText;
            this._exceptionTextBox.Location = new System.Drawing.Point(30, 30);
            this._exceptionTextBox.Margin = new System.Windows.Forms.Padding(30);
            this._exceptionTextBox.Multiline = true;
            this._exceptionTextBox.Name = "_exceptionTextBox";
            this._exceptionTextBox.ReadOnly = true;
            this._exceptionTextBox.Size = new System.Drawing.Size(526, 342);
            this._exceptionTextBox.TabIndex = 0;
            // 
            // SecurityShowExceptionUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.oneGuiPanel1);
            this.Name = "SecurityShowExceptionUserControl";
            this.Size = new System.Drawing.Size(586, 402);
            this.oneGuiPanel1.ResumeLayout(false);
            this.oneGuiPanel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _exceptionTextBox;
    }
}
