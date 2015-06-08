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
    /// Summary description for SequencesFolder.
    /// </summary>
    public class SequencesFolder : SchemaItemsFolder
    {
        /// <summary>
        /// Constructs a Sequences Folder
        /// </summary>
        /// <param name="aTrafodionSchema">The parent schema object</param>
        public SequencesFolder(TrafodionSchema aTrafodionSchema)
            : base(Properties.Resources.Sequences, aTrafodionSchema)
        {
        }

        /// <summary>
        /// Selects the Sequences folder in the Database navigation tree
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        override public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is TrafodionSequence)
            {
                DoPopulate(null);

                string theTargetInternalName = aTrafodionObject.InternalName;
                foreach (TreeNode theNode in Nodes)
                {
                    if (theNode is SequenceLeaf)
                    {
                        SequenceLeaf theSequenceLeaf = theNode as SequenceLeaf;
                        if (theSequenceLeaf.TrafodionSequence.InternalName.Equals(theTargetInternalName))
                        {
                            theSequenceLeaf.TreeView.SelectedNode = theSequenceLeaf;
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
            TheTrafodionSchema.TrafodionSequences = null;
        }

        protected override void PrepareForPopulate()
        {
            object c = TheTrafodionSchema.TrafodionSequences;
        }

        protected override void AddNodes()
        {
            foreach (TrafodionSequence theTrafodionSequence in TheTrafodionSchema.TrafodionSequences)
            {
                SequenceLeaf theSequenceItem = new SequenceLeaf(theTrafodionSequence);
                Nodes.Add(theSequenceItem);
            }
        }

        protected override void AddNodes(NavigationTreeNameFilter nameFilter)
        {
            foreach (TrafodionSequence theTrafodionSequence in TheTrafodionSchema.TrafodionSequences)
            {
                if (nameFilter.Matches(theTrafodionSequence.ExternalName))
                {
                    SequenceLeaf theSequenceItem = new SequenceLeaf(theTrafodionSequence);
                    Nodes.Add(theSequenceItem);
                }
            }
        }
    }
}
