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

namespace Trafodion.Manager.DatabaseArea.NCC
{
    /// <summary>
    /// NCCCQSettingControl
    /// </summary>
	partial class NCCCQSettingControl
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
            TenTec.Windows.iGridLib.iGColPattern iGColPattern1 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern2 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern3 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern4 = new TenTec.Windows.iGridLib.iGColPattern();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(NCCCQSettingControl));
            this.iGCellStyleCheckBox = new TenTec.Windows.iGridLib.iGCellStyleDesign();
            this.iGCellStyleControlCombo = new TenTec.Windows.iGridLib.iGCellStyleDesign();
            this.iGControlDropdownList = new TenTec.Windows.iGridLib.iGDropDownList();
            this.iGCellStyleCQDCombo = new TenTec.Windows.iGridLib.iGCellStyleDesign();
            this.iGCQDDropdownList = new TenTec.Windows.iGridLib.iGDropDownList();
            this.iGColHdrStyleDesign1 = new TenTec.Windows.iGridLib.iGColHdrStyleDesign();
            this.iGCellStyleText = new TenTec.Windows.iGridLib.iGCellStyleDesign();
            this.cqSettingsGrid = new TenTec.Windows.iGridLib.iGrid();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.helpProvider1 = new System.Windows.Forms.HelpProvider();
            this._mainPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._toolStripPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionToolStrip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._theAddStatementStripButton = new System.Windows.Forms.ToolStripButton();
            this._theDeleteStatementStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this._theHelpStripButton = new System.Windows.Forms.ToolStripButton();
            this._theRemoveAllStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.toolStripLabel1 = new System.Windows.Forms.ToolStripLabel();
            this._theCQSettingStats = new System.Windows.Forms.ToolStripLabel();
            ((System.ComponentModel.ISupportInitialize)(this.cqSettingsGrid)).BeginInit();
            this._mainPanel.SuspendLayout();
            this._toolStripPanel.SuspendLayout();
            this.TrafodionToolStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // iGCellStyleCheckBox
            // 
            this.iGCellStyleCheckBox.EmptyStringAs = TenTec.Windows.iGridLib.iGEmptyStringAs.Null;
            this.iGCellStyleCheckBox.Enabled = TenTec.Windows.iGridLib.iGBool.True;
            this.iGCellStyleCheckBox.Flags = ((TenTec.Windows.iGridLib.iGCellFlags)((TenTec.Windows.iGridLib.iGCellFlags.DisplayText | TenTec.Windows.iGridLib.iGCellFlags.DisplayImage)));
            this.iGCellStyleCheckBox.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.iGCellStyleCheckBox.ForeColor = System.Drawing.Color.Black;
            this.iGCellStyleCheckBox.ImageAlign = TenTec.Windows.iGridLib.iGContentAlignment.TopCenter;
            this.iGCellStyleCheckBox.ReadOnly = TenTec.Windows.iGridLib.iGBool.False;
            this.iGCellStyleCheckBox.Selectable = TenTec.Windows.iGridLib.iGBool.True;
            this.iGCellStyleCheckBox.SingleClickEdit = TenTec.Windows.iGridLib.iGBool.True;
            this.iGCellStyleCheckBox.TextAlign = TenTec.Windows.iGridLib.iGContentAlignment.TopLeft;
            this.iGCellStyleCheckBox.TextPosToImage = TenTec.Windows.iGridLib.iGTextPosToImage.Horizontally;
            this.iGCellStyleCheckBox.TextTrimming = TenTec.Windows.iGridLib.iGStringTrimming.EllipsisCharacter;
            this.iGCellStyleCheckBox.Type = TenTec.Windows.iGridLib.iGCellType.Check;
            // 
            // iGCellStyleControlCombo
            // 
            this.iGCellStyleControlCombo.DropDownControl = this.iGControlDropdownList;
            this.iGCellStyleControlCombo.EmptyStringAs = TenTec.Windows.iGridLib.iGEmptyStringAs.Null;
            this.iGCellStyleControlCombo.Enabled = TenTec.Windows.iGridLib.iGBool.True;
            this.iGCellStyleControlCombo.Flags = ((TenTec.Windows.iGridLib.iGCellFlags)((TenTec.Windows.iGridLib.iGCellFlags.DisplayText | TenTec.Windows.iGridLib.iGCellFlags.DisplayImage)));
            this.iGCellStyleControlCombo.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.iGCellStyleControlCombo.ForeColor = System.Drawing.Color.Black;
            this.iGCellStyleControlCombo.ImageAlign = TenTec.Windows.iGridLib.iGContentAlignment.TopLeft;
            this.iGCellStyleControlCombo.ReadOnly = TenTec.Windows.iGridLib.iGBool.False;
            this.iGCellStyleControlCombo.Selectable = TenTec.Windows.iGridLib.iGBool.True;
            this.iGCellStyleControlCombo.SingleClickEdit = TenTec.Windows.iGridLib.iGBool.True;
            this.iGCellStyleControlCombo.TextAlign = TenTec.Windows.iGridLib.iGContentAlignment.TopLeft;
            this.iGCellStyleControlCombo.TextPosToImage = TenTec.Windows.iGridLib.iGTextPosToImage.Horizontally;
            this.iGCellStyleControlCombo.TextTrimming = TenTec.Windows.iGridLib.iGStringTrimming.EllipsisCharacter;
            // 
            // iGControlDropdownList
            // 
            this.iGControlDropdownList.BackColor = System.Drawing.Color.Empty;
            this.iGControlDropdownList.ForeColor = System.Drawing.Color.Empty;
            this.iGControlDropdownList.SelItemBackColor = System.Drawing.Color.Empty;
            this.iGControlDropdownList.SelItemForeColor = System.Drawing.Color.Empty;
            this.iGControlDropdownList.SelectedItemChanged += new TenTec.Windows.iGridLib.iGSelectedItemChangedEventHandler(this.iGControlDropdownList_SelectedItemChanged);
            // 
            // iGCellStyleCQDCombo
            // 
            this.iGCellStyleCQDCombo.DropDownControl = this.iGCQDDropdownList;
            this.iGCellStyleCQDCombo.EmptyStringAs = TenTec.Windows.iGridLib.iGEmptyStringAs.Null;
            this.iGCellStyleCQDCombo.Enabled = TenTec.Windows.iGridLib.iGBool.True;
            this.iGCellStyleCQDCombo.Flags = ((TenTec.Windows.iGridLib.iGCellFlags)((TenTec.Windows.iGridLib.iGCellFlags.DisplayText | TenTec.Windows.iGridLib.iGCellFlags.DisplayImage)));
            this.iGCellStyleCQDCombo.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.iGCellStyleCQDCombo.ForeColor = System.Drawing.Color.Black;
            this.iGCellStyleCQDCombo.ImageAlign = TenTec.Windows.iGridLib.iGContentAlignment.TopLeft;
            this.iGCellStyleCQDCombo.ReadOnly = TenTec.Windows.iGridLib.iGBool.False;
            this.iGCellStyleCQDCombo.Selectable = TenTec.Windows.iGridLib.iGBool.True;
            this.iGCellStyleCQDCombo.SingleClickEdit = TenTec.Windows.iGridLib.iGBool.True;
            this.iGCellStyleCQDCombo.TextAlign = TenTec.Windows.iGridLib.iGContentAlignment.TopLeft;
            this.iGCellStyleCQDCombo.TextPosToImage = TenTec.Windows.iGridLib.iGTextPosToImage.Horizontally;
            this.iGCellStyleCQDCombo.TextTrimming = TenTec.Windows.iGridLib.iGStringTrimming.EllipsisCharacter;
            // 
            // iGCQDDropdownList
            // 
            this.iGCQDDropdownList.BackColor = System.Drawing.Color.Empty;
            this.iGCQDDropdownList.ForeColor = System.Drawing.Color.Empty;
            this.iGCQDDropdownList.MaxVisibleRowCount = 20;
            this.iGCQDDropdownList.SelItemBackColor = System.Drawing.Color.Empty;
            this.iGCQDDropdownList.SelItemForeColor = System.Drawing.Color.Empty;
            this.iGCQDDropdownList.SelectedItemChanged += new TenTec.Windows.iGridLib.iGSelectedItemChangedEventHandler(this.iGCQDDropdownList_SelectedItemChanged);
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
            // cqSettingsGrid
            // 
            iGColPattern1.CellStyle = this.iGCellStyleCheckBox;
            iGColPattern1.Key = "Disable";
            iGColPattern1.Text = "Disable";
            iGColPattern1.Width = 55;
            iGColPattern2.CellStyle = this.iGCellStyleControlCombo;
            iGColPattern2.Key = "ControlType";
            iGColPattern2.Text = "Control Statement";
            iGColPattern2.Width = 183;
            iGColPattern3.CellStyle = this.iGCellStyleCQDCombo;
            iGColPattern3.ColHdrStyle = this.iGColHdrStyleDesign1;
            iGColPattern3.Key = "AttributeString";
            iGColPattern3.Text = "Attribute String";
            iGColPattern3.Width = 254;
            iGColPattern4.CellStyle = this.iGCellStyleText;
            iGColPattern4.Key = "CQDValue";
            iGColPattern4.Text = "CQD Value";
            iGColPattern4.Width = 100;
            this.cqSettingsGrid.Cols.AddRange(new TenTec.Windows.iGridLib.iGColPattern[] {
            iGColPattern1,
            iGColPattern2,
            iGColPattern3,
            iGColPattern4});
            this.cqSettingsGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.cqSettingsGrid.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.cqSettingsGrid.ForeColor = System.Drawing.SystemColors.ControlText;
            this.cqSettingsGrid.Header.Height = 20;
            this.helpProvider1.SetHelpKeyword(this.cqSettingsGrid, "CtrlSettings.html");
            this.helpProvider1.SetHelpNavigator(this.cqSettingsGrid, System.Windows.Forms.HelpNavigator.Topic);
            this.cqSettingsGrid.Location = new System.Drawing.Point(0, 28);
            this.cqSettingsGrid.Name = "cqSettingsGrid";
            this.cqSettingsGrid.RowMode = true;
            this.cqSettingsGrid.RowModeHasCurCell = true;
            this.cqSettingsGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            this.helpProvider1.SetShowHelp(this.cqSettingsGrid, false);
            this.cqSettingsGrid.Size = new System.Drawing.Size(445, 241);
            this.cqSettingsGrid.TabIndex = 1;
            this.cqSettingsGrid.TreeCol = null;
            this.cqSettingsGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.cqSettingsGrid.CellClick += new TenTec.Windows.iGridLib.iGCellClickEventHandler(this.cqSettingsGrid_CellClick);
            this.cqSettingsGrid.SelectionChanged += new System.EventHandler(this.cqSettingsGrid_SelectionChanged);
            this.cqSettingsGrid.AfterCommitEdit += new TenTec.Windows.iGridLib.iGAfterCommitEditEventHandler(this.cqSettingsGrid_AfterCommitEdit);
            this.cqSettingsGrid.Click += new System.EventHandler(this.cqSettingsGrid_Click);
            // 
            // helpProvider1
            // 
            this.helpProvider1.HelpNamespace = "CtrlSettings.chm";
            // 
            // _mainPanel
            // 
            this._mainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._mainPanel.Controls.Add(this.cqSettingsGrid);
            this._mainPanel.Controls.Add(this._toolStripPanel);
            this._mainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._mainPanel.Location = new System.Drawing.Point(0, 0);
            this._mainPanel.Name = "_mainPanel";
            this._mainPanel.Size = new System.Drawing.Size(445, 269);
            this._mainPanel.TabIndex = 13;
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
            this._theHelpStripButton,
            this._theRemoveAllStripButton,
            this.toolStripSeparator2,
            this.toolStripLabel1,
            this._theCQSettingStats});
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
            this._theAddStatementStripButton.ToolTipText = "Add a new control statement";
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
            this._theDeleteStatementStripButton.ToolTipText = "Remove the selected control statements";
            this._theDeleteStatementStripButton.Click += new System.EventHandler(this.removeCQRowBtn_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
            // 
            // _theHelpStripButton
            // 
            this._theHelpStripButton.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this._theHelpStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theHelpStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theHelpStripButton.Image")));
            this._theHelpStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theHelpStripButton.Name = "_theHelpStripButton";
            this._theHelpStripButton.Size = new System.Drawing.Size(23, 22);
            this._theHelpStripButton.Text = "Help";
            this._theHelpStripButton.Click += new System.EventHandler(this._theHelpStripButton_Click);
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
            this._theRemoveAllStripButton.ToolTipText = "Remove all control statements";
            this._theRemoveAllStripButton.Click += new System.EventHandler(this.clearCQgridbtn_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(6, 25);
            // 
            // toolStripLabel1
            // 
            this.toolStripLabel1.AutoSize = false;
            this.toolStripLabel1.Name = "toolStripLabel1";
            this.toolStripLabel1.Overflow = System.Windows.Forms.ToolStripItemOverflow.Never;
            this.toolStripLabel1.Size = new System.Drawing.Size(50, 22);
            // 
            // _theCQSettingStats
            // 
            this._theCQSettingStats.AutoSize = false;
            this._theCQSettingStats.BackColor = System.Drawing.Color.AliceBlue;
            this._theCQSettingStats.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theCQSettingStats.ForeColor = System.Drawing.SystemColors.ControlText;
            this._theCQSettingStats.Name = "_theCQSettingStats";
            this._theCQSettingStats.Size = new System.Drawing.Size(250, 22);
            // 
            // NCCCQSettingControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._mainPanel);
            this.helpProvider1.SetHelpKeyword(this, "CtrlSettings.html");
            this.helpProvider1.SetHelpNavigator(this, System.Windows.Forms.HelpNavigator.Topic);
            this.Name = "NCCCQSettingControl";
            this.helpProvider1.SetShowHelp(this, true);
            this.Size = new System.Drawing.Size(445, 269);
            ((System.ComponentModel.ISupportInitialize)(this.cqSettingsGrid)).EndInit();
            this._mainPanel.ResumeLayout(false);
            this._toolStripPanel.ResumeLayout(false);
            this._toolStripPanel.PerformLayout();
            this.TrafodionToolStrip1.ResumeLayout(false);
            this.TrafodionToolStrip1.PerformLayout();
            this.ResumeLayout(false);

		}

		#endregion

		private TenTec.Windows.iGridLib.iGrid cqSettingsGrid;
		private TenTec.Windows.iGridLib.iGCellStyleDesign iGCellStyleControlCombo;
		private TenTec.Windows.iGridLib.iGDropDownList iGControlDropdownList;
		private TenTec.Windows.iGridLib.iGCellStyleDesign iGCellStyleCheckBox;
		private TenTec.Windows.iGridLib.iGCellStyleDesign iGCellStyleCQDCombo;
		private TenTec.Windows.iGridLib.iGDropDownList iGCQDDropdownList;
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
        private System.Windows.Forms.ToolStripButton _theHelpStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripLabel toolStripLabel1;
        private System.Windows.Forms.ToolStripLabel _theCQSettingStats;
	}
}
