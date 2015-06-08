//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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

using System;
using System.Data;
using System.Drawing;
using System.Windows.Forms;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// for config attribute control
    /// </summary>
	public partial class ConfigAttributeSettingControl : UserControl
    {

        #region Fields
        public delegate void OnRowsChangedHandle();
        public event OnRowsChangedHandle OnRowsChangedImpl;
		private EventHandler _onClickHandler = null;
        private int _iCurrentEditCellColIndex = -1;
        private int _iCurrentEditCellRowIndex = -1;
        private string _sCellText = "";
        const String TRACE_SUB_AREA_NAME = "Attribute Values";
        TenTec.Windows.iGridLib.iGColPattern iGColPattern1;
        TenTec.Windows.iGridLib.iGColPattern iGColPattern2;

        #endregion Fields

        #region Constructor

        /// <summary>
        /// Constructor
        /// </summary>
        public ConfigAttributeSettingControl()
		{
			InitializeComponent();
            Setup();
        }

        #endregion Constructor

        #region Public Methods

        /// <summary>
        ///Handlers and functionality to help to gaurantee
        ///CQSettings widget gets focus when it's controls items
        ///are clicked
        /// </summary>
		public EventHandler OnClickHandler
		{
			get { return _onClickHandler; }
			set { _onClickHandler = value; }
		}

        /// <summary>
        /// Clear the control contents
        /// </summary>
		public void Clear()
		{
			cfSettingsGrid.Rows.Clear();
			cfSettingsGrid.Focus();
            _theDeleteStatementStripButton.Enabled = false;
            _theRemoveAllStripButton.Enabled = false;
            _iCurrentEditCellRowIndex = _iCurrentEditCellColIndex = -1;
		}

        /// <summary>
        /// Set up AttributeName list
        /// </summary>
        private void Setup()
        {
            iGCellStyleAttributeNameCombo.Type = iGCellType.Text;
            iGColPattern1 = new TenTec.Windows.iGridLib.iGColPattern();
            iGColPattern2 = new TenTec.Windows.iGridLib.iGColPattern();
            iGColPattern1.CellStyle = this.iGCellStyleAttributeNameCombo;
            iGColPattern1.Key = "Name";
            iGColPattern1.Text = "Name";
            iGColPattern1.Width = 140;
            iGColPattern1.CellStyle.TypeFlags = iGCellTypeFlags.NoTextEdit;

            iGColPattern2.CellStyle = this.iGCellStyleText;
            iGColPattern2.Key = "Value";
            iGColPattern2.Text = "Value";
            iGColPattern2.Width = 350;

            cfSettingsGrid.AutoResizeCols = true;
            //set header text bold
            cfSettingsGrid.DefaultCol.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);

            this.cfSettingsGrid.Cols.AddRange(new TenTec.Windows.iGridLib.iGColPattern[] {
            iGColPattern1,
            iGColPattern2});

            this.iGAttributeNameDropdownList.Items.Clear();
            this.iGAttributeNameDropdownList.Items.AddRange(
                    new TenTec.Windows.iGridLib.iGDropDownListItem[] {
                    //new iGDropDownListItem("DirectoryBase", "DirectoryBase"),
                    new iGDropDownListItem("UniqueIdentifier", "UniqueIdentifier")
                    
                });
            cfSettingsGrid.AfterCommitEdit += new iGAfterCommitEditEventHandler(cfSettingsGrid_AfterCommitEdit);
            cfSettingsGrid.TextBoxTextChanged += new iGTextBoxTextChangedEventHandler(cfSettingsGrid_TextBoxTextChanged);
        }

        private void cfSettingsGrid_AfterCommitEdit(object sender, iGAfterCommitEditEventArgs e)
        {
            if (OnRowsChangedImpl != null)
            {
                OnRowsChangedImpl();
            }
        }

        private void cfSettingsGrid_TextBoxTextChanged(object sender, iGTextBoxTextChangedEventArgs e)
        {
            cfSettingsGrid.CurCell.ValueType = Type.GetType("System.String");
            this._sCellText = cfSettingsGrid.TextBox.Text;
            this._iCurrentEditCellColIndex = cfSettingsGrid.CurCell.ColIndex;
            this._iCurrentEditCellRowIndex = cfSettingsGrid.CurCell.RowIndex;
        }

        public void populateGrid(DataTable dt)
        {
            foreach (iGCol column in cfSettingsGrid.Cols)
            {
                if (column.Key.Equals("Name"))
                    column.CellStyle.SingleClickEdit = iGBool.False;
            }

            //fill data
            foreach (DataRow row in dt.Rows)
            {
                iGRow igrow = cfSettingsGrid.Rows.Add();
                for (int i = 0; i < dt.Columns.Count; i++)
                {
                    igrow.Cells[i].Value = row[i];
                }
            }

            //set width
            cfSettingsGrid.Cols[0].MinWidth = 140;
            //cfSettingsGrid.Cols[1].MinWidth = 343;
            cfSettingsGrid.Cols[0].AutoWidth(false);
            cfSettingsGrid.Cols[1].AutoWidth(false);

            UpdateToolStrip();
        }

        public iGRowCollection getGridData()
        {
            return this.cfSettingsGrid.Rows;
        }

        #endregion Public Methods

        #region Private methods

        /// <summary>
        /// setAsActiveControl
        /// </summary>
        /// <param name="eArgs"></param>
        private void setAsActiveControl(EventArgs eArgs)
        {
            if (null != _onClickHandler)
                this._onClickHandler(this, eArgs);
        }

        /// <summary>
        /// addRowCQGridBtn_Click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void addRowCQGridBtn_Click(object sender, EventArgs e)
		{
			cfSettingsGrid.RowTextVisible = false;
			iGRow newRow = cfSettingsGrid.Rows.Add();
            //Restore information for not saved user input.
            if (_iCurrentEditCellRowIndex >= 0 && _iCurrentEditCellColIndex >= 0 && !_sCellText.Equals(string.Empty))
            {
                cfSettingsGrid.Cells[_iCurrentEditCellRowIndex, _iCurrentEditCellColIndex].Value = _sCellText;
            }

            newRow.Cells[0].Value = iGAttributeNameDropdownList.Items[0].Value.ToString();
            cfSettingsGrid.PerformAction(TenTec.Windows.iGridLib.iGActions.GoLastCell);
            foreach (iGRow row in cfSettingsGrid.SelectedRows)
            {
                row.Selected = false;
            }
            foreach (iGCell cell in cfSettingsGrid.SelectedCells)
            {
                cell.Selected = false;
            }
            newRow.Selected = true;
            newRow.Cells[1].Selected = true;
			setAsActiveControl(e);
            UpdateToolStrip();
            if (OnRowsChangedImpl != null)
            {
                OnRowsChangedImpl();
            }
		}

        /// <summary>
        /// removeCQRowBtn_Click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void removeCQRowBtn_Click(object sender, EventArgs e)
		{
			try 
            {
				cfSettingsGrid.Focus();
				cfSettingsGrid.RowTextVisible = false;

				if (0 < cfSettingsGrid.SelectedRows.Count) 
                {
                    int count = cfSettingsGrid.SelectedRows.Count;

					for (int i = 0; count > i; i++) {
                        int selectionIndex = cfSettingsGrid.SelectedRows[0].Index;
						cfSettingsGrid.Rows.RemoveAt(selectionIndex);
                        if (selectionIndex == 0)
                            _iCurrentEditCellRowIndex = _iCurrentEditCellColIndex = -1;
					}
				}
			} 
            catch (Exception) 
            { } 
            finally 
            {
				setAsActiveControl(e);
                UpdateToolStrip();
				this.Refresh();
			}
            if (OnRowsChangedImpl != null)
            {
                OnRowsChangedImpl();
            }
		}

        /// <summary>
        /// clearCQgridbtn_Click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void clearCQgridbtn_Click(object sender, EventArgs e)
		{
			cfSettingsGrid.RowTextVisible = false;
			Clear();
			setAsActiveControl(e);
            UpdateToolStrip();
            if (OnRowsChangedImpl != null)
            {
                OnRowsChangedImpl();
            }
		}

        private void cfSettingsGrid_DoubleClick(object sender, iGCellClickEventArgs e)
        {
            if (e.ColIndex == 0)
            {
                this.cfSettingsGrid.Rows[e.RowIndex].Cells[0].Selectable= iGBool.False;
            }
        }

        /// <summary>
        /// update tool strip status
        /// </summary>
        private void UpdateToolStrip()
        {
            if (cfSettingsGrid.Rows.Count > 0)
            {
                _theRemoveAllStripButton.Enabled = true;
                _theDeleteStatementStripButton.Enabled = (cfSettingsGrid.SelectedRows.Count > 0);
            }
            else
            {
                _theDeleteStatementStripButton.Enabled = false;
                _theRemoveAllStripButton.Enabled = false;
            }
        }

        /// <summary>
        /// cqSettingsGrid_SelectionChanged
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void cqSettingsGrid_SelectionChanged(object sender, EventArgs e)
		{
			setAsActiveControl(e);
            _theDeleteStatementStripButton.Enabled = true;
		}

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void cqSettingsGrid_Click(object sender, EventArgs e) 
        {
			setAsActiveControl(e);
		}

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void cqSettingsGrid_CellClick(object sender, iGCellClickEventArgs e)
		{
			setAsActiveControl(e);
		}

        /// <summary>
        /// copyContentsToClipboard
        /// </summary>
		public void copyContentsToClipboard() {
            if (0 >= cfSettingsGrid.SelectedRows.Count)
				return;

            int i = cfSettingsGrid.SelectedRows[0].Index;

			try {
				String value = cfSettingsGrid.CurCell.Value.ToString();
				Clipboard.SetText(value);

			} catch(Exception) 
            { }
		}

        /// <summary>
        /// To select all rows
        /// </summary>
		private void selectAllRows() {
			try {
				cfSettingsGrid.PerformAction(iGActions.SelectAllRows);
			} 
            catch (Exception) 
            { }

		}

        /// <summary>
        /// ProcessCmdKey
        /// </summary>
        /// <param name="msg"></param>
        /// <param name="keyData"></param>
        /// <returns></returns>
		protected override bool ProcessCmdKey(ref Message msg, Keys keyData) {
			bool processFurther = true; 

			switch (msg.Msg) {
				case 0x100:
				case 0x104:
					switch (keyData) {
						case Keys.Control | Keys.C:
						case Keys.Control | Keys.Shift | Keys.C:
							copyContentsToClipboard();
							processFurther = false;
							break;
						case Keys.Control | Keys.A:
						case Keys.Control | Keys.Shift | Keys.A:
							selectAllRows();
							processFurther = false;
							break;

						default:  break;
					}
					break;

				default: break;
			}

			if (!processFurther)
				return false;

			return base.ProcessCmdKey(ref msg, keyData);
		}

        #endregion Private Methods
    }

}
