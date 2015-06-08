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
    abstract public class TableItemsFolder : DatabaseTreeFolder, IHasTrafodionSchema, IHasTrafodionTable
    {
        /// <summary>
        /// Constructor for the Table Items Folder
        /// </summary>
        /// <param name="aText">the Text</param>
        /// <param name="aTrafodionTable">the TrafodionTable</param>
        public TableItemsFolder(string aText, TrafodionTable aTrafodionTable)
            :base(aTrafodionTable, true)
        {
            Text = aText;
        }

        /// <summary>
        /// Selects the TrafodionObject when clicked on the tree
        /// </summary>
        /// <param name="aTrafodionObject">the TrafodionObject this table folder refers to</param>
        /// <returns>boolean value indicating success</returns>
        public abstract bool SelectTrafodionObject(TrafodionObject aTrafodionObject);

        /// <summary>
        /// Returns the short description
        /// </summary>
        override public string ShortDescription
        {
            get
            {
                return Text + " in " + TrafodionTable.ExternalName;
            }
        }

        /// <summary>
        /// Returns the long description
        /// </summary>
        override public string LongerDescription
        {
            get
            {
                return Text + " in Table " + TrafodionTable.VisibleAnsiName;
            }
        }

        private TrafodionSchema theTrafodionSchema;

        public TrafodionTable TrafodionTable
        {
            get { return (TrafodionTable)this.TrafodionObject; }
        }

        public TrafodionSchema TheTrafodionSchema
        {
            get { return TrafodionTable.TheTrafodionSchema; }
        }

    }

}
