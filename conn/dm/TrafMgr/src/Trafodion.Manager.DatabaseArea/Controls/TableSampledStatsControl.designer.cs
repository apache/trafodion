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
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    partial class TableSampledStatsControl
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
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle4 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle5 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle6 = new System.Windows.Forms.DataGridViewCellStyle();
            this._columnNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._summaryDataGridView = new Trafodion.Manager.Framework.Controls.TrafodionDataGridView();
            this._boundaryStatsDataGridView = new Trafodion.Manager.Framework.Controls.TrafodionDataGridView();
            this.headerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.headerLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._frequentValuesDataGridView = new Trafodion.Manager.Framework.Controls.TrafodionDataGridView();
            this.intervalGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.splitContainer2 = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._sampledStatsIntervalControl = new Trafodion.Manager.DatabaseArea.Controls.SampledStatsIntervalControl();
            this._sampledStatsGraphControl = new Trafodion.Manager.DatabaseArea.Controls.SampledStatsGraphControl();
            this.boundaryGridPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.chartPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.topTenGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.splitContainer1 = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._sampledStatsFreqValuesControl = new Trafodion.Manager.DatabaseArea.Controls.SampledStatsFreqValuesControl();
            this._sampledStatsSummaryControl = new Trafodion.Manager.DatabaseArea.Controls.SampledStatsSummaryControl();
            ((System.ComponentModel.ISupportInitialize)(this._summaryDataGridView)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._boundaryStatsDataGridView)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._frequentValuesDataGridView)).BeginInit();
            this.splitContainer2.Panel1.SuspendLayout();
            this.splitContainer2.Panel2.SuspendLayout();
            this.splitContainer2.SuspendLayout();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.SuspendLayout();
            // 
            // _columnNameLabel
            // 
            this._columnNameLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._columnNameLabel.Location = new System.Drawing.Point(0, 0);
            this._columnNameLabel.Name = "_columnNameLabel";
            this._columnNameLabel.Size = new System.Drawing.Size(100, 23);
            this._columnNameLabel.TabIndex = 0;
            // 
            // _summaryDataGridView
            // 
            this._summaryDataGridView.AllowUserToAddRows = false;
            this._summaryDataGridView.AllowUserToOrderColumns = true;
            dataGridViewCellStyle4.BackColor = System.Drawing.Color.WhiteSmoke;
            this._summaryDataGridView.AlternatingRowsDefaultCellStyle = dataGridViewCellStyle4;
            this._summaryDataGridView.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill;
            this._summaryDataGridView.AutoSizeRowsMode = System.Windows.Forms.DataGridViewAutoSizeRowsMode.AllCells;
            this._summaryDataGridView.BackgroundColor = System.Drawing.Color.WhiteSmoke;
            this._summaryDataGridView.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._summaryDataGridView.ClipboardCopyMode = System.Windows.Forms.DataGridViewClipboardCopyMode.EnableAlwaysIncludeHeaderText;
            this._summaryDataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this._summaryDataGridView.EditMode = System.Windows.Forms.DataGridViewEditMode.EditProgrammatically;
            this._summaryDataGridView.GridColor = System.Drawing.Color.LightGray;
            this._summaryDataGridView.Location = new System.Drawing.Point(0, 0);
            this._summaryDataGridView.Name = "_summaryDataGridView";
            this._summaryDataGridView.RowHeadersVisible = false;
            this._summaryDataGridView.RowTemplate.ReadOnly = true;
            this._summaryDataGridView.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this._summaryDataGridView.Size = new System.Drawing.Size(240, 150);
            this._summaryDataGridView.TabIndex = 0;
            this._summaryDataGridView.TheHeaderText = "";
            // 
            // _boundaryStatsDataGridView
            // 
            this._boundaryStatsDataGridView.AllowUserToAddRows = false;
            this._boundaryStatsDataGridView.AllowUserToOrderColumns = true;
            dataGridViewCellStyle5.BackColor = System.Drawing.Color.WhiteSmoke;
            this._boundaryStatsDataGridView.AlternatingRowsDefaultCellStyle = dataGridViewCellStyle5;
            this._boundaryStatsDataGridView.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill;
            this._boundaryStatsDataGridView.AutoSizeRowsMode = System.Windows.Forms.DataGridViewAutoSizeRowsMode.AllCells;
            this._boundaryStatsDataGridView.BackgroundColor = System.Drawing.Color.WhiteSmoke;
            this._boundaryStatsDataGridView.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._boundaryStatsDataGridView.ClipboardCopyMode = System.Windows.Forms.DataGridViewClipboardCopyMode.EnableAlwaysIncludeHeaderText;
            this._boundaryStatsDataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this._boundaryStatsDataGridView.EditMode = System.Windows.Forms.DataGridViewEditMode.EditProgrammatically;
            this._boundaryStatsDataGridView.GridColor = System.Drawing.Color.LightGray;
            this._boundaryStatsDataGridView.Location = new System.Drawing.Point(0, 0);
            this._boundaryStatsDataGridView.Name = "_boundaryStatsDataGridView";
            this._boundaryStatsDataGridView.RowHeadersVisible = false;
            this._boundaryStatsDataGridView.RowTemplate.ReadOnly = true;
            this._boundaryStatsDataGridView.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this._boundaryStatsDataGridView.Size = new System.Drawing.Size(240, 150);
            this._boundaryStatsDataGridView.TabIndex = 0;
            this._boundaryStatsDataGridView.TheHeaderText = "";
            // 
            // headerPanel
            // 
            this.headerPanel.Location = new System.Drawing.Point(0, 0);
            this.headerPanel.Name = "headerPanel";
            this.headerPanel.Size = new System.Drawing.Size(200, 100);
            this.headerPanel.TabIndex = 0;
            // 
            // headerLabel
            // 
            this.headerLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this.headerLabel.Location = new System.Drawing.Point(0, 0);
            this.headerLabel.Name = "headerLabel";
            this.headerLabel.Size = new System.Drawing.Size(100, 23);
            this.headerLabel.TabIndex = 0;
            // 
            // _frequentValuesDataGridView
            // 
            this._frequentValuesDataGridView.AllowUserToAddRows = false;
            this._frequentValuesDataGridView.AllowUserToOrderColumns = true;
            dataGridViewCellStyle6.BackColor = System.Drawing.Color.WhiteSmoke;
            this._frequentValuesDataGridView.AlternatingRowsDefaultCellStyle = dataGridViewCellStyle6;
            this._frequentValuesDataGridView.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill;
            this._frequentValuesDataGridView.AutoSizeRowsMode = System.Windows.Forms.DataGridViewAutoSizeRowsMode.AllCells;
            this._frequentValuesDataGridView.BackgroundColor = System.Drawing.Color.WhiteSmoke;
            this._frequentValuesDataGridView.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._frequentValuesDataGridView.ClipboardCopyMode = System.Windows.Forms.DataGridViewClipboardCopyMode.EnableAlwaysIncludeHeaderText;
            this._frequentValuesDataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this._frequentValuesDataGridView.EditMode = System.Windows.Forms.DataGridViewEditMode.EditProgrammatically;
            this._frequentValuesDataGridView.GridColor = System.Drawing.Color.LightGray;
            this._frequentValuesDataGridView.Location = new System.Drawing.Point(0, 0);
            this._frequentValuesDataGridView.Name = "_frequentValuesDataGridView";
            this._frequentValuesDataGridView.RowHeadersVisible = false;
            this._frequentValuesDataGridView.RowTemplate.ReadOnly = true;
            this._frequentValuesDataGridView.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this._frequentValuesDataGridView.Size = new System.Drawing.Size(240, 150);
            this._frequentValuesDataGridView.TabIndex = 0;
            this._frequentValuesDataGridView.TheHeaderText = "";
            // 
            // intervalGroupBox
            // 
            this.intervalGroupBox.Location = new System.Drawing.Point(0, 0);
            this.intervalGroupBox.Name = "intervalGroupBox";
            this.intervalGroupBox.Size = new System.Drawing.Size(200, 100);
            this.intervalGroupBox.TabIndex = 0;
            this.intervalGroupBox.TabStop = false;
            // 
            // splitContainer2
            // 
            this.splitContainer2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer2.Location = new System.Drawing.Point(0, 0);
            this.splitContainer2.Name = "splitContainer2";
            // 
            // splitContainer2.Panel1
            // 
            this.splitContainer2.Panel1.Controls.Add(this._sampledStatsIntervalControl);
            // 
            // splitContainer2.Panel2
            // 
            this.splitContainer2.Panel2.Controls.Add(this._sampledStatsGraphControl);
            this.splitContainer2.Size = new System.Drawing.Size(972, 345);
            this.splitContainer2.SplitterDistance = 491;
            this.splitContainer2.SplitterWidth = 9;
            this.splitContainer2.TabIndex = 0;
            // 
            // _sampledStatsIntervalControl
            // 
            this._sampledStatsIntervalControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._sampledStatsIntervalControl.Location = new System.Drawing.Point(0, 0);
            this._sampledStatsIntervalControl.Name = "_sampledStatsIntervalControl";
            this._sampledStatsIntervalControl.Size = new System.Drawing.Size(491, 345);
            this._sampledStatsIntervalControl.TabIndex = 2;
            // 
            // _sampledStatsGraphControl
            // 
            this._sampledStatsGraphControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._sampledStatsGraphControl.Location = new System.Drawing.Point(0, 0);
            this._sampledStatsGraphControl.Name = "_sampledStatsGraphControl";
            this._sampledStatsGraphControl.Size = new System.Drawing.Size(472, 345);
            this._sampledStatsGraphControl.TabIndex = 3;
            // 
            // boundaryGridPanel
            // 
            this.boundaryGridPanel.Location = new System.Drawing.Point(0, 0);
            this.boundaryGridPanel.Name = "boundaryGridPanel";
            this.boundaryGridPanel.Size = new System.Drawing.Size(200, 100);
            this.boundaryGridPanel.TabIndex = 0;
            // 
            // chartPanel
            // 
            this.chartPanel.Location = new System.Drawing.Point(0, 0);
            this.chartPanel.Name = "chartPanel";
            this.chartPanel.Size = new System.Drawing.Size(200, 100);
            this.chartPanel.TabIndex = 0;
            // 
            // topTenGroupBox
            // 
            this.topTenGroupBox.Location = new System.Drawing.Point(0, 0);
            this.topTenGroupBox.Name = "topTenGroupBox";
            this.topTenGroupBox.Size = new System.Drawing.Size(200, 100);
            this.topTenGroupBox.TabIndex = 0;
            this.topTenGroupBox.TabStop = false;
            // 
            // splitContainer1
            // 
            this.splitContainer1.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(0, 157);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this._sampledStatsFreqValuesControl);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.splitContainer2);
            this.splitContainer1.Size = new System.Drawing.Size(976, 606);
            this.splitContainer1.SplitterDistance = 248;
            this.splitContainer1.SplitterWidth = 9;
            this.splitContainer1.TabIndex = 4;
            // 
            // _sampledStatsFreqValuesControl
            // 
            this._sampledStatsFreqValuesControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._sampledStatsFreqValuesControl.Location = new System.Drawing.Point(0, 0);
            this._sampledStatsFreqValuesControl.Name = "_sampledStatsFreqValuesControl";
            this._sampledStatsFreqValuesControl.Size = new System.Drawing.Size(972, 244);
            this._sampledStatsFreqValuesControl.TabIndex = 1;
            // 
            // _sampledStatsSummaryControl
            // 
            this._sampledStatsSummaryControl.Dock = System.Windows.Forms.DockStyle.Top;
            this._sampledStatsSummaryControl.Location = new System.Drawing.Point(0, 0);
            this._sampledStatsSummaryControl.Name = "_sampledStatsSummaryControl";
            this._sampledStatsSummaryControl.Size = new System.Drawing.Size(976, 157);
            this._sampledStatsSummaryControl.TabIndex = 0;
            // 
            // TableSampledStatsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.splitContainer1);
            this.Controls.Add(this._sampledStatsSummaryControl);
            this.Name = "TableSampledStatsControl";
            this.Size = new System.Drawing.Size(976, 763);
            ((System.ComponentModel.ISupportInitialize)(this._summaryDataGridView)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._boundaryStatsDataGridView)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._frequentValuesDataGridView)).EndInit();
            this.splitContainer2.Panel1.ResumeLayout(false);
            this.splitContainer2.Panel2.ResumeLayout(false);
            this.splitContainer2.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private SampledStatsSummaryControl _sampledStatsSummaryControl;
        private SampledStatsFreqValuesControl _sampledStatsFreqValuesControl;
        private SampledStatsIntervalControl _sampledStatsIntervalControl;
        private SampledStatsGraphControl _sampledStatsGraphControl;
        private TrafodionLabel _columnNameLabel;
        private TrafodionDataGridView _summaryDataGridView;
        private TrafodionDataGridView _boundaryStatsDataGridView;
        private TrafodionPanel headerPanel;
        private TrafodionDataGridView _frequentValuesDataGridView;
        private TrafodionGroupBox intervalGroupBox;
        private TrafodionGroupBox topTenGroupBox;
        private TrafodionLabel headerLabel;
        private TrafodionPanel chartPanel;
        private TrafodionPanel boundaryGridPanel;
        private TrafodionSplitContainer splitContainer1;
        private TrafodionSplitContainer splitContainer2;
    }
}
