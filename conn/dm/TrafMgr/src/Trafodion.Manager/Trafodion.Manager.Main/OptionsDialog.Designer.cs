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
    partial class OptionsDialog
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
            this.components = new System.ComponentModel.Container();
            this.TrafodionCheckBox1 = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.TrafodionOptionsDoneButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.customTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.customCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.schemaCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.serverCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.userCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.toolTip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.TrafodionButton1 = new Trafodion.Manager.Framework.Controls.TrafodionHelpButton("auto_logon.html");
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionGroupBox1.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this.TrafodionPanel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionCheckBox1
            // 
            this.TrafodionCheckBox1.AutoSize = true;
            this.TrafodionCheckBox1.Checked = true;
            this.TrafodionCheckBox1.CheckState = System.Windows.Forms.CheckState.Checked;
            this.TrafodionCheckBox1.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionCheckBox1.Location = new System.Drawing.Point(10, 29);
            this.TrafodionCheckBox1.Name = "TrafodionCheckBox1";
            this.TrafodionCheckBox1.Size = new System.Drawing.Size(82, 18);
            this.TrafodionCheckBox1.TabIndex = 0;
            this.TrafodionCheckBox1.Text = "Auto Logon";
            this.toolTip1.SetToolTip(this.TrafodionCheckBox1, "Starts NCI with the current user");
            this.TrafodionCheckBox1.UseVisualStyleBackColor = true;
            this.TrafodionCheckBox1.CheckedChanged += new System.EventHandler(this.TrafodionCheckBox1_CheckedChanged);
            // 
            // TrafodionOptionsDoneButton
            // 
            this.TrafodionOptionsDoneButton.Location = new System.Drawing.Point(111, 3);
            this.TrafodionOptionsDoneButton.Name = "TrafodionOptionsDoneButton";
            this.TrafodionOptionsDoneButton.Size = new System.Drawing.Size(75, 28);
            this.TrafodionOptionsDoneButton.TabIndex = 1;
            this.TrafodionOptionsDoneButton.Text = "Done";
            this.TrafodionOptionsDoneButton.UseVisualStyleBackColor = true;
            this.TrafodionOptionsDoneButton.Click += new System.EventHandler(this.TrafodionOptionsDoneButton_Click);
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
            this.TrafodionGroupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(397, 168);
            this.TrafodionGroupBox1.TabIndex = 2;
            this.TrafodionGroupBox1.TabStop = false;
            this.TrafodionGroupBox1.Text = global::Trafodion.Manager.Properties.Resources.NCI;
            // 
            // customTextBox
            // 
            this.customTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.customTextBox.Location = new System.Drawing.Point(47, 127);
            this.customTextBox.Name = "customTextBox";
            this.customTextBox.Size = new System.Drawing.Size(241, 20);
            this.customTextBox.TabIndex = 7;
            this.toolTip1.SetToolTip(this.customTextBox, "Usage: %User %Server %Mode %Server %Datasource\r\nExample: \"%User >\"  or \"%User on " +
                    "%Server >\"");
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel2.Location = new System.Drawing.Point(23, 108);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(79, 14);
            this.TrafodionLabel2.TabIndex = 6;
            this.TrafodionLabel2.Text = "Custom Prompt";
            // 
            // customCheckBox
            // 
            this.customCheckBox.AutoSize = true;
            this.customCheckBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.customCheckBox.Location = new System.Drawing.Point(26, 129);
            this.customCheckBox.Name = "customCheckBox";
            this.customCheckBox.Size = new System.Drawing.Size(15, 14);
            this.customCheckBox.TabIndex = 5;
            this.customCheckBox.UseVisualStyleBackColor = true;
            this.customCheckBox.CheckedChanged += new System.EventHandler(this.customCheckBox_CheckedChanged);
            // 
            // schemaCheckBox
            // 
            this.schemaCheckBox.AutoSize = true;
            this.schemaCheckBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.schemaCheckBox.Location = new System.Drawing.Point(148, 80);
            this.schemaCheckBox.Name = "schemaCheckBox";
            this.schemaCheckBox.Size = new System.Drawing.Size(65, 18);
            this.schemaCheckBox.TabIndex = 4;
            this.schemaCheckBox.Text = "Schema";
            this.toolTip1.SetToolTip(this.schemaCheckBox, "Prompt shows current schema");
            this.schemaCheckBox.UseVisualStyleBackColor = true;
            this.schemaCheckBox.CheckedChanged += new System.EventHandler(this.schemaCheckBox_CheckedChanged);
            // 
            // serverCheckBox
            // 
            this.serverCheckBox.AutoSize = true;
            this.serverCheckBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.serverCheckBox.Location = new System.Drawing.Point(82, 81);
            this.serverCheckBox.Name = "serverCheckBox";
            this.serverCheckBox.Size = new System.Drawing.Size(59, 18);
            this.serverCheckBox.TabIndex = 3;
            this.serverCheckBox.Text = "Server";
            this.toolTip1.SetToolTip(this.serverCheckBox, "Sets prompt to show server name");
            this.serverCheckBox.UseVisualStyleBackColor = true;
            this.serverCheckBox.CheckedChanged += new System.EventHandler(this.serverCheckBox_CheckedChanged);
            // 
            // userCheckBox
            // 
            this.userCheckBox.AutoSize = true;
            this.userCheckBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.userCheckBox.Location = new System.Drawing.Point(26, 81);
            this.userCheckBox.Name = "userCheckBox";
            this.userCheckBox.Size = new System.Drawing.Size(49, 18);
            this.userCheckBox.TabIndex = 2;
            this.userCheckBox.Text = "User";
            this.toolTip1.SetToolTip(this.userCheckBox, "Sets prompt to show current user");
            this.userCheckBox.UseVisualStyleBackColor = true;
            this.userCheckBox.CheckedChanged += new System.EventHandler(this.userCheckBox_CheckedChanged);
            // 
            // TrafodionLabel1
            // 
            this.TrafodionLabel1.AutoSize = true;
            this.TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel1.Location = new System.Drawing.Point(10, 63);
            this.TrafodionLabel1.Name = "TrafodionLabel1";
            this.TrafodionLabel1.Size = new System.Drawing.Size(83, 14);
            this.TrafodionLabel1.TabIndex = 1;
            this.TrafodionLabel1.Text = "Prompt Options:";
            // 
            // TrafodionButton1
            // 
            this.TrafodionButton1.Location = new System.Drawing.Point(211, 3);
            this.TrafodionButton1.Name = "TrafodionButton1";
            this.TrafodionButton1.Size = new System.Drawing.Size(75, 28);
            this.TrafodionButton1.TabIndex = 3;
            //this.TrafodionButton1.Text = "Help";
            this.TrafodionButton1.UseVisualStyleBackColor = true;
            //this.TrafodionButton1.Click += new System.EventHandler(this.TrafodionButton1_Click);
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.Controls.Add(this.TrafodionButton1);
            this.TrafodionPanel1.Controls.Add(this.TrafodionOptionsDoneButton);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 168);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(397, 36);
            this.TrafodionPanel1.TabIndex = 4;
            // 
            // TrafodionPanel2
            // 
            this.TrafodionPanel2.Controls.Add(this.TrafodionGroupBox1);
            this.TrafodionPanel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel2.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel2.Name = "TrafodionPanel2";
            this.TrafodionPanel2.Size = new System.Drawing.Size(397, 168);
            this.TrafodionPanel2.TabIndex = 5;
            // 
            // OptionsDialog
            // 
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.ClientSize = new System.Drawing.Size(397, 204);
            this.Controls.Add(this.TrafodionPanel2);
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "OptionsDialog";
            this.Text = "Trafodion Database Manager - Options";
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.TrafodionGroupBox1.PerformLayout();
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel2.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox TrafodionCheckBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton TrafodionOptionsDoneButton;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox TrafodionGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox userCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox schemaCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox serverCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox customTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox customCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip toolTip1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton TrafodionButton1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel2;
    }
}
