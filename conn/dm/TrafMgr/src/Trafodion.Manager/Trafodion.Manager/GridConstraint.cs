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

namespace Trafodion.Manager.Framework
{
    /// <summary>
    /// Can be used to specify how a container will be placed on the canvas
    /// </summary>
    [Serializable]
    public class GridConstraint
    {
        #region  Member Variables

        private int row;
        private int col;
        private int rowSpan;
        private int colSpan;

        #endregion  /*  End of region  Member Variables.  */

        #region  Properties
        /// <summary>
        /// The row where the widget is to be placed
        /// </summary>
        public int Row
        {
            get { return row; }
            set { row = (value < 0) ? 0 : value;  }
        }

        /// <summary>
        /// The column where the widget is to be placed
        /// </summary>
        public int Col
        {
            get { return col; }
            set { col = (value < 0) ? 0 : value; }
        }

        /// <summary>
        /// Number of rows this widget should span
        /// </summary>
        public int RowSpan
        {
            get { return rowSpan; }
            set { rowSpan = (value < 0) ? 1 : value; }
        }

        /// <summary>
        /// Number of columns this widget should span
        /// </summary>
        public int ColSpan
        {
            get { return colSpan; }
            set { colSpan = (value < 0) ? 1 : value; }
        }
        #endregion  /*  End of region  Properties.  */

        #region  Constructors
        public GridConstraint(int row, int col, int rowSpan, int colSpan)
        {
            this.Row = row;
            this.Col = col;
            this.RowSpan = rowSpan;
            this.ColSpan = colSpan;
        }
        #endregion  /*  End of region  Constructors.  */
    }
}
