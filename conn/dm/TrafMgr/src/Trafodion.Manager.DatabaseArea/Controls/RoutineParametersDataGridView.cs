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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public class RoutineParametersDataGridView : DatabaseAreaObjectsDataGridView, ICloneToWindow
    {
        TrafodionRoutine _theTrafodionRoutine;

        /// <summary>
        /// Constructor for FunctionParametersDataGridView
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionProcedure"></param>
        public RoutineParametersDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionRoutine aTrafodionRoutine)
            
        {
            _theTrafodionRoutine = aTrafodionRoutine;
            Load();
            this.AllowUserToOrderColumns = false;
        }


        private int Load()
        {
            int rows = 0;

            try
            {
                List<TrafodionColumn> sqlMxColumns = _theTrafodionRoutine.Columns;

                Columns.Clear();
                Rows.Clear();

                Columns.Add("theParamName", Properties.Resources.Name);
                Columns.Add("theParamDirection", Properties.Resources.Direction);
                Columns.Add("theParamSQLType", Properties.Resources.SQLDataType);

                foreach (TrafodionColumn sqlMxColumn in sqlMxColumns)
                {
                    Rows.Add(new object[] {
                                sqlMxColumn, 
                                sqlMxColumn.FormattedDirection(), 
                                sqlMxColumn.FormattedDataType()
                        });

                }

                //Should not change the order of the Columns. So make it not sortable.
                foreach (DataGridViewColumn column in Columns)
                {
                    column.SortMode = DataGridViewColumnSortMode.NotSortable;
                }
                rows = Rows.Count;
            }
            catch (Exception ex)
            {
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                    string.Format(Properties.Resources.ColumnsLoadFailed,
                    new string[] { this._theTrafodionRoutine.ExternalName, "\n\n", ex.Message }),
                    Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            return rows;
        }

        public void Reload(TrafodionRoutine sqlMxRoutine)
        {
            _theTrafodionRoutine = sqlMxRoutine;
            Load();
        }

        public void UpdateRow(int rowIndex, TrafodionColumn column)
        {
            this.Rows[rowIndex].Cells["theParamName"].Value = column;
            this.Rows[rowIndex].Cells["theParamDirection"].Value = column.FormattedDirection();
            this.Rows[rowIndex].Cells["theParamSQLType"].Value = column.FormattedDataType();
            Update();
        }

        #region ICloneToWindow

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
       public Control Clone()
        {
            return new RoutineParametersDataGridView(null, _theTrafodionRoutine);
        }

       /// <summary>
       /// Read only property that supplies a suitable base title for the managed window.
       /// </summary>
       public string WindowTitle { get { return _theTrafodionRoutine.VisibleAnsiName + Properties.Resources.Parameters; } }


        /// <summary>
        /// Stores Connection Definition Property for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theTrafodionRoutine.ConnectionDefinition; }
        }


        #endregion
    }
}
