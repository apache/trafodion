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
    partial class SecurityPoliciesTabControl
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
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theTabControl = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this._theButtonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._loadMostSecureButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._loadDefaultButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._applyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._refreshButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.oneGuiPanel1.SuspendLayout();
            this._theButtonPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this._theTabControl);
            this.oneGuiPanel1.Controls.Add(this._theButtonPanel);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(727, 505);
            this.oneGuiPanel1.TabIndex = 0;
            // 
            // _theTabControl
            // 
            this._theTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTabControl.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theTabControl.HotTrack = true;
            this._theTabControl.Location = new System.Drawing.Point(0, 0);
            this._theTabControl.Multiline = true;
            this._theTabControl.Name = "_theTabControl";
            this._theTabControl.Padding = new System.Drawing.Point(10, 5);
            this._theTabControl.SelectedIndex = 0;
            this._theTabControl.Size = new System.Drawing.Size(727, 469);
            this._theTabControl.TabIndex = 0;
            this._theTabControl.TabStop = false;
            // 
            // _theButtonPanel
            // 
            this._theButtonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theButtonPanel.Controls.Add(this._loadMostSecureButton);
            this._theButtonPanel.Controls.Add(this._loadDefaultButton);
            this._theButtonPanel.Controls.Add(this._applyButton);
            this._theButtonPanel.Controls.Add(this._refreshButton);
            this._theButtonPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theButtonPanel.Location = new System.Drawing.Point(0, 469);
            this._theButtonPanel.Name = "_theButtonPanel";
            this._theButtonPanel.Size = new System.Drawing.Size(727, 36);
            this._theButtonPanel.TabIndex = 1;
            // 
            // _loadMostSecureButton
            // 
            this._loadMostSecureButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._loadMostSecureButton.Location = new System.Drawing.Point(220, 7);
            this._loadMostSecureButton.Name = "_loadMostSecureButton";
            this._loadMostSecureButton.Size = new System.Drawing.Size(104, 23);
            this._loadMostSecureButton.TabIndex = 3;
            this._loadMostSecureButton.Text = "Load Most &Secure";
            this.toolTip1.SetToolTip(this._loadMostSecureButton, "Load most secure policy settings");
            this._loadMostSecureButton.UseVisualStyleBackColor = true;
            this._loadMostSecureButton.Click += new System.EventHandler(this._loadMostSecureButton_Click);
            // 
            // _loadDefaultButton
            // 
            this._loadDefaultButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._loadDefaultButton.Location = new System.Drawing.Point(116, 7);
            this._loadDefaultButton.Name = "_loadDefaultButton";
            this._loadDefaultButton.Size = new System.Drawing.Size(98, 23);
            this._loadDefaultButton.TabIndex = 2;
            this._loadDefaultButton.Text = "Load &Default";
            this.toolTip1.SetToolTip(this._loadDefaultButton, "Load factory default settings");
            this._loadDefaultButton.UseVisualStyleBackColor = true;
            this._loadDefaultButton.Click += new System.EventHandler(this._loadDefaultButton_Click);
            // 
            // _applyButton
            // 
            this._applyButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._applyButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._applyButton.Location = new System.Drawing.Point(617, 7);
            this._applyButton.Name = "_applyButton";
            this._applyButton.Size = new System.Drawing.Size(98, 23);
            this._applyButton.TabIndex = 4;
            this._applyButton.Text = "&Apply";
            this.toolTip1.SetToolTip(this._applyButton, "Apply policy changes");
            this._applyButton.UseVisualStyleBackColor = true;
            this._applyButton.Click += new System.EventHandler(this._applyButton_Click);
            // 
            // _refreshButton
            // 
            this._refreshButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._refreshButton.Location = new System.Drawing.Point(12, 7);
            this._refreshButton.Name = "_refreshButton";
            this._refreshButton.Size = new System.Drawing.Size(98, 23);
            this._refreshButton.TabIndex = 1;
            this._refreshButton.Text = "&Refresh";
            this.toolTip1.SetToolTip(this._refreshButton, "Reload the policies with the current policy settings");
            this._refreshButton.UseVisualStyleBackColor = true;
            this._refreshButton.Click += new System.EventHandler(this._refreshButton_Click);
            // 
            // SecurityPoliciesTabControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.oneGuiPanel1);
            this.Name = "SecurityPoliciesTabControl";
            this.Size = new System.Drawing.Size(727, 505);
            this.oneGuiPanel1.ResumeLayout(false);
            this._theButtonPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theButtonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _loadDefaultButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _applyButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _refreshButton;
        private Trafodion.Manager.Framework.Controls.TrafodionTabControl _theTabControl;
        private System.Windows.Forms.ToolTip toolTip1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _loadMostSecureButton;
    }
}
