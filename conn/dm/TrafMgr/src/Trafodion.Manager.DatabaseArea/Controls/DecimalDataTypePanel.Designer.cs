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
    partial class DecimalDataTypePanel
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
            this._precisionLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._precisionUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._scaleNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._scaleLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            ((System.ComponentModel.ISupportInitialize)(this._precisionUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._scaleNumericUpDown)).BeginInit();
            this.SuspendLayout();
            // 
            // _precisionLabel
            // 
            this._precisionLabel.AutoSize = true;
            this._precisionLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._precisionLabel.Location = new System.Drawing.Point(40, 12);
            this._precisionLabel.Name = "_precisionLabel";
            this._precisionLabel.Size = new System.Drawing.Size(51, 14);
            this._precisionLabel.TabIndex = 0;
            this._precisionLabel.Text = "Precision";
            // 
            // _precisionUpDown
            // 
            this._precisionUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._precisionUpDown.Location = new System.Drawing.Point(99, 9);
            this._precisionUpDown.Maximum = new decimal(new int[] {
            32708,
            0,
            0,
            0});
            this._precisionUpDown.Name = "_precisionUpDown";
            this._precisionUpDown.Size = new System.Drawing.Size(60, 20);
            this._precisionUpDown.TabIndex = 1;
            this._precisionUpDown.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._precisionUpDown.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this._precisionUpDown.ValueChanged += new System.EventHandler(this._precisionUpDown_ValueChanged);
            this._precisionUpDown.KeyUp += new System.Windows.Forms.KeyEventHandler(this._precisionUpDown_KeyUp);
            // 
            // _scaleNumericUpDown
            // 
            this._scaleNumericUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this._scaleNumericUpDown.Location = new System.Drawing.Point(245, 9);
            this._scaleNumericUpDown.Maximum = new decimal(new int[] {
            32708,
            0,
            0,
            0});
            this._scaleNumericUpDown.Name = "_scaleNumericUpDown";
            this._scaleNumericUpDown.Size = new System.Drawing.Size(60, 20);
            this._scaleNumericUpDown.TabIndex = 3;
            this._scaleNumericUpDown.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._scaleNumericUpDown.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this._scaleNumericUpDown.ValueChanged += new System.EventHandler(this._scaleNumericUpDown_ValueChanged);
            this._scaleNumericUpDown.KeyUp += new System.Windows.Forms.KeyEventHandler(this._scaleNumericUpDown_KeyUp);
            // 
            // _scaleLabel
            // 
            this._scaleLabel.AutoSize = true;
            this._scaleLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._scaleLabel.Location = new System.Drawing.Point(199, 12);
            this._scaleLabel.Name = "_scaleLabel";
            this._scaleLabel.Size = new System.Drawing.Size(34, 14);
            this._scaleLabel.TabIndex = 2;
            this._scaleLabel.Text = "Scale";
            // 
            // DecimalDataTypePanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._scaleNumericUpDown);
            this.Controls.Add(this._scaleLabel);
            this.Controls.Add(this._precisionUpDown);
            this.Controls.Add(this._precisionLabel);
            this.Name = "DecimalDataTypePanel";
            this.Size = new System.Drawing.Size(376, 38);
            ((System.ComponentModel.ISupportInitialize)(this._precisionUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._scaleNumericUpDown)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionLabel _precisionLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _precisionUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _scaleNumericUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _scaleLabel;
    }
}
