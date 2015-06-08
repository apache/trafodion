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
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.Framework.Controls
{
    public partial class TrafodionIGridRowDisplay : UserControl
    {
        private iGrid _theIGrid;
        private int _theCurrentRow = -1;
        public TrafodionIGridRowDisplay()
        {
            InitializeComponent();
            this.Resize += new EventHandler(TrafodionIGridRowDisplay_Resize);
            _theRowDataGrid.RowMode = false;
            _theRowDataGrid.SelectionMode = iGSelectionMode.MultiExtended;
            _theRowDataGrid.ColWidthChanging += new iGColWidthEventHandler(_theRowDataGrid_ColWidthChanging);
        }

        void TrafodionIGridRowDisplay_Resize(object sender, EventArgs e)
        {
            if (_theRowDataGrid.Rows != null)
            {
                _theRowDataGrid.Rows.AutoHeight();
            }
        }

        void _theRowDataGrid_ColWidthChanging(object sender, iGColWidthEventArgs e)
        {
            if (_theRowDataGrid.Rows != null)
            {
                _theRowDataGrid.Rows.AutoHeight();
            }
        }

        public void ShowRowDetails(iGrid aGrid, int row)
        {
            
            if ((aGrid != null) && (row >= 0))
            {
                _theIGrid = aGrid;  
              
                ShowRowData(row);

            }
        }

        private void ShowRowData(int aRow)
        {
            if ((aRow >= 0) && (aRow <= (_theIGrid.Rows.Count - 1)))
            {
                _theCurrentRow = aRow;
                DataTable table = new DataTable();
                table.Columns.Add(new DataColumn("Column Header"));
                table.Columns.Add(new DataColumn("Data"));
                iGHdrRow headerRow = _theIGrid.Header.Rows[0];
                for (int col = 0; col < _theIGrid.Cols.Count; col++)
                {
                    iGColHdr colHeader = _theIGrid.Header.Cells[0, col];
                    string headerTxt = (colHeader.Value == null) ? "" : colHeader.Value.ToString();
                    object value = _theIGrid.Cells[aRow, col].Text;
                    DataRow dataRow = table.NewRow();

                    //New lines in the header are being removed so that they show correctly on the row
                    dataRow[0] = TrafodionIGridUtils.ConvertBreakToBlank(headerTxt); 

                    dataRow[1] = value;
                    table.Rows.Add(dataRow);
                    
                }
                this._theRowDataGrid.Clear();
                _theRowDataGrid.DefaultCol.CellStyle.TextFormatFlags = iGStringFormatFlags.WordWrap;
                this._theRowDataGrid.FillWithData(table);
                _theRowDataGrid.BeginUpdate();
                //_theRowDataGrid.ResizeGridColumns(table, 7, 20);
                _theRowDataGrid.AutoResizeCols = true;
                _theRowDataGrid.AutoWidthColMode = iGAutoWidthColMode.HeaderAndCells;
                _theRowDataGrid.Cols.AutoWidth();
                _theRowDataGrid.Rows.AutoHeight();
                _theRowDataGrid.EndUpdate();
                enableDisableButton();
            }

        }

        private void _theFirstRowButton_Click(object sender, EventArgs e)
        {
            ShowRowData(0);
        }

        private void _thePreviousRowButton_Click(object sender, EventArgs e)
        {
            ShowRowData(_theCurrentRow - 1);
        }

        private void _theNextRowButton_Click(object sender, EventArgs e)
        {
            ShowRowData(_theCurrentRow + 1);
        }

        private void _theLastRowButton_Click(object sender, EventArgs e)
        {
             ShowRowData(_theIGrid.Rows.Count - 1);
        }

        private void enableDisableButton()
        {
            int totalRows = _theIGrid.Rows.Count;
            _theFirstRowButton.Enabled = (_theCurrentRow > 1);
            _theLastRowButton.Enabled = (_theCurrentRow < (totalRows - 2));
            _thePreviousRowButton.Enabled = (_theCurrentRow > 0);
            _theNextRowButton.Enabled = (_theCurrentRow < (totalRows - 1));
            _theCounterLabel.Text = String.Format("Row {0} of {1}", _theCurrentRow + 1, totalRows);
        }
    }
}
