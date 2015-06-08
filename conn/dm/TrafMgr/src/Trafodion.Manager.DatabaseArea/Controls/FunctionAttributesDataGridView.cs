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
    class FunctionAttributesDataGridView : RoutineAttributesDataGridView
    {
        public FunctionAttributesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionUDFunction aTrafodionUDFunction)
            : base(aDatabaseObjectsControl, aTrafodionUDFunction)
        {
        }
        /// <summary>
        /// Gets the TrafodionUDFunction
        /// </summary>
        public TrafodionUDFunction TrafodionUDFunction
        {
            get
            {
                return TrafodionObject as TrafodionUDFunction;
            }
        }

        override protected void LoadRows()
        {
            TrafodionUDFunction.LoadAttributes();
            AddRow(Properties.Resources.UDRType, TrafodionUDFunction.FormattedUDRType);

            AddRow(Properties.Resources.Language, TrafodionUDFunction.FormattedLanguageType);
            AddRow(Properties.Resources.Deterministic, TrafodionUDFunction.FormattedDeterministic);
            AddRow(Properties.Resources.Isolate, TrafodionUDFunction.FormattedIsolate);
            AddRow(Properties.Resources.ParameterStyle, TrafodionUDFunction.FormattedParameterStyle);
            AddRow(Properties.Resources.TransactionAttributes, TrafodionUDFunction.TransactionAttributes);
            AddRow(Properties.Resources.ExternalPath, TrafodionUDFunction.ExternalPath);
            AddRow(Properties.Resources.ExternalName, TrafodionUDFunction.ExternalName);

            AddRow(Properties.Resources.SQLAccess, TrafodionUDFunction.FormattedSQLAccess);
            AddRow(Properties.Resources.CallOnNull, TrafodionUDFunction.CallOnNull);
            AddRow(Properties.Resources.ParamStyleVersion, TrafodionUDFunction.ParamStyleVersion);

            AddRow(Properties.Resources.InitialCUPCost, TrafodionUDFunction.InitialCPUCost);
            AddRow(Properties.Resources.InitialIOCost, TrafodionUDFunction.InitialIOCost);
            AddRow(Properties.Resources.InitialMSGCost, TrafodionUDFunction.InitialMSGCost);
            AddRow(Properties.Resources.NormalCUPCost, TrafodionUDFunction.NormalCPUCost);
            AddRow(Properties.Resources.NormalIOCost, TrafodionUDFunction.NormalIOCost);
            AddRow(Properties.Resources.NormalMSGCost, TrafodionUDFunction.NormalMSGCost);
            if (!TrafodionUDFunction.IsUniversal)
            {
                AddRow(Properties.Resources.IsUniversal, Utilities.YesNo(TrafodionUDFunction.IsUniversal));
            }
            AddRow(Properties.Resources.Parallelism, TrafodionUDFunction.Parallelism);
            AddRow(Properties.Resources.CanCollapse, TrafodionUDFunction.CanCollapse);
            AddRow(Properties.Resources.UserVersion, TrafodionUDFunction.UserVersion);
            AddRow(Properties.Resources.ExternalSecurity, TrafodionUDFunction.ExternalSecurity);
            if (TrafodionUDFunction.IsUniversal)
            {
                AddRow(Properties.Resources.ActionPosition, TrafodionUDFunction.ActionPosition);
            }
            AddRow(Properties.Resources.ExecuteMode, TrafodionUDFunction.ExecutionMode);
        }
    }
}
