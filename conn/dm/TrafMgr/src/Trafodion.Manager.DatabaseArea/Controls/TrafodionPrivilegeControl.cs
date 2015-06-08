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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class TrafodionPrivilegeControl : UserControl, ICloneToWindow
    {
        private TrafodionObject _sqlMxObject;
        private DatabaseAreaObjectsDataGridView _dataGridView;

        public TrafodionObject TrafodionObject
        {
          get { return _sqlMxObject; }
          set 
          {
              if (_sqlMxObject != null)
              {
                  _sqlMxObject.ModelChangedEvent -= _sqlMxObject_ModelChangedEvent;
              }
              _sqlMxObject = value;
              if (_sqlMxObject != null)
              {
                  _sqlMxObject.ModelChangedEvent += _sqlMxObject_ModelChangedEvent;
                  RefreshGrid();
              }
          }
        }

        void _sqlMxObject_ModelChangedEvent(object sender, TrafodionObject.TrafodionModelChangeEventArgs e)
        {
            if (e.EventId == TrafodionObject.ChangeEvent.PrivilegeGranted || e.EventId == TrafodionObject.ChangeEvent.PrivilegeRevoked)
            {
                RefreshGrid();
            }
        }

        public TrafodionPrivilegeControl(DatabaseAreaObjectsDataGridView aDataGridView)
        {
            InitializeComponent();
            _dataGridView = aDataGridView;
            _dataGridView.Dock = DockStyle.Fill;
            dataGridPanel.Controls.Add(_dataGridView);
            Control buttonsControl = _dataGridView.GetButtonControl();
            buttonsControl.Dock = DockStyle.Bottom;
            buttonsPanel.Controls.Add(buttonsControl);
        }

        void MyDispose()
        {
            if (_sqlMxObject != null)
            {
                _sqlMxObject.ModelChangedEvent -= _sqlMxObject_ModelChangedEvent;
            }
        }

        void RefreshGrid()
        {
              if (_dataGridView != null)
              {
                  if (_dataGridView is SchemaObjectPrivilegeDataGridView)
                  {
                      ((SchemaObjectPrivilegeDataGridView)_dataGridView).SchemaObject = (TrafodionSchemaObject)_sqlMxObject;
                  }
                  else if (_dataGridView is SchemaPrivilegeDataGridView)
                  {
                      ((SchemaPrivilegeDataGridView)_dataGridView).Schema = (TrafodionSchema)_sqlMxObject;
                  }
              }
        }

        public TrafodionPanel GridPanel
        {
            get { return dataGridPanel; }
        }

        public TrafodionPanel ButtonsPanel
        {
            get { return buttonsPanel; }
        }

        private void grantRevokeButton_Click(object sender, EventArgs e)
        {
            if(_sqlMxObject != null)
            {
                GrantRevokeControl grd = new GrantRevokeControl(_sqlMxObject);
                Utilities.LaunchManagedWindow(Properties.Resources.GrantRevokeDialogTitle, grd, _sqlMxObject.ConnectionDefinition, grd.Size, true);
            }
        }

        #region ICloneToWindow Members

        public Control Clone()
        {
            TrafodionPrivilegeControl privilegeControl = new TrafodionPrivilegeControl(Utilities.CloneDataGridView(_dataGridView));
            privilegeControl.TrafodionObject = _sqlMxObject;
            return privilegeControl;
        }

        public string WindowTitle
        {
            get { return _sqlMxObject.VisibleAnsiName + " " + Properties.Resources.Privileges; }
        }

        public Trafodion.Manager.Framework.Connections.ConnectionDefinition ConnectionDefn
        {
            get { return _sqlMxObject.ConnectionDefinition; }
        }

        #endregion
    }
}
