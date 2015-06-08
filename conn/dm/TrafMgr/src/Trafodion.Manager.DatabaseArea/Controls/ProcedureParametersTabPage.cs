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
    /// A TabPage that displays a Procedure's parameters.
    /// </summary>
  public class ProcedureParametersTabPage : DelayedPopulateClonableTabPage
  {
        private TrafodionProcedure _sqlMxProcedure;
        /// <summary>
        /// The procedure object whose parameters are displayed in this tab page
        /// </summary>
        public TrafodionProcedure TrafodionProcedure
        {
            get { return _sqlMxProcedure; }
        }

        /// <summary>
        /// Create a TabPage that displays a table's parameters.
        /// </summary>
        /// <param name="aTrafodionProcedurePanel">The procedure whose columns are to be displayed.</param>
        public ProcedureParametersTabPage(TrafodionProcedure aTrafodionProcedure)
            : base(Properties.Resources.Parameters)
        {
            _sqlMxProcedure = aTrafodionProcedure;
            
        }

        public override void PrepareForPopulate()
        {
            object a = _sqlMxProcedure.Columns;
        }
        /// <summary>
        /// Constructs a procedure parameters panel and adds it to the procedure parameters tab page
        /// </summary>
        override protected void Populate()
        {
            Controls.Clear();

            // Create the panel and fill this tab page with it.
            ProcedureParametersPanel ProcedureParametersPanel = new ProcedureParametersPanel(TrafodionProcedure);
            ProcedureParametersPanel.Dock = DockStyle.Fill;
            Controls.Add(ProcedureParametersPanel);
        }

   }


}
