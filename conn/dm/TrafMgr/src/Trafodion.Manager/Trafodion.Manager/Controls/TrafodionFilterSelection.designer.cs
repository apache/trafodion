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
namespace Trafodion.Manager.Framework.Controls
{
    partial class TrafodionFilterSelection
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

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TrafodionFilterSelection));
            this._theAvailableListGroupBox = new TrafodionGroupBox();
            this.listBoxAvailable = new TrafodionListBox();
            this._theSelectedListGroupBox = new TrafodionGroupBox();
            this.listBoxSelected = new TrafodionListBox();
            this.buttonAddSelected = new TrafodionButton();
            this.buttonRemoveSelected = new TrafodionButton();
            this.buttonSelectAll = new TrafodionButton();
            this.buttonDeselectAll = new TrafodionButton();
            this.buttonOK = new TrafodionButton();
            this.buttonCancel = new TrafodionButton();
            this._TrafodionFilterSelectionToolTip = new TrafodionToolTip(this.components);
            this._theAvailableListGroupBox.SuspendLayout();
            this._theSelectedListGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theAvailableListGroupBox
            // 
            this._theAvailableListGroupBox.Controls.Add(this.listBoxAvailable);
            this._theAvailableListGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theAvailableListGroupBox.Location = new System.Drawing.Point(10, 10);
            this._theAvailableListGroupBox.Name = "_theAvailableListGroupBox";
            this._theAvailableListGroupBox.Size = new System.Drawing.Size(200, 225);
            this._theAvailableListGroupBox.TabIndex = 0;
            this._theAvailableListGroupBox.TabStop = false;
            this._theAvailableListGroupBox.Text = "Available Values";
            // 
            // listBoxAvailable
            // 
            this.listBoxAvailable.Dock = System.Windows.Forms.DockStyle.Fill;
            this.listBoxAvailable.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.listBoxAvailable.FormattingEnabled = true;
            this.listBoxAvailable.HorizontalScrollbar = true;
            this.listBoxAvailable.Location = new System.Drawing.Point(3, 17);
            this.listBoxAvailable.Name = "listBoxAvailable";
            this.listBoxAvailable.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
            this.listBoxAvailable.Size = new System.Drawing.Size(194, 205);
            this.listBoxAvailable.Sorted = true;
            this.listBoxAvailable.TabIndex = 0;
            this._TrafodionFilterSelectionToolTip.SetToolTip(this.listBoxAvailable, "Select one or more values from the list of available values.\r\nHolding down the Co" +
        "ntrol key allows you to select multiple rows.");
            this.listBoxAvailable.DoubleClick += new System.EventHandler(this.listBoxAvailable_DoubleClick);
            // 
            // _theSelectedListGroupBox
            // 
            this._theSelectedListGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._theSelectedListGroupBox.Controls.Add(this.listBoxSelected);
            this._theSelectedListGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theSelectedListGroupBox.Location = new System.Drawing.Point(380, 10);
            this._theSelectedListGroupBox.Name = "_theSelectedListGroupBox";
            this._theSelectedListGroupBox.Size = new System.Drawing.Size(200, 225);
            this._theSelectedListGroupBox.TabIndex = 1;
            this._theSelectedListGroupBox.TabStop = false;
            this._theSelectedListGroupBox.Text = "Currently Used Values";
            // 
            // listBoxSelected
            // 
            this.listBoxSelected.Dock = System.Windows.Forms.DockStyle.Fill;
            this.listBoxSelected.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.listBoxSelected.FormattingEnabled = true;
            this.listBoxSelected.HorizontalScrollbar = true;
            this.listBoxSelected.Location = new System.Drawing.Point(3, 17);
            this.listBoxSelected.Name = "listBoxSelected";
            this.listBoxSelected.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
            this.listBoxSelected.Size = new System.Drawing.Size(194, 205);
            this.listBoxSelected.Sorted = true;
            this.listBoxSelected.TabIndex = 0;
            this._TrafodionFilterSelectionToolTip.SetToolTip(this.listBoxSelected, "Select one or more values from the the InUse list. \r\nHolding down the Control key" +
        " allows you to select multiple rows.\r\n");
            this.listBoxSelected.DoubleClick += new System.EventHandler(this.listBoxSelected_DoubleClick);
            // 
            // buttonAddSelected
            // 
            this.buttonAddSelected.BackColor = System.Drawing.Color.AliceBlue;
            this.buttonAddSelected.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("buttonAddSelected.BackgroundImage")));
            this.buttonAddSelected.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.buttonAddSelected.Enabled = false;
            this.buttonAddSelected.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonAddSelected.ForeColor = System.Drawing.SystemColors.ControlText;
            this.buttonAddSelected.Location = new System.Drawing.Point(230, 40);
            this.buttonAddSelected.Name = "buttonAddSelected";
            this.buttonAddSelected.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.buttonAddSelected.Size = new System.Drawing.Size(125, 25);
            this.buttonAddSelected.TabIndex = 2;
            this.buttonAddSelected.Text = "A&dd Selected";
            this._TrafodionFilterSelectionToolTip.SetToolTip(this.buttonAddSelected, "Add the selected values from the Available section");
            this.buttonAddSelected.UseVisualStyleBackColor = true;
            this.buttonAddSelected.Click += new System.EventHandler(this.buttonAddSelected_Click);
            // 
            // buttonRemoveSelected
            // 
            this.buttonRemoveSelected.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("buttonRemoveSelected.BackgroundImage")));
            this.buttonRemoveSelected.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.buttonRemoveSelected.Enabled = false;
            this.buttonRemoveSelected.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonRemoveSelected.ForeColor = System.Drawing.Color.Red;
            this.buttonRemoveSelected.Location = new System.Drawing.Point(230, 80);
            this.buttonRemoveSelected.Name = "buttonRemoveSelected";
            this.buttonRemoveSelected.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.buttonRemoveSelected.Size = new System.Drawing.Size(125, 25);
            this.buttonRemoveSelected.TabIndex = 3;
            this.buttonRemoveSelected.Text = "    R&emove Selected";
            this._TrafodionFilterSelectionToolTip.SetToolTip(this.buttonRemoveSelected, "Remove the selected values from the list of values \r\nto be used");
            this.buttonRemoveSelected.UseVisualStyleBackColor = true;
            this.buttonRemoveSelected.Click += new System.EventHandler(this.buttonRemoveSelected_Click);
            // 
            // buttonSelectAll
            // 
            this.buttonSelectAll.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("buttonSelectAll.BackgroundImage")));
            this.buttonSelectAll.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.buttonSelectAll.Enabled = false;
            this.buttonSelectAll.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonSelectAll.ForeColor = System.Drawing.SystemColors.ControlText;
            this.buttonSelectAll.Location = new System.Drawing.Point(230, 135);
            this.buttonSelectAll.Name = "buttonSelectAll";
            this.buttonSelectAll.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.buttonSelectAll.Size = new System.Drawing.Size(125, 25);
            this.buttonSelectAll.TabIndex = 4;
            this.buttonSelectAll.Text = " &Add All Values";
            this._TrafodionFilterSelectionToolTip.SetToolTip(this.buttonSelectAll, "Select all the values from the Available section");
            this.buttonSelectAll.UseVisualStyleBackColor = true;
            this.buttonSelectAll.Click += new System.EventHandler(this.buttonSelectAll_Click);
            // 
            // buttonDeselectAll
            // 
            this.buttonDeselectAll.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("buttonDeselectAll.BackgroundImage")));
            this.buttonDeselectAll.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.buttonDeselectAll.Enabled = false;
            this.buttonDeselectAll.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonDeselectAll.ForeColor = System.Drawing.Color.Red;
            this.buttonDeselectAll.Location = new System.Drawing.Point(230, 175);
            this.buttonDeselectAll.Name = "buttonDeselectAll";
            this.buttonDeselectAll.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.buttonDeselectAll.Size = new System.Drawing.Size(125, 25);
            this.buttonDeselectAll.TabIndex = 5;
            this.buttonDeselectAll.Text = "&Remove All";
            this._TrafodionFilterSelectionToolTip.SetToolTip(this.buttonDeselectAll, "Deselect all the values from the list of values to be used");
            this.buttonDeselectAll.UseVisualStyleBackColor = true;
            this.buttonDeselectAll.Click += new System.EventHandler(this.buttonDeselectAll_Click);
            // 
            // buttonOK
            // 
            this.buttonOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.buttonOK.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonOK.Image = ((System.Drawing.Image)(resources.GetObject("buttonOK.Image")));
            this.buttonOK.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.buttonOK.Location = new System.Drawing.Point(370, 255);
            this.buttonOK.Name = "buttonOK";
            this.buttonOK.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.buttonOK.Size = new System.Drawing.Size(80, 25);
            this.buttonOK.TabIndex = 6;
            this.buttonOK.Text = " &OK";
            this._TrafodionFilterSelectionToolTip.SetToolTip(this.buttonOK, "Accept and use the selected values in the filter");
            this.buttonOK.UseVisualStyleBackColor = true;
            this.buttonOK.Click += new System.EventHandler(this.buttonOK_Click);
            // 
            // buttonCancel
            // 
            this.buttonCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.buttonCancel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonCancel.Image = ((System.Drawing.Image)(resources.GetObject("buttonCancel.Image")));
            this.buttonCancel.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.buttonCancel.Location = new System.Drawing.Point(475, 255);
            this.buttonCancel.Name = "buttonCancel";
            this.buttonCancel.Size = new System.Drawing.Size(90, 25);
            this.buttonCancel.TabIndex = 7;
            this.buttonCancel.Text = "    &Cancel";
            this._TrafodionFilterSelectionToolTip.SetToolTip(this.buttonCancel, "Discard all changes");
            this.buttonCancel.UseVisualStyleBackColor = true;
            this.buttonCancel.Click += new System.EventHandler(this.buttonCancel_Click);
            // 
            // TrafodionFilterSelection
            // 
            this.AcceptButton = this.buttonOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.CancelButton = this.buttonCancel;
            this.ClientSize = new System.Drawing.Size(594, 298);
            this.Controls.Add(this.buttonCancel);
            this.Controls.Add(this.buttonOK);
            this.Controls.Add(this.buttonDeselectAll);
            this.Controls.Add(this.buttonSelectAll);
            this.Controls.Add(this.buttonRemoveSelected);
            this.Controls.Add(this.buttonAddSelected);
            this.Controls.Add(this._theSelectedListGroupBox);
            this.Controls.Add(this._theAvailableListGroupBox);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "TrafodionFilterSelection";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Trafodion Database Manager - Filter Selection";
            this._theAvailableListGroupBox.ResumeLayout(false);
            this._theSelectedListGroupBox.ResumeLayout(false);
            this.ResumeLayout(false);

		}

		#endregion

		private TrafodionGroupBox _theAvailableListGroupBox;
        private TrafodionListBox listBoxAvailable;
        private TrafodionGroupBox _theSelectedListGroupBox;
        private TrafodionListBox listBoxSelected;
        private TrafodionButton buttonAddSelected;
        private TrafodionButton buttonRemoveSelected;
        private TrafodionButton buttonSelectAll;
        private TrafodionButton buttonDeselectAll;
        private TrafodionButton buttonOK;
        private TrafodionButton buttonCancel;
        private TrafodionToolTip _TrafodionFilterSelectionToolTip;
	}
}