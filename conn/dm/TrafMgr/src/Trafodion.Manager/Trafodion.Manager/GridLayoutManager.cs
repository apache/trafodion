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
using System.Drawing;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework
{
    /// <summary>
    /// This layout manager shall be used to layout the containers in a grid on the canvas.
    /// </summary>
    public class GridLayoutManager : ILayoutManager
    {
        #region  Member Variables
        
        private int _rows = 1;
        private int _columns = 1;
        private int _cellSpacing = 0;
        private int lastRow = -1;
        private int lastColumn = -1;

        #endregion  /*  End of region  Member Variables.  */

        #region  Properties
        /// <summary>
        /// Rows in the grid
        /// </summary>
        public int Rows
        {
            get { return _rows; }
            set { _rows = value; }
        }

        /// <summary>
        /// Columns in the grid
        /// </summary>
        public int Columns
        {
            get { return _columns; }
            set { _columns = value; }
        }

        /// <summary>
        /// Spacing between each grid
        /// </summary>
        public int CellSpacing
        {
            get { return _cellSpacing; }
            set { _cellSpacing = value; }
        }
        #endregion  /*  End of region  Properties.  */

        #region  Constructors
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="rows"></param>
        /// <param name="cols"></param>
        public GridLayoutManager(int rows, int cols)
        {
            this.Rows = rows;
            this.Columns = cols;
        }
        #endregion  /*  End of region  Constructors.  */
        
        #region  Public methods
        /// <summary>
        /// Adds the container to the canvas. The container is added after the last cell in the grid.
        /// </summary>
        /// <param name="canvas"></param>
        /// <param name="container"></param>
        /// <param name="index"></param>
        void ILayoutManager.addComponent(WidgetCanvas canvas, WidgetContainer container, int index)
        {
            int row = (lastRow < 0) ? 0 : lastRow;
            int col = 0;
            col =  (lastColumn < (Columns - 1)) ? (lastColumn + 1) : 0;
            row = (col < lastColumn) ? (row + 1) : row;
            addComponent(canvas, container, row, col, 1, 1);
        }

        /// <summary>
        /// Adds the container to the canvas at a position specified by the layout constraint location
        /// </summary>
        /// <param name="canvas"></param>
        /// <param name="container"></param>
        /// <param name="aLayoutConstraint"></param>
        /// <param name="index"></param>
        void ILayoutManager.addComponent(WidgetCanvas canvas, WidgetContainer container, Object aLayoutConstraint, int index)
        {
            GridConstraint gc = aLayoutConstraint as GridConstraint;
            if (gc != null)
            {
                addComponent(canvas, container, gc.Row, gc.Col, gc.RowSpan, gc.ColSpan);
            }
            else
            {
                ((ILayoutManager)this).addComponent(canvas, container, index);
            }
       
        }


        void ILayoutManager.LayoutComponent(WidgetCanvas canvas, WidgetContainer container, Object aLayoutConstraint, int index)
        {
            GridConstraint gc = aLayoutConstraint as GridConstraint;
            if (gc != null)
            {
                positionComponent(canvas, container, gc.Row , gc.Col, gc.RowSpan, gc.ColSpan);
            }
        }

        #endregion  /*  End of region Public methods.  */

        #region  Private methods
        private void addComponent(WidgetCanvas canvas, WidgetContainer container, int row, int col, int rowSpan, int colSpan)
        {
            Boolean sizeChanged = false;
            if (row > Rows)
            {
                Rows = row + 1;
                sizeChanged = true;
            }
            if (col > Columns)
            {
                Columns = col + 1;
                sizeChanged = true;
            }


            //Layout the component that is being added
            Rectangle parentDisplayRectangle = canvas.ClientRectangle;
            Point loc = new Point();
            int gridWidth = (parentDisplayRectangle.Width / Columns);
            int gridHeight = (parentDisplayRectangle.Height / Rows);
            loc.X = gridWidth * col;
            loc.Y = gridHeight * row;

            container.Size = new Size((gridWidth * colSpan) - this.CellSpacing, (gridHeight * rowSpan) - this.CellSpacing);
            container.Location = loc;

            canvas.Controls.Add(container);

            lastRow = row;
            lastColumn = col;
        }

        private void positionComponent(WidgetCanvas canvas, WidgetContainer container, int row, int col, int rowSpan, int colSpan)
        {
            Boolean sizeChanged = false;
            if (row > Rows)
            {
                Rows = row + 1;
                sizeChanged = true;
            }
            if (col > Columns)
            {
                Columns = col + 1;
                sizeChanged = true;
            }


            //Layout the component that is being added
            Rectangle parentDisplayRectangle = canvas.ClientRectangle;
            Point loc = new Point();
            int gridWidth = (parentDisplayRectangle.Width / Columns);
            int gridHeight = (parentDisplayRectangle.Height / Rows);
            loc.X = gridWidth * col;
            loc.Y = gridHeight * row;

            container.Size = new Size((gridWidth * colSpan) - this.CellSpacing, (gridHeight * rowSpan) - this.CellSpacing);
            container.Location = loc;

            lastRow = row;
            lastColumn = col;
        }
        #endregion  /*  End of region Private methods.  */
    }
}
