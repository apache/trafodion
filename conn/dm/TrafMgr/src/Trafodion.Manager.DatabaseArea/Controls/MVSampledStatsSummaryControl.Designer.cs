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
    partial class MVSampledStatsSummaryControl
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
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle1 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle2 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle3 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle4 = new System.Windows.Forms.DataGridViewCellStyle();
            this._columnNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.headerLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._summaryDataGridView = new Trafodion.Manager.Framework.Controls.TrafodionDataGridView();
            this._sampleTimeStamp = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._progressBar = new Trafodion.Manager.Framework.Controls.TrafodionProgressBar();
            ((System.ComponentModel.ISupportInitialize)(this._summaryDataGridView)).BeginInit();
            this.SuspendLayout();
            // 
            // _columnNameLabel
            // 
            this._columnNameLabel.AutoSize = true;
            this._columnNameLabel.Location = new System.Drawing.Point(1, 0);
            this._columnNameLabel.Name = "_columnNameLabel";
            this._columnNameLabel.Size = new System.Drawing.Size(79, 13);
            this._columnNameLabel.TabIndex = 2;
            this._columnNameLabel.Text = "Column Name :";
            // 
            // headerLabel
            // 
            this.headerLabel.AutoSize = true;
            this.headerLabel.Location = new System.Drawing.Point(1, 23);
            this.headerLabel.Name = "headerLabel";
            this.headerLabel.Size = new System.Drawing.Size(100, 13);
            this.headerLabel.TabIndex = 2;
            this.headerLabel.Text = "Sampled from table ";
            // 
            // _summaryDataGridView
            // 
            this._summaryDataGridView.AllowUserToAddRows = false;
            this._summaryDataGridView.AllowUserToOrderColumns = true;
            dataGridViewCellStyle1.BackColor = System.Drawing.Color.WhiteSmoke;
            this._summaryDataGridView.AlternatingRowsDefaultCellStyle = dataGridViewCellStyle1;
            this._summaryDataGridView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._summaryDataGridView.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill;
            this._summaryDataGridView.AutoSizeRowsMode = System.Windows.Forms.DataGridViewAutoSizeRowsMode.AllCells;
            this._summaryDataGridView.BackgroundColor = System.Drawing.Color.WhiteSmoke;
            this._summaryDataGridView.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._summaryDataGridView.ClipboardCopyMode = System.Windows.Forms.DataGridViewClipboardCopyMode.EnableAlwaysIncludeHeaderText;
            dataGridViewCellStyle2.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle2.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle2.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle2.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle2.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle2.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle2.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this._summaryDataGridView.ColumnHeadersDefaultCellStyle = dataGridViewCellStyle2;
            this._summaryDataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            dataGridViewCellStyle3.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle3.BackColor = System.Drawing.Color.WhiteSmoke;
            dataGridViewCellStyle3.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle3.ForeColor = System.Drawing.SystemColors.ControlText;
            dataGridViewCellStyle3.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle3.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle3.WrapMode = System.Windows.Forms.DataGridViewTriState.False;
            this._summaryDataGridView.DefaultCellStyle = dataGridViewCellStyle3;
            this._summaryDataGridView.EditMode = System.Windows.Forms.DataGridViewEditMode.EditProgrammatically;
            this._summaryDataGridView.GridColor = System.Drawing.Color.LightGray;
            this._summaryDataGridView.Location = new System.Drawing.Point(0, 77);
            this._summaryDataGridView.Margin = new System.Windows.Forms.Padding(0, 2, 0, 0);
            this._summaryDataGridView.Name = "_summaryDataGridView";
            dataGridViewCellStyle4.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle4.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle4.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle4.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle4.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle4.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle4.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this._summaryDataGridView.RowHeadersDefaultCellStyle = dataGridViewCellStyle4;
            this._summaryDataGridView.RowHeadersVisible = false;
            this._summaryDataGridView.RowTemplate.Height = 24;
            this._summaryDataGridView.RowTemplate.ReadOnly = true;
            this._summaryDataGridView.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this._summaryDataGridView.Size = new System.Drawing.Size(694, 155);
            this._summaryDataGridView.TabIndex = 5;
            this._summaryDataGridView.TheHeaderText = "";
            // 
            // _sampleTimeStamp
            // 
            this._sampleTimeStamp.AutoSize = true;
            this._sampleTimeStamp.Location = new System.Drawing.Point(1, 47);
            this._sampleTimeStamp.Name = "_sampleTimeStamp";
            this._sampleTimeStamp.Size = new System.Drawing.Size(104, 13);
            this._sampleTimeStamp.TabIndex = 2;
            this._sampleTimeStamp.Text = "Computing Sample...";
            // 
            // _progressBar
            // 
            this._progressBar.Location = new System.Drawing.Point(125, 47);
            this._progressBar.Name = "_progressBar";
            this._progressBar.Size = new System.Drawing.Size(556, 13);
            this._progressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            this._progressBar.TabIndex = 6;
            // 
            // SampledStatsSummaryControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._progressBar);
            this.Controls.Add(this._summaryDataGridView);
            this.Controls.Add(this._columnNameLabel);
            this.Controls.Add(this._sampleTimeStamp);
            this.Controls.Add(this.headerLabel);
            this.Name = "SampledStatsSummaryControl";
            this.Size = new System.Drawing.Size(694, 230);
            ((System.ComponentModel.ISupportInitialize)(this._summaryDataGridView)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionLabel _columnNameLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel headerLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionDataGridView _summaryDataGridView;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _sampleTimeStamp;
        private Trafodion.Manager.Framework.Controls.TrafodionProgressBar _progressBar;

    }
}
