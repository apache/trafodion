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
    partial class CharDataTypePanel
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
            this.lengthLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.lengthNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.varyingCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.upshiftCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.charsetComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            ((System.ComponentModel.ISupportInitialize)(this.lengthNumericUpDown)).BeginInit();
            this.SuspendLayout();
            // 
            // lengthLabel
            // 
            this.lengthLabel.AutoSize = true;
            this.lengthLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lengthLabel.Location = new System.Drawing.Point(51, 9);
            this.lengthLabel.Name = "lengthLabel";
            this.lengthLabel.Size = new System.Drawing.Size(43, 14);
            this.lengthLabel.TabIndex = 0;
            this.lengthLabel.Text = "Length ";
            // 
            // lengthNumericUpDown
            // 
            this.lengthNumericUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lengthNumericUpDown.Location = new System.Drawing.Point(99, 8);
            this.lengthNumericUpDown.Maximum = new decimal(new int[] {
            32708,
            0,
            0,
            0});
            this.lengthNumericUpDown.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.lengthNumericUpDown.Name = "lengthNumericUpDown";
            this.lengthNumericUpDown.Size = new System.Drawing.Size(68, 20);
            this.lengthNumericUpDown.TabIndex = 1;
            this.lengthNumericUpDown.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.lengthNumericUpDown.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.lengthNumericUpDown.KeyUp += new System.Windows.Forms.KeyEventHandler(this.lengthNumericUpDown_KeyUp);
            // 
            // varyingCheckBox
            // 
            this.varyingCheckBox.AutoSize = true;
            this.varyingCheckBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.varyingCheckBox.Location = new System.Drawing.Point(196, 10);
            this.varyingCheckBox.Name = "varyingCheckBox";
            this.varyingCheckBox.Size = new System.Drawing.Size(64, 18);
            this.varyingCheckBox.TabIndex = 2;
            this.varyingCheckBox.Text = "Varying";
            this.varyingCheckBox.UseVisualStyleBackColor = true;
            // 
            // upshiftCheckBox
            // 
            this.upshiftCheckBox.AutoSize = true;
            this.upshiftCheckBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.upshiftCheckBox.Location = new System.Drawing.Point(276, 10);
            this.upshiftCheckBox.Name = "upshiftCheckBox";
            this.upshiftCheckBox.Size = new System.Drawing.Size(60, 18);
            this.upshiftCheckBox.TabIndex = 2;
            this.upshiftCheckBox.Text = "Upshift";
            this.upshiftCheckBox.UseVisualStyleBackColor = true;
            // 
            // charsetComboBox
            // 
            this.charsetComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.charsetComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.charsetComboBox.FormattingEnabled = true;
            this.charsetComboBox.Location = new System.Drawing.Point(353, 8);
            this.charsetComboBox.Name = "charsetComboBox";
            this.charsetComboBox.Size = new System.Drawing.Size(121, 22);
            this.charsetComboBox.TabIndex = 3;
            // 
            // CharDataTypePanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.charsetComboBox);
            this.Controls.Add(this.upshiftCheckBox);
            this.Controls.Add(this.varyingCheckBox);
            this.Controls.Add(this.lengthNumericUpDown);
            this.Controls.Add(this.lengthLabel);
            this.Name = "CharDataTypePanel";
            this.Size = new System.Drawing.Size(494, 37);
            ((System.ComponentModel.ISupportInitialize)(this.lengthNumericUpDown)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionLabel lengthLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown lengthNumericUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox varyingCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox upshiftCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox charsetComboBox;
    }
}
