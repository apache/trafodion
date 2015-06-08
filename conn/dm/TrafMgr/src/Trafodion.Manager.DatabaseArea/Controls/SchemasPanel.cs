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
using System.Collections.Generic;
using System.Text;
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Controls 
{
    public class SchemasPanel : TrafodionPanel, ICloneToWindow
    {
        DatabaseObjectsControl _theDatabaseObjectsControl;
        TrafodionCatalog _theTrafodionCatalog;

        

        public SchemasPanel(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionCatalog aTrafodionCatalog)
        {
            _theDatabaseObjectsControl = aDatabaseObjectsControl;

            _theTrafodionCatalog = aTrafodionCatalog;

            SchemasDataGridView theSchemasDataGridView = new SchemasDataGridView(aDatabaseObjectsControl);
            theSchemasDataGridView.Load(aTrafodionCatalog);
            theSchemasDataGridView.Dock = DockStyle.Fill;

            Controls.Add(theSchemasDataGridView);

            theSchemasDataGridView.AddCountControlToParent(Properties.Resources.CatalogNumSchemas, DockStyle.Top);
            theSchemasDataGridView.AddButtonControlToParent(DockStyle.Bottom);
                     
            
        }



        public DatabaseObjectsControl TheDatabaseObjectsControl
        {
            get { return _theDatabaseObjectsControl; }
            
        }

        public TrafodionCatalog TheTrafodionCatalog
        {
            get { return _theTrafodionCatalog; }
           
        }

        #region ICloneToWindow Members

        public Control Clone()
        {
            return new SchemasPanel(null, TheTrafodionCatalog);
        }

        public string WindowTitle
        {
           get { return TheTrafodionCatalog.VisibleAnsiName + " " + Properties.Resources.Schemas; }
        }

        /// <summary>
        /// Stores Connection Definition Property for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return TheTrafodionCatalog.ConnectionDefinition; }
        }


        #endregion
    }
}
