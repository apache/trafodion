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

using System;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public class TrafodionFunctionActionsTabPage : SchemaLevelObjectListTabPage
    {
        private TrafodionObject _sqlMxObject;

        public TrafodionFunctionActionsTabPage(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionSchema aTrafodionSchema)
            : base(aDatabaseObjectsControl, aTrafodionSchema, Properties.Resources.FunctionActions)
        {
            _headerText = Properties.Resources.ThisSchemaHasNObjects + Text;
            _sqlMxObject = aTrafodionSchema;
        }

        public TrafodionFunctionActionsTabPage(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionUDFunction aTrafodionUDFunction)
            : base(aDatabaseObjectsControl, aTrafodionUDFunction.TheTrafodionSchema, Properties.Resources.FunctionActions)
        {
            _headerText = Properties.Resources.ThereAreNFunctionActions;
            _sqlMxObject = aTrafodionUDFunction;
        }


        public override void PrepareForPopulate()
        {
            if (_sqlMxObject is TrafodionSchema)
            {
                object a = (_sqlMxObject as TrafodionSchema).TrafodionFunctionActions;
            }
            else if (_sqlMxObject is TrafodionUDFunction)
            {
                object a = (_sqlMxObject as TrafodionUDFunction).TrafodionFunctionActions;
            }            
        }

        protected override void Populate()
        {
            FunctionActionsPanel functionActionsPanel;
            if (_sqlMxObject is TrafodionSchema)
            {
                functionActionsPanel = new FunctionActionsPanel(
                TheDatabaseObjectsControl,
                _sqlMxObject as TrafodionSchema,
                (_sqlMxObject as TrafodionSchema).TrafodionFunctionActions,
                _headerText);
            }
            else if (_sqlMxObject is TrafodionUDFunction)
            {
                functionActionsPanel = new FunctionActionsPanel(
                TheDatabaseObjectsControl,
                _sqlMxObject as TrafodionUDFunction,
                (_sqlMxObject as TrafodionUDFunction).TrafodionFunctionActions,
                _headerText);
            }
            else
            {
                // This is unexpected and indicates a coding error.
                throw new ApplicationException();
            }

            functionActionsPanel.Dock = DockStyle.Fill;

            Controls.Clear();
            Controls.Add(functionActionsPanel);            
        }
    }
}
