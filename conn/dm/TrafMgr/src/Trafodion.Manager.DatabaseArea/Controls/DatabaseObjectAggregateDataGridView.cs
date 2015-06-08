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
using System.Data.Odbc;
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;
using System.ComponentModel;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A generic datagridview to display the aggregate sizes for Tables, Table Indexes,
    /// MVs and MV Indexes
    /// </summary>
    public class DatabaseObjectAggregateDataGridView : TrafodionDataGridView  
    {
        SqlMxSchemaObjectsSummary _theSqlMxSchemaObjectsSummary = null;
        DatabaseObjectAggregateControl _theDatabaseObjectAggregateControl = null;
        DataTable _theDataTable;
        private System.ComponentModel.BackgroundWorker _backgroundWorker;
        private bool cancelledQuery = false;

        /// <summary>
        /// The DatabaseObjectAggregateControl that will house this widget
        /// </summary>
        public DatabaseObjectAggregateControl TheDatabaseObjectAggregateControl
        {
            get { return _theDatabaseObjectAggregateControl; }
            set 
            { 
                _theDatabaseObjectAggregateControl = value;
                _theDatabaseObjectAggregateControl.Disposed += new System.EventHandler(_theDatabaseObjectAggregateControl_Disposed);
            }
        }

        void _theDatabaseObjectAggregateControl_Disposed(object sender, System.EventArgs e)
        {
            this.CancelAsync();
        }

        /// <summary>
        /// The Datatable that will have the data for the view
        /// </summary>
        public DataTable TheDataTable
        {
            get { return _theDataTable; }
            set { _theDataTable = value; }
        }


        internal SqlMxSchemaObjectsSummary TheSqlMxSchemaObjectsSummary
        {
            get { return _theSqlMxSchemaObjectsSummary; }
            set { _theSqlMxSchemaObjectsSummary = value; }
        }

        /// <summary>
        /// Default constructor for the UI designer
        /// </summary>
        public DatabaseObjectAggregateDataGridView()
            : base()
        {
        }

        /// <summary>
        /// Create a generic Datagridview to display the list of Schema Objects
        /// </summary>
        /// <param name="aParentSqlMxObject">The parent sql object in whose context, we are displaying this list</param>
        /// <param name="aSqlMxObjectList">The list of SqlMxSchemaObjects that need to be displayed</param>
        public DatabaseObjectAggregateDataGridView(
            SqlMxSystem aSqlMxSystem,
            SqlMxCatalog aSqlMxCatalog,
            SqlMxSchema aSqlMxSchema)
        {
            InitializeBackgoundWorker();
            _theSqlMxSchemaObjectsSummary = new SqlMxSchemaObjectsSummary(aSqlMxSystem, aSqlMxCatalog, aSqlMxSchema);

        }

        /// <summary>
        /// Method to make the async call to load the data
        /// </summary>
        public void DoLoad()
        {
            if (!_backgroundWorker.IsBusy)
            {
                Columns.Clear();
                _theDatabaseObjectAggregateControl.ShowProgress(true);
                _backgroundWorker.RunWorkerAsync();
            }
        }

        /// <summary>
        /// Load the datagridview with the Sql Object's data
        /// </summary>
        /// <returns></returns>
        private void Load()
        {
            Columns.Clear();
            TheDataTable = TheSqlMxSchemaObjectsSummary.getObjectAggregate();
        }


        /// <summary>
        /// Set up the BackgroundWorker object by attaching event handlers. 
        /// </summary>
        private void InitializeBackgoundWorker()
        {

            _backgroundWorker = new System.ComponentModel.BackgroundWorker();
            _backgroundWorker.WorkerReportsProgress = true;
            _backgroundWorker.WorkerSupportsCancellation = true;
            _backgroundWorker.DoWork += new DoWorkEventHandler(BackgroundWorker_DoWork);
            _backgroundWorker.RunWorkerCompleted +=
                new RunWorkerCompletedEventHandler(BackgroundWorker_RunWorkerCompleted);
            _backgroundWorker.ProgressChanged +=
                new ProgressChangedEventHandler(BackgroundWorker_ProgressChanged);
        }


        //Cancel the running background thread
        public void CancelAsync()
        {
            if ((this._backgroundWorker != null) && (this._backgroundWorker.IsBusy))
            {
                cancelledQuery = true;
                this._backgroundWorker.CancelAsync();
                if (this.TheSqlMxSchemaObjectsSummary.TheQuery != null)
                {
                    this.TheSqlMxSchemaObjectsSummary.TheQuery.Cancel();
                }
            }
        }

        /// <summary>
        /// This event handler is where the actual,
        /// potentially time-consuming DDL work is done.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_DoWork(object sender,
            DoWorkEventArgs e)
        {
            // Get the BackgroundWorker that raised this event.
            BackgroundWorker worker = sender as BackgroundWorker;

            // Assign the result of the computation
            // to the Result property of the DoWorkEventArgs
            // object. This is will be available to the 
            // RunWorkerCompleted eventhandler.            
             Load();
        }

        /// <summary>
        /// This event handler deals with the results of the
        /// background operation.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_RunWorkerCompleted(
            object sender, RunWorkerCompletedEventArgs e)
        {
            _theDatabaseObjectAggregateControl.ShowProgress(false);
            // First, handle the case where an exception was thrown.
            if (e.Error != null)
            {
                if (!cancelledQuery)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), e.Error.Message, Properties.Resources.Error, MessageBoxButtons.OK);
                }
            }
            else if (e.Cancelled)
            {
                // Next, handle the case where the user canceled 
                // the operation.
                // Note that due to a race condition in 
                // the DoWork event handler, the Cancelled
                // flag may not have been set, even though
                // CancelAsync was called.
            }
            else
            {
                // Finally, handle the case where the operation 
                // succeeded.
                if (TheDataTable != null)
                {
                    this.DataSource = TheDataTable;
                    //this.DataSource = dataTable;
                    long totalSize = 0;
                    for (int i = 0; i < TheDataTable.Rows.Count; i++)
                    {
                        if (TheDataTable.Rows[i].ItemArray[1] is SizeObject)
                        {
                            totalSize += ((SizeObject)TheDataTable.Rows[i].ItemArray[1]).TheValue;
                        }
                    }

                    TheDatabaseObjectAggregateControl.OnPopulateComplete(totalSize);
                }
            }
        }
        /// <summary>
        /// This event handler updates the progress bar and appends the DDL text to the output textbox
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>

        private void BackgroundWorker_ProgressChanged(object sender,
            ProgressChangedEventArgs e)
        {
        }

    }
}
