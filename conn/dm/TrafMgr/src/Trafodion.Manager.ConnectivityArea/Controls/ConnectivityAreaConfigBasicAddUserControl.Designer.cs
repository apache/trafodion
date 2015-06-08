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
    partial class ConnectivityAreaConfigBasicAddUserControl
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
            this.basicName_TrafodionTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.basicLabel_TrafodionLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.basicValOrAttribute_TrafodionTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionLabel1
            // 
            TrafodionLabel1.AutoSize = true;
            TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8F);
            TrafodionLabel1.Location = new System.Drawing.Point(14, 14);
            TrafodionLabel1.Name = "TrafodionLabel1";
            TrafodionLabel1.Size = new System.Drawing.Size(37, 14);
            TrafodionLabel1.TabIndex = 17;
            TrafodionLabel1.Text = "Name:";
            // 
            // basicName_TrafodionTextBox
            // 
            this.basicName_TrafodionTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.basicName_TrafodionTextBox.Location = new System.Drawing.Point(74, 11);
            this.basicName_TrafodionTextBox.Name = "basicName_TrafodionTextBox";
            this.basicName_TrafodionTextBox.Size = new System.Drawing.Size(287, 20);
            this.basicName_TrafodionTextBox.TabIndex = 0;
            this.basicName_TrafodionTextBox.TextChanged += new System.EventHandler(this.basicName_TrafodionTextBox_TextChanged);
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.Controls.Add(this.basicLabel_TrafodionLabel);
            this.TrafodionPanel1.Controls.Add(this.basicValOrAttribute_TrafodionTextBox);
            this.TrafodionPanel1.Controls.Add(TrafodionLabel1);
            this.TrafodionPanel1.Controls.Add(this.basicName_TrafodionTextBox);
            this.TrafodionPanel1.Location = new System.Drawing.Point(9, 29);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(376, 100);
            this.TrafodionPanel1.TabIndex = 16;
            // 
            // basicLabel_TrafodionLabel
            // 
            this.basicLabel_TrafodionLabel.AutoSize = true;
            this.basicLabel_TrafodionLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this.basicLabel_TrafodionLabel.Location = new System.Drawing.Point(14, 40);
            this.basicLabel_TrafodionLabel.Name = "basicLabel_TrafodionLabel";
            this.basicLabel_TrafodionLabel.Size = new System.Drawing.Size(54, 14);
            this.basicLabel_TrafodionLabel.TabIndex = 19;
            this.basicLabel_TrafodionLabel.Text = "Attribute: ";
            // 
            // basicValOrAttribute_TrafodionTextBox
            // 
            this.basicValOrAttribute_TrafodionTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.basicValOrAttribute_TrafodionTextBox.Location = new System.Drawing.Point(74, 37);
            this.basicValOrAttribute_TrafodionTextBox.Name = "basicValOrAttribute_TrafodionTextBox";
            this.basicValOrAttribute_TrafodionTextBox.Size = new System.Drawing.Size(287, 20);
            this.basicValOrAttribute_TrafodionTextBox.TabIndex = 18;
            this.basicValOrAttribute_TrafodionTextBox.TextChanged += new System.EventHandler(this.basicValOrAttribute_TrafodionTextBox_TextChanged);
            // 
            // ConnectivityAreaConfigBasicAddUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "ConnectivityAreaConfigBasicAddUserControl";
            this.Size = new System.Drawing.Size(393, 140);
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionTextBox basicName_TrafodionTextBox;
        private TrafodionToolTip _theToolTip;
        private TrafodionPanel TrafodionPanel1;
        private TrafodionTextBox basicValOrAttribute_TrafodionTextBox;
        private TrafodionLabel basicLabel_TrafodionLabel;
    }
}
