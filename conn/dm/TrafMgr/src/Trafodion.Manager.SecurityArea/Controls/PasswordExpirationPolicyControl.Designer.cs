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
﻿namespace Trafodion.Manager.SecurityArea.Controls
{
    partial class PasswordExpirationPolicyControl
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
            this.components = new System.ComponentModel.Container();
            this._theMainPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._thePasswordExpirationGroupbox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theExpiryDays = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._theExpiryDate = new System.Windows.Forms.DateTimePicker();
            this._theExpiryDateLabel = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this.oneGuiLabel5 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theExpiresEveryLabel = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theUseExpirationRadio = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._theNoExpirationRadio = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._toolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._theMainPanel.SuspendLayout();
            this._thePasswordExpirationGroupbox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theExpiryDays)).BeginInit();
            this.SuspendLayout();
            // 
            // _theMainPanel
            // 
            this._theMainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theMainPanel.Controls.Add(this._thePasswordExpirationGroupbox);
            this._theMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMainPanel.Location = new System.Drawing.Point(0, 0);
            this._theMainPanel.Name = "_theMainPanel";
            this._theMainPanel.Size = new System.Drawing.Size(483, 135);
            this._theMainPanel.TabIndex = 0;
            // 
            // _thePasswordExpirationGroupbox
            // 
            this._thePasswordExpirationGroupbox.Controls.Add(this._theExpiryDays);
            this._thePasswordExpirationGroupbox.Controls.Add(this._theExpiryDate);
            this._thePasswordExpirationGroupbox.Controls.Add(this._theExpiryDateLabel);
            this._thePasswordExpirationGroupbox.Controls.Add(this.oneGuiLabel5);
            this._thePasswordExpirationGroupbox.Controls.Add(this._theExpiresEveryLabel);
            this._thePasswordExpirationGroupbox.Controls.Add(this._theUseExpirationRadio);
            this._thePasswordExpirationGroupbox.Controls.Add(this._theNoExpirationRadio);
            this._thePasswordExpirationGroupbox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._thePasswordExpirationGroupbox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._thePasswordExpirationGroupbox.Location = new System.Drawing.Point(0, 0);
            this._thePasswordExpirationGroupbox.Name = "_thePasswordExpirationGroupbox";
            this._thePasswordExpirationGroupbox.Size = new System.Drawing.Size(483, 135);
            this._thePasswordExpirationGroupbox.TabIndex = 1;
            this._thePasswordExpirationGroupbox.TabStop = false;
            this._thePasswordExpirationGroupbox.Text = "Password Expiration Policy";
            // 
            // _theExpiryDays
            // 
            this._theExpiryDays.Location = new System.Drawing.Point(115, 67);
            this._theExpiryDays.Maximum = new decimal(new int[] {
            365,
            0,
            0,
            0});
            this._theExpiryDays.Name = "_theExpiryDays";
            this._theExpiryDays.Size = new System.Drawing.Size(60, 21);
            this._theExpiryDays.TabIndex = 6;
            // 
            // _theExpiryDate
            // 
            this._theExpiryDate.CustomFormat = "MM-dd-yyyy";
            this._theExpiryDate.Format = System.Windows.Forms.DateTimePickerFormat.Custom;
            this._theExpiryDate.Location = new System.Drawing.Point(115, 94);
            this._theExpiryDate.Name = "_theExpiryDate";
            this._theExpiryDate.ShowCheckBox = true;
            this._theExpiryDate.Size = new System.Drawing.Size(97, 21);
            this._theExpiryDate.TabIndex = 5;
            this._theExpiryDate.Value = System.DateTime.Now;// new System.DateTime(2010, 8, 12, 0, 0, 0, 0);
            // 
            // _theExpiryDateLabel
            // 
            this._theExpiryDateLabel.AutoSize = true;
            this._theExpiryDateLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theExpiryDateLabel.Location = new System.Drawing.Point(27, 98);
            this._theExpiryDateLabel.Name = "_theExpiryDateLabel";
            this._theExpiryDateLabel.ShowRequired = true;
            this._theExpiryDateLabel.Size = new System.Drawing.Size(88, 17);
            this._theExpiryDateLabel.TabIndex = 5;
            this._theExpiryDateLabel.TabStop = false;
            // 
            // oneGuiLabel5
            // 
            this.oneGuiLabel5.AutoSize = true;
            this.oneGuiLabel5.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel5.Location = new System.Drawing.Point(181, 71);
            this.oneGuiLabel5.Name = "oneGuiLabel5";
            this.oneGuiLabel5.Size = new System.Drawing.Size(31, 13);
            this.oneGuiLabel5.TabIndex = 4;
            this.oneGuiLabel5.Text = "Days";
            // 
            // _theExpiresEveryLabel
            // 
            this._theExpiresEveryLabel.AutoSize = true;
            this._theExpiresEveryLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theExpiresEveryLabel.Location = new System.Drawing.Point(27, 68);
            this._theExpiresEveryLabel.Name = "_theExpiresEveryLabel";
            this._theExpiresEveryLabel.ShowRequired = true;
            this._theExpiresEveryLabel.Size = new System.Drawing.Size(88, 18);
            this._theExpiresEveryLabel.TabIndex = 2;
            this._theExpiresEveryLabel.TabStop = false;
            // 
            // _theUseExpirationRadio
            // 
            this._theUseExpirationRadio.AutoSize = true;
            this._theUseExpirationRadio.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theUseExpirationRadio.Location = new System.Drawing.Point(27, 44);
            this._theUseExpirationRadio.Name = "_theUseExpirationRadio";
            this._theUseExpirationRadio.Size = new System.Drawing.Size(100, 18);
            this._theUseExpirationRadio.TabIndex = 3;
            this._theUseExpirationRadio.Text = "Use Expiration";
            this._theUseExpirationRadio.UseVisualStyleBackColor = true;
            this._theUseExpirationRadio.CheckedChanged += new System.EventHandler(this._theUseExpirationRadio_CheckedChanged);
            // 
            // _theNoExpirationRadio
            // 
            this._theNoExpirationRadio.AutoSize = true;
            this._theNoExpirationRadio.Checked = true;
            this._theNoExpirationRadio.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theNoExpirationRadio.Location = new System.Drawing.Point(27, 20);
            this._theNoExpirationRadio.Name = "_theNoExpirationRadio";
            this._theNoExpirationRadio.Size = new System.Drawing.Size(95, 18);
            this._theNoExpirationRadio.TabIndex = 2;
            this._theNoExpirationRadio.TabStop = true;
            this._theNoExpirationRadio.Text = "No Expiration";
            this._theNoExpirationRadio.UseVisualStyleBackColor = true;
            this._theNoExpirationRadio.CheckedChanged += new System.EventHandler(this._theNoExpirationRadio_CheckedChanged);
            // 
            // _toolTip
            // 
            this._toolTip.IsBalloon = true;
            // 
            // PasswordExpirationPolicyControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theMainPanel);
            this.Name = "PasswordExpirationPolicyControl";
            this.Size = new System.Drawing.Size(483, 135);
            this._theMainPanel.ResumeLayout(false);
            this._thePasswordExpirationGroupbox.ResumeLayout(false);
            this._thePasswordExpirationGroupbox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theExpiryDays)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theMainPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _thePasswordExpirationGroupbox;
        private System.Windows.Forms.DateTimePicker _theExpiryDate;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _theExpiryDateLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel5;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _theExpiresEveryLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton _theUseExpirationRadio;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton _theNoExpirationRadio;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip _toolTip;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _theExpiryDays;
    }
}
