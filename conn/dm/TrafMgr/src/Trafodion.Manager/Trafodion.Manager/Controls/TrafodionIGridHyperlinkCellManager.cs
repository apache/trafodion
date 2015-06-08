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
using TenTec.Windows.iGridLib;
using System.Drawing;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{
    /// <summary>
    /// 
    /// </summary>
	public class TrafodionIGridHyperlinkCellManager
	{
        private TrafodionIGrid _TrafodionIGrid;
		private Font _iGridHyperlinkFont;
        private int _iGridHyperlinkColIndex;
        private string _iGridHyperlinkColKey;

        // Event Handlers 
        private iGCellDynamicFormattingEventHandler _cellDynamicFormattingEventHandler = null;
        private iGCellMouseEnterLeaveEventHandler _cellMouseEnterEventHandler = null;
        private iGCellMouseEnterLeaveEventHandler _cellMouseLeaveEventHandler = null;
        private iGCellClickEventHandler _cellClickHandler = null;


        /// <summary>
        /// Making iGrid with hyper links by Column name
        /// </summary>
        /// <param name="grid"></param>
        /// <param name="hyperLinkColKey"></param>
        public void Attach(TrafodionIGrid grid, int hyperLinkColIndex)
        {
            _TrafodionIGrid = grid;
            if (grid.Cols.Count > 0)
            {
                if (hyperLinkColIndex >= 0)
                {
                    _iGridHyperlinkColKey = _TrafodionIGrid.Cols[hyperLinkColIndex].Key;
                }

                _iGridHyperlinkFont = new Font(_TrafodionIGrid.Font, FontStyle.Underline);

                _cellDynamicFormattingEventHandler = new iGCellDynamicFormattingEventHandler(TrafodionIGrid_CellDynamicFormatting);
                _TrafodionIGrid.CellDynamicFormatting += _cellDynamicFormattingEventHandler;

                _cellMouseEnterEventHandler = new iGCellMouseEnterLeaveEventHandler(TrafodionIGrid_CellMouseEnter);
                _TrafodionIGrid.CellMouseEnter += _cellMouseEnterEventHandler;

                _cellMouseLeaveEventHandler = new iGCellMouseEnterLeaveEventHandler(TrafodionIGrid_CellMouseLeave);
                _TrafodionIGrid.CellMouseLeave += _cellMouseLeaveEventHandler;

                _cellClickHandler = new iGCellClickEventHandler(TrafodionIGrid_CellClick);
                _TrafodionIGrid.CellClick += _cellClickHandler;
            }
        }

        /// <summary>
        /// Clean up Event Handlers
        /// </summary>
        public void Detach()
        {
            if (_cellDynamicFormattingEventHandler != null)
            {
                _TrafodionIGrid.CellDynamicFormatting -= _cellDynamicFormattingEventHandler;
            }

            if (_cellMouseEnterEventHandler != null)
            {
                _TrafodionIGrid.CellMouseEnter -= _cellMouseEnterEventHandler;
            }

            if (_cellMouseLeaveEventHandler != null)
            {
                _TrafodionIGrid.CellMouseLeave -= _cellMouseLeaveEventHandler;
            }

            if (_cellClickHandler != null)
            {
                _TrafodionIGrid.CellClick -= _cellClickHandler;
            }

        }

        private bool CellIsHyperlink(iGCell cell)
		{
            if (_iGridHyperlinkColIndex == -1)
                return false;
            else
                return (cell.ColKey == _iGridHyperlinkColKey) ? true : false;
		}

        void TrafodionIGrid_CellDynamicFormatting(object sender, iGCellDynamicFormattingEventArgs e)
		{
            if (CellIsHyperlink(_TrafodionIGrid.Cells[e.RowIndex, e.ColIndex]))
			{
                e.Font = _iGridHyperlinkFont;
			}
		}

        void TrafodionIGrid_CellMouseEnter(object sender, iGCellMouseEnterLeaveEventArgs e)
		{
            if (CellIsHyperlink(_TrafodionIGrid.Cells[e.RowIndex, e.ColIndex]))
            {
                _TrafodionIGrid.Cursor = Cursors.Hand;
                _TrafodionIGrid.Cells[e.RowIndex, e.ColIndex].ForeColor = Color.Blue;
            }
        }

        void TrafodionIGrid_CellMouseLeave(object sender, iGCellMouseEnterLeaveEventArgs e)
		{
            if (CellIsHyperlink(_TrafodionIGrid.Cells[e.RowIndex, e.ColIndex]))
            {
                _TrafodionIGrid.Cells[e.RowIndex, e.ColIndex].ForeColor = System.Drawing.SystemColors.WindowText;
            }
            
            _TrafodionIGrid.Cursor = Cursors.Default;
		}


        void TrafodionIGrid_CellClick(object sender, iGCellClickEventArgs e)
		{
            // can be either handled here or by the object
		}

	}
}
