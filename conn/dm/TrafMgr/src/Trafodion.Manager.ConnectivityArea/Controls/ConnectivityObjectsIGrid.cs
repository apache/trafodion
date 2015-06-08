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
using System.Data;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.ConnectivityArea.Controls.Tree;
using Trafodion.Manager.ConnectivityArea.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    class ConnectivityObjectsIGrid<T> : TrafodionIGrid where T: NDCSObject
    {
        private NavigationTreeView _treeView = null;
        private iGCellStyle linkForegroundStyle;
        private int objectLinkColumnNumber = -1;

        public int ObjectLinkColumnNumber
        {
            get { return objectLinkColumnNumber; }
            set 
            { 
                objectLinkColumnNumber = value;
                if (value >= 0)
                {
                    //BeginUpdate();
                    //Cols[value].CellStyle = linkForegroundStyle;
                    //EndUpdate();
                }
            }
        }

        public NavigationTreeView TreeView
        {
            get { return _treeView; }
            set { _treeView = value; }
        }

        public ConnectivityObjectsIGrid(string licenseKey)
            : base(licenseKey)
        {
            InitializeGrid();
        }

        public ConnectivityObjectsIGrid()
            : base("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0ATQB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAIABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==")
        {
            InitializeGrid();
        }

        public ConnectivityObjectsIGrid(NavigationTreeView navigationTreeView)
            :base("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0ATQB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAIABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==")
        {
            _treeView = navigationTreeView;
            InitializeGrid();
        }

        public void InitializeGrid()
        {
            this.linkForegroundStyle = new iGCellStyle(true);
            this.linkForegroundStyle.CustomDrawFlags = iGCustomDrawFlags.Foreground;
            this.linkForegroundStyle.ForeColor = Color.Blue;
            this.linkForegroundStyle.DropDownControl = null;
            this.linkForegroundStyle.EmptyStringAs = iGEmptyStringAs.Null;
            this.linkForegroundStyle.Flags = iGCellFlags.DisplayText;
            this.linkForegroundStyle.ImageList = null;
            this.linkForegroundStyle.ValueType = null;

        }

        void ConnectivityObjectsIGrid_CellMouseEnter(object sender, iGCellMouseEnterLeaveEventArgs e)
        {
            Invalidate(e.Bounds);
        }

        void ConnectivityObjectsIGrid_CellMouseLeave(object sender, iGCellMouseEnterLeaveEventArgs e)
        {
            Invalidate(e.Bounds);
        }

        void ConnectivityObjectsIGrid_CustomDrawCellForeground(object sender, iGCustomDrawCellEventArgs e)
        {
            if (objectLinkColumnNumber >= 0 && e.ColIndex == objectLinkColumnNumber)
            {
                // Retrieve the client location of the mouse pointer.
                Font font = new Font(this.Font, FontStyle.Underline);
                Point theCursorPosition = PointToClient(Cursor.Position);

                if (e.Bounds.Contains(theCursorPosition))
                {
                    e.Graphics.DrawString(Cells[e.RowIndex, e.ColIndex].Text,
                        font, Brushes.Blue,
                        new Rectangle(e.Bounds.X, e.Bounds.Y, e.Bounds.Width, e.Bounds.Height));

                }
                else
                {
                    e.Graphics.DrawString(Cells[e.RowIndex, e.ColIndex].Text,
                        font, SystemBrushes.ControlText,
                        new Rectangle(e.Bounds.X, e.Bounds.Y, e.Bounds.Width, e.Bounds.Height));
                }
            }
        }
    }
}
