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
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    class TableMappingFunctionAttributesDataGridView : RoutineAttributesDataGridView
    {
        public TableMappingFunctionAttributesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionTableMappingFunction aTrafodionTableMappingFunction)
            : base(aDatabaseObjectsControl, aTrafodionTableMappingFunction)
        {
        }
        /// <summary>
        /// Gets the TrafodionTableMappingFunction
        /// </summary>
        public TrafodionTableMappingFunction TrafodionTableMappingFunction
        {
            get
            {
                return TrafodionObject as TrafodionTableMappingFunction;
            }
        }

        override protected void LoadRows()
        {
            TrafodionTableMappingFunction.LoadAttributes();

            AddRow(Properties.Resources.UDRType, TrafodionTableMappingFunction.FormattedUDRType);
            AddRow(Properties.Resources.DynamicResultSets, TrafodionTableMappingFunction.FormattedDynamicResultSets);
            AddRow(Properties.Resources.SQLAccess, TrafodionTableMappingFunction.FormattedSQLAccess);
            AddRow(Properties.Resources.Language, TrafodionTableMappingFunction.FormattedLanguageType);
            AddRow(Properties.Resources.Deterministic, TrafodionTableMappingFunction.FormattedDeterministic);
            AddRow(Properties.Resources.Isolate, TrafodionTableMappingFunction.FormattedIsolate);
            AddRow(Properties.Resources.ParameterStyle, TrafodionTableMappingFunction.FormattedParameterStyle);
            AddRow(Properties.Resources.TransactionAttributes, TrafodionTableMappingFunction.TransactionAttributes);
            AddRow(Properties.Resources.ExternalPath, TrafodionTableMappingFunction.ExternalPath);
            AddRow(Properties.Resources.ExternalName, TrafodionTableMappingFunction.ExternalName);

            AddRow(Properties.Resources.InitialCUPCost, TrafodionTableMappingFunction.InitialCPUCost);
            AddRow(Properties.Resources.InitialIOCost, TrafodionTableMappingFunction.InitialIOCost);
            AddRow(Properties.Resources.InitialMSGCost, TrafodionTableMappingFunction.InitialMSGCost);
            AddRow(Properties.Resources.NormalCUPCost, TrafodionTableMappingFunction.NormalCPUCost);
            AddRow(Properties.Resources.NormalIOCost, TrafodionTableMappingFunction.NormalIOCost);
            AddRow(Properties.Resources.NormalMSGCost, TrafodionTableMappingFunction.NormalMSGCost);

            AddRow(Properties.Resources.Language, TrafodionTableMappingFunction.FormattedLanguageType);
            AddRow(Properties.Resources.Deterministic, TrafodionTableMappingFunction.FormattedDeterministic);
            AddRow(Properties.Resources.Isolate, TrafodionTableMappingFunction.FormattedIsolate);
            AddRow(Properties.Resources.ParameterStyle, TrafodionTableMappingFunction.FormattedParameterStyle);
            AddRow(Properties.Resources.TransactionAttributes, TrafodionTableMappingFunction.TransactionAttributes);
            AddRow(Properties.Resources.ExternalPath, TrafodionTableMappingFunction.ExternalPath);
            AddRow(Properties.Resources.ExternalName, TrafodionTableMappingFunction.ExternalName);

        }
    }
}
