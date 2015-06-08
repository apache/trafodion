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
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{

    /// <summary>
    /// A TabPage that displays a Trigger's attributes.
    /// </summary>
    public class TriggerAttributesTabPage : DelayedPopulateClonableTabPage
    {
        private TrafodionTrigger _sqlMxTrigger;
        /// <summary>
        /// The trigger object whose attributes are displayed in this tab page
        /// </summary>
        public TrafodionTrigger TrafodionTrigger
        {
            get { return _sqlMxTrigger; }
        }

        /// <summary>
        /// Create a TabPage that displays a table's attributes.
        /// </summary>
        /// <param name="aTrafodionTriggerPanel">The trigger whose columns are to be displayed.</param>
        public TriggerAttributesTabPage(TrafodionTrigger aTrafodionTrigger)
            : base(Properties.Resources.Attributes)
        {
            _sqlMxTrigger = aTrafodionTrigger;
        }

        public override void PrepareForPopulate()
        {
            object a = _sqlMxTrigger.FormattedOperation;
        }

        /// <summary>
        /// Constructs a trigger attributes panel and adds it to the trigger attributes tab page
        /// </summary>
        override protected void Populate()
        {
            Controls.Clear();

            // Create the panel and fill this tab page with it.
            TriggerAttributesPanel TriggerAttributesPanel = new TriggerAttributesPanel(TrafodionTrigger);
            TriggerAttributesPanel.Dock = DockStyle.Fill;
            Controls.Add(TriggerAttributesPanel);
        }

   }


}
