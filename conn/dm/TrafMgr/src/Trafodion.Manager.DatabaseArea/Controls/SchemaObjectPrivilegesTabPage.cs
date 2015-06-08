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
    /// <summary>
    /// The tab page that shows privileges on a schema object.
    /// </summary>
    public class SchemaObjectPrivilegesTabPage : Trafodion.Manager.Framework.Controls.DelayedPopulateClonableTabPage
    {
        public TrafodionPrivilegeControl _privilegeControl = new TrafodionPrivilegeControl(new SchemaObjectPrivilegeDataGridView());
        private TrafodionSchemaObject _schemaObject;

        #region Properties

        /// <summary>
        /// The schema object that this tab is displaying.
        /// </summary>
        public TrafodionSchemaObject SchemaObject
        {
            get { return _schemaObject; }
            set
            {
                _schemaObject = value;
                _privilegeControl.TrafodionObject = _schemaObject;
            }
        }

        #endregion

        /// <summary>
        /// Creates a new tab page to show schema object privileges.
        /// </summary>
        /// <param name="schemaObject">The schema object whose privileges will be displayed.</param>
        public SchemaObjectPrivilegesTabPage(TrafodionSchemaObject schemaObject)
            : base(Properties.Resources.Privileges)
        {
            _schemaObject = schemaObject;
            Controls.Clear();
            _privilegeControl.Dock = DockStyle.Fill;
            Controls.Add(_privilegeControl);
        }

        public override void PrepareForPopulate()
        {
            object a = _schemaObject.Privileges;
            a = _schemaObject.TheTrafodionSchema.Privileges;
            if (_schemaObject is IHasTrafodionColumns)
            {
                a = ((IHasTrafodionColumns)_schemaObject).ColumnPrivileges;
            }
        }

        /// <summary>
        /// Adds the tab's datagrid to the tab.
        /// </summary>
        override protected void Populate()
        {
            Controls.Clear();

            if (_schemaObject.TheTrafodionSchema.Version < 2000)
            {
                Label msg = new Label();
                msg.Text = Properties.Resources.Error_PrivilegeIsNotSupportedForSchemaVersion;
                msg.Dock = DockStyle.Fill;
                Controls.Add(msg);
            }
            else
            {
                _privilegeControl.TrafodionObject = _schemaObject;
                Controls.Add(_privilegeControl);
            }
        }

        private void InitializeComponent()
        {
            this.SuspendLayout();
            this.ResumeLayout(false);

        }
    }
}
