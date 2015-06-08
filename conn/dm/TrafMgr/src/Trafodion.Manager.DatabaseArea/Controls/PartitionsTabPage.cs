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

namespace Trafodion.Manager.DatabaseArea.Controls
{

    /// <summary>
    /// A tabpage that contains a panel that shows the partition info for any partitoned object.
    /// </summary>
    class PartitionsTabPage : DelayedPopulateClonableTabPage
    {
        private PartitionedSchemaObject thePartitionedSchemaObject;

        /// <summary>
        /// The partitioned schema object property.  Changing it to a new value will not have an effect
        /// until the Refresh() method is called.  We don't call it directly because we want this page to be delayed.
        /// </summary>
        public PartitionedSchemaObject ThePartitionedSchemaObject
        {
            get { return thePartitionedSchemaObject; }
            set { thePartitionedSchemaObject = value; }
        }

        /// <summary>
        /// Create a tabpage that contains a panel that shows the partition info for any partitoned object.
        /// </summary>
        /// <param name="aPartitionedSchemaObject">The partitioned object</param>
        public PartitionsTabPage(PartitionedSchemaObject aPartitionedSchemaObject)
            : base("Partitions")
        {
            ThePartitionedSchemaObject = aPartitionedSchemaObject;
        }

        override protected void Populate()
        {
            Controls.Clear();
            //PartitionsPanel thePartitionsPanel = new PartitionsPanel(ThePartitionedSchemaObject);
            //thePartitionsPanel.Dock = DockStyle.Fill;
            //Controls.Add(thePartitionsPanel);
            PartitionsUserControl thePartitionsUserControl = new PartitionsUserControl(ThePartitionedSchemaObject);
            thePartitionsUserControl.Dock = DockStyle.Fill;
            Controls.Add(thePartitionsUserControl);
        }

    }

}
