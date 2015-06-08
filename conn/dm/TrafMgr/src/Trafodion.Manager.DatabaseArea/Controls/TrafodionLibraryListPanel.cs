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

using System.Collections.Generic;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A panel to display a list of schema objects
    /// </summary>
    public class TrafodionLibraryListPanel : TrafodionSchemaObjectListPanel<TrafodionLibrary>
    {  
        /// <summary>
        /// Constructs a generic panel to display a list of TrafodionLibrary
        /// </summary>
        /// <param name="aDatabaseObjectsControl">The control that has reference to the DatabaseTreeView</param>
        /// <param name="aHeaderText">The text to display as the header for the object list</param>
        /// <param name="parentTrafodionObject">The parent sql object in whose context, we are displaying this list</param>
        /// <param name="sqlMxObjects">The list of TrafodionLibrary that need to be displayed</param>
        /// <param name="aTitle"></param>
        public TrafodionLibraryListPanel(DatabaseObjectsControl aDatabaseObjectsControl, string aHeaderText, TrafodionObject parentTrafodionObject, 
                    List<TrafodionLibrary> sqlMxObjects, string aTitle) : base(aDatabaseObjectsControl, aHeaderText, parentTrafodionObject, sqlMxObjects, aTitle)
        {
        }
        
        protected override void AddGridColumn()
        {
            this.grid.Cols.Add("Name", Properties.Resources.Name);
            this.grid.Cols.Add("Type", "Type");
            this.grid.Cols.Add("UID", Properties.Resources.MetadataUID);
            this.grid.Cols.Add("CreateTime", Properties.Resources.CreationTime);
            this.grid.Cols.Add("RedefTime", Properties.Resources.RedefinitionTime);
        }

        protected override object[] ExtractValues(TrafodionSchemaObject sqlMxSchemaObject)
        {
            TrafodionLibrary sqlMxLibrary = (TrafodionLibrary)sqlMxSchemaObject;
            return new object[] {
                            sqlMxLibrary.ExternalName,
                            sqlMxLibrary.IsMetadataObject?"System":"User",
                            sqlMxLibrary.UID,                            
                            sqlMxLibrary.FormattedCreateTime(),
                            sqlMxLibrary.FormattedRedefTime()
                        };
        }
    }

}
