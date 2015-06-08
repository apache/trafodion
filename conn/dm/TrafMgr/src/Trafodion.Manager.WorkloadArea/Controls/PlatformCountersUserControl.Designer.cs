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
    partial class PlatformCountersUserControl
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
            this._platformCountersGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._excQueriesCounterText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._excQueriesLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._countersDetailsButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._memUsageCounterText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._nodeBusyCounterText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._memUsageLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._nodeBusyLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._platformCountersGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _platformCountersGroupBox
            // 
            this._platformCountersGroupBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._platformCountersGroupBox.Controls.Add(this._excQueriesCounterText);
            this._platformCountersGroupBox.Controls.Add(this._excQueriesLabel);
            this._platformCountersGroupBox.Controls.Add(this._countersDetailsButton);
            this._platformCountersGroupBox.Controls.Add(this._memUsageCounterText);
            this._platformCountersGroupBox.Controls.Add(this._nodeBusyCounterText);
            this._platformCountersGroupBox.Controls.Add(this._memUsageLabel);
            this._platformCountersGroupBox.Controls.Add(this._nodeBusyLabel);
            this._platformCountersGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._platformCountersGroupBox.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._platformCountersGroupBox.Location = new System.Drawing.Point(0, 0);
            this._platformCountersGroupBox.Name = "_platformCountersGroupBox";
            this._platformCountersGroupBox.Size = new System.Drawing.Size(241, 200);
            this._platformCountersGroupBox.TabIndex = 1;
            this._platformCountersGroupBox.TabStop = false;
            this._platformCountersGroupBox.Text = "Platform Counters";
            // 
            // _excQueriesCounterText
            // 
            this._excQueriesCounterText.AutoSize = true;
            this._excQueriesCounterText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._excQueriesCounterText.ForeColor = System.Drawing.Color.Green;
            this._excQueriesCounterText.Location = new System.Drawing.Point(155, 99);
            this._excQueriesCounterText.Name = "_excQueriesCounterText";
            this._excQueriesCounterText.Size = new System.Drawing.Size(32, 16);
            this._excQueriesCounterText.TabIndex = 12;
            this._excQueriesCounterText.Text = "100";
            // 
            // _excQueriesLabel
            // 
            this._excQueriesLabel.AutoSize = true;
            this._excQueriesLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._excQueriesLabel.Location = new System.Drawing.Point(40, 99);
            this._excQueriesLabel.Name = "_excQueriesLabel";
            this._excQueriesLabel.Size = new System.Drawing.Size(107, 16);
            this._excQueriesLabel.TabIndex = 10;
            this._excQueriesLabel.Text = "Exec Queries    : ";
            // 
            // _countersDetailsButton
            // 
            this._countersDetailsButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._countersDetailsButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._countersDetailsButton.Image = global::Trafodion.Manager.WorkloadArea.Properties.Resources.MagnifyingGlass;
            this._countersDetailsButton.Location = new System.Drawing.Point(215, 10);
            this._countersDetailsButton.Name = "_countersDetailsButton";
            this._countersDetailsButton.Size = new System.Drawing.Size(25, 25);
            this._countersDetailsButton.TabIndex = 7;
            this._countersDetailsButton.UseVisualStyleBackColor = true;
            this._countersDetailsButton.Click += new System.EventHandler(this._countersDetailsButton_Click);
            // 
            // _memUsageCounterText
            // 
            this._memUsageCounterText.AutoSize = true;
            this._memUsageCounterText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._memUsageCounterText.ForeColor = System.Drawing.Color.Green;
            this._memUsageCounterText.Location = new System.Drawing.Point(155, 66);
            this._memUsageCounterText.Name = "_memUsageCounterText";
            this._memUsageCounterText.Size = new System.Drawing.Size(32, 16);
            this._memUsageCounterText.TabIndex = 1;
            this._memUsageCounterText.Text = "100";
            // 
            // _nodeBusyCounterText
            // 
            this._nodeBusyCounterText.AutoSize = true;
            this._nodeBusyCounterText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._nodeBusyCounterText.ForeColor = System.Drawing.Color.Green;
            this._nodeBusyCounterText.Location = new System.Drawing.Point(155, 33);
            this._nodeBusyCounterText.Name = "_nodeBusyCounterText";
            this._nodeBusyCounterText.Size = new System.Drawing.Size(32, 16);
            this._nodeBusyCounterText.TabIndex = 1;
            this._nodeBusyCounterText.Text = "100";
            // 
            // _memUsageLabel
            // 
            this._memUsageLabel.AutoSize = true;
            this._memUsageLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._memUsageLabel.Location = new System.Drawing.Point(13, 66);
            this._memUsageLabel.Name = "_memUsageLabel";
            this._memUsageLabel.Size = new System.Drawing.Size(134, 16);
            this._memUsageLabel.TabIndex = 0;
            this._memUsageLabel.Text = "Memory Usage %    : ";
            // 
            // _nodeBusyLabel
            // 
            this._nodeBusyLabel.AutoSize = true;
            this._nodeBusyLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._nodeBusyLabel.Location = new System.Drawing.Point(39, 33);
            this._nodeBusyLabel.Name = "_nodeBusyLabel";
            this._nodeBusyLabel.Size = new System.Drawing.Size(108, 16);
            this._nodeBusyLabel.TabIndex = 0;
            this._nodeBusyLabel.Text = "Node Busy %    : ";
            // 
            // PlatformCountersUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._platformCountersGroupBox);
            this.Name = "PlatformCountersUserControl";
            this.Size = new System.Drawing.Size(241, 200);
            this._platformCountersGroupBox.ResumeLayout(false);
            this._platformCountersGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _platformCountersGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _nodeBusyCounterText;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _memUsageLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _nodeBusyLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _memUsageCounterText;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _countersDetailsButton;
        private Framework.Controls.TrafodionLabel _excQueriesLabel;
        private Framework.Controls.TrafodionLabel _excQueriesCounterText;
    }
}
