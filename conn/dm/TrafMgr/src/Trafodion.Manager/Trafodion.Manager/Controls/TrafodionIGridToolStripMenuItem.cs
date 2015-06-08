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
using System.Drawing;
using System.Data;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
namespace Trafodion.Manager.Framework.Controls
{
    public class TrafodionIGridToolStripMenuItem : ToolStripMenuItem
    {
        TrafodionIGridEventObject _theTrafodionIGridEventObject;

        public TrafodionIGridEventObject TrafodionIGridEventObject
        {
            get { return _theTrafodionIGridEventObject; }
            set { _theTrafodionIGridEventObject = value; }
        }

    }


    /// <summary>
    /// This method has the attributes that has the user click information
    /// </summary>
    public class TrafodionIGridEventObject
    {
        TrafodionIGrid _theGrid;
        int row = -1;
        int col = -1;
        Point _thePointOfClick;

        public Point PointOfClick
        {
            get { return _thePointOfClick; }
            set { _thePointOfClick = value; }
        }

        public int Row
        {
            get { return row; }
            set { row = value; }
        }

        public int Col
        {
            get { return col; }
            set { col = value; }
        }

        public TrafodionIGrid TheGrid
        {
            get { return _theGrid; }
            set { _theGrid = value; }
        }
    }
}
