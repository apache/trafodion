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

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    partial class ReportDefinitionChangedByUserDialog
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
            TrafodionPanel panel1;
            TrafodionLabel label1;
            this._theDiscardChangesButton = new TrafodionButton();
            this._theUpdateExistingButton = new TrafodionButton();
            this._theAddNewButton = new TrafodionButton();
            panel1 = new TrafodionPanel();
            label1 = new TrafodionLabel();
            panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            panel1.Controls.Add(this._theDiscardChangesButton);
            panel1.Controls.Add(this._theUpdateExistingButton);
            panel1.Controls.Add(this._theAddNewButton);
            panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            panel1.Location = new System.Drawing.Point(0, 67);
            panel1.Name = "panel1";
            panel1.Size = new System.Drawing.Size(541, 29);
            panel1.TabIndex = 0;
            // 
            // _theDiscardChangesButton
            // 
            this._theDiscardChangesButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            //this._theDiscardChangesButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this._theDiscardChangesButton.Location = new System.Drawing.Point(425, 3);
            this._theDiscardChangesButton.Name = "_theDiscardChangesButton";
            this._theDiscardChangesButton.Size = new System.Drawing.Size(112, 23);
            this._theDiscardChangesButton.TabIndex = 2;
            this._theDiscardChangesButton.Text = "&Discard Changes";
            this._theDiscardChangesButton.Click += new System.EventHandler(this.TheDiscardChangesButtonClick);
            // 
            // _theUpdateExistingButton
            // 
            this._theUpdateExistingButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            //this._theUpdateExistingButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this._theUpdateExistingButton.Location = new System.Drawing.Point(186, 3);
            this._theUpdateExistingButton.Name = "_theUpdateExistingButton";
            this._theUpdateExistingButton.Size = new System.Drawing.Size(172, 23);
            this._theUpdateExistingButton.TabIndex = 1;
            this._theUpdateExistingButton.Text = "&Update Existing Statement";
            this._theUpdateExistingButton.Click += new System.EventHandler(this.TheUpdateExistingButtonClick);
            // 
            // _theAddNewButton
            // 
            this._theAddNewButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            //this._theAddNewButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this._theAddNewButton.Location = new System.Drawing.Point(6, 3);
            this._theAddNewButton.Name = "_theAddNewButton";
            this._theAddNewButton.Size = new System.Drawing.Size(172, 23);
            this._theAddNewButton.TabIndex = 0;
            this._theAddNewButton.Text = "&Add as New Statement";
            this._theAddNewButton.Click += new System.EventHandler(this.TheAddNewButtonClick);
            // 
            // label1
            // 
            label1.Dock = System.Windows.Forms.DockStyle.Fill;
            label1.Location = new System.Drawing.Point(0, 0);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(541, 67);
            label1.TabIndex = 1;
            label1.Text = "You have changed the current statement and/or its name.";
            label1.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // ReportDefinitionChangedByUserDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(541, 96);
            this.Controls.Add(label1);
            this.Controls.Add(panel1);
            this.Name = "ReportDefinitionChangedByUserDialog";
            this.Text = "Trafodion Database Manager - Statement and/or Name Changed";
            panel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionButton _theDiscardChangesButton;
        private TrafodionButton _theUpdateExistingButton;
        private TrafodionButton _theAddNewButton;
    }
}