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
﻿namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    partial class QueryDiscardOptionDialog
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
            this._headerLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._discardResultsRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._discardBothRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._okButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.TrafodionGroupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // _headerLabel
            // 
            this._headerLabel.AutoSize = true;
            this._headerLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._headerLabel.Location = new System.Drawing.Point(3, 5);
            this._headerLabel.Name = "_headerLabel";
            this._headerLabel.Size = new System.Drawing.Size(335, 14);
            this._headerLabel.TabIndex = 0;
            this._headerLabel.Text = "Choose a Discard option for the selected statements, then click OK: ";
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.Controls.Add(this._discardResultsRadioButton);
            this.TrafodionGroupBox1.Controls.Add(this._discardBothRadioButton);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(3, 22);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(366, 86);
            this.TrafodionGroupBox1.TabIndex = 1;
            this.TrafodionGroupBox1.TabStop = false;
            this.TrafodionGroupBox1.Text = "Discard Option";
            // 
            // _discardResultsRadioButton
            // 
            this._discardResultsRadioButton.AutoSize = true;
            this._discardResultsRadioButton.Checked = true;
            this._discardResultsRadioButton.Font = new System.Drawing.Font("Tahoma", 8F);
            this._discardResultsRadioButton.Location = new System.Drawing.Point(9, 50);
            this._discardResultsRadioButton.Name = "_discardResultsRadioButton";
            this._discardResultsRadioButton.Size = new System.Drawing.Size(86, 18);
            this._discardResultsRadioButton.TabIndex = 0;
            this._discardResultsRadioButton.TabStop = true;
            this._discardResultsRadioButton.Text = "Results Only";
            this._discardResultsRadioButton.UseVisualStyleBackColor = true;
            // 
            // _discardBothRadioButton
            // 
            this._discardBothRadioButton.AutoSize = true;
            this._discardBothRadioButton.Font = new System.Drawing.Font("Tahoma", 8F);
            this._discardBothRadioButton.Location = new System.Drawing.Point(9, 19);
            this._discardBothRadioButton.Name = "_discardBothRadioButton";
            this._discardBothRadioButton.Size = new System.Drawing.Size(139, 18);
            this._discardBothRadioButton.TabIndex = 0;
            this._discardBothRadioButton.Text = "Statements and Results";
            this._discardBothRadioButton.UseVisualStyleBackColor = true;
            // 
            // _okButton
            // 
            this._okButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this._okButton.Location = new System.Drawing.Point(86, 113);
            this._okButton.Name = "_okButton";
            this._okButton.Size = new System.Drawing.Size(75, 23);
            this._okButton.TabIndex = 2;
            this._okButton.Text = "&OK";
            this._okButton.UseVisualStyleBackColor = true;
            // 
            // _cancelButton
            // 
            this._cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._cancelButton.Location = new System.Drawing.Point(181, 113);
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Size = new System.Drawing.Size(75, 23);
            this._cancelButton.TabIndex = 2;
            this._cancelButton.Text = "&Cancel";
            this._cancelButton.UseVisualStyleBackColor = true;
            // 
            // QueryDiscardOptionDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(372, 139);
            this.Controls.Add(this._cancelButton);
            this.Controls.Add(this._okButton);
            this.Controls.Add(this.TrafodionGroupBox1);
            this.Controls.Add(this._headerLabel);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Name = "QueryDiscardOptionDialog";
            this.Text = "Trafodion Database Manager - Discard Statement";
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.TrafodionGroupBox1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionLabel _headerLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox TrafodionGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton _discardResultsRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton _discardBothRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _okButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _cancelButton;
    }
}