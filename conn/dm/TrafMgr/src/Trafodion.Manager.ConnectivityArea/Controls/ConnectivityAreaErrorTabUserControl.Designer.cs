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
﻿namespace Trafodion.Manager.ConnectivityArea.Controls
{
    partial class ConnectivityAreaErrorTabUserControl
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
            this.TrafodionTextBox1 = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionTextBox1
            // 
            this.TrafodionTextBox1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionTextBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionTextBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionTextBox1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionTextBox1.Multiline = true;
            this.TrafodionTextBox1.Name = "TrafodionTextBox1";
            this.TrafodionTextBox1.Size = new System.Drawing.Size(832, 613);
            this.TrafodionTextBox1.TabIndex = 1;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this.TrafodionTextBox1);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(832, 613);
            this.TrafodionPanel1.TabIndex = 2;
            // 
            // ConnectivityAreaErrorTabUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "ConnectivityAreaErrorTabUserControl";
            this.Size = new System.Drawing.Size(832, 613);
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionTextBox TrafodionTextBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;



    }
}
