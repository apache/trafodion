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
    /// The tab page that shows privileges on a schema.
    /// </summary>
    public class SchemaPrivilegesTabPage : DelayedPopulateTabPage, IHasTrafodionSchema
    {
        public TrafodionPrivilegeControl _privilegeControl = new TrafodionPrivilegeControl(new SchemaPrivilegeDataGridView());
        private TrafodionSchema _schema;

        #region Properties

        /// <summary>
        /// The schema that this tab is displaying.
        /// </summary>
        public TrafodionSchema TheTrafodionSchema
        {
            get { return _schema; }
            set
            {
                _schema = value;
                _privilegeControl.TrafodionObject = _schema;
            }
        }

        #endregion

        /// <summary>
        /// Creates a new tab page to show schema privileges.
        /// </summary>
        /// <param name="schema">The schema whose privileges will be displayed.</param>
        public SchemaPrivilegesTabPage(TrafodionSchema schema)
            : base(Properties.Resources.Privileges)
        {
            _schema = schema;

        }

        public override void PrepareForPopulate()
        {
            //load privileges
            object priv = _schema.Privileges;
        }

        /// <summary>
        /// Adds the tab's datagrid to the tab.
        /// </summary>
        override protected void Populate()
        {
              if (_schema.Version < 2000)
            {
                Label msg = new Label();
                msg.Text = Properties.Resources.Error_PrivilegeIsNotSupportedForSchemaVersion;
                msg.Dock = DockStyle.Fill;
                Controls.Clear();
                Controls.Add(msg);
            }
            else
            {
            _privilegeControl.Dock = DockStyle.Fill;
            Controls.Add(_privilegeControl);          
                _privilegeControl.TrafodionObject = _schema;
            }
        }
    }
}
