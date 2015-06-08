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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework
{
    /// <summary>
    /// This class shall be used to wrap the ToolStrip reference and pass around
    /// without allowing unauthorized updates to it.
    /// </summary>
    public class ImmutableToolStripWrapper
    {
        private TrafodionToolStrip _theToolStrip;

        public ImmutableToolStripWrapper(TrafodionToolStrip aToolStrip)
        {
            _theToolStrip = aToolStrip;
        }

        /// <summary>
        /// Given a toolstrtip item name, finds the first occurance of that item in the tool strip 
        /// and finds the index of that item under it's parent. 
        /// </summary>
        /// <param name="menuName"></param>
        /// <returns></returns>
        public int getToolStripItemIndex(String toolStripItemName)
        {
            ToolStripItem aToolStripItem = findToolStripItem(toolStripItemName);
            return getToolStripItemIndex(aToolStripItem);
        }

        /// <summary>
        /// Given a toolstrip item returns it's index at it's parent's level
        /// </summary>
        /// <param name="aMenuItem"></param>
        /// <returns></returns>
        private int getToolStripItemIndex(ToolStripItem aToolStripItem)
        {
            if (aToolStripItem != null)
            {
                return _theToolStrip.Items.IndexOf(aToolStripItem);
            }
            return -1;
        }

        /// <summary>
        /// Finds a toolstrip item from the menu tool strip using recurssion
        /// </summary>
        /// <param name="menuName"></param>
        /// <returns></returns>
        private ToolStripItem findToolStripItem(String toolStripName)
        {
            if (_theToolStrip != null)
            {
                ToolStripItem aToolStripItem = _theToolStrip.Items[toolStripName] as ToolStripItem;
                if (aToolStripItem != null)
                {
                    return aToolStripItem;
                }
            }
            return null;
        }
    }
}
