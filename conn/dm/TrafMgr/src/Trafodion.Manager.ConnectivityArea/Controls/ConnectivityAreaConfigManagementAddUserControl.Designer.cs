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
    partial class ConnectivityAreaConfigManagementAddUserControl
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
            Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel3;
            this._theToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.mgmtAttribute_TrafodionComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.mgmtAction_TrafodionComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.mgmtLimit_TrafodionNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            TrafodionLabel3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionPanel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.mgmtLimit_TrafodionNumericUpDown)).BeginInit();
            this.SuspendLayout();
            // 
            // TrafodionLabel1
            // 
            TrafodionLabel1.AutoSize = true;
            TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8F);
            TrafodionLabel1.Location = new System.Drawing.Point(65, 65);
            TrafodionLabel1.Name = "TrafodionLabel1";
            TrafodionLabel1.Size = new System.Drawing.Size(41, 14);
            TrafodionLabel1.TabIndex = 17;
            TrafodionLabel1.Text = "Action:";
            // 
            // TrafodionLabel2
            // 
            TrafodionLabel2.AutoSize = true;
            TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8F);
            TrafodionLabel2.Location = new System.Drawing.Point(65, 39);
            TrafodionLabel2.Name = "TrafodionLabel2";
            TrafodionLabel2.Size = new System.Drawing.Size(57, 14);
            TrafodionLabel2.TabIndex = 19;
            TrafodionLabel2.Text = "Attributes:";
            // 
            // TrafodionLabel3
            // 
            TrafodionLabel3.AutoSize = true;
            TrafodionLabel3.Font = new System.Drawing.Font("Tahoma", 8F);
            TrafodionLabel3.Location = new System.Drawing.Point(65, 87);
            TrafodionLabel3.Name = "TrafodionLabel3";
            TrafodionLabel3.Size = new System.Drawing.Size(31, 14);
            TrafodionLabel3.TabIndex = 20;
            TrafodionLabel3.Text = "Limit:";
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.Controls.Add(this.mgmtAttribute_TrafodionComboBox);
            this.TrafodionPanel1.Controls.Add(this.mgmtAction_TrafodionComboBox);
            this.TrafodionPanel1.Controls.Add(this.mgmtLimit_TrafodionNumericUpDown);
            this.TrafodionPanel1.Controls.Add(TrafodionLabel3);
            this.TrafodionPanel1.Controls.Add(TrafodionLabel2);
            this.TrafodionPanel1.Controls.Add(TrafodionLabel1);
            this.TrafodionPanel1.Location = new System.Drawing.Point(16, 29);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(316, 142);
            this.TrafodionPanel1.TabIndex = 16;
            // 
            // mgmtAttribute_TrafodionComboBox
            // 
            this.mgmtAttribute_TrafodionComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.mgmtAttribute_TrafodionComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.mgmtAttribute_TrafodionComboBox.FormattingEnabled = true;
            this.mgmtAttribute_TrafodionComboBox.Items.AddRange(new object[] {
            "ESTIMATED COST"});
            this.mgmtAttribute_TrafodionComboBox.Location = new System.Drawing.Point(138, 31);
            this.mgmtAttribute_TrafodionComboBox.Name = "mgmtAttribute_TrafodionComboBox";
            this.mgmtAttribute_TrafodionComboBox.Size = new System.Drawing.Size(121, 22);
            this.mgmtAttribute_TrafodionComboBox.TabIndex = 23;
            // 
            // mgmtAction_TrafodionComboBox
            // 
            this.mgmtAction_TrafodionComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.mgmtAction_TrafodionComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.mgmtAction_TrafodionComboBox.FormattingEnabled = true;
            this.mgmtAction_TrafodionComboBox.Location = new System.Drawing.Point(138, 57);
            this.mgmtAction_TrafodionComboBox.Name = "mgmtAction_TrafodionComboBox";
            this.mgmtAction_TrafodionComboBox.Size = new System.Drawing.Size(121, 22);
            this.mgmtAction_TrafodionComboBox.TabIndex = 22;
            // 
            // mgmtLimit_TrafodionNumericUpDown
            // 
            this.mgmtLimit_TrafodionNumericUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this.mgmtLimit_TrafodionNumericUpDown.Location = new System.Drawing.Point(138, 83);
            this.mgmtLimit_TrafodionNumericUpDown.Maximum = new decimal(new int[] {
            2147483647,
            0,
            0,
            0});
            this.mgmtLimit_TrafodionNumericUpDown.Name = "mgmtLimit_TrafodionNumericUpDown";
            this.mgmtLimit_TrafodionNumericUpDown.Size = new System.Drawing.Size(121, 20);
            this.mgmtLimit_TrafodionNumericUpDown.TabIndex = 21;
            // 
            // ConnectivityAreaConfigManagementAddUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "ConnectivityAreaConfigManagementAddUserControl";
            this.Size = new System.Drawing.Size(340, 190);
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.mgmtLimit_TrafodionNumericUpDown)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionToolTip _theToolTip;
        private TrafodionPanel TrafodionPanel1;
        private TrafodionComboBox mgmtAttribute_TrafodionComboBox;
        private TrafodionComboBox mgmtAction_TrafodionComboBox;
        private TrafodionNumericUpDown mgmtLimit_TrafodionNumericUpDown;
    }

    public class ActionItem
    {
        string _theActionString;
        int _theActionValue;

        public int ActionValue
        {
            get { return _theActionValue; }
            set { _theActionValue = value; }
        }

        public ActionItem(string aActionString, int aActionValue)
        {
            this._theActionString = aActionString;
            this._theActionValue = aActionValue;
        }

        public string ActionString
        {
            get { return _theActionString; }
            set { _theActionString = value; }
        }

        public override string ToString()
        {
            return _theActionString;
        }

        public override bool Equals(object obj)
        {
            ActionItem item = obj as ActionItem;
            if (item != null)
            {
                return (item.ActionValue == ActionValue);
            }
            return false;
        }

    }
}
