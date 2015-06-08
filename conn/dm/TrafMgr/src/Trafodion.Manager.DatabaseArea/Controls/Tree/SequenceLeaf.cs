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
	/// Summary description for SequenceLeaf.
	/// </summary>
    public class SequenceLeaf : DatabaseTreeNode
	{
        public SequenceLeaf(TrafodionSequence aTrafodionSequence)
            : base(aTrafodionSequence)
		{
            ImageKey = DatabaseTreeView.DB_SYNONYM_ICON;
            SelectedImageKey = DatabaseTreeView.DB_SYNONYM_ICON;
        }

        public TrafodionSequence TrafodionSequence
		{
            get { return (TrafodionSequence)this.TrafodionObject; }
		}

		override public string LongerDescription
		{
			get
			{
                return "Sequence " + TrafodionSequence.VisibleAnsiName;
			}
		}
	}
}
