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
using Trafodion.Manager.Framework.Controls;
namespace Trafodion.Manager.Framework.Connections.Controls
{
    partial class ConnectionDefinitionDialog
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
            this.panel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theAddOnlyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theCancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theOKButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theConnectionDefinitionUserControl = new Trafodion.Manager.Framework.Connections.Controls.ConnectionDefinitionUserControl();
            this._theToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.AutoSize = true;
            this.panel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel1.Controls.Add(this._theAddOnlyButton);
            this.panel1.Controls.Add(this._theCancelButton);
            this.panel1.Controls.Add(this._theOKButton);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel1.Location = new System.Drawing.Point(0, 443);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(614, 28);
            this.panel1.TabIndex = 50;
            // 
            // _theAddOnlyButton
            // 
            this._theAddOnlyButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theAddOnlyButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theAddOnlyButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAddOnlyButton.Location = new System.Drawing.Point(13, 2);
            this._theAddOnlyButton.Name = "_theAddOnlyButton";
            this._theAddOnlyButton.Size = new System.Drawing.Size(94, 23);
            this._theAddOnlyButton.TabIndex = 52;
            this._theAddOnlyButton.Text = "&Add";
            this._theAddOnlyButton.UseVisualStyleBackColor = true;
            this._theAddOnlyButton.Click += new System.EventHandler(this._theAddOnlyButton_Click);
            // 
            // _theCancelButton
            // 
            this._theCancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theCancelButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theCancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCancelButton.Location = new System.Drawing.Point(513, 2);
            this._theCancelButton.Name = "_theCancelButton";
            this._theCancelButton.Size = new System.Drawing.Size(89, 23);
            this._theCancelButton.TabIndex = 55;
            this._theCancelButton.Text = "&Cancel";
            this._theCancelButton.UseVisualStyleBackColor = true;
            this._theCancelButton.Click += new System.EventHandler(this.TheCancelButtonClick);
            // 
            // _theOKButton
            // 
            this._theOKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theOKButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theOKButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theOKButton.Location = new System.Drawing.Point(413, 2);
            this._theOKButton.Name = "_theOKButton";
            this._theOKButton.Size = new System.Drawing.Size(94, 23);
            this._theOKButton.TabIndex = 51;
            this._theOKButton.Text = "C&onnect";
            this._theOKButton.UseVisualStyleBackColor = true;
            this._theOKButton.Click += new System.EventHandler(this.TheOKButtonClick);
            // 
            // _theConnectionDefinitionUserControl
            // 
            this._theConnectionDefinitionUserControl.AutoSize = true;
            this._theConnectionDefinitionUserControl.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theConnectionDefinitionUserControl.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this._theConnectionDefinitionUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theConnectionDefinitionUserControl.Location = new System.Drawing.Point(0, 0);
            this._theConnectionDefinitionUserControl.Name = "_theConnectionDefinitionUserControl";
            this._theConnectionDefinitionUserControl.Size = new System.Drawing.Size(614, 443);
            this._theConnectionDefinitionUserControl.TabIndex = 0;
            // 
            // _theToolTip
            // 
            this._theToolTip.IsBalloon = true;
            // 
            // ConnectionDefinitionDialog
            // 
            this.AcceptButton = this._theOKButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(614, 471);
            this.Controls.Add(this._theConnectionDefinitionUserControl);
            this.Controls.Add(this.panel1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Name = "ConnectionDefinitionDialog";
            this.Text = "Trafodion Database Manager (Trafodion) - Trafodion Database Manager - ConnectionDefinitionDialo" +
    "g";
            this.panel1.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private ConnectionDefinitionUserControl _theConnectionDefinitionUserControl;
        private TrafodionPanel panel1;
        private TrafodionButton _theCancelButton;
        private TrafodionButton _theOKButton;
        private TrafodionToolTip _theToolTip;
        private TrafodionButton _theAddOnlyButton;
    }
}