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
    partial class ServicesCountersUserControl
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
            this._servicesCountersGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._countersDetailsButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._thresholdCounterText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._activeCounterText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._totalCounterText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._thresholdLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._activeLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._totalLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._servicesCountersGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _servicesCountersGroupBox
            // 
            this._servicesCountersGroupBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._servicesCountersGroupBox.Controls.Add(this._countersDetailsButton);
            this._servicesCountersGroupBox.Controls.Add(this._thresholdCounterText);
            this._servicesCountersGroupBox.Controls.Add(this._activeCounterText);
            this._servicesCountersGroupBox.Controls.Add(this._totalCounterText);
            this._servicesCountersGroupBox.Controls.Add(this._thresholdLabel);
            this._servicesCountersGroupBox.Controls.Add(this._activeLabel);
            this._servicesCountersGroupBox.Controls.Add(this._totalLabel);
            this._servicesCountersGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._servicesCountersGroupBox.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._servicesCountersGroupBox.Location = new System.Drawing.Point(0, 0);
            this._servicesCountersGroupBox.Name = "_servicesCountersGroupBox";
            this._servicesCountersGroupBox.Size = new System.Drawing.Size(188, 169);
            this._servicesCountersGroupBox.TabIndex = 1;
            this._servicesCountersGroupBox.TabStop = false;
            this._servicesCountersGroupBox.Text = "WMS Services";
            // 
            // _countersDetailsButton
            // 
            this._countersDetailsButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._countersDetailsButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._countersDetailsButton.Image = global::Trafodion.Manager.WorkloadArea.Properties.Resources.MagnifyingGlass;
            this._countersDetailsButton.Location = new System.Drawing.Point(162, 10);
            this._countersDetailsButton.Name = "_countersDetailsButton";
            this._countersDetailsButton.Size = new System.Drawing.Size(25, 25);
            this._countersDetailsButton.TabIndex = 8;
            this._countersDetailsButton.UseVisualStyleBackColor = true;
            this._countersDetailsButton.Click += new System.EventHandler(this._countersDetailsButton_Click);
            // 
            // _thresholdCounterText
            // 
            this._thresholdCounterText.AutoSize = true;
            this._thresholdCounterText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._thresholdCounterText.ForeColor = System.Drawing.Color.Red;
            this._thresholdCounterText.Location = new System.Drawing.Point(101, 101);
            this._thresholdCounterText.Name = "_thresholdCounterText";
            this._thresholdCounterText.Size = new System.Drawing.Size(32, 16);
            this._thresholdCounterText.TabIndex = 1;
            this._thresholdCounterText.Text = "100";
            // 
            // _activeCounterText
            // 
            this._activeCounterText.AutoSize = true;
            this._activeCounterText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._activeCounterText.ForeColor = System.Drawing.Color.Green;
            this._activeCounterText.Location = new System.Drawing.Point(101, 67);
            this._activeCounterText.Name = "_activeCounterText";
            this._activeCounterText.Size = new System.Drawing.Size(32, 16);
            this._activeCounterText.TabIndex = 1;
            this._activeCounterText.Text = "100";
            // 
            // _totalCounterText
            // 
            this._totalCounterText.AutoSize = true;
            this._totalCounterText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._totalCounterText.Location = new System.Drawing.Point(101, 33);
            this._totalCounterText.Name = "_totalCounterText";
            this._totalCounterText.Size = new System.Drawing.Size(32, 16);
            this._totalCounterText.TabIndex = 1;
            this._totalCounterText.Text = "100";
            // 
            // _thresholdLabel
            // 
            this._thresholdLabel.AutoSize = true;
            this._thresholdLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._thresholdLabel.Location = new System.Drawing.Point(6, 101);
            this._thresholdLabel.Name = "_thresholdLabel";
            this._thresholdLabel.Size = new System.Drawing.Size(90, 16);
            this._thresholdLabel.TabIndex = 0;
            this._thresholdLabel.Text = "Threshold    : ";
            // 
            // _activeLabel
            // 
            this._activeLabel.AutoSize = true;
            this._activeLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._activeLabel.Location = new System.Drawing.Point(31, 67);
            this._activeLabel.Name = "_activeLabel";
            this._activeLabel.Size = new System.Drawing.Size(67, 16);
            this._activeLabel.TabIndex = 0;
            this._activeLabel.Text = "Active    : ";
            // 
            // _totalLabel
            // 
            this._totalLabel.AutoSize = true;
            this._totalLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._totalLabel.Location = new System.Drawing.Point(37, 33);
            this._totalLabel.Name = "_totalLabel";
            this._totalLabel.Size = new System.Drawing.Size(62, 16);
            this._totalLabel.TabIndex = 0;
            this._totalLabel.Text = "Total    : ";
            // 
            // ServicesCountersUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._servicesCountersGroupBox);
            this.Name = "ServicesCountersUserControl";
            this.Size = new System.Drawing.Size(188, 169);
            this._servicesCountersGroupBox.ResumeLayout(false);
            this._servicesCountersGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _servicesCountersGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _totalCounterText;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _thresholdLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _activeLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _totalLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _thresholdCounterText;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _activeCounterText;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _countersDetailsButton;
    }
}
