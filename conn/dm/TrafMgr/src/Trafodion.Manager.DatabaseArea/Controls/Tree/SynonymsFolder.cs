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
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Summary description for SynonymsFolder.
    /// </summary>
    public class SynonymsFolder : SchemaItemsFolder
    {
        /// <summary>
        /// Constructs a Synonyms Folder
        /// </summary>
        /// <param name="aTrafodionSchema">The parent schema object</param>
        public SynonymsFolder(TrafodionSchema aTrafodionSchema)
            : base(Properties.Resources.Synonyms, aTrafodionSchema)
        {
        }

        /// <summary>
        /// Selects the Synonyms folder in the Database navigation tree
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        override public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is TrafodionSynonym)
            {
                DoPopulate(null);

                string theTargetInternalName = aTrafodionObject.InternalName;
                foreach (TreeNode theNode in Nodes)
                {
                    if (theNode is SynonymLeaf)
                    {
                        SynonymLeaf theSynonymLeaf = theNode as SynonymLeaf;
                        if (theSynonymLeaf.TrafodionSynonym.InternalName.Equals(theTargetInternalName))
                        {
                            theSynonymLeaf.TreeView.SelectedNode = theSynonymLeaf;
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        /// <summary>
        /// Resets the synonyms list in the parent schema. The list will be populated when it is accessed next
        /// </summary>
        /// <param name="aNameFilter">A navigation tree filter</param>
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TheTrafodionSchema.TrafodionSynonyms = null;
        }

        protected override void PrepareForPopulate()
        {
            object c = TheTrafodionSchema.TrafodionSynonyms;
        }

        protected override void AddNodes()
        {
            foreach (TrafodionSynonym theTrafodionSynonym in TheTrafodionSchema.TrafodionSynonyms)
            {
                SynonymLeaf theSynonymItem = new SynonymLeaf(theTrafodionSynonym);
                Nodes.Add(theSynonymItem);
            }
        }

        protected override void AddNodes(NavigationTreeNameFilter nameFilter)
        {
            foreach (TrafodionSynonym theTrafodionSynonym in TheTrafodionSchema.TrafodionSynonyms)
            {
                if (nameFilter.Matches(theTrafodionSynonym.ExternalName))
                {
                    SynonymLeaf theSynonymItem = new SynonymLeaf(theTrafodionSynonym);
                    Nodes.Add(theSynonymItem);
                }
            }
        }
    }
}
