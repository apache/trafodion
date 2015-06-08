// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
//

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// NCCCQSettingControl
    /// </summary>
    partial class ConfigAttributeSettingControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ConfigAttributeSettingControl));
            this.iGCellStyleAttributeNameCombo = new TenTec.Windows.iGridLib.iGCellStyleDesign();
            this.iGAttributeNameDropdownList = new TenTec.Windows.iGridLib.iGDropDownList();
            this.iGCellStyleText = new TenTec.Windows.iGridLib.iGCellStyleDesign();
            this.iGColHdrStyleDesign1 = new TenTec.Windows.iGridLib.iGColHdrStyleDesign();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.helpProvider1 = new System.Windows.Forms.HelpProvider();
            this._mainPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.cfSettingsGrid = new TenTec.Windows.iGridLib.iGrid();
            this._toolStripPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionToolStrip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._theAddStatementStripButton = new System.Windows.Forms.ToolStripButton();
            this._theDeleteStatementStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this._theRemoveAllStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this._mainPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.cfSettingsGrid)).BeginInit();
            this._toolStripPanel.SuspendLayout();
            this.TrafodionToolStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // iGCellStyleAttributeNameCombo
            // 
            this.iGCellStyleAttributeNameCombo.DropDownControl = this.iGAttributeNameDropdownList;
            this.iGCellStyleAttributeNameCombo.EmptyStringAs = TenTec.Windows.iGridLib.iGEmptyStringAs.Null;
            this.iGCellStyleAttributeNameCombo.Enabled = TenTec.Windows.iGridLib.iGBool.True;
            this.iGCellStyleAttributeNameCombo.Flags = ((TenTec.Windows.iGridLib.iGCellFlags)((TenTec.Windows.iGridLib.iGCellFlags.DisplayText | TenTec.Windows.iGridLib.iGCellFlags.DisplayImage)));
            this.iGCellStyleAttributeNameCombo.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.iGCellStyleAttributeNameCombo.ForeColor = System.Drawing.Color.Black;
            this.iGCellStyleAttributeNameCombo.ImageAlign = TenTec.Windows.iGridLib.iGContentAlignment.TopLeft;
            this.iGCellStyleAttributeNameCombo.ReadOnly = TenTec.Windows.iGridLib.iGBool.False;
            this.iGCellStyleAttributeNameCombo.Selectable = TenTec.Windows.iGridLib.iGBool.True;
            this.iGCellStyleAttributeNameCombo.SingleClickEdit = TenTec.Windows.iGridLib.iGBool.False;
            this.iGCellStyleAttributeNameCombo.TextAlign = TenTec.Windows.iGridLib.iGContentAlignment.TopLeft;
            this.iGCellStyleAttributeNameCombo.TextPosToImage = TenTec.Windows.iGridLib.iGTextPosToImage.Horizontally;
            this.iGCellStyleAttributeNameCombo.TextTrimming = TenTec.Windows.iGridLib.iGStringTrimming.EllipsisCharacter;
            // 
            // iGAttributeNameDropdownList
            // 
            this.iGAttributeNameDropdownList.BackColor = System.Drawing.Color.Empty;
            this.iGAttributeNameDropdownList.ForeColor = System.Drawing.Color.Empty;
            this.iGAttributeNameDropdownList.SelItemBackColor = System.Drawing.Color.Empty;
            this.iGAttributeNameDropdownList.SelItemForeColor = System.Drawing.Color.Empty;
            // 
            // iGCellStyleText
            // 
            this.iGCellStyleText.EmptyStringAs = TenTec.Windows.iGridLib.iGEmptyStringAs.Null;
            this.iGCellStyleText.Enabled = TenTec.Windows.iGridLib.iGBool.True;
            this.iGCellStyleText.Flags = ((TenTec.Windows.iGridLib.iGCellFlags)((TenTec.Windows.iGridLib.iGCellFlags.DisplayText | TenTec.Windows.iGridLib.iGCellFlags.DisplayImage)));
            this.iGCellStyleText.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.iGCellStyleText.ForeColor = System.Drawing.Color.Black;
            this.iGCellStyleText.ImageAlign = TenTec.Windows.iGridLib.iGContentAlignment.TopLeft;
            this.iGCellStyleText.ReadOnly = TenTec.Windows.iGridLib.iGBool.False;
            this.iGCellStyleText.Selectable = TenTec.Windows.iGridLib.iGBool.True;
            this.iGCellStyleText.SingleClickEdit = TenTec.Windows.iGridLib.iGBool.True;
            this.iGCellStyleText.TextAlign = TenTec.Windows.iGridLib.iGContentAlignment.TopLeft;
            this.iGCellStyleText.TextPosToImage = TenTec.Windows.iGridLib.iGTextPosToImage.Horizontally;
            this.iGCellStyleText.TextTrimming = TenTec.Windows.iGridLib.iGStringTrimming.EllipsisCharacter;
            this.iGCellStyleText.Type = TenTec.Windows.iGridLib.iGCellType.Text;
            // 
            // helpProvider1
            // 
            this.helpProvider1.HelpNamespace = "CtrlSettings.chm";
            // 
            // _mainPanel
            // 
            this._mainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._mainPanel.Controls.Add(this.cfSettingsGrid);
            this._mainPanel.Controls.Add(this._toolStripPanel);
            this._mainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._mainPanel.Location = new System.Drawing.Point(0, 0);
            this._mainPanel.Name = "_mainPanel";
            this._mainPanel.Size = new System.Drawing.Size(445, 269);
            this._mainPanel.TabIndex = 13;
            // 
            // cfSettingsGrid
            // 
            this.cfSettingsGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.cfSettingsGrid.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.cfSettingsGrid.ForeColor = System.Drawing.SystemColors.ControlText;
            this.cfSettingsGrid.Header.Height = 20;
            this.helpProvider1.SetHelpKeyword(this.cfSettingsGrid, "CtrlSettings.html");
            this.helpProvider1.SetHelpNavigator(this.cfSettingsGrid, System.Windows.Forms.HelpNavigator.Topic);
            this.cfSettingsGrid.Location = new System.Drawing.Point(0, 28);
            this.cfSettingsGrid.Name = "cfSettingsGrid";
            this.cfSettingsGrid.RowMode = true;
            this.cfSettingsGrid.RowModeHasCurCell = true;
            this.cfSettingsGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            this.helpProvider1.SetShowHelp(this.cfSettingsGrid, false);
            this.cfSettingsGrid.Size = new System.Drawing.Size(445, 241);
            this.cfSettingsGrid.TabIndex = 1;
            this.cfSettingsGrid.TreeCol = null;
            this.cfSettingsGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.cfSettingsGrid.SelectionChanged += new System.EventHandler(this.cqSettingsGrid_SelectionChanged);
            this.cfSettingsGrid.CellClick += new TenTec.Windows.iGridLib.iGCellClickEventHandler(this.cqSettingsGrid_CellClick);
            this.cfSettingsGrid.Click += new System.EventHandler(this.cqSettingsGrid_Click);
            // 
            // _toolStripPanel
            // 
            this._toolStripPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._toolStripPanel.Controls.Add(this.TrafodionToolStrip1);
            this._toolStripPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._toolStripPanel.Location = new System.Drawing.Point(0, 0);
            this._toolStripPanel.Name = "_toolStripPanel";
            this._toolStripPanel.Size = new System.Drawing.Size(445, 28);
            this._toolStripPanel.TabIndex = 13;
            // 
            // TrafodionToolStrip1
            // 
            this.TrafodionToolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._theAddStatementStripButton,
            this._theDeleteStatementStripButton,
            this.toolStripSeparator1,
            this._theRemoveAllStripButton,
            this.toolStripSeparator2});
            this.TrafodionToolStrip1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionToolStrip1.Name = "TrafodionToolStrip1";
            this.TrafodionToolStrip1.Size = new System.Drawing.Size(445, 25);
            this.TrafodionToolStrip1.TabIndex = 0;
            this.TrafodionToolStrip1.Text = "TrafodionToolStrip1";
            // 
            // _theAddStatementStripButton
            // 
            this._theAddStatementStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theAddStatementStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theAddStatementStripButton.Image")));
            this._theAddStatementStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theAddStatementStripButton.Name = "_theAddStatementStripButton";
            this._theAddStatementStripButton.Size = new System.Drawing.Size(23, 22);
            this._theAddStatementStripButton.Text = "Add";
            this._theAddStatementStripButton.ToolTipText = "Add a new attribute";
            this._theAddStatementStripButton.Click += new System.EventHandler(this.addRowCQGridBtn_Click);
            // 
            // _theDeleteStatementStripButton
            // 
            this._theDeleteStatementStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theDeleteStatementStripButton.Enabled = false;
            this._theDeleteStatementStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theDeleteStatementStripButton.Image")));
            this._theDeleteStatementStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theDeleteStatementStripButton.Name = "_theDeleteStatementStripButton";
            this._theDeleteStatementStripButton.Size = new System.Drawing.Size(23, 22);
            this._theDeleteStatementStripButton.Text = "Remove";
            this._theDeleteStatementStripButton.ToolTipText = "Remove the selected attribute";
            this._theDeleteStatementStripButton.Click += new System.EventHandler(this.removeCQRowBtn_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
            // 
            // _theRemoveAllStripButton
            // 
            this._theRemoveAllStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theRemoveAllStripButton.Enabled = false;
            this._theRemoveAllStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theRemoveAllStripButton.Image")));
            this._theRemoveAllStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theRemoveAllStripButton.Name = "_theRemoveAllStripButton";
            this._theRemoveAllStripButton.Size = new System.Drawing.Size(23, 22);
            this._theRemoveAllStripButton.Text = "Remove All";
            this._theRemoveAllStripButton.ToolTipText = "Remove all attribute rows";
            this._theRemoveAllStripButton.Click += new System.EventHandler(this.clearCQgridbtn_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(6, 25);
            // 
            // ConfigAttributeSettingControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._mainPanel);
            this.helpProvider1.SetHelpKeyword(this, "CtrlSettings.html");
            this.helpProvider1.SetHelpNavigator(this, System.Windows.Forms.HelpNavigator.Topic);
            this.Name = "ConfigAttributeSettingControl";
            this.helpProvider1.SetShowHelp(this, true);
            this.Size = new System.Drawing.Size(445, 269);
            this._mainPanel.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.cfSettingsGrid)).EndInit();
            this._toolStripPanel.ResumeLayout(false);
            this._toolStripPanel.PerformLayout();
            this.TrafodionToolStrip1.ResumeLayout(false);
            this.TrafodionToolStrip1.PerformLayout();
            this.ResumeLayout(false);

		}

		#endregion

		private TenTec.Windows.iGridLib.iGrid cfSettingsGrid;
		private TenTec.Windows.iGridLib.iGCellStyleDesign iGCellStyleAttributeNameCombo;
        private TenTec.Windows.iGridLib.iGDropDownList iGAttributeNameDropdownList;
        private TenTec.Windows.iGridLib.iGCellStyleDesign iGCellStyleText;
        private TenTec.Windows.iGridLib.iGColHdrStyleDesign iGColHdrStyleDesign1;
		private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.HelpProvider helpProvider1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _mainPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _toolStripPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStrip TrafodionToolStrip1;
        private System.Windows.Forms.ToolStripButton _theAddStatementStripButton;
        private System.Windows.Forms.ToolStripButton _theDeleteStatementStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripButton _theRemoveAllStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
	}
}
