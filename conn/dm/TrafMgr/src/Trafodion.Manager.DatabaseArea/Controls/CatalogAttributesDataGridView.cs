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
    /// Used to load catalog attributes data into a data grid
    /// </summary>
    public class CatalogAttributesDataGridView : TrafodionObjectAttributesDataGridView
    {
        /// <summary>
        /// Holds catalog attributes 
        /// </summary>
        /// <param name="aDatabaseObjectsControl A Database object control or null"></param>
        /// <param name="aTrafodionCatalog A Trafodion catalog object"></param>
        public CatalogAttributesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionCatalog aTrafodionCatalog)
            : base(aDatabaseObjectsControl, aTrafodionCatalog)
        {
            //TODO: Implementation of context sensitive help still needs work                    
            //This is the line that starts help topic 
            //HelpProviderTrafodionDataGrid.SetHelpKeyword(this,  global::Trafodion.Manager.Properties.Resources.CatalogRegistrationsHelp);
            
        }

        //TODO: Implementation of context sensitive help still needs work
        ///// <summary>
        ///// Set the help htm file accordingly.  In this case set it to the catalogAttributes.htm file
        ///// </summary>
        //override public string HelpTopic
        //{
        //    get { return global::Trafodion.Manager.Properties.Resources.CatalogAttributesHelp; }
        //}
        

        

        /// <summary>
        /// Gets or Set SQLMx catalog object value
        /// </summary>
        public TrafodionCatalog TheTrafodionCatalog
        {
            get { return TrafodionObject as TrafodionCatalog; }
            set { TrafodionObject = value; }
        }

        /// <summary>
        /// Put attributes data into data grid.  The rest of the attributes data gets 
        /// displayed to the grid in the base class
        /// </summary>
        override protected void LoadRows()
        {
            AddRow(Properties.Resources.Location, TheTrafodionCatalog.VolumeName);
             
        }

        #region ICloneToWindow

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        override public Control Clone()
        {
            return new CatalogAttributesDataGridView(null, TheTrafodionCatalog);
        }

        #endregion

    }
}
