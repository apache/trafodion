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
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

using TenTec.Windows.iGridLib;
using System.Collections;
using System.Data;
using System.Drawing;


namespace Trafodion.Manager.Framework.Controls {
	public class TrafodionIGridUtils {

		#region Members

		#region Static Members

        private const string UNDERLINE = "_";
        private const string BLANK_SPACE = " ";
        private const string LINE_BREAK_1 = "\n";
        private const string LINE_BREAK_2 = "\r\n";

		/**
		 *  Minimum IGrid column width.
		 */
		public const int MIN_IGRID_COLUMN_WDIDTH = 64;


		/**
		 *  Maximum IGrid column width.
		 */
		public const int MAX_IGRID_COLUMN_WDIDTH = 500;


		#endregion  /*  End of  region  Static Members.  */

		#endregion  /*  End of  region  Members.  */


		// A custom comparer for sorting integer values.
		public class NCCHashTableIntValueSorter : IComparer {
			int IComparer.Compare(object x, object y) {
				return (int) x - (int) y;
			}

		}


		#region Public Methods
        
        /// <summary>
        /// Helper method to populate a GataGrid from the DataTable using the attributes from the config
        /// </summary>
        /// <param name="aConfig"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aDataGrid"></param>
        public static void PopulateGrid(IColumnMappingInfo iColumnMappingInfo, DataTable aDataTable, TrafodionIGrid aDataGrid)
        {
            if (aDataTable != null)
            {
                aDataGrid.Clear();
                foreach (DataColumn dc in aDataTable.Columns)
                {
                    string colName = dc.ColumnName;
                    string colText;
                    int colWidth;
                    iColumnMappingInfo.GetColumnMappingInfo(colName, out colText, out colWidth);

                    //if the column text is null or empty, use the default formatting
                    if (colText == null)
                    {
                        colText = ConvertUnderlineToBreak(colName);
                    }

                    //Add the column based on the passed width
                    iGCol col = null;
                    if (colWidth > 0)
                    {
                        col = aDataGrid.Cols.Add(colName, colText, colWidth);
                        col.CellStyle.ValueType = dc.DataType;
                    }
                    else
                    {
                        col = aDataGrid.Cols.Add(colName, colText);
                        col.CellStyle.ValueType = dc.DataType;
                    }
                    col.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);
                }
                if (aDataTable.Rows.Count > 0)
                {

                    //set alignments based on data
                    DataRow dataRow = aDataTable.Rows[0];
                    int col = 0;
                    for (int i = 0; i < aDataTable.Columns.Count; i++)
                    {
                        if (IsNumeric(dataRow[i]))
                        {
                            aDataGrid.Cols[col].CellStyle.TextAlign = iGContentAlignment.MiddleRight;
                            //aDataGrid.Cols[col].CellStyle.FormatString = "{0:#,0.##}";
                        }
                        else
                        {
                            aDataGrid.Cols[col].CellStyle.TextAlign = iGContentAlignment.MiddleLeft;
                        }
                        col++;
                    }

                    //set the actual data
                    foreach (DataRow dr in aDataTable.Rows)
                    {
                        iGRow row = aDataGrid.Rows.Add();

                        for (int i = 0; i < aDataTable.Columns.Count; i++)
                        {
                            row.Cells[i].Value = dr[i];
                        }
                    }
                }
            }
        }
        
        public static void PopulateGrid(IColumnMappingInfo iColumnMappingInfo, MethodInvoker methodFillData, TrafodionIGrid dataGrid)
        {
            methodFillData(); 
            foreach (iGCol gridColumn in dataGrid.Cols)
            {
                gridColumn.ColHdrStyle.TextFormatFlags = iGStringFormatFlags.WordWrap;

                if (gridColumn.Text == null
                    || Convert.ToString(gridColumn.Text) == string.Empty
                    || 0 == string.Compare(gridColumn.Key, Convert.ToString(gridColumn.Text), true))
                {
                    string colKey = gridColumn.Key;
                    string colText;
                    int colWidth;
                    iColumnMappingInfo.GetColumnMappingInfo(colKey, out colText, out colWidth);

                    if (colText == null)
                    {
                        colText = ConvertUnderlineToBreak(colKey);
                    }
                    else
                    {
                        gridColumn.Text = colText;
                    }
                }

                gridColumn.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);
            }

            if (dataGrid.Rows.Count > 0)
            {
                // Set alignments based on data
                iGRow gridRow = dataGrid.Rows[0];
                for (int i = 0; i < dataGrid.Cols.Count; i++)
                {
                    if (IsNumeric(gridRow.Cells[i].Value))
                    {
                        dataGrid.Cols[i].CellStyle.TextAlign = iGContentAlignment.MiddleRight;
                    }
                    else
                    {
                        dataGrid.Cols[i].CellStyle.TextAlign = iGContentAlignment.MiddleLeft;
                    }
                }
            }
        }
        
        public static bool IsNumeric(Object value)
        {
            if (value != null)
            {
                if ((value is sbyte)
                    || (value is byte)
                    || (value is short)
                    || (value is ushort)
                    || (value is int)
                    || (value is uint)
                    || (value is long)
                    || (value is ulong)
                    || (value is float)
                    || (value is double)
                    || (value is decimal)
                    )
                {
                    return true;
                }
            }
            return false;
        }

        public static void MoveConfigColumnOrder(iGColCollection gridColumns, List<string> configColumns, string movedColumn, int movedToGridColumnOrder)
        {
            int sourceVisibleColumnIndex = configColumns.IndexOf(movedColumn);
            if (sourceVisibleColumnIndex >= 0)
            {
                int destinationVisibleColumnIndex = CalculateDragDestinationIndex(gridColumns, movedToGridColumnOrder);
                if (destinationVisibleColumnIndex >= 0 && destinationVisibleColumnIndex <= configColumns.Count)
                {
                    configColumns.RemoveAt(sourceVisibleColumnIndex);
                    configColumns.Insert(destinationVisibleColumnIndex, movedColumn);
                }
            }
        }        

        private static int CalculateDragDestinationIndex(iGColCollection gridColumns, int dragDestinationOrder)
        {
            int dragDestinationIndex = dragDestinationOrder;

            for (int i = 0; i < dragDestinationOrder; i++)
            {
                if (!gridColumns.FromOrder(i).Visible)
                {
                    dragDestinationIndex = dragDestinationIndex - 1;
                }
            }

            return dragDestinationIndex;
        }

        public static void ApplyGridSetting(TrafodionIGrid grid)
        {
            //set the current filter for the igrid
            if (grid.CurrentFilter == null)
            {
                grid.CurrentFilter = new TrafodionIGridFilter(grid, grid.Config.CurrentVisibleColumns, grid.Config.DefaultVisibleColumns);
            }

            //If we already have persisted the earlier visible columns, use that.
            if (grid.Config.CurrentVisibleColumns.Count > 0)
            {
                // It's key to disable AutoResizeCols, since it will overwrite the persisted columns width
                grid.AutoResizeCols = false;

                // Bring the Filter and the DataGrid in sync in terms of columns added/removed since the last fetch
                if (!grid.CurrentFilter.Visible)
                {
                    grid.CurrentFilter.CorrectItemList(grid);
                }

                // Set the column widths
                foreach (string columnName in grid.Config.CurrentVisibleColumns)
                {
                    ColumnMappingInfo cm = grid.Config.GetColumnMapping(columnName);
                    if (cm != null)
                    {
                        if (grid.Cols.KeyExists(columnName))
                        {
                            grid.Cols[columnName].Width = cm.ColumnWidth;
                        }
                    }
                }
            }

            // Apply sort 
            grid.SortObject.Clear();
            foreach (ColumnSort columnSort in grid.Config.ColumnSorts)
            {
                grid.SortObject.Add(columnSort.ColIndex, (iGSortOrder)columnSort.SortOrder);
            }
            if (grid.Config.ColumnSorts.Count > 0)
            {
                grid.Sort();
            }

            // Apply filter
            grid.ApplyFilter();
            grid.SetAutoRowHeight();
        }

		/**
		 *  <summary>
		 *  Adds a column to the specified IGrid Control.
		 *  </summary>
		 *
		 *  <param name="gridControl">The iGrid Control</param>
		 *  <param name="columnName">Name of the column to be added</param>
		 *  <remarks>Since: NPA1.5</remarks>
		 */
		public static iGCol addColumnToIGridControl(iGrid gridControl, String columnName) {
			return addColumnToIGridControl(gridControl, columnName, false, MIN_IGRID_COLUMN_WDIDTH,
										   MAX_IGRID_COLUMN_WDIDTH);

		}  /*  End of  addColumnToIGridControl(iGrid, String)  method.  */




		/**
		 *  <summary>
		 *  Adds a column to the specified IGrid Control (columns are hidden based on the
		 *  hideColumn flag).
		 *  </summary>
		 *
		 *  <param name="gridControl">The iGrid Control</param>
		 *  <param name="columnName">Name of the column to be added</param>
		 *  <param name="hideColumn">Whether or not to add the column as a hidden column</param>
		 *  <remarks>Since: NPA1.5</remarks>
		 */
		public static iGCol addColumnToIGridControl(iGrid gridControl, String columnName,
													bool hideColumn) {
			return addColumnToIGridControl(gridControl, columnName, hideColumn, MIN_IGRID_COLUMN_WDIDTH,
										   MAX_IGRID_COLUMN_WDIDTH);

		}  /*  End of  addColumnToIGridControl(iGrid, String, bool)  method.  */



		/**
		 *  <summary>
		 *  Adds a column to the specified IGrid Control (columns are hidden based on the
		 *  hideColumn flag). The size settings control the minimum and maximum column width.
		 *  </summary>
		 *
		 *  <param name="gridControl">The iGrid Control</param>
		 *  <param name="columnName">Name of the column to be added</param>
		 *  <param name="hideColumn">Whether or not to add the column as a hidden column</param>
		 *  <param name="minColWidth">Minimum column width</param>
		 *  <param name="maxColWidth">Maximum column width</param>
		 *  <remarks>Since: NPA1.5</remarks>
		 */
		public static iGCol addColumnToIGridControl(iGrid gridControl, String columnName,
													bool hideColumn, int minColWidth,
													int maxColWidth) {
			iGColPattern colPat = new iGColPattern();
			iGColHdrStyle colHeaderStyle = new iGColHdrStyle(true);
			iGCellStyle colCellStyle = new iGCellStyle(true);

			colPat.CellStyle = colCellStyle;
			colPat.ColHdrStyle = colHeaderStyle;
			colPat.Key = columnName;
            
            String columnHeading = columnName.Replace("_", " ");
			colPat.Text = columnHeading;

			colPat.Width = Math.Max(minColWidth, Math.Min(maxColWidth, columnName.Length * 8));

			iGCol addedColumn = gridControl.Cols.Add(colPat);

			if (hideColumn)
				gridControl.Cols[columnName].Visible = false;

			return addedColumn;

		}  /*  End of  addColumnToIGridControl(iGrid, String, bool, int, int)  method.  */



		/**
		 *  <summary>
		 *  Updates the IGrid column names to a more user-friendly (humanly readable) form. 
		 *  </summary>
		 *
		 *  <param name="theGrid">The iGrid Control</param>
		 *  <remarks>Since: NPA2.0</remarks>
		 */
        public static void updateIGridColumnHeadings(iGrid theGrid) {
            foreach (iGCol col in theGrid.Cols) {
                try {
                    String name = col.Key;
                    if (null != col.Text)
                        name = col.Text.ToString();

                    col.Text = ConvertUnderlineToBreak(name);

                } catch (Exception) {
                }

            }

        }  /*  End of  updateIGridColumnHeadings(iGrid)  method.  */

        /// <summary>
        /// Convert line break to blank space in the text
        /// </summary>
        /// <param name="text"></param>
        /// <returns></returns>
        public static string ConvertBreakToBlank(string text)
        {
            text = text.Replace(LINE_BREAK_2, BLANK_SPACE);
            text = text.Replace(LINE_BREAK_1, BLANK_SPACE);
            return text;
        }
        /// <summary>
        /// Convert underline to line break in the text
        /// </summary>
        /// <param name="text"></param>
        /// <returns></returns>
        public static string ConvertUnderlineToBreak(string text)
        {
            return text.Replace(UNDERLINE, Environment.NewLine);
        }

        public static string ConvertBlankToBreak(string text)
        {
            return text.Replace(BLANK_SPACE, Environment.NewLine);
        }

		/**
		 *  <summary>
		 *  Gets the content of the selected IGrid rows for copying to the clipboard.
		 *  </summary>
		 *
		 *  <param name="theGrid">The iGrid Control</param>
		 *  <param name="showWarning">Whether or not to show Warnings if no rows have been selected</param>
		 *  <remarks>Since: NPA2.0</remarks>
		 */
		public static String getSelectedIGridContents(iGrid theGrid, bool showWarning) {
			if (0 == theGrid.SelectedRows.Count) {
				if (0 == theGrid.Rows.Count)
					return null;

				if (showWarning)
					MessageBox.Show("\nWarning:  No Row(s) have been selected for copying.\n\n",
									"No Row(s) Selected Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);

				return null;
			}

			StringBuilder headersSB = new StringBuilder();
			StringBuilder rowsSB = new StringBuilder();
			int nRows = theGrid.Rows.Count;
			int nCols = theGrid.Cols.Count;

			int i = 0;
			bool isTheFirstColumn = true;

            for (i = 0; i < nCols; i++)
            {
                if (!isTheFirstColumn)
                    headersSB.Append('\t');

                headersSB.Append(ConvertBreakToBlank(theGrid.Cols[i].Text.ToString()));
                isTheFirstColumn = false;
            }

            for (i = 0; i < theGrid.SelectedRows.Count; i++)
            {
				isTheFirstColumn = true;

				for (int j = 0; j < nCols; j++) 
                {
					if (!isTheFirstColumn)
						rowsSB.Append('\t');

                    String colValue = theGrid.SelectedRows[i].Cells[j].Text;
					rowsSB.Append(colValue);
					isTheFirstColumn = false;
				}

				rowsSB.Append(Environment.NewLine);
			}

			String copiedData = "";
			if (0 < rowsSB.Length)
				copiedData = headersSB.ToString() + Environment.NewLine + rowsSB.ToString();

			return copiedData;

		}  /*  End of  getSelectedIGridContents(iGrid, bool)  method.  */



        /**
         *  <summary>
         *  Gets the content of the selected IGrid rows for copying to the clipboard.
         *  </summary>
         *
         *  <param name="theGrid">The iGrid Control</param>
         *  <param name="showWarning">Whether or not to show Warnings if no rows have been selected</param>
         *  <remarks>Since: NPA2.0</remarks>
         */
        public static String getSelectedIGridCellContents(iGrid theGrid, bool showWarning)
        {
            if (0 == theGrid.SelectedCells.Count)
            {
                if (showWarning)
                    MessageBox.Show("\nWarning:  No Cells(s) have been selected for copying.\n\n",
                                    "No Row(s) Selected Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return null;
            }
            StringBuilder headersSB = new StringBuilder();
            StringBuilder rowsSB = new StringBuilder();
            int nRows = theGrid.Rows.Count;
            int nCols = theGrid.Cols.Count;
            bool isTheFirstColumn = true;
            iGCell cell = null;
            iGCol tempCol = null;
            int visibleColIndex = -1;

            for (int row = 0; row < nRows; row++)
            {
                bool selectedCellFound = false;
                isTheFirstColumn = true;
                for (int col = 0; col < nCols; col++)
                {
                    visibleColIndex = theGrid.Cols.FromOrder(col).Index;
                    cell = theGrid.Cells[row, visibleColIndex];
                    if (theGrid.Rows[row].Visible && cell.Selected)
                    {
                        selectedCellFound = true;
                        if (!isTheFirstColumn)
                            rowsSB.Append('\t');

                        String colValue = theGrid.Cells[row, visibleColIndex].Text;
                        rowsSB.Append(colValue);
                        isTheFirstColumn = false;

                    }
                }
                if (selectedCellFound)
                {
                    rowsSB.Append(Environment.NewLine);
                }

            }

            String copiedData = "";
            if (0 < rowsSB.Length)
            {
                //No Column headers needed for now

                //if (cols != null)
                //{
                //    copiedData = headersSB.ToString() + Environment.NewLine;
                //}
                copiedData += rowsSB.ToString();
            }

            return copiedData;

        }  /*  End of  getSelectedIGridContents(iGrid, bool)  method.  */

        private static string NeedColumnHeaders(iGrid theGrid)
        {
            if (0 == theGrid.SelectedCells.Count)
            {
                return null;
            }
            StringBuilder rowsSB;
            int nRows = theGrid.Rows.Count;
            int nCols = theGrid.Cols.Count;
            bool isTheFirstColumn = true;
            bool hasAnyPriorSelectedRow = false;
            string lastRowCols = null;
            iGCell cell = null;

            for (int row = 0; row < nRows; row++)
            {
                rowsSB = new StringBuilder();
                string selectedRowCols = "";
                isTheFirstColumn = true;
                for (int col = 0; col < nCols; col++)
                {
                    cell = theGrid.Cells[row, col];
                    if (theGrid.Rows[row].Visible && cell.Selected)
                    {
                        if (!isTheFirstColumn)
                            rowsSB.Append(",");                        
                        rowsSB.Append(col + "");
                        isTheFirstColumn = false;

                    }
                }
                selectedRowCols = rowsSB.ToString();
                if (hasAnyPriorSelectedRow)
                {
                    if (selectedRowCols.Length > 0)
                    {
                        if (! selectedRowCols.Equals(lastRowCols))
                        {
                            return null;
                        }
                    }
                }
                hasAnyPriorSelectedRow = (hasAnyPriorSelectedRow) ? hasAnyPriorSelectedRow : (selectedRowCols.Length > 0);
                lastRowCols = (selectedRowCols.Length > 0) ? selectedRowCols : lastRowCols;
            }
            return lastRowCols;
        }
		/**
		 *  <summary>
		 *  Copies the contents of the selected IGrid rows to the clipboard.
		 *  </summary>
		 *
		 *  <param name="theGrid">The iGrid Control</param>
		 *  <param name="showWarning">Whether or not to show Warnings if no rows have been selected</param>
		 *  <remarks>Since: NPA2.0</remarks>
		 */
		public static String copyIGridContentsToClipboard(iGrid theGrid, bool showWarning) {
            String copiedData = "";

            if (theGrid.RowMode)
                copiedData = TrafodionIGridUtils.getSelectedIGridContents(theGrid, showWarning);
            else if (0 < theGrid.SelectedCells.Count)
                copiedData = TrafodionIGridUtils.getSelectedIGridCellContents(theGrid, showWarning);
            else if (showWarning) {
                MessageBox.Show("\nWarning:  No Cell(s) have been selected for copying.\n\n",
                                "No Cell(s) Selected Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return null;
            }


			if (null != copiedData)
				Clipboard.SetDataObject(copiedData, true);

			return copiedData;

		}  /*  End of  copyIGridContentsToClipboard(iGrid, bool)  method.  */

        /**
		 *  <summary>
		 *  Initializes the data grid default row heights. 
		 *  </summary>
		 *
		 *  <param name="theGrid">The iGrid Control</param>
		 *  <remarks>Since: NPA2.0</remarks>
		 */
        public static void initIGridRowHeight(iGrid theGrid)
        {
            String gridName = "";
            try
            {
                gridName = theGrid.Name;

                String testColName = "ROW_SIZE_COL";
                theGrid.Cols.Add(testColName, testColName);

                iGRow theRow = theGrid.Rows.Add();
                String kbdLine1 = @"1!2@3#4$5%6^7&8*9(0)-_=+";
                String kbdLine2 = @"qQwWeErRtTyYuUiIoOpP[{]}\|";
                String kdbLine3 = @"aAsSdDfFgGhHjJkKlL;:'";
                String kdbLine4 = @"zZxXcCvVbBnNmM,<.>/?";

                theRow.Cells[testColName].Value = "Sample_Data: " + kbdLine1 + kbdLine2 + kdbLine3 + kdbLine4;
                theGrid.Rows.AutoHeight();

                theGrid.DefaultRow.Height = (int)(1.15 * theRow.Height);
                theGrid.DefaultRow.NormalCellHeight = (int)(1.15 * theRow.Height);

                theGrid.Rows.Clear();
                theGrid.Cols.RemoveAt(testColName);

            }
            catch (Exception exc)
            {
                if (Logger.IsTracingEnabled)
                    Logger.OutputToLog("@@@ Ignoring iGrid '" + gridName + "' initialization error. " +
                                                "Details=" + exc.Message);

            }

        }  /*  End of  initIGridRowHeight(iGrid)  method.  */
		
		/**
		 *  <summary>
		 *  Fills the data grid using the specified table and fixes up any specified DateTime fields.
		 *  </summary>
		 *		 
		 *  <param name="theGrid">The iGrid Control</param>
		 *  <param name="theTable">Source data table</param>
		 *  <param name="dateTimeFields">Any DateTime fields to be fixed up</param>
		 *  <remarks>Since: NPA2.0</remarks>
		 */
        public static void fillGridWithDataTable(iGrid theGrid, DataTable theTable,
                                                 String[] dateTimeFields)
        {
            // Save Column widths.
            Hashtable colWidths_ht = getIGridColumnWidths(theGrid);
            Hashtable colOrders_ht = getIGridColumnOrder(theGrid);
            Hashtable sortOrders_ht = getIGridSortOrder(theGrid);

            int hScrollBarValue = theGrid.HScrollBar.Value;
            int vScrollBarValue = theGrid.VScrollBar.Value;

            try
            {
                DataTable fixedup_dt = fixDataTableForGridDisplay(theTable, dateTimeFields);

                theGrid.BeginUpdate();
                theGrid.Cols.Clear();
                foreach (DataColumn dc in fixedup_dt.Columns)
                    theGrid.Cols.Add(dc.ColumnName);

                theGrid.FillWithData(fixedup_dt);

            }
            finally
            {
                restoreIGridColumnWidths(theGrid, colWidths_ht);
                restoreIGridColumnOrder(theGrid, colOrders_ht);
                restoreIGridSortOrder(theGrid, sortOrders_ht);
                theGrid.Sort();
                theGrid.EndUpdate();

                theGrid.HScrollBar.Value = hScrollBarValue;
                theGrid.VScrollBar.Value = vScrollBarValue;
            }

        }  /*  End of  fillGridWithDataTable(iGrid, DataTable, String[])  method.  */


		
		/**
		 *  <summary>
		 *  Gets a hashtable containing column names and their widths for the specified iGrid.
		 *  </summary>
		 *
		 *  <param name="theGrid">The iGrid Control</param>
		 *  <remarks>Since: NPA2.0</remarks>
		 */
		public static Hashtable getIGridColumnWidths(iGrid theGrid) {
			Hashtable colWidths_ht = new Hashtable();

			colWidths_ht.Clear();

			if (null != theGrid) {
				foreach (iGCol igc in theGrid.Cols)
					colWidths_ht.Add(igc.Key, igc.Width);

			}

			return colWidths_ht;

		}  /*  End of  getIGridColumnWidths(iGrid)  method.  */



        /**
         *  <summary>
         *  Restores iGrid column widths using the specified hashtable.
         *  </summary>
         *
         *  <param name="theGrid">The iGrid Control</param>
         *  <param name="widths_ht">Hashtable containing saved widths</param>
         *  <remarks>Since: NPA2.0</remarks>
         */
        public static void restoreIGridColumnWidths(iGrid theGrid, Hashtable widths_ht) {
			for (int i = 0; i < theGrid.Cols.Count; i++) {
				try {
					String colName = (String) theGrid.Cols[i].Key;
                    if (widths_ht.ContainsKey(colName) )
                        theGrid.Cols[i].Width = (int) widths_ht[colName];

				} catch (Exception) {
				}
			}

		}  /*  End of  restoreIGridColumnWidths(iGrid, Hashtable)  method.  */



		/**
		 *  <summary>
		 *  Gets a hashtable containing column names and their order for the specified iGrid.
		 *  </summary>
		 *
		 *  <param name="theGrid">The iGrid Control</param>
		 *  <remarks>Since: NPA2.0</remarks>
		 */
		public static Hashtable getIGridColumnOrder(iGrid theGrid) {
			int cnt = theGrid.Cols.Count;
			Hashtable colOrder_ht = new Hashtable();

			colOrder_ht.Clear();
			for (int idx = 0; idx < cnt; idx++) {
				iGCol igc = theGrid.Cols.FromOrder(idx);
				colOrder_ht.Add(igc.Key, idx);
			}

			return colOrder_ht;

		}  /*  End of  getIGridColumnOrder(iGrid)  method.  */



		/**
		 *  <summary>
		 *  Restores iGrid column orders using the specified hashtable.
		 *  </summary>
		 *
		 *  <param name="theGrid">The iGrid Control</param>
		 *  <param name="order_ht">Hashtable containing the column order</param>
		 *  <remarks>Since: NPA2.0</remarks>
		 */
		public static void restoreIGridColumnOrder(iGrid theGrid, Hashtable order_ht) {
			if ((null == theGrid)  ||  (null == order_ht) )
				return;


			int      numEntries  = order_ht.Count;
			String[] orderKeys   = new String[numEntries];
			int[]    orderValues = new int[numEntries];

			order_ht.Keys.CopyTo(orderKeys, 0);
			order_ht.Values.CopyTo(orderValues, 0);

			Array.Sort(orderValues, orderKeys, new NCCHashTableIntValueSorter() );

			Hashtable currentColumnNames_ht = new Hashtable();
			foreach (iGCol aCol in theGrid.Cols)
				currentColumnNames_ht.Add(aCol.Key, aCol.Key);


            int destIdx = 0;
            for (int idx = 0; idx < numEntries; idx++) {
				String colName = orderKeys[idx];
				try {
                    if (currentColumnNames_ht.ContainsKey(colName) )
                        theGrid.Cols[colName].Move(destIdx++);

				} catch (Exception) {
				}

			}

		}  /*  End of  restoreIGridColumnOrder(iGrid, Hashtable)  method.  */





		/**
		 *  NCCiGridSortObjectStruct structure contaning iGrid sort information for a 
		 *  specific column.
		 *
		 *  <remarks>Since: NPA2.0</remarks>
		 */

		[Serializable]
		struct NCCiGridSortObjectStruct {
			public int ColumnIndex;
			public String ColumnName;
			public int SortIndex;
			public iGSortOrder SortOrder;
			public iGSortType SortType;


			/**
			 *  <summary>
			 *  Constructs a new NCCiGridSortObjectStruct structure using the specified column
			 *  name and sort information.
			 *  </summary>
			 *
			 *  <param name="columnName">iGrid Column name</param>
			 *  <param name="sortField">iGrid sort field information</param>
			 *  <remarks>Since: NPA2.0</remarks>
			 */
			public NCCiGridSortObjectStruct(String columnName, iGSortItem sortField) {
				this.ColumnName = columnName;
				this.ColumnIndex = sortField.ColIndex;
				this.SortIndex = sortField.Index;
				this.SortOrder = sortField.SortOrder;
				this.SortType = sortField.SortType;

			}  /*  End of  NCCiGridSortObjectStruct  constructor.  */

		};   /*  End of  struct  NCCiGridSortObjectStruct.  */



		/**
		 *  Comparer class for 2 iGrid columns.
		 *
		 *  <remarks>Since: NPA2.0</remarks>
		 */
		public class NCCiGridSortObjectSorter : IComparer {

			/**
			 *  <summary>
			 *  Compares two iGrid Column Sort Order objects.
			 *  </summary>
			 *
			 *  <param name="a">The first column sort oder object</param>
			 *  <param name="b">The second column sort order object</param>
			 *  <remarks>Since: NPA2.0</remarks>
			 */
			int IComparer.Compare(Object a, Object b) {
				int aInt = (int) a;
				int bInt = (int) b;

				return (aInt - bInt);

			}   /*  End of  Compare(Object, Object)  method.  */

		};   /*  End of  class  NCCiGridSortObjectSorter.  */



	    /**
		 *  <summary>
		 *  Gets a hashtable containing the iGrid sorting order. 
		 *  </summary>
		 *
		 *  <param name="theGrid">The iGrid Control</param>
		 *  <remarks>Since: NPA2.0</remarks>
		 */
		public static Hashtable getIGridSortOrder(iGrid theGrid) {
			Hashtable sortOrder_ht = new Hashtable();

			try {
				int numCols = theGrid.SortObject.Count;

				sortOrder_ht.Clear();
				for (int i = 0; i < numCols; i++) {
					iGSortItem sortItem = theGrid.SortObject[i];

					int    colIndex = sortItem.ColIndex;
					String colName  = theGrid.Cols[colIndex].Key;

					NCCiGridSortObjectStruct sortObj = new NCCiGridSortObjectStruct(colName, sortItem);
					sortOrder_ht.Add(sortItem.Index, sortObj);
				}

			} catch (Exception) {
			}

			return sortOrder_ht;

		}  /*  End of  getIGridSortOrder(iGrid)  method.  */



		/**
		 *  <summary>
		 *  Restores the iGrid sorting order using the specified hashtable.
		 *  </summary>
		 *
		 *  <param name="theGrid">The iGrid Control</param>
		 *  <param name="sortOrder_ht">The column sorting order</param>
		 *  <remarks>Since: NPA2.0</remarks>
		 */
		public static void restoreIGridSortOrder(iGrid theGrid, Hashtable sortOrder_ht) {
			if (null == sortOrder_ht)
				return;

			ArrayList  sortOrderKeyArrayList   = new ArrayList(sortOrder_ht.Keys);
			ArrayList  sortOrderValueArrayList = new ArrayList(sortOrder_ht.Values);

			Object[]   sortKeys   = sortOrderKeyArrayList.ToArray();
			Object[]   sortValues = sortOrderValueArrayList.ToArray();

			Array.Sort(sortKeys, sortValues, new NCCiGridSortObjectSorter() );

			foreach (Object de in sortKeys) {
				try {
					NCCiGridSortObjectStruct sortObj = (NCCiGridSortObjectStruct) sortOrder_ht[de];
					int colIndex = theGrid.Cols[sortObj.ColumnName].Index;
					theGrid.SortObject.Add(colIndex, sortObj.SortOrder, sortObj.SortType);

				} catch (Exception) {
				}

			}  /*  End of  FOR Loop  on all the sort keys.  */

		}  /*  End of  restoreIGridSortOrder(iGrid, Hashtable)  method.  */



        /**
		 *  <summary>
		 *  Creates a grid filter for selecting columns to display in the iGrid in question.
		 *  </summary>
		 *
		 *  <param name="theGrid">The iGrid</param>
		 *  <param name="filterTitle">The title to use for the Grid Filter</param>
		 *  <remarks>Since: NPA2.0</remarks>
		 */
        public static TrafodionIGridFilter createAGridFilter(TrafodionIGrid theGrid, String filterTitle) {            
            TrafodionIGridFilter theFilter = new TrafodionIGridFilter(theGrid);
            theFilter.Text = filterTitle;

            return theFilter;

        }  /*   End of  createGridFilter(iGrid, String)  method.  */



        /**
		 *  <summary>
		 *  Applies a grid filter (hide/show columns) to the iGrid in question.
		 *  </summary>
		 *
		 *  <param name="theGrid">The iGrid</param>
		 *  <param name="theFilter">The title to use for the Grid Filter</param>
		 *  <remarks>Since: NPA2.0</remarks>
		 */
        public static void applyGridFilter(iGrid theGrid, TrafodionIGridFilter theFilter) {
            if (null == theFilter)
                return;

            theGrid.Visible = false;

            Hashtable gridCols_ht = new Hashtable();
            for (int idx = 0; idx < theGrid.Cols.Count; idx++) {
                try { gridCols_ht.Add(theGrid.Cols[idx].Key, idx); } catch (Exception) { }

            }

            Hashtable showCols_ht = new Hashtable();
            List<string> allFilterItemKeys = theFilter.AllColumns;
            List<string> checkedFilterItemKeys = theFilter.CurrentVisibleColumns;
            for (int idx = 0; idx < allFilterItemKeys.Count; idx++)
            {
                String colName = allFilterItemKeys[idx];
                if (checkedFilterItemKeys.Contains(colName))
                    showCols_ht.Add(colName, idx);

                if (gridCols_ht.ContainsKey(colName)) {
                    try { 
                        theGrid.Cols[colName].Move(idx, 1); 
                    } catch (Exception) { }

                }

            }

            foreach (iGCol igc in theGrid.Cols)
                igc.Visible = showCols_ht.ContainsKey(igc.Key);

            theGrid.Header.AutoHeight();
            theGrid.Visible = true;
            
        }  /*   End of  applyGridFilter(iGrid, TrafodionIGridFilter)  method.  */

        //Checks to see if the filter is indeed for the grid passed. If all the columns in the grid are in the 
        //filter it's considered a match. 
        //private static bool IsFilterForGrid(iGrid theGrid, TrafodionIGridFilter theFilter)
        //{
        //    List<string> gridColumns = new List<string>();
        //    List<string> columnsInFilter = new List<string>();

        ////    int colCountInFilter = theFilter.CheckListBoxGridFilter.Items.Count;
        ////    int colCountInGrid = theGrid.Cols.Count;

        //    foreach(iGCol col in theGrid.Cols)
        //    {
        //        gridColumns.Add(col.Key);
        //    }
        //    for (int idx = 0; idx < theFilter.CheckListBoxGridFilter.Items.Count; idx++)
        //    {
        //        columnsInFilter.Add(theFilter.CheckListBoxGridFilter.Items[idx].ToString());
        //    }

        ////    if (colCountInFilter != colCountInGrid)
        ////    {
        ////        return false;
        ////    }

        ////    foreach (string colNameInFilter in columnsInFilter)
        ////    {
        ////        if (!gridColumns.Contains(colNameInFilter))
        ////        {
        ////            return false;
        ////        }
        ////    }

        //    foreach (string colName in gridColumns)
        //    {
        //        if (!columnsInFilter.Contains(colName))
        //        {
        //            return false;
        //        }
        //    }

        //    return true;
        //}
        

        /**
         *  <summary>
         *  Applies header formatter
         *  </summary>
         *
         *  <param name="theGrid">The iGrid</param>
         */
        public static void ApplyCustomHeaderFormatter(iGrid aGrid, IFormatProvider aFormatProvider)
        {
            if (aGrid != null)
            {
                foreach (iGCol igc in aGrid.Cols)
                    igc.ColHdrStyle.FormatProvider = aFormatProvider;
            }

        }  


		/**
		 *  <summary>
		 *  Fixes up a data table converting all the specified DateTime fields to the specified timezone 
		 *  for use with the iGrid.
		 *  </summary>	 *		 *  
		 *  <param name="theTable">The DataTable</param>
		 *  <param name="dateTimeFields">List of DateTime fields/columns in the DataTable</param>
		 *  <remarks>Since: NPA2.0</remarks>
		 */
        public static DataTable fixDataTableForGridDisplay(DataTable theTable,
                                                           String[] dateTimeFields)
        {

            DataTable dtCloned = Utilities.cloneDataTableColumns(theTable);
            foreach (DataColumn col in dtCloned.Columns)
                col.ColumnName = col.ColumnName.Replace('_', ' ');

            foreach (DataRow row in theTable.Rows)
            {
                DataRow newRow = dtCloned.NewRow();

                /*
                 *  Clone 'em. Copy all the items associated with the original row. 
                 */
                for (int i = 0; i < row.ItemArray.Length; i++)
                {
                    newRow[i] = row[i];
                }


                /*
                 *  Adjust all the DateTime columns we were "instructed" to change. 
                 */
                foreach (String colName in dateTimeFields)
                {
                    try
                    {
                        DateTime theTime = (DateTime)newRow[colName];
                        //newRow[colName] = ws.getDisplayTimeFromGMT(theTime);

                    }
                    catch (Exception)
                    {
                    }

                }  /*  End of  FOR loop  on all specified DateTime fields in the table.  */


                /*
                 *  Attack of the Clones!!
                 *  Add the cloned row to the clone table.
                 */
                dtCloned.Rows.Add(newRow);


            }  /*  End of  FOR loop  on all rows in the table.  */

            return dtCloned;

        }  /*  End of  fixDataTableForGridDisplay(NCCWorkspace, DataTable, String[])  method.  */


        
		#endregion  /*  End of  region  Public Methods.  */



	}  /*  End of  class  TrafodionIGridUtils.  */


}  

