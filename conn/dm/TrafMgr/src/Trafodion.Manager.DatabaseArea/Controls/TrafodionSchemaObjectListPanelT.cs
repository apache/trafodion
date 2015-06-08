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

using System.Collections.Generic;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A panel to display a list of schema objects
    /// THIS VERSION IS USED ONLY BY TRIGGERS, CLUDGE TO ALLOW TRIGGERS TO ADD IT'S COLUMNS TO SCHEMA PANEL DATA GRID
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public class TrafodionSchemaObjectListPanelT<T> : TrafodionPanel, ICloneToWindow where T : TrafodionTriggerUsage
    {
        private DatabaseObjectsControl _databaseObjectsControl;
        private List<T> _sqlMxObjects;
        private TrafodionObject _parentTrafodionObject;
        private string _title;
        private string _headerText;

        /// <summary>
        /// The datagridview that displays the list of schema objects
        /// </summary>
        protected TrafodionSchemaObjectsDataGridViewT<T> _sqlMxSchemaObjectsDataGridView;


        /// <summary>
        /// A reference to the database navigation tree
        /// </summary>
        public DatabaseObjectsControl TheDatabaseObjectsControl
        {
            get { return _databaseObjectsControl; }
        }


        /// <summary>
        /// The list of sql schema objects to display
        /// </summary>
        public List<T> TheTrafodionObjects
        {
            get { return _sqlMxObjects; }
        }


        /// <summary>
        /// A reference to the parent schema
        /// </summary>
        public TrafodionObject TheParentTrafodionObject
        {
            get { return _parentTrafodionObject; }
        }
 

        /// <summary>
        /// The title of the panel
        /// </summary>
        public string TheTitle
        {
            get { return _title; }
        }

        /// <summary>
        /// The header text for the datagridview
        /// </summary>
        public string TheHeaderText
        {
            get { return _headerText; }
        }

  
        /// <summary>
        /// Constructs a generic panel to display a list of TrafodionSchemaObjects
        /// </summary>
        /// <param name="aDatabaseObjectsControl">The control that has reference to the DatabaseTreeView</param>
        /// <param name="aHeaderText">The text to display as the header for the object list</param>
        /// <param name="parentTrafodionObject">The parent sql object in whose context, we are displaying this list</param>
        /// <param name="sqlMxObjects">The list of TrafodionSchemaObjects that need to be displayed</param>
        /// <param name="aTitle"></param>
        public TrafodionSchemaObjectListPanelT(DatabaseObjectsControl aDatabaseObjectsControl, string aHeaderText, TrafodionObject parentTrafodionObject, 
                    List<T> sqlMxObjects, string aTitle)
        {
            _databaseObjectsControl = aDatabaseObjectsControl;
            _headerText = aHeaderText;
            _parentTrafodionObject = parentTrafodionObject;
            _sqlMxObjects = sqlMxObjects;
            _title = aTitle;
            CreateDataGridView();
        }
        /// <summary>
        /// Creates the datagridview that will list the sql schema object list
        /// </summary>
        virtual protected void CreateDataGridView()
        {
            //Construct the generic datagridview to display the TrafodionSchemaObject list
            _sqlMxSchemaObjectsDataGridView = new TrafodionSchemaObjectsDataGridViewT<T>(TheDatabaseObjectsControl, TheParentTrafodionObject, TheTrafodionObjects);
            _sqlMxSchemaObjectsDataGridView.Dock = DockStyle.Fill;
            Controls.Add(_sqlMxSchemaObjectsDataGridView);

            // These will be added to the grid's parent which is us
            _sqlMxSchemaObjectsDataGridView.AddCountControlToParent(TheHeaderText, DockStyle.Top);
            _sqlMxSchemaObjectsDataGridView.AddButtonControlToParent(DockStyle.Bottom);
        }


        #region ICloneToWindow

        /// <summary>
        /// Makes a clone of this panel suitable for inclusion in some container.
        /// </summary>
        /// <returns>The clone control</returns>
        virtual public Control Clone()
        {
            TrafodionSchemaObjectListPanel<T> sqlMxObjectListPanel = new TrafodionSchemaObjectListPanel<T>(null, TheHeaderText, 
                                                                        TheParentTrafodionObject, TheTrafodionObjects, TheTitle);
            return sqlMxObjectListPanel;
        }

        /// <summary>
        /// A string suitable for a window title.
        /// </summary>
        /// <returns>A string</returns>
        virtual public string WindowTitle
        {
            get { return TheParentTrafodionObject.VisibleAnsiName + " " + TheTitle; }
        }

        /// <summary>
        /// Stores Connection Definition Property for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return TheParentTrafodionObject.ConnectionDefinition; }
        }

        #endregion

    }

}
