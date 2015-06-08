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
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{
    public class TrafodionToolStripMenuItem : ToolStripMenuItem, ITrafodionMenuItem
    {
        #region Members

        /// <summary>
        /// Menus can either belog to the framework or to a specifc area. A true value
        /// of this flag will indicate that the menu belongs to the framework.
        /// </summary>
        private bool _IsFrameworkMenu = false;


        #endregion

        #region Properties
        public bool IsFrameworkMenu
        {
            get { return _IsFrameworkMenu; }
            set { _IsFrameworkMenu = value; }
        }



        #endregion

        #region Constructors
        public TrafodionToolStripMenuItem()
            : base()
        {
        }
        public TrafodionToolStripMenuItem(Image aImage)
            : base(aImage)
        {
        }

        public TrafodionToolStripMenuItem(String aText, Image aImage)
            : base(aText, aImage)
        {
        }

        public TrafodionToolStripMenuItem(String aText, Image aImage, EventHandler onClick)
            : base(aText, aImage, onClick)
        {
        }

        public TrafodionToolStripMenuItem(String aText, Image aImage, ToolStripItem[] toolStripItems) : base(aText, aImage, toolStripItems)
        {
        }

        public TrafodionToolStripMenuItem(String aText, Image aImage, EventHandler onClick, String aName)
            : base(aText, aImage, onClick, aName)
        {
        }

        public TrafodionToolStripMenuItem(string aText, Image aImage, EventHandler onClick, Keys shortcutKeys)
            : base(aText, aImage, onClick, shortcutKeys)
        {
        }
        #endregion


    }
}
