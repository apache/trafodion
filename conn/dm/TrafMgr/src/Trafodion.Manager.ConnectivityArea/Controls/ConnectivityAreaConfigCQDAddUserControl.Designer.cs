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
﻿using Trafodion.Manager.Framework.Controls;
namespace Trafodion.Manager.ConnectivityArea.Controls
{
    partial class ConnectivityAreaConfigCQDAddUserControl
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
            Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel1;
            Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel2;
            this.CQDName_TrafodionTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.CQDValue_TrafodionTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionLabel1
            // 
            TrafodionLabel1.AutoSize = true;
            TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8F);
            TrafodionLabel1.Location = new System.Drawing.Point(14, 14);
            TrafodionLabel1.Name = "TrafodionLabel1";
            TrafodionLabel1.Size = new System.Drawing.Size(62, 14);
            TrafodionLabel1.TabIndex = 17;
            TrafodionLabel1.Text = "CQD Name:";
            // 
            // TrafodionLabel2
            // 
            TrafodionLabel2.AutoSize = true;
            TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8F);
            TrafodionLabel2.Location = new System.Drawing.Point(14, 40);
            TrafodionLabel2.Name = "TrafodionLabel2";
            TrafodionLabel2.Size = new System.Drawing.Size(38, 14);
            TrafodionLabel2.TabIndex = 19;
            TrafodionLabel2.Text = "Value:";
            // 
            // CQDName_TrafodionTextBox
            // 
            this.CQDName_TrafodionTextBox.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper;
            this.CQDName_TrafodionTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.CQDName_TrafodionTextBox.Location = new System.Drawing.Point(82, 11);
            this.CQDName_TrafodionTextBox.Name = "CQDName_TrafodionTextBox";
            this.CQDName_TrafodionTextBox.Size = new System.Drawing.Size(279, 20);
            this.CQDName_TrafodionTextBox.TabIndex = 0;
            this.CQDName_TrafodionTextBox.TextChanged += new System.EventHandler(this.CQDName_TrafodionTextBox_TextChanged);
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.Controls.Add(TrafodionLabel2);
            this.TrafodionPanel1.Controls.Add(this.CQDValue_TrafodionTextBox);
            this.TrafodionPanel1.Controls.Add(TrafodionLabel1);
            this.TrafodionPanel1.Controls.Add(this.CQDName_TrafodionTextBox);
            this.TrafodionPanel1.Location = new System.Drawing.Point(9, 29);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(376, 173);
            this.TrafodionPanel1.TabIndex = 16;
            // 
            // CQDValue_TrafodionTextBox
            // 
            this.CQDValue_TrafodionTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.CQDValue_TrafodionTextBox.Location = new System.Drawing.Point(82, 37);
            this.CQDValue_TrafodionTextBox.Multiline = true;
            this.CQDValue_TrafodionTextBox.Name = "CQDValue_TrafodionTextBox";
            this.CQDValue_TrafodionTextBox.Size = new System.Drawing.Size(279, 121);
            this.CQDValue_TrafodionTextBox.TabIndex = 18;
            this.CQDValue_TrafodionTextBox.TextChanged += new System.EventHandler(this.CQDValue_TrafodionTextBox_TextChanged);
            // 
            // TrafodionLabel3
            // 
            this.TrafodionLabel3.AutoSize = true;
            this.TrafodionLabel3.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel3.Location = new System.Drawing.Point(7, 12);
            this.TrafodionLabel3.Name = "TrafodionLabel3";
            this.TrafodionLabel3.Size = new System.Drawing.Size(131, 14);
            this.TrafodionLabel3.TabIndex = 17;
            this.TrafodionLabel3.Text = "Provide a name and value";
            // 
            // ConnectivityAreaConfigCQDAddUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.TrafodionLabel3);
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "ConnectivityAreaConfigCQDAddUserControl";
            this.Size = new System.Drawing.Size(393, 263);
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private TrafodionTextBox CQDName_TrafodionTextBox;
        private TrafodionToolTip _theToolTip;
        private TrafodionPanel TrafodionPanel1;
        private TrafodionTextBox CQDValue_TrafodionTextBox;
        private TrafodionLabel TrafodionLabel3;
    }
}
