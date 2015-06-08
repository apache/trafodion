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

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    /// <summary>
    /// CQSettingDialog
    /// </summary>
    partial class CQSettingDialog
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(CQSettingDialog));
            this._helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._applyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.oneGuiToolTip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.oneGuiPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.ncccqSettingControl1 = new Trafodion.Manager.DatabaseArea.NCC.NCCCQSettingControl();
            this.oneGuiPanel1.SuspendLayout();
            this.oneGuiPanel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // _helpButton
            // 
            this._helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._helpButton.Location = new System.Drawing.Point(5, 5);
            this._helpButton.Name = "_helpButton";
            this._helpButton.Size = new System.Drawing.Size(75, 23);
            this._helpButton.TabIndex = 2;
            this._helpButton.Text = "Help";
            this._helpButton.UseVisualStyleBackColor = true;
            this._helpButton.Click += new System.EventHandler(this._helpButton_Click);
            // 
            // _applyButton
            // 
            this._applyButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._applyButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this._applyButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._applyButton.Location = new System.Drawing.Point(387, 5);
            this._applyButton.Name = "_applyButton";
            this._applyButton.Size = new System.Drawing.Size(75, 23);
            this._applyButton.TabIndex = 0;
            this._applyButton.Text = "A&pply";
            this._applyButton.UseVisualStyleBackColor = true;
            this._applyButton.Click += new System.EventHandler(this._applyButton_Click);
            // 
            // _cancelButton
            // 
            this._cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._cancelButton.Location = new System.Drawing.Point(470, 5);
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Size = new System.Drawing.Size(75, 23);
            this._cancelButton.TabIndex = 1;
            this._cancelButton.Text = "&Cancel";
            this._cancelButton.UseVisualStyleBackColor = true;
            this._cancelButton.Click += new System.EventHandler(this._cancelButton_Click);
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this.ncccqSettingControl1);
            this.oneGuiPanel1.Controls.Add(this.oneGuiPanel2);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(550, 277);
            this.oneGuiPanel1.TabIndex = 2;
            // 
            // oneGuiToolTip1
            // 
            this.oneGuiToolTip1.IsBalloon = true;
            // 
            // oneGuiPanel2
            // 
            this.oneGuiPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel2.Controls.Add(this._cancelButton);
            this.oneGuiPanel2.Controls.Add(this._helpButton);
            this.oneGuiPanel2.Controls.Add(this._applyButton);
            this.oneGuiPanel2.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.oneGuiPanel2.Location = new System.Drawing.Point(0, 245);
            this.oneGuiPanel2.Name = "oneGuiPanel2";
            this.oneGuiPanel2.Size = new System.Drawing.Size(550, 32);
            this.oneGuiPanel2.TabIndex = 3;
            // 
            // ncccqSettingControl1
            // 
            this.ncccqSettingControl1.ControlStatementHolder = null;
            this.ncccqSettingControl1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.ncccqSettingControl1.Location = new System.Drawing.Point(0, 0);
            this.ncccqSettingControl1.Name = "ncccqSettingControl1";
            this.ncccqSettingControl1.OnClickHandler = null;
            this.ncccqSettingControl1.Size = new System.Drawing.Size(550, 245);
            this.ncccqSettingControl1.TabIndex = 4;
            // 
            // CQSettingDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(550, 277);
            this.Controls.Add(this.oneGuiPanel1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "CQSettingDialog";
            this.Text = "HP Database Manager - Statement Control Settings";
            this.oneGuiPanel1.ResumeLayout(false);
            this.oneGuiPanel2.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        //private System.Windows.Forms.FlowLayoutPanel buttonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _applyButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip oneGuiToolTip1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _helpButton;
        private Trafodion.Manager.DatabaseArea.NCC.NCCCQSettingControl ncccqSettingControl1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel2;
    }
}
