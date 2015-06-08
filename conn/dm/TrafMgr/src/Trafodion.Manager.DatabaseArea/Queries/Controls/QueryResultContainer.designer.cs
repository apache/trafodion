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
namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    partial class QueryResultContainer
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
            this._lastExecutionTimeLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._timeToEvaluateLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._executionProgressBar = new Trafodion.Manager.Framework.Controls.TrafodionProgressBar();
            this._executionStatusLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._nextButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._resultControlPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.headerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._queryStatusPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _queryStatusPanel
            // 
            this._queryStatusPanel.Controls.Add(this._lastExecutionTimeLabel);
            this._queryStatusPanel.Controls.Add(this._timeToEvaluateLabel);
            this._queryStatusPanel.Controls.Add(this._executionProgressBar);
            this._queryStatusPanel.Controls.Add(this._executionStatusLabel);
            this._queryStatusPanel.Controls.Add(this._nextButton);
            this._queryStatusPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._queryStatusPanel.Location = new System.Drawing.Point(0, 36);
            this._queryStatusPanel.Name = "_queryStatusPanel";
            this._queryStatusPanel.Size = new System.Drawing.Size(714, 28);
            this._queryStatusPanel.TabIndex = 2;
            // 
            // _lastExecutionTimeLabel
            // 
            this._lastExecutionTimeLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this._lastExecutionTimeLabel.AutoSize = true;
            this._lastExecutionTimeLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._lastExecutionTimeLabel.Location = new System.Drawing.Point(3, 8);
            this._lastExecutionTimeLabel.Name = "_lastExecutionTimeLabel";
            this._lastExecutionTimeLabel.Size = new System.Drawing.Size(108, 13);
            this._lastExecutionTimeLabel.TabIndex = 3;
            this._lastExecutionTimeLabel.Text = "Last Executed: \"N/A\"";
            // 
            // _timeToEvaluateLabel
            // 
            this._timeToEvaluateLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this._timeToEvaluateLabel.AutoSize = true;
            this._timeToEvaluateLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._timeToEvaluateLabel.Location = new System.Drawing.Point(117, 8);
            this._timeToEvaluateLabel.Name = "_timeToEvaluateLabel";
            this._timeToEvaluateLabel.Size = new System.Drawing.Size(115, 13);
            this._timeToEvaluateLabel.TabIndex = 4;
            this._timeToEvaluateLabel.Text = "Evaluation Time: \"N/A\"";
            // 
            // _executionProgressBar
            // 
            this._executionProgressBar.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this._executionProgressBar.Location = new System.Drawing.Point(238, 8);
            this._executionProgressBar.Margin = new System.Windows.Forms.Padding(3, 5, 3, 3);
            this._executionProgressBar.Name = "_executionProgressBar";
            this._executionProgressBar.Size = new System.Drawing.Size(283, 14);
            this._executionProgressBar.Step = 1;
            this._executionProgressBar.TabIndex = 5;
            this._executionProgressBar.Visible = false;
            // 
            // _executionStatusLabel
            // 
            this._executionStatusLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this._executionStatusLabel.AutoSize = true;
            this._executionStatusLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._executionStatusLabel.Location = new System.Drawing.Point(527, 8);
            this._executionStatusLabel.Name = "_executionStatusLabel";
            this._executionStatusLabel.Padding = new System.Windows.Forms.Padding(5, 0, 0, 0);
            this._executionStatusLabel.Size = new System.Drawing.Size(38, 13);
            this._executionStatusLabel.TabIndex = 4;
            this._executionStatusLabel.Text = "\"N/A\"";
            // 
            // _nextButton
            // 
            this._nextButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this._nextButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._nextButton.Location = new System.Drawing.Point(571, 3);
            this._nextButton.Name = "_nextButton";
            this._nextButton.Size = new System.Drawing.Size(75, 23);
            this._nextButton.TabIndex = 6;
            this._nextButton.Text = "Next Page";
            this._nextButton.UseVisualStyleBackColor = true;
            this._nextButton.Click += new System.EventHandler(this.nextButton_Click);
            // 
            // _resultControlPanel
            // 
            this._resultControlPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._resultControlPanel.Location = new System.Drawing.Point(0, 64);
            this._resultControlPanel.Name = "_resultControlPanel";
            this._resultControlPanel.Size = new System.Drawing.Size(714, 292);
            this._resultControlPanel.TabIndex = 3;
            // 
            // headerPanel
            // 
            this.headerPanel.BackColor = System.Drawing.SystemColors.Control;
            this.headerPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.headerPanel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.headerPanel.Location = new System.Drawing.Point(0, 0);
            this.headerPanel.Name = "headerPanel";
            this.headerPanel.Size = new System.Drawing.Size(714, 36);
            this.headerPanel.TabIndex = 4;
            // 
            // QueryResultContainer
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._resultControlPanel);
            this.Controls.Add(this._queryStatusPanel);
            this.Controls.Add(this.headerPanel);
            this.Name = "QueryResultContainer";
            this.Size = new System.Drawing.Size(714, 356);
            this._queryStatusPanel.ResumeLayout(false);
            this._queryStatusPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.FlowLayoutPanel _queryStatusPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _lastExecutionTimeLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _timeToEvaluateLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionProgressBar _executionProgressBar;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _executionStatusLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _resultControlPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _nextButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel headerPanel;
    }
}
