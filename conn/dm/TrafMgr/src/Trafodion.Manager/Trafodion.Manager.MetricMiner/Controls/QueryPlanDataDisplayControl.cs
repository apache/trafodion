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
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.DatabaseArea.NCC;
using Trafodion.Manager.DatabaseArea.Queries.Controls;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.MetricMiner.Controls
{
    /// <summary>
    /// User control for displaying query plan in Metric Minor
    /// </summary>
    public partial class QueryPlanDataDisplayControl : UserControl, IDataDisplayControl
    {
        #region Fields

        private UniversalWidgetConfig _theConfig;
        private QueryPlanControl _queryPlanControl;
        private DataProvider _theDataProvider = null;
        private IDataDisplayHandler _theDataDisplayHandler;

        //explain display options
        private bool enableExplainColorProcessBoundaries = true;
        private bool enableSortExplainGirdByLevels = true;

        private bool isActiveTabPage = false;
        private bool planIsDirty = false;

        #endregion Fields

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        public QueryPlanDataDisplayControl()
        {
            InitializeComponent();
            _queryPlanControl = new QueryPlanControl();
            _queryPlanControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theQueryPlanContainer.Controls.Add(_queryPlanControl);

            // Froce control/handler & all child controls/handles creation.
            // Thus, enable to call "Invoke" method easily. 
            this.CreateControl();

            enableExplainColorProcessBoundaries = DatabaseAreaOptions.GetOptions().EnableExplainColorProcessBoundaries;
            enableSortExplainGirdByLevels = DatabaseAreaOptions.GetOptions().EnableSortExplainGirdByLevels;
            DatabaseAreaOptions.GetOptions().DatabaseOptionsChanged += new EventHandler(QueryPlanDataDisplayControl_DatabaseOptionsChanged);
        }
        #endregion Constructors

        #region Properties

        /// <summary>
        /// Property: DataProvider
        /// </summary>
        public DataProvider DataProvider
        {
            get
            {
                return _theDataProvider;
            }
            set
            {
                if (_theDataProvider != null)
                {
                    RemoveHandlers();
                }
                _theDataProvider = value;
                AddHandlers();
            }
        }

        /// <summary>
        /// Property: UniversalWidgetConfiguration
        /// </summary>
        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get{ return _theConfig;}
            set{_theConfig = value;}
        }

        /// <summary>
        /// Property: DataDisplayHandler
        /// </summary>
        public IDataDisplayHandler DataDisplayHandler
        {
            get { return _theDataDisplayHandler; }
            set { _theDataDisplayHandler = value; }
        }

        /// <summary>
        /// Property: DrillDownManager
        /// </summary>
        public DrillDownManager DrillDownManager
        {
            get { return null; }
            set { }
        }

        public bool IsActiveTabPage
        {
            set
            {
                isActiveTabPage = value;
            }
        }

        #endregion Properties 

        #region Public methods

        void QueryPlanDataDisplayControl_DatabaseOptionsChanged(object sender, EventArgs e)
        {
            //if the explain options are changted then we need to re draw this graph.
            if (enableExplainColorProcessBoundaries != DatabaseAreaOptions.GetOptions().EnableExplainColorProcessBoundaries ||
                    enableSortExplainGirdByLevels != DatabaseAreaOptions.GetOptions().EnableSortExplainGirdByLevels)
            {
                if (isActiveTabPage)
                {
                    this.Populate();
                    planIsDirty = false;
                }
                else
                {
                    planIsDirty = true;
                }
            }

            enableExplainColorProcessBoundaries = DatabaseAreaOptions.GetOptions().EnableExplainColorProcessBoundaries;
            enableSortExplainGirdByLevels = DatabaseAreaOptions.GetOptions().EnableSortExplainGirdByLevels;

        }

        public void ReloadQueryPlan()
        {
            if (planIsDirty)
            {
                Populate();
            }
        }
        /// <summary>
        /// To persist if any 
        /// </summary>
        public void PersistConfiguration()
        {
            //do nothing
        }

        /// <summary>
        /// To load the query data directly
        /// </summary>
        /// <param name="wqd"></param>
        public void LoadQueryData(NCCWorkbenchQueryData wqd)
        {
            if (null == wqd)
            {
                return;
            }

            _queryPlanControl.LoadQueryData(wqd);
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// To add all event handlers
        /// </summary>
        private void AddHandlers()
        {
            if (_theDataProvider != null)
            {
                //Associate the event handlers
                _theDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
            }
        }

        /// <summary>
        /// To remove all event handlers
        /// </summary>
        private void RemoveHandlers()
        {
            if (_theDataProvider != null)
            {
                //Remove the event handlers
                _theDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
            }
        }

        /// <summary>
        /// Event handler for new data arrived event
        /// </summary>
        /// <param name="obj"></param>
        /// <param name="e"></param>
        protected virtual void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                this.Invoke(new MethodInvoker(Populate));
        }
            catch (Exception ex)
            {
                // just re-throw the exception for now
                throw ex;
            }
        }

        /// <summary>
        /// Anything need to be disposed when the object is disposed
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                //Dispose the result set control to free up resources.
                if (_queryPlanControl != null)
                {
                    _queryPlanControl.Dispose();
                    _queryPlanControl = null;
                }

                DatabaseAreaOptions.GetOptions().DatabaseOptionsChanged -= QueryPlanDataDisplayControl_DatabaseOptionsChanged;

            }
        }

        /// <summary>
        /// Re-draws the graph in the UI
        /// </summary>
        protected virtual void Populate()
        {
            DatabaseDataProvider dataProvider = _theDataProvider as DatabaseDataProvider;
            if ((dataProvider != null) && (dataProvider.ExplainMode) && (dataProvider.WorkbenchQueryData != null))
            {
                try
                {
                    _queryPlanControl.LoadQueryData(dataProvider.WorkbenchQueryData);
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Error encountered while populating explain plan - " + ex.Message, "Error in displaying explain plan", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    Trafodion.Manager.Framework.Logger.OutputErrorLog(ex.StackTrace);
                }
            }
        }

        #endregion Private methods
    }
}
