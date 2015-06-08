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
namespace Trafodion.Manager.Framework.Connections.Controls
{
    partial class FixSystemUserControl
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
            MyDispose();
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            Trafodion.Manager.Framework.Controls.TrafodionPanel panel1;
            this.theEditButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theOKButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theTestButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theConnectionDefinitionUserControl = new Trafodion.Manager.Framework.Connections.Controls.ConnectionDefinitionUserControl();
            panel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            panel1.BackColor = System.Drawing.Color.WhiteSmoke;
            panel1.Controls.Add(this.theEditButton);
            panel1.Controls.Add(this._theOKButton);
            panel1.Controls.Add(this._theTestButton);
            panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            panel1.Location = new System.Drawing.Point(0, 419);
            panel1.Name = "panel1";
            panel1.Size = new System.Drawing.Size(701, 30);
            panel1.TabIndex = 0;
            // 
            // theEditButton
            // 
            this.theEditButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.theEditButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.theEditButton.Location = new System.Drawing.Point(79, 4);
            this.theEditButton.Name = "theEditButton";
            this.theEditButton.Size = new System.Drawing.Size(75, 23);
            this.theEditButton.TabIndex = 1;
            this.theEditButton.Text = "&Save";
            this.theEditButton.UseVisualStyleBackColor = true;
            this.theEditButton.Click += new System.EventHandler(this.theEditButton_Click);
            // 
            // _theOKButton
            // 
            this._theOKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theOKButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theOKButton.Location = new System.Drawing.Point(614, 4);
            this._theOKButton.Name = "_theOKButton";
            this._theOKButton.Size = new System.Drawing.Size(75, 23);
            this._theOKButton.TabIndex = 1;
            this._theOKButton.Text = "&Connect";
            this._theOKButton.UseVisualStyleBackColor = true;
            this._theOKButton.Click += new System.EventHandler(this.TheApplyButtonClick);
            // 
            // _theTestButton
            // 
            this._theTestButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theTestButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theTestButton.Location = new System.Drawing.Point(3, 4);
            this._theTestButton.Name = "_theTestButton";
            this._theTestButton.Size = new System.Drawing.Size(75, 23);
            this._theTestButton.TabIndex = 0;
            this._theTestButton.Text = "&Test";
            this._theTestButton.UseVisualStyleBackColor = true;
            this._theTestButton.Click += new System.EventHandler(this.TheTestButtonClick);
            // 
            // _theConnectionDefinitionUserControl
            // 
            this._theConnectionDefinitionUserControl.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theConnectionDefinitionUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theConnectionDefinitionUserControl.Location = new System.Drawing.Point(0, 0);
            this._theConnectionDefinitionUserControl.Name = "_theConnectionDefinitionUserControl";
            this._theConnectionDefinitionUserControl.Size = new System.Drawing.Size(701, 419);
            this._theConnectionDefinitionUserControl.TabIndex = 1;
            // 
            // FixSystemUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._theConnectionDefinitionUserControl);
            this.Controls.Add(panel1);
            this.Name = "FixSystemUserControl";
            this.Size = new System.Drawing.Size(701, 449);
            panel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionButton _theTestButton;
        private TrafodionButton _theOKButton;
        private Trafodion.Manager.Framework.Connections.Controls.ConnectionDefinitionUserControl _theConnectionDefinitionUserControl;
        private TrafodionButton theEditButton;
    }
}
