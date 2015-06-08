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
﻿namespace Trafodion.Manager.MetricMiner.Controls
{
    partial class QueryDesignerDialog
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
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theSaveButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theCancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theQueryDesigner = new Trafodion.Manager.MetricMiner.Controls.QueryDesigner();
            this.TrafodionPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._theSaveButton);
            this.TrafodionPanel1.Controls.Add(this._theCancelButton);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 516);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(799, 42);
            this.TrafodionPanel1.TabIndex = 0;
            // 
            // _theSaveButton
            // 
            this._theSaveButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theSaveButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theSaveButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theSaveButton.Location = new System.Drawing.Point(651, 6);
            this._theSaveButton.Name = "_theSaveButton";
            this._theSaveButton.Size = new System.Drawing.Size(72, 23);
            this._theSaveButton.TabIndex = 1;
            this._theSaveButton.Text = "OK";
            this._theSaveButton.UseVisualStyleBackColor = true;
            this._theSaveButton.Click += new System.EventHandler(this._theSaveButton_Click);
            // 
            // _theCancelButton
            // 
            this._theCancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theCancelButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theCancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCancelButton.Location = new System.Drawing.Point(729, 6);
            this._theCancelButton.Name = "_theCancelButton";
            this._theCancelButton.Size = new System.Drawing.Size(58, 23);
            this._theCancelButton.TabIndex = 0;
            this._theCancelButton.Text = "Cancel";
            this._theCancelButton.UseVisualStyleBackColor = true;
            this._theCancelButton.Click += new System.EventHandler(this._theCancelButton_Click);
            // 
            // _theQueryDesigner
            // 
            this._theQueryDesigner.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theQueryDesigner.Location = new System.Drawing.Point(0, 0);
            this._theQueryDesigner.Name = "_theQueryDesigner";
            this._theQueryDesigner.Size = new System.Drawing.Size(799, 516);
            this._theQueryDesigner.TabIndex = 1;
            this._theQueryDesigner.TheSelectedConnectionDefinition = null;
            // 
            // QueryDesignerDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(799, 558);
            this.Controls.Add(this._theQueryDesigner);
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "QueryDesignerDialog";
            this.Text = "Trafodion Database Manager - SaveConfigurationDialog";
            this.TrafodionPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theSaveButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theCancelButton;
        private QueryDesigner _theQueryDesigner;
    }
}