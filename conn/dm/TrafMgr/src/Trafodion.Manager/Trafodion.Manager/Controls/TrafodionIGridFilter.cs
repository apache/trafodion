//
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

using System;
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Windows.Forms;
using TenTec.Windows.iGridLib;
using System.Drawing;
using System.Runtime.InteropServices;


namespace Trafodion.Manager.Framework.Controls
{
    public partial class TrafodionIGridFilter : TrafodionForm
    {

        /**
         *  List of initially visible columns.
         */
        private List<String> _currentVisibleColumns = new List<String>();
        private List<String> _defaultVisibleColumns = new List<String>();
        private TrafodionIGrid _grid = null;
        private string _helpTopic = "";

        #region Members for drag & drop
         
        private int indexOfItemUnderMouseToDrag = ListBox.NoMatches;
        private int indexOfItemUnderMouseToDrop = ListBox.NoMatches;
        private Rectangle dragBoxFromMouseDown;

        private const int TOLERANCE_SCROLL_TOP = 3;        // Tolerance value to control availabilty for scrolling towards top
        private const int TOLERANCE_SCROLL_BOTTOM = 10;    // Tolerance value to control availabilty for scrolling towards bottom

        #endregion

        #region  Properties

        public TrafodionIGrid Grid
        {
            get {return _grid;}
            set { _grid = value; }
        }

        public List<String> DefaultVisibleColumns
        {
            get
            {
                return _defaultVisibleColumns;
            }
        }

        public List<String> CurrentVisibleColumns
        {
            get { return _currentVisibleColumns; }
            set
            {                
                SetVisibleColumns(value);
            }
        }

        public List<string> AllColumns
        {
            get
            {
                List<string> allColumns = new List<string>();
                foreach (DictionaryEntry keyValuePair in _TrafodionGridFilterCheckBox.Items)
                {
                    allColumns.Add((string)keyValuePair.Key);
                }
                return allColumns;
            }
        }

        #endregion  /*  End of  region  Properties.  */

        public TrafodionIGridFilter Clone()
        {
            return new TrafodionIGridFilter(this._grid, this.CurrentVisibleColumns, this.DefaultVisibleColumns);
        }

        #region  Constructors

        public TrafodionIGridFilter(TrafodionIGrid grid)
            : this(grid, null, null)
        {
        }

        public TrafodionIGridFilter(TrafodionIGrid grid, List<string> currentVisibleColumns, List<string> defaultVisibleColumns)
        {
            InitializeComponent();
            BindFilter(grid, currentVisibleColumns, defaultVisibleColumns);
            SetHelpTopic(grid.HelpTopic);
        }

        #endregion  /*  End of  region  Constructors.  */

        /// <summary>
        /// Correct item list:
        /// 1. Remove inexistent columns, which can be found in the filter item list, but cannot be found in the actual data table;
        /// 2. Add new columns, which can be found in the actual data table, but cannot be found in the filter item list
        /// It's used to fix the issueof incorrect iGrid column visibility when switching systems between M7 and M8
        /// </summary>
        /// <param name="columns"></param>
        public void CorrectItemList(TrafodionIGrid grid)
        {
            iGColCollection columns = grid.Cols;
            CheckedListBox.ObjectCollection filterListItems = this._TrafodionGridFilterCheckBox.Items;

            // Remove inexistent columns
            for (int i = this._TrafodionGridFilterCheckBox.Items.Count - 1; i >= 0; i--)
            {
                string columnName = (string)((DictionaryEntry)filterListItems[i]).Key;
                if (!columns.KeyExists(columnName))
                {
                    filterListItems.RemoveAt(i);
                }
            }
            // Add new columns
            foreach (iGCol column in columns)
            {
                string columnName = column.Key;
                bool isColumnExistingInFilter = false;
                foreach (DictionaryEntry keyValuePair in filterListItems)
                {
                    if (0 == string.Compare((string)keyValuePair.Key, columnName, true))
                    {
                        isColumnExistingInFilter = true;
                        break;
                    }
                }

                if (!isColumnExistingInFilter && !grid.AlwaysHiddenColumnNames.Contains(columnName))
                {
                    string itemText = column.Text == null ? string.Empty : TrafodionIGridUtils.ConvertBreakToBlank(column.Text.ToString());
                    DictionaryEntry keyValuePair = new DictionaryEntry(column.Key, itemText);
                    filterListItems.Insert(column.Order, keyValuePair);
                    this._TrafodionGridFilterCheckBox.SetItemChecked(column.Order, true); // Set the new column to be visible by default.
                }
            }
        }

        #region  PrivateMethods

        /// <summary>
        /// Bind data to Filter according to columns of grid;
        /// Extract filter items from grid's columns for default visible items and initial visibile items
        /// </summary>
        /// <param name="grid"></param>
        /// <returns></returns>
        private void BindFilter(TrafodionIGrid grid, List<string> initialCurrentVisibleColumns, List<string> initialDefaultVisibleColumns)
        {
            this._grid = grid;
            this._TrafodionGridFilterCheckBox.DisplayMember = "Value";

            // Make copies
            initialCurrentVisibleColumns = initialCurrentVisibleColumns == null ? new List<string>() : initialCurrentVisibleColumns.GetRange(0, initialCurrentVisibleColumns.Count);
            initialDefaultVisibleColumns = initialDefaultVisibleColumns == null ? new List<string>() : initialDefaultVisibleColumns.GetRange(0, initialDefaultVisibleColumns.Count);

            ArrayList keyTextPairs = new ArrayList();
            List<string> defaultVisibleColumns = new List<string>();
            List<string> currentVisibleColumns = new List<string>();

            foreach (iGCol gridColumn in grid.Cols)
            {
                // If column is set to forcefully hidden, do not include in filter list
                if (!grid.AlwaysHiddenColumnNames.Contains(gridColumn.Key))
                {
                    string itemKey = gridColumn.Key;
                    string itemText = gridColumn.Text == null ? string.Empty : TrafodionIGridUtils.ConvertBreakToBlank(gridColumn.Text.ToString());
                    DictionaryEntry keyTextPair = new DictionaryEntry(itemKey, itemText);                    
                    keyTextPairs.Add(keyTextPair);
                    defaultVisibleColumns.Add(gridColumn.Key);

                    if (gridColumn.Visible)
                    {
                        currentVisibleColumns.Add(gridColumn.Key);
                    }
                }
            }

            this._TrafodionGridFilterCheckBox.Items.AddRange(keyTextPairs.ToArray());

            _defaultVisibleColumns = (initialDefaultVisibleColumns != null && initialDefaultVisibleColumns.Count > 0) ? initialDefaultVisibleColumns : defaultVisibleColumns;
            CurrentVisibleColumns = (initialCurrentVisibleColumns != null && initialCurrentVisibleColumns.Count > 0) ? initialCurrentVisibleColumns : currentVisibleColumns;
        }

        private void RenewOrderedVisibleColumns()
        {
            this._currentVisibleColumns.Clear();

            for (int i = 0; i < _TrafodionGridFilterCheckBox.Items.Count; i++)
            {
                bool isChecked = this._TrafodionGridFilterCheckBox.GetItemChecked(i);
                if (isChecked)
                {
                    String column = (string)((DictionaryEntry)_TrafodionGridFilterCheckBox.Items[i]).Key;
                    this._currentVisibleColumns.Add(column);
                }
            }
        }

        private void SetVisibleColumns(List<string> visibleColumns)
        {
            this._currentVisibleColumns = visibleColumns.GetRange(0, visibleColumns.Count);

            for (int i = 0; i < _TrafodionGridFilterCheckBox.Items.Count; i++)
            {
                String colName = (string)((DictionaryEntry)_TrafodionGridFilterCheckBox.Items[i]).Key;
                _TrafodionGridFilterCheckBox.SetItemChecked(i, this._currentVisibleColumns.Contains(colName));
            }

            //Now reorder the list order based on the initial columns order
            for (int j = 0; j < this._currentVisibleColumns.Count; j++)
            {
                for (int i = 0; i < _TrafodionGridFilterCheckBox.Items.Count; i++)
                {
                    String colName = (string)((DictionaryEntry)_TrafodionGridFilterCheckBox.Items[i]).Key;
                    if (colName.Equals(this._currentVisibleColumns[j]))
                    {
                        MoveGridFilterItem(i, j);
                        break;
                    }
                }
            }
        }

        private void DefaultsButton_Click(object sender, EventArgs e)
        {
            SetVisibleColumns(_defaultVisibleColumns);
        }
        
        private void CancelButton_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void OKButton_Click(object sender, EventArgs e)
        {
            RenewOrderedVisibleColumns();

            if (this._currentVisibleColumns.Count > 0)
            {
                this.DialogResult = DialogResult.OK;
                this.Close();
            }
            else
            {
                MessageBox.Show(Properties.Resources.You_must_select_one_column, "No column selected", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }


        private void TrafodionIGridFilter_ResizeEnd(object sender, EventArgs e)
        {
            this._filterColumnGroupBox.Width = this.Width - this._filterColumnGroupBox.Left - 30;
            this._filterColumnGroupBox.Height = this._okButton.Top -
                                                      this._filterColumnGroupBox.Top - 30;
        }


        private void EnableDisableUpDownButtons(bool enableFlag)
        {
            int idx = this._TrafodionGridFilterCheckBox.SelectedIndex;
            int lastItemIdx = this._TrafodionGridFilterCheckBox.Items.Count - 1;
            this.btnTop.Enabled = this._moveColumnUpButton.Enabled = enableFlag && (idx > 0);
            this.btnBottom.Enabled = this._moveColumnDownButton.Enabled = enableFlag && (idx < lastItemIdx);             
        }
        
        private void MoveGridFilterItem(int currentIdx, int destIndex)
        {
            int lastIdx = this._TrafodionGridFilterCheckBox.Items.Count - 1;
            bool itemCheckState = this._TrafodionGridFilterCheckBox.CheckedIndices.Contains(currentIdx);

            if ((0 <= destIndex) && (lastIdx >= destIndex))
            {
                DictionaryEntry keyTextPair = (DictionaryEntry)this._TrafodionGridFilterCheckBox.Items[currentIdx];
                this._TrafodionGridFilterCheckBox.Items.RemoveAt(currentIdx);
                this._TrafodionGridFilterCheckBox.Items.Insert(destIndex, keyTextPair);
                this._TrafodionGridFilterCheckBox.SelectedIndex = destIndex;
                this._TrafodionGridFilterCheckBox.SetItemChecked(destIndex, itemCheckState);
            }

            EnableDisableUpDownButtons(true);
        }

        public void MoveVisibleColumn(string column, int order)
        {
            List<string> visibleColumns = this._currentVisibleColumns.GetRange(0, this._currentVisibleColumns.Count);
            order = (order >= 0 && order <= visibleColumns.Count - 1) ? order : (visibleColumns.Count - 1);
            if (visibleColumns.Contains(column))
            {
                visibleColumns.Remove(column);
                visibleColumns.Insert(order, column);
            }
            SetVisibleColumns(visibleColumns);
        }


        private void MoveColumnUpButton_Click(object sender, EventArgs e)
        {
            int currentIdx = this._TrafodionGridFilterCheckBox.SelectedIndex;
            if (0 < currentIdx)
                MoveGridFilterItem(currentIdx, currentIdx - 1);

        }



        private void MoveColumnDownButton_Click(object sender, EventArgs e)
        {
            int currentIdx = this._TrafodionGridFilterCheckBox.SelectedIndex;
            int lastIdx = this._TrafodionGridFilterCheckBox.Items.Count - 1;
            if (lastIdx > currentIdx)
                MoveGridFilterItem(currentIdx, currentIdx + 1);

        }



        private void FilterCheckBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            bool enabled = this._TrafodionGridFilterCheckBox.SelectedIndex > -1;
            EnableDisableUpDownButtons(enabled);
        }

        private void SetHelpTopic(string topic)
        {
            _helpButton.HelpTopic = topic;

            if (!String.IsNullOrEmpty(topic))
            {
                _helpButton.Enabled = true;
                _helpButton.Visible = true;
            }
            else
            {
                _helpButton.Enabled = false;
                _helpButton.Visible = false;
            }
        }

        #endregion  /*  End of  region  PrivateMethods.  */

        private void TrafodionGridFilter_Load(object sender, EventArgs e)
        {

        }

        private void HelpButton_Click(object sender, EventArgs e)
        {
            if (!String.IsNullOrEmpty(_helpTopic))
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(_helpTopic);
            }
        }

        private void _TrafodionGridFilterCheckBox_MouseClick(object sender, MouseEventArgs e)
        {
            if ((e.Button == MouseButtons.Left) && (e.X < 13))
            {
                int index = this._TrafodionGridFilterCheckBox.IndexFromPoint(e.Location);
                if (index > -1)
                {
                    /*
                     * It's very tricky regarding clicking the check box to check the item in the list. 
                     * When select one item, and then select it again, it will reverse the status of check box.
                     * SetItemChecked() method actually contains behavor of selecting the item. 
                     * Below code looks strange but effective.
                     * 
                     */
                    this._TrafodionGridFilterCheckBox.SetItemChecked(index, this._TrafodionGridFilterCheckBox.GetItemChecked(index));
                    this._TrafodionGridFilterCheckBox.ClearSelected();
                    this._TrafodionGridFilterCheckBox.SelectedIndex = index;
                }
            }
        }

        private void chkAll_CheckedChanged(object sender, EventArgs e)
        {
            bool isChecked = chkAll.Checked;
            this._currentVisibleColumns.Clear();
            for (int i = 0; i < _TrafodionGridFilterCheckBox.Items.Count; i++)
            {
                _TrafodionGridFilterCheckBox.SetItemChecked(i, isChecked);
            }
        }

        private void _TrafodionGridFilterCheckBox_MouseDown(object sender, MouseEventArgs e)
        {              
            // Get the index of the item the mouse is below.
            this.indexOfItemUnderMouseToDrag = _TrafodionGridFilterCheckBox.IndexFromPoint(e.X, e.Y);

            if (this.indexOfItemUnderMouseToDrag != ListBox.NoMatches)
            {
                // Remember the point where the mouse down occurred. The DragSize indicates
                // the size that the mouse can move before a drag event should be started.  
                Size dragSize = SystemInformation.DragSize;

                // Create a rectangle using the DragSize, with the mouse position being at the center of the rectangle.
                this.dragBoxFromMouseDown = new Rectangle(new Point(e.X - (dragSize.Width / 2), e.Y - (dragSize.Height / 2)), dragSize);
            }
            else
            {
                // Reset the rectangle if the mouse is not over an item in the ListBox.
                this.dragBoxFromMouseDown = Rectangle.Empty;
            }
        }

        private void _TrafodionGridFilterCheckBox_MouseUp(object sender, MouseEventArgs e)
        {
            // Reset the drag rectangle when the mouse button is raised.
            this.dragBoxFromMouseDown = Rectangle.Empty;
        }

        /// <summary>
        /// Sequence:
        ///         MouseDown ->    MouseMove (call DoDragDrop) 
        ///                                       -> DragOver 
        ///                                       -> DragDrop
        ///                         MouseMove <-
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _TrafodionGridFilterCheckBox_MouseMove(object sender, MouseEventArgs e)
        {
            if ((e.Button & MouseButtons.Left) == MouseButtons.Left)
            {
                // If the mouse moves outside the rectangle, start the drag.
                if (this.dragBoxFromMouseDown != Rectangle.Empty
                    && !this.dragBoxFromMouseDown.Contains(e.X, e.Y))
                {
                    // Proceed with the drag-and-drop, passing in the list item. 
                   _TrafodionGridFilterCheckBox.DoDragDrop(_TrafodionGridFilterCheckBox.Items[indexOfItemUnderMouseToDrag], DragDropEffects.Move);
                }
            }
        }

        private void _TrafodionGridFilterCheckBox_DragOver(object sender, DragEventArgs e)
        {
            // Determine whether DictionaryEntry data exists in the drop data. 
            // If not, then the drop effect reflects that the drop cannot occur.
            if (!e.Data.GetDataPresent(typeof(DictionaryEntry)))
            {
                e.Effect = DragDropEffects.None;
                return;
            } 

            // Scroll towards top or bottom
            Point position = _TrafodionGridFilterCheckBox.PointToClient(new Point(e.X, e.Y));
            if (position.Y - TOLERANCE_SCROLL_TOP < 0)
            {
                Utilities.SendMessage(_TrafodionGridFilterCheckBox.Handle, (int)ScrollDirection.Vertical, (IntPtr)VScrollBarCommands.SB_LINEUP, IntPtr.Zero);
            }
            else if (position.Y + TOLERANCE_SCROLL_BOTTOM > _TrafodionGridFilterCheckBox.Height)
            {
                Utilities.SendMessage(_TrafodionGridFilterCheckBox.Handle, (int)ScrollDirection.Vertical, (IntPtr)VScrollBarCommands.SB_LINEDOWN, IntPtr.Zero);
            }           

            // Get the index of the item the mouse is below
            this.indexOfItemUnderMouseToDrop = _TrafodionGridFilterCheckBox.IndexFromPoint(_TrafodionGridFilterCheckBox.PointToClient(new Point(e.X, e.Y)));
            if (this.indexOfItemUnderMouseToDrop != ListBox.NoMatches)
            {
                e.Effect = DragDropEffects.Move;
            }
            else
            {
                e.Effect = DragDropEffects.None;
            }
        }
        
        private void _TrafodionGridFilterCheckBox_DragDrop(object sender, DragEventArgs e)
        {
            // Ensure that the list item index is contained in the data.
            if (e.Data.GetDataPresent(typeof(DictionaryEntry)))
            {
                // Perform drag-and-drop, depending upon the effect.
                if (e.Effect == DragDropEffects.Move)
                {
                    MoveGridFilterItem(indexOfItemUnderMouseToDrag, indexOfItemUnderMouseToDrop);
                }
            }
        }

        private void btnTop_Click(object sender, EventArgs e)
        {
            MoveGridFilterItem(_TrafodionGridFilterCheckBox.SelectedIndex, 0);
        }

        private void btnBottom_Click(object sender, EventArgs e)
        {
            MoveGridFilterItem(_TrafodionGridFilterCheckBox.SelectedIndex, _TrafodionGridFilterCheckBox.Items.Count - 1);
        }        

    }   /*  End of  class  TrafodionIGridFilter.  */


}  /*  End of  Trafodion.Manager.Framework.Controls  namespace.  */
