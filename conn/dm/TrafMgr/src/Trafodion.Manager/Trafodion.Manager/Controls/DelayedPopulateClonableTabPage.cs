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

using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;


namespace Trafodion.Manager.Framework.Controls
{
    /// <summary>
    /// Used by Tabs to be able to display tab-name in Italics to indicate that the panel is Clonable
    /// </summary>
    public class DelayedPopulateClonableTabPage : DelayedPopulateTabPage
    {

        /// <summary>
        /// Default constructor for the UI designer.
        /// </summary>
        public DelayedPopulateClonableTabPage()
        {
        }

        
        /// <summary>
        /// Constuctor
        /// </summary>
        /// <param name="aTabName">The name to appear on the tab e.g. "Tables" </param>
        public DelayedPopulateClonableTabPage(string aTabName)
            : base(aTabName)
        {
            // to take care of "Italics" in tab-name
            ClonablePanel theClonablePanel = new ClonablePanel();
            theClonablePanel.Dock = DockStyle.Fill;
            Controls.Add(theClonablePanel);

        }
    }
}
