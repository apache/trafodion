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
﻿namespace Trafodion.Manager.WorkloadArea.Controls
{
    partial class RulesSummaryUserControl
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
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theAdditionalButtonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _bottomPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theWidgetPanel;

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this._theAdditionalButtonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._alterButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._deleteButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._associateButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._addButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._bottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theWidgetPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._exportButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theAdditionalButtonPanel.SuspendLayout();
            this._bottomPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theAdditionalButtonPanel
            // 
            this._theAdditionalButtonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAdditionalButtonPanel.Controls.Add(this._alterButton);
            this._theAdditionalButtonPanel.Controls.Add(this._deleteButton);
            this._theAdditionalButtonPanel.Controls.Add(this._exportButton);
            this._theAdditionalButtonPanel.Controls.Add(this._associateButton);
            this._theAdditionalButtonPanel.Controls.Add(this._addButton);
            this._theAdditionalButtonPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAdditionalButtonPanel.Location = new System.Drawing.Point(0, 0);
            this._theAdditionalButtonPanel.Name = "_theAdditionalButtonPanel";
            this._theAdditionalButtonPanel.Size = new System.Drawing.Size(918, 33);
            this._theAdditionalButtonPanel.TabIndex = 1;
            // 
            // _alterButton
            // 
            this._alterButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._alterButton.Location = new System.Drawing.Point(86, 5);
            this._alterButton.Name = "_alterButton";
            this._alterButton.Size = new System.Drawing.Size(75, 23);
            this._alterButton.TabIndex = 0;
            this._alterButton.Text = "A&lter";
            this._alterButton.UseVisualStyleBackColor = true;
            this._alterButton.Click += new System.EventHandler(this._alterButton_Click);
            // 
            // _deleteButton
            // 
            this._deleteButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._deleteButton.Location = new System.Drawing.Point(167, 5);
            this._deleteButton.Name = "_deleteButton";
            this._deleteButton.Size = new System.Drawing.Size(75, 23);
            this._deleteButton.TabIndex = 0;
            this._deleteButton.Text = "&Delete";
            this._deleteButton.UseVisualStyleBackColor = true;
            this._deleteButton.Click += new System.EventHandler(this._deleteButton_Click);
            // 
            // _associateButton
            // 
            this._associateButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._associateButton.Location = new System.Drawing.Point(248, 5);
            this._associateButton.Name = "_associateButton";
            this._associateButton.Size = new System.Drawing.Size(75, 23);
            this._associateButton.TabIndex = 0;
            this._associateButton.Text = "A&ssociate";
            this._associateButton.UseVisualStyleBackColor = true;
            this._associateButton.Click += new System.EventHandler(this._associateButton_Click);
            // 
            // _addButton
            // 
            this._addButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._addButton.Location = new System.Drawing.Point(5, 5);
            this._addButton.Name = "_addButton";
            this._addButton.Size = new System.Drawing.Size(75, 23);
            this._addButton.TabIndex = 0;
            this._addButton.Text = "&Add";
            this._addButton.UseVisualStyleBackColor = true;
            this._addButton.Click += new System.EventHandler(this._addButton_Click);

            //
            //Context Menu Items
            //
            this.addRuleMenuItem = new Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem();
            this.alterRuleMenuItem = new Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem();
            this.deleteRuleMenuItem = new Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem();
            this.associateRuleMenuItem = new Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem();            

            // 
            // _bottomPanel
            // 
            this._bottomPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._bottomPanel.Controls.Add(this._theAdditionalButtonPanel);
            this._bottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._bottomPanel.Location = new System.Drawing.Point(0, 424);
            this._bottomPanel.Name = "_bottomPanel";
            this._bottomPanel.Size = new System.Drawing.Size(918, 33);
            this._bottomPanel.TabIndex = 2;
            // 
            // _theWidgetPanel
            // 
            this._theWidgetPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theWidgetPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theWidgetPanel.Location = new System.Drawing.Point(0, 0);
            this._theWidgetPanel.Name = "_theWidgetPanel";
            this._theWidgetPanel.Size = new System.Drawing.Size(918, 424);
            this._theWidgetPanel.TabIndex = 3;
            // 
            // _exportButton
            // 
            this._exportButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._exportButton.Location = new System.Drawing.Point(329, 5);
            this._exportButton.Name = "_exportButton";
            this._exportButton.Size = new System.Drawing.Size(75, 23);
            this._exportButton.TabIndex = 0;
            this._exportButton.Text = "&Export";
            this._exportButton.UseVisualStyleBackColor = true;
            this._exportButton.Click += new System.EventHandler(this._exportButton_Click);
            // 
            // RulesSummaryUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._theWidgetPanel);
            this.Controls.Add(this._bottomPanel);
            this.Name = "RulesSummaryUserControl";
            this.Size = new System.Drawing.Size(918, 457);
            this._theAdditionalButtonPanel.ResumeLayout(false);
            this._bottomPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionButton _addButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _alterButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _associateButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _deleteButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _exportButton;

        private Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem addRuleMenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem alterRuleMenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem deleteRuleMenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem associateRuleMenuItem;       

    }
}
