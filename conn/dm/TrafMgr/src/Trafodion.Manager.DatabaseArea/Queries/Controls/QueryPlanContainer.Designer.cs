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
﻿namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    partial class QueryPlanContainer
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
            MyDispose(disposing);
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
            this._queryStatusPanel = new System.Windows.Forms.FlowLayoutPanel();
            this._lastEvaluationTimeLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._timeToEvaluateLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._evaluationProgressBar = new Trafodion.Manager.Framework.Controls.TrafodionProgressBar();
            this._evaluationStatusLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._queryPlanControlPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._queryStatusPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _queryStatusPanel
            // 
            this._queryStatusPanel.Controls.Add(this._lastEvaluationTimeLabel);
            this._queryStatusPanel.Controls.Add(this._timeToEvaluateLabel);
            this._queryStatusPanel.Controls.Add(this._evaluationProgressBar);
            this._queryStatusPanel.Controls.Add(this._evaluationStatusLabel);
            this._queryStatusPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._queryStatusPanel.Location = new System.Drawing.Point(0, 0);
            this._queryStatusPanel.Margin = new System.Windows.Forms.Padding(3, 5, 3, 3);
            this._queryStatusPanel.Name = "_queryStatusPanel";
            this._queryStatusPanel.Padding = new System.Windows.Forms.Padding(0, 3, 0, 0);
            this._queryStatusPanel.Size = new System.Drawing.Size(655, 28);
            this._queryStatusPanel.TabIndex = 3;
            // 
            // _lastEvaluationTimeLabel
            // 
            this._lastEvaluationTimeLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this._lastEvaluationTimeLabel.AutoSize = true;
            this._lastEvaluationTimeLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._lastEvaluationTimeLabel.Location = new System.Drawing.Point(3, 4);
            this._lastEvaluationTimeLabel.Name = "_lastEvaluationTimeLabel";
            this._lastEvaluationTimeLabel.Size = new System.Drawing.Size(111, 13);
            this._lastEvaluationTimeLabel.TabIndex = 3;
            this._lastEvaluationTimeLabel.Text = "Last Evaluated: \"N/A\"";
            // 
            // _timeToEvaluateLabel
            // 
            this._timeToEvaluateLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this._timeToEvaluateLabel.AutoSize = true;
            this._timeToEvaluateLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._timeToEvaluateLabel.Location = new System.Drawing.Point(120, 4);
            this._timeToEvaluateLabel.Name = "_timeToEvaluateLabel";
            this._timeToEvaluateLabel.Size = new System.Drawing.Size(115, 13);
            this._timeToEvaluateLabel.TabIndex = 4;
            this._timeToEvaluateLabel.Text = "Evaluation Time: \"N/A\"";
            // 
            // _evaluationProgressBar
            // 
            this._evaluationProgressBar.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this._evaluationProgressBar.Location = new System.Drawing.Point(241, 3);
            this._evaluationProgressBar.Margin = new System.Windows.Forms.Padding(3, 0, 3, 3);
            this._evaluationProgressBar.Name = "_evaluationProgressBar";
            this._evaluationProgressBar.Size = new System.Drawing.Size(283, 13);
            this._evaluationProgressBar.Step = 1;
            this._evaluationProgressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            this._evaluationProgressBar.TabIndex = 5;
            // 
            // _evaluationStatusLabel
            // 
            this._evaluationStatusLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this._evaluationStatusLabel.AutoSize = true;
            this._evaluationStatusLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._evaluationStatusLabel.Location = new System.Drawing.Point(530, 4);
            this._evaluationStatusLabel.Name = "_evaluationStatusLabel";
            this._evaluationStatusLabel.Padding = new System.Windows.Forms.Padding(5, 0, 0, 0);
            this._evaluationStatusLabel.Size = new System.Drawing.Size(38, 13);
            this._evaluationStatusLabel.TabIndex = 4;
            this._evaluationStatusLabel.Text = "\"N/A\"";
            // 
            // _queryPlanControlPanel
            // 
            this._queryPlanControlPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._queryPlanControlPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._queryPlanControlPanel.Location = new System.Drawing.Point(0, 28);
            this._queryPlanControlPanel.Name = "_queryPlanControlPanel";
            this._queryPlanControlPanel.Size = new System.Drawing.Size(655, 283);
            this._queryPlanControlPanel.TabIndex = 4;
            // 
            // QueryPlanContainer
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._queryPlanControlPanel);
            this.Controls.Add(this._queryStatusPanel);
            this.Name = "QueryPlanContainer";
            this.Size = new System.Drawing.Size(655, 311);
            this._queryStatusPanel.ResumeLayout(false);
            this._queryStatusPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.FlowLayoutPanel _queryStatusPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _lastEvaluationTimeLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _timeToEvaluateLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionProgressBar _evaluationProgressBar;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _evaluationStatusLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _queryPlanControlPanel;
    }
}
