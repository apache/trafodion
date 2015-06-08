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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
 
namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A panel to display a list of TrafodionSchemaObject
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public class TrafodionUsagePanel<T> : TrafodionPanel, ICloneToWindow where T : TrafodionSchemaObject
    {
        private T _sqlMxSchemaObject;
        protected TrafodionUsageDataGridView<T> _sqlMxUsageDataGridView;
        protected DatabaseObjectsControl _databaseObjectsControl;

        /// <summary>
        /// A reference to the parent schema
        /// </summary>
        public T TrafodionSchemaObject
        {
            get { return _sqlMxSchemaObject; }
        }

        /// <summary>
        /// Constructs a generic panel to display a list of TrafodionSchemaObjects
        /// </summary>
        /// <param name="aDatabaseObjectsControl">The control that has reference to the DatabaseTreeView</param>
        /// <param name="sqlMxSchemaObject">The parent sql object in whose context, we are displaying this list</param>
        public TrafodionUsagePanel(DatabaseObjectsControl aDatabaseObjectsControl, T sqlMxSchemaObject)
        {
            _databaseObjectsControl = aDatabaseObjectsControl;
            _sqlMxSchemaObject = sqlMxSchemaObject;

            //Create the grid
            CreateDataGridView();

            _sqlMxUsageDataGridView.Dock = DockStyle.Fill;
            Controls.Add(_sqlMxUsageDataGridView);

            // These will be added to the grid's parent which is us
            _sqlMxUsageDataGridView.AddCountControlToParent(Properties.Resources.UsageGridHeader, DockStyle.Top);
            _sqlMxUsageDataGridView.AddButtonControlToParent(DockStyle.Bottom);
        }

        /// <summary>
        /// Creates a grid to hold the usage details
        /// </summary>
        public virtual void CreateDataGridView()
        {
            //Construct the generic datagridview to display the TrafodionSchemaObject list
            _sqlMxUsageDataGridView = new TrafodionUsageDataGridView<T>(_databaseObjectsControl, TrafodionSchemaObject);
        }

        /// <summary>
        /// Add the usage objects to the grid
        /// </summary>
        /// <typeparam name="UT">The usage object type</typeparam>
        /// <param name="usageType">The type of the usage object</param>
        /// <param name="usageObjects">List of the usage objects</param>
        public virtual void AddUsageObjects<UT>(string usageType, List<UT> usageObjects) where UT : TrafodionSchemaObject
        {
            _sqlMxUsageDataGridView.AddUsage(usageType, usageObjects);
        }

        #region ICloneToWindow

        /// <summary>
        /// Makes a clone of this panel suitable for inclusion in some container.
        /// </summary>
        /// <returns>The clone control</returns>
        virtual public Control Clone()
        {
            TrafodionUsagePanel<T> theTrafodionUsagePanel = new TrafodionUsagePanel<T>(null, TrafodionSchemaObject);
            return theTrafodionUsagePanel;
        }
        /// <summary>
        /// A string suitable for a window title.
        /// </summary>
        /// <returns>A string</returns>
        virtual public string WindowTitle
        {
            get { return TrafodionSchemaObject.VisibleAnsiName + " " + Properties.Resources.Usage; }
        }

        /// <summary>
        /// Stores Connection Definition Property for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return TrafodionSchemaObject.ConnectionDefinition; }
        }

        #endregion ICloneToWindow

    }

}
