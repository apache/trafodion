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
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.MetricMiner.Controls
{
    /// <summary>
    /// Metric Miner: To display query execution result
    /// </summary>
    public partial class TextBoxDataDisplayControl : UserControl, IDataDisplayControl
    {
        #region Fields

        private UniversalWidgetConfig _theConfig;
        private DataTable _theDataTable;
        private DataProvider _theDataProvider = null;
        private IDataDisplayHandler _theDataDisplayHandler;

        #endregion Fields

        #region Constructors

        public TextBoxDataDisplayControl()
        {
            InitializeComponent();

            // To force controls and handles creation; this paves the way for invoke to work all the time.
            this.CreateControl();
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

        #endregion Properties

        #region Public methods

        /// <summary>
        /// To persist if any thing needed
        /// </summary>
        public void PersistConfiguration()
        {
            //do nothing
        }

        #endregion Public methods

        #region Private methods

        private void AddHandlers()
        {
            if (_theDataProvider != null)
            {
                //Associate the event handlers
                _theDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
            }
        }

        private void RemoveHandlers()
        {
            if (_theDataProvider != null)
            {
                //Remove the event handlers
                _theDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
            }
        }

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
        /// Re-draws the graph in the UI
        /// </summary>
        protected virtual void Populate()
        {
            DatabaseDataProvider dbDataProvider = _theDataProvider as DatabaseDataProvider;
            if ((dbDataProvider != null) && (!dbDataProvider.ExplainMode))
            {
                _theTextBox.Text = "";
                _theDataTable = _theDataProvider.GetDataTable();
                try
                {
                    //We will set the value of the first cell of the table in the text box
                    if (_theDataTable.Rows.Count > 0)
                    {
                        _theTextBox.Text = _theDataTable.Rows[0][0].ToString();
                    }

                }
                catch (Exception ex)
                {
                    MessageBox.Show("Error encountered while populating value - " + ex.Message, "Error in displaying data", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    Trafodion.Manager.Framework.Logger.OutputErrorLog(ex.StackTrace);
                }
            }
        }

        #endregion Private methods
    }
}
