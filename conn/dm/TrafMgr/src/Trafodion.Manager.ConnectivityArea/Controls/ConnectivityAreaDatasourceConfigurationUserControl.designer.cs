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
namespace Trafodion.Manager.ConnectivityArea.Controls
{
    partial class ConnectivityAreaDatasourceConfigurationUserControl
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
            this._connectivityTopTabControl = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this._ConnectivityMainBodyPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.theTopPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.theTopPanelLowerLabel = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.theBottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.flowLayoutPanel2 = new System.Windows.Forms.FlowLayoutPanel();
            this.dsCancel_TrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.dsReload_TrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.dsApply_TrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.DSConfig_TrafodionToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._ConnectivityMainBodyPanel.SuspendLayout();
            this.theTopPanel.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this.theBottomPanel.SuspendLayout();
            this.flowLayoutPanel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // _connectivityTopTabControl
            // 
            this._connectivityTopTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._connectivityTopTabControl.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._connectivityTopTabControl.Location = new System.Drawing.Point(0, 0);
            this._connectivityTopTabControl.Multiline = true;
            this._connectivityTopTabControl.Name = "_connectivityTopTabControl";
            this._connectivityTopTabControl.Padding = new System.Drawing.Point(10, 5);
            this._connectivityTopTabControl.SelectedIndex = 0;
            this._connectivityTopTabControl.Size = new System.Drawing.Size(996, 627);
            this._connectivityTopTabControl.TabIndex = 0;
            this._connectivityTopTabControl.Visible = false;
            // 
            // _ConnectivityMainBodyPanel
            // 
            this._ConnectivityMainBodyPanel.AutoScroll = true;
            this._ConnectivityMainBodyPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._ConnectivityMainBodyPanel.Controls.Add(this._connectivityTopTabControl);
            this._ConnectivityMainBodyPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._ConnectivityMainBodyPanel.Location = new System.Drawing.Point(0, 33);
            this._ConnectivityMainBodyPanel.Name = "_ConnectivityMainBodyPanel";
            this._ConnectivityMainBodyPanel.Size = new System.Drawing.Size(996, 627);
            this._ConnectivityMainBodyPanel.TabIndex = 1;
            // 
            // theTopPanel
            // 
            this.theTopPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanel.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.theTopPanel.Controls.Add(this.theTopPanelLowerLabel);
            this.theTopPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.theTopPanel.Location = new System.Drawing.Point(0, 0);
            this.theTopPanel.Name = "theTopPanel";
            this.theTopPanel.Size = new System.Drawing.Size(996, 33);
            this.theTopPanel.TabIndex = 4;
            // 
            // theTopPanelLowerLabel
            // 
            this.theTopPanelLowerLabel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanelLowerLabel.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.theTopPanelLowerLabel.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theTopPanelLowerLabel.Location = new System.Drawing.Point(5, 7);
            this.theTopPanelLowerLabel.Name = "theTopPanelLowerLabel";
            this.theTopPanelLowerLabel.ReadOnly = true;
            this.theTopPanelLowerLabel.Size = new System.Drawing.Size(984, 15);
            this.theTopPanelLowerLabel.TabIndex = 2;
            this.theTopPanelLowerLabel.Text = "<theTopPanelLowerLabel>";
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._ConnectivityMainBodyPanel);
            this.TrafodionPanel1.Controls.Add(this.theTopPanel);
            this.TrafodionPanel1.Controls.Add(this.theBottomPanel);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(996, 693);
            this.TrafodionPanel1.TabIndex = 3;
            // 
            // theBottomPanel
            // 
            this.theBottomPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theBottomPanel.Controls.Add(this.flowLayoutPanel2);
            this.theBottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.theBottomPanel.Location = new System.Drawing.Point(0, 660);
            this.theBottomPanel.Name = "theBottomPanel";
            this.theBottomPanel.Size = new System.Drawing.Size(996, 33);
            this.theBottomPanel.TabIndex = 5;
            // 
            // flowLayoutPanel2
            // 
            this.flowLayoutPanel2.Controls.Add(this.dsCancel_TrafodionButton);
            this.flowLayoutPanel2.Controls.Add(this.dsReload_TrafodionButton);
            this.flowLayoutPanel2.Controls.Add(this.dsApply_TrafodionButton);
            this.flowLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel2.FlowDirection = System.Windows.Forms.FlowDirection.RightToLeft;
            this.flowLayoutPanel2.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel2.Name = "flowLayoutPanel2";
            this.flowLayoutPanel2.Size = new System.Drawing.Size(996, 33);
            this.flowLayoutPanel2.TabIndex = 1;
            this.flowLayoutPanel2.WrapContents = false;
            // 
            // dsCancel_TrafodionButton
            // 
            this.dsCancel_TrafodionButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.dsCancel_TrafodionButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.dsCancel_TrafodionButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.dsCancel_TrafodionButton.Location = new System.Drawing.Point(918, 3);
            this.dsCancel_TrafodionButton.Name = "dsCancel_TrafodionButton";
            this.dsCancel_TrafodionButton.Size = new System.Drawing.Size(75, 23);
            this.dsCancel_TrafodionButton.TabIndex = 6;
            this.dsCancel_TrafodionButton.Text = "Ca&ncel";
            this.dsCancel_TrafodionButton.UseVisualStyleBackColor = true;
            this.dsCancel_TrafodionButton.Click += new System.EventHandler(this.dsCancel_TrafodionButton_Click);
            // 
            // dsReload_TrafodionButton
            // 
            this.dsReload_TrafodionButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.dsReload_TrafodionButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.dsReload_TrafodionButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.dsReload_TrafodionButton.Location = new System.Drawing.Point(837, 3);
            this.dsReload_TrafodionButton.Name = "dsReload_TrafodionButton";
            this.dsReload_TrafodionButton.Size = new System.Drawing.Size(75, 23);
            this.dsReload_TrafodionButton.TabIndex = 4;
            this.dsReload_TrafodionButton.Text = "&Reload";
            this.dsReload_TrafodionButton.UseVisualStyleBackColor = true;
            this.dsReload_TrafodionButton.Click += new System.EventHandler(this.dsReload_TrafodionButton_Click);
            // 
            // dsApply_TrafodionButton
            // 
            this.dsApply_TrafodionButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.dsApply_TrafodionButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.dsApply_TrafodionButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.dsApply_TrafodionButton.Location = new System.Drawing.Point(756, 3);
            this.dsApply_TrafodionButton.Name = "dsApply_TrafodionButton";
            this.dsApply_TrafodionButton.Size = new System.Drawing.Size(75, 23);
            this.dsApply_TrafodionButton.TabIndex = 5;
            this.dsApply_TrafodionButton.Text = "&Apply";
            this.dsApply_TrafodionButton.UseVisualStyleBackColor = true;
            // 
            // ConnectivityAreaDatasourceConfigurationUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "ConnectivityAreaDatasourceConfigurationUserControl";
            this.Size = new System.Drawing.Size(996, 693);
            this._ConnectivityMainBodyPanel.ResumeLayout(false);
            this.theTopPanel.ResumeLayout(false);
            this.theTopPanel.PerformLayout();
            this.TrafodionPanel1.ResumeLayout(false);
            this.theBottomPanel.ResumeLayout(false);
            this.flowLayoutPanel2.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionTabControl _connectivityTopTabControl;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _ConnectivityMainBodyPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox theTopPanelLowerLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel theTopPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel theBottomPanel;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel2;
        private Trafodion.Manager.Framework.Controls.TrafodionButton dsReload_TrafodionButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton dsApply_TrafodionButton;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip DSConfig_TrafodionToolTip;
        private Trafodion.Manager.Framework.Controls.TrafodionButton dsCancel_TrafodionButton;
    }
}
