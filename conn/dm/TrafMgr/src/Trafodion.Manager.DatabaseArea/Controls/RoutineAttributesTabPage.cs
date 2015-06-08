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
    /// A TabPage that displays a Routine's attributes.
    /// </summary>
    public class RoutineAttributesTabPage : DelayedPopulateClonableTabPage
    {
        private TrafodionRoutine _sqlMxRoutine;
        /// <summary>
        /// The routine object whose attributes are displayed in this tab page
        /// </summary>
        public TrafodionRoutine TrafodionRoutine
        {
            get { return _sqlMxRoutine; }
        }

        /// <summary>
        /// Create a TabPage that displays a routine's attributes.
        /// </summary>
        /// <param name="aTrafodionProcedurePanel">The routine whose columns are to be displayed.</param>
        public RoutineAttributesTabPage(TrafodionRoutine aTrafodionRoutine)
            : base(Properties.Resources.Attributes)
        {
            _sqlMxRoutine = aTrafodionRoutine;
        }
        /// <summary>
        /// Constructs a routine attributes panel and adds it to the routine attributes tab page
        /// </summary>
        override protected void Populate()
        {
            Controls.Clear();

            // Create the panel and fill this tab page with it.
            RoutineAttributesPanel ProcedureAttributesPanel = new RoutineAttributesPanel(TrafodionRoutine);
            ProcedureAttributesPanel.Dock = DockStyle.Fill;
            Controls.Add(ProcedureAttributesPanel);
        }

    }


}
