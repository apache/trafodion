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
    partial class RowDetailsContainer
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
            this._theHeader = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theRowDetailsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.SuspendLayout();
            // 
            // _theHeader
            // 
            this._theHeader.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(117)))), ((int)(((byte)(145)))), ((int)(((byte)(172)))));
            this._theHeader.Dock = System.Windows.Forms.DockStyle.Top;
            this._theHeader.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theHeader.ForeColor = System.Drawing.Color.White;
            this._theHeader.Location = new System.Drawing.Point(0, 0);
            this._theHeader.Name = "_theHeader";
            this._theHeader.Size = new System.Drawing.Size(280, 23);
            this._theHeader.TabIndex = 1;
            this._theHeader.Text = "Row Details";
            this._theHeader.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // _theRowDetailsPanel
            // 
            this._theRowDetailsPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theRowDetailsPanel.Location = new System.Drawing.Point(0, 23);
            this._theRowDetailsPanel.Name = "_theRowDetailsPanel";
            this._theRowDetailsPanel.Size = new System.Drawing.Size(280, 524);
            this._theRowDetailsPanel.TabIndex = 2;
            // 
            // RowDetailsContainer
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theRowDetailsPanel);
            this.Controls.Add(this._theHeader);
            this.Name = "RowDetailsContainer";
            this.Size = new System.Drawing.Size(280, 547);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionLabel _theHeader;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theRowDetailsPanel;
    }
}
