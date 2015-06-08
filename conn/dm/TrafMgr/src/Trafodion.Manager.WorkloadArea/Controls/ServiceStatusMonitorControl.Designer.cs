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
    partial class ServiceStatusMonitorControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ServiceStatusMonitorControl));
            this.TrafodionIGrid1DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.TrafodionIGrid1DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.TrafodionIGrid1RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.dataGridGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._servicesGridPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._servicesDataGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this.buttonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._logButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.thresholdsGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.servicesOverThresholdLabel = new System.Windows.Forms.Label();
            this.totalServicesLabel = new System.Windows.Forms.Label();
            this.activeServicesLabel = new System.Windows.Forms.Label();
            this._totalServices = new System.Windows.Forms.Label();
            this._activeServices = new System.Windows.Forms.Label();
            this._servicesOverThreshold = new System.Windows.Forms.Label();
            this._thresholdExceededReason = new System.Windows.Forms.Label();
            this.dataGridGroupBox.SuspendLayout();
            this._servicesGridPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._servicesDataGrid)).BeginInit();
            this.buttonsPanel.SuspendLayout();
            this.thresholdsGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // dataGridGroupBox
            // 
            this.dataGridGroupBox.Controls.Add(this._servicesGridPanel);
            this.dataGridGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.dataGridGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.dataGridGroupBox.Location = new System.Drawing.Point(0, 94);
            this.dataGridGroupBox.Name = "dataGridGroupBox";
            this.dataGridGroupBox.Size = new System.Drawing.Size(862, 340);
            this.dataGridGroupBox.TabIndex = 0;
            this.dataGridGroupBox.TabStop = false;
            this.dataGridGroupBox.Text = "Service Details";
            // 
            // _servicesGridPanel
            // 
            this._servicesGridPanel.AutoScroll = true;
            this._servicesGridPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._servicesGridPanel.Controls.Add(this._servicesDataGrid);
            this._servicesGridPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._servicesGridPanel.Location = new System.Drawing.Point(3, 17);
            this._servicesGridPanel.Name = "_servicesGridPanel";
            this._servicesGridPanel.Size = new System.Drawing.Size(856, 320);
            this._servicesGridPanel.TabIndex = 12;
            // 
            // _servicesDataGrid
            // 
            this._servicesDataGrid.AllowColumnFilter = true;
            this._servicesDataGrid.AllowWordWrap = false;
            this._servicesDataGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("_servicesDataGrid.AlwaysHiddenColumnNames")));
            this._servicesDataGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this._servicesDataGrid.CurrentFilter = null;
            this._servicesDataGrid.DefaultCol.CellStyle = this.TrafodionIGrid1DefaultCellStyle;
            this._servicesDataGrid.DefaultCol.ColHdrStyle = this.TrafodionIGrid1DefaultColHdrStyle;
            this._servicesDataGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._servicesDataGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._servicesDataGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._servicesDataGrid.Header.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._servicesDataGrid.Header.ForeColor = System.Drawing.Color.Black;
            this._servicesDataGrid.Header.Height = 20;
            this._servicesDataGrid.HelpTopic = "";
            this._servicesDataGrid.Location = new System.Drawing.Point(0, 0);
            this._servicesDataGrid.Name = "_servicesDataGrid";
            this._servicesDataGrid.ReadOnly = true;
            this._servicesDataGrid.RowMode = true;
            this._servicesDataGrid.RowTextCol.CellStyle = this.TrafodionIGrid1RowTextColCellStyle;
            this._servicesDataGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this._servicesDataGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this._servicesDataGrid.SearchAsType.SearchCol = null;
            this._servicesDataGrid.Size = new System.Drawing.Size(856, 320);
            this._servicesDataGrid.TabIndex = 0;
            this._servicesDataGrid.TreeCol = null;
            this._servicesDataGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this._servicesDataGrid.WordWrap = false;
            // 
            // buttonsPanel
            // 
            this.buttonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.buttonsPanel.Controls.Add(this._logButton);
            this.buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.buttonsPanel.Location = new System.Drawing.Point(0, 434);
            this.buttonsPanel.Name = "buttonsPanel";
            this.buttonsPanel.Size = new System.Drawing.Size(862, 34);
            this.buttonsPanel.TabIndex = 12;
            // 
            // _logButton
            // 
            this._logButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._logButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._logButton.Image = ((System.Drawing.Image)(resources.GetObject("_logButton.Image")));
            this._logButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._logButton.Location = new System.Drawing.Point(752, 3);
            this._logButton.Name = "_logButton";
            this._logButton.Size = new System.Drawing.Size(102, 26);
            this._logButton.TabIndex = 0;
            this._logButton.Text = " &Log To File";
            this._logButton.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this._logButton.UseVisualStyleBackColor = true;
            this._logButton.Click += new System.EventHandler(this._logButton_Click);
            // 
            // thresholdsGroupBox
            // 
            this.thresholdsGroupBox.Controls.Add(this.servicesOverThresholdLabel);
            this.thresholdsGroupBox.Controls.Add(this.totalServicesLabel);
            this.thresholdsGroupBox.Controls.Add(this.activeServicesLabel);
            this.thresholdsGroupBox.Controls.Add(this._totalServices);
            this.thresholdsGroupBox.Controls.Add(this._activeServices);
            this.thresholdsGroupBox.Controls.Add(this._servicesOverThreshold);
            this.thresholdsGroupBox.Controls.Add(this._thresholdExceededReason);
            this.thresholdsGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this.thresholdsGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.thresholdsGroupBox.Location = new System.Drawing.Point(0, 0);
            this.thresholdsGroupBox.Name = "thresholdsGroupBox";
            this.thresholdsGroupBox.Size = new System.Drawing.Size(862, 94);
            this.thresholdsGroupBox.TabIndex = 1;
            this.thresholdsGroupBox.TabStop = false;
            this.thresholdsGroupBox.Text = "Service Summary";
            // 
            // servicesOverThresholdLabel
            // 
            this.servicesOverThresholdLabel.AutoSize = true;
            this.servicesOverThresholdLabel.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.servicesOverThresholdLabel.ForeColor = System.Drawing.Color.Black;
            this.servicesOverThresholdLabel.Location = new System.Drawing.Point(224, 28);
            this.servicesOverThresholdLabel.Name = "servicesOverThresholdLabel";
            this.servicesOverThresholdLabel.Size = new System.Drawing.Size(185, 14);
            this.servicesOverThresholdLabel.TabIndex = 11;
            this.servicesOverThresholdLabel.Text = "Services that exceed threshold :";
            // 
            // totalServicesLabel
            // 
            this.totalServicesLabel.AutoSize = true;
            this.totalServicesLabel.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.totalServicesLabel.ForeColor = System.Drawing.Color.Black;
            this.totalServicesLabel.Location = new System.Drawing.Point(17, 28);
            this.totalServicesLabel.Name = "totalServicesLabel";
            this.totalServicesLabel.Size = new System.Drawing.Size(91, 14);
            this.totalServicesLabel.TabIndex = 9;
            this.totalServicesLabel.Text = "Total Services :";
            // 
            // activeServicesLabel
            // 
            this.activeServicesLabel.AutoSize = true;
            this.activeServicesLabel.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.activeServicesLabel.ForeColor = System.Drawing.Color.Black;
            this.activeServicesLabel.Location = new System.Drawing.Point(11, 58);
            this.activeServicesLabel.Name = "activeServicesLabel";
            this.activeServicesLabel.Size = new System.Drawing.Size(97, 14);
            this.activeServicesLabel.TabIndex = 10;
            this.activeServicesLabel.Text = "Active Services :";
            // 
            // _totalServices
            // 
            this._totalServices.AutoSize = true;
            this._totalServices.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._totalServices.ForeColor = System.Drawing.Color.Black;
            this._totalServices.Location = new System.Drawing.Point(114, 28);
            this._totalServices.Name = "_totalServices";
            this._totalServices.Size = new System.Drawing.Size(16, 16);
            this._totalServices.TabIndex = 7;
            this._totalServices.Text = "0";
            // 
            // _activeServices
            // 
            this._activeServices.AutoSize = true;
            this._activeServices.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._activeServices.ForeColor = System.Drawing.Color.ForestGreen;
            this._activeServices.Location = new System.Drawing.Point(114, 58);
            this._activeServices.Name = "_activeServices";
            this._activeServices.Size = new System.Drawing.Size(16, 16);
            this._activeServices.TabIndex = 6;
            this._activeServices.Text = "0";
            // 
            // _servicesOverThreshold
            // 
            this._servicesOverThreshold.AutoSize = true;
            this._servicesOverThreshold.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._servicesOverThreshold.ForeColor = System.Drawing.Color.Red;
            this._servicesOverThreshold.Location = new System.Drawing.Point(415, 28);
            this._servicesOverThreshold.Name = "_servicesOverThreshold";
            this._servicesOverThreshold.Size = new System.Drawing.Size(16, 16);
            this._servicesOverThreshold.TabIndex = 5;
            this._servicesOverThreshold.Text = "0";
            // 
            // _thresholdExceededReason
            // 
            this._thresholdExceededReason.AutoSize = true;
            this._thresholdExceededReason.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._thresholdExceededReason.ForeColor = System.Drawing.Color.ForestGreen;
            this._thresholdExceededReason.Location = new System.Drawing.Point(224, 62);
            this._thresholdExceededReason.Name = "_thresholdExceededReason";
            this._thresholdExceededReason.Size = new System.Drawing.Size(88, 14);
            this._thresholdExceededReason.TabIndex = 8;
            this._thresholdExceededReason.Text = "Last Sample :  ";
            // 
            // ServiceStatusMonitorControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.dataGridGroupBox);
            this.Controls.Add(this.buttonsPanel);
            this.Controls.Add(this.thresholdsGroupBox);
            this.Name = "ServiceStatusMonitorControl";
            this.Size = new System.Drawing.Size(862, 468);
            this.dataGridGroupBox.ResumeLayout(false);
            this._servicesGridPanel.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this._servicesDataGrid)).EndInit();
            this.buttonsPanel.ResumeLayout(false);
            this.thresholdsGroupBox.ResumeLayout(false);
            this.thresholdsGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox thresholdsGroupBox;
        private System.Windows.Forms.Label servicesOverThresholdLabel;
        private System.Windows.Forms.Label totalServicesLabel;
        private System.Windows.Forms.Label activeServicesLabel;
        private System.Windows.Forms.Label _totalServices;
        private System.Windows.Forms.Label _activeServices;
        private System.Windows.Forms.Label _servicesOverThreshold;
        private System.Windows.Forms.Label _thresholdExceededReason;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox dataGridGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionIGrid _servicesDataGrid;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle TrafodionIGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle TrafodionIGrid1RowTextColCellStyle;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel buttonsPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _logButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _servicesGridPanel;
    }
}
