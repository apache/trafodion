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
namespace Trafodion.Manager.WorkloadArea.Controls
{
	partial class TriageFilterPropertyGrid
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
            this.components = new System.ComponentModel.Container();
            this.nccWorkloadFilterButtonsPanel = new System.Windows.Forms.Panel();
            this.nccWorkloadFilterPropertyGridLoadButton = new System.Windows.Forms.Button();
            this.nccWorkloadFilterPropertyGridSaveButton = new System.Windows.Forms.Button();
            this.nccWorkloadFilterPropertyGridApplyButton = new System.Windows.Forms.Button();
            this.nccWorkloadFilterPropertyGridFetchButton = new System.Windows.Forms.Button();
            this.nccWorkloadFilterPropertyGridResetButton = new System.Windows.Forms.Button();
            this.nccWorkloadFilterInfoPanel = new System.Windows.Forms.Panel();
            this.filterPropertyGrid = new System.Windows.Forms.PropertyGrid();
            this.nccWorkloadFilterToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.nccWorkloadFilterButtonsPanel.SuspendLayout();
            this.nccWorkloadFilterInfoPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // nccWorkloadFilterButtonsPanel
            // 
            this.nccWorkloadFilterButtonsPanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.nccWorkloadFilterButtonsPanel.Controls.Add(this.nccWorkloadFilterPropertyGridLoadButton);
            this.nccWorkloadFilterButtonsPanel.Controls.Add(this.nccWorkloadFilterPropertyGridSaveButton);
            this.nccWorkloadFilterButtonsPanel.Controls.Add(this.nccWorkloadFilterPropertyGridApplyButton);
            this.nccWorkloadFilterButtonsPanel.Controls.Add(this.nccWorkloadFilterPropertyGridFetchButton);
            this.nccWorkloadFilterButtonsPanel.Controls.Add(this.nccWorkloadFilterPropertyGridResetButton);
            this.nccWorkloadFilterButtonsPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.nccWorkloadFilterButtonsPanel.Location = new System.Drawing.Point(0, 0);
            this.nccWorkloadFilterButtonsPanel.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.nccWorkloadFilterButtonsPanel.Name = "nccWorkloadFilterButtonsPanel";
            this.nccWorkloadFilterButtonsPanel.Size = new System.Drawing.Size(278, 61);
            this.nccWorkloadFilterButtonsPanel.TabIndex = 0;
            // 
            // nccWorkloadFilterPropertyGridLoadButton
            // 
            this.nccWorkloadFilterPropertyGridLoadButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.nccWorkloadFilterPropertyGridLoadButton.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.nccWorkloadFilterPropertyGridLoadButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccWorkloadFilterPropertyGridLoadButton.Image = global::Trafodion.Manager.WorkloadArea.Properties.Resources.openHS;
            this.nccWorkloadFilterPropertyGridLoadButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.nccWorkloadFilterPropertyGridLoadButton.Location = new System.Drawing.Point(8, 3);
            this.nccWorkloadFilterPropertyGridLoadButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.nccWorkloadFilterPropertyGridLoadButton.Name = "nccWorkloadFilterPropertyGridLoadButton";
            this.nccWorkloadFilterPropertyGridLoadButton.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.nccWorkloadFilterPropertyGridLoadButton.Size = new System.Drawing.Size(80, 25);
            this.nccWorkloadFilterPropertyGridLoadButton.TabIndex = 1;
            this.nccWorkloadFilterPropertyGridLoadButton.Text = " &Open";
            this.nccWorkloadFilterPropertyGridLoadButton.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.nccWorkloadFilterToolTip.SetToolTip(this.nccWorkloadFilterPropertyGridLoadButton, "Load filter information from a file");
            this.nccWorkloadFilterPropertyGridLoadButton.UseVisualStyleBackColor = true;
            this.nccWorkloadFilterPropertyGridLoadButton.Click += new System.EventHandler(this.nccFilterPropertyGridLoadButton_Click);
            // 
            // nccWorkloadFilterPropertyGridSaveButton
            // 
            this.nccWorkloadFilterPropertyGridSaveButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.nccWorkloadFilterPropertyGridSaveButton.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.nccWorkloadFilterPropertyGridSaveButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccWorkloadFilterPropertyGridSaveButton.Image = global::Trafodion.Manager.WorkloadArea.Properties.Resources.saveHS;
            this.nccWorkloadFilterPropertyGridSaveButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.nccWorkloadFilterPropertyGridSaveButton.Location = new System.Drawing.Point(98, 3);
            this.nccWorkloadFilterPropertyGridSaveButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.nccWorkloadFilterPropertyGridSaveButton.Name = "nccWorkloadFilterPropertyGridSaveButton";
            this.nccWorkloadFilterPropertyGridSaveButton.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.nccWorkloadFilterPropertyGridSaveButton.Size = new System.Drawing.Size(80, 25);
            this.nccWorkloadFilterPropertyGridSaveButton.TabIndex = 1;
            this.nccWorkloadFilterPropertyGridSaveButton.Text = " &Save";
            this.nccWorkloadFilterPropertyGridSaveButton.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.nccWorkloadFilterToolTip.SetToolTip(this.nccWorkloadFilterPropertyGridSaveButton, "Save filter information to a file");
            this.nccWorkloadFilterPropertyGridSaveButton.UseVisualStyleBackColor = true;
            this.nccWorkloadFilterPropertyGridSaveButton.Click += new System.EventHandler(this.nccFilterPropertyGridSaveButton_Click);
            // 
            // nccWorkloadFilterPropertyGridApplyButton
            // 
            this.nccWorkloadFilterPropertyGridApplyButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.nccWorkloadFilterPropertyGridApplyButton.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.nccWorkloadFilterPropertyGridApplyButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccWorkloadFilterPropertyGridApplyButton.Image = global::Trafodion.Manager.WorkloadArea.Properties.Resources.ApplyIcon;
            this.nccWorkloadFilterPropertyGridApplyButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.nccWorkloadFilterPropertyGridApplyButton.Location = new System.Drawing.Point(8, 32);
            this.nccWorkloadFilterPropertyGridApplyButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.nccWorkloadFilterPropertyGridApplyButton.Name = "nccWorkloadFilterPropertyGridApplyButton";
            this.nccWorkloadFilterPropertyGridApplyButton.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.nccWorkloadFilterPropertyGridApplyButton.Size = new System.Drawing.Size(80, 25);
            this.nccWorkloadFilterPropertyGridApplyButton.TabIndex = 1;
            this.nccWorkloadFilterPropertyGridApplyButton.Text = " Fi&lter";
            this.nccWorkloadFilterPropertyGridApplyButton.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.nccWorkloadFilterToolTip.SetToolTip(this.nccWorkloadFilterPropertyGridApplyButton, "Apply filter to the Triage Space (client-side filtering)");
            this.nccWorkloadFilterPropertyGridApplyButton.UseVisualStyleBackColor = true;
            this.nccWorkloadFilterPropertyGridApplyButton.Click += new System.EventHandler(this.nccFilterPropertyGridApplyButton_Click);
            // 
            // nccWorkloadFilterPropertyGridFetchButton
            // 
            this.nccWorkloadFilterPropertyGridFetchButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.nccWorkloadFilterPropertyGridFetchButton.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.nccWorkloadFilterPropertyGridFetchButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccWorkloadFilterPropertyGridFetchButton.Image = global::Trafodion.Manager.WorkloadArea.Properties.Resources.attach;
            this.nccWorkloadFilterPropertyGridFetchButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.nccWorkloadFilterPropertyGridFetchButton.Location = new System.Drawing.Point(98, 32);
            this.nccWorkloadFilterPropertyGridFetchButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.nccWorkloadFilterPropertyGridFetchButton.Name = "nccWorkloadFilterPropertyGridFetchButton";
            this.nccWorkloadFilterPropertyGridFetchButton.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.nccWorkloadFilterPropertyGridFetchButton.Size = new System.Drawing.Size(80, 25);
            this.nccWorkloadFilterPropertyGridFetchButton.TabIndex = 2;
            this.nccWorkloadFilterPropertyGridFetchButton.Text = " &Fetch";
            this.nccWorkloadFilterPropertyGridFetchButton.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.nccWorkloadFilterToolTip.SetToolTip(this.nccWorkloadFilterPropertyGridFetchButton, "Fetch and filter queries from the Trafodion platform matching the specified criteri" +
        "a");
            this.nccWorkloadFilterPropertyGridFetchButton.UseVisualStyleBackColor = true;
            this.nccWorkloadFilterPropertyGridFetchButton.Click += new System.EventHandler(this.nccFilterPropertyGridFetchButton_Click);
            // 
            // nccWorkloadFilterPropertyGridResetButton
            // 
            this.nccWorkloadFilterPropertyGridResetButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.nccWorkloadFilterPropertyGridResetButton.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.nccWorkloadFilterPropertyGridResetButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccWorkloadFilterPropertyGridResetButton.Image = global::Trafodion.Manager.WorkloadArea.Properties.Resources.cancelImage;
            this.nccWorkloadFilterPropertyGridResetButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.nccWorkloadFilterPropertyGridResetButton.Location = new System.Drawing.Point(188, 32);
            this.nccWorkloadFilterPropertyGridResetButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.nccWorkloadFilterPropertyGridResetButton.Name = "nccWorkloadFilterPropertyGridResetButton";
            this.nccWorkloadFilterPropertyGridResetButton.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.nccWorkloadFilterPropertyGridResetButton.Size = new System.Drawing.Size(80, 25);
            this.nccWorkloadFilterPropertyGridResetButton.TabIndex = 2;
            this.nccWorkloadFilterPropertyGridResetButton.Text = " &Reset";
            this.nccWorkloadFilterPropertyGridResetButton.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.nccWorkloadFilterToolTip.SetToolTip(this.nccWorkloadFilterPropertyGridResetButton, "Reset the filters on the Triage Space");
            this.nccWorkloadFilterPropertyGridResetButton.UseVisualStyleBackColor = true;
            this.nccWorkloadFilterPropertyGridResetButton.Click += new System.EventHandler(this.nccFilterPropertyGridResetButton_Click);
            // 
            // nccWorkloadFilterInfoPanel
            // 
            this.nccWorkloadFilterInfoPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.nccWorkloadFilterInfoPanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.nccWorkloadFilterInfoPanel.Controls.Add(this.filterPropertyGrid);
            this.nccWorkloadFilterInfoPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.nccWorkloadFilterInfoPanel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccWorkloadFilterInfoPanel.Location = new System.Drawing.Point(0, 61);
            this.nccWorkloadFilterInfoPanel.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.nccWorkloadFilterInfoPanel.Name = "nccWorkloadFilterInfoPanel";
            this.nccWorkloadFilterInfoPanel.Size = new System.Drawing.Size(278, 389);
            this.nccWorkloadFilterInfoPanel.TabIndex = 1;
            // 
            // filterPropertyGrid
            // 
            this.filterPropertyGrid.BackColor = System.Drawing.Color.WhiteSmoke;
            this.filterPropertyGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.filterPropertyGrid.HelpBackColor = System.Drawing.Color.WhiteSmoke;
            this.filterPropertyGrid.Location = new System.Drawing.Point(0, 0);
            this.filterPropertyGrid.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.filterPropertyGrid.Name = "filterPropertyGrid";
            this.filterPropertyGrid.Size = new System.Drawing.Size(276, 387);
            this.filterPropertyGrid.TabIndex = 0;
            this.filterPropertyGrid.PropertyValueChanged += new System.Windows.Forms.PropertyValueChangedEventHandler(this.filterPropertyGrid_PropertyValueChanged);
            // 
            // TriageFilterPropertyGrid
            // 
            this.AllowDrop = true;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.nccWorkloadFilterInfoPanel);
            this.Controls.Add(this.nccWorkloadFilterButtonsPanel);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.Name = "TriageFilterPropertyGrid";
            this.Size = new System.Drawing.Size(278, 450);
            this.DragDrop += new System.Windows.Forms.DragEventHandler(this.NCCWorkloadFilterPropertyGrid_DragDrop);
            this.DragEnter += new System.Windows.Forms.DragEventHandler(this.NCCWorkloadFilterPropertyGrid_DragEnter);
            this.nccWorkloadFilterButtonsPanel.ResumeLayout(false);
            this.nccWorkloadFilterInfoPanel.ResumeLayout(false);
            this.ResumeLayout(false);

		}

		#endregion

        private System.Windows.Forms.Panel nccWorkloadFilterButtonsPanel;
		private System.Windows.Forms.Panel nccWorkloadFilterInfoPanel;
		private System.Windows.Forms.PropertyGrid filterPropertyGrid;
		private System.Windows.Forms.Button nccWorkloadFilterPropertyGridApplyButton;
		private System.Windows.Forms.Button nccWorkloadFilterPropertyGridResetButton;
		private System.Windows.Forms.Button nccWorkloadFilterPropertyGridFetchButton;
		private System.Windows.Forms.Button nccWorkloadFilterPropertyGridSaveButton;
        private System.Windows.Forms.Button nccWorkloadFilterPropertyGridLoadButton;
		private System.Windows.Forms.ToolTip nccWorkloadFilterToolTip;

	}
}
