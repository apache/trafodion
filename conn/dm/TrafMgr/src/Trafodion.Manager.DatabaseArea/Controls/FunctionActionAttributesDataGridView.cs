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

using System;
using System.Collections.Generic;
using System.Text;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public class FunctionActionAttributesDataGridView : RoutineAttributesDataGridView
    {
        public FunctionActionAttributesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionFunctionAction aTrafodionFunctionAction)
            : base(aDatabaseObjectsControl, aTrafodionFunctionAction)
        {
        }
        /// <summary>
        /// Gets the TrafodionFunctionAction
        /// </summary>
        public TrafodionFunctionAction TrafodionFunctionAction
        {
            get
            {
                return TrafodionObject as TrafodionFunctionAction;
            }
        }

        override protected void LoadRows()
        {
            TrafodionFunctionAction.LoadAttributes();

            //AddRow(Properties.Resources.UDRType, TrafodionFunctionAction.FormattedUDRType);

            //AddRow(Properties.Resources.Language, TrafodionFunctionAction.FormattedLanguageType);
            AddRow(Properties.Resources.Deterministic, TrafodionFunctionAction.FormattedDeterministic);
            //AddRow(Properties.Resources.Isolate, TrafodionFunctionAction.FormattedIsolate);
            //AddRow(Properties.Resources.ParameterStyle, TrafodionFunctionAction.FormattedParameterStyle);
            AddRow(Properties.Resources.TransactionAttributes, TrafodionFunctionAction.TransactionAttributes);
            //AddRow(Properties.Resources.ExternalPath, TrafodionFunctionAction.ExternalPath);
            //AddRow(Properties.Resources.ExternalName, TrafodionFunctionAction.ExternalName);

            AddRow(Properties.Resources.SQLAccess, TrafodionFunctionAction.FormattedSQLAccess);
            //AddRow(Properties.Resources.CallOnNull, TrafodionFunctionAction.CallOnNull);
            //AddRow(Properties.Resources.ParamStyleVersion, TrafodionFunctionAction.ParamStyleVersion);

            AddRow(Properties.Resources.InitialCUPCost, TrafodionFunctionAction.InitialCPUCost);
            AddRow(Properties.Resources.InitialIOCost, TrafodionFunctionAction.InitialIOCost);
            AddRow(Properties.Resources.InitialMSGCost, TrafodionFunctionAction.InitialMSGCost);
            //AddRow(Properties.Resources.NormalCUPCost, TrafodionFunctionAction.NormalCPUCost);
            //AddRow(Properties.Resources.NormalIOCost, TrafodionFunctionAction.NormalIOCost);
            //AddRow(Properties.Resources.NormalMSGCost, TrafodionFunctionAction.NormalMSGCost);

            //AddRow(Properties.Resources.IsUniversal, Utilities.YesNo(TrafodionFunctionAction.IsUniversal));
            AddRow(Properties.Resources.Parallelism, TrafodionFunctionAction.Parallelism);
            AddRow(Properties.Resources.CanCollapse, TrafodionFunctionAction.CanCollapse);
            AddRow(Properties.Resources.UserVersion, TrafodionFunctionAction.UserVersion);
            //AddRow(Properties.Resources.ExternalSecurity, TrafodionFunctionAction.ExternalSecurity);
            //AddRow(Properties.Resources.ActionPosition, TrafodionFunctionAction.ActionPosition);
            //AddRow(Properties.Resources.ExecuteMode, TrafodionFunctionAction.ExecutionMode);
        }
    }
}
