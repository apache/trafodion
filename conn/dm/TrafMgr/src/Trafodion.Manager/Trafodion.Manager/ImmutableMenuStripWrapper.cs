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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework
{    
    /// <summary>
    /// This class shall be used to wrap the MenuStrip reference and pass around
    /// without allowing unauthorized updates to it.
    /// </summary>
    public class ImmutableMenuStripWrapper
    {
        private TrafodionMenuStrip _theMenuStrip;
        public ImmutableMenuStripWrapper(TrafodionMenuStrip aMenuStrip)
        {
            _theMenuStrip = aMenuStrip;
        }

        /// <summary>
        /// Given a menu item name, finds the first occurance of that item in the menu strip 
        /// and finds the index of that item under it's parent. 
        /// </summary>
        /// <param name="menuName"></param>
        /// <returns></returns>
        public int getMenuIndex(String menuName)
        {
            ToolStripItem aMenuItem = findMenuItem(menuName);
            return getMenuIndex(aMenuItem);
        }

        /// <summary>
        /// Given a menu item returns it's index at it's parent's level
        /// </summary>
        /// <param name="aMenuItem"></param>
        /// <returns></returns>
        private int getMenuIndex(ToolStripItem aMenuItem)
        {
            if (aMenuItem != null)
            {
                ToolStripMenuItem parent = aMenuItem.OwnerItem as ToolStripMenuItem;
                if (parent != null)
                {
                    return parent.DropDownItems.IndexOf(aMenuItem);
                }
                else
                {
                    return _theMenuStrip.Items.IndexOf(aMenuItem);
                }
            }
            return -1;
        }

        /// <summary>
        /// Finds a menu item from the menu tool strip using recurssion
        /// </summary>
        /// <param name="menuName"></param>
        /// <returns></returns>
        private ToolStripMenuItem findMenuItem(String menuName)
        {
            if (_theMenuStrip != null)
            {
                ToolStripMenuItem menuItem = _theMenuStrip.Items[menuName] as ToolStripMenuItem;
                if (menuItem != null)
                {
                    return menuItem;
                }
                else
                {
                    foreach (ToolStripItem tsi in _theMenuStrip.Items)
                    {
                        ToolStripMenuItem tempMenuItem = tsi as ToolStripMenuItem;
                        if (tempMenuItem != null)
                        {
                            return findMenuItem(tempMenuItem, menuName);
                        }
                    }
                }
            }
            return null;
        }

        /// <summary>
        /// Part of the regression call
        /// </summary>
        /// <param name="aMenuItem"></param>
        /// <param name="menuName"></param>
        /// <returns></returns>
        private ToolStripMenuItem findMenuItem(ToolStripMenuItem aMenuItem, String menuName)
        {
            if (aMenuItem != null)
            {
                ToolStripMenuItem menuItem = aMenuItem.DropDownItems[menuName] as ToolStripMenuItem;
                if (menuItem != null)
                {
                    return menuItem;
                }
                else
                {
                    foreach (ToolStripItem tsi in aMenuItem.DropDownItems)
                    {
                        ToolStripMenuItem tempMenuItem = tsi as ToolStripMenuItem;
                        if (tempMenuItem != null)
                        {
                            return findMenuItem(tempMenuItem, menuName);
                        }
                    }
                }
            }
            return null;
        }
    
    }
}
