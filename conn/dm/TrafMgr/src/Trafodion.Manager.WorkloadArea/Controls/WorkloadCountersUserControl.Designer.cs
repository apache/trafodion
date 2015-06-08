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
    partial class WorkloadCountersUserControl
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
            this._workloadCountersGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._countersDetailsButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._rejectedCounterText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._holdCounterText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._waitingCounterText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._completedCounterText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._executingCounterText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._rejectedLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._holdLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._waitingLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._completedLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._executingLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._workloadCountersGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _workloadCountersGroupBox
            // 
            this._workloadCountersGroupBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._workloadCountersGroupBox.Controls.Add(this._countersDetailsButton);
            this._workloadCountersGroupBox.Controls.Add(this._rejectedCounterText);
            this._workloadCountersGroupBox.Controls.Add(this._holdCounterText);
            this._workloadCountersGroupBox.Controls.Add(this._waitingCounterText);
            this._workloadCountersGroupBox.Controls.Add(this._completedCounterText);
            this._workloadCountersGroupBox.Controls.Add(this._executingCounterText);
            this._workloadCountersGroupBox.Controls.Add(this._rejectedLabel);
            this._workloadCountersGroupBox.Controls.Add(this._holdLabel);
            this._workloadCountersGroupBox.Controls.Add(this._waitingLabel);
            this._workloadCountersGroupBox.Controls.Add(this._completedLabel);
            this._workloadCountersGroupBox.Controls.Add(this._executingLabel);
            this._workloadCountersGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._workloadCountersGroupBox.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._workloadCountersGroupBox.Location = new System.Drawing.Point(0, 0);
            this._workloadCountersGroupBox.Name = "_workloadCountersGroupBox";
            this._workloadCountersGroupBox.Size = new System.Drawing.Size(269, 206);
            this._workloadCountersGroupBox.TabIndex = 1;
            this._workloadCountersGroupBox.TabStop = false;
            this._workloadCountersGroupBox.Text = "Workload Summary";
            // 
            // _countersDetailsButton
            // 
            this._countersDetailsButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._countersDetailsButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._countersDetailsButton.Image = global::Trafodion.Manager.WorkloadArea.Properties.Resources.MagnifyingGlass;
            this._countersDetailsButton.Location = new System.Drawing.Point(243, 10);
            this._countersDetailsButton.Name = "_countersDetailsButton";
            this._countersDetailsButton.Size = new System.Drawing.Size(25, 25);
            this._countersDetailsButton.TabIndex = 8;
            this._countersDetailsButton.UseVisualStyleBackColor = true;
            this._countersDetailsButton.Click += new System.EventHandler(this._countersDetailsButton_Click);
            // 
            // _rejectedCounterText
            // 
            this._rejectedCounterText.AutoSize = true;
            this._rejectedCounterText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._rejectedCounterText.ForeColor = System.Drawing.Color.Red;
            this._rejectedCounterText.Location = new System.Drawing.Point(151, 165);
            this._rejectedCounterText.Name = "_rejectedCounterText";
            this._rejectedCounterText.Size = new System.Drawing.Size(32, 16);
            this._rejectedCounterText.TabIndex = 1;
            this._rejectedCounterText.Text = "100";
            // 
            // _holdCounterText
            // 
            this._holdCounterText.AutoSize = true;
            this._holdCounterText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._holdCounterText.ForeColor = System.Drawing.Color.DimGray;
            this._holdCounterText.Location = new System.Drawing.Point(151, 132);
            this._holdCounterText.Name = "_holdCounterText";
            this._holdCounterText.Size = new System.Drawing.Size(32, 16);
            this._holdCounterText.TabIndex = 1;
            this._holdCounterText.Text = "100";
            // 
            // _waitingCounterText
            // 
            this._waitingCounterText.AutoSize = true;
            this._waitingCounterText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._waitingCounterText.ForeColor = System.Drawing.Color.Orange;
            this._waitingCounterText.Location = new System.Drawing.Point(151, 99);
            this._waitingCounterText.Name = "_waitingCounterText";
            this._waitingCounterText.Size = new System.Drawing.Size(32, 16);
            this._waitingCounterText.TabIndex = 1;
            this._waitingCounterText.Text = "100";
            // 
            // _completedCounterText
            // 
            this._completedCounterText.AutoSize = true;
            this._completedCounterText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._completedCounterText.ForeColor = System.Drawing.Color.MidnightBlue;
            this._completedCounterText.Location = new System.Drawing.Point(151, 66);
            this._completedCounterText.Name = "_completedCounterText";
            this._completedCounterText.Size = new System.Drawing.Size(32, 16);
            this._completedCounterText.TabIndex = 1;
            this._completedCounterText.Text = "100";
            // 
            // _executingCounterText
            // 
            this._executingCounterText.AutoSize = true;
            this._executingCounterText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._executingCounterText.ForeColor = System.Drawing.Color.Green;
            this._executingCounterText.Location = new System.Drawing.Point(151, 33);
            this._executingCounterText.Name = "_executingCounterText";
            this._executingCounterText.Size = new System.Drawing.Size(32, 16);
            this._executingCounterText.TabIndex = 1;
            this._executingCounterText.Text = "100";
            // 
            // _rejectedLabel
            // 
            this._rejectedLabel.AutoSize = true;
            this._rejectedLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._rejectedLabel.Location = new System.Drawing.Point(59, 165);
            this._rejectedLabel.Name = "_rejectedLabel";
            this._rejectedLabel.Size = new System.Drawing.Size(83, 16);
            this._rejectedLabel.TabIndex = 0;
            this._rejectedLabel.Text = "Rejected    : ";
            // 
            // _holdLabel
            // 
            this._holdLabel.AutoSize = true;
            this._holdLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._holdLabel.Location = new System.Drawing.Point(16, 132);
            this._holdLabel.Name = "_holdLabel";
            this._holdLabel.Size = new System.Drawing.Size(126, 16);
            this._holdLabel.TabIndex = 0;
            this._holdLabel.Text = "Hold/Suspended    : ";
            // 
            // _waitingLabel
            // 
            this._waitingLabel.AutoSize = true;
            this._waitingLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._waitingLabel.Location = new System.Drawing.Point(66, 99);
            this._waitingLabel.Name = "_waitingLabel";
            this._waitingLabel.Size = new System.Drawing.Size(76, 16);
            this._waitingLabel.TabIndex = 0;
            this._waitingLabel.Text = "Waiting    : ";
            // 
            // _completedLabel
            // 
            this._completedLabel.AutoSize = true;
            this._completedLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._completedLabel.Location = new System.Drawing.Point(48, 66);
            this._completedLabel.Name = "_completedLabel";
            this._completedLabel.Size = new System.Drawing.Size(94, 16);
            this._completedLabel.TabIndex = 0;
            this._completedLabel.Text = "Completed    : ";
            // 
            // _executingLabel
            // 
            this._executingLabel.AutoSize = true;
            this._executingLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._executingLabel.Location = new System.Drawing.Point(55, 33);
            this._executingLabel.Name = "_executingLabel";
            this._executingLabel.Size = new System.Drawing.Size(87, 16);
            this._executingLabel.TabIndex = 0;
            this._executingLabel.Text = "Executing    : ";
            // 
            // WorkloadCountersUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._workloadCountersGroupBox);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "WorkloadCountersUserControl";
            this.Size = new System.Drawing.Size(269, 206);
            this._workloadCountersGroupBox.ResumeLayout(false);
            this._workloadCountersGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _workloadCountersGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _executingCounterText;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _holdLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _waitingLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _completedLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _executingLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _holdCounterText;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _waitingCounterText;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _completedCounterText;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _countersDetailsButton;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _rejectedCounterText;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _rejectedLabel;
    }
}
