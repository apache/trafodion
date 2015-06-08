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
using Trafodion.Manager.Framework.Controls;
namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    partial class NameFilterDialog
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.theNamePartTextBox = new TrafodionTextBox();
            this.theWhereComboBox = new TrafodionComboBox();
            this.theCancelButton = new TrafodionButton();
            this.theOKButton = new TrafodionButton();
            this.theObjectNameCheckBox = new TrafodionCheckBox();
            this.SuspendLayout();
            // 
            // theNamePartTextBox
            // 
            this.theNamePartTextBox.Location = new System.Drawing.Point(208, 40);
            this.theNamePartTextBox.Name = "theNamePartTextBox";
            this.theNamePartTextBox.Size = new System.Drawing.Size(379, 20);
            this.theNamePartTextBox.TabIndex = 4;
            this.theNamePartTextBox.TextChanged += new System.EventHandler(this.theNamePartTextBox_TextChanged);
            // 
            // theWhereComboBox
            // 
            this.theWhereComboBox.FormattingEnabled = true;
            this.theWhereComboBox.Items.AddRange(new object[] {
            "is exactly",
            "starts with",
            "contains",
            "ends with",
            "all"});
            this.theWhereComboBox.Location = new System.Drawing.Point(104, 40);
            this.theWhereComboBox.Name = "theWhereComboBox";
            this.theWhereComboBox.Size = new System.Drawing.Size(98, 21);
            this.theWhereComboBox.TabIndex = 5;
            this.theWhereComboBox.SelectedIndexChanged += new System.EventHandler(this.theWhereComboBox_SelectedIndexChanged);
            // 
            // theCancelButton
            // 
            this.theCancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.theCancelButton.Location = new System.Drawing.Point(512, 117);
            this.theCancelButton.Name = "theCancelButton";
            this.theCancelButton.Size = new System.Drawing.Size(75, 23);
            this.theCancelButton.TabIndex = 6;
            this.theCancelButton.Text = "&Cancel";
            // 
            // theOKButton
            // 
            this.theOKButton.Location = new System.Drawing.Point(431, 117);
            this.theOKButton.Name = "theOKButton";
            this.theOKButton.Size = new System.Drawing.Size(75, 23);
            this.theOKButton.TabIndex = 7;
            this.theOKButton.Text = "&OK";
            this.theOKButton.Click += new System.EventHandler(this.theOKButton_Click);
            // 
            // theObjectNameCheckBox
            // 
            this.theObjectNameCheckBox.AutoSize = true;
            this.theObjectNameCheckBox.Location = new System.Drawing.Point(12, 42);
            this.theObjectNameCheckBox.Name = "theObjectNameCheckBox";
            this.theObjectNameCheckBox.Size = new System.Drawing.Size(86, 17);
            this.theObjectNameCheckBox.TabIndex = 8;
            this.theObjectNameCheckBox.Text = "Object name";
            this.theObjectNameCheckBox.CheckedChanged += new System.EventHandler(this.theObjectNameCheckBox_CheckedChanged);
            // 
            // NameFilterDialog
            // 
            this.AcceptButton = this.theOKButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.theCancelButton;
            this.ClientSize = new System.Drawing.Size(599, 152);
            this.Controls.Add(this.theObjectNameCheckBox);
            this.Controls.Add(this.theOKButton);
            this.Controls.Add(this.theCancelButton);
            this.Controls.Add(this.theWhereComboBox);
            this.Controls.Add(this.theNamePartTextBox);
            this.Name = "NameFilterDialog";
            this.Text = "Filter By";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private TrafodionTextBox theNamePartTextBox;
        private TrafodionComboBox theWhereComboBox;
        private TrafodionButton theCancelButton;
        private TrafodionButton theOKButton;
        private TrafodionCheckBox theObjectNameCheckBox;
    }
}