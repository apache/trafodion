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
    partial class MySystemsUserControl
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
            Trafodion.Manager.Framework.Controls.TrafodionPanel panel1;
            this._theDisconnectButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theRemoveButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theEditButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theAddLikeButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theAddButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theTestButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.gridPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            panel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            panel1.Controls.Add(this._theDisconnectButton);
            panel1.Controls.Add(this._theRemoveButton);
            panel1.Controls.Add(this._theEditButton);
            panel1.Controls.Add(this._theAddLikeButton);
            panel1.Controls.Add(this._theAddButton);
            panel1.Controls.Add(this._theTestButton);
            panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            panel1.Location = new System.Drawing.Point(0, 309);
            panel1.Name = "panel1";
            panel1.Size = new System.Drawing.Size(676, 40);
            panel1.TabIndex = 0;
            // 
            // _theDisconnectButton
            // 
            this._theDisconnectButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theDisconnectButton.Location = new System.Drawing.Point(572, 10);
            this._theDisconnectButton.Name = "_theDisconnectButton";
            this._theDisconnectButton.Size = new System.Drawing.Size(92, 23);
            this._theDisconnectButton.TabIndex = 5;
            this._theDisconnectButton.Text = "&Disconnect";
            this._theDisconnectButton.UseVisualStyleBackColor = true;
            this._theDisconnectButton.Click += new System.EventHandler(this.TheDisconnectButtonClick);
            // 
            // _theRemoveButton
            // 
            this._theRemoveButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theRemoveButton.Location = new System.Drawing.Point(474, 10);
            this._theRemoveButton.Name = "_theRemoveButton";
            this._theRemoveButton.Size = new System.Drawing.Size(92, 23);
            this._theRemoveButton.TabIndex = 4;
            this._theRemoveButton.Text = "&Remove";
            this._theRemoveButton.UseVisualStyleBackColor = true;
            this._theRemoveButton.Click += new System.EventHandler(this.TheRemoveButtonClick);
            // 
            // _theEditButton
            // 
            this._theEditButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theEditButton.Location = new System.Drawing.Point(107, 10);
            this._theEditButton.Name = "_theEditButton";
            this._theEditButton.Size = new System.Drawing.Size(92, 23);
            this._theEditButton.TabIndex = 3;
            this._theEditButton.Text = "Ed&it ...";
            this._theEditButton.UseVisualStyleBackColor = true;
            this._theEditButton.Click += new System.EventHandler(this.TheEditButtonClick);
            // 
            // _theAddLikeButton
            // 
            this._theAddLikeButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theAddLikeButton.Location = new System.Drawing.Point(303, 10);
            this._theAddLikeButton.Name = "_theAddLikeButton";
            this._theAddLikeButton.Size = new System.Drawing.Size(92, 23);
            this._theAddLikeButton.TabIndex = 2;
            this._theAddLikeButton.Text = "Add &Like ...";
            this._theAddLikeButton.UseVisualStyleBackColor = true;
            this._theAddLikeButton.Click += new System.EventHandler(this.TheAddLikeButtonClick);
            // 
            // _theAddButton
            // 
            this._theAddButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theAddButton.Location = new System.Drawing.Point(205, 10);
            this._theAddButton.Name = "_theAddButton";
            this._theAddButton.Size = new System.Drawing.Size(92, 23);
            this._theAddButton.TabIndex = 1;
            this._theAddButton.Text = "&Add ...";
            this._theAddButton.UseVisualStyleBackColor = true;
            this._theAddButton.Click += new System.EventHandler(this.TheAddButtonClick);
            // 
            // _theTestButton
            // 
            this._theTestButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theTestButton.Location = new System.Drawing.Point(9, 10);
            this._theTestButton.Name = "_theTestButton";
            this._theTestButton.Size = new System.Drawing.Size(92, 23);
            this._theTestButton.TabIndex = 0;
            this._theTestButton.Text = "Te&st";
            this._theTestButton.UseVisualStyleBackColor = true;
            this._theTestButton.Click += new System.EventHandler(this.TheTestButtonClick);
            // 
            // gridPanel
            // 
            this.gridPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.gridPanel.Location = new System.Drawing.Point(0, 0);
            this.gridPanel.Name = "gridPanel";
            this.gridPanel.Size = new System.Drawing.Size(676, 309);
            this.gridPanel.TabIndex = 1;
            // 
            // MySystemsUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.gridPanel);
            this.Controls.Add(panel1);
            this.Name = "MySystemsUserControl";
            this.Size = new System.Drawing.Size(676, 349);
            panel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionButton _theAddButton;
        private TrafodionButton _theTestButton;
        private TrafodionButton _theDisconnectButton;
        private TrafodionButton _theRemoveButton;
        private TrafodionButton _theEditButton;
        private TrafodionButton _theAddLikeButton;
        private TrafodionPanel gridPanel;
    }
}
