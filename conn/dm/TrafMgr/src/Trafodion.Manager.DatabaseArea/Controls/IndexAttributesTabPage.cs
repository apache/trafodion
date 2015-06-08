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
    /// The tab for an index attribute
    /// </summary>
    public class IndexAttributesTabPage : DelayedPopulateClonableTabPage, IHasTrafodionSchema
    {
        #region private member variables
        
        private TrafodionIndex _sqlMxIndex;

        #endregion

        /// <summary>
        /// Constructor for IndexAttributesTabPage
        /// </summary>
        /// <param name="aTrafodionIndex"></param>
        public IndexAttributesTabPage(TrafodionIndex aTrafodionIndex)
            : base(Properties.Resources.Attributes)
        {
            _sqlMxIndex = aTrafodionIndex;            
        }

        public override void PrepareForPopulate()
        {
            object a = _sqlMxIndex.LoadAttributes();
        }

        /// <summary>
        /// Populate the foreign key pane with information from the table.  
        /// </summary>
        override protected void Populate()
        {
            // Clear the controls, Create the panel and fill this tab page with it.
            Controls.Clear();
            IndexAttributesPanel theIndexAttributesPanel = new IndexAttributesPanel(TheTrafodionIndex);
            theIndexAttributesPanel.Dock = DockStyle.Fill;
            Controls.Add(theIndexAttributesPanel);
        }

        /// <summary>
        /// Get and set the Index
        /// </summary>
        public TrafodionIndex TheTrafodionIndex
        {
            get 
            { 
                return _sqlMxIndex; 
            }
        }

        /// <summary>
        /// Get the Index's Schema
        /// </summary>
        public TrafodionSchema TheTrafodionSchema
        {
            get { return TheTrafodionIndex.TheTrafodionSchema; }
        }




    }

}
