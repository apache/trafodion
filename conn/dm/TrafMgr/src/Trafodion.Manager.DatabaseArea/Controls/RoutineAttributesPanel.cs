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
    /// Panel for an Routine's Attributes
    /// </summary>
    public class RoutineAttributesPanel : RoutinePanel
    {
        // Member Variables
        private RoutineAttributesDataGridView theDataGridView;

        /// <summary>
        /// Constructor for RoutineAttributesPanel
        /// </summary>
        /// <param name="aTrafodionProcedure"></param>
        public RoutineAttributesPanel(TrafodionRoutine aTrafodionRoutine)
            : base(Properties.Resources.Attributes, aTrafodionRoutine)
        {
            Load();
        }

        /// <summary>
        /// Loads the data grid view and puts it into the controls
        /// </summary>
        override protected void Load()
        {
            if (TrafodionRoutine is TrafodionUDFunction)
            {
                theDataGridView = new FunctionAttributesDataGridView(null, TrafodionRoutine as TrafodionUDFunction);
            }
            else if (TrafodionRoutine is TrafodionFunctionAction)
            {
                theDataGridView = new FunctionActionAttributesDataGridView(null, TrafodionRoutine as TrafodionFunctionAction);
            }
            else if (TrafodionRoutine is TrafodionTableMappingFunction)
            {
                theDataGridView = new TableMappingFunctionAttributesDataGridView(null, TrafodionRoutine as TrafodionTableMappingFunction);
            }
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
            return new RoutineAttributesPanel(TrafodionRoutine);
        }

    }

}
