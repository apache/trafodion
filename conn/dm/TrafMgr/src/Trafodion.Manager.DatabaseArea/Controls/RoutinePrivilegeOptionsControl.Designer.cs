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
    partial class RoutinePrivilegeOptionsControl
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
            this._privGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._executeCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._privGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _privGroupBox
            // 
            this._privGroupBox.Controls.Add(this._executeCheckBox);
            this._privGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._privGroupBox.Location = new System.Drawing.Point(7, 2);
            this._privGroupBox.Name = "_privGroupBox";
            this._privGroupBox.Size = new System.Drawing.Size(238, 72);
            this._privGroupBox.TabIndex = 5;
            this._privGroupBox.TabStop = false;
            this._privGroupBox.Text = "Procedure Privileges";
            // 
            // _executeCheckBox
            // 
            this._executeCheckBox.AutoSize = true;
            this._executeCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._executeCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._executeCheckBox.Location = new System.Drawing.Point(24, 32);
            this._executeCheckBox.Name = "_executeCheckBox";
            this._executeCheckBox.Size = new System.Drawing.Size(71, 18);
            this._executeCheckBox.TabIndex = 4;
            this._executeCheckBox.Text = "Execute";
            this._executeCheckBox.UseVisualStyleBackColor = true;
            // 
            // RoutinePrivilegeOptionsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoScroll = true;
            this.Controls.Add(this._privGroupBox);
            this.Name = "RoutinePrivilegeOptionsControl";
            this.Size = new System.Drawing.Size(352, 122);
            this._privGroupBox.ResumeLayout(false);
            this._privGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _privGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _executeCheckBox;
    }
}
