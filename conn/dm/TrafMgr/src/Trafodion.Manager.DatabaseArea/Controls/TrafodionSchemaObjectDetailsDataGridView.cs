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
using System.ComponentModel;
using Trafodion.Manager.Framework;
using System.Data.Odbc;
using System.Data;
using System.Drawing;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A generic datagridview to display the details for a list of Sql schema Objects.
    /// </summary>
    public class SqlMxSchemaObjectDetailsDataGridView : TrafodionDataGridView
    {
        private SqlMxObject _parentSqlMxObject;
        private NavigationTreeNameFilter theNameFilter = null;
        private SqlMxSchemaObjectsSummary _theSqlMxSchemaObjectsSummary = null;
        private string _theType = null;
        private System.ComponentModel.BackgroundWorker _backgroundWorker;
        DataTable _theDataTable;
        private bool _cancelledQuery = false;
        private DatabaseObjectDetailsControl _theDatabaseObjectDetailsControl;
        private int _PercentFilledThreshold = 80;

        /// <summary>
        /// Datatable that contains the data displayed in the view
        /// </summary>
        public DataTable TheDataTable
        {
            get { return _theDataTable; }
            set { _theDataTable = value; }
        }


        public virtual string ObjectType
        {
            get { return ""; }
        }

        public virtual string ParentObjectType
        {
            get { return "Schema"; }
        }

        /// <summary>
        /// The value above which the row will be marked in red
        /// </summary>
        public virtual int PercentFilledThreshold
        {
            get { return _PercentFilledThreshold; }
            set { _PercentFilledThreshold = value; }
        }


        /// <summary>
        /// The parent schema object
        /// </summary>
        public SqlMxObject TheParentSqlMxObject
        {
            get { return _parentSqlMxObject; }
            private set
            {
                _parentSqlMxObject = value;
                SetupColumns();
            }
        }

        private List<SqlMxObject> _sqlMxObjects;
        /// <summary>
        /// The list of schema objects whose details are displayed in the datagridview
        /// </summary>
        public List<SqlMxObject> TheSqlMxObjects
        {
            get { return _sqlMxObjects; }
        }

        public NavigationTreeNameFilter TheNameFilter
        {
            get { return theNameFilter; }
            set { theNameFilter = value; }
        }

        internal SqlMxSchemaObjectsSummary TheSqlMxSchemaObjectsSummary
        {
            get
            {
                return _theSqlMxSchemaObjectsSummary;
            }
            set
            {
                _theSqlMxSchemaObjectsSummary = value;
            }
        }


        /// <summary>
        /// Default constructor for the UI designer
        /// </summary>
        public SqlMxSchemaObjectDetailsDataGridView()
            : base()
        {
            InitializeBackgoundWorker();
            TheParentSqlMxObject = null;
            this.Sorted += new System.EventHandler(SqlMxSchemaObjectDetailsDataGridView_Sorted);
        }

        void SqlMxSchemaObjectDetailsDataGridView_Sorted(object sender, System.EventArgs e)
        {
            UpdateRowColor();
        }

        /// <summary>
        /// Sets the background row color of the table to red that has the percent column with a 
        /// value greater than the threshold
        /// </summary>
        protected virtual void UpdateRowColor()
        {
            if (this.RowCount > 0)
            {
                int idx = 0;
                int col = (this.Columns[Properties.Resources.PercentAllocated] != null) ? 
                    this.Columns[Properties.Resources.PercentAllocated].Index : -1;
                if (col > 0)
                {
                    foreach (DataGridViewColumn dataCol in this.Columns)
                    {
                        if (dataCol.ValueType.Equals(typeof(long)))
                        {
                            dataCol.DefaultCellStyle.Format = "N0";
                        }
                    }

                    foreach (DataGridViewRow row in this.Rows)
                    {
                        PercentObject val = row.Cells[col].Value as PercentObject;
                        if ((val != null) && (val.ThePercent >= this.PercentFilledThreshold))
                        {
                            this.Rows[idx].DefaultCellStyle.BackColor = Color.Red;
                        }
                        idx++;
                    }
                }
            }
        }

        /// <summary>
        /// Create a generic Datagridview to display the list of Schema Objects
        /// </summary>
        /// <param name="aParentSqlMxObject">The parent sql object in whose context, we are displaying this list</param>
        /// <param name="aSqlMxObjectList">The list of SqlMxSchemaObjects that need to be displayed</param>
        public SqlMxSchemaObjectDetailsDataGridView(DatabaseObjectDetailsControl aDatabaseObjectDetailsControl,
            SqlMxObject aParentSqlMxObject,
            SqlMxSystem aSqlMxSystem,
            SqlMxCatalog aSqlMxCatalog,
            SqlMxSchema aSqlMxSchema)
            : base()
        {
            _theDatabaseObjectDetailsControl = aDatabaseObjectDetailsControl;
            TheSqlMxSchemaObjectsSummary = new SqlMxSchemaObjectsSummary(aSqlMxSystem, aSqlMxCatalog, aSqlMxSchema);
            TheParentSqlMxObject = aParentSqlMxObject;
            //Load();
            aDatabaseObjectDetailsControl.populateGrid(this);
            InitializeBackgoundWorker();
            this.Sorted += new System.EventHandler(SqlMxSchemaObjectDetailsDataGridView_Sorted);

        }


        /// <summary>
        /// Create a generic Datagridview to display the list of Schema Objects
        /// </summary>
        /// <param name="aParentSqlMxObject">The parent sql object in whose context, we are displaying this list</param>
        /// <param name="aSqlMxObjectList">The list of SqlMxSchemaObjects that need to be displayed</param>
        public SqlMxSchemaObjectDetailsDataGridView(DatabaseObjectDetailsControl aDatabaseObjectDetailsControl,
            SqlMxObject aParentSqlMxObject,
            SqlMxSystem aSqlMxSystem,
            SqlMxCatalog aSqlMxCatalog,
            SqlMxSchema aSqlMxSchema,
            string aType)
        {
            _theDatabaseObjectDetailsControl = aDatabaseObjectDetailsControl;
            _theType = aType;
            TheSqlMxSchemaObjectsSummary = new SqlMxSchemaObjectsSummary(aSqlMxSystem, aSqlMxCatalog, aSqlMxSchema);
            TheParentSqlMxObject = aParentSqlMxObject;
            //Load(_theType);
            aDatabaseObjectDetailsControl.populateGrid(this);
            InitializeBackgoundWorker();
            this.Sorted += new System.EventHandler(SqlMxSchemaObjectDetailsDataGridView_Sorted);

        }

        /// <summary>
        /// This method invokes the async load method to load the data in the tables
        /// </summary>
        public void DoLoad()
        {
            if (!_backgroundWorker.IsBusy)
            {
                Columns.Clear();
                _theDatabaseObjectDetailsControl.ShowProgress(true);

                _backgroundWorker.RunWorkerAsync();
            }
        }

        /// <summary>
        /// This method invokes the async load method to load the data in the tables
        /// for the type specified
        /// </summary>
        public void DoLoad(string type)
        {
            if (!_backgroundWorker.IsBusy)
            {
                Columns.Clear();
                _theDatabaseObjectDetailsControl.ShowProgress(true);

                _backgroundWorker.RunWorkerAsync(type);
            }
        }

        /// <summary>
        /// This method invokes the async load method to load the data in the tables
        /// </summary>
        public void Refresh()
        {
            if (_theType == null)
            {
                DoLoad();
            }
            else
            {
                DoLoad(_theType);
            }
        }


        virtual public string ObjectInformation
        {
            get { return string.Format("{0} for {1} {2}", new object[] { ObjectType, ParentObjectType, TheParentSqlMxObject.VisibleAnsiName }); }
        }

        virtual public string WindowTitle
        {
            get { return string.Format("Summary Details for {0} for {1} {2}", new object[] { ObjectType, ParentObjectType, TheParentSqlMxObject.VisibleAnsiName }); }
        }

        virtual public int Load(string aType)
        {
            return 0;
        }
        /// <summary>
        /// Load the datagridview with the Sql Object's data
        /// </summary>
        /// <returns></returns>
        virtual public int Load()
        {
            Rows.Clear();

            foreach (SqlMxSchemaObject sqlMxSchemaObject in TheSqlMxObjects)
            {
                //TODO:ROLEBasedDisplay
                //if (sqlMxSchemaObject.IsMetadataObject)
                //    continue;

                if ((TheNameFilter == null) || TheNameFilter.Matches(sqlMxSchemaObject.VisibleAnsiName))
                {
                    Rows.Add(new object[] {
                            sqlMxSchemaObject, 
                            sqlMxSchemaObject.UID, 
                            sqlMxSchemaObject.FormattedCreateTime(),
                            sqlMxSchemaObject.FormattedRedefTime()
                    });
                }
            }

            return Rows.Count;
        }

        /// <summary>
        /// Call this function if the columns ever need to change.
        /// </summary>
        virtual protected void SetupColumns()
        {
            Columns.Clear();
            Rows.Clear();

            Columns.Add(new DatabaseAreaObjectsDataGridViewLinkColumn("theNameColumn", Properties.Resources.Name));

            Columns.Add("theUIDColumn", Properties.Resources.MetadataUID);
            //Create time and redefinition time apply to all schema objects
            Columns.Add("theCreateTimeColumn", Properties.Resources.CreationTime);
            Columns.Add("theRedefTimeColumn", Properties.Resources.RedefinitionTime);
        }

        /// <summary>
        /// Set up the BackgroundWorker object by attaching event handlers. 
        /// </summary>
        private void InitializeBackgoundWorker()
        {
            //Before do that, first prepare for the case which user has canceled the request.
            _theDatabaseObjectDetailsControl.Disposed += new System.EventHandler(_theDatabaseObjectDetailsControl_Disposed);
            _backgroundWorker = new System.ComponentModel.BackgroundWorker();
            _backgroundWorker.WorkerReportsProgress = true;
            _backgroundWorker.WorkerSupportsCancellation = true;
            _backgroundWorker.DoWork += new DoWorkEventHandler(BackgroundWorker_DoWork);
            _backgroundWorker.RunWorkerCompleted +=
                new RunWorkerCompletedEventHandler(BackgroundWorker_RunWorkerCompleted);
            _backgroundWorker.ProgressChanged +=
                new ProgressChangedEventHandler(BackgroundWorker_ProgressChanged);
        }

        void _theDatabaseObjectDetailsControl_Disposed(object sender, System.EventArgs e)
        {
            this.CancelAsync();
        }

        //Cancel the running background thread
        public void CancelAsync()
        {
            if ((this._backgroundWorker != null) && (this._backgroundWorker.IsBusy))
            {
                _cancelledQuery = true;
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
            if (e.Argument != null)
            {
                Load((string)e.Argument);
            }
            else
            {
                Load();
            }
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
            _theDatabaseObjectDetailsControl.ShowProgress(false);
            // First, handle the case where an exception was thrown.
            if (e.Error != null)
            {
                if (!_cancelledQuery)
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
                this.TheSqlMxSchemaObjectsSummary.TheQuery.Cancel();
            }
            else
            {
                // Finally, handle the case where the operation 
                // succeeded.
                if (TheDataTable != null)
                {
                    this.DataSource = TheDataTable;
                    DataGridViewColumn sortColumn = this.Columns[Properties.Resources.Name];
                    if (sortColumn != null)
                    {
                        this.Sort(sortColumn, ListSortDirection.Ascending);
                    }
                    else
                    {
                        //This should never happen that the "Name" column is not present.
                        //Once the view is sorted, the sort handler will update the row 
                        //color.
                        UpdateRowColor();
                    }
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
