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
using System.Windows.Forms;
namespace Trafodion.Manager.Framework.Connections.Controls
{
    partial class ConnectionsTool
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
            Trafodion.Manager.Framework.Controls.TrafodionPanel panel1;
            this._theDoneButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.dataGridViewTextBoxColumn1 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataGridViewTextBoxColumn2 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataGridViewTextBoxColumn3 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this._theMySystemsUserControl = new Trafodion.Manager.Framework.Connections.Controls.MySystemsUserControl();
            panel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            panel1.Controls.Add(this._theDoneButton);
            panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            panel1.Location = new System.Drawing.Point(4, 373);
            panel1.Name = "panel1";
            panel1.Size = new System.Drawing.Size(736, 32);
            panel1.TabIndex = 0;
            // 
            // _theDoneButton
            // 
            this._theDoneButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theDoneButton.Location = new System.Drawing.Point(658, 6);
            this._theDoneButton.Name = "_theDoneButton";
            this._theDoneButton.Size = new System.Drawing.Size(75, 23);
            this._theDoneButton.TabIndex = 0;
            this._theDoneButton.Text = "D&one";
            this._theDoneButton.UseVisualStyleBackColor = true;
            this._theDoneButton.Click += new System.EventHandler(this.TheDoneButtonClick);
            // 
            // dataGridViewTextBoxColumn1
            // 
            this.dataGridViewTextBoxColumn1.HeaderText = "Column1";
            this.dataGridViewTextBoxColumn1.Name = "dataGridViewTextBoxColumn1";
            // 
            // dataGridViewTextBoxColumn2
            // 
            this.dataGridViewTextBoxColumn2.HeaderText = "Column2";
            this.dataGridViewTextBoxColumn2.Name = "dataGridViewTextBoxColumn2";
            this.dataGridViewTextBoxColumn2.ReadOnly = true;
            // 
            // dataGridViewTextBoxColumn3
            // 
            this.dataGridViewTextBoxColumn3.HeaderText = "Column3";
            this.dataGridViewTextBoxColumn3.Name = "dataGridViewTextBoxColumn3";
            // 
            // _theMySystemsUserControl
            // 
            this._theMySystemsUserControl.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theMySystemsUserControl.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this._theMySystemsUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMySystemsUserControl.Location = new System.Drawing.Point(4, 4);
            this._theMySystemsUserControl.Name = "_theMySystemsUserControl";
            this._theMySystemsUserControl.Size = new System.Drawing.Size(736, 369);
            this._theMySystemsUserControl.TabIndex = 1;
            // 
            // ConnectionsTool
            // 
            this.AcceptButton = this._theDoneButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(744, 409);
            this.Controls.Add(this._theMySystemsUserControl);
            this.Controls.Add(panel1);
            this.Name = "ConnectionsTool";
            this.Padding = new System.Windows.Forms.Padding(4);
            this.Text = "Trafodion Database Manager - Systems Tool";
            panel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private DataGridViewTextBoxColumn dataGridViewTextBoxColumn1;
        private DataGridViewTextBoxColumn dataGridViewTextBoxColumn2;
        private DataGridViewTextBoxColumn dataGridViewTextBoxColumn3;
        private TrafodionButton _theDoneButton;
        private Trafodion.Manager.Framework.Connections.Controls.MySystemsUserControl _theMySystemsUserControl;

    }
}