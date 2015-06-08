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
    partial class TimestampDataTypePanel
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
            this._timeStampPrecisionLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._timeStampPrecisionUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            ((System.ComponentModel.ISupportInitialize)(this._timeStampPrecisionUpDown)).BeginInit();
            this.SuspendLayout();
            // 
            // _timeStampPrecisionLabel
            // 
            this._timeStampPrecisionLabel.AutoSize = true;
            this._timeStampPrecisionLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._timeStampPrecisionLabel.Location = new System.Drawing.Point(39, 12);
            this._timeStampPrecisionLabel.Name = "_timeStampPrecisionLabel";
            this._timeStampPrecisionLabel.Size = new System.Drawing.Size(54, 14);
            this._timeStampPrecisionLabel.TabIndex = 0;
            this._timeStampPrecisionLabel.Text = " Precision";
            // 
            // _timeStampPrecisionUpDown
            // 
            this._timeStampPrecisionUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._timeStampPrecisionUpDown.Location = new System.Drawing.Point(99, 9);
            this._timeStampPrecisionUpDown.Maximum = new decimal(new int[] {
            32708,
            0,
            0,
            0});
            this._timeStampPrecisionUpDown.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this._timeStampPrecisionUpDown.Name = "_timeStampPrecisionUpDown";
            this._timeStampPrecisionUpDown.Size = new System.Drawing.Size(60, 20);
            this._timeStampPrecisionUpDown.TabIndex = 1;
            this._timeStampPrecisionUpDown.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._timeStampPrecisionUpDown.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this._timeStampPrecisionUpDown.KeyUp += new System.Windows.Forms.KeyEventHandler(this._timeStampPrecisionUpDown_KeyUp);
            // 
            // TimestampDataTypePanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._timeStampPrecisionUpDown);
            this.Controls.Add(this._timeStampPrecisionLabel);
            this.Name = "TimestampDataTypePanel";
            this.Size = new System.Drawing.Size(233, 37);
            ((System.ComponentModel.ISupportInitialize)(this._timeStampPrecisionUpDown)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionLabel _timeStampPrecisionLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _timeStampPrecisionUpDown;
    }
}
