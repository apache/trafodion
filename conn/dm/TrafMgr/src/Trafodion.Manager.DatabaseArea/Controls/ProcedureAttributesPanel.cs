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
    /// Panel for an Procedure's Attributes
    /// </summary>
    public class ProcedureAttributesPanel : ProcedurePanel
    {
        // Member Variables
        private ProcedureAttributesDataGridView theDataGridView;

        /// <summary>
        /// Constructor for ProcedureAttributesPanel
        /// </summary>
        /// <param name="aTrafodionProcedure"></param>
        public ProcedureAttributesPanel(TrafodionProcedure aTrafodionProcedure)
            : base(Properties.Resources.Attributes, aTrafodionProcedure)
        {
            Load();
        }

        /// <summary>
        /// Loads the data grid view and puts it into the controls
        /// </summary>
        override protected void Load()
        {
            theDataGridView = new ProcedureAttributesDataGridView(null, TrafodionProcedure);
            theDataGridView.TheHeaderText = WindowTitle;
            theDataGridView.Dock = DockStyle.Fill;
            Controls.Add(theDataGridView);
            theDataGridView.AddButtonControlToParent(DockStyle.Bottom);
        }

        /// <summary>
        /// Clones the panel
        /// </summary>
        /// <returns></returns>
        override public Control Clone()
        {
            return new ProcedureAttributesPanel(TrafodionProcedure);
        }
         
    }

}
