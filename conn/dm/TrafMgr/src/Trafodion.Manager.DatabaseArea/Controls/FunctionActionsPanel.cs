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

using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public class FunctionActionsPanel : Panel, ICloneToWindow
    {
        private TrafodionObject _parentObject;
        private List<TrafodionFunctionAction> _functionActionList = new List<TrafodionFunctionAction>();
        private FunctionActionsDataGridView _datagrid;
        private DatabaseObjectsControl _databaseObjectsControl;
        private string _dataGridHeader;


        public TrafodionObject ParentObject
        {
            get { return _parentObject; }
        }

        public IList<TrafodionFunctionAction> FunctionActionList
        {
            get
            {
                IList<TrafodionFunctionAction> functionActionList = _functionActionList.AsReadOnly();
                return functionActionList;
            }
        }

        public string WindowTitle
        {
            get { return _parentObject.VisibleAnsiName + " " + "test: function action"; }
        }

        public ConnectionDefinition ConnectionDefn
        {
            get { return ParentObject.ConnectionDefinition; }
        }

        public FunctionActionsPanel(DatabaseObjectsControl databaseObjectsControl, TrafodionUDFunction parentObject,
            List<TrafodionFunctionAction> functionActionList, string header)
        {
            _databaseObjectsControl = databaseObjectsControl;
            _parentObject = parentObject;
            _functionActionList.InsertRange(0, functionActionList);
            _dataGridHeader = header;
            CreateDataGridView();
        }

        public FunctionActionsPanel(DatabaseObjectsControl databaseObjectsControl, TrafodionSchema parentObject,
            List<TrafodionFunctionAction> functionActionList, string header)
        {
            _databaseObjectsControl = databaseObjectsControl;
            _parentObject = parentObject;
            _functionActionList.InsertRange(0, functionActionList);
            _dataGridHeader = header;
            CreateDataGridView();
        }

        public Control Clone()
        {
            FunctionActionsPanel panel = null;

            if (_parentObject is TrafodionSchema)
            {
                panel = new FunctionActionsPanel(null, _parentObject as TrafodionSchema, _functionActionList, _dataGridHeader);
            }
            else if (_parentObject is IndexedSchemaObject)
            {
                panel = new FunctionActionsPanel(null, _parentObject as TrafodionUDFunction, _functionActionList, _dataGridHeader);
            }

            return panel;
        }

        private void CreateDataGridView()
        {
            //Construct the generic datagridview to display the TrafodionSchemaObject list
            if (_parentObject is TrafodionUDFunction)
            {
                _datagrid = new FunctionActionsDataGridView(_databaseObjectsControl, _parentObject as TrafodionUDFunction, _functionActionList);
            }
            else if (_parentObject is TrafodionSchema)
            {
                _datagrid = new FunctionActionsDataGridView(_databaseObjectsControl, _parentObject as TrafodionSchema, _functionActionList);
            }
            else
            {
                // This is a coding issue and should be addressed ASAP.
                throw new ApplicationException();
            }

            _datagrid.Dock = DockStyle.Fill;
            Controls.Add(_datagrid);

            // These will be added to the grid's parent which is us
            _datagrid.AddCountControlToParent(_dataGridHeader, DockStyle.Top);
            _datagrid.AddButtonControlToParent(DockStyle.Bottom);
        }
    }
}
