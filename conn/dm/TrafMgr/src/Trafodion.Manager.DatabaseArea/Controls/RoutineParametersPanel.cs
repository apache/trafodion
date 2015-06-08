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
    public class RoutineParametersPanel : TrafodionObjectPanel
    {
        // Member Variables
        private RoutineParametersDataGridView theDataGridView;

        /// <summary>
        /// Constructor for ProcedureParametersPanel
        /// </summary>
        /// <param name="aTrafodionProcedure"></param>
        public RoutineParametersPanel(TrafodionRoutine aTrafodionRoutine)
            : base(Properties.Resources.Parameters, aTrafodionRoutine)
        {
            Load();
        }

        /// <summary>
        /// Loads the data grid view and puts it into the controls
        /// </summary>
        private void Load()
        {
            theDataGridView = new RoutineParametersDataGridView(null, (TrafodionRoutine)TheTrafodionObject);
            theDataGridView.TheHeaderText = WindowTitle;
            theDataGridView.Dock = DockStyle.Fill;
            Controls.Add(theDataGridView);
            if (TheTrafodionObject is TrafodionUDFunction)
            {
                theDataGridView.AddCountControlToParent(Properties.Resources.ThisFunctionasNParameters, DockStyle.Top);
            }
            else if (TheTrafodionObject is TrafodionFunctionAction)
            {
                theDataGridView.AddCountControlToParent(Properties.Resources.ThisFunctionActionasNParameters, DockStyle.Top);
            }
            else if (TheTrafodionObject is TrafodionTableMappingFunction)
            {
                theDataGridView.AddCountControlToParent(Properties.Resources.ThisTableMappingFunctionasNParameters, DockStyle.Top);
            }
            else
            {
                theDataGridView.AddCountControlToParent(Properties.Resources.ThisProcedureasNParameters, DockStyle.Top);
            }
            theDataGridView.AddButtonControlToParent(DockStyle.Bottom);
        }

        /// <summary>
        /// Clones the panel
        /// </summary>
        /// <returns></returns>
        override public Control Clone()
        {
            return new RoutineParametersPanel((TrafodionRoutine)TheTrafodionObject);
        }
         
    }
}
