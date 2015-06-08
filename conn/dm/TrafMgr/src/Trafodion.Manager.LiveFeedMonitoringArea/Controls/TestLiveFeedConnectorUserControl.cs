#region Copyright info
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
#endregion Copyright info

using System;
using System.Data;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.LiveFeedMonitoringArea.Controls
{
    /// <summary>
    /// Monitor LiveFeed connector
    /// </summary>
    public partial class TestLiveFeedConnectorUserControl : UserControl, IDataDisplayControl
    {
        #region Fields

        private DataProvider _dataProvider = null;
        private UniversalWidgetConfig _widgetConfig = null;
        private IDataDisplayHandler _dataDisplayHandler = null;
        private LiveFeedConnection _LiveFeedConnection = null;
        private bool _started = false;
        private DataTable _pubDatatable = null;
        private Color _origColor;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: the LiveFeed Connection 
        /// </summary>
        public LiveFeedConnection LiveFeedConnection
        {
            get { return _LiveFeedConnection; }
        }

        /// <summary>
        /// Property: the last recevied publication
        /// </summary>
        public string LastReceivedStats
        {
            get { return _lastReceivedStats.Text; }
            set 
            { 
                _lastReceivedStats.Text = value;
                //if (null != value)
                //{
                //    TallyPublicationCounter(value);
                //}
            }
        }

        /// <summary>
        /// Property: The number of packages received since the start
        /// </summary>
        public string GoodCounterLabel
        {
            get { return _receivedCounter.Text; }
            set { _receivedCounter.Text = value; }
        }

        /// <summary>
        /// Property: The timestamp of the last recevied packet
        /// </summary>
        public DateTime ReceivedTime
        {
            set { _receivedTime.Text = string.Format(Properties.Resources.ReceivedTime, value); }
        }

        /// <summary>
        /// Property: The connector's current status.
        /// </summary>
        public string ConnectorStatus
        {
            set { _connectorStatus.Text = string.Format(Properties.Resources.ConnectorStatus, value); }
        }

        /// <summary>
        /// Property: Monitor started?
        /// </summary>
        public bool Started
        {
            get { return _started; }
        }

        /// <summary>
        /// Proerty: the Data provider
        /// </summary>
        public DataProvider DataProvider
        {
            get { return _dataProvider; }
            set { _dataProvider = value; }
        }

        /// <summary>
        /// Property: the configuration 
        /// </summary>
        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get { return _widgetConfig; }
            set { _widgetConfig = value; }
        }

        /// <summary>
        /// Property: the data display handler
        /// </summary>
        public IDataDisplayHandler DataDisplayHandler
        {
            get { return _dataDisplayHandler; }
            set { _dataDisplayHandler = value; }
        }

        /// <summary>
        /// Property: Drill down manager
        /// </summary>
        public DrillDownManager DrillDownManager
        {
            get;
            set;
        }

        /// <summary>
        /// Property: The grid for displaying all publications
        /// </summary>
        private TrafodionIGrid Grid
        {
            get { return _publicationGrid; }
        }

        #endregion Fields

        #region Constructors

        /// <summary>
        /// Constructor for creating a new object
        /// </summary>
        /// <param name="anLiveFeedConnection"></param>
        public TestLiveFeedConnectorUserControl(LiveFeedConnection anLiveFeedConnection)
        {
            _LiveFeedConnection = anLiveFeedConnection;
            InitializeComponent();
            ShowWidgets();
        }

        /// <summary>
        /// Constructor for cloning
        /// </summary>
        /// <param name="aMonitorLiveFeedConnectorUserControl"></param>
        public TestLiveFeedConnectorUserControl(TestLiveFeedConnectorUserControl aTestLiveFeedConnectorUserControl)
        {
            _LiveFeedConnection = aTestLiveFeedConnectorUserControl.LiveFeedConnection;
            InitializeComponent();
            ShowWidgets(aTestLiveFeedConnectorUserControl.Grid);
            _started = aTestLiveFeedConnectorUserControl.Started;
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Persist configuration data if there is any
        /// </summary>
        public void PersistConfiguration()
        {
        }

        /// <summary>
        /// To start monitor
        /// </summary>
        public void Start()
        {
            _started = true;
            Reset();
        }

        /// <summary>
        /// To stop monitor
        /// </summary>
        public void Stop()
        {
            _started = false;
            Reset();
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Reset the monitor
        /// </summary>
        private void Reset()
        {
            ConnectorStatus = _LiveFeedConnection.CurrentState.ToString();
            LastReceivedStats = null;
            GoodCounterLabel = string.Format(Properties.Resources.ReceivedCounter, 0);

            foreach (iGRow row in _publicationGrid.Rows)
            {
                row.Cells["theTotalCountColumn"].Value = 0;
            }
        }

        /// <summary>
        /// To show widget for brand new
        /// </summary>
        private void ShowWidgets()
        {
            foreach (string publication in _LiveFeedConnection.AllPublications)
            {
                iGRow row = _publicationGrid.Rows.Add();
                row.Cells["thePublicationColumn"].Value = publication;
                row.Cells["theTotalCountColumn"].Value = 0;
            }

            _origColor = _publicationGrid.Rows[0].BackColor;
        }

        /// <summary>
        /// To show widgets for cloning
        /// </summary>
        /// <param name="aGrid"></param>
        private void ShowWidgets(TrafodionIGrid aGrid)
        {
            this.LastReceivedStats = _LiveFeedConnection.LastReceivedPacket.Publication;
            this.ReceivedTime = _LiveFeedConnection.LastReceivedPacket.Timestamp;
            this.GoodCounterLabel = string.Format(Properties.Resources.ReceivedCounter, _LiveFeedConnection.TotalReceivedCount);
            this.ConnectorStatus = _LiveFeedConnection.CurrentState.ToString();

            foreach (iGRow row in aGrid.Rows)
            {
                iGRow newRow = this._publicationGrid.Rows.Add();
                newRow.Cells["thePublicationColumn"].Value = row.Cells["thePublicationColumn"].Value;
                newRow.Cells["theTotalCountColumn"].Value = row.Cells["theTotalCountColumn"].Value;
            }
           
            _origColor = _publicationGrid.Rows[0].BackColor;
        }

        /// <summary>
        /// Update the counters
        /// </summary>
        /// <param name="publication"></param>
        public void TallyPublicationCounter(string publication)
        {
            foreach (iGRow row in _publicationGrid.Rows)
            {
                string name = row.Cells["thePublicationColumn"].Value as string;
                if (name.Equals(publication))
                {
                    row.Cells["theTotalCountColumn"].Value = ((int)row.Cells["theTotalCountColumn"].Value) + 1;
                    row.BackColor = SystemColors.Highlight;
                }
                else
                {
                    row.BackColor = _origColor;
                }
            }
        }

        #endregion Private methods

    }
}
