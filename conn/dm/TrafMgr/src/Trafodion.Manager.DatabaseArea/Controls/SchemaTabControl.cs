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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public class SchemaTabControl : TrafodionTabControl
    {

        public SchemaTabControl(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionSchema aTrafodionSchema)
        {
            //Console.WriteLine("schema tab control enter : " + DateTime.Now.ToString("yyyy-MM-dd hh':'mm':'ss.FFFFFF"));

            TabPages.Add(new SchemaAttributesTabPage(aDatabaseObjectsControl, aTrafodionSchema));

            // first arg aDatabaseObjectsControl is passed to be handy because of --
            // hyperlinks so that right pane knows about left side tree-view 
            // The last arg is the name of the Tab from resource e.g. "Tables"  etc.
            TabPages.Add(new TablesTabPage(aDatabaseObjectsControl, aTrafodionSchema,  Properties.Resources.Tables)); 
            //TabPages.Add(new MaterializedViewsTabPage(aDatabaseObjectsControl, aTrafodionSchema, Properties.Resources.MaterializedViews)); 
            //TabPages.Add(new MaterializedViewGroupsTabPage(aDatabaseObjectsControl, aTrafodionSchema, Properties.Resources.MaterializedViewGroups)); 
            TabPages.Add(new ViewsTabPage(aDatabaseObjectsControl, aTrafodionSchema, Properties.Resources.Views));
            TabPages.Add(new TrafodionIndexesTabPage(aDatabaseObjectsControl, aTrafodionSchema));
            TabPages.Add(new LibrariesTabPage(aDatabaseObjectsControl, aTrafodionSchema, Properties.Resources.Libraries));
            TabPages.Add(new ProceduresTabPage(aDatabaseObjectsControl, aTrafodionSchema, Properties.Resources.Procedures));            
            TabPages.Add(new SequencesTabPage(aDatabaseObjectsControl, aTrafodionSchema, Properties.Resources.Sequences));
            //if (aTrafodionSchema.Version >= 2500 && aTrafodionSchema.TrafodionUDFunctions.Count > 0)
            //{
            //    TabPages.Add(new FunctionsTabPage(aDatabaseObjectsControl, aTrafodionSchema, Properties.Resources.UserDefinedFunctions));
            //}
            //if (aTrafodionSchema.Version >= 2500 && aTrafodionSchema.TrafodionFunctionActions.Count > 0)
            //{
            //    TabPages.Add(new TrafodionFunctionActionsTabPage(aDatabaseObjectsControl,aTrafodionSchema));
            //}
            //if (aTrafodionSchema.Version >= 2500 && aTrafodionSchema.TrafodionTableMappingFunctions.Count > 0)
            //{
            //    TabPages.Add(new TableMappingFunctionsTabPage(aDatabaseObjectsControl, aTrafodionSchema, Properties.Resources.TableMappingFunctions));
            //}
            //TabPages.Add(new SynonymsTabPage(aDatabaseObjectsControl, aTrafodionSchema, Properties.Resources.Synonyms));
            //TabPages.Add(new TrafodionObjectDDLTabPage(aTrafodionSchema));
           //TabPages.Add(new SchemaPrivilegesTabPage(aTrafodionSchema));
            //TabPages.Add(new SchemaSpaceUsagesTabPage(aDatabaseObjectsControl, aTrafodionSchema));
           //Console.WriteLine("schema tab control leave : " + DateTime.Now.ToString("yyyy-MM-dd hh':'mm':'ss.FFFFFF"));


        }

    }
}
