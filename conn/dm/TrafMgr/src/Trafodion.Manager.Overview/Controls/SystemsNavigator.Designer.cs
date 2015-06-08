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
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.OverviewArea
{
    partial class SystemsNavigator
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
            this.theAllPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._navigationTreeViewUserControl = new Trafodion.Manager.Framework.Navigation.NavigationTreeViewUserControl();
            this.theAllLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.theAllPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // theAllPanel
            // 
            this.theAllPanel.BackColor = System.Drawing.SystemColors.Control;
            this.theAllPanel.Controls.Add(this._navigationTreeViewUserControl);
            this.theAllPanel.Controls.Add(this.theAllLabel);
            this.theAllPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theAllPanel.Location = new System.Drawing.Point(0, 0);
            this.theAllPanel.Name = "theAllPanel";
            this.theAllPanel.Size = new System.Drawing.Size(194, 221);
            this.theAllPanel.TabIndex = 11;
            // 
            // _navigationTreeViewUserControl
            // 
            this._navigationTreeViewUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._navigationTreeViewUserControl.Location = new System.Drawing.Point(0, 23);
            this._navigationTreeViewUserControl.Name = "_navigationTreeViewUserControl";
            this._navigationTreeViewUserControl.Size = new System.Drawing.Size(194, 198);
            this._navigationTreeViewUserControl.TabIndex = 5;
            // 
            // theAllLabel
            // 
            //this.theAllLabel.BackColor = System.Drawing.Color.SteelBlue;
            this.theAllLabel.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(117)))), ((int)(((byte)(145)))), ((int)(((byte)(172)))));
            this.theAllLabel.Dock = System.Windows.Forms.DockStyle.Top;
            this.theAllLabel.Font = new System.Drawing.Font("Tahoma", 9.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theAllLabel.ForeColor = System.Drawing.Color.White;
            this.theAllLabel.Location = new System.Drawing.Point(0, 0);
            this.theAllLabel.Name = "theAllLabel";
            this.theAllLabel.Size = new System.Drawing.Size(194, 23);
            this.theAllLabel.TabIndex = 0;
            this.theAllLabel.Text = "All";
            this.theAllLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // SystemsNavigator
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.theAllPanel);
            this.Name = "SystemsNavigator";
            this.Size = new System.Drawing.Size(194, 221);
            this.theAllPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionPanel theAllPanel;
        private NavigationTreeViewUserControl _navigationTreeViewUserControl;
        private TrafodionLabel theAllLabel;
    }
}
