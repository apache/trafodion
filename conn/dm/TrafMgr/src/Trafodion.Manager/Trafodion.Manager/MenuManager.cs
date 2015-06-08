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
using System.Collections;
using System.Collections.Generic;
using System.Text;
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework
{
    public class MenuManager
    {
        /// <summary>
        /// Given a menu provider, this method iterates over the TrafodionToolStripMenuItem returns
        /// and appropriately populates the menu strip        
        /// It removes all menus that don't pertain to the main area
        /// It then adds menus that pertain to the menu provider. The menus will be added in the same order
        /// as was returned by the menu provider.
        /// </summary>
        /// <param name="aMenuProvder"></param>
        /// <param name="aMenuStrip"></param>
        /// <param name="defaultMenu"></param>
        public void PopulateMainMenuBar(IMenuProvider aMenuProvder, TrafodionMenuStrip aMenuStrip, MainMenu defaultMenu)
        {
            //Clean up the merged items
            if (aMenuStrip != null)
            {
                ToolStripManager.RevertMerge(aMenuStrip);
            }

            //Add the default elements
            System.Windows.Forms.ToolStripManager.Merge(defaultMenu.TheMainMenuBar, aMenuStrip);

            //add the new menus from the area
            if (aMenuProvder != null)
            {
                TrafodionMenuStrip menuItemsToBeAdded = aMenuProvder.GetMenuItems(new ImmutableMenuStripWrapper(aMenuStrip));
                if ((aMenuProvder != null) && (aMenuStrip != null) && (menuItemsToBeAdded != null))
                {
                    ToolStripManager.Merge(menuItemsToBeAdded, aMenuStrip);
                }
            }
        }
    }

}
