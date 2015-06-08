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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
	/// <summary>
	/// Summary description for TableTriggerLeaf.
	/// </summary>
    /// 
    public class TableTriggerLeaf : DatabaseTreeNode
	{
        /// <summary>
        /// Constructor for Table Trigger Leaf
        /// </summary>
        /// <param name="aTrafodionTableIndex"></param>
        public TableTriggerLeaf(TrafodionTrigger aTrafodionTableTrigger)
            : base(aTrafodionTableTrigger)
		{
            ImageKey = DatabaseTreeView.DB_TRIGGER_ICON;
            SelectedImageKey = DatabaseTreeView.DB_TRIGGER_ICON;
		}

        /// <summary>
        /// Accessor for the TrafodionIndex object
        /// </summary>
        public TrafodionTrigger TrafodionTrigger
        {
            get { return (TrafodionTrigger)this.TrafodionObject; }
        }

    }
}
