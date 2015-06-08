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
﻿namespace Trafodion.Manager.DatabaseArea.Controls
{
    partial class FloatDataTypePanel
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
            this._precisionNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._precisionLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            ((System.ComponentModel.ISupportInitialize)(this._precisionNumericUpDown)).BeginInit();
            this.SuspendLayout();
            // 
            // _precisionNumericUpDown
            // 
            this._precisionNumericUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._precisionNumericUpDown.Location = new System.Drawing.Point(99, 9);
            this._precisionNumericUpDown.Maximum = new decimal(new int[] {
            32708,
            0,
            0,
            0});
            this._precisionNumericUpDown.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this._precisionNumericUpDown.Name = "_precisionNumericUpDown";
            this._precisionNumericUpDown.Size = new System.Drawing.Size(60, 20);
            this._precisionNumericUpDown.TabIndex = 3;
            this._precisionNumericUpDown.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._precisionNumericUpDown.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            // 
            // _precisionLabel
            // 
            this._precisionLabel.AutoSize = true;
            this._precisionLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._precisionLabel.Location = new System.Drawing.Point(41, 11);
            this._precisionLabel.Name = "_precisionLabel";
            this._precisionLabel.Size = new System.Drawing.Size(51, 14);
            this._precisionLabel.TabIndex = 2;
            this._precisionLabel.Text = "Precision";
            // 
            // FloatDataTypePanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._precisionNumericUpDown);
            this.Controls.Add(this._precisionLabel);
            this.Name = "FloatDataTypePanel";
            this.Size = new System.Drawing.Size(235, 47);
            ((System.ComponentModel.ISupportInitialize)(this._precisionNumericUpDown)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _precisionNumericUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _precisionLabel;

    }
}
