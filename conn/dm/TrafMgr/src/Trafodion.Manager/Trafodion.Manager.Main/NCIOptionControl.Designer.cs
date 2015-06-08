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
﻿namespace Trafodion.Manager.Main
{
    partial class NCIOptionControl
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
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.customTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.customCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.schemaCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.serverCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.userCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionCheckBox1 = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.toolTip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.TrafodionGroupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.browseButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.nciExecutableText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionGroupBox1.SuspendLayout();
            this.TrafodionGroupBox2.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel2.Location = new System.Drawing.Point(23, 93);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(80, 13);
            this.TrafodionLabel2.TabIndex = 6;
            this.TrafodionLabel2.Text = "Custom Prompt";
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.Controls.Add(this.customTextBox);
            this.TrafodionGroupBox1.Controls.Add(this.TrafodionLabel2);
            this.TrafodionGroupBox1.Controls.Add(this.customCheckBox);
            this.TrafodionGroupBox1.Controls.Add(this.schemaCheckBox);
            this.TrafodionGroupBox1.Controls.Add(this.serverCheckBox);
            this.TrafodionGroupBox1.Controls.Add(this.userCheckBox);
            this.TrafodionGroupBox1.Controls.Add(this.TrafodionLabel1);
            this.TrafodionGroupBox1.Controls.Add(this.TrafodionCheckBox1);
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(6, 66);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(452, 134);
            this.TrafodionGroupBox1.TabIndex = 5;
            this.TrafodionGroupBox1.TabStop = false;
            this.TrafodionGroupBox1.Text = "TRAFCI Executable Options ";
            // 
            // customTextBox
            // 
            this.customTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.customTextBox.Location = new System.Drawing.Point(47, 108);
            this.customTextBox.Name = "customTextBox";
            this.customTextBox.Size = new System.Drawing.Size(241, 20);
            this.customTextBox.TabIndex = 7;
            // 
            // customCheckBox
            // 
            this.customCheckBox.AutoSize = true;
            this.customCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.customCheckBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.customCheckBox.Location = new System.Drawing.Point(26, 110);
            this.customCheckBox.Name = "customCheckBox";
            this.customCheckBox.Size = new System.Drawing.Size(41, 18);
            this.customCheckBox.TabIndex = 5;
            this.customCheckBox.Text = "   ";
            this.customCheckBox.UseVisualStyleBackColor = true;
            this.customCheckBox.CheckedChanged += new System.EventHandler(this.customCheckBox_CheckedChanged);
            // 
            // schemaCheckBox
            // 
            this.schemaCheckBox.AutoSize = true;
            this.schemaCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.schemaCheckBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.schemaCheckBox.Location = new System.Drawing.Point(148, 70);
            this.schemaCheckBox.Name = "schemaCheckBox";
            this.schemaCheckBox.Size = new System.Drawing.Size(69, 18);
            this.schemaCheckBox.TabIndex = 4;
            this.schemaCheckBox.Text = "Schema";
            this.schemaCheckBox.UseVisualStyleBackColor = true;
            this.schemaCheckBox.CheckedChanged += new System.EventHandler(this.schemaCheckBox_CheckedChanged);
            // 
            // serverCheckBox
            // 
            this.serverCheckBox.AutoSize = true;
            this.serverCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.serverCheckBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.serverCheckBox.Location = new System.Drawing.Point(82, 71);
            this.serverCheckBox.Name = "serverCheckBox";
            this.serverCheckBox.Size = new System.Drawing.Size(64, 18);
            this.serverCheckBox.TabIndex = 3;
            this.serverCheckBox.Text = "Server";
            this.serverCheckBox.UseVisualStyleBackColor = true;
            this.serverCheckBox.CheckedChanged += new System.EventHandler(this.serverCheckBox_CheckedChanged);
            // 
            // userCheckBox
            // 
            this.userCheckBox.AutoSize = true;
            this.userCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.userCheckBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.userCheckBox.Location = new System.Drawing.Point(26, 71);
            this.userCheckBox.Name = "userCheckBox";
            this.userCheckBox.Size = new System.Drawing.Size(54, 18);
            this.userCheckBox.TabIndex = 2;
            this.userCheckBox.Text = "User";
            this.userCheckBox.UseVisualStyleBackColor = true;
            this.userCheckBox.CheckedChanged += new System.EventHandler(this.userCheckBox_CheckedChanged);
            // 
            // TrafodionLabel1
            // 
            this.TrafodionLabel1.AutoSize = true;
            this.TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel1.Location = new System.Drawing.Point(10, 53);
            this.TrafodionLabel1.Name = "TrafodionLabel1";
            this.TrafodionLabel1.Size = new System.Drawing.Size(85, 13);
            this.TrafodionLabel1.TabIndex = 1;
            this.TrafodionLabel1.Text = "Prompt Options:";
            // 
            // TrafodionCheckBox1
            // 
            this.TrafodionCheckBox1.AutoSize = true;
            this.TrafodionCheckBox1.Checked = true;
            this.TrafodionCheckBox1.CheckState = System.Windows.Forms.CheckState.Checked;
            this.TrafodionCheckBox1.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.TrafodionCheckBox1.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionCheckBox1.Location = new System.Drawing.Point(10, 28);
            this.TrafodionCheckBox1.Name = "TrafodionCheckBox1";
            this.TrafodionCheckBox1.Size = new System.Drawing.Size(87, 18);
            this.TrafodionCheckBox1.TabIndex = 0;
            this.TrafodionCheckBox1.Text = "Auto Logon";
            this.TrafodionCheckBox1.UseVisualStyleBackColor = true;
            this.TrafodionCheckBox1.CheckedChanged += new System.EventHandler(this.TrafodionCheckBox1_CheckedChanged);
            // 
            // toolTip1
            // 
            this.toolTip1.IsBalloon = true;
            // 
            // TrafodionGroupBox2
            // 
            this.TrafodionGroupBox2.Controls.Add(this.browseButton);
            this.TrafodionGroupBox2.Controls.Add(this.nciExecutableText);
            this.TrafodionGroupBox2.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionGroupBox2.Location = new System.Drawing.Point(6, 3);
            this.TrafodionGroupBox2.Name = "TrafodionGroupBox2";
            this.TrafodionGroupBox2.Size = new System.Drawing.Size(539, 57);
            this.TrafodionGroupBox2.TabIndex = 6;
            this.TrafodionGroupBox2.TabStop = false;
            this.TrafodionGroupBox2.Text = "TRAFCI Executable";
            // 
            // browseButton
            // 
            this.browseButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.browseButton.Location = new System.Drawing.Point(458, 26);
            this.browseButton.Name = "browseButton";
            this.browseButton.Size = new System.Drawing.Size(75, 23);
            this.browseButton.TabIndex = 1;
            this.browseButton.Text = "&Browse...";
            this.browseButton.UseVisualStyleBackColor = true;
            this.browseButton.Click += new System.EventHandler(this.browseButton_Click);
            // 
            // nciExecutableText
            // 
            this.nciExecutableText.Font = new System.Drawing.Font("Tahoma", 8F);
            this.nciExecutableText.Location = new System.Drawing.Point(10, 28);
            this.nciExecutableText.Name = "nciExecutableText";
            this.nciExecutableText.Size = new System.Drawing.Size(442, 20);
            this.nciExecutableText.TabIndex = 0;
            // 
            // NCIOptionControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.TrafodionGroupBox2);
            this.Controls.Add(this.TrafodionGroupBox1);
            this.Name = "NCIOptionControl";
            this.Size = new System.Drawing.Size(548, 210);
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.TrafodionGroupBox1.PerformLayout();
            this.TrafodionGroupBox2.ResumeLayout(false);
            this.TrafodionGroupBox2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox TrafodionGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox customTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip toolTip1;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox customCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox schemaCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox serverCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox userCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox TrafodionCheckBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox TrafodionGroupBox2;
        private Trafodion.Manager.Framework.Controls.TrafodionButton browseButton;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox nciExecutableText;

    }
}
