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

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Summary description for SchemaItemFolder.
    /// </summary>
    abstract public class MaterializedViewItemsFolder : DatabaseTreeFolder, IHasTrafodionSchema
    {
        /// <summary>
        /// Constructs the folder to hold child objects for a materialized view
        /// </summary>
        /// <param name="aText"></param>
        /// <param name="aTrafodionMaterializedView"></param>
        public MaterializedViewItemsFolder(string aText, TrafodionMaterializedView aTrafodionMaterializedView)
            :base (aTrafodionMaterializedView, true)
        {
            Text = aText;
        }

        /// <summary>
        /// Selects the child object contained in this folder
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        public abstract bool SelectTrafodionObject(TrafodionObject aTrafodionObject);

        /// <summary>
        /// Short description
        /// </summary>
        override public string ShortDescription
        {
            get
            {
                return Text + " in " + TrafodionMaterializedView.ExternalName;
            }
        }

        /// <summary>
        /// Long description
        /// </summary>
        override public string LongerDescription
        {
            get
            {
                return Text + " in Materialized View " + TrafodionMaterializedView.VisibleAnsiName;
            }
        }

        /// <summary>
        /// The materialized view to which the child objects belong to
        /// </summary>
        public TrafodionMaterializedView TrafodionMaterializedView
        {
            get { return (TrafodionMaterializedView)TrafodionObject;  }
        }

        /// <summary>
        /// The schema in which the materialized view is defined
        /// </summary>
        public TrafodionSchema TheTrafodionSchema
        {
            get { return TrafodionMaterializedView.TheTrafodionSchema; }
        }

    }

}
