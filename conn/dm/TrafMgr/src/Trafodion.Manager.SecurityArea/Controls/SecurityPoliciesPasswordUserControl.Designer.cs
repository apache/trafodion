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
    partial class SecurityPoliciesPasswordUserControl
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
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._thePasswordExpirationPolicyControl = new Trafodion.Manager.SecurityArea.Controls.PasswordExpirationPolicyControl();
            this._thePwdControlGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._pwdLogonDelaySpinner = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._pwdCanChangeSpinner = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._pwdMaxLogonSpinner = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._pwdGracePeriodSpinner = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._pwdHistorySpinner = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.oneGuiLabel11 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiLabel10 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiLabel7 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiLabel6 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiLabel5 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._thePwdQualityGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.oneGuiLabel8 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._pwdUpperRequired = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._pwdLowerRequired = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._pwdQualNoRepeatCharsCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.oneGuiLabel9 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._pwdQualNoUserNameCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._pwdNumberRequired = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._pwdMinLengthSpinner = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._numberOfPwdCriteria = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.oneGuiLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._pwdSpecialRequired = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._toolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.oneGuiPanel1.SuspendLayout();
            this._thePwdControlGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._pwdLogonDelaySpinner)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._pwdCanChangeSpinner)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._pwdMaxLogonSpinner)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._pwdGracePeriodSpinner)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._pwdHistorySpinner)).BeginInit();
            this._thePwdQualityGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._pwdMinLengthSpinner)).BeginInit();
            this.SuspendLayout();
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this._thePasswordExpirationPolicyControl);
            this.oneGuiPanel1.Controls.Add(this._thePwdControlGroupBox);
            this.oneGuiPanel1.Controls.Add(this._thePwdQualityGroupBox);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(682, 402);
            this.oneGuiPanel1.TabIndex = 0;
            // 
            // _thePasswordExpirationPolicyControl
            // 
            this._thePasswordExpirationPolicyControl.ExpiryDate = "Mar 10 2010";
            this._thePasswordExpirationPolicyControl.ExpiryDays = 0;
            this._thePasswordExpirationPolicyControl.Location = new System.Drawing.Point(344, 186);
            this._thePasswordExpirationPolicyControl.Name = "_thePasswordExpirationPolicyControl";
            this._thePasswordExpirationPolicyControl.NoExpiry = true;
            this._thePasswordExpirationPolicyControl.Size = new System.Drawing.Size(323, 131);
            this._thePasswordExpirationPolicyControl.TabIndex = 9;
            // 
            // _thePwdControlGroupBox
            // 
            this._thePwdControlGroupBox.Controls.Add(this._pwdLogonDelaySpinner);
            this._thePwdControlGroupBox.Controls.Add(this._pwdCanChangeSpinner);
            this._thePwdControlGroupBox.Controls.Add(this._pwdMaxLogonSpinner);
            this._thePwdControlGroupBox.Controls.Add(this._pwdGracePeriodSpinner);
            this._thePwdControlGroupBox.Controls.Add(this._pwdHistorySpinner);
            this._thePwdControlGroupBox.Controls.Add(this.oneGuiLabel11);
            this._thePwdControlGroupBox.Controls.Add(this.oneGuiLabel10);
            this._thePwdControlGroupBox.Controls.Add(this.oneGuiLabel7);
            this._thePwdControlGroupBox.Controls.Add(this.oneGuiLabel6);
            this._thePwdControlGroupBox.Controls.Add(this.oneGuiLabel5);
            this._thePwdControlGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._thePwdControlGroupBox.Location = new System.Drawing.Point(344, 3);
            this._thePwdControlGroupBox.Name = "_thePwdControlGroupBox";
            this._thePwdControlGroupBox.Size = new System.Drawing.Size(323, 181);
            this._thePwdControlGroupBox.TabIndex = 1;
            this._thePwdControlGroupBox.TabStop = false;
            this._thePwdControlGroupBox.Text = "Password Control Policy";
            // 
            // _pwdLogonDelaySpinner
            // 
            this._pwdLogonDelaySpinner.Location = new System.Drawing.Point(226, 147);
            this._pwdLogonDelaySpinner.Maximum = new decimal(new int[] {
            86400,
            0,
            0,
            0});
            this._pwdLogonDelaySpinner.Name = "_pwdLogonDelaySpinner";
            this._pwdLogonDelaySpinner.Size = new System.Drawing.Size(72, 21);
            this._pwdLogonDelaySpinner.TabIndex = 7;
            this._pwdLogonDelaySpinner.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._pwdLogonDelaySpinner.Value = new decimal(new int[] {
            60,
            0,
            0,
            0});
            this._pwdLogonDelaySpinner.ValueChanged += new System.EventHandler(this.Object_Changed);
            // 
            // _pwdCanChangeSpinner
            // 
            this._pwdCanChangeSpinner.Location = new System.Drawing.Point(226, 87);
            this._pwdCanChangeSpinner.Maximum = new decimal(new int[] {
            365,
            0,
            0,
            0});
            this._pwdCanChangeSpinner.Name = "_pwdCanChangeSpinner";
            this._pwdCanChangeSpinner.Size = new System.Drawing.Size(72, 21);
            this._pwdCanChangeSpinner.TabIndex = 5;
            this._pwdCanChangeSpinner.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._pwdCanChangeSpinner.ValueChanged += new System.EventHandler(this.Object_Changed);
            // 
            // _pwdMaxLogonSpinner
            // 
            this._pwdMaxLogonSpinner.Location = new System.Drawing.Point(226, 117);
            this._pwdMaxLogonSpinner.Maximum = new decimal(new int[] {
            60,
            0,
            0,
            0});
            this._pwdMaxLogonSpinner.Name = "_pwdMaxLogonSpinner";
            this._pwdMaxLogonSpinner.Size = new System.Drawing.Size(72, 21);
            this._pwdMaxLogonSpinner.TabIndex = 6;
            this._pwdMaxLogonSpinner.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._pwdMaxLogonSpinner.Value = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this._pwdMaxLogonSpinner.ValueChanged += new System.EventHandler(this.Object_Changed);
            // 
            // _pwdGracePeriodSpinner
            // 
            this._pwdGracePeriodSpinner.Location = new System.Drawing.Point(226, 57);
            this._pwdGracePeriodSpinner.Maximum = new decimal(new int[] {
            365,
            0,
            0,
            0});
            this._pwdGracePeriodSpinner.Name = "_pwdGracePeriodSpinner";
            this._pwdGracePeriodSpinner.Size = new System.Drawing.Size(72, 21);
            this._pwdGracePeriodSpinner.TabIndex = 4;
            this._pwdGracePeriodSpinner.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._pwdGracePeriodSpinner.Value = new decimal(new int[] {
            7,
            0,
            0,
            0});
            this._pwdGracePeriodSpinner.ValueChanged += new System.EventHandler(this.Object_Changed);
            // 
            // _pwdHistorySpinner
            // 
            this._pwdHistorySpinner.Location = new System.Drawing.Point(226, 27);
            this._pwdHistorySpinner.Maximum = new decimal(new int[] {
            60,
            0,
            0,
            0});
            this._pwdHistorySpinner.Name = "_pwdHistorySpinner";
            this._pwdHistorySpinner.Size = new System.Drawing.Size(72, 21);
            this._pwdHistorySpinner.TabIndex = 3;
            this._pwdHistorySpinner.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._pwdHistorySpinner.Value = new decimal(new int[] {
            10,
            0,
            0,
            0});
            this._pwdHistorySpinner.ValueChanged += new System.EventHandler(this.Object_Changed);
            // 
            // oneGuiLabel11
            // 
            this.oneGuiLabel11.AutoSize = true;
            this.oneGuiLabel11.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel11.Location = new System.Drawing.Point(25, 151);
            this.oneGuiLabel11.Name = "oneGuiLabel11";
            this.oneGuiLabel11.Size = new System.Drawing.Size(125, 13);
            this.oneGuiLabel11.TabIndex = 4;
            this.oneGuiLabel11.Text = "Login Failed Delay (secs)";
            // 
            // oneGuiLabel10
            // 
            this.oneGuiLabel10.AutoSize = true;
            this.oneGuiLabel10.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel10.Location = new System.Drawing.Point(25, 121);
            this.oneGuiLabel10.Name = "oneGuiLabel10";
            this.oneGuiLabel10.Size = new System.Drawing.Size(102, 13);
            this.oneGuiLabel10.TabIndex = 2;
            this.oneGuiLabel10.Text = "Max Login Attempts";
            // 
            // oneGuiLabel7
            // 
            this.oneGuiLabel7.AutoSize = true;
            this.oneGuiLabel7.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel7.Location = new System.Drawing.Point(25, 91);
            this.oneGuiLabel7.Name = "oneGuiLabel7";
            this.oneGuiLabel7.Size = new System.Drawing.Size(187, 13);
            this.oneGuiLabel7.TabIndex = 0;
            this.oneGuiLabel7.Text = "May Change before Expiration (days)";
            // 
            // oneGuiLabel6
            // 
            this.oneGuiLabel6.AutoSize = true;
            this.oneGuiLabel6.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel6.Location = new System.Drawing.Point(25, 61);
            this.oneGuiLabel6.Name = "oneGuiLabel6";
            this.oneGuiLabel6.Size = new System.Drawing.Size(180, 13);
            this.oneGuiLabel6.TabIndex = 0;
            this.oneGuiLabel6.Text = "Grace Period after Expiration (days)";
            // 
            // oneGuiLabel5
            // 
            this.oneGuiLabel5.AutoSize = true;
            this.oneGuiLabel5.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel5.Location = new System.Drawing.Point(25, 31);
            this.oneGuiLabel5.Name = "oneGuiLabel5";
            this.oneGuiLabel5.Size = new System.Drawing.Size(69, 13);
            this.oneGuiLabel5.TabIndex = 0;
            this.oneGuiLabel5.Text = "History Level";
            // 
            // _thePwdQualityGroupBox
            // 
            this._thePwdQualityGroupBox.Controls.Add(this.oneGuiLabel8);
            this._thePwdQualityGroupBox.Controls.Add(this._pwdUpperRequired);
            this._thePwdQualityGroupBox.Controls.Add(this._pwdLowerRequired);
            this._thePwdQualityGroupBox.Controls.Add(this._pwdQualNoRepeatCharsCheckBox);
            this._thePwdQualityGroupBox.Controls.Add(this.oneGuiLabel9);
            this._thePwdQualityGroupBox.Controls.Add(this._pwdQualNoUserNameCheckBox);
            this._thePwdQualityGroupBox.Controls.Add(this._pwdNumberRequired);
            this._thePwdQualityGroupBox.Controls.Add(this._pwdMinLengthSpinner);
            this._thePwdQualityGroupBox.Controls.Add(this._numberOfPwdCriteria);
            this._thePwdQualityGroupBox.Controls.Add(this.oneGuiLabel1);
            this._thePwdQualityGroupBox.Controls.Add(this._pwdSpecialRequired);
            this._thePwdQualityGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._thePwdQualityGroupBox.Location = new System.Drawing.Point(3, 3);
            this._thePwdQualityGroupBox.Name = "_thePwdQualityGroupBox";
            this._thePwdQualityGroupBox.Size = new System.Drawing.Size(337, 314);
            this._thePwdQualityGroupBox.TabIndex = 8;
            this._thePwdQualityGroupBox.TabStop = false;
            this._thePwdQualityGroupBox.Text = "Password Quality Policy";
            // 
            // oneGuiLabel8
            // 
            this.oneGuiLabel8.AutoSize = true;
            this.oneGuiLabel8.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel8.Location = new System.Drawing.Point(21, 121);
            this.oneGuiLabel8.Name = "oneGuiLabel8";
            this.oneGuiLabel8.Size = new System.Drawing.Size(95, 13);
            this.oneGuiLabel8.TabIndex = 3;
            this.oneGuiLabel8.Text = "Password requires";
            // 
            // _pwdUpperRequired
            // 
            this._pwdUpperRequired.AutoSize = true;
            this._pwdUpperRequired.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._pwdUpperRequired.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._pwdUpperRequired.Location = new System.Drawing.Point(41, 148);
            this._pwdUpperRequired.Name = "_pwdUpperRequired";
            this._pwdUpperRequired.Size = new System.Drawing.Size(180, 18);
            this._pwdUpperRequired.TabIndex = 2;
            this._pwdUpperRequired.Text = "Uppercase Character Required";
            this._pwdUpperRequired.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._pwdUpperRequired.UseVisualStyleBackColor = true;
            this._pwdUpperRequired.CheckedChanged += new System.EventHandler(this.PwdQualRequirement_Changed);
            // 
            // _pwdLowerRequired
            // 
            this._pwdLowerRequired.AutoSize = true;
            this._pwdLowerRequired.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._pwdLowerRequired.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._pwdLowerRequired.Location = new System.Drawing.Point(41, 175);
            this._pwdLowerRequired.Name = "_pwdLowerRequired";
            this._pwdLowerRequired.Size = new System.Drawing.Size(180, 18);
            this._pwdLowerRequired.TabIndex = 3;
            this._pwdLowerRequired.Text = "Lowercase Character Required";
            this._pwdLowerRequired.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._pwdLowerRequired.UseVisualStyleBackColor = true;
            this._pwdLowerRequired.CheckedChanged += new System.EventHandler(this.PwdQualRequirement_Changed);
            // 
            // _pwdQualNoRepeatCharsCheckBox
            // 
            this._pwdQualNoRepeatCharsCheckBox.AutoSize = true;
            this._pwdQualNoRepeatCharsCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._pwdQualNoRepeatCharsCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._pwdQualNoRepeatCharsCheckBox.Location = new System.Drawing.Point(24, 59);
            this._pwdQualNoRepeatCharsCheckBox.Name = "_pwdQualNoRepeatCharsCheckBox";
            this._pwdQualNoRepeatCharsCheckBox.Size = new System.Drawing.Size(311, 18);
            this._pwdQualNoRepeatCharsCheckBox.TabIndex = 7;
            this._pwdQualNoRepeatCharsCheckBox.Text = "Do Not Repeat a Character More Than Once in Succession";
            this._pwdQualNoRepeatCharsCheckBox.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._pwdQualNoRepeatCharsCheckBox.UseVisualStyleBackColor = true;
            this._pwdQualNoRepeatCharsCheckBox.Click += new System.EventHandler(this.Object_Changed);
            // 
            // oneGuiLabel9
            // 
            this.oneGuiLabel9.AutoSize = true;
            this.oneGuiLabel9.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel9.Location = new System.Drawing.Point(164, 121);
            this.oneGuiLabel9.Name = "oneGuiLabel9";
            this.oneGuiLabel9.Size = new System.Drawing.Size(89, 13);
            this.oneGuiLabel9.TabIndex = 5;
            this.oneGuiLabel9.Text = "following criteria:";
            // 
            // _pwdQualNoUserNameCheckBox
            // 
            this._pwdQualNoUserNameCheckBox.AutoSize = true;
            this._pwdQualNoUserNameCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._pwdQualNoUserNameCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._pwdQualNoUserNameCheckBox.Location = new System.Drawing.Point(24, 31);
            this._pwdQualNoUserNameCheckBox.Name = "_pwdQualNoUserNameCheckBox";
            this._pwdQualNoUserNameCheckBox.Size = new System.Drawing.Size(201, 18);
            this._pwdQualNoUserNameCheckBox.TabIndex = 6;
            this._pwdQualNoUserNameCheckBox.Text = "Do Not Allow to Contain User Name";
            this._pwdQualNoUserNameCheckBox.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._pwdQualNoUserNameCheckBox.UseVisualStyleBackColor = true;
            this._pwdQualNoUserNameCheckBox.Click += new System.EventHandler(this.Object_Changed);
            // 
            // _pwdNumberRequired
            // 
            this._pwdNumberRequired.AutoSize = true;
            this._pwdNumberRequired.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._pwdNumberRequired.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._pwdNumberRequired.Location = new System.Drawing.Point(41, 202);
            this._pwdNumberRequired.Name = "_pwdNumberRequired";
            this._pwdNumberRequired.Size = new System.Drawing.Size(167, 18);
            this._pwdNumberRequired.TabIndex = 4;
            this._pwdNumberRequired.Text = "Numeric Character Required";
            this._pwdNumberRequired.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._pwdNumberRequired.UseVisualStyleBackColor = true;
            this._pwdNumberRequired.CheckedChanged += new System.EventHandler(this.PwdQualRequirement_Changed);
            // 
            // _pwdMinLengthSpinner
            // 
            this._pwdMinLengthSpinner.Location = new System.Drawing.Point(122, 87);
            this._pwdMinLengthSpinner.Maximum = new decimal(new int[] {
            64,
            0,
            0,
            0});
            this._pwdMinLengthSpinner.Minimum = new decimal(new int[] {
            6,
            0,
            0,
            0});
            this._pwdMinLengthSpinner.Name = "_pwdMinLengthSpinner";
            this._pwdMinLengthSpinner.Size = new System.Drawing.Size(72, 21);
            this._pwdMinLengthSpinner.TabIndex = 1;
            this._pwdMinLengthSpinner.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._pwdMinLengthSpinner.Value = new decimal(new int[] {
            8,
            0,
            0,
            0});
            this._pwdMinLengthSpinner.ValueChanged += new System.EventHandler(this.Object_Changed);
            // 
            // _numberOfPwdCriteria
            // 
            this._numberOfPwdCriteria.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._numberOfPwdCriteria.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._numberOfPwdCriteria.FormattingEnabled = true;
            this._numberOfPwdCriteria.Items.AddRange(new object[] {
            "0",
            "1",
            "2",
            "3",
            "4"});
            this._numberOfPwdCriteria.Location = new System.Drawing.Point(122, 117);
            this._numberOfPwdCriteria.MaxDropDownItems = 4;
            this._numberOfPwdCriteria.Name = "_numberOfPwdCriteria";
            this._numberOfPwdCriteria.Size = new System.Drawing.Size(34, 21);
            this._numberOfPwdCriteria.TabIndex = 1;
            this._numberOfPwdCriteria.SelectedIndexChanged += new System.EventHandler(this._numberOfPwdCriteria_SelectedIndexChanged);
            // 
            // oneGuiLabel1
            // 
            this.oneGuiLabel1.AutoSize = true;
            this.oneGuiLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel1.Location = new System.Drawing.Point(21, 91);
            this.oneGuiLabel1.Name = "oneGuiLabel1";
            this.oneGuiLabel1.Size = new System.Drawing.Size(83, 13);
            this.oneGuiLabel1.TabIndex = 0;
            this.oneGuiLabel1.Text = "Minimum Length";
            // 
            // _pwdSpecialRequired
            // 
            this._pwdSpecialRequired.AutoSize = true;
            this._pwdSpecialRequired.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._pwdSpecialRequired.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._pwdSpecialRequired.Location = new System.Drawing.Point(41, 229);
            this._pwdSpecialRequired.Name = "_pwdSpecialRequired";
            this._pwdSpecialRequired.Size = new System.Drawing.Size(162, 18);
            this._pwdSpecialRequired.TabIndex = 5;
            this._pwdSpecialRequired.Text = "Special Character Required";
            this._pwdSpecialRequired.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._pwdSpecialRequired.UseVisualStyleBackColor = true;
            this._pwdSpecialRequired.CheckedChanged += new System.EventHandler(this.PwdQualRequirement_Changed);
            // 
            // _toolTip
            // 
            this._toolTip.IsBalloon = true;
            // 
            // SecurityPoliciesPasswordUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.oneGuiPanel1);
            this.Name = "SecurityPoliciesPasswordUserControl";
            this.Size = new System.Drawing.Size(682, 402);
            this.oneGuiPanel1.ResumeLayout(false);
            this._thePwdControlGroupBox.ResumeLayout(false);
            this._thePwdControlGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._pwdLogonDelaySpinner)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._pwdCanChangeSpinner)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._pwdMaxLogonSpinner)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._pwdGracePeriodSpinner)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._pwdHistorySpinner)).EndInit();
            this._thePwdQualityGroupBox.ResumeLayout(false);
            this._thePwdQualityGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._pwdMinLengthSpinner)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _thePwdControlGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _pwdLogonDelaySpinner;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _pwdCanChangeSpinner;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _pwdMaxLogonSpinner;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _pwdGracePeriodSpinner;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _pwdHistorySpinner;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _pwdMinLengthSpinner;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel11;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel10;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel7;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel6;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel5;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _thePwdQualityGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel9;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _numberOfPwdCriteria;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel8;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _pwdSpecialRequired;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _pwdNumberRequired;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _pwdLowerRequired;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _pwdUpperRequired;
        private PasswordExpirationPolicyControl _thePasswordExpirationPolicyControl;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _pwdQualNoUserNameCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _pwdQualNoRepeatCharsCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip _toolTip;
    }
}
