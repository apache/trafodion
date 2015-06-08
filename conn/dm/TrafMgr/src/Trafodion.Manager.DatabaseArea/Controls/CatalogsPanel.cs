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
    public class CatalogsPanel : TrafodionPanel, ICloneToWindow
    {
        DatabaseObjectsControl _theDatabaseObjectsControl;
        TrafodionSystem _theTrafodionSystem;

        public CatalogsPanel(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionSystem aTrafodionSystem)
        {
            _theDatabaseObjectsControl = aDatabaseObjectsControl;
            _theTrafodionSystem = aTrafodionSystem;

            CatalogsDataGridView theCatalogsDataGridView = new CatalogsDataGridView(aDatabaseObjectsControl);
                       
            theCatalogsDataGridView.Load(TheTrafodionSystem);
            theCatalogsDataGridView.Dock = DockStyle.Fill;

            Controls.Add(theCatalogsDataGridView);

            string theFormat = "This system has {0} catalogs";
            theCatalogsDataGridView.AddCountControlToParent(theFormat, DockStyle.Top);
            theCatalogsDataGridView.AddButtonControlToParent(DockStyle.Bottom);

        }

        public TrafodionSystem TheTrafodionSystem
        {
            get { return _theTrafodionSystem; }
            set { _theTrafodionSystem = value; }
        }

        #region ICloneToWindow Members

        public Control Clone()
        {
            return new CatalogsPanel(null, TheTrafodionSystem); 
        }

        public string WindowTitle
        {
            get { return _theTrafodionSystem.ConnectionDefinition.Name + " " + Properties.Resources.Catalogs; }
        }

        /// <summary>
        /// Stores Connection Definition Property for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return TheTrafodionSystem.ConnectionDefinition; }
        }



        #endregion
    }
}
