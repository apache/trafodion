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

namespace Trafodion.Manager.DatabaseArea.Controls
{

    public class CatalogAttributesTabPage : Trafodion.Manager.Framework.Controls.DelayedPopulateTabPage, IHasTrafodionCatalog
    {

        private TrafodionCatalog _sqlMxCatalog;
        private DatabaseObjectsControl _databaseObjectsControl;


        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aDatabaseObjectsControl A Database object control or null"></param>
        /// <param name="aTrafodionCatalog A Trafodion catalog object"></param>
        public CatalogAttributesTabPage(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionCatalog aTrafodionCatalog)
            : base(Properties.Resources.Attributes)
        {
            TheDatabaseObjectsControl = aDatabaseObjectsControl;
            _sqlMxCatalog = aTrafodionCatalog;
            
            //TODO: Implementation of context sensitive help still needs work 
            //HelpProviderTrafodionTabPage.SetHelpKeyword(this, Properties.Resources.CatalogAttributesHelp);
                    
        }

        /// <summary>
        /// Populate the grid
        /// </summary>
        override protected void Populate()
        {
            Controls.Clear();        
            CatalogAttributesDataGridView theDataGridView = new CatalogAttributesDataGridView(TheDatabaseObjectsControl, TheTrafodionCatalog);
            theDataGridView.Dock = DockStyle.Fill;
            Controls.Add(theDataGridView);
        }
        /// <summary>
        /// Get or Set the SQLMXCatalog model
        /// </summary>
        public TrafodionCatalog TheTrafodionCatalog
        {
            get { return _sqlMxCatalog; }
        }

        /// <summary>
        /// get or set the TheDatabaseObjectsControl
        /// </summary>
        public DatabaseObjectsControl TheDatabaseObjectsControl
        {
            get { return _databaseObjectsControl; }
            set { _databaseObjectsControl = value; }
        }

        //TODO: Implementation of context sensitive help still needs work
        ///// <summary>
        ///// Set the help htm file accordingly.  In this case set it to the catalogAttributes.htm file
        ///// </summary>
        //override public string HelpTopic
        //{
        //    get { return Properties.Resources.CatalogAttributesHelp; }
        //}
       

        

    }

}
