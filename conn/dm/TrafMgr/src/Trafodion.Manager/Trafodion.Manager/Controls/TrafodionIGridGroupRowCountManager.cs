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
using System;
using System.Collections.Generic;
using System.Text;
using TenTec.Windows.iGridLib;
using System.Drawing;

namespace Trafodion.Manager.Framework.Controls
{
    /// <summary>
    /// This class is to help display grouped row count in an TrafodionIGrid
    /// </summary>
    public class TrafodionIGridGroupRowCountManager
	{
		private TrafodionIGrid _theGrid;
		private string _theTotalColKey;
		private Font _theTotalFont;
		private Brush _theGroupTextBrush;
        private CustomDrawCellForeGroundDelegate _theCustomDrawCellForeGroundDelegate = null;

        public delegate void CustomDrawCellForeGroundDelegate(object sender, iGCustomDrawCellEventArgs e);

        /// <summary>
        /// To attach an Grid to this row count manager
        /// </summary>
        /// <param name="grid"></param>
        /// <param name="totalColKey"></param>
        public void Attach(TrafodionIGrid grid, string totalColKey, CustomDrawCellForeGroundDelegate customDrawCellDelegate)
		{
            _theGrid = grid;
            _theTotalColKey = totalColKey;
            _theCustomDrawCellForeGroundDelegate = customDrawCellDelegate;

			// The next loop causes all group rows cells be custom drawn
            foreach (iGCellStyle grcs in _theGrid.GroupRowLevelStyles)
                grcs.CustomDrawFlags = iGCustomDrawFlags.Foreground;

            _theTotalFont = new Font(_theGrid.Font, FontStyle.Bold);
            _theGroupTextBrush = new SolidBrush(_theGrid.ForeColor);

            _theGrid.AfterContentsGrouped += new EventHandler(TheGrid_AfterContentsGrouped);
            _theGrid.CustomDrawCellForeground += new iGCustomDrawCellEventHandler(TheGrid_CustomDrawCellForeground);
		}

		void TheGrid_AfterContentsGrouped(object sender, EventArgs e)
		{
            if (_theGrid.Rows.Count == 0) return;

			// The array to store totals by levels;
			// the last row is always a row with normal cells with max available level:
            int maxGroupLevel = _theGrid.Rows[_theGrid.Rows.Count - 1].Level - 1;
			int[] LevelSum = new int[maxGroupLevel + 1];
			int count = 0;

            for (int iRow = _theGrid.Rows.Count - 1; iRow >= 0; iRow--)
			{
                iGRow curRow = _theGrid.Rows[iRow];
				switch (curRow.Type)
				{
					case iGRowType.AutoGroupRow:
						if (curRow.Level == maxGroupLevel)
						{
							for (int i = 0; i <= maxGroupLevel; i++)
								LevelSum[i] += count;  // total count for the level
						}
						// We store the total count for this group row directly in the cell of the total column
						// because its contents isn't displayed on the screen:
                        curRow.Cells[_theTotalColKey].Value = LevelSum[curRow.Level];
						LevelSum[curRow.Level] = 0;
						count = 0;
						break;

					case iGRowType.Normal:
						if (curRow.Visible)
						{
                            count++;
						}
						break;
				}
			}
		}

        /// <summary>
        /// To display the grouped row count
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		void TheGrid_CustomDrawCellForeground(object sender, iGCustomDrawCellEventArgs e)
		{
            if (_theCustomDrawCellForeGroundDelegate == null)
            {
                string myGroupText = (string)_theGrid.Cells[e.RowIndex, e.ColIndex].Value;
                object myTotalValue = _theGrid.Cells[e.RowIndex, _theTotalColKey].Value;
                if (myTotalValue != null)
                {
                    myGroupText = String.Format("{0}  ({1})", myGroupText, myTotalValue);
                }

                e.Graphics.DrawString(myGroupText, _theTotalFont, _theGroupTextBrush, e.Bounds);
            }
            else
            {
                _theCustomDrawCellForeGroundDelegate(sender, e);
            }
		}
	}
}
