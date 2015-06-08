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
namespace Trafodion.Manager.Framework.Controls
{
    partial class GetStringDialog
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
            this.thePromptLabel = new TrafodionLabel();
            this.theTextBox = new TrafodionTextBox();
            this.theCancelButton = new TrafodionButton();
            this.theOKButton = new TrafodionButton();
            this.theErrorMessageLabel = new TrafodionLabel();
            this.SuspendLayout();
            // 
            // thePromptLabel
            // 
            this.thePromptLabel.AutoSize = true;
            this.thePromptLabel.Location = new System.Drawing.Point(32, 19);
            this.thePromptLabel.Name = "thePromptLabel";
            this.thePromptLabel.Size = new System.Drawing.Size(35, 13);
            this.thePromptLabel.TabIndex = 0;
            this.thePromptLabel.Text = "label1";
            // 
            // theTextBox
            // 
            this.theTextBox.Location = new System.Drawing.Point(35, 52);
            this.theTextBox.Name = "theTextBox";
            this.theTextBox.Size = new System.Drawing.Size(350, 20);
            this.theTextBox.TabIndex = 1;
            this.theTextBox.TextChanged += new System.EventHandler(this.theTextBox_TextChanged);
            // 
            // theCancelButton
            // 
            this.theCancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.theCancelButton.Location = new System.Drawing.Point(334, 105);
            this.theCancelButton.Name = "theCancelButton";
            this.theCancelButton.Size = new System.Drawing.Size(75, 23);
            this.theCancelButton.TabIndex = 2;
            this.theCancelButton.Text = "Cancel";
            // 
            // theOKButton
            // 
            this.theOKButton.Location = new System.Drawing.Point(253, 105);
            this.theOKButton.Name = "theOKButton";
            this.theOKButton.Size = new System.Drawing.Size(75, 23);
            this.theOKButton.TabIndex = 3;
            this.theOKButton.Text = "OK";
            this.theOKButton.Click += new System.EventHandler(this.theOKButton_Click);
            // 
            // theErrorMessageLabel
            // 
            this.theErrorMessageLabel.AutoSize = true;
            this.theErrorMessageLabel.ForeColor = System.Drawing.Color.Red;
            this.theErrorMessageLabel.Location = new System.Drawing.Point(35, 79);
            this.theErrorMessageLabel.Name = "theErrorMessageLabel";
            this.theErrorMessageLabel.Size = new System.Drawing.Size(35, 13);
            this.theErrorMessageLabel.TabIndex = 4;
            this.theErrorMessageLabel.Text = "label1";
            // 
            // QuestionDialog
            // 
            this.AcceptButton = this.theOKButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.theCancelButton;
            this.ClientSize = new System.Drawing.Size(421, 140);
            this.Controls.Add(this.theErrorMessageLabel);
            this.Controls.Add(this.theOKButton);
            this.Controls.Add(this.theCancelButton);
            this.Controls.Add(this.theTextBox);
            this.Controls.Add(this.thePromptLabel);
            this.Name = Properties.Resources.QuestionDialog;
            this.Text = Properties.Resources.ProductName + " - " + Name;
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private TrafodionLabel thePromptLabel;
        private TrafodionTextBox theTextBox;
        private TrafodionButton theCancelButton;
        private TrafodionButton theOKButton;
        private TrafodionLabel theErrorMessageLabel;
    }
}