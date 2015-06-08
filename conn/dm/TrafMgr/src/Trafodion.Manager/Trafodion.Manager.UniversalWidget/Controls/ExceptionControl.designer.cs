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
    partial class ExceptionControl
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
            this._theExceptionText = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this.SuspendLayout();
            // 
            // _theExceptionText
            // 
            this._theExceptionText.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theExceptionText.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theExceptionText.Location = new System.Drawing.Point(0, 0);
            this._theExceptionText.Multiline = true;
            this._theExceptionText.Name = "_theExceptionText";
            this._theExceptionText.ReadOnly = true;
            this._theExceptionText.Size = new System.Drawing.Size(337, 217);
            this._theExceptionText.TabIndex = 0;
            // 
            // ExceptionControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theExceptionText);
            this.Name = "ExceptionControl";
            this.Size = new System.Drawing.Size(337, 217);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionRichTextBox _theExceptionText;
    }
}
