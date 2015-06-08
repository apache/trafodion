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
    partial class OffenderWorkloadCanvas
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
            MyDispose(disposing);
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
            this.toolTip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._theWidgetGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.SuspendLayout();
            // 
            // toolTip1
            // 
            this.toolTip1.IsBalloon = true;
            // 
            // _theWidgetGroupBox
            // 
            this._theWidgetGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theWidgetGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theWidgetGroupBox.Location = new System.Drawing.Point(0, 0);
            this._theWidgetGroupBox.Name = "_theWidgetGroupBox";
            this._theWidgetGroupBox.Size = new System.Drawing.Size(678, 345);
            this._theWidgetGroupBox.TabIndex = 0;
            this._theWidgetGroupBox.TabStop = false;
            // 
            // OffenderWorkloadCanvas
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theWidgetGroupBox);
            this.Name = "OffenderWorkloadCanvas";
            this.Size = new System.Drawing.Size(678, 345);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionToolTip toolTip1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _theWidgetGroupBox;
    }
}
