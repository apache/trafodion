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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Datagrid for an Procedure's attributes
    /// </summary>
    public class ProcedureAttributesDataGridView : TrafodionSchemaObjectAttributesDataGridView
    {
        /// <summary>
        /// Constructor for ProcedureAttributesDataGridView
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionProcedure"></param>
        public ProcedureAttributesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionProcedure aTrafodionProcedure)
            : base(aDatabaseObjectsControl, aTrafodionProcedure)
        {
        }

        /// <summary>
        /// Gets the TrafodionProcedure
        /// </summary>
        public TrafodionProcedure TrafodionProcedure
        {
            get 
            {
                return TrafodionObject as TrafodionProcedure; 
            }
        }

        override protected void LoadRows()
        {
            AddRow(Properties.Resources.DynamicResultSets, TrafodionProcedure.FormattedDynamicResultSets);
            AddRow(Properties.Resources.SQLAccess, TrafodionProcedure.FormattedSQLAccess);
            AddRow(Properties.Resources.Signature, TrafodionProcedure.FormattedSignature);
            if (ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                //M7
                if (TrafodionProcedure.LibraryUID > 0)
                {
                    AddRow(Properties.Resources.Library, TrafodionProcedure.FormattedLibraryName);
                }
                else
                {
                    AddRow(Properties.Resources.ExternalPath, TrafodionProcedure.FormattedExternalPath);
                }
            }
            else
            {
                //M6
                AddRow(Properties.Resources.ExternalPath, TrafodionProcedure.FormattedExternalPath);
            }   
            
            AddRow(Properties.Resources.ExternalFile, TrafodionProcedure.FormattedExternaClassFileName);
            AddRow(Properties.Resources.ExternalName, TrafodionProcedure.FormattedMethodName);
            AddRow(Properties.Resources.Language, TrafodionProcedure.FormattedLanguageType);
            AddRow(Properties.Resources.ParameterStyle, TrafodionProcedure.FormattedParameterStyle);
            AddRow(Properties.Resources.Deterministic, TrafodionProcedure.FormattedDeterministic);
            AddRow(Properties.Resources.Isolate, TrafodionProcedure.FormattedIsolate);
            AddRow(Properties.Resources.TransactionAttributes, TrafodionProcedure.TransactionAttributes);
            AddRow(Properties.Resources.ExternalSecurity, TrafodionProcedure.FormattedExternalSecurity);
        }

        #region ICloneToWindow

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        override public Control Clone()
        {
            return new ProcedureAttributesDataGridView(null, TrafodionProcedure);
        }

        #endregion
    }
}
