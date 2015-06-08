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
﻿namespace Trafodion.Manager.DatabaseArea.Controls
{
    partial class PartitionsUserControl
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
            this.TrafodionPanel3 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._exportButtonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._currentButtonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._fetchCurrentButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._partitionsPanel = new Trafodion.Manager.DatabaseArea.Controls.PartitionsPanel();
            this.TrafodionPanel3.SuspendLayout();
            this._currentButtonPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionPanel3
            // 
            this.TrafodionPanel3.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel3.Controls.Add(this._exportButtonsPanel);
            this.TrafodionPanel3.Controls.Add(this._currentButtonPanel);
            this.TrafodionPanel3.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionPanel3.Location = new System.Drawing.Point(0, 339);
            this.TrafodionPanel3.Name = "TrafodionPanel3";
            this.TrafodionPanel3.Size = new System.Drawing.Size(827, 31);
            this.TrafodionPanel3.TabIndex = 1;
            // 
            // _exportButtonsPanel
            // 
            this._exportButtonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._exportButtonsPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._exportButtonsPanel.Location = new System.Drawing.Point(155, 0);
            this._exportButtonsPanel.Name = "_exportButtonsPanel";
            this._exportButtonsPanel.Size = new System.Drawing.Size(672, 31);
            this._exportButtonsPanel.TabIndex = 1;
            // 
            // _currentButtonPanel
            // 
            this._currentButtonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._currentButtonPanel.Controls.Add(this._fetchCurrentButton);
            this._currentButtonPanel.Dock = System.Windows.Forms.DockStyle.Left;
            this._currentButtonPanel.Location = new System.Drawing.Point(0, 0);
            this._currentButtonPanel.Name = "_currentButtonPanel";
            this._currentButtonPanel.Size = new System.Drawing.Size(155, 31);
            this._currentButtonPanel.TabIndex = 1;
            // 
            // _fetchCurrentButton
            // 
            this._fetchCurrentButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._fetchCurrentButton.Location = new System.Drawing.Point(4, 4);
            this._fetchCurrentButton.Name = "_fetchCurrentButton";
            this._fetchCurrentButton.Size = new System.Drawing.Size(148, 23);
            this._fetchCurrentButton.TabIndex = 2;
            this._fetchCurrentButton.Text = "&Current Usage";
            this._fetchCurrentButton.UseVisualStyleBackColor = true;
            this._fetchCurrentButton.Visible = false;
            this._fetchCurrentButton.Click += new System.EventHandler(this._fetchCurrentButton_Click);
            // 
            // _partitionsPanel
            // 
            this._partitionsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._partitionsPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._partitionsPanel.Location = new System.Drawing.Point(0, 0);
            this._partitionsPanel.Name = "_partitionsPanel";
            this._partitionsPanel.PartitionedSchemaObject = null;
            this._partitionsPanel.Size = new System.Drawing.Size(827, 339);
            this._partitionsPanel.TabIndex = 0;
            this._partitionsPanel.TitleSuffix = "Partitions";
            // 
            // PartitionsUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._partitionsPanel);
            this.Controls.Add(this.TrafodionPanel3);
            this.Name = "PartitionsUserControl";
            this.Size = new System.Drawing.Size(827, 370);
            this.TrafodionPanel3.ResumeLayout(false);
            this._currentButtonPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionPanel TrafodionPanel3;
        private Framework.Controls.TrafodionPanel _exportButtonsPanel;
        private Framework.Controls.TrafodionPanel _currentButtonPanel;
        private Framework.Controls.TrafodionButton _fetchCurrentButton;
        private PartitionsPanel _partitionsPanel;
    }
}
