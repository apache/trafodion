//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A TabPage that displays a Library's attributes.
    /// </summary>
    public class LibraryAttributesTabPage : DelayedPopulateClonableTabPage
    {
        private TrafodionLibrary _sqlMxLibrary;

        /// <summary>
        /// The library object whose attributes are displayed in this tab page
        /// </summary>
        public TrafodionLibrary TrafodionLibrary
        {
            get { return _sqlMxLibrary; }
        }

        /// <summary>
        /// Create a TabPage that displays a Library's attributes.
        /// </summary>
        /// <param name="aTrafodionLibraryPanel">The library whose columns are to be displayed.</param>
        public LibraryAttributesTabPage(TrafodionLibrary aTrafodionLibrary)
            : base(Properties.Resources.Attributes)
        {
            _sqlMxLibrary = aTrafodionLibrary;
        }

        public override void PrepareForPopulate()
        {
            object a = _sqlMxLibrary.ClientCodeFileName;
        }
        /// <summary>
        /// Constructs a library attributes panel and adds it to the library attributes tab page
        /// </summary>
        override protected void Populate()
        {
            Controls.Clear();

            // Create the panel and fill this tab page with it.
            LibraryAttributesPanel LibraryAttributesPanel = new LibraryAttributesPanel(TrafodionLibrary);
            LibraryAttributesPanel.Dock = DockStyle.Fill;
            Controls.Add(LibraryAttributesPanel);
        }
    }
}
