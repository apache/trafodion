// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
//
namespace Trafodion.Manager.SecurityArea.Controls
{
    partial class MultilineTextUserControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MultilineTextUserControl));
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theTextPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theImportPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theImportButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.toolTip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.oneGuiPanel1.SuspendLayout();
            this._theTextPanel.SuspendLayout();
            this._theImportPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this._theTextPanel);
            this.oneGuiPanel1.Controls.Add(this._theImportPanel);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(602, 130);
            this.oneGuiPanel1.TabIndex = 0;
            // 
            // _theTextPanel
            // 
            this._theTextPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theTextPanel.Controls.Add(this._theTextBox);
            this._theTextPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTextPanel.Location = new System.Drawing.Point(0, 0);
            this._theTextPanel.Name = "_theTextPanel";
            this._theTextPanel.Size = new System.Drawing.Size(527, 130);
            this._theTextPanel.TabIndex = 0;
            // 
            // _theTextBox
            // 
            this._theTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theTextBox.Location = new System.Drawing.Point(0, 0);
            this._theTextBox.Multiline = true;
            this._theTextBox.Name = "_theTextBox";
            this._theTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this._theTextBox.Size = new System.Drawing.Size(527, 130);
            this._theTextBox.TabIndex = 0;
            // 
            // _theImportPanel
            // 
            this._theImportPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theImportPanel.Controls.Add(this._theImportButton);
            this._theImportPanel.Dock = System.Windows.Forms.DockStyle.Right;
            this._theImportPanel.Location = new System.Drawing.Point(527, 0);
            this._theImportPanel.Name = "_theImportPanel";
            this._theImportPanel.Size = new System.Drawing.Size(75, 130);
            this._theImportPanel.TabIndex = 1;
            // 
            // _theImportButton
            // 
            this._theImportButton.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this._theImportButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theImportButton.Image = ((System.Drawing.Image)(resources.GetObject("_theImportButton.Image")));
            this._theImportButton.Location = new System.Drawing.Point(21, 45);
            this._theImportButton.Name = "_theImportButton";
            this._theImportButton.Size = new System.Drawing.Size(33, 33);
            this._theImportButton.TabIndex = 1;
            this._theImportButton.Text = "...";
            this.toolTip1.SetToolTip(this._theImportButton, "Import from a file");
            this._theImportButton.UseVisualStyleBackColor = true;
            // 
            // MultilineTextUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.oneGuiPanel1);
            this.Name = "MultilineTextUserControl";
            this.Size = new System.Drawing.Size(602, 130);
            this.oneGuiPanel1.ResumeLayout(false);
            this._theTextPanel.ResumeLayout(false);
            this._theTextPanel.PerformLayout();
            this._theImportPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theTextPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theImportPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theImportButton;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip toolTip1;
    }
}
