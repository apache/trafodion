//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
namespace Trafodion.Manager.MetricMiner.Controls
{
    public partial class TrafodionIGridHtmlRowDisplay : UserControl
    {

        private iGrid _theIGrid;
        private int _theCurrentRow = -1;
        public TrafodionIGridHtmlRowDisplay()
        {
            InitializeComponent();
            enableDisableButton();
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
                _theIGrid.SetCurRow(_theCurrentRow);                

                StringBuilder sb = new StringBuilder();
                sb.AppendLine("<html><head></head><body><table width=\"100%\">");
                sb.AppendLine(getTableHeader());
                for (int col = 0; col < _theIGrid.Cols.Count; col++)
                {
                    iGColHdr colHeader = _theIGrid.Header.Cells[0, col];
                    string headerTxt = colHeader.Value.ToString();
                    object value = _theIGrid.Cells[aRow, col].Value;
                    if ((headerTxt != null) && (headerTxt.Trim().Length > 0))
                    {
                        sb.AppendLine(getTableRow(headerTxt, ((value != null) ? value.ToString() : "&nbsp;"), col));
                    }
                }
                sb.AppendLine("</table></body></html>");
                _theBrowser.DocumentText = sb.ToString();
                enableDisableButton();
            }

        }

                //        table.Columns.Add(new DataColumn("Column Header"));
                //table.Columns.Add(new DataColumn("Data"));

        private string getTableHeader()
        {
            return string.Format("<tr><th style=\"font-family:Tahoma; font-weight:bold; font-size:12px; background-color:Silver; \" width=\"30%\" align=\"left\" valign=\"top\" >{0}</th><th style=\"font-family:Tahoma; font-weight:bold; font-size:12px; background-color:Silver; \"  width=\"70%\" align=\"left\" valign=\"top\" >{1}</th></tr>", "Column Header", "Data");
        }

        private string getTableRow(string key, string value, int row)
        {
            if ((row % 2) == 0)
            {
                return string.Format("<tr><td style=\"font-family:Tahoma; font-weight:bold; font-size:12px;\" width=\"30%\" align=\"left\" valign=\"top\" >{0}</td><td style=\"font-family:Tahoma; font-size:12px; \"  width=\"70%\" align=\"left\" valign=\"top\" >{1}</td></tr>", key, value);
            }
            else
            {
                return string.Format("<tr><td style=\"font-family:Tahoma; font-weight:bold; font-size:12px; background-color:WhiteSmoke; \" width=\"30%\" align=\"left\" valign=\"top\" >{0}</td><td style=\"font-family:Tahoma; font-size:12px;  background-color:WhiteSmoke; \"  width=\"70%\" align=\"left\" valign=\"top\" >{1}</td></tr>", key, value);
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
            int totalRows = (_theIGrid != null) ? _theIGrid.Rows.Count : 0;
            _theFirstRowButton.Enabled = (_theCurrentRow > 1);
            _theLastRowButton.Enabled = (_theCurrentRow < (totalRows - 2));
            _thePreviousRowButton.Enabled = (_theCurrentRow > 0);
            _theNextRowButton.Enabled = (_theCurrentRow < (totalRows - 1));
            _theCounterLabel.Text = String.Format("Row {0} of {1}", _theCurrentRow + 1, totalRows);
        }
    }
}
